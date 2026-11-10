#ifndef OV_TELEMETRY_H
#define OV_TELEMETRY_H

#include "../types/ov_types.h"
#include <time.h>

typedef struct {
    uint64_t resolver_time_us;
    uint64_t cache_lookup_time_us;
    uint64_t translation_time_us;
    uint64_t optimizer_time_us;
    uint64_t emission_time_us;
    uint64_t scheduler_time_us;
    uint64_t total_pipeline_time_us;
    bool cache_hit;
    uint32_t optimizations_applied;
    uint32_t allocated_cores;
    uint64_t allocated_ram_mb;
    ov_status_t execution_status;
} ov_execution_telemetry_t;

typedef struct {
    uint64_t dbt_trace_hits;
    uint64_t dbt_trace_misses;
    uint64_t dbt_hot_loops_detected;
    uint64_t dbt_links_traversed;
    uint64_t neon_insns_translated;
    uint64_t neon_native_vector_execs;
    uint64_t neon_scalar_fallbacks;
    uint64_t mmu_tlb_hits;
    uint64_t mmu_tlb_misses;
    uint64_t mmu_page_walks;
    uint64_t mmu_page_faults;
    // Phase 14 GPU Translation & Execution metrics
    uint64_t gpu_shaders_parsed;
    uint64_t gpu_shaders_translated;
    uint64_t gpu_shaders_optimized;
    uint64_t gpu_shaders_compiled;
    uint64_t gpu_reconstructions_applied;
    uint64_t gpu_cpu_fallbacks_invoked;
} ov_runtime_telemetry_t;

typedef struct timespec ov_timestamp_t;

void ov_timer_mark(ov_timestamp_t *ts);
uint64_t ov_timer_elapsed_us(const ov_timestamp_t *start, const ov_timestamp_t *end);

// Phase 13 Runtime telemetry recording functions
void ov_telemetry_record_dbt_hit(void);
void ov_telemetry_record_dbt_miss(void);
void ov_telemetry_record_dbt_hot_loop(void);
void ov_telemetry_record_dbt_link(void);
void ov_telemetry_record_neon_insn(void);
void ov_telemetry_record_neon_native(void);
void ov_telemetry_record_neon_fallback(void);
void ov_telemetry_record_mmu_tlb_hit(void);
void ov_telemetry_record_mmu_tlb_miss(void);
void ov_telemetry_record_mmu_page_walk(void);
void ov_telemetry_record_mmu_page_fault(void);

// Phase 14 GPU telemetry recording functions
void ov_telemetry_record_gpu_shader_parsed(void);
void ov_telemetry_record_gpu_shader_translated(void);
void ov_telemetry_record_gpu_shader_optimized(void);
void ov_telemetry_record_gpu_shader_compiled(void);
void ov_telemetry_record_gpu_reconstruction(void);
void ov_telemetry_record_gpu_cpu_fallback(void);

const ov_runtime_telemetry_t *ov_telemetry_get_runtime_stats(void);
void ov_telemetry_reset_runtime_stats(void);

#endif /* OV_TELEMETRY_H */
