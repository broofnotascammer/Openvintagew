#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../core/types/ov_types.h"
#include "../core/memory/ov_memory.h"
#include "../core/runtime/ov_runtime.h"
#include "../core/resolver/ov_resolver.h"
#include "../core/pipeline/ov_pipeline.h"
#include "../core/status/ov_status_system.h"
#include "../core/installer/ov_efi_installer.h"
#include "../ovir/gpu/ovir_gpu.h"

static int g_tests_passed = 0;
static int g_tests_total = 0;

#define TEST_ASSERT(cond, msg) do { \
    g_tests_total++; \
    if (cond) { \
        g_tests_passed++; \
        printf("  [PASS] %s\n", msg); \
    } else { \
        printf("  [FAIL] %s (line %d)\n", msg, __LINE__); \
    } \
} while (0)

// 1. Memory mapping, alignment, and basic operations
void test_memory_mapping_and_alignment(void) {
    printf("\n=== Phase 11 Test 1: Memory Mapping & Page Alignment ===\n");
    ov_guest_memory_t mem;
    ov_mem_init(&mem);

    // Unaligned guest address should fail
    ov_status_t st1 = ov_mem_map(&mem, 0x1005, 4096, OV_MEM_READ | OV_MEM_WRITE, "unaligned_base");
    TEST_ASSERT(st1 == OV_ERR_INVALID_PARAM, "Unaligned base address rejected");

    // Unaligned size should fail
    ov_status_t st2 = ov_mem_map(&mem, 0x1000, 3000, OV_MEM_READ | OV_MEM_WRITE, "unaligned_size");
    TEST_ASSERT(st2 == OV_ERR_INVALID_PARAM, "Unaligned size rejected");

    // Properly aligned mapping should succeed
    ov_status_t st3 = ov_mem_map(&mem, 0x10000, 4096 * 4, OV_MEM_READ | OV_MEM_WRITE, "guest_data");
    TEST_ASSERT(st3 == OV_SUCCESS, "Page-aligned 16KB data mapping succeeded");
    TEST_ASSERT(mem.total_mapped_bytes == 4096 * 4, "Total mapped bytes tracked accurately");

    // Overlapping mapping must fail
    ov_status_t st4 = ov_mem_map(&mem, 0x12000, 4096, OV_MEM_READ, "overlap_test");
    TEST_ASSERT(st4 == OV_ERR_INVALID_PARAM, "Overlapping memory mapping safely rejected");

    ov_mem_destroy(&mem);
}

// 2. Strict W^X Security Enforcement
void test_memory_wx_security(void) {
    printf("\n=== Phase 11 Test 2: Strict W^X (Write XOR Execute) Security Gate ===\n");
    ov_guest_memory_t mem;
    ov_mem_init(&mem);

    // Simultaneous Write + Execute at mapping time must be rejected
    ov_status_t st_rwx = ov_mem_map(&mem, 0x20000, 4096, OV_MEM_READ | OV_MEM_WRITE | OV_MEM_EXECUTE, "evil_rwx");
    TEST_ASSERT(st_rwx == OV_ERR_SECURITY_VIOLATION, "Simultaneous W+X rejected with OV_ERR_SECURITY_VIOLATION");
    TEST_ASSERT(mem.execution_violations == 1, "Execution violations counter incremented");

    // Map as writeable first
    ov_status_t st_w = ov_mem_map(&mem, 0x20000, 4096, OV_MEM_READ | OV_MEM_WRITE, "jit_staging");
    TEST_ASSERT(st_w == OV_SUCCESS, "Read-Write mapping succeeded");

    // Attempt to protect as RWX must also be rejected
    ov_status_t st_prot_rwx = ov_mem_protect(&mem, 0x20000, 4096, OV_MEM_READ | OV_MEM_WRITE | OV_MEM_EXECUTE);
    TEST_ASSERT(st_prot_rwx == OV_ERR_SECURITY_VIOLATION, "Protect to RWX rejected");

    // Transition from W to X (proper JIT flow)
    ov_status_t st_prot_rx = ov_mem_protect(&mem, 0x20000, 4096, OV_MEM_READ | OV_MEM_EXECUTE);
    TEST_ASSERT(st_prot_rx == OV_SUCCESS, "Transition to Read-Execute succeeded");

    ov_mem_destroy(&mem);
}

