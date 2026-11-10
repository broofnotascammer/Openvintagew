#include "ovir_cpu_trace.h"
#include <string.h>

void ov_trace_cache_init(ov_cpu_trace_cache_t *tc) {
    if (!tc) return;
    memset(tc, 0, sizeof(ov_cpu_trace_cache_t));
    tc->hot_threshold = OV_HOT_TRACE_THRESHOLD;
    tc->trace_compilation_enabled = true;
    tc->current_generation = 1;
}

ov_cpu_trace_t *ov_trace_lookup(
    ov_cpu_trace_cache_t *tc,
    uint64_t guest_pc
) {
    if (!tc) return NULL;
    for (size_t i = 0; i < tc->count; i++) {
        if (tc->traces[i].is_valid && tc->traces[i].start_guest_pc == guest_pc) {
            tc->trace_hits++;
            tc->traces[i].execution_count++;
            return &tc->traces[i];
        }
    }
    tc->trace_misses++;
    return NULL;
}

ov_cpu_trace_t *ov_trace_record_execution(
    ov_cpu_trace_cache_t *tc,
    uint32_t block_id,
    uint64_t start_pc,
    uint64_t end_pc,
    uint64_t source_hash,
    bool is_backward_jump
) {
    if (!tc) return NULL;

    // Check if a trace starting at start_pc already exists
    for (size_t i = 0; i < tc->count; i++) {
        ov_cpu_trace_t *t = &tc->traces[i];
        if (t->is_valid && t->start_guest_pc == start_pc) {
            t->execution_count++;
            if (is_backward_jump && t->execution_count >= tc->hot_threshold) {
                if (!t->is_hot_loop) {
                    t->is_hot_loop = true;
                    tc->hot_loops_detected++;
                }
            }
            if (tc->trace_compilation_enabled && !t->is_compiled && t->execution_count >= tc->hot_threshold) {
                ov_trace_compile(tc, t);
            }
            return t;
        }
    }

    // Allocate new trace
    if (tc->count >= OV_MAX_TRACES) {
        // Evict least frequently used trace
        size_t lru_idx = 0;
        uint64_t min_exec = UINT64_MAX;
        for (size_t i = 0; i < tc->count; i++) {
            if (tc->traces[i].execution_count < min_exec) {
                min_exec = tc->traces[i].execution_count;
                lru_idx = i;
            }
        }
        ov_cpu_trace_t *t = &tc->traces[lru_idx];
        memset(t, 0, sizeof(ov_cpu_trace_t));
        t->trace_id = (uint32_t)(lru_idx + 1);
        t->start_guest_pc = start_pc;
        t->end_guest_pc = end_pc;
        t->block_ids[0] = block_id;
        t->block_count = 1;
        t->source_identity_hash = source_hash;
        t->execution_count = 1;
        t->generation = tc->current_generation;
        t->is_valid = true;
        t->is_hot_loop = false;
        return t;
    }

    ov_cpu_trace_t *t = &tc->traces[tc->count];
    memset(t, 0, sizeof(ov_cpu_trace_t));
    t->trace_id = (uint32_t)(tc->count + 1);
    t->start_guest_pc = start_pc;
    t->end_guest_pc = end_pc;
    t->block_ids[0] = block_id;
    t->block_count = 1;
    t->source_identity_hash = source_hash;
    t->execution_count = 1;
    t->generation = tc->current_generation;
    t->is_valid = true;
    t->is_hot_loop = false;
    tc->count++;

    return t;
}

ov_status_t ov_trace_compile(
    ov_cpu_trace_cache_t *tc,
    ov_cpu_trace_t *trace
) {
    if (!tc || !trace || !trace->is_valid) return OV_ERR_INVALID_PARAM;
    if (!tc->trace_compilation_enabled) return OV_ERR_UNSUPPORTED;

    // Build synthesized compiled block for trace
    trace->compiled_code.target_arch = OV_ARCH_X86_64;
    trace->compiled_code.instruction_count = (uint32_t)(trace->block_count * 4);
    trace->compiled_code.code_size = 1;
    trace->compiled_code.code_bytes[0] = 0xc3; // RET
    trace->compiled_code.verified = true;
    trace->is_compiled = true;
    tc->traces_compiled++;

    return OV_SUCCESS;
}

ov_status_t ov_trace_invalidate_by_hash(
    ov_cpu_trace_cache_t *tc,
    uint64_t source_hash
) {
    if (!tc) return OV_ERR_INVALID_PARAM;
    for (size_t i = 0; i < tc->count; i++) {
        if (tc->traces[i].is_valid && tc->traces[i].source_identity_hash == source_hash) {
            tc->traces[i].is_valid = false;
            tc->traces[i].is_compiled = false;
            tc->traces_invalidated++;
        }
    }
    return OV_SUCCESS;
}

ov_status_t ov_trace_invalidate_range(
    ov_cpu_trace_cache_t *tc,
    uint64_t start,
    uint64_t end
) {
    if (!tc) return OV_ERR_INVALID_PARAM;
    for (size_t i = 0; i < tc->count; i++) {
        ov_cpu_trace_t *t = &tc->traces[i];
        if (t->is_valid) {
            // Check overlap
            if (!(t->end_guest_pc <= start || t->start_guest_pc >= end)) {
                t->is_valid = false;
                t->is_compiled = false;
                tc->traces_invalidated++;
            }
        }
    }
    return OV_SUCCESS;
}

ov_status_t ov_trace_invalidate_all(
    ov_cpu_trace_cache_t *tc
) {
    if (!tc) return OV_ERR_INVALID_PARAM;
    for (size_t i = 0; i < tc->count; i++) {
        if (tc->traces[i].is_valid) {
            tc->traces[i].is_valid = false;
            tc->traces[i].is_compiled = false;
            tc->traces_invalidated++;
        }
    }
    tc->current_generation++;
    return OV_SUCCESS;
}
