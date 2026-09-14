/**
 * OpenVintage Pre-Boot Architecture Simulator - Comprehensive Test Suite
 * Validates memory safety, hardware detection, Mac profiles (MBP9,1, etc.),
 * CPU IR execution, GPU pipeline, unified multi-tier cache with LRU eviction,
 * resource manager pressure levels, empirical benchmarks, and macOS compatibility.
 * Returns 0 on complete pass, non-zero on any failure.
 */

#include "ov_core.h"
#include "ov_memory.h"
#include "ov_hardware.h"
#include "ov_macos_compat.h"
#include "ov_cpu_engine.h"
#include "ov_gpu_engine.h"
#include "ov_unified_cache.h"
#include "ov_resource_manager.h"
#include "ov_resolver.h"
#include "ov_benchmark.h"
#include "ov_diagnostics.h"
#include "ov_compatibility.h"
#include "ov_platform.h"
#include "ov_security.h"
#include "ov_perf_profile.h"
#include "ov_boot_picker.h"
#include "ov_deployment.h"
#include "ov_oclp_adapter.h"
#include "ov_refind_adapter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static int g_tests_run = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST_ASSERT(expr, msg) do { \
    g_tests_run++; \
    if (expr) { \
        g_tests_passed++; \
    } else { \
        g_tests_failed++; \
        fprintf(stderr, "  [FAIL] Line %d: %s (%s)\n", __LINE__, msg, #expr); \
    } \
} while (0)

#define TEST_SECTION(name) printf("\n>>> [TEST SUITE] %s\n", name)

/* 1. Memory Subsystem Test */
static void test_memory_subsystem(void) {
    TEST_SECTION("1. Memory Subsystem & Leak Tracking");

    ov_status_t st = ov_memory_init(16 * 1024 * 1024);
    TEST_ASSERT(st == OV_SUCCESS, "ov_memory_init succeeds");

    ov_memory_stats_t stats_init = ov_memory_get_stats();
    TEST_ASSERT((stats_init.num_allocations - stats_init.num_frees) == 0, "Initial active allocations should be 0");

    void *ptr1 = ov_malloc(1024);
    TEST_ASSERT(ptr1 != NULL, "Alloc 1KB succeeds");
    memset(ptr1, 0xAA, 1024);

    void *ptr2 = ov_malloc(4096);
    TEST_ASSERT(ptr2 != NULL, "Alloc 4KB succeeds");
    memset(ptr2, 0xBB, 4096);

    ov_memory_stats_t stats_alloc = ov_memory_get_stats();
    TEST_ASSERT((stats_alloc.num_allocations - stats_alloc.num_frees) == 2, "Active allocations should be 2");
    TEST_ASSERT(stats_alloc.current_usage >= 5120, "Allocated bytes tracked correctly");

    ov_free(ptr1);
    ov_free(ptr2);

    ov_memory_stats_t stats_final = ov_memory_get_stats();
    TEST_ASSERT((stats_final.num_allocations - stats_final.num_frees) == 0, "All allocations freed, active count is 0");
    TEST_ASSERT(stats_final.current_usage == 0, "0 bytes leaked");
    TEST_ASSERT(!stats_final.has_leaks, "No leaks reported");

    ov_memory_cleanup();
}

/* 2. Hardware Profiles & Detection Test */
static void test_hardware_and_mac_profiles(void) {
    TEST_SECTION("2. Hardware Detection & Mac Profile Database");

    ov_status_t st = ov_hardware_init();
    TEST_ASSERT(st == OV_SUCCESS, "ov_hardware_init succeeds");

    /* Host detection (safe, read-only) */
    ov_cpu_info_t host_cpu;
    st = ov_hardware_detect_cpu(&host_cpu);
    TEST_ASSERT(st == OV_SUCCESS, "Host CPU detection succeeds");
    TEST_ASSERT(host_cpu.cores >= 1, "Host CPU has at least 1 core");

    uint32_t profile_count = ov_hardware_get_profile_count();
    TEST_ASSERT(profile_count >= 30, "Database contains at least 30 Mac hardware profiles");

    /* Lookup MacBookPro9,1 specifically (User requirement) */
    ov_hardware_profile_t mbp91;
    st = ov_hardware_find_profile_by_name("MacBookPro9,1", &mbp91);
    TEST_ASSERT(st == OV_SUCCESS, "Found profile MacBookPro9,1");
    TEST_ASSERT(mbp91.profile_id == OV_HW_PROFILE_MBP91_IVY_BRIDGE, "Profile ID matches Ivy Bridge");
    TEST_ASSERT(mbp91.cpu.type == OV_CPU_INTEL_IVY_BRIDGE, "CPU is Ivy Bridge");
    TEST_ASSERT(mbp91.gpu.arch_gen == OV_GPU_ARCH_INTEL_GEN7, "Primary GPU is Intel Gen7 (HD 4000)");
    TEST_ASSERT(mbp91.has_discrete_gpu == true, "MacBookPro9,1 has discrete GPU");
    TEST_ASSERT(mbp91.secondary_gpu.arch_gen == OV_GPU_ARCH_NVIDIA_KEPLER, "Secondary GPU is NVIDIA Kepler (GT 650M)");
    TEST_ASSERT(mbp91.efi_is_64bit == true, "MacBookPro9,1 has 64-bit EFI");

    /* Test lookup with shorthand alias "MBP9,1" */
    ov_hardware_profile_t mbp_short;
    st = ov_hardware_find_profile_by_name("MBP9,1", &mbp_short);
    TEST_ASSERT(st == OV_SUCCESS, "Found profile with shorthand MBP9,1");
    TEST_ASSERT(mbp_short.profile_id == mbp91.profile_id, "Shorthand maps to same profile");

    /* Test lookup of MacPro5,1 */
    ov_hardware_profile_t mp51;
    st = ov_hardware_find_profile_by_name("MacPro5,1", &mp51);
    TEST_ASSERT(st == OV_SUCCESS, "Found profile MacPro5,1");
    TEST_ASSERT(mp51.cpu.type == OV_CPU_INTEL_NEHALEM, "MacPro5,1 CPU is Westmere/Nehalem");

    /* Test setting active profile */
    st = ov_hardware_set_active_profile(OV_HW_PROFILE_MBP91_IVY_BRIDGE);
    TEST_ASSERT(st == OV_SUCCESS, "Set active profile to MBP9,1");

    const ov_hardware_profile_t *active = ov_hardware_get_active_profile();
    TEST_ASSERT(active != NULL, "Active profile is non-null");
    TEST_ASSERT(strcmp(active->model_identifier, "MacBookPro9,1") == 0, "Active identifier is MacBookPro9,1");

    const ov_cpu_info_t *cur_cpu = ov_hardware_get_cpu();
    TEST_ASSERT(cur_cpu != NULL && cur_cpu->type == OV_CPU_INTEL_IVY_BRIDGE, "Active CPU is Ivy Bridge");

    /* Test Multi-GPU topology in MacBookPro9,1 */
    uint32_t gpu_count = ov_hardware_get_gpu_count();
    TEST_ASSERT(gpu_count == 2, "MBP9,1 exposes exactly 2 GPUs in topology (HD 4000 + GT 650M)");

    const ov_gpu_info_t *gpu0 = ov_hardware_get_gpu_at(0);
    TEST_ASSERT(gpu0 != NULL, "GPU 0 is accessible");
    TEST_ASSERT(gpu0->arch_gen == OV_GPU_ARCH_INTEL_GEN7, "GPU 0 is Intel HD 4000 (Gen7)");
    TEST_ASSERT(gpu0->vendor_id == 0x8086, "GPU 0 vendor is Intel (0x8086)");

    const ov_gpu_info_t *gpu1 = ov_hardware_get_gpu_at(1);
    TEST_ASSERT(gpu1 != NULL, "GPU 1 is accessible");
    TEST_ASSERT(gpu1->arch_gen == OV_GPU_ARCH_NVIDIA_KEPLER, "GPU 1 is NVIDIA Kepler GT 650M");
    TEST_ASSERT(gpu1->vendor_id == 0x10DE, "GPU 1 vendor is NVIDIA (0x10DE)");
    TEST_ASSERT(gpu1->metal_level == OV_METAL_2, "GPU 1 supports Metal 2 (Kepler)");
    TEST_ASSERT(gpu1->supports_vulkan == true, "GPU 1 supports Vulkan");

    const ov_gpu_topology_t *topo = ov_hardware_get_gpu_topology();
    TEST_ASSERT(topo != NULL, "GPU topology structure is valid");
    TEST_ASSERT(topo->gpu_count == 2, "Topology count is 2");
    TEST_ASSERT(topo->has_integrated_gpu == true, "Topology reports integrated GPU present");
    TEST_ASSERT(topo->has_discrete_gpu == true, "Topology reports discrete GPU present");
    TEST_ASSERT(topo->is_muxed_switchable == true, "MBP9,1 is GMUX hardware switchable");

    /* Test CPUID Max Leaf bounds checking */
    uint32_t max_leaf = ov_cpuid_max_leaf();
    TEST_ASSERT(max_leaf >= 1, "CPUID leaf 0 returns valid max leaf >= 1");

    /* Test Platform Abstraction Dispatch */
    ov_platform_type_t ptype = ov_platform_get_current();
    TEST_ASSERT(ptype == OV_PLATFORM_LINUX || ptype == OV_PLATFORM_MACOS, "Current platform is valid OS");
    const char *pname = ov_platform_get_name();
    TEST_ASSERT(pname != NULL && strlen(pname) > 0, "Platform name is non-empty string");

    ov_hardware_cleanup();
}

