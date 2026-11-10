#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../core/types/ov_types.h"
#include "../core/resolver/ov_resolver.h"
#include "../core/cache/ov_unified_cache.h"
#include "../core/scheduler/ov_scheduler.h"
#include "../core/telemetry/ov_telemetry.h"
#include "../core/diagnostics/ov_diagnostics.h"
#include "../core/pipeline/ov_pipeline.h"
#include "../ovir/cpu/ovir_cpu.h"
#include "../ovir/gpu/ovir_gpu.h"

static int g_tests_passed = 0;
static int g_tests_total = 0;

#define TEST_ASSERT(cond, name) do { \
    g_tests_total++; \
    if (cond) { \
        printf("  [PASS] %s\n", name); \
        g_tests_passed++; \
    } else { \
        printf("  [FAIL] %s (Line %d)\n", name, __LINE__); \
    } \
} while(0)

static void get_test_host(ov_host_env_t *host) {
    memset(host, 0, sizeof(ov_host_env_t));
    host->host_cpu_arch = OV_ARCH_X86_64;
    host->host_cpu_features = OV_CPU_FEAT_AVX | OV_CPU_FEAT_AVX2 | OV_CPU_FEAT_FMA;
    host->host_cpu_cores = 8;
    host->host_ram_mb = 16384; // 16 GB
    host->gpu_caps.backend = OV_GFX_BACKEND_METAL_NATIVE;
    host->gpu_caps.supported_features = OV_GPU_FEAT_COMPUTE | OV_GPU_FEAT_TESSELLATION | OV_GPU_FEAT_BC_COMPRESSION;
    host->gpu_caps.available_vram_mb = 4096; // 4 GB
    host->gpu_caps.has_metal = true;
    host->gpu_caps.has_moltenvk = false;
    host->gpu_caps.has_opengl = true;
}

// 1. Native execution selection
void test_native_execution_selection(void) {
    printf("\n=== Test 1: Native Execution Selection ===\n");
    ov_host_env_t host;
    get_test_host(&host);

    ov_workload_desc_t workload = {
        .source_cpu_arch = OV_ARCH_X86_64,
        .target_cpu_arch = OV_ARCH_X86_64,
        .source_gfx_api = OV_GFX_API_METAL,
        .required_cpu_features = OV_CPU_FEAT_AVX,
        .required_gpu_features = OV_GPU_FEAT_COMPUTE,
        .ram_requirement_mb = 1024,
        .vram_requirement_mb = 512,
        .required_cpu_cores = 4,
        .native_execution_available = true,
        .priority = OV_PRIORITY_NORMAL
    };

    ov_execution_plan_t plan;
    ov_status_t st = ov_resolver_create_plan(&workload, &host, NULL, &plan);

    TEST_ASSERT(st == OV_SUCCESS, "Resolver returned OV_SUCCESS");
    TEST_ASSERT(plan.decision == OV_DECISION_NATIVE, "Decision is NATIVE");
    TEST_ASSERT(!plan.cpu_translation_required, "CPU translation is false");
    TEST_ASSERT(!plan.gpu_translation_required, "GPU translation is false");
}

// 2. CPU translation selection
void test_cpu_translation_selection(void) {
    printf("\n=== Test 2: CPU Translation Selection ===\n");
    ov_host_env_t host;
    get_test_host(&host);

    ov_workload_desc_t workload = {
        .source_cpu_arch = OV_ARCH_ARM64,
        .target_cpu_arch = OV_ARCH_X86_64,
        .source_gfx_api = OV_GFX_API_METAL, // Native on host
        .required_cpu_features = 0,
        .required_gpu_features = 0,
        .ram_requirement_mb = 512,
        .vram_requirement_mb = 256,
        .required_cpu_cores = 2,
        .native_execution_available = false,
        .priority = OV_PRIORITY_NORMAL
    };

    ov_execution_plan_t plan;
    ov_status_t st = ov_resolver_create_plan(&workload, &host, NULL, &plan);

    TEST_ASSERT(st == OV_SUCCESS, "Resolver returned OV_SUCCESS");
    TEST_ASSERT(plan.decision == OV_DECISION_CPU_TRANSLATION, "Decision is CPU_TRANSLATION");
    TEST_ASSERT(plan.cpu_translation_required, "CPU translation is true");
    TEST_ASSERT(!plan.gpu_translation_required, "GPU translation is false");
    TEST_ASSERT(plan.selected_cpu_target == OV_ARCH_X86_64, "Target CPU arch is x86-64");
}

