#ifndef OVIR_CPU_TRACE_H
#define OVIR_CPU_TRACE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../../core/types/ov_types.h"
#include "ovir_cpu.h"

#define OV_HOT_TRACE_THRESHOLD 5
#define OV_MAX_TRACE_BLOCKS 16
#define OV_MAX_TRACES 64

typedef struct {
    uint32_t trace_id;
    uint64_t start_guest_pc;
    uint64_t end_guest_pc;
    uint32_t block_ids[OV_MAX_TRACE_BLOCKS];
    size_t block_count;
    uint64_t execution_count;
    uint64_t source_identity_hash;
    uint32_t generation;
    bool is_valid;
    bool is_compiled;
    bool is_hot_loop;
    ovir_compiled_block_t compiled_code;
} ov_cpu_trace_t;

typedef struct {
    ov_cpu_trace_t traces[OV_MAX_TRACES];
    size_t count;
    uint32_t hot_threshold;
    uint64_t trace_hits;
    uint64_t trace_misses;
    uint64_t hot_loops_detected;
    uint64_t traces_compiled;
    uint64_t traces_invalidated;
    bool trace_compilation_enabled;
    uint32_t current_generation;
} ov_cpu_trace_cache_t;

void ov_trace_cache_init(ov_cpu_trace_cache_t *tc);

ov_cpu_trace_t *ov_trace_lookup(
    ov_cpu_trace_cache_t *tc,
    uint64_t guest_pc
);

ov_cpu_trace_t *ov_trace_record_execution(
    ov_cpu_trace_cache_t *tc,
    uint32_t block_id,
    uint64_t start_pc,
    uint64_t end_pc,
    uint64_t source_hash,
    bool is_backward_jump
);

ov_status_t ov_trace_compile(
    ov_cpu_trace_cache_t *tc,
    ov_cpu_trace_t *trace
);

ov_status_t ov_trace_invalidate_by_hash(
    ov_cpu_trace_cache_t *tc,
    uint64_t source_hash
);

ov_status_t ov_trace_invalidate_range(
    ov_cpu_trace_cache_t *tc,
    uint64_t start,
    uint64_t end
);

ov_status_t ov_trace_invalidate_all(
    ov_cpu_trace_cache_t *tc
);

#endif /* OVIR_CPU_TRACE_H */