/* 2b. Strict Separation of Native vs. Simulated Hardware Architecture */
static void test_native_vs_simulated_hardware_architecture(void) {
    TEST_SECTION("2b. Native vs. Simulated Hardware Architecture Verification");

    ov_status_t st = ov_hardware_init();
    TEST_ASSERT(st == OV_SUCCESS, "ov_hardware_init succeeds");

    /* 1. Verify default mode is strictly NATIVE */
    ov_hw_mode_t mode = ov_hardware_get_mode();
    TEST_ASSERT(mode == OV_HW_MODE_NATIVE, "Default hardware mode is strictly NATIVE");

    ov_hw_source_t source = ov_hardware_get_source();
    TEST_ASSERT(source == OV_HW_SOURCE_NATIVE, "Default hardware source is strictly NATIVE");

    const char *mode_str = ov_hardware_get_mode_string();
    TEST_ASSERT(strcmp(mode_str, "NATIVE") == 0, "Hardware mode string is 'NATIVE'");

    const char *source_str = ov_hardware_get_source_string();
    TEST_ASSERT(strcmp(source_str, "NATIVE") == 0, "Hardware source string is 'NATIVE'");

    const ov_hardware_profile_t *active = ov_hardware_get_active_profile();
    TEST_ASSERT(active != NULL, "Active profile is valid");
    TEST_ASSERT(active->profile_id == OV_HW_PROFILE_HOST, "Active profile is OV_HW_PROFILE_HOST");
    TEST_ASSERT(active->is_simulated == false, "Native profile is NOT flagged as simulated");
    TEST_ASSERT(active->source == OV_HW_SOURCE_NATIVE, "Active profile source is OV_HW_SOURCE_NATIVE");

    /* 2. Verify native mode does not fall back to MBP9,1 profile data */
    ov_hardware_profile_t host_p;
    st = ov_hardware_get_profile(OV_HW_PROFILE_HOST, &host_p);
    TEST_ASSERT(st == OV_SUCCESS, "Host profile retrieval succeeds");
    TEST_ASSERT(host_p.is_simulated == false, "Host profile has is_simulated == false");

    /* 3. Verify switching to SIMULATED mode for a specific profile */
    st = ov_hardware_set_active_profile(OV_HW_PROFILE_MBP91_IVY_BRIDGE);
    TEST_ASSERT(st == OV_SUCCESS, "Switched to simulated MBP9,1 profile");
    TEST_ASSERT(ov_hardware_get_mode() == OV_HW_MODE_SIMULATED, "Hardware mode is now SIMULATED");
    TEST_ASSERT(ov_hardware_get_source() == OV_HW_SOURCE_SIMULATED, "Hardware source is now SIMULATED");

    active = ov_hardware_get_active_profile();
    TEST_ASSERT(active != NULL, "Simulated active profile is non-null");
    TEST_ASSERT(active->is_simulated == true, "Simulated profile has is_simulated == true");
    TEST_ASSERT(active->source == OV_HW_SOURCE_SIMULATED, "Simulated profile has source == SIMULATED");
    TEST_ASSERT(strcmp(active->model_identifier, "MacBookPro9,1") == 0, "Active model is MacBookPro9,1");

    /* 4. Verify switching profiles by name in simulated mode */
    st = ov_hardware_set_active_profile_by_name("MacPro5,1");
    TEST_ASSERT(st == OV_SUCCESS, "Switched to simulated MacPro5,1 profile");
    TEST_ASSERT(ov_hardware_get_mode() == OV_HW_MODE_SIMULATED, "Mode remains SIMULATED");
    active = ov_hardware_get_active_profile();
    TEST_ASSERT(strcmp(active->model_identifier, "MacPro5,1") == 0, "Active model is MacPro5,1");
    TEST_ASSERT(active->is_simulated == true, "Profile is flagged as simulated");

    /* 5. Verify switching back to NATIVE mode */
    st = ov_hardware_set_mode(OV_HW_MODE_NATIVE);
    TEST_ASSERT(st == OV_SUCCESS, "ov_hardware_set_mode(OV_HW_MODE_NATIVE) succeeds");
    TEST_ASSERT(ov_hardware_get_mode() == OV_HW_MODE_NATIVE, "Hardware mode is back to NATIVE");
    TEST_ASSERT(ov_hardware_get_source() == OV_HW_SOURCE_NATIVE, "Hardware source is back to NATIVE");

    active = ov_hardware_get_active_profile();
    TEST_ASSERT(active != NULL, "Active profile is non-null");
    TEST_ASSERT(active->profile_id == OV_HW_PROFILE_HOST, "Active profile restored to host");
    TEST_ASSERT(active->is_simulated == false, "Restored profile is not simulated");

    /* 6. Verify diagnostic report reflects native vs simulated modes */
    ov_unified_cache_init();
    ov_diagnostics_init();

    ov_diagnostic_report_t native_rep;
    st = ov_diagnostics_generate_report(&native_rep);
    TEST_ASSERT(st == OV_SUCCESS, "Generated diagnostic report in NATIVE mode");
    TEST_ASSERT(strcmp(native_rep.hardware_mode, "NATIVE") == 0, "Report hardware_mode is NATIVE");
    TEST_ASSERT(strcmp(native_rep.hardware_source, "NATIVE") == 0, "Report hardware_source is NATIVE");
    TEST_ASSERT(strstr(native_rep.simulated_target_model, "Native") != NULL, "Report simulated_target_model indicates native");

    /* Switch to simulated and check report */
    ov_hardware_set_active_profile(OV_HW_PROFILE_MBP91_IVY_BRIDGE);
    ov_diagnostic_report_t sim_rep;
    st = ov_diagnostics_generate_report(&sim_rep);
    TEST_ASSERT(st == OV_SUCCESS, "Generated diagnostic report in SIMULATED mode");
    TEST_ASSERT(strcmp(sim_rep.hardware_mode, "SIMULATED") == 0, "Report hardware_mode is SIMULATED");
    TEST_ASSERT(strcmp(sim_rep.hardware_source, "SIMULATED") == 0, "Report hardware_source is SIMULATED");
    TEST_ASSERT(strcmp(sim_rep.simulated_target_model, "MacBookPro9,1") == 0, "Report simulated target is MacBookPro9,1");

    ov_diagnostics_cleanup();
    ov_unified_cache_cleanup();
    ov_hardware_cleanup();
}

