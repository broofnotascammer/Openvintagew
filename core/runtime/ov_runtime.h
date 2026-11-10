#ifndef OV_RUNTIME_H
#define OV_RUNTIME_H

#include "../types/ov_types.h"
#include "../memory/ov_memory.h"
#include "../../ovir/cpu/ovir_cpu.h"

#define OV_MAX_GPRS 32
#define OV_MAX_RUNTIME_TBS 128

typedef enum {
    OV_CPU_STATE_IDLE = 0,
    OV_CPU_STATE_RUNNING,
    OV_CPU_STATE_HALTED,
    OV_CPU_STATE_EXCEPTION
} ov_cpu_exec_state_t;

typedef enum {
    OV_TB_STATE_CREATED = 0,
    OV_TB_STATE_COMPILED,
    OV_TB_STATE_VALID,
    OV_TB_STATE_INVALIDATED,
    OV_TB_STATE_EVICTED,
    OV_TB_STATE_DESTROYED
} ov_tb_state_t;

typedef enum {
    OV_RESIDENCY_COLD = 0,
    OV_RESIDENCY_PERSISTENT_WARM,
    OV_RESIDENCY_RAM_HOT
} ov_cache_residency_t;

typedef struct {
    uint32_t block_id;
    uint64_t guest_start_addr;
    uint64_t guest_end_addr;
    ov_arch_t guest_arch;
    ov_arch_t host_arch;
    uint64_t source_identity_hash;
    uint64_t compiled_code_hash;
    uint32_t code_size;
    ov_tb_state_t state;
    bool is_valid;
    uint32_t generation;
    uint64_t execution_count;
    uint64_t last_used_timestamp;
    ov_cache_residency_t residency;
    ovir_compiled_block_t compiled_code;
    ovir_ir_block_t ir_block;
} ov_translation_block_t;

typedef struct {
    ov_arch_t arch;
    uint64_t gpr[OV_MAX_GPRS];
    uint64_t pc;
    uint64_t sp;
    uint64_t flags;
    ov_cpu_exec_state_t state;
    uint32_t current_tb_id;
    uint32_t thread_id;
    ov_exception_code_t exception;
    uint64_t fault_addr;
    char exception_info[128];
    uint64_t cycles_executed;
    uint64_t instructions_retired;
} ov_cpu_context_t;

typedef struct {
    ov_translation_block_t tbs[OV_MAX_RUNTIME_TBS];
    size_t tb_count;
    uint32_t current_generation;
    uint64_t total_executions;
    uint64_t invalidation_count;
    uint64_t exception_count;
} ov_runtime_t;

void ov_runtime_init(ov_runtime_t *rt);
void ov_cpu_context_init(ov_cpu_context_t *ctx, ov_arch_t arch, uint32_t thread_id);

ov_translation_block_t *ov_runtime_create_tb(
    ov_runtime_t *rt,
    uint64_t guest_start,
    uint64_t guest_end,
    ov_arch_t guest_arch,
    ov_arch_t host_arch,
    uint64_t source_hash
);

ov_status_t ov_runtime_compile_tb(
    ov_translation_block_t *tb,
    const uint32_t *raw_insns,
    uint32_t insn_count
);

ov_translation_block_t *ov_runtime_lookup_tb(
    ov_runtime_t *rt,
    uint64_t guest_pc,
    ov_arch_t guest_arch
);

ov_status_t ov_runtime_execute_tb(
    ov_runtime_t *rt,
    ov_translation_block_t *tb,
    ov_cpu_context_t *ctx,
    ov_guest_memory_t *mem
);

// Invalidation APIs
ov_status_t ov_runtime_invalidate_by_hash(ov_runtime_t *rt, uint64_t source_hash);
ov_status_t ov_runtime_invalidate_range(ov_runtime_t *rt, uint64_t start_addr, uint64_t end_addr);
ov_status_t ov_runtime_invalidate_all(ov_runtime_t *rt);

const char *ov_tb_state_name(ov_tb_state_t state);
const char *ov_exception_name(ov_exception_code_t code);

#endif /* OV_RUNTIME_H */
