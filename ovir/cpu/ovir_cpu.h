#ifndef OVIR_CPU_H
#define OVIR_CPU_H

#include "../../core/types/ov_types.h"

#define OVIR_CPU_MAX_INSTRUCTIONS 256
#define OVIR_CPU_MAX_CODE_SIZE 2048
#define OVIR_CPU_REG_COUNT 32

// Condition codes for branches and conditional execution
typedef enum {
    OVIR_CC_AL = 0, // Always
    OVIR_CC_EQ,     // Equal (Z == 1)
    OVIR_CC_NE,     // Not Equal (Z == 0)
    OVIR_CC_CS,     // Carry Set / Unsigned Higher or Same (C == 1)
    OVIR_CC_CC,     // Carry Clear / Unsigned Lower (C == 0)
    OVIR_CC_MI,     // Minus / Negative (N == 1)
    OVIR_CC_PL,     // Plus / Positive or Zero (N == 0)
    OVIR_CC_VS,     // Overflow Set (V == 1)
    OVIR_CC_VC,     // Overflow Clear (V == 0)
    OVIR_CC_HI,     // Unsigned Higher (C == 1 && Z == 0)
    OVIR_CC_LS,     // Unsigned Lower or Same (C == 0 || Z == 1)
    OVIR_CC_GE,     // Signed Greater than or Equal (N == V)
    OVIR_CC_LT,     // Signed Less than (N != V)
    OVIR_CC_GT,     // Signed Greater than (Z == 0 && N == V)
    OVIR_CC_LE      // Signed Less than or Equal (Z == 1 || N != V)
} ovir_condition_t;

// CPU Status Flags
#define OV_CPU_FLAG_Z (1ULL << 0) // Zero
#define OV_CPU_FLAG_N (1ULL << 1) // Negative / Sign
#define OV_CPU_FLAG_C (1ULL << 2) // Carry
#define OV_CPU_FLAG_V (1ULL << 3) // Overflow

typedef enum {
    OVIR_OP_NOP = 0,
    OVIR_OP_MOV_IMM,
    OVIR_OP_MOV_REG,
    OVIR_OP_MVN,
    OVIR_OP_ADD,
    OVIR_OP_ADDS,
    OVIR_OP_SUB,
    OVIR_OP_SUBS,
    OVIR_OP_MUL,
    OVIR_OP_NEG,
    OVIR_OP_AND,
    OVIR_OP_ANDS,
    OVIR_OP_ORR,
    OVIR_OP_EOR,
    OVIR_OP_LSL,
    OVIR_OP_LSR,
    OVIR_OP_ASR,
    OVIR_OP_CMP,
    OVIR_OP_TST,
    OVIR_OP_LOAD,
    OVIR_OP_STORE,
    OVIR_OP_JMP,
    OVIR_OP_JMP_COND,
    OVIR_OP_CBZ,
    OVIR_OP_CBNZ,
    OVIR_OP_CALL,
    OVIR_OP_RET,
    OVIR_OP_BARRIER,
    OVIR_OP_SYSCALL,
    // Phase 13 ARM64 NEON Vector Opcodes
    OVIR_OP_VEC_ADD,
    OVIR_OP_VEC_SUB,
    OVIR_OP_VEC_MUL,
    OVIR_OP_VEC_AND,
    OVIR_OP_VEC_ORR,
    OVIR_OP_VEC_EOR,
    OVIR_OP_VEC_DUP,
    OVIR_OP_VEC_MOVI
} ovir_opcode_t;

// Phase 13 Vector Arrangements
typedef enum {
    OVIR_VEC_ARRANGE_NONE = 0,
    OVIR_VEC_ARRANGE_16B, // 16 lanes x 8-bit
    OVIR_VEC_ARRANGE_8H,  // 8 lanes x 16-bit
    OVIR_VEC_ARRANGE_4S,  // 4 lanes x 32-bit (Single)
    OVIR_VEC_ARRANGE_2D   // 2 lanes x 64-bit (Double)
} ovir_vec_arrangement_t;

#define OVIR_VEC_REG_COUNT 32

typedef struct {
    ovir_opcode_t opcode;
    uint8_t dst_reg;
    uint8_t src_reg1;
    uint8_t src_reg2;
    int64_t imm_val;
    int32_t offset;
    bool is_constant;
    uint8_t op_size; // 1, 2, 4, 8 bytes
    ovir_condition_t cond;
    bool sets_flags;
    uint64_t guest_pc;
    int64_t branch_target_disp; // relative target displacement in bytes
    char debug_mnemonic[32];
    bool is_vector;
    ovir_vec_arrangement_t vec_arrangement;
    uint8_t vec_lane;
} ovir_insn_t;

typedef struct {
    ovir_insn_t instructions[OVIR_CPU_MAX_INSTRUCTIONS];
    uint32_t count;
    ov_arch_t source_arch;
    uint64_t source_code_hash;
    uint64_t start_guest_pc;
    uint64_t end_guest_pc;
    bool has_branches;
    bool has_memory_access;
    bool has_vector_ops;
} ovir_ir_block_t;

typedef struct {
    uint8_t code_bytes[OVIR_CPU_MAX_CODE_SIZE];
    uint32_t code_size;
    ov_arch_t target_arch;
    uint32_t instruction_count;
    bool verified;
    uint32_t jit_entry_offset;
    bool uses_vector_backend;
} ovir_compiled_block_t;

typedef struct {
    uint64_t registers[OVIR_CPU_REG_COUNT];
    ovir_vec128_t vregs[OVIR_VEC_REG_COUNT];
    uint64_t pc;
    uint64_t sp;
    uint64_t flags;
    bool halted;
    uint32_t cycles;
    uint32_t instructions_retired;
    ov_exception_code_t exception;
    uint64_t fault_addr;
} ovir_cpu_state_t;

// OVIR-CPU Pipeline APIs
ov_status_t ovir_cpu_decode(const uint32_t *raw_arm64_insns, uint32_t count, ovir_ir_block_t *out_ir);
ov_status_t ovir_cpu_optimize(ovir_ir_block_t *ir, uint32_t *out_optimizations_applied);
ov_status_t ovir_cpu_emit_x86_64(const ovir_ir_block_t *ir, ovir_compiled_block_t *out_compiled);
ov_status_t ovir_cpu_verify(const ovir_compiled_block_t *compiled);
ov_status_t ovir_cpu_simulate(const ovir_ir_block_t *ir, ovir_cpu_state_t *state);

// Phase 12 Extended APIs
ov_status_t ovir_cpu_ir_validate(const ovir_ir_block_t *ir, char *out_err, size_t err_len);
int ovir_cpu_ir_dump(const ovir_ir_block_t *ir, char *buf, size_t buf_len);
const char *ovir_opcode_name(ovir_opcode_t op);
const char *ovir_condition_name(ovir_condition_t cond);

// Phase 12 Real JIT Execution on Host x86-64
ov_status_t ovir_cpu_execute_native(
    const ovir_compiled_block_t *compiled,
    ovir_cpu_state_t *state
);

#endif /* OVIR_CPU_H */