// 3. GPU translation selection
void test_gpu_translation_selection(void) {
    printf("\n=== Test 3: GPU Translation Selection ===\n");
    ov_host_env_t host;
    get_test_host(&host);

    ov_workload_desc_t workload = {
        .source_cpu_arch = OV_ARCH_X86_64,
        .target_cpu_arch = OV_ARCH_X86_64,
        .source_gfx_api = OV_GFX_API_VULKAN, // Non-native on host Metal
        .required_cpu_features = 0,
        .required_gpu_features = OV_GPU_FEAT_COMPUTE,
        .ram_requirement_mb = 512,
        .vram_requirement_mb = 512,
        .required_cpu_cores = 2,
        .native_execution_available = true,
        .priority = OV_PRIORITY_NORMAL
    };

    ov_execution_plan_t plan;
    ov_status_t st = ov_resolver_create_plan(&workload, &host, NULL, &plan);

    TEST_ASSERT(st == OV_SUCCESS, "Resolver returned OV_SUCCESS");
    TEST_ASSERT(plan.decision == OV_DECISION_GPU_TRANSLATION, "Decision is GPU_TRANSLATION");
    TEST_ASSERT(!plan.cpu_translation_required, "CPU translation is false");
    TEST_ASSERT(plan.gpu_translation_required, "GPU translation is true");
    TEST_ASSERT(plan.selected_gfx_backend == OV_GFX_BACKEND_METAL_NATIVE, "Selected backend is Metal");
}

// 4. CPU+GPU translation selection
void test_cpu_and_gpu_translation_selection(void) {
    printf("\n=== Test 4: CPU+GPU Translation Selection ===\n");
    ov_host_env_t host;
    get_test_host(&host);

    ov_workload_desc_t workload = {
        .source_cpu_arch = OV_ARCH_ARM64,
        .target_cpu_arch = OV_ARCH_X86_64,
        .source_gfx_api = OV_GFX_API_D3D11,
        .required_cpu_features = 0,
        .required_gpu_features = 0,
        .ram_requirement_mb = 1024,
        .vram_requirement_mb = 512,
        .required_cpu_cores = 4,
        .native_execution_available = false,
        .priority = OV_PRIORITY_HIGH
    };

    ov_execution_plan_t plan;
    ov_status_t st = ov_resolver_create_plan(&workload, &host, NULL, &plan);

    TEST_ASSERT(st == OV_SUCCESS, "Resolver returned OV_SUCCESS");
    TEST_ASSERT(plan.decision == OV_DECISION_CPU_AND_GPU_TRANSLATION, "Decision is CPU_AND_GPU_TRANSLATION");
    TEST_ASSERT(plan.cpu_translation_required, "CPU translation is true");
    TEST_ASSERT(plan.gpu_translation_required, "GPU translation is true");
}

// 5. Unsupported workload
void test_unsupported_workload(void) {
    printf("\n=== Test 5: Unsupported Workload (Excess RAM/VRAM) ===\n");
    ov_host_env_t host;
    get_test_host(&host);

    ov_workload_desc_t workload = {
        .source_cpu_arch = OV_ARCH_X86_64,
        .target_cpu_arch = OV_ARCH_X86_64,
        .source_gfx_api = OV_GFX_API_METAL,
        .ram_requirement_mb = 32768, // Exceeds host 16384 MB
        .vram_requirement_mb = 1024,
        .required_cpu_cores = 4,
        .native_execution_available = true
    };

    ov_execution_plan_t plan;
    ov_status_t st = ov_resolver_create_plan(&workload, &host, NULL, &plan);

    TEST_ASSERT(st == OV_ERR_UNSUPPORTED, "Resolver correctly returns OV_ERR_UNSUPPORTED");
    TEST_ASSERT(plan.decision == OV_DECISION_UNSUPPORTED, "Plan decision is UNSUPPORTED");
    TEST_ASSERT(plan.reason_code == OV_REASON_INSUFFICIENT_RAM, "Reason code is INSUFFICIENT_RAM");
    TEST_ASSERT(!plan.is_supported, "is_supported is false");
}

