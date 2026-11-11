#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../core/types/ov_types.h"
#include "../core/platform/ov_platform.h"
#include "../core/platform/ov_platform_adapter.h"
#include "../core/resolver/ov_resolver.h"
#include "../core/pipeline/ov_pipeline.h"
#include "../core/memory/ov_memory.h"
#include "../ovir/gpu/ovir_gpu.h"
#include "../ovir/gpu/ovir_spirv.h"
#include "../ovir/gpu/ovir_gpu_ir.h"
#include "../ovir/gpu/ovir_gpu_opt.h"
#include "../ovir/gpu/ovir_gpu_backend.h"

// Sample vertex shader words from Phase 14
static const uint32_t s_sample_vert_spirv[] = {
    0x07230203, 0x00010300, 0x00080001, 25, 0,
    (2 << 16) | 17, 1,
    (3 << 16) | 14, 0, 0,
    (6 << 16) | 15, 0, 4, 0x6e69616d, 0, 9,
    (3 << 16) | 16, 4, 7,
    (4 << 16) | 71, 9, 30, 0,
    (4 << 16) | 71, 10, 30, 0,
    (2 << 16) | 19, 1,
    (3 << 16) | 33, 2, 1,
    (3 << 16) | 22, 5, 32,
    (4 << 16) | 23, 6, 5, 4,
    (4 << 16) | 32, 7, 1, 6,
    (4 << 16) | 32, 8, 3, 6,
    (4 << 16) | 59, 7, 9, 1,
    (4 << 16) | 59, 8, 10, 3,
    (4 << 16) | 43, 5, 11, 0x3f800000,
    (4 << 16) | 43, 5, 12, 0x3f000000,
    (5 << 16) | 54, 1, 4, 0, 2,
    (2 << 16) | 248, 13,
    (4 << 16) | 61, 6, 14, 9,
    (5 << 16) | 129, 6, 15, 14, 11,
    (3 << 16) | 62, 10, 15,
    (1 << 16) | 253,
    (1 << 16) | 56
};

static void test_platform_initialization_and_discovery(void) {
    printf("[Phase 15 Test 1] Platform Initialization & Device Discovery...\n");

    ov_status_t st = ov_platform_init();
    assert(st == OV_SUCCESS);

    const ov_platform_info_t *info = ov_platform_get_info();
    assert(info != NULL);
    assert(info->init_state == OV_PLATFORM_INIT_READY);
    assert(info->total_ram_mb > 0);
    assert(info->page_size == 4096);
    assert(strlen(info->os_name) > 0);
    assert(strlen(info->os_version) > 0);

    // CPU discovery
    assert(info->cpu.physical_cores > 0);
    assert(info->cpu.logical_threads > 0);
    assert(info->cpu.sse4_1_supported);
    assert(info->cpu.os_vector_enabled);

    // GPU discovery
    assert(info->gpu_device_count > 0);
    assert(strlen(info->gpu_devices[0].device_name) > 0);
    assert(info->gpu_devices[0].supports_opengl_33);

    // Backend registry
    assert(info->backend_count >= 2);
    bool found_gl = false;
    for (uint32_t i = 0; i < info->backend_count; i++) {
        if (info->backends[i].backend_id == OV_GFX_BACKEND_OPENGL_CORE) {
            found_gl = true;
            assert(info->backends[i].is_available);
            assert(info->backends[i].is_initialized);
        }
    }
    assert(found_gl);

    printf("  [PASS] Platform detected: %s (%s), %u CPU cores, %u GPU device(s), %u registered backend(s).\n",
           info->os_name, info->os_version, info->cpu.physical_cores, info->gpu_device_count, info->backend_count);
}

