#include "ov_telemetry.h"
#include <string.h>

static ov_runtime_telemetry_t s_runtime_stats = {0};

void ov_timer_mark(ov_timestamp_t *ts) {
    if (!ts) return;
    clock_gettime(CLOCK_MONOTONIC, ts);
}

uint64_t ov_timer_elapsed_us(const ov_timestamp_t *start, const ov_timestamp_t *end) {
    if (!start || !end) return 0;
    int64_t sec = (int64_t)end->tv_sec - (int64_t)start->tv_sec;
    int64_t nsec = (int64_t)end->tv_nsec - (int64_t)start->tv_nsec;
    int64_t total_us = (sec * 1000000LL) + (nsec / 1000LL);
    return (total_us > 0) ? (uint64_t)total_us : 0;
}

void ov_telemetry_record_dbt_hit(void) {
    s_runtime_stats.dbt_trace_hits++;
}

void ov_telemetry_record_dbt_miss(void) {
    s_runtime_stats.dbt_trace_misses++;
}

void ov_telemetry_record_dbt_hot_loop(void) {
    s_runtime_stats.dbt_hot_loops_detected++;
}

void ov_telemetry_record_dbt_link(void) {
    s_runtime_stats.dbt_links_traversed++;
}

void ov_telemetry_record_neon_insn(void) {
    s_runtime_stats.neon_insns_translated++;
}

void ov_telemetry_record_neon_native(void) {
    s_runtime_stats.neon_native_vector_execs++;
}

void ov_telemetry_record_neon_fallback(void) {
    s_runtime_stats.neon_scalar_fallbacks++;
}

void ov_telemetry_record_mmu_tlb_hit(void) {
    s_runtime_stats.mmu_tlb_hits++;
}

void ov_telemetry_record_mmu_tlb_miss(void) {
    s_runtime_stats.mmu_tlb_misses++;
}

void ov_telemetry_record_mmu_page_walk(void) {
    s_runtime_stats.mmu_page_walks++;
}

void ov_telemetry_record_mmu_page_fault(void) {
    s_runtime_stats.mmu_page_faults++;
}

void ov_telemetry_record_gpu_shader_parsed(void) {
    s_runtime_stats.gpu_shaders_parsed++;
}

void ov_telemetry_record_gpu_shader_translated(void) {
    s_runtime_stats.gpu_shaders_translated++;
}

void ov_telemetry_record_gpu_shader_optimized(void) {
    s_runtime_stats.gpu_shaders_optimized++;
}

void ov_telemetry_record_gpu_shader_compiled(void) {
    s_runtime_stats.gpu_shaders_compiled++;
}

void ov_telemetry_record_gpu_reconstruction(void) {
    s_runtime_stats.gpu_reconstructions_applied++;
}

void ov_telemetry_record_gpu_cpu_fallback(void) {
    s_runtime_stats.gpu_cpu_fallbacks_invoked++;
}

const ov_runtime_telemetry_t *ov_telemetry_get_runtime_stats(void) {
    return &s_runtime_stats;
}

void ov_telemetry_reset_runtime_stats(void) {
    memset(&s_runtime_stats, 0, sizeof(s_runtime_stats));
}