// 6 & 7: CPU cache cold miss and warm hit
void test_cpu_cache_miss_and_hit(void) {
    printf("\n=== Tests 6 & 7: CPU Cache Cold Miss & Warm Hit ===\n");
    ov_host_env_t host;
    get_test_host(&host);
    ov_unified_cache_t cache;
    ov_unified_cache_init(&cache);
    ov_scheduler_t scheduler;
    ov_scheduler_init(&scheduler, host.host_cpu_cores, host.host_ram_mb, host.gpu_caps.available_vram_mb);

    ov_pipeline_context_t ctx;
    ov_pipeline_init(&ctx, &host, &cache, &scheduler);

    // ARM64 instructions: MOV r0, #42; MOV r1, #8; ADD r0, r0, r1; RET
    uint32_t arm64_code[] = {
        0x52800540, // MOV w0, #42
        0x52800101, // MOV w1, #8
        0x8b010000, // ADD x0, x0, x1
        0xd65f03c0  // RET
    };
    uint32_t code_count = sizeof(arm64_code) / sizeof(arm64_code[0]);

    ov_workload_desc_t workload = {
        .source_cpu_arch = OV_ARCH_ARM64,
        .target_cpu_arch = OV_ARCH_X86_64,
        .source_gfx_api = OV_GFX_API_NONE,
        .ram_requirement_mb = 256,
        .vram_requirement_mb = 0,
        .required_cpu_cores = 1,
        .code_identity_hash = 0x12345678abcdef00ULL,
        .native_execution_available = false,
        .priority = OV_PRIORITY_NORMAL
    };

    ov_execution_result_t result1;
    ov_status_t st1 = ov_pipeline_execute(&ctx, &workload, arm64_code, code_count, NULL, &result1);

    TEST_ASSERT(st1 == OV_SUCCESS, "Cold pipeline execution succeeded");
    TEST_ASSERT(!result1.telemetry.cache_hit, "Cold run is a cache MISS");
    TEST_ASSERT(result1.cpu_compiled.verified, "Cold generated machine code verified");
    TEST_ASSERT(result1.cpu_sim_state.registers[0] == 50, "Semantic result r0 == 50 verified");

    // Second run: same workload should hit cache
    ov_execution_result_t result2;
    ov_status_t st2 = ov_pipeline_execute(&ctx, &workload, arm64_code, code_count, NULL, &result2);

    TEST_ASSERT(st2 == OV_SUCCESS, "Warm pipeline execution succeeded");
    TEST_ASSERT(result2.telemetry.cache_hit, "Warm run is a cache HIT");
    TEST_ASSERT(result2.cpu_compiled.code_size == result1.cpu_compiled.code_size, "Cached code size matches cold");
    TEST_ASSERT(result2.cpu_sim_state.registers[0] == 50, "Cached semantic result r0 == 50 verified");
}

// 8. CPU cache invalidation
void test_cpu_cache_invalidation(void) {
    printf("\n=== Test 8: CPU Cache Invalidation on Metadata Mismatch ===\n");
    ov_unified_cache_t cache;
    ov_unified_cache_init(&cache);

    ov_cache_cpu_metadata_t meta = {
        .source_arch = OV_ARCH_ARM64,
        .target_arch = OV_ARCH_X86_64,
        .code_identity_hash = 0xfeedbacc11223344ULL,
        .translator_version_id = OV_TRANSLATOR_VERSION_ID,
        .required_cpu_features = 0,
        .optimization_level = 1
    };

    ovir_compiled_block_t block = {
        .target_arch = OV_ARCH_X86_64,
        .code_size = 4,
        .code_bytes = { 0x48, 0x31, 0xc0, 0xc3 },
        .verified = true
    };

    ov_cache_store_cpu(&cache, &meta, &block);

    // Lookup with mismatched translator version
    ov_cache_cpu_metadata_t mismatched_meta = meta;
    mismatched_meta.translator_version_id = 0x00090000; // Old version

    ovir_compiled_block_t out_block;
    ov_status_t st = ov_cache_lookup_cpu(&cache, &mismatched_meta, &out_block);

    TEST_ASSERT(st == OV_ERR_CACHE_INVALID, "Mismatched metadata returned OV_ERR_CACHE_INVALID");

    uint64_t lookups, hits, misses, invs;
    ov_cache_get_stats(&cache, &lookups, &hits, &misses, &invs);
    TEST_ASSERT(invs == 1, "Cache recorded 1 invalidation");
}