// 3. Checked Read/Write and Bounds Checking
void test_memory_bounds_and_checked_io(void) {
    printf("\n=== Phase 11 Test 3: Checked Memory Access & Bounds Protection ===\n");
    ov_guest_memory_t mem;
    ov_mem_init(&mem);

    ov_mem_map(&mem, 0x30000, 4096, OV_MEM_READ | OV_MEM_WRITE, "ram");

    // Write data to guest memory
    const char *payload = "OpenVintage Phase 11";
    size_t payload_len = strlen(payload) + 1;
    ov_status_t wr_st = ov_mem_write(&mem, 0x30100, payload, payload_len);
    TEST_ASSERT(wr_st == OV_SUCCESS, "Checked write to valid region succeeded");

    // Read back data
    char read_buf[64] = {0};
    ov_status_t rd_st = ov_mem_read(&mem, 0x30100, read_buf, payload_len);
    TEST_ASSERT(rd_st == OV_SUCCESS, "Checked read from valid region succeeded");
    TEST_ASSERT(strcmp(read_buf, payload) == 0, "Read data matches written payload");

    // Out-of-bounds access
    char oob_buf[16];
    ov_status_t oob_st = ov_mem_read(&mem, 0x30FFF, oob_buf, 16);
    TEST_ASSERT(oob_st == OV_ERR_FAULT, "Cross-boundary access triggered OV_ERR_FAULT");
    TEST_ASSERT(mem.memory_faults > 0, "Memory faults counter recorded fault");

    ov_mem_destroy(&mem);
}

// 4. Runtime Context & Translation Block Lifecycle
void test_runtime_context_and_tb_lifecycle(void) {
    printf("\n=== Phase 11 Test 4: Runtime Context & Translation Block Lifecycle ===\n");
    ov_runtime_t rt;
    ov_runtime_init(&rt);

    ov_cpu_context_t ctx;
    ov_cpu_context_init(&ctx, OV_ARCH_ARM64, 1);
    TEST_ASSERT(ctx.arch == OV_ARCH_ARM64, "CPU context architecture initialized");
    TEST_ASSERT(ctx.state == OV_CPU_STATE_IDLE, "CPU state is IDLE");
    TEST_ASSERT(ctx.sp == 0x7FFF0000, "Default stack pointer configured");

    // Create TB
    ov_translation_block_t *tb = ov_runtime_create_tb(&rt, 0x4000, 0x4010, OV_ARCH_ARM64, OV_ARCH_X86_64, 0xABCDEF01ULL);
    TEST_ASSERT(tb != NULL, "Translation block created");
    TEST_ASSERT(tb->state == OV_TB_STATE_CREATED, "TB initial state is CREATED");
    TEST_ASSERT(!tb->is_valid, "TB initially not marked valid");

    // Compile TB with ARM64 instructions: MOV r0, #15; MOV r1, #25; ADD r0, r0, r1; RET
    uint32_t arm64_code[] = {
        0x528001e0, // MOV w0, #15
        0x52800321, // MOV w1, #25
        0x8b010000, // ADD x0, x0, x1
        0xd65f03c0  // RET
    };
    uint32_t code_count = sizeof(arm64_code) / sizeof(arm64_code[0]);

    ov_status_t comp_st = ov_runtime_compile_tb(tb, arm64_code, code_count);
    TEST_ASSERT(comp_st == OV_SUCCESS, "TB compiled successfully");
    TEST_ASSERT(tb->state == OV_TB_STATE_VALID, "TB state transitions to VALID");
    TEST_ASSERT(tb->is_valid, "TB marked is_valid = true");
    TEST_ASSERT(tb->code_size > 0, "Generated machine code size > 0");

    // Lookup TB
    ov_translation_block_t *found_tb = ov_runtime_lookup_tb(&rt, 0x4004, OV_ARCH_ARM64);
    TEST_ASSERT(found_tb == tb, "Lookup by guest PC found matching TB");
}