static void test_cross_platform_adapters(void) {
    printf("[Phase 15 Test 2] Multi-Platform Adapters (macOS, Linux, Windows)...\n");

    // 1. Test macOS Adapter (Targeting MacBookPro9,1 Catalina)
    const ov_platform_adapter_t *mac_adapter = ov_adapter_macos_get();
    assert(mac_adapter != NULL);
    assert(mac_adapter->os_type == OV_OS_MACOS);

    ov_platform_info_t mac_info;
    mac_adapter->detect_system(&mac_info);
    assert(strstr(mac_info.os_version, "Catalina") != NULL);

    ov_platform_cpu_caps_t mac_cpu;
    mac_adapter->detect_cpu(&mac_cpu);
    assert(strstr(mac_cpu.model_name, "i7-3615QM") != NULL);
    assert(!mac_cpu.avx2_supported); // Ivy Bridge does not have AVX2
    assert(mac_cpu.avx_supported);

    ov_platform_gpu_device_t mac_gpus[4];
    uint32_t mac_gpu_count = 0;
    mac_adapter->detect_gpus(mac_gpus, &mac_gpu_count);
    assert(mac_gpu_count == 2); // HD 4000 + GT 650M
    assert(strstr(mac_gpus[0].device_name, "HD Graphics 4000") != NULL);
    assert(strstr(mac_gpus[1].device_name, "GT 650M") != NULL);

    // 2. Test Linux Adapter
    const ov_platform_adapter_t *linux_adapter = ov_adapter_linux_get();
    assert(linux_adapter != NULL);
    assert(linux_adapter->os_type == OV_OS_LINUX);

    ov_platform_info_t linux_info;
    linux_adapter->detect_system(&linux_info);
    assert(linux_info.os_type == OV_OS_LINUX);

    // 3. Test Windows Adapter
    const ov_platform_adapter_t *win_adapter = ov_adapter_windows_get();
    assert(win_adapter != NULL);
    assert(win_adapter->os_type == OV_OS_WINDOWS);

    ov_platform_backend_info_t win_backends[4];
    uint32_t win_bcount = 0;
    win_adapter->register_backends(win_backends, &win_bcount);
    assert(win_bcount >= 2); // OpenGL WGL + D3D11

    printf("  [PASS] Verified macOS (MacBookPro9,1 / Ivy Bridge dual-GPU), Linux, and Windows adapters.\n");
}

static void test_phase14_gpu_pipeline_and_driver_execution(void) {
    printf("[Phase 15 Test 3] Phase 14 SPIR-V -> OVIR-GPU IR -> GLSL 330 -> Platform Driver Execution...\n");

    // 1. SPIR-V parsing (Phase 14)
    ovir_spirv_module_t spv;
    ov_status_t st = ovir_spirv_parse(s_sample_vert_spirv, sizeof(s_sample_vert_spirv)/sizeof(uint32_t), &spv);
    assert(st == OV_SUCCESS);

    // 2. Architecture-Neutral IR (Phase 14)
    ovir_gpu_shader_ir_t ir;
    st = ovir_gpu_ir_from_spirv(s_sample_vert_spirv, sizeof(s_sample_vert_spirv)/sizeof(uint32_t), &ir);
    assert(st == OV_SUCCESS);

    // 3. IR Optimization (Phase 14)
    ovir_gpu_opt_stats_t opt_stats;
    st = ovir_gpu_optimize(&ir, &opt_stats);
    assert(st == OV_SUCCESS);

    // 4. GLSL 330 Core Emission (Phase 14)
    ovir_gpu_compiled_shader_t compiled;
    st = ovir_gpu_translate_to_glsl(&ir, &compiled);
    assert(st == OV_SUCCESS);
    assert(compiled.target_backend == OV_GFX_BACKEND_OPENGL_CORE);
    assert(strstr(compiled.source_code, "#version 330 core") != NULL);

    // 5. Phase 15 Platform Graphics Execution Lifecycle
    ov_platform_shader_execution_t exec;
    st = ov_platform_graphics_execute_glsl(&compiled, &exec);
    assert(st == OV_SUCCESS);
    assert(exec.driver_compiled);
    assert(exec.linked);
    assert(exec.executed);
    assert(exec.lifecycle_state == OV_SHADER_EXEC_STATE_EXECUTED);
    assert(exec.shader_gl_handle > 0);
    assert(exec.program_gl_handle > 0);
    assert(strstr(exec.driver_compile_log, "SUCCESS") != NULL);

    printf("  [PASS] Full 9-stage lifecycle verified (PARSED -> IR -> OPT -> TRANSLATED -> COMPILED -> LINKED -> EXECUTED).\n");
}