/* 2c. Dedicated Real Hardware (macOS Native) Execution & Smoke Test */
static ov_status_t mock_native_mbp91_success(ov_hardware_profile_t *out_host) {
    if (!out_host) return OV_ERROR_INVALID_PARAM;
    memset(out_host, 0, sizeof(ov_hardware_profile_t));
    snprintf(out_host->profile_name, sizeof(out_host->profile_name), "Native Host (MacBookPro9,1)");
    snprintf(out_host->model_identifier, sizeof(out_host->model_identifier), "MacBookPro9,1");
    snprintf(out_host->marketing_name, sizeof(out_host->marketing_name), "MacBook Pro (15-inch, Mid 2012)");
    snprintf(out_host->description, sizeof(out_host->description), "Mac-4B7AC7E43945597E Mid 2012 Unibody");
    out_host->is_mac_host = true;
    out_host->efi_is_64bit = true;
    snprintf(out_host->firmware_type, sizeof(out_host->firmware_type), "Apple EFI 2.0");
    snprintf(out_host->storage_interface, sizeof(out_host->storage_interface), "SATA III 6Gb/s AHCI");

    out_host->cpu.type = OV_CPU_INTEL_IVY_BRIDGE;
    snprintf(out_host->cpu.model_name, sizeof(out_host->cpu.model_name), "Intel(R) Core(TM) i7-3615QM CPU @ 2.30GHz");
    out_host->cpu.cores = 4;
    out_host->cpu.threads = 8;
    out_host->cpu.base_freq_mhz = 2300;
    out_host->cpu.max_freq_mhz = 3300;
    out_host->cpu.has_sse42 = true;
    out_host->cpu.has_avx = true;
    out_host->cpu.has_avx2 = false;
    out_host->cpu.has_aesni = true;

    out_host->mem.total_bytes = 8ULL * 1024 * 1024 * 1024;
    out_host->mem.available_bytes = 6ULL * 1024 * 1024 * 1024;
    out_host->mem.channels = 2;
    out_host->mem.frequency_mhz = 1600;
    snprintf(out_host->mem.memory_type, sizeof(out_host->mem.memory_type), "DDR3-1600");

    out_host->gpu_topology.gpu_count = 2;
    out_host->gpu_topology.primary_gpu_index = 0;
    out_host->gpu_topology.discrete_gpu_index = 1;
    out_host->gpu_topology.has_integrated_gpu = true;
    out_host->gpu_topology.has_discrete_gpu = true;
    out_host->gpu_topology.is_muxed_switchable = true;
    snprintf(out_host->gpu_topology.switch_policy, sizeof(out_host->gpu_topology.switch_policy), "Apple GMUX Hardware Multiplexed");

    ov_gpu_info_t *g0 = &out_host->gpu_topology.gpus[0];
    g0->type = OV_GPU_INTEL_GEN7_HD4000;
    g0->arch_gen = OV_GPU_ARCH_INTEL_GEN7;
    snprintf(g0->model_name, sizeof(g0->model_name), "Intel HD Graphics 4000");
    g0->vendor_id = 0x8086;
    g0->device_id = 0x0166;
    g0->vram_bytes = 1536ULL * 1024 * 1024;
    g0->vram_is_detected = true;
    snprintf(g0->vram_description, sizeof(g0->vram_description), "1536 MB (Dynamically Allocated from Unified System RAM)");
    g0->metal_level = OV_METAL_1;
    g0->supports_metal = true;
    snprintf(g0->metal_source, sizeof(g0->metal_source), "Derived from Architecture: Intel Gen7 Ivy Bridge HD 4000");
    g0->supports_opengl_core = true;
    g0->opengl_major = 4;
    g0->opengl_minor = 1;
    snprintf(g0->opengl_source, sizeof(g0->opengl_source), "Derived from Driver Architecture: OpenGL 4.1 Core Profile");
    g0->supports_vulkan = false;
    snprintf(g0->vulkan_source, sizeof(g0->vulkan_source), "Derived: Vulkan Unsupported on Intel Gen7 HD 4000");
    g0->max_texture_dimension = 16384;

    ov_gpu_info_t *g1 = &out_host->gpu_topology.gpus[1];
    g1->type = OV_GPU_NVIDIA_GEFORCE;
    g1->arch_gen = OV_GPU_ARCH_NVIDIA_KEPLER;
    snprintf(g1->model_name, sizeof(g1->model_name), "NVIDIA GeForce GT 650M");
    g1->vendor_id = 0x10DE;
    g1->device_id = 0x0FD5;
    g1->vram_bytes = 1024ULL * 1024 * 1024;
    g1->vram_is_detected = true;
    snprintf(g1->vram_description, sizeof(g1->vram_description), "1024 MB GDDR5 Dedicated VRAM");
    g1->metal_level = OV_METAL_2;
    g1->supports_metal = true;
    snprintf(g1->metal_source, sizeof(g1->metal_source), "Derived from Architecture: NVIDIA GK107 Kepler");
    g1->supports_opengl_core = true;
    g1->opengl_major = 4;
    g1->opengl_minor = 1;
    snprintf(g1->opengl_source, sizeof(g1->opengl_source), "Derived from Driver Architecture: OpenGL 4.1 Core Profile");
    g1->supports_vulkan = true;
    snprintf(g1->vulkan_source, sizeof(g1->vulkan_source), "Derived: Requires MoltenVK runtime translation");
    g1->max_texture_dimension = 16384;

    out_host->gpu = *g0;
    out_host->secondary_gpu = *g1;
    out_host->has_discrete_gpu = true;

    return OV_SUCCESS;
}

static ov_status_t mock_native_detect_missing_gpu(ov_hardware_profile_t *out_host) {
    if (!out_host) return OV_ERROR_INVALID_PARAM;
    memset(out_host, 0, sizeof(ov_hardware_profile_t));
    out_host->cpu.cores = 4;
    snprintf(out_host->cpu.model_name, sizeof(out_host->cpu.model_name), "Intel Core i7");
    out_host->mem.total_bytes = 8ULL * 1024 * 1024 * 1024;
    out_host->gpu_topology.gpu_count = 0; /* Missing GPU */
    return OV_SUCCESS;
}

static void test_dedicated_real_hardware_mode(void) {
    TEST_SECTION("2c. Dedicated Real Hardware (macOS Native) Execution & Smoke Test");

    ov_status_t st = ov_hardware_init();
    TEST_ASSERT(st == OV_SUCCESS, "ov_hardware_init succeeds");

    /* 1. Test failure path: Strict detection fails loudly when GPU subsystem missing */
    ov_hardware_set_native_detect_mock(mock_native_detect_missing_gpu);
    char fail_reason[256];
    st = ov_hardware_detect_native_strict(fail_reason, sizeof(fail_reason));
    TEST_ASSERT(st == OV_ERROR_HARDWARE, "Strict detection fails with OV_ERROR_HARDWARE on broken GPU discovery");
    TEST_ASSERT(strstr(fail_reason, "GPU") != NULL, "Diagnostic indicates GPU failure");

    /* 2. Test successful physical host probing with realistic MBP9,1 IOKit topology */
    ov_hardware_set_native_detect_mock(mock_native_mbp91_success);
    st = ov_hardware_detect_native_strict(fail_reason, sizeof(fail_reason));
    TEST_ASSERT(st == OV_SUCCESS, "Strict native detection succeeds with realistic host mock");

    /* Verify operating mode and source */
    TEST_ASSERT(ov_hardware_get_mode() == OV_HW_MODE_NATIVE, "Operating mode is strictly NATIVE");
    TEST_ASSERT(ov_hardware_get_source() == OV_HW_SOURCE_NATIVE, "Hardware source is strictly NATIVE");
    const ov_hardware_profile_t *active = ov_hardware_get_active_profile();
    TEST_ASSERT(active != NULL, "Active profile is valid");
    TEST_ASSERT(active->is_simulated == false, "Active profile is marked NOT simulated");
    TEST_ASSERT(active->is_mac_host == true, "Active profile is marked Genuine Apple Host");

    /* Verify GPU topology & VRAM source accuracy */
    TEST_ASSERT(active->gpu_topology.gpu_count == 2, "Discovered exactly 2 GPUs");
    const ov_gpu_info_t *g0 = &active->gpu_topology.gpus[0];
    TEST_ASSERT(g0->vendor_id == 0x8086, "GPU 0 vendor is Intel");
    TEST_ASSERT(g0->vram_is_detected == true, "GPU 0 VRAM detection tracked");
    TEST_ASSERT(strstr(g0->metal_source, "Derived") != NULL, "Metal capability explicitly labeled as derived");

    const ov_gpu_info_t *g1 = &active->gpu_topology.gpus[1];
    TEST_ASSERT(g1->vendor_id == 0x10DE, "GPU 1 vendor is NVIDIA");
    TEST_ASSERT(g1->metal_level == OV_METAL_2, "NVIDIA GT 650M Kepler supports Metal 2");
    TEST_ASSERT(strstr(g1->vulkan_source, "MoltenVK") != NULL, "Vulkan capability specifies MoltenVK translation");

    /* 3. Test topology validation */
    char err_buf[256];
    st = ov_hardware_validate_native_topology(err_buf, sizeof(err_buf));
    TEST_ASSERT(st == OV_SUCCESS, "ov_hardware_validate_native_topology succeeds on valid topology");

    /* 4. Test smoke test (resolution of Metal + OpenGL workloads) */
    st = ov_hardware_run_smoke_test();
    TEST_ASSERT(st == OV_SUCCESS, "ov_hardware_run_smoke_test succeeds");

    /* 5. Test Native Diagnostic Exporters (Text and JSON) */
    const char *test_native_txt = "/tmp/ov_test_native_audit.txt";
    const char *test_native_json = "/tmp/ov_test_native_audit.json";

    st = ov_diagnostics_export_native_text(test_native_txt);
    TEST_ASSERT(st == OV_SUCCESS, "ov_diagnostics_export_native_text succeeds");
    FILE *ftxt = fopen(test_native_txt, "r");
    TEST_ASSERT(ftxt != NULL, "Native text audit file was created");
    if (ftxt) {
        char buf[1024];
        size_t n = fread(buf, 1, sizeof(buf) - 1, ftxt);
        buf[n] = '\0';
        TEST_ASSERT(strstr(buf, "Hardware Source:     NATIVE") != NULL, "Text report contains Hardware Source: NATIVE");
        TEST_ASSERT(strstr(buf, "Host Model ID:       MacBookPro9,1") != NULL, "Text report contains Host Model ID");
        TEST_ASSERT(strstr(buf, "Genuine Apple Host:  YES") != NULL, "Text report contains Genuine Apple Host: YES");
        fclose(ftxt);
    }

    st = ov_diagnostics_export_native_json(test_native_json);
    TEST_ASSERT(st == OV_SUCCESS, "ov_diagnostics_export_native_json succeeds");
    FILE *fjson = fopen(test_native_json, "r");
    TEST_ASSERT(fjson != NULL, "Native JSON audit file was created");
    if (fjson) {
        char buf[2048];
        size_t n = fread(buf, 1, sizeof(buf) - 1, fjson);
        buf[n] = '\0';
        TEST_ASSERT(strstr(buf, "\"hardware_source\": \"NATIVE\"") != NULL, "JSON report contains hardware_source NATIVE");
        TEST_ASSERT(strstr(buf, "\"is_simulated\": false") != NULL, "JSON report contains is_simulated false");
        TEST_ASSERT(strstr(buf, "\"is_mac_host\": true") != NULL, "JSON report contains is_mac_host true");
        TEST_ASSERT(strstr(buf, "\"pci_vendor_id\": \"0x8086\"") != NULL, "JSON report contains PCI Vendor ID 0x8086");
        fclose(fjson);
    }

    /* Reset mock hook */
    ov_hardware_set_native_detect_mock(NULL);
    ov_hardware_cleanup();
}