// 5. Runtime Execution with Memory Gate
void test_runtime_execution_and_gates(void) {
    printf("\n=== Phase 11 Test 5: Runtime Execution & Memory Execution Gate ===\n");
    ov_runtime_t rt;
    ov_runtime_init(&rt);

    ov_guest_memory_t mem;
    ov_mem_init(&mem);

    // Map code region with READ | EXECUTE
    ov_mem_map(&mem, 0x5000, 4096, OV_MEM_READ | OV_MEM_EXECUTE, "code_sec");

    ov_cpu_context_t ctx;
    ov_cpu_context_init(&ctx, OV_ARCH_ARM64, 1);

    // Create and compile TB located inside 0x5000 region
    ov_translation_block_t *tb = ov_runtime_create_tb(&rt, 0x5000, 0x5010, OV_ARCH_ARM64, OV_ARCH_X86_64, 0x5555AAAAULL);
    uint32_t arm64_code[] = {
        0x528000a0, // MOV w0, #5
        0x52800141, // MOV w1, #10
        0x8b010000, // ADD x0, x0, x1
        0xd65f03c0  // RET
    };
    ov_runtime_compile_tb(tb, arm64_code, 4);

    // Execute with valid execution permission
    ov_status_t exec_st = ov_runtime_execute_tb(&rt, tb, &ctx, &mem);
    TEST_ASSERT(exec_st == OV_SUCCESS, "TB executed successfully through execute gate");
    TEST_ASSERT(ctx.gpr[0] == 15, "Result r0 == 15 verified");
    TEST_ASSERT(ctx.instructions_retired == 4, "Instructions retired tracked (4)");
    TEST_ASSERT(tb->execution_count == 1, "TB execution count incremented");

    // Invalidate TB and verify execute gate rejects execution
    tb->state = OV_TB_STATE_INVALIDATED;
    tb->is_valid = false;

    ov_status_t fail_exec = ov_runtime_execute_tb(&rt, tb, &ctx, &mem);
    TEST_ASSERT(fail_exec == OV_ERR_SECURITY_VIOLATION, "Execution of invalidated TB rejected by security gate");
    TEST_ASSERT(ctx.state == OV_CPU_STATE_EXCEPTION, "CPU state transitions to EXCEPTION");
    TEST_ASSERT(ctx.exception == OV_EXC_INVALID_TRANSLATION_BLOCK, "Exception code is INVALID_TRANSLATION_BLOCK");

    ov_mem_destroy(&mem);
}

// 6. Runtime Invalidation Mechanisms
void test_runtime_invalidation(void) {
    printf("\n=== Phase 11 Test 6: Invalidation Mechanisms (Hash, Range, All) ===\n");
    ov_runtime_t rt;
    ov_runtime_init(&rt);

    ov_translation_block_t *tb1 = ov_runtime_create_tb(&rt, 0x1000, 0x1020, OV_ARCH_ARM64, OV_ARCH_X86_64, 0x1111ULL);
    tb1->state = OV_TB_STATE_VALID;
    tb1->is_valid = true;

    ov_translation_block_t *tb2 = ov_runtime_create_tb(&rt, 0x2000, 0x2020, OV_ARCH_ARM64, OV_ARCH_X86_64, 0x2222ULL);
    tb2->state = OV_TB_STATE_VALID;
    tb2->is_valid = true;

    ov_translation_block_t *tb3 = ov_runtime_create_tb(&rt, 0x3000, 0x3020, OV_ARCH_ARM64, OV_ARCH_X86_64, 0x3333ULL);
    tb3->state = OV_TB_STATE_VALID;
    tb3->is_valid = true;

    // 1. Invalidate by hash
    ov_runtime_invalidate_by_hash(&rt, 0x1111ULL);
    TEST_ASSERT(!tb1->is_valid && tb1->state == OV_TB_STATE_INVALIDATED, "TB1 invalidated by source hash");
    TEST_ASSERT(tb2->is_valid && tb3->is_valid, "TB2 and TB3 remain valid");

    // 2. Invalidate by range
    ov_runtime_invalidate_range(&rt, 0x1FFF, 0x2010);
    TEST_ASSERT(!tb2->is_valid && tb2->state == OV_TB_STATE_INVALIDATED, "TB2 invalidated by address range");
    TEST_ASSERT(tb3->is_valid, "TB3 remains valid");

    // 3. Invalidate all
    uint32_t gen_before = rt.current_generation;
    ov_runtime_invalidate_all(&rt);
    TEST_ASSERT(!tb3->is_valid && tb3->state == OV_TB_STATE_INVALIDATED, "TB3 invalidated by invalidate_all");
    TEST_ASSERT(rt.current_generation > gen_before, "Runtime generation bumped after full invalidation");
}