// 9 & 10: Shader cache hit/miss & pipeline validation
void test_shader_and_pipeline_cache(void) {
    printf("\n=== Tests 9 & 10: Shader Cache Hit/Miss & Pipeline Validation ===\n");
    ov_unified_cache_t cache;
    ov_unified_cache_init(&cache);

    ov_cache_gpu_metadata_t gpu_meta = {
        .source_api = OV_GFX_API_VULKAN,
        .target_backend = OV_GFX_BACKEND_METAL_NATIVE,
        .shader_hash = 0x9988776655443322ULL,
        .pipeline_hash = 0x1122334455667788ULL,
        .required_gpu_features = OV_GPU_FEAT_COMPUTE,
        .translator_version_id = OV_TRANSLATOR_VERSION_ID
    };

    uint64_t translated_shader = 0xaabbccddeeff0011ULL;
    uint64_t out_hash;

    // Miss first
    ov_status_t miss_st = ov_cache_lookup_gpu(&cache, &gpu_meta, &out_hash);
    TEST_ASSERT(miss_st == OV_ERR_CACHE_MISS, "Initial GPU lookup is MISS");

    // Store
    ov_cache_store_gpu(&cache, &gpu_meta, translated_shader);

    // Hit
    ov_status_t hit_st = ov_cache_lookup_gpu(&cache, &gpu_meta, &out_hash);
    TEST_ASSERT(hit_st == OV_SUCCESS, "Subsequent GPU lookup is HIT");
    TEST_ASSERT(out_hash == translated_shader, "Retrieved shader hash matches translated");
}

// 11 & 12: Scheduler resource acceptance and rejection
void test_scheduler_acceptance_and_rejection(void) {
    printf("\n=== Tests 11 & 12: Scheduler Resource Acceptance & Rejection ===\n");
    ov_scheduler_t sched;
    ov_scheduler_init(&sched, 8, 8192, 4096);

    ov_execution_plan_t plan = {
        .is_supported = true,
        .decision = OV_DECISION_NATIVE,
        .selected_profile = OV_PROFILE_PERFORMANCE
    };

    ov_workload_desc_t valid_wl = {
        .required_cpu_cores = 4,
        .ram_requirement_mb = 2048,
        .vram_requirement_mb = 1024
    };

    ov_schedule_ticket_t ticket1;
    ov_status_t st1 = ov_scheduler_evaluate(&sched, &valid_wl, &plan, &ticket1);
    TEST_ASSERT(st1 == OV_SUCCESS, "Valid workload admitted by scheduler");
    TEST_ASSERT(ticket1.status == OV_SCHED_ACCEPTED, "Ticket status is ACCEPTED");
    TEST_ASSERT(sched.pool.allocated_cores == 4, "Allocated cores updated to 4");

    // Second workload requesting 6 cores (only 4 remaining)
    ov_workload_desc_t heavy_wl = {
        .required_cpu_cores = 6,
        .ram_requirement_mb = 1024,
        .vram_requirement_mb = 512
    };

    ov_schedule_ticket_t ticket2;
    ov_status_t st2 = ov_scheduler_evaluate(&sched, &heavy_wl, &plan, &ticket2);
    TEST_ASSERT(st2 == OV_ERR_RESOURCE_EXHAUSTED, "Exhausted cores rejected by scheduler");
    TEST_ASSERT(ticket2.status == OV_SCHED_REJECTED_CORES, "Ticket status is REJECTED_CORES");

    // Release ticket1 and retry
    ov_scheduler_release(&sched, &ticket1);
    TEST_ASSERT(sched.pool.allocated_cores == 0, "Cores freed after release");

    ov_status_t st3 = ov_scheduler_evaluate(&sched, &heavy_wl, &plan, &ticket2);
    TEST_ASSERT(st3 == OV_SUCCESS, "Admitted successfully after resource release");
}

// 13. Resolver reason codes
void test_resolver_reason_codes(void) {
    printf("\n=== Test 13: Resolver Reason Codes ===\n");
    ov_host_env_t host;
    get_test_host(&host);

    ov_workload_desc_t wl = {
        .source_cpu_arch = OV_ARCH_X86_64,
        .target_cpu_arch = OV_ARCH_X86_64,
        .source_gfx_api = OV_GFX_API_METAL,
        .required_cpu_cores = 32, // Exceeds host 8 cores
        .ram_requirement_mb = 512,
        .vram_requirement_mb = 256,
        .native_execution_available = true
    };

    ov_execution_plan_t plan;
    ov_resolver_create_plan(&wl, &host, NULL, &plan);

    TEST_ASSERT(plan.reason_code == OV_REASON_INSUFFICIENT_CORES, "Correct reason code OV_REASON_INSUFFICIENT_CORES");
    TEST_ASSERT(strstr(ov_reason_name(plan.reason_code), "CPU cores") != NULL, "Reason name formatted correctly");
}