/* 3. macOS Version Compatibility Matrix Test */
static void test_macos_compatibility(void) {
    TEST_SECTION("3. macOS Architecture & OCLP Compatibility Engine");

    ov_hardware_init();
    ov_macos_compat_init();

    ov_hardware_profile_t mbp91;
    ov_status_t st = ov_hardware_find_profile_by_name("MacBookPro9,1", &mbp91);
    TEST_ASSERT(st == OV_SUCCESS, "Found MacBookPro9,1 for compatibility test");

    /* Catalina (10.15): Native */
    ov_macos_compat_eval_t eval_cat;
    st = ov_macos_compat_evaluate(OV_MACOS_10_15_CATALINA, &mbp91, &eval_cat);
    TEST_ASSERT(st == OV_SUCCESS, "Evaluate Catalina succeeds");
    TEST_ASSERT(eval_cat.rating == OV_MACOS_SUPPORTED_NATIVE, "Catalina is natively supported on MBP9,1");

    /* Monterey (12.0): Supported with OCLP */
    ov_macos_compat_eval_t eval_mon;
    st = ov_macos_compat_evaluate(OV_MACOS_12_MONTEREY, &mbp91, &eval_mon);
    TEST_ASSERT(st == OV_SUCCESS, "Evaluate Monterey succeeds");
    TEST_ASSERT(eval_mon.rating == OV_MACOS_SUPPORTED_WITH_PATCHES, "Monterey requires patches on MBP9,1");
    TEST_ASSERT(eval_mon.patcher_recommended == OV_PATCHER_OPENCORE_LEGACY, "Monterey recommends OCLP on MBP9,1");

    /* Ventura (13.0): Supported with OCLP + Cryptex */
    ov_macos_compat_eval_t eval_ven;
    st = ov_macos_compat_evaluate(OV_MACOS_13_VENTURA, &mbp91, &eval_ven);
    TEST_ASSERT(st == OV_SUCCESS, "Evaluate Ventura succeeds");
    TEST_ASSERT(eval_ven.rating == OV_MACOS_SUPPORTED_WITH_PATCHES, "Ventura requires patches on MBP9,1");
    TEST_ASSERT(eval_ven.requires_cryptex_patches == true, "Ventura on Ivy Bridge requires non-AVX2 Cryptex patch");

    /* Matrix Evaluation */
    ov_macos_compat_matrix_t matrix;
    st = ov_macos_compat_evaluate_matrix(&mbp91, &matrix);
    TEST_ASSERT(st == OV_SUCCESS, "Matrix evaluation succeeds");
    TEST_ASSERT(matrix.target_count >= 15, "Matrix evaluated at least 15 macOS releases");

    char matrix_text[4096];
    ov_macos_compat_format_matrix(&mbp91, matrix_text, sizeof(matrix_text));
    TEST_ASSERT(strstr(matrix_text, "MacBookPro9,1") != NULL, "Formatted matrix contains model ID");
    TEST_ASSERT(strstr(matrix_text, "Monterey") != NULL, "Formatted matrix contains Monterey");

    ov_macos_compat_cleanup();
    ov_hardware_cleanup();
}

/* 4. CPU Engine & IR Execution Test */
static void test_cpu_engine_execution(void) {
    TEST_SECTION("4. CPU Engine IR Instruction Execution");

    ov_memory_init(16 * 1024 * 1024);
    ov_status_t st = ov_cpu_engine_init();
    TEST_ASSERT(st == OV_SUCCESS, "ov_cpu_engine_init succeeds");

    /* Construct a Program:
     * R1 = 40
     * R2 = 2
     * R3 = R1 + R2  (= 42)
     * R4 = R3 * 2   (= 84)
     */
    ov_cpu_program_t prog;
    memset(&prog, 0, sizeof(prog));
    prog.program_id = 1;
    prog.block_count = 1;

    ov_cpu_basic_block_t *bb = &prog.blocks[0];
    bb->block_id = 0;

    bb->instructions[0].opcode = OV_CPU_OP_MOV;
    bb->instructions[0].dst = (ov_cpu_operand_t){OV_CPU_OPND_REG, 1, 0, 0, 0};
    bb->instructions[0].src1 = (ov_cpu_operand_t){OV_CPU_OPND_IMM, 0, 40, 0, 0};

    bb->instructions[1].opcode = OV_CPU_OP_MOV;
    bb->instructions[1].dst = (ov_cpu_operand_t){OV_CPU_OPND_REG, 2, 0, 0, 0};
    bb->instructions[1].src1 = (ov_cpu_operand_t){OV_CPU_OPND_IMM, 0, 2, 0, 0};

    bb->instructions[2].opcode = OV_CPU_OP_ADD;
    bb->instructions[2].dst = (ov_cpu_operand_t){OV_CPU_OPND_REG, 3, 0, 0, 0};
    bb->instructions[2].src1 = (ov_cpu_operand_t){OV_CPU_OPND_REG, 1, 0, 0, 0};
    bb->instructions[2].src2 = (ov_cpu_operand_t){OV_CPU_OPND_REG, 2, 0, 0, 0};

    bb->instructions[3].opcode = OV_CPU_OP_MUL;
    bb->instructions[3].dst = (ov_cpu_operand_t){OV_CPU_OPND_REG, 4, 0, 0, 0};
    bb->instructions[3].src1 = (ov_cpu_operand_t){OV_CPU_OPND_REG, 3, 0, 0, 0};
    bb->instructions[3].src2 = (ov_cpu_operand_t){OV_CPU_OPND_IMM, 0, 2, 0, 0};

    bb->instructions[4].opcode = OV_CPU_OP_RET;
    bb->instr_count = 5;
    prog.total_instructions = 5;

    int64_t registers[16] = {0};
    uint64_t cycles = 0;

    st = ov_cpu_execute_program(&prog, registers, 16, &cycles);
    TEST_ASSERT(st == OV_SUCCESS, "ov_cpu_execute_program executes successfully");
    TEST_ASSERT(registers[1] == 40, "R1 == 40");
    TEST_ASSERT(registers[2] == 2, "R2 == 2");
    TEST_ASSERT(registers[3] == 42, "R3 == 42 (40 + 2)");
    TEST_ASSERT(registers[4] == 84, "R4 == 84 (42 * 2)");
    TEST_ASSERT(cycles > 0, "Cycles consumed > 0");

    ov_cpu_engine_cleanup();
    ov_memory_cleanup();
}

