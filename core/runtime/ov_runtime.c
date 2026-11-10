#include "ov_runtime.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

void ov_runtime_init(ov_runtime_t *rt) {
    if (!rt) return;
    memset(rt, 0, sizeof(ov_runtime_t));
    rt->current_generation = 1;
}

void ov_cpu_context_init(ov_cpu_context_t *ctx, ov_arch_t arch, uint32_t thread_id) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(ov_cpu_context_t));
    ctx->arch = arch;
    ctx->thread_id = thread_id;
    ctx->state = OV_CPU_STATE_IDLE;
    ctx->sp = 0x7FFF0000; // Default guest stack base
}

ov_translation_block_t *ov_runtime_create_tb(
    ov_runtime_t *rt,
    uint64_t guest_start,
    uint64_t guest_end,
    ov_arch_t guest_arch,
    ov_arch_t host_arch,
    uint64_t source_hash
) {
    if (!rt || rt->tb_count >= OV_MAX_RUNTIME_TBS) return NULL;

    ov_translation_block_t *tb = &rt->tbs[rt->tb_count++];
    memset(tb, 0, sizeof(ov_translation_block_t));
    tb->block_id = (uint32_t)rt->tb_count;
    tb->guest_start_addr = guest_start;
    tb->guest_end_addr = guest_end;
    tb->guest_arch = guest_arch;
    tb->host_arch = host_arch;
    tb->source_identity_hash = source_hash;
    tb->state = OV_TB_STATE_CREATED;
    tb->is_valid = false;
    tb->generation = rt->current_generation;
    tb->residency = OV_RESIDENCY_COLD;
    tb->last_used_timestamp = (uint64_t)time(NULL);

    return tb;
}

ov_status_t ov_runtime_compile_tb(
    ov_translation_block_t *tb,
    const uint32_t *raw_insns,
    uint32_t insn_count
) {
    if (!tb || !raw_insns || insn_count == 0) return OV_ERR_INVALID_PARAM;

    // Decode via OVIR-CPU
    ov_status_t st = ovir_cpu_decode(raw_insns, insn_count, &tb->ir_block);
    if (st != OV_SUCCESS) {
        tb->state = OV_TB_STATE_DESTROYED;
        return st;
    }

    // Optimize
    uint32_t opts = 0;
    ovir_cpu_optimize(&tb->ir_block, &opts);

    // Emit x86-64 machine code
    st = ovir_cpu_emit_x86_64(&tb->ir_block, &tb->compiled_code);
    if (st != OV_SUCCESS) {
        tb->state = OV_TB_STATE_DESTROYED;
        return st;
    }

    tb->code_size = tb->compiled_code.code_size;
    tb->compiled_code_hash = 0x9e3779b97f4a7c15ULL ^ (uint64_t)tb->code_size;
    tb->state = OV_TB_STATE_VALID;
    tb->is_valid = true;

    return OV_SUCCESS;
}

ov_translation_block_t *ov_runtime_lookup_tb(
    ov_runtime_t *rt,
    uint64_t guest_pc,
    ov_arch_t guest_arch
) {
    if (!rt) return NULL;

    for (size_t i = 0; i < rt->tb_count; i++) {
        ov_translation_block_t *tb = &rt->tbs[i];
        if (tb->is_valid && tb->state == OV_TB_STATE_VALID &&
            tb->guest_arch == guest_arch &&
            guest_pc >= tb->guest_start_addr && guest_pc < tb->guest_end_addr) {
            tb->last_used_timestamp = (uint64_t)time(NULL);
            return tb;
        }
    }
    return NULL;
}