// 7. OVIR-GPU 5-Stage Route Evaluation & CPU Fallback
void test_ovir_gpu_route_hierarchy(void) {
    printf("\n=== Phase 11 Test 7: OVIR-GPU 5-Stage Route Evaluation & CPU Fallback ===\n");

    ov_gpu_host_caps_t caps = {
        .backend = OV_GFX_BACKEND_METAL_NATIVE,
        .supported_features = OV_GPU_FEAT_COMPUTE | OV_GPU_FEAT_BC_COMPRESSION,
        .partial_features = OV_GPU_FEAT_GEOMETRY_SHADER,
        .available_vram_mb = 2048,
        .has_metal = true,
        .has_moltenvk = false,
        .has_opengl = true,
        .cpu_fallback_supported = true
    };

    ovir_gpu_workload_t wl;

    // Route 1: Native (Metal on Metal host with supported features)
    ovir_gpu_create_sample_workload(&wl, OV_GFX_API_METAL, OV_GPU_FEAT_COMPUTE);
    ov_reason_code_t r1;
    ov_gpu_route_t route1 = ovir_gpu_evaluate_route(&wl, &caps, &r1);
    TEST_ASSERT(route1 == OV_GPU_ROUTE_NATIVE, "Stage 1: Native GPU route identified");
    TEST_ASSERT(r1 == OV_REASON_NATIVE_SUPPORTED, "Reason code is NATIVE_SUPPORTED");

    // Route 2: Translation (Vulkan on Metal host with supported features)
    ovir_gpu_create_sample_workload(&wl, OV_GFX_API_VULKAN, OV_GPU_FEAT_COMPUTE);
    ov_reason_code_t r2;
    ov_gpu_route_t route2 = ovir_gpu_evaluate_route(&wl, &caps, &r2);
    TEST_ASSERT(route2 == OV_GPU_ROUTE_TRANSLATION, "Stage 2: Translation route identified");
    TEST_ASSERT(r2 == OV_REASON_GPU_API_MISMATCH_TRANSLATABLE, "Reason code is GPU_API_MISMATCH_TRANSLATABLE");

    // Route 3: Reconstruction (Needs geometry shader which is in partial_features)
    ovir_gpu_create_sample_workload(&wl, OV_GFX_API_VULKAN, OV_GPU_FEAT_COMPUTE | OV_GPU_FEAT_GEOMETRY_SHADER);
    ov_reason_code_t r3;
    ov_gpu_route_t route3 = ovir_gpu_evaluate_route(&wl, &caps, &r3);
    TEST_ASSERT(route3 == OV_GPU_ROUTE_RECONSTRUCTION, "Stage 3: Reconstruction route identified for partial features");
    TEST_ASSERT(r3 == OV_REASON_GPU_PARTIAL_SUPPORT_RECONSTRUCTION, "Reason code is GPU_PARTIAL_SUPPORT_RECONSTRUCTION");

    // Route 4: CPU Fallback (Needs Mesh Shaders - not supported on GPU, but host CPU fallback is enabled)
    ovir_gpu_create_sample_workload(&wl, OV_GFX_API_D3D12, OV_GPU_FEAT_MESH_SHADERS);
    ov_reason_code_t r4;
    ov_gpu_route_t route4 = ovir_gpu_evaluate_route(&wl, &caps, &r4);
    TEST_ASSERT(route4 == OV_GPU_ROUTE_CPU_FALLBACK, "Stage 4: CPU Fallback route engaged when GPU lacks feature");
    TEST_ASSERT(r4 == OV_REASON_CPU_FALLBACK_REQUIRED, "Reason code is CPU_FALLBACK_REQUIRED");

    // Dispatch translation with CPU fallback
    ovir_gpu_dispatch_result_t disp_result;
    ov_status_t disp_st = ovir_gpu_translate_workload(&wl, &caps, &disp_result);
    TEST_ASSERT(disp_st == OV_SUCCESS, "CPU Fallback workload translation succeeded");
    TEST_ASSERT(disp_result.cpu_fallback.cpu_fallback_engaged, "CPU fallback flag set in dispatch result");
    TEST_ASSERT(disp_result.selected_backend == OV_GFX_BACKEND_NONE, "Backend is NONE (executed on CPU)");
    TEST_ASSERT(strstr(disp_result.cpu_fallback.cpu_execution_path, "Host SIMD") != NULL, "SIMD CPU rasterizer path chosen");

    // Route 5: Truly Unsupported (CPU fallback disabled and missing features)
    ov_gpu_host_caps_t no_fallback_caps = caps;
    no_fallback_caps.cpu_fallback_supported = false;
    ov_reason_code_t r5;
    ov_gpu_route_t route5 = ovir_gpu_evaluate_route(&wl, &no_fallback_caps, &r5);
    TEST_ASSERT(route5 == OV_GPU_ROUTE_UNSUPPORTED, "Stage 5: Truly unsupported when CPU fallback unavailable");
}