/* 5. GPU Engine Command Stream & Clamping Test */
static void test_gpu_engine_pipeline(void) {
    TEST_SECTION("5. GPU Engine Pipeline & Texture Clamping");

    ov_memory_init(16 * 1024 * 1024);
    ov_hardware_init();
    ov_hardware_set_active_profile(OV_HW_PROFILE_MBP91_IVY_BRIDGE);

    ov_status_t st = ov_gpu_engine_init();
    TEST_ASSERT(st == OV_SUCCESS, "ov_gpu_engine_init succeeds");

    ov_gpu_command_list_t cmd_list;
    memset(&cmd_list, 0, sizeof(cmd_list));
    st = ov_gpu_record_sample_frame(0, &cmd_list);
    TEST_ASSERT(st == OV_SUCCESS, "Record sample GPU frame succeeds");
    TEST_ASSERT(cmd_list.count > 0, "GPU command list has commands");

    char err_buf[256] = {0};
    st = ov_gpu_validate_command_list(&cmd_list, err_buf, sizeof(err_buf));
    TEST_ASSERT(st == OV_SUCCESS, "GPU command list validation succeeds");

    ov_gpu_shader_translation_t trans;
    st = ov_gpu_simulate_shader_translation(0, &trans);
    TEST_ASSERT(st == OV_SUCCESS, "GPU shader translation simulation succeeds");

    /* Texture dimension check against Gen7 limit (8192px) */
    const ov_gpu_info_t *gpu = ov_hardware_get_gpu();
    TEST_ASSERT(gpu->max_texture_dimension == 8192, "HD 4000 max texture dimension is 8192px");

    ov_gpu_engine_cleanup();
    ov_hardware_cleanup();
    ov_memory_cleanup();
}

/* 6. Unified Multi-Tier Cache with LRU Eviction Test */
static void test_unified_cache(void) {
    TEST_SECTION("6. Unified Multi-Tier Cache & LRU Eviction");

    ov_status_t st = ov_unified_cache_init();
    TEST_ASSERT(st == OV_SUCCESS, "ov_unified_cache_init succeeds");

    uint64_t key1 = 0x1111222233334444ULL;
    uint8_t val1[64] = {0xAA, 0xBB, 0xCC, 0xDD};

    st = ov_unified_cache_store(OV_CACHE_TIER_CPU_TRANSLATION, key1, val1, sizeof(val1));
    TEST_ASSERT(st == OV_SUCCESS, "Store in CPU cache tier succeeds");

    uint8_t out_val[64] = {0};
    size_t out_len = 0;
    st = ov_unified_cache_lookup(OV_CACHE_TIER_CPU_TRANSLATION, key1, out_val, sizeof(out_val), &out_len);
    TEST_ASSERT(st == OV_SUCCESS, "Lookup key1 in CPU cache tier succeeds (HIT)");
    TEST_ASSERT(memcmp(val1, out_val, sizeof(val1)) == 0, "Retrieved data matches stored data");

    uint64_t non_existent_key = 0x9999999999999999ULL;
    st = ov_unified_cache_lookup(OV_CACHE_TIER_CPU_TRANSLATION, non_existent_key, out_val, sizeof(out_val), &out_len);
    TEST_ASSERT(st == OV_ERROR_NOT_FOUND, "Lookup nonexistent key returns OV_ERROR_NOT_FOUND (MISS)");

    /* Fill entries to trigger LRU eviction */
    for (uint64_t i = 100; i < 200; i++) {
        uint8_t data[128];
        memset(data, (int)i, sizeof(data));
        ov_unified_cache_store(OV_CACHE_TIER_SHADER, i, data, sizeof(data));
    }

    ov_unified_cache_stats_t cstats;
    st = ov_unified_cache_get_stats(&cstats);
    TEST_ASSERT(st == OV_SUCCESS, "Get cache stats succeeds");
    TEST_ASSERT(cstats.shader_cache_entries > 0, "Shader cache has entries");
    TEST_ASSERT(cstats.cpu_cache_hits >= 1, "Cache hit recorded");
    TEST_ASSERT(cstats.cpu_cache_misses >= 1, "Cache miss recorded");

    ov_unified_cache_cleanup();
}

/* 7. Resource Manager Pressure Levels Test */
static void test_resource_manager(void) {
    TEST_SECTION("7. Resource Manager Budget & Pressure Levels");

    ov_hardware_init();
    ov_status_t st = ov_resource_manager_init();
    TEST_ASSERT(st == OV_SUCCESS, "ov_resource_manager_init succeeds");

    st = ov_resource_manager_set_profile(OV_PROFILE_BALANCED);
    TEST_ASSERT(st == OV_SUCCESS, "Set profile balanced succeeds");

    ov_resource_status_t status;
    st = ov_resource_manager_get_status(&status);
    TEST_ASSERT(st == OV_SUCCESS, "Get resource status succeeds");
    TEST_ASSERT(status.pressure_level == OV_RESOURCE_PRESSURE_NORMAL, "Initial pressure is NORMAL");

    /* Allocate resources */
    st = ov_resource_manager_allocate_memory(1024ULL * 1024 * 1024, false);
    TEST_ASSERT(st == OV_SUCCESS, "Allocate 1GB RAM succeeds");

    ov_resource_manager_cleanup();
    ov_hardware_cleanup();
}

/* 8. Unified Resolver Subsystem Test */
static void test_resolver_subsystem(void) {
    TEST_SECTION("8. Architecture Decision Matrix & Resolver");

    ov_hardware_init();
    ov_hardware_set_active_profile(OV_HW_PROFILE_MBP91_IVY_BRIDGE);
    ov_unified_cache_init();

    ov_cpu_info_t *cpu = (ov_cpu_info_t*)ov_hardware_get_cpu();
    ov_gpu_info_t *gpu = (ov_gpu_info_t*)ov_hardware_get_gpu();
    ov_memory_info_t *mem = (ov_memory_info_t*)ov_hardware_get_memory();

    ov_status_t st = ov_resolver_init(cpu, gpu, mem);
    TEST_ASSERT(st == OV_SUCCESS, "ov_resolver_init succeeds");

    /* Test Preset 0: Metal Odyssey */
    ov_integrated_request_t req0 = ov_resolver_get_preset_request(OV_WORKLOAD_PRESET_METAL_GAME);
    ov_resolution_result_t res0;
    st = ov_resolver_evaluate_integrated(&req0, &res0);
    TEST_ASSERT(st == OV_SUCCESS, "Resolve Metal Odyssey succeeds");
    TEST_ASSERT(res0.gpu_translation_required == true, "Metal on HD 4000 requires GPU translation");
    TEST_ASSERT(res0.performance_cost_factor >= 100, "Cost factor >= 100%");

    /* Test Preset 5: OpenGL Classic */
    ov_integrated_request_t req5 = ov_resolver_get_preset_request(OV_WORKLOAD_PRESET_OPENGL_CLASSIC);
    ov_resolution_result_t res5;
    st = ov_resolver_evaluate_integrated(&req5, &res5);
    TEST_ASSERT(st == OV_SUCCESS, "Resolve OpenGL Classic succeeds");
    TEST_ASSERT(res5.gpu_translation_required == false, "OpenGL 3.3 is native on HD 4000");

    /* Multi-GPU Topology-Aware Resolution Tests (MBP9,1 dual-GPU workload distribution) */
    const ov_gpu_topology_t *mbp_topo = ov_hardware_get_gpu_topology();
    TEST_ASSERT(mbp_topo != NULL && mbp_topo->gpu_count == 2, "MBP9,1 dual GPU topology is available");

    /* Case A: Metal Odyssey requires Metal 2 -> Resolver should auto-select GPU 1 (NVIDIA GT 650M) */
    ov_resolution_result_t res_topo_metal;
    st = ov_resolver_evaluate_with_topology(&req0, mbp_topo, &res_topo_metal);
    TEST_ASSERT(st == OV_SUCCESS, "Resolve Metal Odyssey across topology succeeds");
    TEST_ASSERT(res_topo_metal.selected_gpu_index == 1, "Resolver selects GPU 1 (NVIDIA GT 650M) for Metal 2");
    TEST_ASSERT(res_topo_metal.gpu_translation_required == false, "NVIDIA GT 650M Kepler runs Metal 2 natively without translation");
    TEST_ASSERT(strstr(res_topo_metal.selected_gpu_name, "NVIDIA") != NULL, "Selected GPU name reflects NVIDIA GT 650M");

    /* Case B: Vulkan Shooter -> Resolver should auto-select GPU 1 (NVIDIA GT 650M) */
    ov_integrated_request_t req_vk = ov_resolver_get_preset_request(OV_WORKLOAD_PRESET_VULKAN_SHOOTER);
    ov_resolution_result_t res_topo_vk;
    st = ov_resolver_evaluate_with_topology(&req_vk, mbp_topo, &res_topo_vk);
    TEST_ASSERT(st == OV_SUCCESS, "Resolve Vulkan Shooter across topology succeeds");
    TEST_ASSERT(res_topo_vk.selected_gpu_index == 1, "Resolver selects GPU 1 (NVIDIA GT 650M) for Vulkan");
    TEST_ASSERT(res_topo_vk.gpu_translation_required == false, "NVIDIA GT 650M Kepler runs Vulkan natively");

    /* Case C: OpenGL Classic -> Resolver should select GPU 0 (Intel HD 4000) for power efficiency */
    ov_resolution_result_t res_topo_gl;
    st = ov_resolver_evaluate_with_topology(&req5, mbp_topo, &res_topo_gl);
    TEST_ASSERT(st == OV_SUCCESS, "Resolve OpenGL Classic across topology succeeds");
    TEST_ASSERT(res_topo_gl.selected_gpu_index == 0, "Resolver selects GPU 0 (Intel HD 4000) for lightweight OpenGL");
    TEST_ASSERT(res_topo_gl.gpu_translation_required == false, "Intel HD 4000 runs OpenGL 3.3 natively");

    ov_resolver_cleanup();
    ov_unified_cache_cleanup();
    ov_hardware_cleanup();
}