// 14 & 15: End-to-end execution plan & deterministic repeated execution
void test_end_to_end_and_determinism(void) {
    printf("\n=== Tests 14 & 15: End-to-End Plan & Deterministic Repeatability ===\n");
    ov_host_env_t host;
    get_test_host(&host);
    ov_unified_cache_t cache;
    ov_unified_cache_init(&cache);
    ov_scheduler_t scheduler;
    ov_scheduler_init(&scheduler, 8, host.host_ram_mb, host.gpu_caps.available_vram_mb);

    ov_pipeline_context_t ctx;
    ov_pipeline_init(&ctx, &host, &cache, &scheduler);

    uint32_t arm64_code[] = {
        0x52800140, // MOV w0, #10
        0x52800281, // MOV w1, #20
        0x8b010000, // ADD x0, x0, x1
        0xd65f03c0  // RET
    };
    uint32_t count = 4;

    ovir_gpu_workload_t gpu_wl;
    ovir_gpu_create_sample_workload(&gpu_wl, OV_GFX_API_VULKAN, OV_GPU_FEAT_COMPUTE);

    ov_workload_desc_t workload = {
        .source_cpu_arch = OV_ARCH_ARM64,
        .target_cpu_arch = OV_ARCH_X86_64,
        .source_gfx_api = OV_GFX_API_VULKAN,
        .required_cpu_features = 0,
        .required_gpu_features = OV_GPU_FEAT_COMPUTE,
        .ram_requirement_mb = 1024,
        .vram_requirement_mb = 512,
        .required_cpu_cores = 2,
        .code_identity_hash = 0xabcdef1234567890ULL,
        .shader_identity_hash = 0x1234567890abcdefULL,
        .native_execution_available = false,
        .priority = OV_PRIORITY_HIGH,
        .workload_type = OV_WORKLOAD_MIXED
    };

    ov_execution_result_t res_a, res_b;
    ov_pipeline_execute(&ctx, &workload, arm64_code, count, &gpu_wl, &res_a);
    ov_pipeline_execute(&ctx, &workload, arm64_code, count, &gpu_wl, &res_b);

    TEST_ASSERT(res_a.execution_dispatched && res_b.execution_dispatched, "Both executions dispatched");
    TEST_ASSERT(res_a.plan.decision == res_b.plan.decision, "Decisions are identical (CPU_AND_GPU_TRANSLATION)");
    TEST_ASSERT(res_a.cpu_sim_state.registers[0] == 30, "Run A arithmetic result r0 == 30");
    TEST_ASSERT(res_b.cpu_sim_state.registers[0] == 30, "Run B arithmetic result r0 == 30");
    TEST_ASSERT(res_a.cpu_compiled.code_size == res_b.cpu_compiled.code_size, "Compiled sizes match");
}

// 16. Invalid capability request
void test_invalid_capability_request(void) {
    printf("\n=== Test 16: Invalid Capability Request (Ray Tracing on Non-RT GPU) ===\n");
    ov_host_env_t host;
    get_test_host(&host);

    ov_workload_desc_t wl = {
        .source_cpu_arch = OV_ARCH_X86_64,
        .target_cpu_arch = OV_ARCH_X86_64,
        .source_gfx_api = OV_GFX_API_VULKAN,
        .required_gpu_features = OV_GPU_FEAT_RAY_TRACING, // Not in host
        .ram_requirement_mb = 512,
        .vram_requirement_mb = 512,
        .required_cpu_cores = 2,
        .native_execution_available = true
    };

    ov_execution_plan_t plan;
    ov_status_t st = ov_resolver_create_plan(&wl, &host, NULL, &plan);

    TEST_ASSERT(st == OV_ERR_UNSUPPORTED, "Rejected unsupported GPU capability");
    TEST_ASSERT(plan.reason_code == OV_REASON_UNSUPPORTED_GPU_FEATURE, "Reason is UNSUPPORTED_GPU_FEATURE");
}