// 8. Resolver CPU Fallback Plan Creation
void test_resolver_cpu_fallback_plan(void) {
    printf("\n=== Phase 11 Test 8: Resolver CPU Fallback Execution Plan ===\n");

    ov_host_env_t host = {
        .host_cpu_arch = OV_ARCH_X86_64,
        .host_cpu_features = OV_CPU_FEAT_AVX | OV_CPU_FEAT_AVX2,
        .host_cpu_cores = 8,
        .host_ram_mb = 16384,
        .gpu_caps = {
            .backend = OV_GFX_BACKEND_METAL_NATIVE,
            .supported_features = OV_GPU_FEAT_COMPUTE,
            .partial_features = 0,
            .available_vram_mb = 2048,
            .has_metal = true,
            .cpu_fallback_supported = true // Host can execute software rasterization
        }
    };

    ov_workload_desc_t wl = {
        .source_cpu_arch = OV_ARCH_X86_64,
        .target_cpu_arch = OV_ARCH_X86_64,
        .source_gfx_api = OV_GFX_API_D3D12,
        .required_gpu_features = OV_GPU_FEAT_RAY_TRACING, // Missing on GPU
        .ram_requirement_mb = 1024,
        .vram_requirement_mb = 256,
        .required_cpu_cores = 4,
        .native_execution_available = true,
        .priority = OV_PRIORITY_HIGH
    };

    ov_execution_plan_t plan;
    ov_status_t st = ov_resolver_create_plan(&wl, &host, NULL, &plan);

    TEST_ASSERT(st == OV_SUCCESS, "Resolver successfully planned CPU Fallback workload");
    TEST_ASSERT(plan.decision == OV_DECISION_CPU_FALLBACK, "Execution decision is OV_DECISION_CPU_FALLBACK");
    TEST_ASSERT(plan.reason_code == OV_REASON_CPU_FALLBACK_REQUIRED, "Reason is CPU_FALLBACK_REQUIRED");
    TEST_ASSERT(plan.cpu_fallback_engaged, "cpu_fallback_engaged flag is true");
    TEST_ASSERT(plan.is_supported, "Workload is supported via CPU fallback");
    TEST_ASSERT(strstr(plan.limitations, "CPU fallback software reconstruction") != NULL, "Limitations explain CPU fallback");
}