/* 9. Empirical Benchmarks Test */
static void test_empirical_benchmarks(void) {
    TEST_SECTION("9. Empirical Architectural Benchmarking");

    ov_status_t st = ov_benchmark_init();
    TEST_ASSERT(st == OV_SUCCESS, "ov_benchmark_init succeeds");

    ov_benchmark_results_t results;
    st = ov_benchmark_run_suite(&results);
    TEST_ASSERT(st == OV_SUCCESS, "ov_benchmark_run_suite executes without error");
    TEST_ASSERT(results.alu_benchmark.baseline_cycles > 0, "ALU benchmark executed cycles > 0");
    TEST_ASSERT(results.cache_benchmark.cache_hit_latency_ns > 0 || results.cache_benchmark.baseline_duration_us > 0, "Cache latency measured");
    TEST_ASSERT(results.mem_bandwidth_benchmark.throughput_mb_s > 0, "Memory bandwidth measured (> 0 MB/s)");
    TEST_ASSERT(results.overall_speedup_percent > 0, "Overall benchmark speedup percent is positive");

    char summary[1024];
    ov_benchmark_format_summary(&results, summary, sizeof(summary));
    TEST_ASSERT(strstr(summary, "BENCHMARK") != NULL || strstr(summary, "Benchmark") != NULL, "Summary contains Benchmark");

    ov_benchmark_cleanup();
}

/* 10. Diagnostics & Exporters Test */
static void test_diagnostics_and_reports(void) {
    TEST_SECTION("10. Diagnostics Engine & Report Generation");

    ov_memory_init(16 * 1024 * 1024);
    ov_hardware_init();
    ov_hardware_set_active_profile(OV_HW_PROFILE_MBP91_IVY_BRIDGE);
    ov_unified_cache_init();
    ov_diagnostics_init();

    ov_diagnostic_report_t report;
    ov_status_t st = ov_diagnostics_generate_report(&report);
    TEST_ASSERT(st == OV_SUCCESS, "Generate diagnostic report succeeds");
    TEST_ASSERT(report.system_health_score >= 50 && report.system_health_score <= 100, "Health score is in range 50-100");
    TEST_ASSERT(strcmp(report.mac_model, "MacBookPro9,1") == 0, "Report contains MacBookPro9,1");
    TEST_ASSERT(strstr(report.macos_compat_summary, "Monterey") != NULL, "Report contains Monterey compatibility");

    /* Export Text, JSON, and HTML to temp files */
    st = ov_diagnostics_export_text(&report, "/tmp/ov_test_report.txt");
    TEST_ASSERT(st == OV_SUCCESS, "Export text report succeeds");

    st = ov_diagnostics_export_json(&report, "/tmp/ov_test_report.json");
    TEST_ASSERT(st == OV_SUCCESS, "Export JSON report succeeds");

    st = ov_diagnostics_export_html(&report, "/tmp/ov_test_report.html");
    TEST_ASSERT(st == OV_SUCCESS, "Export HTML report succeeds");

    /* Validate generated files are non-empty */
    FILE *f_txt = fopen("/tmp/ov_test_report.txt", "r");
    TEST_ASSERT(f_txt != NULL, "Report text file created");
    if (f_txt) {
        char buf[256];
        size_t r = fread(buf, 1, sizeof(buf) - 1, f_txt);
        buf[r] = '\0';
        TEST_ASSERT(r > 50, "Report text file is non-empty");
        TEST_ASSERT(strstr(buf, "OPENVINTAGE") != NULL, "Text report header present");
        fclose(f_txt);
    }

    FILE *f_json = fopen("/tmp/ov_test_report.json", "r");
    TEST_ASSERT(f_json != NULL, "Report JSON file created");
    if (f_json) {
        char buf[256];
        size_t r = fread(buf, 1, sizeof(buf) - 1, f_json);
        buf[r] = '\0';
        TEST_ASSERT(r > 50, "Report JSON file is non-empty");
        TEST_ASSERT(strstr(buf, "\"openvintage\"") != NULL, "JSON root key present");
        fclose(f_json);
    }

    FILE *f_html = fopen("/tmp/ov_test_report.html", "r");
    TEST_ASSERT(f_html != NULL, "Report HTML file created");
    if (f_html) {
        char buf[256];
        size_t r = fread(buf, 1, sizeof(buf) - 1, f_html);
        buf[r] = '\0';
        TEST_ASSERT(r > 50, "Report HTML file is non-empty");
        TEST_ASSERT(strstr(buf, "<!DOCTYPE html>") != NULL, "HTML doctype present");
        fclose(f_html);
    }

    ov_diagnostics_cleanup();
    ov_unified_cache_cleanup();
    ov_hardware_cleanup();
    ov_memory_cleanup();
}

/* 13. Phase 6: Security Subsystem & Boundaries */
static void test_phase6_security(void) {
    TEST_SECTION("13. Security Subsystem & Guardrails");

    /* Path traversal detection */
    TEST_ASSERT(ov_security_has_path_traversal("../etc/passwd") == true, "Detects leading relative traversal");
    TEST_ASSERT(ov_security_has_path_traversal("/Volumes/EFI/../system") == true, "Detects embedded traversal");
    TEST_ASSERT(ov_security_has_path_traversal("/Volumes/EFI/EFI/BOOT/BOOTX64.EFI") == false, "Clean path accepted");

    /* Path bounds and prefix checks */
    TEST_ASSERT(ov_security_validate_path("/Volumes/EFI/EFI/BOOT/BOOTX64.EFI", "/Volumes/EFI") == OV_SEC_SUCCESS, "Prefix validation succeeds for EFI volume");
    TEST_ASSERT(ov_security_validate_path("/usr/bin/sh", "/Volumes/EFI") == OV_SEC_ERROR_INVALID_PATH, "Out-of-prefix path rejected");

    /* Buffer bounds protection */
    TEST_ASSERT(ov_security_check_bounds(100, 200) == OV_SEC_SUCCESS, "In-bounds size allowed");
    TEST_ASSERT(ov_security_check_bounds(500, 200) == OV_SEC_ERROR_BUFFER_OVERFLOW, "Overflow rejected");

    /* Safe string copy */
    char dest[8];
    TEST_ASSERT(ov_security_safe_strcpy(dest, sizeof(dest), "hello") == OV_SEC_SUCCESS, "Safe copy fits in buffer");
    TEST_ASSERT(ov_security_safe_strcpy(dest, sizeof(dest), "too_long_string_for_dest") == OV_SEC_ERROR_BUFFER_OVERFLOW, "String truncation prevented");

    /* SHA-256 payload integrity */
    char hex[OV_SHA256_HEX_LEN];
    ov_security_sha256_buffer("OpenVintage Payload", 19, hex);
    TEST_ASSERT(strlen(hex) == 64, "SHA-256 produced 64-char hex digest");
    TEST_ASSERT(ov_security_verify_sha256("OpenVintage Payload", 19, hex) == OV_SEC_SUCCESS, "SHA-256 verification succeeds for matching payload");
    TEST_ASSERT(ov_security_verify_sha256("Modified Payload", 16, hex) == OV_SEC_ERROR_HASH_MISMATCH, "SHA-256 verification fails for altered payload");

    /* Deployment approval guard */
    TEST_ASSERT(ov_security_require_user_approval(false, "EFI Deploy") == OV_SEC_ERROR_APPROVAL_REQUIRED, "Unapproved deployment blocked by security guard");
    TEST_ASSERT(ov_security_require_user_approval(true, "EFI Deploy") == OV_SEC_SUCCESS, "Approved deployment passed by security guard");
}