ov_status_t ov_runtime_execute_tb(
    ov_runtime_t *rt,
    ov_translation_block_t *tb,
    ov_cpu_context_t *ctx,
    ov_guest_memory_t *mem
) {
    if (!rt || !tb || !ctx) return OV_ERR_INVALID_PARAM;

    // Security Gate: NEVER execute invalid or corrupted translation blocks
    if (!tb->is_valid || tb->state != OV_TB_STATE_VALID) {
        ctx->state = OV_CPU_STATE_EXCEPTION;
        ctx->exception = OV_EXC_INVALID_TRANSLATION_BLOCK;
        snprintf(ctx->exception_info, sizeof(ctx->exception_info),
                 "Attempted execution of invalid/stale TB #%u", tb->block_id);
        rt->exception_count++;
        return OV_ERR_SECURITY_VIOLATION;
    }

    // Verify memory bounds for execution permission if guest memory is mapped
    if (mem && tb->guest_start_addr != 0) {
        void *host_exec_ptr = NULL;
        ov_status_t mem_st = ov_mem_translate(mem, tb->guest_start_addr, tb->guest_end_addr - tb->guest_start_addr,
                                              OV_MEM_EXECUTE, &host_exec_ptr);
        if (mem_st != OV_SUCCESS) {
            ctx->state = OV_CPU_STATE_EXCEPTION;
            ctx->exception = OV_EXC_EXECUTE_VIOLATION;
            ctx->fault_addr = tb->guest_start_addr;
            snprintf(ctx->exception_info, sizeof(ctx->exception_info),
                     "Execute permission violation at guest 0x%llx", (unsigned long long)tb->guest_start_addr);
            rt->exception_count++;
            return OV_ERR_SECURITY_VIOLATION;
        }
    }

    ctx->state = OV_CPU_STATE_RUNNING;
    ctx->current_tb_id = tb->block_id;

    // Map context registers into OVIR-CPU simulator state
    ovir_cpu_state_t sim_state;
    memset(&sim_state, 0, sizeof(ovir_cpu_state_t));
    for (int i = 0; i < 8; i++) {
        sim_state.registers[i] = ctx->gpr[i];
    }

    ov_status_t sim_st = ovir_cpu_simulate(&tb->ir_block, &sim_state);
    if (sim_st != OV_SUCCESS) {
        ctx->state = OV_CPU_STATE_EXCEPTION;
        ctx->exception = OV_EXC_INVALID_INSTRUCTION;
        rt->exception_count++;
        return sim_st;
    }

    // Write back updated registers
    for (int i = 0; i < 8; i++) {
        ctx->gpr[i] = sim_state.registers[i];
    }
    ctx->pc = tb->guest_end_addr;
    ctx->cycles_executed += sim_state.cycles;
    ctx->instructions_retired += tb->ir_block.count;
    ctx->state = OV_CPU_STATE_IDLE;

    tb->execution_count++;
    rt->total_executions++;

    return OV_SUCCESS;
}

ov_status_t ov_runtime_invalidate_by_hash(ov_runtime_t *rt, uint64_t source_hash) {
    if (!rt) return OV_ERR_INVALID_PARAM;
    for (size_t i = 0; i < rt->tb_count; i++) {
        if (rt->tbs[i].source_identity_hash == source_hash) {
            rt->tbs[i].is_valid = false;
            rt->tbs[i].state = OV_TB_STATE_INVALIDATED;
            rt->invalidation_count++;
        }
    }
    return OV_SUCCESS;
}

ov_status_t ov_runtime_invalidate_range(ov_runtime_t *rt, uint64_t start_addr, uint64_t end_addr) {
    if (!rt) return OV_ERR_INVALID_PARAM;
    for (size_t i = 0; i < rt->tb_count; i++) {
        ov_translation_block_t *tb = &rt->tbs[i];
        if (tb->is_valid && !(tb->guest_end_addr <= start_addr || tb->guest_start_addr >= end_addr)) {
            tb->is_valid = false;
            tb->state = OV_TB_STATE_INVALIDATED;
            rt->invalidation_count++;
        }
    }
    return OV_SUCCESS;
}

ov_status_t ov_runtime_invalidate_all(ov_runtime_t *rt) {
    if (!rt) return OV_ERR_INVALID_PARAM;
    for (size_t i = 0; i < rt->tb_count; i++) {
        if (rt->tbs[i].is_valid) {
            rt->tbs[i].is_valid = false;
            rt->tbs[i].state = OV_TB_STATE_INVALIDATED;
            rt->invalidation_count++;
        }
    }
    rt->current_generation++;
    return OV_SUCCESS;
}

const char *ov_tb_state_name(ov_tb_state_t state) {
    switch (state) {
        case OV_TB_STATE_CREATED: return "CREATED";
        case OV_TB_STATE_COMPILED: return "COMPILED";
        case OV_TB_STATE_VALID: return "VALID";
        case OV_TB_STATE_INVALIDATED: return "INVALIDATED";
        case OV_TB_STATE_EVICTED: return "EVICTED";
        case OV_TB_STATE_DESTROYED: return "DESTROYED";
        default: return "UNKNOWN";
    }
}

const char *ov_exception_name(ov_exception_code_t code) {
    switch (code) {
        case OV_EXC_NONE: return "NONE";
        case OV_EXC_INVALID_MEMORY: return "INVALID_MEMORY";
        case OV_EXC_EXECUTE_VIOLATION: return "EXECUTE_VIOLATION";
        case OV_EXC_INVALID_INSTRUCTION: return "INVALID_INSTRUCTION";
        case OV_EXC_TRANSLATION_FAILURE: return "TRANSLATION_FAILURE";
        case OV_EXC_INVALID_TRANSLATION_BLOCK: return "INVALID_TRANSLATION_BLOCK";
        case OV_EXC_GUEST_FAULT: return "GUEST_FAULT";
        case OV_EXC_RESOURCE_LIMIT: return "RESOURCE_LIMIT";
        case OV_EXC_INTERRUPTED: return "INTERRUPTED";
        default: return "UNKNOWN_EXCEPTION";
    }
}