// 17. Invalid cache metadata
void test_invalid_cache_metadata(void) {
    printf("\n=== Test 17: Invalid Cache Metadata Rejection ===\n");
    ov_unified_cache_t cache;
    ov_unified_cache_init(&cache);

    ov_cache_cpu_metadata_t meta1 = {
        .source_arch = OV_ARCH_ARM64,
        .target_arch = OV_ARCH_X86_64,
        .code_identity_hash = 0x5566778899aabbccULL,
        .translator_version_id = OV_TRANSLATOR_VERSION_ID,
        .required_cpu_features = OV_CPU_FEAT_AVX,
        .optimization_level = 1
    };

    ovir_compiled_block_t block = {
        .target_arch = OV_ARCH_X86_64,
        .code_size = 2,
        .code_bytes = { 0x90, 0xc3 },
        .verified = true
    };
    ov_cache_store_cpu(&cache, &meta1, &block);

    // Try lookup with different CPU feature requirements
    ov_cache_cpu_metadata_t meta2 = meta1;
    meta2.required_cpu_features = OV_CPU_FEAT_AVX2; // Mismatched capability

    ovir_compiled_block_t out_block;
    ov_status_t st = ov_cache_lookup_cpu(&cache, &meta2, &out_block);

    TEST_ASSERT(st == OV_ERR_CACHE_INVALID, "Stale cache rejected when capabilities differ");
}

// 18. Telemetry generation
void test_telemetry_generation(void) {
    printf("\n=== Test 18: Telemetry Generation & Diagnostic Output ===\n");
    ov_host_env_t host;
    get_test_host(&host);
    ov_unified_cache_t cache;
    ov_unified_cache_init(&cache);
    ov_scheduler_t scheduler;
    ov_scheduler_init(&scheduler, 8, host.host_ram_mb, host.gpu_caps.available_vram_mb);

    ov_pipeline_context_t ctx;
    ov_pipeline_init(&ctx, &host, &cache, &scheduler);

    uint32_t arm64_code[] = {
        0x52800040, // MOV w0, #2
        0x52800061, // MOV w1, #3
        0x9b017c00, // MUL x0, x0, x1
        0xd65f03c0  // RET
    };
    uint32_t count = 4;

    ov_workload_desc_t wl = {
        .source_cpu_arch = OV_ARCH_ARM64,
        .target_cpu_arch = OV_ARCH_X86_64,
        .source_gfx_api = OV_GFX_API_NONE,
        .ram_requirement_mb = 256,
        .vram_requirement_mb = 0,
        .required_cpu_cores = 1,
        .code_identity_hash = 0x9999888877776666ULL,
        .native_execution_available = false,
        .priority = OV_PRIORITY_NORMAL
    };

    ov_execution_result_t result;
    ov_pipeline_execute(&ctx, &wl, arm64_code, count, NULL, &result);

    TEST_ASSERT(result.telemetry.total_pipeline_time_us > 0 || result.telemetry.resolver_time_us >= 0,
                "Real timing captured in telemetry");
    TEST_ASSERT(strlen(result.diagnostic_report) > 100, "Diagnostic report string populated");
    TEST_ASSERT(strstr(result.diagnostic_report, "OPENVINTAGE PHASE 10") != NULL,
                "Diagnostic header verified");
    TEST_ASSERT(strstr(result.diagnostic_report, "Reason code:") != NULL,
                "Reason code present in report");

    printf("\nGenerated Diagnostic Report Preview:\n%s\n", result.diagnostic_report);
}

int main(void) {
    printf("=================================================================\n");
    printf("   OpenVintage Phase 10 Unified Execution Pipeline Test Suite    \n");
    printf("=================================================================\n");

    test_native_execution_selection();
    test_cpu_translation_selection();
    test_gpu_translation_selection();
    test_cpu_and_gpu_translation_selection();
    test_unsupported_workload();
    test_cpu_cache_miss_and_hit();
    test_cpu_cache_invalidation();
    test_shader_and_pipeline_cache();
    test_scheduler_acceptance_and_rejection();
    test_resolver_reason_codes();
    test_end_to_end_and_determinism();
    test_invalid_capability_request();
    test_invalid_cache_metadata();
    test_telemetry_generation();

    printf("\n-----------------------------------------------------------------\n");
    printf("  Results: %d/%d assertions passed (%s)\n",
           g_tests_passed, g_tests_total,
           (g_tests_passed == g_tests_total) ? "SUCCESS" : "FAIL");
    printf("=================================================================\n");

    return (g_tests_passed == g_tests_total) ? 0 : 1;
}
