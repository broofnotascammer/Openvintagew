/**
 * OpenVintage Pre-Boot Simulator - Reproducible Benchmark Framework (Phase 5)
 * Empirical measurement of baseline vs OpenVintage optimized execution.
 */

#ifndef OV_BENCHMARK_H
#define OV_BENCHMARK_H

#include "ov_types.h"

/* Individual Workload Metric */
typedef struct {
    char     workload_name[48];

    /* Baseline Metrics */
    uint64_t baseline_cycles;
    uint32_t baseline_duration_us;
    uint64_t baseline_memory_bytes;
    uint32_t baseline_instruction_count;

    /* Optimized Metrics */
    uint64_t optimized_cycles;
    uint32_t optimized_duration_us;
    uint64_t optimized_memory_bytes;
    uint32_t optimized_instruction_count;

    /* Differential Measurements */
    uint64_t cycles_saved;
    uint32_t speedup_percent;           /* e.g. 145 = 1.45x */
    uint32_t instruction_reduction_pct;
    uint64_t memory_saved_bytes;
    uint32_t cache_hit_latency_ns;
} ov_benchmark_metric_t;

/* Full Benchmark Suite Results */
typedef struct {
    ov_benchmark_metric_t alu_benchmark;         /* Test 1: Arithmetic & Constant Folding */
    ov_benchmark_metric_t dce_benchmark;         /* Test 2: Dead Code & Move Elimination */
    ov_benchmark_metric_t cache_benchmark;       /* Test 3: Cold Miss vs Warm Cache */
    ov_benchmark_metric_t jit_pipeline_benchmark;/* Test 4: End-to-End JIT Decode-Optimize-Emit */

    uint64_t total_baseline_cycles;
    uint64_t total_optimized_cycles;
    uint64_t total_cycles_saved;
    uint32_t overall_speedup_percent;
    uint32_t cache_efficiency_gain_pct;
    bool     has_run;
} ov_benchmark_results_t;

/* Subsystem APIs */
ov_status_t ov_benchmark_init(void);
void        ov_benchmark_cleanup(void);

ov_status_t ov_benchmark_run_suite(ov_benchmark_results_t *out_results);
const ov_benchmark_results_t* ov_benchmark_get_latest_results(void);
void        ov_benchmark_format_summary(const ov_benchmark_results_t *results, char *out_buf, size_t max_len);

#endif /* OV_BENCHMARK_H */