// 9. Status Matrix System & Hardware Registry
void test_status_matrix_system(void) {
    printf("\n=== Phase 11 Test 9: Status Matrix System & Registry Validation ===\n");
    ov_status_registry_t reg;
    ov_status_system_init(&reg);

    TEST_ASSERT(reg.item_count >= 8, "Status system initialized with comprehensive entries");

    // Find ARM64 translation item
    const ov_status_item_t *arm64_item = ov_status_find(&reg, "ARM64 Translation");
    TEST_ASSERT(arm64_item != NULL, "Found ARM64 translation entry");
    TEST_ASSERT(arm64_item->status_level == OV_STATUS_TRANSLATED, "ARM64 status is TRANSLATED");
    TEST_ASSERT(arm64_item->translation_support, "Translation support is true");

    // Find MacBookPro9,1 target
    const ov_status_item_t *mac_item = ov_status_find(&reg, "MacBookPro9,1");
    TEST_ASSERT(mac_item != NULL, "Found MacBookPro9,1 target entry");
    TEST_ASSERT(mac_item->status_level == OV_STATUS_SUPPORTED, "MacBookPro9,1 status is SUPPORTED");
    TEST_ASSERT(mac_item->cpu_fallback_support, "CPU fallback supported on legacy Mac");

    // Check status name conversions
    TEST_ASSERT(strcmp(ov_compat_status_name(OV_STATUS_CPU_FALLBACK), "CPU FALLBACK") == 0, "ov_compat_status_name returns CPU FALLBACK");
    TEST_ASSERT(strcmp(ov_status_category_name(OV_CATEGORY_MAC_MODEL), "Mac Model") == 0, "ov_status_category_name returns Mac Model");
}

// 10. Non-Destructive Installer Verification
void test_installer_safety_verification(void) {
    printf("\n=== Phase 11 Test 10: Non-Destructive Installer Safety Controls ===\n");

    ov_installation_config_t config = {
        .target_device_node = "/dev/disk2s1", // External USB
        .esp_mount_point = "/Volumes/OPENVINTAGE_USB",
        .dry_run = true,
        .verify_sha256 = true,
        .backup_existing = true
    };

    ov_install_manifest_t manifest;
    ov_status_t st = ov_installer_prepare_manifest(&config, &manifest);

    TEST_ASSERT(st == OV_SUCCESS, "Installer manifest prepared");
    TEST_ASSERT(manifest.artifact_count == 4, "4 core EFI artifacts staged for installation");

    // Verify disk safety: internal ESP disk0s1 MUST be rejected
    ov_installation_config_t unsafe_config = config;
    strncpy(unsafe_config.target_device_node, "/dev/disk0s1", sizeof(unsafe_config.target_device_node) - 1);

    ov_status_t safe_check = ov_installer_verify_target_disk(&unsafe_config);
    TEST_ASSERT(safe_check == OV_ERR_SECURITY_VIOLATION, "Internal disk0s1 strictly rejected by safety guard");
}

int main(void) {
    printf("=================================================================\n");
    printf("   OPENVINTAGE PHASE 11 — RUNTIME, MEMORY & FALLBACK TEST SUITE  \n");
    printf("=================================================================\n");

    test_memory_mapping_and_alignment();
    test_memory_wx_security();
    test_memory_bounds_and_checked_io();
    test_runtime_context_and_tb_lifecycle();
    test_runtime_execution_and_gates();
    test_runtime_invalidation();
    test_ovir_gpu_route_hierarchy();
    test_resolver_cpu_fallback_plan();
    test_status_matrix_system();
    test_installer_safety_verification();

    printf("\n-----------------------------------------------------------------\n");
    printf("  Phase 11 Results: %d/%d assertions passed (%s)\n",
           g_tests_passed, g_tests_total,
           (g_tests_passed == g_tests_total) ? "SUCCESS" : "FAILURE");
    printf("=================================================================\n");

    return (g_tests_passed == g_tests_total) ? 0 : 1;
}