static void test_resolver_integration_with_platform_caps(void) {
    printf("[Phase 15 Test 4] OVResolver & Unified Pipeline Platform Integration...\n");

    ov_host_env_t host;
    ov_platform_populate_host_env(&host);

    assert(host.host_cpu_arch == OV_ARCH_X86_64);
    assert(host.host_cpu_cores > 0);
    assert(host.host_ram_mb > 0);
    assert(host.gpu_caps.has_opengl);

    // Plan evaluation for an ARM64 + Vulkan workload
    ov_workload_desc_t workload = {
        .source_cpu_arch = OV_ARCH_ARM64,
        .target_cpu_arch = OV_ARCH_X86_64,
        .source_gfx_api = OV_GFX_API_VULKAN,
        .preferred_gfx_backend = OV_GFX_BACKEND_OPENGL_CORE,
        .required_cpu_features = OV_CPU_FEAT_NEON,
        .required_gpu_features = OV_GPU_FEAT_COMPUTE,
        .ram_requirement_mb = 1024,
        .vram_requirement_mb = 512,
        .required_cpu_cores = 2,
        .code_identity_hash = 0x12345678ULL,
        .shader_identity_hash = 0x87654321ULL,
        .priority = OV_PRIORITY_NORMAL,
        .workload_type = OV_WORKLOAD_MIXED,
        .native_execution_available = false
    };

    ov_execution_plan_t plan;
    ov_status_t st = ov_resolver_create_plan(&workload, &host, NULL, &plan);
    assert(st == OV_SUCCESS);
    assert(plan.is_supported);
    assert(plan.cpu_translation_required);
    assert(plan.gpu_translation_required);
    assert(plan.decision == OV_DECISION_CPU_AND_GPU_TRANSLATION);
    assert(plan.selected_gfx_backend == OV_GFX_BACKEND_OPENGL_CORE ||
           plan.selected_gfx_backend == OV_GFX_BACKEND_METAL_NATIVE ||
           plan.selected_gfx_backend == OV_GFX_BACKEND_VULKAN_MVK ||
           plan.selected_gfx_backend == OV_GFX_BACKEND_OVIR_TRANSLATED);

    printf("  [PASS] Host capabilities correctly fed into OVResolver: Decision=%s, Backend=%s, Reason=%s.\n",
           ov_decision_name(plan.decision), ov_gfx_backend_name(plan.selected_gfx_backend), ov_reason_name(plan.reason_code));
}

static void test_wx_memory_protection(void) {
    printf("[Phase 15 Test 5] Cross-Platform W^X Memory Lifecycle...\n");

    void *rw_ptr = NULL;
    void *rx_ptr = NULL;
    size_t size = 4096;

    ov_status_t st = ov_platform_alloc_wx(size, &rw_ptr, &rx_ptr);
    assert(st == OV_SUCCESS);
    assert(rw_ptr != NULL);

    // Write to memory in RW mode
    uint8_t *byte_ptr = (uint8_t *)rw_ptr;
    byte_ptr[0] = 0x90; // NOP
    byte_ptr[1] = 0xc3; // RET

    // Transition to RX mode
    st = ov_platform_protect_wx(rw_ptr, size, true);
    assert(st == OV_SUCCESS);

    // Free memory
    st = ov_platform_free_wx(rw_ptr, size);
    assert(st == OV_SUCCESS);

    printf("  [PASS] W^X allocated, transitioned (RW -> RX), and freed cleanly.\n");
}

int main(void) {
    printf("====================================================================\n");
    printf("  OPENVINTAGE PHASE 15: CROSS-PLATFORM NATIVE BACKENDS TEST SUITE   \n");
    printf("====================================================================\n");

    test_platform_initialization_and_discovery();
    test_cross_platform_adapters();
    test_phase14_gpu_pipeline_and_driver_execution();
    test_resolver_integration_with_platform_caps();
    test_wx_memory_protection();

    printf("\n>>> [ALL PHASE 15 PLATFORM ASSERTIONS PASSED SUCCESSFULLY] <<<\n");
    return 0;
}
