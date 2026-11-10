#include "ovir_cpu_decoder.h"
#include <string.h>
#include <stdio.h>

const char *ov_decode_status_name(ov_decode_status_t status) {
    switch (status) {
        case OV_DECODE_SUCCESS: return "SUCCESS";
        case OV_DECODE_ERR_UNKNOWN_OPCODE: return "UNKNOWN_OPCODE";
        case OV_DECODE_ERR_MALFORMED: return "MALFORMED_INSTRUCTION";
        case OV_DECODE_ERR_UNSUPPORTED_OPERAND: return "UNSUPPORTED_OPERAND";
        case OV_DECODE_ERR_UNALIGNED_BRANCH: return "UNALIGNED_BRANCH";
        case OV_DECODE_ERR_INVALID_REG: return "INVALID_REGISTER";
        case OV_DECODE_ERR_BUFFER_OVERFLOW: return "BUFFER_OVERFLOW";
        case OV_DECODE_ERR_UNSUPPORTED_PREFIX: return "UNSUPPORTED_PREFIX";
        default: return "UNKNOWN_ERROR";
    }
}

static int64_t sign_extend(int64_t val, int bits) {
    int64_t m = 1ULL << (bits - 1);
    return (val ^ m) - m;
}

ov_decode_status_t ovir_cpu_decode_single(
    uint32_t raw,
    uint64_t guest_pc,
    ov_decoded_insn_t *out,
    ov_cpu_decode_diag_t *diag
) {
    if (!out) return OV_DECODE_ERR_MALFORMED;
    memset(out, 0, sizeof(ov_decoded_insn_t));
    out->guest_pc = guest_pc;
    out->raw_insn = raw;
    out->op_size = OV_OPERAND_SIZE_64;
    out->cond = OVIR_CC_AL;

    if (diag) {
        memset(diag, 0, sizeof(ov_cpu_decode_diag_t));
        diag->guest_pc = guest_pc;
        diag->raw_insn = raw;
    }

    // 1. NOP: 0xd503201f
    if (raw == 0xd503201f) {
        out->opcode = OVIR_OP_NOP;
        snprintf(out->mnemonic, sizeof(out->mnemonic), "NOP");
        return OV_DECODE_SUCCESS;
    }

    // 2. RET: 0xd65f0000 | (Rn << 5)
    if ((raw & 0xfffffc1f) == 0xd65f0000) {
        uint8_t rn = (uint8_t)((raw >> 5) & 0x1f);
        out->opcode = OVIR_OP_RET;
        out->src_reg1 = rn;
        out->is_branch = true;
        snprintf(out->mnemonic, sizeof(out->mnemonic), "RET X%u", rn);
        return OV_DECODE_SUCCESS;
    }

    // 3. SVC: 0xd4000001 | (imm16 << 5)
    if ((raw & 0xffe0001f) == 0xd4000001) {
        out->opcode = OVIR_OP_SYSCALL;
        out->imm_val = (raw >> 5) & 0xffff;
        snprintf(out->mnemonic, sizeof(out->mnemonic), "SVC #%llu", (unsigned long long)out->imm_val);
        return OV_DECODE_SUCCESS;
    }

    // 4. Memory Barriers: DMB / DSB / ISB (0xd5033000 mask)
    if ((raw & 0xffffff00) == 0xd5033800 || (raw & 0xffffff00) == 0xd5033900 || raw == 0xd5033fdf) {
        out->opcode = OVIR_OP_BARRIER;
        out->imm_val = raw & 0xff;
        snprintf(out->mnemonic, sizeof(out->mnemonic), "BARRIER (0x%08x)", raw);
        return OV_DECODE_SUCCESS;
    }

    // 5. MOV immediate (MOVZ 32-bit: 0x52800000, 64-bit: 0xd2800000)
    if ((raw & 0x7f800000) == 0x52800000) {
        bool is_64 = (raw & 0x80000000) != 0;
        out->opcode = OVIR_OP_MOV_IMM;
        out->op_size = is_64 ? OV_OPERAND_SIZE_64 : OV_OPERAND_SIZE_32;
        out->dst_reg = (uint8_t)(raw & 0x1f);
        out->imm_val = (int64_t)((raw >> 5) & 0xffff);
        snprintf(out->mnemonic, sizeof(out->mnemonic), "MOVZ %c%u, #0x%llx",
                 is_64 ? 'X' : 'W', out->dst_reg, (unsigned long long)out->imm_val);
        return OV_DECODE_SUCCESS;
    }

    // 6. MOVN (32-bit: 0x12800000, 64-bit: 0x92800000)
    if ((raw & 0x7f800000) == 0x12800000) {
        bool is_64 = (raw & 0x80000000) != 0;
        out->opcode = OVIR_OP_MVN;
        out->op_size = is_64 ? OV_OPERAND_SIZE_64 : OV_OPERAND_SIZE_32;
        out->dst_reg = (uint8_t)(raw & 0x1f);
        out->imm_val = ~(int64_t)((raw >> 5) & 0xffff);
        snprintf(out->mnemonic, sizeof(out->mnemonic), "MOVN %c%u, #0x%x",
                 is_64 ? 'X' : 'W', out->dst_reg, (raw >> 5) & 0xffff);
        return OV_DECODE_SUCCESS;
    }

    // 7. MOV register (ORR Rd, XZR, Rm: 0xaa0003e0 | (Rm << 16) | Rd)
    if ((raw & 0x7fe0ffe0) == 0x2a0003e0) {
        bool is_64 = (raw & 0x80000000) != 0;
        out->opcode = OVIR_OP_MOV_REG;
        out->op_size = is_64 ? OV_OPERAND_SIZE_64 : OV_OPERAND_SIZE_32;
        out->dst_reg = (uint8_t)(raw & 0x1f);
        out->src_reg1 = (uint8_t)((raw >> 16) & 0x1f);
        snprintf(out->mnemonic, sizeof(out->mnemonic), "MOV %c%u, %c%u",
                 is_64 ? 'X' : 'W', out->dst_reg, is_64 ? 'X' : 'W', out->src_reg1);
        return OV_DECODE_SUCCESS;
    }

    // 8. ADD immediate (32-bit: 0x11000000, 64-bit: 0x91000000)
    if ((raw & 0x7f800000) == 0x11000000) {
        bool is_64 = (raw & 0x80000000) != 0;
        out->opcode = OVIR_OP_ADD;
        out->op_size = is_64 ? OV_OPERAND_SIZE_64 : OV_OPERAND_SIZE_32;
        out->dst_reg = (uint8_t)(raw & 0x1f);
        out->src_reg1 = (uint8_t)((raw >> 5) & 0x1f);
        out->imm_val = (int64_t)((raw >> 10) & 0xfff);
        snprintf(out->mnemonic, sizeof(out->mnemonic), "ADD %c%u, %c%u, #%lld",
                 is_64 ? 'X' : 'W', out->dst_reg, is_64 ? 'X' : 'W', out->src_reg1, (long long)out->imm_val);
        return OV_DECODE_SUCCESS;
    }

    // 9. SUB immediate (32-bit: 0x51000000, 64-bit: 0xd1000000)
    if ((raw & 0x7f800000) == 0x51000000) {
        bool is_64 = (raw & 0x80000000) != 0;
        out->opcode = OVIR_OP_SUB;
        out->op_size = is_64 ? OV_OPERAND_SIZE_64 : OV_OPERAND_SIZE_32;
        out->dst_reg = (uint8_t)(raw & 0x1f);
        out->src_reg1 = (uint8_t)((raw >> 5) & 0x1f);
        out->imm_val = (int64_t)((raw >> 10) & 0xfff);
        snprintf(out->mnemonic, sizeof(out->mnemonic), "SUB %c%u, %c%u, #%lld",
                 is_64 ? 'X' : 'W', out->dst_reg, is_64 ? 'X' : 'W', out->src_reg1, (long long)out->imm_val);
        return OV_DECODE_SUCCESS;
    }

    // 10. CMP immediate (SUBS XZR, Rn, #imm: 0x7100001f / 0xf100001f)
    if ((raw & 0x7f80001f) == 0x7100001f) {
        bool is_64 = (raw & 0x80000000) != 0;
        out->opcode = OVIR_OP_CMP;
        out->op_size = is_64 ? OV_OPERAND_SIZE_64 : OV_OPERAND_SIZE_32;
        out->src_reg1 = (uint8_t)((raw >> 5) & 0x1f);
        out->imm_val = (int64_t)((raw >> 10) & 0xfff);
        out->sets_flags = true;
        snprintf(out->mnemonic, sizeof(out->mnemonic), "CMP %c%u, #%lld",
                 is_64 ? 'X' : 'W', out->src_reg1, (long long)out->imm_val);
        return OV_DECODE_SUCCESS;
    }

    // 11. CMP register (SUBS XZR, Rn, Rm: 0x6b00001f / 0xeb00001f)
    if ((raw & 0x7fe0001f) == 0x6b00001f) {
        bool is_64 = (raw & 0x80000000) != 0;
        out->opcode = OVIR_OP_CMP;
        out->op_size = is_64 ? OV_OPERAND_SIZE_64 : OV_OPERAND_SIZE_32;
        out->src_reg1 = (uint8_t)((raw >> 5) & 0x1f);
        out->src_reg2 = (uint8_t)((raw >> 16) & 0x1f);
        out->sets_flags = true;
        snprintf(out->mnemonic, sizeof(out->mnemonic), "CMP %c%u, %c%u",
                 is_64 ? 'X' : 'W', out->src_reg1, is_64 ? 'X' : 'W', out->src_reg2);
        return OV_DECODE_SUCCESS;
    }

    // 12. TST register (ANDS XZR, Rn, Rm: 0x6a00001f / 0xea00001f)
    if ((raw & 0x7fe0001f) == 0x6a00001f) {
        bool is_64 = (raw & 0x80000000) != 0;
        out->opcode = OVIR_OP_TST;
        out->op_size = is_64 ? OV_OPERAND_SIZE_64 : OV_OPERAND_SIZE_32;
        out->src_reg1 = (uint8_t)((raw >> 5) & 0x1f);
        out->src_reg2 = (uint8_t)((raw >> 16) & 0x1f);
        out->sets_flags = true;
        snprintf(out->mnemonic, sizeof(out->mnemonic), "TST %c%u, %c%u",
                 is_64 ? 'X' : 'W', out->src_reg1, is_64 ? 'X' : 'W', out->src_reg2);
        return OV_DECODE_SUCCESS;
    }

    // 13. ADD / ADDS shifted register (0x0b000000, 0x8b000000, 0x2b000000, 0xab000000)
    if ((raw & 0x5f200000) == 0x0b000000) {
        bool is_64 = (raw & 0x80000000) != 0;
        bool sets_flags = (raw & 0x20000000) != 0;
        out->opcode = sets_flags ? OVIR_OP_ADDS : OVIR_OP_ADD;
        out->op_size = is_64 ? OV_OPERAND_SIZE_64 : OV_OPERAND_SIZE_32;
        out->dst_reg = (uint8_t)(raw & 0x1f);
        out->src_reg1 = (uint8_t)((raw >> 5) & 0x1f);
        out->src_reg2 = (uint8_t)((raw >> 16) & 0x1f);
        out->sets_flags = sets_flags;
        snprintf(out->mnemonic, sizeof(out->mnemonic), "%s %c%u, %c%u, %c%u",
                 sets_flags ? "ADDS" : "ADD",
                 is_64 ? 'X' : 'W', out->dst_reg,
                 is_64 ? 'X' : 'W', out->src_reg1,
                 is_64 ? 'X' : 'W', out->src_reg2);
        return OV_DECODE_SUCCESS;
    }

    // 14. SUB / SUBS shifted register (0x4b000000, 0xcb000000, 0x6b000000, 0xeb000000)
    if ((raw & 0x5f200000) == 0x4b000000) {
        bool is_64 = (raw & 0x80000000) != 0;
        bool sets_flags = (raw & 0x20000000) != 0;
        out->opcode = sets_flags ? OVIR_OP_SUBS : OVIR_OP_SUB;
        out->op_size = is_64 ? OV_OPERAND_SIZE_64 : OV_OPERAND_SIZE_32;
        out->dst_reg = (uint8_t)(raw & 0x1f);
        out->src_reg1 = (uint8_t)((raw >> 5) & 0x1f);
        out->src_reg2 = (uint8_t)((raw >> 16) & 0x1f);
        out->sets_flags = sets_flags;
        snprintf(out->mnemonic, sizeof(out->mnemonic), "%s %c%u, %c%u, %c%u",
                 sets_flags ? "SUBS" : "SUB",
                 is_64 ? 'X' : 'W', out->dst_reg,
                 is_64 ? 'X' : 'W', out->src_reg1,
                 is_64 ? 'X' : 'W', out->src_reg2);
        return OV_DECODE_SUCCESS;
    }

    // 15. MUL: 0x1b007c00 / 0x9b007c00
    if ((raw & 0x7fe0fc00) == 0x1b007c00) {
        bool is_64 = (raw & 0x80000000) != 0;
        out->opcode = OVIR_OP_MUL;
        out->op_size = is_64 ? OV_OPERAND_SIZE_64 : OV_OPERAND_SIZE_32;
        out->dst_reg = (uint8_t)(raw & 0x1f);
        out->src_reg1 = (uint8_t)((raw >> 5) & 0x1f);
        out->src_reg2 = (uint8_t)((raw >> 16) & 0x1f);
        snprintf(out->mnemonic, sizeof(out->mnemonic), "MUL %c%u, %c%u, %c%u",
                 is_64 ? 'X' : 'W', out->dst_reg,
                 is_64 ? 'X' : 'W', out->src_reg1,
                 is_64 ? 'X' : 'W', out->src_reg2);
        return OV_DECODE_SUCCESS;
    }

    // 16. Logical: AND (0x0a000000, 0x8a000000)
    if ((raw & 0x7f200000) == 0x0a000000) {
        bool is_64 = (raw & 0x80000000) != 0;
        out->opcode = OVIR_OP_AND;
        out->op_size = is_64 ? OV_OPERAND_SIZE_64 : OV_OPERAND_SIZE_32;
        out->dst_reg = (uint8_t)(raw & 0x1f);
        out->src_reg1 = (uint8_t)((raw >> 5) & 0x1f);
        out->src_reg2 = (uint8_t)((raw >> 16) & 0x1f);
        snprintf(out->mnemonic, sizeof(out->mnemonic), "AND %c%u, %c%u, %c%u",
                 is_64 ? 'X' : 'W', out->dst_reg,
                 is_64 ? 'X' : 'W', out->src_reg1,
                 is_64 ? 'X' : 'W', out->src_reg2);
        return OV_DECODE_SUCCESS;
    }

    // 16b. Logical: ANDS (0x6a000000, 0xea000000)
    if ((raw & 0x7f200000) == 0x6a000000) {
        bool is_64 = (raw & 0x80000000) != 0;
        out->opcode = OVIR_OP_ANDS;
        out->op_size = is_64 ? OV_OPERAND_SIZE_64 : OV_OPERAND_SIZE_32;
        out->dst_reg = (uint8_t)(raw & 0x1f);
        out->src_reg1 = (uint8_t)((raw >> 5) & 0x1f);
        out->src_reg2 = (uint8_t)((raw >> 16) & 0x1f);
        out->sets_flags = true;
        snprintf(out->mnemonic, sizeof(out->mnemonic), "ANDS %c%u, %c%u, %c%u",
                 is_64 ? 'X' : 'W', out->dst_reg,
                 is_64 ? 'X' : 'W', out->src_reg1,
                 is_64 ? 'X' : 'W', out->src_reg2);
        return OV_DECODE_SUCCESS;
    }

    // 17. Logical: ORR (0x2a000000, 0xaa000000)
    if ((raw & 0x7f200000) == 0x2a000000) {
        bool is_64 = (raw & 0x80000000) != 0;
        out->opcode = OVIR_OP_ORR;
        out->op_size = is_64 ? OV_OPERAND_SIZE_64 : OV_OPERAND_SIZE_32;
        out->dst_reg = (uint8_t)(raw & 0x1f);
        out->src_reg1 = (uint8_t)((raw >> 5) & 0x1f);
        out->src_reg2 = (uint8_t)((raw >> 16) & 0x1f);
        snprintf(out->mnemonic, sizeof(out->mnemonic), "ORR %c%u, %c%u, %c%u",
                 is_64 ? 'X' : 'W', out->dst_reg,
                 is_64 ? 'X' : 'W', out->src_reg1,
                 is_64 ? 'X' : 'W', out->src_reg2);
        return OV_DECODE_SUCCESS;
    }

    // 18. Logical: EOR (0x4a000000, 0xca000000)
    if ((raw & 0x7f200000) == 0x4a000000) {
        bool is_64 = (raw & 0x80000000) != 0;
        out->opcode = OVIR_OP_EOR;
        out->op_size = is_64 ? OV_OPERAND_SIZE_64 : OV_OPERAND_SIZE_32;
        out->dst_reg = (uint8_t)(raw & 0x1f);
        out->src_reg1 = (uint8_t)((raw >> 5) & 0x1f);
        out->src_reg2 = (uint8_t)((raw >> 16) & 0x1f);
        snprintf(out->mnemonic, sizeof(out->mnemonic), "EOR %c%u, %c%u, %c%u",
                 is_64 ? 'X' : 'W', out->dst_reg,
                 is_64 ? 'X' : 'W', out->src_reg1,
                 is_64 ? 'X' : 'W', out->src_reg2);
        return OV_DECODE_SUCCESS;
    }

    // 19. Shifts: LSLV (0x1ac02000), LSRV (0x1ac02400), ASRV (0x1ac02800)
    if ((raw & 0x7fe0fc00) == 0x1ac02000) {
        bool is_64 = (raw & 0x80000000) != 0;
        uint32_t op_type = (raw >> 10) & 0x3f;
        out->op_size = is_64 ? OV_OPERAND_SIZE_64 : OV_OPERAND_SIZE_32;
        out->dst_reg = (uint8_t)(raw & 0x1f);
        out->src_reg1 = (uint8_t)((raw >> 5) & 0x1f);
        out->src_reg2 = (uint8_t)((raw >> 16) & 0x1f);

        if (op_type == 0x08) {
            out->opcode = OVIR_OP_LSL;
            snprintf(out->mnemonic, sizeof(out->mnemonic), "LSL %c%u, %c%u, %c%u",
                     is_64 ? 'X' : 'W', out->dst_reg, is_64 ? 'X' : 'W', out->src_reg1, is_64 ? 'X' : 'W', out->src_reg2);
            return OV_DECODE_SUCCESS;
        } else if (op_type == 0x09) {
            out->opcode = OVIR_OP_LSR;
            snprintf(out->mnemonic, sizeof(out->mnemonic), "LSR %c%u, %c%u, %c%u",
                     is_64 ? 'X' : 'W', out->dst_reg, is_64 ? 'X' : 'W', out->src_reg1, is_64 ? 'X' : 'W', out->src_reg2);
            return OV_DECODE_SUCCESS;
        } else if (op_type == 0x0a) {
            out->opcode = OVIR_OP_ASR;
            snprintf(out->mnemonic, sizeof(out->mnemonic), "ASR %c%u, %c%u, %c%u",
                     is_64 ? 'X' : 'W', out->dst_reg, is_64 ? 'X' : 'W', out->src_reg1, is_64 ? 'X' : 'W', out->src_reg2);
            return OV_DECODE_SUCCESS;
        }
    }

    // 20. Load / Store with unsigned immediate offset
    // LDRB (0x39400000), LDRH (0x79400000), LDR 32 (0xb9400000), LDR 64 (0xf9400000)
    // STRB (0x39000000), STRH (0x79000000), STR 32 (0xb9000000), STR 64 (0xf9000000)
    if ((raw & 0x3b200000) == 0x39000000) {
        uint32_t size_bits = (raw >> 30) & 0x3;
        bool is_load = (raw & 0x00400000) != 0;
        uint32_t imm12 = (raw >> 10) & 0xfff;

        out->opcode = is_load ? OVIR_OP_LOAD : OVIR_OP_STORE;
        out->dst_reg = (uint8_t)(raw & 0x1f);      // Rt
        out->src_reg1 = (uint8_t)((raw >> 5) & 0x1f); // Rn (base)
        out->addr_mode = OV_ADDR_MODE_REG_OFFSET;

        int scale = 0;
        if (size_bits == 0) {
            out->op_size = OV_OPERAND_SIZE_8;
            scale = 1;
        } else if (size_bits == 1) {
            out->op_size = OV_OPERAND_SIZE_16;
            scale = 2;
        } else if (size_bits == 2) {
            out->op_size = OV_OPERAND_SIZE_32;
            scale = 4;
        } else {
            out->op_size = OV_OPERAND_SIZE_64;
            scale = 8;
        }
        out->mem_offset = (int32_t)(imm12 * scale);

        snprintf(out->mnemonic, sizeof(out->mnemonic), "%s %c%u, [%c%u, #%d]",
                 is_load ? "LDR" : "STR",
                 (out->op_size == OV_OPERAND_SIZE_64) ? 'X' : 'W', out->dst_reg,
                 'X', out->src_reg1, out->mem_offset);
        return OV_DECODE_SUCCESS;
    }

    // 21. Unconditional Branch B: 0x14000000 | imm26
    if ((raw & 0xfc000000) == 0x14000000) {
        int64_t imm26 = raw & 0x03ffffff;
        int64_t disp = sign_extend(imm26, 26) * 4;
        out->opcode = OVIR_OP_JMP;
        out->is_branch = true;
        out->branch_target_disp = disp;
        snprintf(out->mnemonic, sizeof(out->mnemonic), "B #%+lld", (long long)disp);
        return OV_DECODE_SUCCESS;
    }

    // 22. Branch with Link BL: 0x94000000 | imm26
    if ((raw & 0xfc000000) == 0x94000000) {
        int64_t imm26 = raw & 0x03ffffff;
        int64_t disp = sign_extend(imm26, 26) * 4;
        out->opcode = OVIR_OP_CALL;
        out->is_branch = true;
        out->branch_target_disp = disp;
        snprintf(out->mnemonic, sizeof(out->mnemonic), "BL #%+lld", (long long)disp);
        return OV_DECODE_SUCCESS;
    }

    // 23. Conditional Branch B.cond: 0x54000000 | (imm19 << 5) | cond
    if ((raw & 0xff000010) == 0x54000000) {
        uint8_t cond_raw = (uint8_t)(raw & 0xf);
        int64_t imm19 = (raw >> 5) & 0x7ffff;
        int64_t disp = sign_extend(imm19, 19) * 4;

        out->opcode = OVIR_OP_JMP_COND;
        out->is_branch = true;
        out->branch_target_disp = disp;
        // Map ARM64 condition code (0-14) to ovir_condition_t
        out->cond = (cond_raw <= 14) ? (ovir_condition_t)(cond_raw + 1) : OVIR_CC_AL;

        static const char *cond_names[] = {
            "EQ", "NE", "CS", "CC", "MI", "PL", "VS", "VC",
            "HI", "LS", "GE", "LT", "GT", "LE", "AL"
        };
        const char *cname = (cond_raw < 15) ? cond_names[cond_raw] : "??";
        snprintf(out->mnemonic, sizeof(out->mnemonic), "B.%s #%+lld", cname, (long long)disp);
        return OV_DECODE_SUCCESS;
    }

    // 24. CBZ / CBNZ: 0x34000000 / 0x35000000 (32-bit), 0xb4000000 / 0xb5000000 (64-bit)
    if ((raw & 0x7e000000) == 0x34000000) {
        bool is_64 = (raw & 0x80000000) != 0;
        bool is_cbnz = (raw & 0x01000000) != 0;
        uint8_t rt = (uint8_t)(raw & 0x1f);
        int64_t imm19 = (raw >> 5) & 0x7ffff;
        int64_t disp = sign_extend(imm19, 19) * 4;

        out->opcode = is_cbnz ? OVIR_OP_CBNZ : OVIR_OP_CBZ;
        out->op_size = is_64 ? OV_OPERAND_SIZE_64 : OV_OPERAND_SIZE_32;
        out->dst_reg = rt;
        out->is_branch = true;
        out->branch_target_disp = disp;

        snprintf(out->mnemonic, sizeof(out->mnemonic), "%s %c%u, #%+lld",
                 is_cbnz ? "CBNZ" : "CBZ",
                 is_64 ? 'X' : 'W', rt, (long long)disp);
        return OV_DECODE_SUCCESS;
    }

    // =========================================================================
    // Phase 13: ARM64 NEON Vector Instructions
    // =========================================================================

    // 25. NEON ADD.4S: 0x4e208400 | (Rm << 16) | (Rn << 5) | Rd
    if ((raw & 0xffe0fc00) == 0x4e208400) {
        out->opcode = OVIR_OP_VEC_ADD;
        out->is_vector = true;
        out->vec_arrangement = OVIR_VEC_ARRANGE_4S;
        out->dst_reg = (uint8_t)(raw & 0x1f);
        out->src_reg1 = (uint8_t)((raw >> 5) & 0x1f);
        out->src_reg2 = (uint8_t)((raw >> 16) & 0x1f);
        snprintf(out->mnemonic, sizeof(out->mnemonic), "ADD V%u.4S, V%u.4S, V%u.4S",
                 out->dst_reg, out->src_reg1, out->src_reg2);
        return OV_DECODE_SUCCESS;
    }

    // 26. NEON SUB.4S: 0x4e608400 or 0x6e208400 | (Rm << 16) | (Rn << 5) | Rd
    if ((raw & 0xffe0fc00) == 0x4e608400 || (raw & 0xffe0fc00) == 0x6e208400) {
        out->opcode = OVIR_OP_VEC_SUB;
        out->is_vector = true;
        out->vec_arrangement = OVIR_VEC_ARRANGE_4S;
        out->dst_reg = (uint8_t)(raw & 0x1f);
        out->src_reg1 = (uint8_t)((raw >> 5) & 0x1f);
        out->src_reg2 = (uint8_t)((raw >> 16) & 0x1f);
        snprintf(out->mnemonic, sizeof(out->mnemonic), "SUB V%u.4S, V%u.4S, V%u.4S",
                 out->dst_reg, out->src_reg1, out->src_reg2);
        return OV_DECODE_SUCCESS;
    }

    // 27. NEON MUL.4S: 0x4e209c00 | (Rm << 16) | (Rn << 5) | Rd
    if ((raw & 0xffe0fc00) == 0x4e209c00) {
        out->opcode = OVIR_OP_VEC_MUL;
        out->is_vector = true;
        out->vec_arrangement = OVIR_VEC_ARRANGE_4S;
        out->dst_reg = (uint8_t)(raw & 0x1f);
        out->src_reg1 = (uint8_t)((raw >> 5) & 0x1f);
        out->src_reg2 = (uint8_t)((raw >> 16) & 0x1f);
        snprintf(out->mnemonic, sizeof(out->mnemonic), "MUL V%u.4S, V%u.4S, V%u.4S",
                 out->dst_reg, out->src_reg1, out->src_reg2);
        return OV_DECODE_SUCCESS;
    }

    // 28. NEON ADD.2D: 0x4ee08400 | (Rm << 16) | (Rn << 5) | Rd
    if ((raw & 0xffe0fc00) == 0x4ee08400) {
        out->opcode = OVIR_OP_VEC_ADD;
        out->is_vector = true;
        out->vec_arrangement = OVIR_VEC_ARRANGE_2D;
        out->dst_reg = (uint8_t)(raw & 0x1f);
        out->src_reg1 = (uint8_t)((raw >> 5) & 0x1f);
        out->src_reg2 = (uint8_t)((raw >> 16) & 0x1f);
        snprintf(out->mnemonic, sizeof(out->mnemonic), "ADD V%u.2D, V%u.2D, V%u.2D",
                 out->dst_reg, out->src_reg1, out->src_reg2);
        return OV_DECODE_SUCCESS;
    }

    // 29. NEON SUB.2D: 0x6ee08400 | (Rm << 16) | (Rn << 5) | Rd
    if ((raw & 0xffe0fc00) == 0x6ee08400) {
        out->opcode = OVIR_OP_VEC_SUB;
        out->is_vector = true;
        out->vec_arrangement = OVIR_VEC_ARRANGE_2D;
        out->dst_reg = (uint8_t)(raw & 0x1f);
        out->src_reg1 = (uint8_t)((raw >> 5) & 0x1f);
        out->src_reg2 = (uint8_t)((raw >> 16) & 0x1f);
        snprintf(out->mnemonic, sizeof(out->mnemonic), "SUB V%u.2D, V%u.2D, V%u.2D",
                 out->dst_reg, out->src_reg1, out->src_reg2);
        return OV_DECODE_SUCCESS;
    }

    // 30. NEON AND (16B): 0x4e201c00 | (Rm << 16) | (Rn << 5) | Rd
    if ((raw & 0xffe0fc00) == 0x4e201c00) {
        out->opcode = OVIR_OP_VEC_AND;
        out->is_vector = true;
        out->vec_arrangement = OVIR_VEC_ARRANGE_16B;
        out->dst_reg = (uint8_t)(raw & 0x1f);
        out->src_reg1 = (uint8_t)((raw >> 5) & 0x1f);
        out->src_reg2 = (uint8_t)((raw >> 16) & 0x1f);
        snprintf(out->mnemonic, sizeof(out->mnemonic), "AND V%u.16B, V%u.16B, V%u.16B",
                 out->dst_reg, out->src_reg1, out->src_reg2);
        return OV_DECODE_SUCCESS;
    }

    // 31. NEON ORR (16B): 0x4ea01c00 | (Rm << 16) | (Rn << 5) | Rd
    if ((raw & 0xffe0fc00) == 0x4ea01c00) {
        out->opcode = OVIR_OP_VEC_ORR;
        out->is_vector = true;
        out->vec_arrangement = OVIR_VEC_ARRANGE_16B;
        out->dst_reg = (uint8_t)(raw & 0x1f);
        out->src_reg1 = (uint8_t)((raw >> 5) & 0x1f);
        out->src_reg2 = (uint8_t)((raw >> 16) & 0x1f);
        snprintf(out->mnemonic, sizeof(out->mnemonic), "ORR V%u.16B, V%u.16B, V%u.16B",
                 out->dst_reg, out->src_reg1, out->src_reg2);
        return OV_DECODE_SUCCESS;
    }

    // 32. NEON EOR (16B): 0x4e201400 or 0x6e201c00 | (Rm << 16) | (Rn << 5) | Rd
    if ((raw & 0xffe0fc00) == 0x4e201400 || (raw & 0xffe0fc00) == 0x6e201c00) {
        out->opcode = OVIR_OP_VEC_EOR;
        out->is_vector = true;
        out->vec_arrangement = OVIR_VEC_ARRANGE_16B;
        out->dst_reg = (uint8_t)(raw & 0x1f);
        out->src_reg1 = (uint8_t)((raw >> 5) & 0x1f);
        out->src_reg2 = (uint8_t)((raw >> 16) & 0x1f);
        snprintf(out->mnemonic, sizeof(out->mnemonic), "EOR V%u.16B, V%u.16B, V%u.16B",
                 out->dst_reg, out->src_reg1, out->src_reg2);
        return OV_DECODE_SUCCESS;
    }

    // 33. NEON DUP (element): 0x4e040c00 | (Rn << 5) | Rd
    if ((raw & 0xfffffc00) == 0x4e040c00) {
        out->opcode = OVIR_OP_VEC_DUP;
        out->is_vector = true;
        out->vec_arrangement = OVIR_VEC_ARRANGE_4S;
        out->dst_reg = (uint8_t)(raw & 0x1f);
        out->src_reg1 = (uint8_t)((raw >> 5) & 0x1f);
        snprintf(out->mnemonic, sizeof(out->mnemonic), "DUP V%u.4S, W%u",
                 out->dst_reg, out->src_reg1);
        return OV_DECODE_SUCCESS;
    }

    // 34. NEON MOVI (vector): 0x4f000400 | (imm8 << 5) | Rd
    if ((raw & 0xff800400) == 0x4f000400) {
        out->opcode = OVIR_OP_VEC_MOVI;
        out->is_vector = true;
        out->vec_arrangement = OVIR_VEC_ARRANGE_4S;
        out->dst_reg = (uint8_t)(raw & 0x1f);
        out->imm_val = (raw >> 5) & 0xff;
        snprintf(out->mnemonic, sizeof(out->mnemonic), "MOVI V%u.4S, #0x%llx",
                 out->dst_reg, (unsigned long long)out->imm_val);
        return OV_DECODE_SUCCESS;
    }

    // Unrecognized / unsupported instruction
    if (diag) {
        diag->error_code = OV_DECODE_ERR_UNKNOWN_OPCODE;
        snprintf(diag->mnemonic_guess, sizeof(diag->mnemonic_guess), "UNKNOWN (0x%08x)", raw);
        snprintf(diag->description, sizeof(diag->description),
                 "Instruction at guest PC 0x%llx (0x%08x) not in supported ARM64 subset",
                 (unsigned long long)guest_pc, raw);
        snprintf(diag->suggested_fallback, sizeof(diag->suggested_fallback),
                 "Interpreter Fallback / Software Reconstruction");
    }

    return OV_DECODE_ERR_UNKNOWN_OPCODE;
}

