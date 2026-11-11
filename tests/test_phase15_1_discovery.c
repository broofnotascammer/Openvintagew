#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../core/types/ov_types.h"
#include "../core/platform/ov_platform.h"
#include "../core/platform/ov_platform_adapter.h"
#include "../core/resolver/ov_resolver.h"

static void test_real_hardware_discovery_default(void) {
    printf("[Phase 15.1 Test 1] Real Native Hardware Discovery (Default Mode)...\n");

    // Force real hardware discovery mode
    ov_status_t st = ov_platform_set_discovery_mode(OV_DISCOVERY_MODE_REAL, NULL);
    assert(st == OV_SUCCESS);
    assert(ov_platform_get_discovery_mode() == OV_DISCOVERY_MODE_REAL);

    const ov_platform_info_t *info = ov_platform_get_info();
    assert(info != NULL);
    assert(info->init_state == OV_PLATFORM_INIT_READY);
    assert(!info->is_simulated_profile);
    assert(info->discovery_mode == OV_DISCOVERY_MODE_REAL);

    // 1. Verify Real OS Detection
    assert(info->os_type == OV_OS_LINUX || info->os_type == OV_OS_MACOS || info->os_type == OV_OS_WINDOWS);
    assert(strlen(info->kernel_version) > 0);
    assert(strlen(info->os_version) > 0);

    // 2. Verify Real Memory Detection
    assert(info->total_ram_mb > 0);
    assert(info->page_size == 4096);
    assert(info->available_ram_mb <= info->total_ram_mb);

    // 3. Verify Real CPU Architecture & Features
    assert(info->cpu.physical_cores >= 1);
    assert(info->cpu.logical_threads >= 1);
    assert(strlen(info->cpu.vendor) > 0);
    assert(strlen(info->cpu.model_name) > 0);

#if defined(__x86_64__) || defined(_M_X64)
    assert(info->cpu.arch == OV_ARCH_X86_64);
    assert(info->cpu.sse_supported);
    assert(info->cpu.sse2_supported);
    // Vector register enablement requires OS verification
    if (info->cpu.osxsave_supported) {
        printf("  [NOTE] Host CPU supports OSXSAVE; OS vector state verified: %s\n",
               info->cpu.os_vector_enabled ? "ACTIVE (XCR0 YMM/XMM enabled)" : "DISABLED by OS");
    }
#endif

    // 4. Verify GPU Enumeration
    assert(info->gpu_device_count >= 1);
    const ov_platform_gpu_device_t *gpu0 = &info->gpu_devices[0];
    assert(strlen(gpu0->vendor_name) > 0);
    assert(strlen(gpu0->device_name) > 0);
    assert(gpu0->supports_opengl_33);

    printf("  [PASS] Real Host Hardware: OS='%s', Kernel='%s', RAM=%lluMB, CPU='%s' (%s, %u cores/%u threads), GPU='%s'.\n",
           info->os_name, info->kernel_version, (unsigned long long)info->total_ram_mb,
           info->cpu.model_name, info->cpu.vendor, info->cpu.physical_cores, info->cpu.logical_threads,
           gpu0->device_name);
}

static void test_mock_profile_explicit_separation(void) {
    printf("[Phase 15.1 Test 2] Mock Hardware Profile Explicit Separation...\n");

    // Must require explicit opt-in
    ov_status_t st = ov_platform_set_discovery_mode(OV_DISCOVERY_MODE_MOCK_PROFILE, "MacBookPro9,1");
    assert(st == OV_SUCCESS);
    assert(ov_platform_get_discovery_mode() == OV_DISCOVERY_MODE_MOCK_PROFILE);

    const ov_platform_info_t *mock = ov_platform_get_info();
    assert(mock != NULL);
    assert(mock->is_simulated_profile == true);
    assert(mock->discovery_mode == OV_DISCOVERY_MODE_MOCK_PROFILE);
    assert(strcmp(mock->profile_name, "MacBookPro9,1") == 0);

    // Verify mock profile reflects exact MacBookPro9,1 specs
    assert(mock->os_type == OV_OS_MACOS);
    assert(strstr(mock->os_version, "Catalina 10.15.8") != NULL);
    assert(strstr(mock->kernel_version, "Darwin 19.6.0") != NULL);
    assert(mock->total_ram_mb == 8192);
    assert(mock->available_ram_mb == 5840);

    // Mock CPU: Ivy Bridge Core i7-3615QM
    assert(strstr(mock->cpu.model_name, "i7-3615QM") != NULL);
    assert(mock->cpu.physical_cores == 4);
    assert(mock->cpu.logical_threads == 8);
    assert(mock->cpu.avx_supported == true);
    assert(mock->cpu.avx2_supported == false); // Critical: Ivy Bridge lacks AVX2

    // Mock GPU: Dual GPU (Intel HD 4000 + NVIDIA GT 650M)
    assert(mock->gpu_device_count == 2);
    assert(strstr(mock->gpu_devices[0].device_name, "HD Graphics 4000") != NULL);
    assert(mock->gpu_devices[0].is_discrete == false);
    assert(strstr(mock->gpu_devices[1].device_name, "GT 650M") != NULL);
    assert(mock->gpu_devices[1].is_discrete == true);
    assert(mock->gpu_devices[1].dedicated_vram_mb == 1024);

    // Switch back to real discovery
    st = ov_platform_set_discovery_mode(OV_DISCOVERY_MODE_REAL, NULL);
    assert(st == OV_SUCCESS);
    const ov_platform_info_t *real_again = ov_platform_get_info();
    assert(real_again->is_simulated_profile == false);
    assert(real_again->discovery_mode == OV_DISCOVERY_MODE_REAL);

    printf("  [PASS] Mock Profile isolation verified: explicit activation, tagged as simulated, restored to real.\n");
}