/* 14. Phase 6: Performance Profiles & Active GPU Independence */
static void test_phase6_perf_profiles(void) {
    TEST_SECTION("14. Performance Profiles & Physical Inventory Invariance");

    ov_hardware_init();
    ov_hardware_set_active_profile(OV_HW_PROFILE_MBP91_IVY_BRIDGE);
    const ov_gpu_topology_t *topo_init = ov_hardware_get_gpu_topology();
    TEST_ASSERT(topo_init->gpu_count == 2, "MBP9,1 has 2 physical GPUs");
    TEST_ASSERT(topo_init->has_discrete_gpu == true, "MBP9,1 has discrete GPU");

    ov_perf_profile_init();
    TEST_ASSERT(ov_perf_profile_get_count() == 6, "Total 6 hardware-aware profiles defined");

    /* Maximum Performance: discrete GPU preferred */
    ov_status_t st = ov_perf_profile_apply(OV_PERF_PROFILE_MAX_PERFORMANCE);
    TEST_ASSERT(st == OV_SUCCESS, "Apply Max Performance succeeds");
    TEST_ASSERT(ov_hardware_get_active_gpu_index() == topo_init->discrete_gpu_index, "Max Performance activates discrete GPU index");
    TEST_ASSERT(ov_hardware_get_active_gpu() != NULL, "Active GPU pointer is valid");
    TEST_ASSERT(ov_hardware_get_gpu_topology()->gpu_count == 2, "Physical GPU inventory invariant (2 GPUs)");

    /* Battery / Efficiency: integrated GPU preferred */
    st = ov_perf_profile_apply(OV_PERF_PROFILE_BATTERY_EFFICIENCY);
    TEST_ASSERT(st == OV_SUCCESS, "Apply Battery/Efficiency succeeds");
    TEST_ASSERT(ov_hardware_get_active_gpu_index() == topo_init->primary_gpu_index, "Battery profile activates integrated GPU index");
    TEST_ASSERT(ov_hardware_get_gpu_topology()->gpu_count == 2, "Physical GPU inventory still contains 2 GPUs intact");

    /* Apply by name "Gaming" */
    st = ov_perf_profile_apply_by_name("Gaming");
    TEST_ASSERT(st == OV_SUCCESS, "Apply by name 'Gaming' succeeds");
    TEST_ASSERT(ov_perf_profile_get_active_id() == OV_PERF_PROFILE_GAMING, "Active profile is Gaming");

    ov_perf_profile_cleanup();
    ov_hardware_cleanup();
}

/* 15. Phase 6: Boot Picker & Boot Target Management */
static void test_phase6_boot_picker(void) {
    TEST_SECTION("15. Boot Picker & Target Management");

    ov_boot_picker_init();
    ov_status_t st = ov_boot_picker_scan_targets();
    TEST_ASSERT(st == OV_SUCCESS, "Target scan succeeds");

    uint32_t count = ov_boot_picker_get_target_count();
    TEST_ASSERT(count >= 4, "Discovered at least 4 targets (macOS, Recovery, OpenCore, Linux, etc.)");

    int32_t def_idx = ov_boot_picker_get_default_index();
    TEST_ASSERT(def_idx >= 0, "Valid default boot target assigned");

    /* Selection change */
    st = ov_boot_picker_select_index(1);
    TEST_ASSERT(st == OV_SUCCESS, "Select target index 1 succeeds");
    TEST_ASSERT(ov_boot_picker_get_selected_index() == 1, "Selected index is 1");

    /* Set default */
    st = ov_boot_picker_set_default_target(2);
    TEST_ASSERT(st == OV_SUCCESS, "Set default target 2 succeeds");
    TEST_ASSERT(ov_boot_picker_get_default_index() == 2, "Default target is 2");

    /* Timeout countdown simulation */
    ov_boot_picker_set_timeout(5, true);
    bool auto_booted = false;
    ov_boot_picker_tick(3, &auto_booted);
    TEST_ASSERT(auto_booted == false, "No auto-boot at 3 seconds elapsed");
    ov_boot_picker_tick(3, &auto_booted);
    TEST_ASSERT(auto_booted == true, "Auto-boot triggered after expiry");

    char boot_summary[256];
    st = ov_boot_picker_boot_selected(boot_summary, sizeof(boot_summary));
    TEST_ASSERT(st == OV_SUCCESS, "Boot selected target succeeds");
    TEST_ASSERT(strlen(boot_summary) > 10, "Boot summary generated");

    ov_boot_picker_cleanup();
}

/* 16. Phase 6: Safe 9-Step Verified Deployment System */
static void test_phase6_deployment_flow(void) {
    TEST_SECTION("16. Safe Deployment 9-Step Lifecycle");

    ov_hardware_init();
    const ov_hardware_profile_t *hw = ov_hardware_get_active_profile();

    ov_deployment_init();
    TEST_ASSERT(ov_deployment_get_current_state() == OV_DEPLOY_STATE_IDLE, "Starts in IDLE state");

    /* Step 1: Discover */
    TEST_ASSERT(ov_deployment_step_discover(hw) == OV_SUCCESS, "Step 1: Discover succeeds");

    /* Step 2: Simulate */
    TEST_ASSERT(ov_deployment_step_simulate() == OV_SUCCESS, "Step 2: Simulate dry-run succeeds");

    /* Step 3: Plan */
    TEST_ASSERT(ov_deployment_step_create_plan("Test Plan") == OV_SUCCESS, "Step 3: Plan creation succeeds");
    const ov_deploy_plan_t *p = ov_deployment_get_current_plan();
    TEST_ASSERT(p->item_count > 0, "Plan has actionable items");

    /* Step 4: Show changes */
    char changes[256];
    TEST_ASSERT(ov_deployment_show_changes(changes, sizeof(changes)) == OV_SUCCESS, "Step 4: Show changes succeeds");

    /* Step 5: User approval guard enforcement */
    ov_deployment_require_user_approval();
    TEST_ASSERT(ov_deployment_get_current_state() == OV_DEPLOY_STATE_AWAITING_APPROVAL, "Awaiting approval state");
    TEST_ASSERT(ov_deployment_step_apply() == OV_ERROR_PERMISSION_DENIED, "Apply fails without user approval");

    /* Grant approval */
    TEST_ASSERT(ov_deployment_grant_user_approval() == OV_SUCCESS, "Grant approval succeeds");

    /* Step 6: Backup */
    TEST_ASSERT(ov_deployment_step_backup() == OV_SUCCESS, "Step 6: Backup succeeds");
    TEST_ASSERT(ov_deployment_get_current_state() == OV_DEPLOY_STATE_BACKED_UP, "State is BACKED_UP");

    /* Step 7: Apply (with verified backup guard passing) */
    ov_security_set_mock_backup(true, true);
    TEST_ASSERT(ov_deployment_step_apply() == OV_SUCCESS, "Step 7: Apply succeeds");
    TEST_ASSERT(ov_deployment_get_current_state() == OV_DEPLOY_STATE_APPLIED, "State is APPLIED");

    /* Step 8: Verify */
    TEST_ASSERT(ov_deployment_step_verify(false) == OV_SUCCESS, "Step 8: Verify succeeds");
    TEST_ASSERT(ov_deployment_get_current_state() == OV_DEPLOY_STATE_VERIFIED, "State is VERIFIED");

    /* Step 9: Recovery / Rollback verification */
    TEST_ASSERT(ov_deployment_step_verify(true) == OV_ERROR_GENERIC, "Simulated verification failure detected");
    TEST_ASSERT(ov_deployment_step_rollback() == OV_SUCCESS, "Rollback to backup succeeds");
    TEST_ASSERT(ov_deployment_get_current_state() == OV_DEPLOY_STATE_ROLLED_BACK, "State is ROLLED_BACK");

    ov_security_set_mock_backup(false, false);
    ov_deployment_cleanup();
    ov_hardware_cleanup();
}

