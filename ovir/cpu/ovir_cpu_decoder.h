#ifndef OVIR_CPU_DECODER_H
#define OVIR_CPU_DECODER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../../core/types/ov_types.h"
#include "ovir_cpu.h"

// Structured diagnostic codes for decoding failures
typedef enum {
    OV_DECODE_SUCCESS = 0,
    OV_DECODE_ERR_UNKNOWN_OPCODE = -1,
    OV_DECODE_ERR_MALFORMED = -2,
    OV_DECODE_ERR_UNSUPPORTED_OPERAND = -3,
    OV_DECODE_ERR_UNALIGNED_BRANCH = -4,
    OV_DECODE_ERR_INVALID_REG = -5,
    OV_DECODE_ERR_BUFFER_OVERFLOW = -6,
    OV_DECODE_ERR_UNSUPPORTED_PREFIX = -7
} ov_decode_status_t;

typedef struct {
    uint64_t guest_pc;
    uint32_t raw_insn;
    ov_decode_status_t error_code;
    char mnemonic_guess[32];
    char description[128];
    char suggested_fallback[64];
} ov_cpu_decode_diag_t;

// Decoded instruction details with operand size and addressing modes
typedef enum {
    OV_OPERAND_SIZE_8 = 1,
    OV_OPERAND_SIZE_16 = 2,
    OV_OPERAND_SIZE_32 = 4,
    OV_OPERAND_SIZE_64 = 8
} ov_operand_size_t;

typedef enum {
    OV_ADDR_MODE_NONE = 0,
    OV_ADDR_MODE_REG_OFFSET,     // [Rn + offset]
    OV_ADDR_MODE_PRE_INDEXED,    // [Rn + offset]!
    OV_ADDR_MODE_POST_INDEXED    // [Rn], offset
} ov_addressing_mode_t;

typedef struct {
    ovir_opcode_t opcode;
    uint64_t guest_pc;
    uint32_t raw_insn;
    ov_operand_size_t op_size;
    uint8_t dst_reg;
    uint8_t src_reg1;
    uint8_t src_reg2;
    int64_t imm_val;
    int32_t mem_offset;
    ov_addressing_mode_t addr_mode;
    ovir_condition_t cond;
    bool sets_flags;
    bool is_branch;
    int64_t branch_target_disp;
    char mnemonic[32];
    bool is_vector;
    ovir_vec_arrangement_t vec_arrangement;
    uint8_t vec_lane;
} ov_decoded_insn_t;

// API functions
ov_decode_status_t ovir_cpu_decode_single(
    uint32_t raw_insn,
    uint64_t guest_pc,
    ov_decoded_insn_t *out_insn,
    ov_cpu_decode_diag_t *out_diag
);

ov_status_t ovir_cpu_decode_sequence(
    const uint32_t *raw_insns,
    uint32_t count,
    uint64_t start_pc,
    ovir_ir_block_t *out_ir,
    ov_cpu_decode_diag_t *out_diag
);

const char *ov_decode_status_name(ov_decode_status_t status);

#endif /* OVIR_CPU_DECODER_H */
