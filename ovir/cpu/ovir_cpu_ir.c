#include "ovir_cpu.h"
#include <string.h>
#include <stdio.h>

const char *ovir_opcode_name(ovir_opcode_t op) {
    switch (op) {
        case OVIR_OP_NOP: return "NOP";
        case OVIR_OP_MOV_IMM: return "MOV_IMM";
        case OVIR_OP_MOV_REG: return "MOV_REG";
        case OVIR_OP_MVN: return "MVN";
        case OVIR_OP_ADD: return "ADD";
        case OVIR_OP_ADDS: return "ADDS";
        case OVIR_OP_SUB: return "SUB";
        case OVIR_OP_SUBS: return "SUBS";
        case OVIR_OP_MUL: return "MUL";
        case OVIR_OP_NEG: return "NEG";
        case OVIR_OP_AND: return "AND";
        case OVIR_OP_ANDS: return "ANDS";
        case OVIR_OP_ORR: return "ORR";
        case OVIR_OP_EOR: return "EOR";
        case OVIR_OP_LSL: return "LSL";
        case OVIR_OP_LSR: return "LSR";
        case OVIR_OP_ASR: return "ASR";
        case OVIR_OP_CMP: return "CMP";
        case OVIR_OP_TST: return "TST";
        case OVIR_OP_LOAD: return "LOAD";
        case OVIR_OP_STORE: return "STORE";
        case OVIR_OP_JMP: return "JMP";
        case OVIR_OP_JMP_COND: return "JMP_COND";
        case OVIR_OP_CBZ: return "CBZ";
        case OVIR_OP_CBNZ: return "CBNZ";
        case OVIR_OP_CALL: return "CALL";
        case OVIR_OP_RET: return "RET";
        case OVIR_OP_BARRIER: return "BARRIER";
        case OVIR_OP_SYSCALL: return "SYSCALL";
        case OVIR_OP_VEC_ADD: return "VEC_ADD";
        case OVIR_OP_VEC_SUB: return "VEC_SUB";
        case OVIR_OP_VEC_MUL: return "VEC_MUL";
        case OVIR_OP_VEC_AND: return "VEC_AND";
        case OVIR_OP_VEC_ORR: return "VEC_ORR";
        case OVIR_OP_VEC_EOR: return "VEC_EOR";
        case OVIR_OP_VEC_DUP: return "VEC_DUP";
        case OVIR_OP_VEC_MOVI: return "VEC_MOVI";
        default: return "UNKNOWN";
    }
}

const char *ovir_condition_name(ovir_condition_t cond) {
    switch (cond) {
        case OVIR_CC_AL: return "AL";
        case OVIR_CC_EQ: return "EQ";
        case OVIR_CC_NE: return "NE";
        case OVIR_CC_CS: return "CS";
        case OVIR_CC_CC: return "CC";
        case OVIR_CC_MI: return "MI";
        case OVIR_CC_PL: return "PL";
        case OVIR_CC_VS: return "VS";
        case OVIR_CC_VC: return "VC";
        case OVIR_CC_HI: return "HI";
        case OVIR_CC_LS: return "LS";
        case OVIR_CC_GE: return "GE";
        case OVIR_CC_LT: return "LT";
        case OVIR_CC_GT: return "GT";
        case OVIR_CC_LE: return "LE";
        default: return "??";
    }
}

ov_status_t ovir_cpu_ir_validate(const ovir_ir_block_t *ir, char *out_err, size_t err_len) {
    if (!ir) {
        if (out_err && err_len > 0) snprintf(out_err, err_len, "Null IR block pointer");
        return OV_ERR_INVALID_PARAM;
    }

    if (ir->count == 0) {
        if (out_err && err_len > 0) snprintf(out_err, err_len, "IR block is empty");
        return OV_ERR_INVALID_PARAM;
    }

    if (ir->count > OVIR_CPU_MAX_INSTRUCTIONS) {
        if (out_err && err_len > 0) snprintf(out_err, err_len, "IR block instruction count %u exceeds max %u", ir->count, OVIR_CPU_MAX_INSTRUCTIONS);
        return OV_ERR_INVALID_PARAM;
    }

    for (uint32_t i = 0; i < ir->count; i++) {
        const ovir_insn_t *insn = &ir->instructions[i];

        if ((int)insn->opcode < 0 || insn->opcode > OVIR_OP_VEC_MOVI) {
            if (out_err && err_len > 0) snprintf(out_err, err_len, "Insn %u: invalid opcode %d", i, (int)insn->opcode);
            return OV_ERR_INVALID_PARAM;
        }

        if (insn->dst_reg >= OVIR_CPU_REG_COUNT) {
            if (out_err && err_len > 0) snprintf(out_err, err_len, "Insn %u: destination reg %u out of bounds", i, insn->dst_reg);
            return OV_ERR_INVALID_PARAM;
        }

        if (insn->src_reg1 >= OVIR_CPU_REG_COUNT) {
            if (out_err && err_len > 0) snprintf(out_err, err_len, "Insn %u: src1 reg %u out of bounds", i, insn->src_reg1);
            return OV_ERR_INVALID_PARAM;
        }

        if (insn->src_reg2 >= OVIR_CPU_REG_COUNT) {
            if (out_err && err_len > 0) snprintf(out_err, err_len, "Insn %u: src2 reg %u out of bounds", i, insn->src_reg2);
            return OV_ERR_INVALID_PARAM;
        }

        // Branch displacements must be 4-byte aligned
        if (insn->opcode == OVIR_OP_JMP || insn->opcode == OVIR_OP_JMP_COND ||
            insn->opcode == OVIR_OP_CBZ || insn->opcode == OVIR_OP_CBNZ || insn->opcode == OVIR_OP_CALL) {
            if ((insn->branch_target_disp & 0x3) != 0) {
                if (out_err && err_len > 0) snprintf(out_err, err_len, "Insn %u: unaligned branch displacement %lld", i, (long long)insn->branch_target_disp);
                return OV_ERR_INVALID_PARAM;
            }
        }
    }

    if (out_err && err_len > 0) {
        out_err[0] = '\0';
    }
    return OV_SUCCESS;
}

int ovir_cpu_ir_dump(const ovir_ir_block_t *ir, char *buf, size_t buf_len) {
    if (!ir || !buf || buf_len == 0) return 0;

    size_t offset = 0;
    offset += snprintf(buf + offset, (offset < buf_len) ? buf_len - offset : 0,
                       "; OVIR-CPU Block: %u insns, Hash=0x%016llx, Start=0x%llx\n",
                       ir->count, (unsigned long long)ir->source_code_hash, (unsigned long long)ir->start_guest_pc);

    for (uint32_t i = 0; i < ir->count && offset < buf_len; i++) {
        const ovir_insn_t *insn = &ir->instructions[i];
        offset += snprintf(buf + offset, buf_len - offset,
                           "  [%04u] %-10s ; %s\n",
                           i, ovir_opcode_name(insn->opcode),
                           insn->debug_mnemonic[0] ? insn->debug_mnemonic : "");
    }

    return (int)offset;
}