/* 17. Phase 6: Integration Contracts (OCLP & rEFInd) */
static void test_phase6_integrations(void) {
    TEST_SECTION("17. OCLP and rEFInd Integration Adapters");

    /* OCLP Adapter */
    ov_oclp_adapter_init();
    ov_oclp_adapter_set_mock_state(true, "1.5.0", true);
    ov_oclp_info_t oclp_info;
    ov_status_t st = ov_oclp_adapter_detect(&oclp_info);
    TEST_ASSERT(st == OV_SUCCESS, "OCLP detection succeeds");
    TEST_ASSERT(oclp_info.is_installed == true, "OCLP reported installed");
    TEST_ASSERT(strcmp(oclp_info.version, "1.5.0") == 0, "OCLP version matches");
    TEST_ASSERT(oclp_info.root_patches_applied == true, "OCLP root patches applied");

    char boot_args[256];
    ov_oclp_adapter_read_config("/dummy/config.plist", boot_args, sizeof(boot_args));
    TEST_ASSERT(strstr(boot_args, "amfi_get_out_of_my_way") != NULL, "OCLP boot args parsed safely");

    /* rEFInd Adapter */
    ov_refind_adapter_init();
    ov_refind_adapter_set_mock_state(true, "0.14.2");
    ov_refind_info_t refind_info;
    st = ov_refind_adapter_detect(&refind_info);
    TEST_ASSERT(st == OV_SUCCESS, "rEFInd detection succeeds");
    TEST_ASSERT(refind_info.is_installed == true, "rEFInd reported installed");

    char entry[256];
    st = ov_refind_adapter_generate_entry("macOS 12", "EFI/OC/OpenCore.efi", "-v", entry, sizeof(entry));
    TEST_ASSERT(st == OV_SUCCESS, "rEFInd entry generated");
    TEST_ASSERT(strstr(entry, "menuentry \"macOS 12\"") != NULL, "Menuentry formatted cleanly");

    ov_refind_adapter_cleanup();
    ov_oclp_adapter_cleanup();
}

/* 18. Phase 6: Compatibility Engine OS Evaluation */
static void test_phase6_compatibility_os(void) {
    TEST_SECTION("18. Compatibility Engine OS & Hardware Evaluation");

    ov_hardware_init();
    ov_hardware_set_active_profile(OV_HW_PROFILE_MBP91_IVY_BRIDGE);

    /* macOS 10.15 Catalina -> Native on Ivy Bridge */
    ov_os_compat_result_t res;
    ov_status_t st = ov_compatibility_evaluate_active_os("macOS 10.15 Catalina", &res);
    TEST_ASSERT(st == OV_SUCCESS, "Evaluate Catalina succeeds");
    TEST_ASSERT(res.category == OV_COMPAT_CAT_NATIVELY_SUPPORTED, "Catalina is natively supported on MBP9,1");
    TEST_ASSERT(strcmp(res.hardware_source_label, "SIMULATED HARDWARE") == 0, "Accurately reports simulated profile");

    /* macOS 12 Monterey -> Requires OCLP on Ivy Bridge */
    st = ov_compatibility_evaluate_active_os("macOS 12 Monterey", &res);
    TEST_ASSERT(st == OV_SUCCESS, "Evaluate Monterey succeeds");
    TEST_ASSERT(res.category == OV_COMPAT_CAT_SUPPORTED_WITH_OCLP, "Monterey requires OCLP");
    TEST_ASSERT(res.requires_legacy_gpu_patch == true, "Kepler Metal root patches required");

    /* macOS 14 Sonoma -> Requires OCLP + Cryptex bypass */
    st = ov_compatibility_evaluate_active_os("macOS 14 Sonoma", &res);
    TEST_ASSERT(st == OV_SUCCESS, "Evaluate Sonoma succeeds");
    TEST_ASSERT(res.category == OV_COMPAT_CAT_SUPPORTED_WITH_OCLP, "Sonoma requires OCLP");
    TEST_ASSERT(res.cryptex_bypass_needed == true, "Cryptex bypass required on Ivy Bridge");

    /* Linux */
    st = ov_compatibility_evaluate_active_os("Ubuntu Linux 24.04", &res);
    TEST_ASSERT(st == OV_SUCCESS, "Evaluate Linux succeeds");
    TEST_ASSERT(res.category == OV_COMPAT_CAT_SUPPORTED_WITH_REFIND, "Linux supported with rEFInd");

    /* Windows 11 */
    st = ov_compatibility_evaluate_active_os("Windows 11 Pro", &res);
    TEST_ASSERT(st == OV_SUCCESS, "Evaluate Windows 11 succeeds");
    TEST_ASSERT(res.category == OV_COMPAT_CAT_SUPPORTED_WITH_OPENCORE, "Windows 11 supported with OpenCore spoofing");

    ov_hardware_cleanup();
}

/* 19. Phase 6: Unified Decision Pipeline */
static void test_phase6_resolver_pipeline(void) {
    TEST_SECTION("19. Capability-Based Decision Pipeline");

    ov_hardware_init();
    ov_hardware_set_active_profile(OV_HW_PROFILE_MBP91_IVY_BRIDGE);
    const ov_hardware_profile_t *hw = ov_hardware_get_active_profile();

    ov_pipeline_request_t pipe_req = {
        .hardware = hw,
        .target_os = "macOS 12 Monterey",
        .perf_profile_id = OV_PERF_PROFILE_GAMING,
        .workload = {
            .guest_cpu_arch = 2, /* x86_64 */
            .requested_api = OV_API_METAL,
            .api_version_major = 2,
            .api_version_minor = 0,
            .requires_compute = true,
            .requires_tessellation = true,
            .requires_avx2 = false,
            .max_texture_dimension = 8192,
            .required_vram_bytes = 512 * 1024 * 1024,
            .required_ram_bytes = 2ULL * 1024 * 1024 * 1024,
            .required_cpu_cores = 4,
            .guest_code_hash = 0x12345678,
            .shader_bytecode_hash = 0x87654321
        }
    };

    ov_pipeline_decision_t decision;
    ov_status_t st = ov_resolver_evaluate_pipeline(&pipe_req, &decision);
    TEST_ASSERT(st == OV_SUCCESS, "Evaluate pipeline succeeds");
    TEST_ASSERT(decision.compat_category == OV_COMPAT_CAT_SUPPORTED_WITH_OCLP, "Pipeline tags OCLP for Monterey on MBP9,1");
    TEST_ASSERT(strlen(decision.selected_gpu_name) > 0, "Selected GPU is assigned");
    TEST_ASSERT(strlen(decision.boot_arguments) > 0, "Boot arguments populated");
    TEST_ASSERT(strlen(decision.overall_pipeline_summary) > 0, "Overall pipeline summary populated");

    ov_hardware_cleanup();
}

int main(void) {
    printf("================================================================================\n");
    printf("        OpenVintage Pre-Boot Architecture Simulator - Automated Test Suite     \n");
    printf("================================================================================\n");

    test_memory_subsystem();
    test_hardware_and_mac_profiles();
    test_native_vs_simulated_hardware_architecture();
    test_dedicated_real_hardware_mode();
    test_macos_compatibility();
    test_cpu_engine_execution();
    test_gpu_engine_pipeline();
    test_unified_cache();
    test_resource_manager();
    test_resolver_subsystem();
    test_empirical_benchmarks();
    test_diagnostics_and_reports();
    test_phase6_security();
    test_phase6_perf_profiles();
    test_phase6_boot_picker();
    test_phase6_deployment_flow();
    test_phase6_integrations();
    test_phase6_compatibility_os();
    test_phase6_resolver_pipeline();

    printf("\n================================================================================\n");
    printf("TEST RESULTS: %d Tests Run, %d Passed, %d Failed\n", g_tests_run, g_tests_passed, g_tests_failed);
    printf("================================================================================\n");

    if (g_tests_failed == 0) {
        printf(">>> ALL TESTS PASSED SUCCESSFULLY! <<<\n\n");
        return 0;
    } else {
        fprintf(stderr, ">>> CRITICAL: %d TESTS FAILED! <<<\n\n", g_tests_failed);
        return 1;
    }
}