ov_status_t ovir_cpu_decode_sequence(
    const uint32_t *raw_insns,
    uint32_t count,
    uint64_t start_pc,
    ovir_ir_block_t *out_ir,
    ov_cpu_decode_diag_t *out_diag
) {
    if (!raw_insns || count == 0 || !out_ir) {
        return OV_ERR_INVALID_PARAM;
    }
    if (count > OVIR_CPU_MAX_INSTRUCTIONS) {
        if (out_diag) {
            out_diag->error_code = OV_DECODE_ERR_BUFFER_OVERFLOW;
            snprintf(out_diag->description, sizeof(out_diag->description), "Block size exceeds %d", OVIR_CPU_MAX_INSTRUCTIONS);
        }
        return OV_ERR_UNSUPPORTED;
    }

    memset(out_ir, 0, sizeof(ovir_ir_block_t));
    out_ir->source_arch = OV_ARCH_ARM64;
    out_ir->start_guest_pc = start_pc;

    // Compute hash
    uint64_t hash = 0xcbf29ce484222325ULL;
    for (uint32_t i = 0; i < count; i++) {
        hash ^= (uint64_t)raw_insns[i];
        hash *= 0x100000001b3ULL;
    }
    out_ir->source_code_hash = hash;

    for (uint32_t i = 0; i < count; i++) {
        uint64_t current_pc = start_pc + (i * 4);
        ov_decoded_insn_t decoded;
        ov_decode_status_t st = ovir_cpu_decode_single(raw_insns[i], current_pc, &decoded, out_diag);
        if (st != OV_DECODE_SUCCESS) {
            return OV_ERR_UNSUPPORTED;
        }

        ovir_insn_t *insn = &out_ir->instructions[out_ir->count++];
        insn->opcode = decoded.opcode;
        insn->dst_reg = decoded.dst_reg;
        insn->src_reg1 = decoded.src_reg1;
        insn->src_reg2 = decoded.src_reg2;
        insn->imm_val = decoded.imm_val;
        insn->offset = decoded.mem_offset;
        insn->op_size = (uint8_t)decoded.op_size;
        insn->cond = decoded.cond;
        insn->sets_flags = decoded.sets_flags;
        insn->guest_pc = current_pc;
        insn->branch_target_disp = decoded.branch_target_disp;
        insn->is_constant = (decoded.opcode == OVIR_OP_MOV_IMM);
        insn->is_vector = decoded.is_vector;
        insn->vec_arrangement = decoded.vec_arrangement;
        insn->vec_lane = decoded.vec_lane;
        snprintf(insn->debug_mnemonic, sizeof(insn->debug_mnemonic), "%s", decoded.mnemonic);

        if (decoded.is_branch) {
            out_ir->has_branches = true;
        }
        if (decoded.opcode == OVIR_OP_LOAD || decoded.opcode == OVIR_OP_STORE) {
            out_ir->has_memory_access = true;
        }
        if (decoded.is_vector) {
            out_ir->has_vector_ops = true;
        }
    }

    out_ir->end_guest_pc = start_pc + (out_ir->count * 4);
    return OV_SUCCESS;
}
