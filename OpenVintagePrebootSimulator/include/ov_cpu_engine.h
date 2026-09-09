/**
 * OpenVintage Pre-Boot Simulator - OVIR-CPU Translation Engine (Phase 4)
 * Architecture-agnostic intermediate representation, optimizer passes, and JIT simulation.
 */

#ifndef OV_CPU_ENGINE_H
#define OV_CPU_ENGINE_H

#include "ov_types.h"

/* Opcodes */
typedef enum {
    OV_CPU_OP_NOP = 0,
    OV_CPU_OP_MOV,
    OV_CPU_OP_LOAD,
    OV_CPU_OP_STORE,
    OV_CPU_OP_ADD,
    OV_CPU_OP_SUB,
    OV_CPU_OP_MUL,
    OV_CPU_OP_DIV,
    OV_CPU_OP_AND,
    OV_CPU_OP_OR,
    OV_CPU_OP_XOR,
    OV_CPU_OP_SHL,
    OV_CPU_OP_SHR,
    OV_CPU_OP_CMP,
    OV_CPU_OP_JMP,
    OV_CPU_OP_JCC,
    OV_CPU_OP_CALL,
    OV_CPU_OP_RET,
    OV_CPU_OP_VEC_ADD,
    OV_CPU_OP_VEC_MUL
} ov_cpu_opcode_t;

/* Operand Kinds */
typedef enum {
    OV_CPU_OPND_NONE = 0,
    OV_CPU_OPND_REG,
    OV_CPU_OPND_IMM,
    OV_CPU_OPND_MEM,
    OV_CPU_OPND_LABEL
} ov_cpu_operand_kind_t;

/* Operand */
typedef struct {
    ov_cpu_operand_kind_t kind;
    uint16_t reg_index;
    int64_t  imm_value;
    int64_t  mem_disp;
    uint32_t label_id;
} ov_cpu_operand_t;

/* Instruction */
typedef struct {
    ov_cpu_opcode_t  opcode;
    uint16_t         bit_width;      /* 32, 64, 128, 256 */
    bool             set_flags;
    ov_cpu_operand_t dst;
    ov_cpu_operand_t src1;
    ov_cpu_operand_t src2;
    uint64_t         guest_pc;
    bool             is_dead;        /* Marked during DCE pass */
} ov_cpu_instruction_t;

#define OV_CPU_MAX_BLOCK_INSTR 32
#define OV_CPU_MAX_BLOCKS      16

/* Basic Block */
typedef struct {
    uint32_t             block_id;
    uint64_t             start_pc;
    uint32_t             instr_count;
    ov_cpu_instruction_t instructions[OV_CPU_MAX_BLOCK_INSTR];
    uint32_t             successors[2];
    uint32_t             successor_count;
} ov_cpu_basic_block_t;

/* Program */
typedef struct {
    uint32_t             program_id;
    char                 program_name[64];
    uint32_t             source_arch;    /* 1 = ARM64, 2 = x86_64 */
    uint32_t             target_arch;    /* 2 = x86_64 */
    uint32_t             block_count;
    ov_cpu_basic_block_t blocks[OV_CPU_MAX_BLOCKS];
    uint32_t             total_instructions;
} ov_cpu_program_t;

/* Optimizer Pass Options & Results */
typedef struct {
    bool     enable_constant_folding;
    bool     enable_dce;                 /* Dead Code Elimination */
    bool     enable_move_elimination;
    uint32_t constants_folded;
    uint32_t dead_instr_removed;
    uint32_t moves_eliminated;
    uint32_t original_instr_count;
    uint32_t optimized_instr_count;
    float    reduction_ratio;
} ov_cpu_optimizer_stats_t;

/* JIT Code Cache Entry */
typedef struct {
    uint64_t guest_pc;
    uint64_t code_hash;
    uint32_t native_code_size;
    uint32_t execution_count;
    bool     is_valid;
} ov_cpu_jit_entry_t;

#define OV_CPU_JIT_CACHE_MAX 64

typedef struct {
    ov_cpu_jit_entry_t entries[OV_CPU_JIT_CACHE_MAX];
    uint32_t           entry_count;
    uint64_t           cache_hits;
    uint64_t           cache_misses;
} ov_cpu_jit_cache_t;

/* APIs */
ov_status_t ov_cpu_engine_init(void);
void        ov_cpu_engine_cleanup(void);

/* Sample Program Builders */
ov_status_t ov_cpu_build_sample_program(int sample_id, ov_cpu_program_t *out_program);

/* Optimizer */
ov_status_t ov_cpu_optimize_program(
    ov_cpu_program_t         *program,
    bool                      enable_folding,
    bool                      enable_dce,
    bool                      enable_move_elim,
    ov_cpu_optimizer_stats_t *out_stats
);

/* JIT Cache Simulation */
ov_status_t ov_cpu_jit_lookup(uint64_t guest_pc, uint64_t code_hash, bool *out_hit);
void        ov_cpu_jit_cache_clear(void);
const ov_cpu_jit_cache_t* ov_cpu_get_jit_cache(void);

/* Formatting */
const char* ov_cpu_opcode_to_string(ov_cpu_opcode_t opcode);
void        ov_cpu_format_program_ir(const ov_cpu_program_t *program, char *out_buf, size_t max_len);

#endif /* OV_CPU_ENGINE_H */