static void test_multi_gpu_ranking_and_resolver_ingestion(void) {
    printf("[Phase 15.1 Test 3] Multi-GPU Ranking & OVResolver Host Environment Ingestion...\n");

    // Apply mock profile with dual GPUs
    ov_platform_set_discovery_mode(OV_DISCOVERY_MODE_MOCK_PROFILE, "MacBookPro9,1");

    ov_host_env_t host;
    ov_platform_populate_host_env(&host);

    // Host environment must select discrete GPU VRAM (1024 MB for GT 650M) rather than blindly device 0
    assert(host.gpu_caps.available_vram_mb == 1024);
    assert(host.host_cpu_cores == 4);
    assert(host.host_ram_mb == 8192);
    assert(host.gpu_caps.has_metal);

    // Test a workload requiring 1000 MB VRAM: should succeed on discrete Kepler GT 650M
    ov_workload_desc_t wl = {
        .source_cpu_arch = OV_ARCH_X86_64,
        .target_cpu_arch = OV_ARCH_X86_64,
        .source_gfx_api = OV_GFX_API_METAL,
        .preferred_gfx_backend = OV_GFX_BACKEND_METAL_NATIVE,
        .required_cpu_features = OV_CPU_FEAT_AVX,
        .required_gpu_features = OV_GPU_FEAT_COMPUTE,
        .ram_requirement_mb = 4096,
        .vram_requirement_mb = 1000,
        .required_cpu_cores = 2,
        .priority = OV_PRIORITY_NORMAL,
        .native_execution_available = true
    };

    ov_execution_plan_t plan;
    ov_status_t rst = ov_resolver_create_plan(&wl, &host, NULL, &plan);
    assert(rst == OV_SUCCESS);
    assert(plan.is_supported);
    assert(plan.decision == OV_DECISION_NATIVE);

    // Test a workload requiring 2000 MB VRAM: should be rejected due to VRAM limitation
    wl.vram_requirement_mb = 2000;
    rst = ov_resolver_create_plan(&wl, &host, NULL, &plan);
    assert(rst == OV_ERR_UNSUPPORTED);
    assert(plan.reason_code == OV_REASON_INSUFFICIENT_VRAM);

    // Restore real hardware discovery
    ov_platform_set_discovery_mode(OV_DISCOVERY_MODE_REAL, NULL);

    printf("  [PASS] Multi-GPU selection picked discrete GPU (1024MB VRAM); OVResolver correctly bounded workload.\n");
}

static void test_capability_states_and_naming(void) {
    printf("[Phase 15.1 Test 4] Discovery & Capability State Names...\n");

    assert(strcmp(ov_discovery_mode_name(OV_DISCOVERY_MODE_REAL), "REAL_HARDWARE") == 0);
    assert(strcmp(ov_discovery_mode_name(OV_DISCOVERY_MODE_MOCK_PROFILE), "MOCK_PROFILE") == 0);

    assert(strcmp(ov_cap_state_name(OV_CAP_STATE_UNKNOWN), "UNKNOWN") == 0);
    assert(strcmp(ov_cap_state_name(OV_CAP_STATE_DETECTED), "DETECTED") == 0);
    assert(strcmp(ov_cap_state_name(OV_CAP_STATE_AVAILABLE), "AVAILABLE") == 0);
    assert(strcmp(ov_cap_state_name(OV_CAP_STATE_INITIALIZED), "INITIALIZED") == 0);
    assert(strcmp(ov_cap_state_name(OV_CAP_STATE_VERIFIED), "VERIFIED") == 0);
    assert(strcmp(ov_cap_state_name(OV_CAP_STATE_UNAVAILABLE), "UNAVAILABLE") == 0);

    printf("  [PASS] All state string mappings conform to Phase 15.1 specification.\n");
}

int main(void) {
    printf("====================================================================\n");
    printf("  OPENVINTAGE PHASE 15.1: REAL HARDWARE DISCOVERY TEST SUITE        \n");
    printf("====================================================================\n");

    test_real_hardware_discovery_default();
    test_mock_profile_explicit_separation();
    test_multi_gpu_ranking_and_resolver_ingestion();
    test_capability_states_and_naming();

    printf("\n>>> [ALL PHASE 15.1 DISCOVERY ASSERTIONS PASSED SUCCESSFULLY] <<<\n");
    return 0;
}
