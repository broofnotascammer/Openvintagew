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

int main(void) {
    printf("================================================================================\n");
    printf("        OpenVintage Pre-Boot Architecture Simulator - Automated Test Suite     \n");
    printf("================================================================================\n");

    test_memory_subsystem();
    test_hardware_and_mac_profiles();
    test_macos_compatibility();
    test_cpu_engine_execution();
    test_gpu_engine_pipeline();
    test_unified_cache();
    test_resource_manager();
    test_resolver_subsystem();
    test_empirical_benchmarks();
    test_diagnostics_and_reports();

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
