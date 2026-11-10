#include "ovir_cpu.h"
#include "ovir_cpu_decoder.h"
#include "ovir_cpu_backend.h"
#include <string.h>

ov_status_t ovir_cpu_decode(const uint32_t *raw_arm64_insns, uint32_t count, ovir_ir_block_t *out_ir) {
    ov_cpu_decode_diag_t diag;
    return ovir_cpu_decode_sequence(raw_arm64_insns, count, 0x1000, out_ir, &diag);
}

ov_status_t ovir_cpu_optimize(ovir_ir_block_t *ir, uint32_t *out_optimizations_applied) {
    if (!ir) return OV_ERR_INVALID_PARAM;
    uint32_t optimizations = 0;

    bool has_const[OVIR_CPU_REG_COUNT] = {false};
    int64_t const_val[OVIR_CPU_REG_COUNT] = {0};

    for (uint32_t i = 0; i < ir->count; i++) {
        ovir_insn_t *insn = &ir->instructions[i];

        if (insn->opcode == OVIR_OP_MOV_IMM) {
            if (insn->dst_reg < OVIR_CPU_REG_COUNT) {
                has_const[insn->dst_reg] = true;
                const_val[insn->dst_reg] = insn->imm_val;
            }
        } else if (insn->opcode == OVIR_OP_MOV_REG) {
            if (insn->dst_reg == insn->src_reg1) {
                // Redundant MOV Rd, Rd -> NOP
                insn->opcode = OVIR_OP_NOP;
                optimizations++;
            } else if (insn->src_reg1 < OVIR_CPU_REG_COUNT && has_const[insn->src_reg1]) {
                insn->opcode = OVIR_OP_MOV_IMM;
                insn->imm_val = const_val[insn->src_reg1];
                insn->is_constant = true;
                has_const[insn->dst_reg] = true;
                const_val[insn->dst_reg] = insn->imm_val;
                optimizations++;
            }
        } else if (insn->opcode == OVIR_OP_ADD) {
            // Constant folding for ADD
            if (insn->src_reg1 < OVIR_CPU_REG_COUNT && insn->src_reg2 < OVIR_CPU_REG_COUNT &&
                has_const[insn->src_reg1] && has_const[insn->src_reg2]) {
                insn->opcode = OVIR_OP_MOV_IMM;
                insn->imm_val = const_val[insn->src_reg1] + const_val[insn->src_reg2];
                insn->is_constant = true;
                has_const[insn->dst_reg] = true;
                const_val[insn->dst_reg] = insn->imm_val;
                optimizations++;
            } else if (insn->dst_reg < OVIR_CPU_REG_COUNT) {
                has_const[insn->dst_reg] = false;
            }
        } else if (insn->opcode == OVIR_OP_SUB) {
            // Constant folding for SUB
            if (insn->src_reg1 < OVIR_CPU_REG_COUNT && insn->src_reg2 < OVIR_CPU_REG_COUNT &&
                has_const[insn->src_reg1] && has_const[insn->src_reg2]) {
                insn->opcode = OVIR_OP_MOV_IMM;
                insn->imm_val = const_val[insn->src_reg1] - const_val[insn->src_reg2];
                insn->is_constant = true;
                has_const[insn->dst_reg] = true;
                const_val[insn->dst_reg] = insn->imm_val;
                optimizations++;
            } else if (insn->dst_reg < OVIR_CPU_REG_COUNT) {
                has_const[insn->dst_reg] = false;
            }
        } else if (insn->opcode == OVIR_OP_MUL) {
            // Constant folding for MUL
            if (insn->src_reg1 < OVIR_CPU_REG_COUNT && insn->src_reg2 < OVIR_CPU_REG_COUNT &&
                has_const[insn->src_reg1] && has_const[insn->src_reg2]) {
                insn->opcode = OVIR_OP_MOV_IMM;
                insn->imm_val = const_val[insn->src_reg1] * const_val[insn->src_reg2];
                insn->is_constant = true;
                has_const[insn->dst_reg] = true;
                const_val[insn->dst_reg] = insn->imm_val;
                optimizations++;
            } else if (insn->dst_reg < OVIR_CPU_REG_COUNT) {
                has_const[insn->dst_reg] = false;
            }
        } else if (insn->opcode == OVIR_OP_RET || insn->opcode == OVIR_OP_JMP) {
            // Dead code elimination: unreachable instructions after RET/unconditional JMP
            for (uint32_t j = i + 1; j < ir->count; j++) {
                if (ir->instructions[j].opcode != OVIR_OP_NOP) {
                    ir->instructions[j].opcode = OVIR_OP_NOP;
                    optimizations++;
                }
            }
            break;
        } else if (insn->dst_reg < OVIR_CPU_REG_COUNT) {
            has_const[insn->dst_reg] = false;
        }
    }

    if (out_optimizations_applied) {
        *out_optimizations_applied = optimizations;
    }
    return OV_SUCCESS;
}

ov_status_t ovir_cpu_emit_x86_64(const ovir_ir_block_t *ir, ovir_compiled_block_t *out_compiled) {
    return ovir_cpu_emit_x86_64_extended(ir, out_compiled);
}

ov_status_t ovir_cpu_verify(const ovir_compiled_block_t *compiled) {
    if (!compiled) return OV_ERR_INVALID_PARAM;
    if (compiled->code_size == 0 || compiled->code_size > OVIR_CPU_MAX_CODE_SIZE) {
        return OV_ERR_VERIFICATION_FAILED;
    }
    // Must end with RET (0xc3)
    if (compiled->code_bytes[compiled->code_size - 1] != 0xc3) {
        return OV_ERR_VERIFICATION_FAILED;
    }
    ((ovir_compiled_block_t *)compiled)->verified = true;
    return OV_SUCCESS;
}

static bool check_condition(uint64_t flags, ovir_condition_t cond) {
    bool z = (flags & OV_CPU_FLAG_Z) != 0;
    bool n = (flags & OV_CPU_FLAG_N) != 0;
    bool c = (flags & OV_CPU_FLAG_C) != 0;
    bool v = (flags & OV_CPU_FLAG_V) != 0;

    switch (cond) {
        case OVIR_CC_AL: return true;
        case OVIR_CC_EQ: return z;
        case OVIR_CC_NE: return !z;
        case OVIR_CC_CS: return c;
        case OVIR_CC_CC: return !c;
        case OVIR_CC_MI: return n;
        case OVIR_CC_PL: return !n;
        case OVIR_CC_VS: return v;
        case OVIR_CC_VC: return !v;
        case OVIR_CC_HI: return c && !z;
        case OVIR_CC_LS: return !c || z;
        case OVIR_CC_GE: return n == v;
        case OVIR_CC_LT: return n != v;
        case OVIR_CC_GT: return !z && (n == v);
        case OVIR_CC_LE: return z || (n != v);
        default: return true;
    }
}

static void update_nz_flags(uint64_t *flags, uint64_t result) {
    if (result == 0) {
        *flags |= OV_CPU_FLAG_Z;
    } else {
        *flags &= ~OV_CPU_FLAG_Z;
    }
    if ((int64_t)result < 0) {
        *flags |= OV_CPU_FLAG_N;
    } else {
        *flags &= ~OV_CPU_FLAG_N;
    }
}

// 64KB simulated scratchpad memory for loads/stores in simulation mode
static uint8_t s_sim_ram[65536];

ov_status_t ovir_cpu_simulate(const ovir_ir_block_t *ir, ovir_cpu_state_t *state) {
    if (!ir || !state) return OV_ERR_INVALID_PARAM;

    state->halted = false;
    uint32_t pc_idx = 0;

    while (pc_idx < ir->count && !state->halted) {
        const ovir_insn_t *insn = &ir->instructions[pc_idx];
        state->cycles++;
        state->instructions_retired++;
        state->pc = insn->guest_pc;

        switch (insn->opcode) {
            case OVIR_OP_NOP:
                break;

            case OVIR_OP_MOV_IMM:
                if (insn->dst_reg < OVIR_CPU_REG_COUNT) {
                    state->registers[insn->dst_reg] = (uint64_t)insn->imm_val;
                }
                break;

            case OVIR_OP_MOV_REG:
                if (insn->dst_reg < OVIR_CPU_REG_COUNT && insn->src_reg1 < OVIR_CPU_REG_COUNT) {
                    state->registers[insn->dst_reg] = state->registers[insn->src_reg1];
                }
                break;

            case OVIR_OP_MVN:
                if (insn->dst_reg < OVIR_CPU_REG_COUNT) {
                    state->registers[insn->dst_reg] = (uint64_t)insn->imm_val;
                }
                break;

            case OVIR_OP_ADD:
            case OVIR_OP_ADDS:
                if (insn->dst_reg < OVIR_CPU_REG_COUNT &&
                    insn->src_reg1 < OVIR_CPU_REG_COUNT) {
                    uint64_t op1 = state->registers[insn->src_reg1];
                    uint64_t op2 = (insn->imm_val != 0 && insn->src_reg2 == 0) ? (uint64_t)insn->imm_val :
                                   (insn->src_reg2 < OVIR_CPU_REG_COUNT ? state->registers[insn->src_reg2] : 0);
                    uint64_t res = op1 + op2;
                    state->registers[insn->dst_reg] = res;
                    if (insn->sets_flags) {
                        update_nz_flags(&state->flags, res);
                        if (res < op1) state->flags |= OV_CPU_FLAG_C; else state->flags &= ~OV_CPU_FLAG_C;
                    }
                }
                break;

            case OVIR_OP_SUB:
            case OVIR_OP_SUBS:
                if (insn->dst_reg < OVIR_CPU_REG_COUNT &&
                    insn->src_reg1 < OVIR_CPU_REG_COUNT) {
                    uint64_t op1 = state->registers[insn->src_reg1];
                    uint64_t op2 = (insn->imm_val != 0 && insn->src_reg2 == 0) ? (uint64_t)insn->imm_val :
                                   (insn->src_reg2 < OVIR_CPU_REG_COUNT ? state->registers[insn->src_reg2] : 0);
                    uint64_t res = op1 - op2;
                    state->registers[insn->dst_reg] = res;
                    if (insn->sets_flags) {
                        update_nz_flags(&state->flags, res);
                        if (op1 >= op2) state->flags |= OV_CPU_FLAG_C; else state->flags &= ~OV_CPU_FLAG_C;
                    }
                }
                break;

            case OVIR_OP_MUL:
                if (insn->dst_reg < OVIR_CPU_REG_COUNT &&
                    insn->src_reg1 < OVIR_CPU_REG_COUNT &&
                    insn->src_reg2 < OVIR_CPU_REG_COUNT) {
                    state->registers[insn->dst_reg] = state->registers[insn->src_reg1] * state->registers[insn->src_reg2];
                }
                break;

            case OVIR_OP_NEG:
                if (insn->dst_reg < OVIR_CPU_REG_COUNT && insn->src_reg1 < OVIR_CPU_REG_COUNT) {
                    state->registers[insn->dst_reg] = (uint64_t)(-(int64_t)state->registers[insn->src_reg1]);
                }
                break;

            case OVIR_OP_AND:
            case OVIR_OP_ANDS:
                if (insn->dst_reg < OVIR_CPU_REG_COUNT &&
                    insn->src_reg1 < OVIR_CPU_REG_COUNT &&
                    insn->src_reg2 < OVIR_CPU_REG_COUNT) {
                    uint64_t res = state->registers[insn->src_reg1] & state->registers[insn->src_reg2];
                    state->registers[insn->dst_reg] = res;
                    if (insn->sets_flags) {
                        update_nz_flags(&state->flags, res);
                    }
                }
                break;

            case OVIR_OP_ORR:
                if (insn->dst_reg < OVIR_CPU_REG_COUNT &&
                    insn->src_reg1 < OVIR_CPU_REG_COUNT &&
                    insn->src_reg2 < OVIR_CPU_REG_COUNT) {
                    state->registers[insn->dst_reg] = state->registers[insn->src_reg1] | state->registers[insn->src_reg2];
                }
                break;

            case OVIR_OP_EOR:
                if (insn->dst_reg < OVIR_CPU_REG_COUNT &&
                    insn->src_reg1 < OVIR_CPU_REG_COUNT &&
                    insn->src_reg2 < OVIR_CPU_REG_COUNT) {
                    state->registers[insn->dst_reg] = state->registers[insn->src_reg1] ^ state->registers[insn->src_reg2];
                }
                break;

            case OVIR_OP_LSL:
                if (insn->dst_reg < OVIR_CPU_REG_COUNT &&
                    insn->src_reg1 < OVIR_CPU_REG_COUNT &&
                    insn->src_reg2 < OVIR_CPU_REG_COUNT) {
                    state->registers[insn->dst_reg] = state->registers[insn->src_reg1] << (state->registers[insn->src_reg2] & 0x3f);
                }
                break;

            case OVIR_OP_LSR:
                if (insn->dst_reg < OVIR_CPU_REG_COUNT &&
                    insn->src_reg1 < OVIR_CPU_REG_COUNT &&
                    insn->src_reg2 < OVIR_CPU_REG_COUNT) {
                    state->registers[insn->dst_reg] = state->registers[insn->src_reg1] >> (state->registers[insn->src_reg2] & 0x3f);
                }
                break;

            case OVIR_OP_ASR:
                if (insn->dst_reg < OVIR_CPU_REG_COUNT &&
                    insn->src_reg1 < OVIR_CPU_REG_COUNT &&
                    insn->src_reg2 < OVIR_CPU_REG_COUNT) {
                    state->registers[insn->dst_reg] = (uint64_t)((int64_t)state->registers[insn->src_reg1] >> (state->registers[insn->src_reg2] & 0x3f));
                }
                break;

            case OVIR_OP_CMP: {
                uint64_t op1 = (insn->src_reg1 < OVIR_CPU_REG_COUNT) ? state->registers[insn->src_reg1] : 0;
                uint64_t op2 = (insn->imm_val != 0 && insn->src_reg2 == 0) ? (uint64_t)insn->imm_val :
                               ((insn->src_reg2 < OVIR_CPU_REG_COUNT) ? state->registers[insn->src_reg2] : 0);
                uint64_t res = op1 - op2;
                update_nz_flags(&state->flags, res);
                if (op1 >= op2) state->flags |= OV_CPU_FLAG_C; else state->flags &= ~OV_CPU_FLAG_C;
                break;
            }

            case OVIR_OP_TST: {
                uint64_t op1 = (insn->src_reg1 < OVIR_CPU_REG_COUNT) ? state->registers[insn->src_reg1] : 0;
                uint64_t op2 = (insn->src_reg2 < OVIR_CPU_REG_COUNT) ? state->registers[insn->src_reg2] : 0;
                uint64_t res = op1 & op2;
                update_nz_flags(&state->flags, res);
                break;
            }

            case OVIR_OP_LOAD: {
                uint64_t base = (insn->src_reg1 < OVIR_CPU_REG_COUNT) ? state->registers[insn->src_reg1] : 0;
                uint64_t addr = base + (uint64_t)insn->offset;
                uint64_t val = 0;
                uint32_t mem_idx = (uint32_t)(addr % sizeof(s_sim_ram));
                if (insn->op_size == 1) val = s_sim_ram[mem_idx];
                else if (insn->op_size == 2) memcpy(&val, &s_sim_ram[mem_idx], 2);
                else if (insn->op_size == 4) memcpy(&val, &s_sim_ram[mem_idx], 4);
                else memcpy(&val, &s_sim_ram[mem_idx], 8);
                if (insn->dst_reg < OVIR_CPU_REG_COUNT) state->registers[insn->dst_reg] = val;
                break;
            }

            case OVIR_OP_STORE: {
                uint64_t base = (insn->src_reg1 < OVIR_CPU_REG_COUNT) ? state->registers[insn->src_reg1] : 0;
                uint64_t addr = base + (uint64_t)insn->offset;
                uint64_t val = (insn->dst_reg < OVIR_CPU_REG_COUNT) ? state->registers[insn->dst_reg] : 0;
                uint32_t mem_idx = (uint32_t)(addr % sizeof(s_sim_ram));
                if (insn->op_size == 1) s_sim_ram[mem_idx] = (uint8_t)val;
                else if (insn->op_size == 2) memcpy(&s_sim_ram[mem_idx], &val, 2);
                else if (insn->op_size == 4) memcpy(&s_sim_ram[mem_idx], &val, 4);
                else memcpy(&s_sim_ram[mem_idx], &val, 8);
                break;
            }

            case OVIR_OP_JMP: {
                int32_t target_idx = (int32_t)pc_idx + (int32_t)(insn->branch_target_disp / 4);
                if (target_idx >= 0 && (uint32_t)target_idx < ir->count) {
                    pc_idx = (uint32_t)target_idx;
                    continue;
                } else {
                    state->halted = true;
                }
                break;
            }

            case OVIR_OP_JMP_COND: {
                if (check_condition(state->flags, insn->cond)) {
                    int32_t target_idx = (int32_t)pc_idx + (int32_t)(insn->branch_target_disp / 4);
                    if (target_idx >= 0 && (uint32_t)target_idx < ir->count) {
                        pc_idx = (uint32_t)target_idx;
                        continue;
                    } else {
                        state->halted = true;
                    }
                }
                break;
            }

            case OVIR_OP_CBZ: {
                uint64_t val = (insn->dst_reg < OVIR_CPU_REG_COUNT) ? state->registers[insn->dst_reg] : 0;
                if (val == 0) {
                    int32_t target_idx = (int32_t)pc_idx + (int32_t)(insn->branch_target_disp / 4);
                    if (target_idx >= 0 && (uint32_t)target_idx < ir->count) {
                        pc_idx = (uint32_t)target_idx;
                        continue;
                    } else {
                        state->halted = true;
                    }
                }
                break;
            }

            case OVIR_OP_CBNZ: {
                uint64_t val = (insn->dst_reg < OVIR_CPU_REG_COUNT) ? state->registers[insn->dst_reg] : 0;
                if (val != 0) {
                    int32_t target_idx = (int32_t)pc_idx + (int32_t)(insn->branch_target_disp / 4);
                    if (target_idx >= 0 && (uint32_t)target_idx < ir->count) {
                        pc_idx = (uint32_t)target_idx;
                        continue;
                    } else {
                        state->halted = true;
                    }
                }
                break;
            }

            case OVIR_OP_VEC_ADD: {
                if (insn->dst_reg < OVIR_VEC_REG_COUNT &&
                    insn->src_reg1 < OVIR_VEC_REG_COUNT &&
                    insn->src_reg2 < OVIR_VEC_REG_COUNT) {
                    ovir_vec128_t *dst = &state->vregs[insn->dst_reg];
                    const ovir_vec128_t *s1 = &state->vregs[insn->src_reg1];
                    const ovir_vec128_t *s2 = &state->vregs[insn->src_reg2];
                    if (insn->vec_arrangement == OVIR_VEC_ARRANGE_2D) {
                        dst->u64[0] = s1->u64[0] + s2->u64[0];
                        dst->u64[1] = s1->u64[1] + s2->u64[1];
                    } else {
                        for (int l = 0; l < 4; l++) dst->u32[l] = s1->u32[l] + s2->u32[l];
                    }
                }
                break;
            }

            case OVIR_OP_VEC_SUB: {
                if (insn->dst_reg < OVIR_VEC_REG_COUNT &&
                    insn->src_reg1 < OVIR_VEC_REG_COUNT &&
                    insn->src_reg2 < OVIR_VEC_REG_COUNT) {
                    ovir_vec128_t *dst = &state->vregs[insn->dst_reg];
                    const ovir_vec128_t *s1 = &state->vregs[insn->src_reg1];
                    const ovir_vec128_t *s2 = &state->vregs[insn->src_reg2];
                    if (insn->vec_arrangement == OVIR_VEC_ARRANGE_2D) {
                        dst->u64[0] = s1->u64[0] - s2->u64[0];
                        dst->u64[1] = s1->u64[1] - s2->u64[1];
                    } else {
                        for (int l = 0; l < 4; l++) dst->u32[l] = s1->u32[l] - s2->u32[l];
                    }
                }
                break;
            }

            case OVIR_OP_VEC_MUL: {
                if (insn->dst_reg < OVIR_VEC_REG_COUNT &&
                    insn->src_reg1 < OVIR_VEC_REG_COUNT &&
                    insn->src_reg2 < OVIR_VEC_REG_COUNT) {
                    ovir_vec128_t *dst = &state->vregs[insn->dst_reg];
                    const ovir_vec128_t *s1 = &state->vregs[insn->src_reg1];
                    const ovir_vec128_t *s2 = &state->vregs[insn->src_reg2];
                    for (int l = 0; l < 4; l++) dst->u32[l] = s1->u32[l] * s2->u32[l];
                }
                break;
            }

            case OVIR_OP_VEC_AND: {
                if (insn->dst_reg < OVIR_VEC_REG_COUNT &&
                    insn->src_reg1 < OVIR_VEC_REG_COUNT &&
                    insn->src_reg2 < OVIR_VEC_REG_COUNT) {
                    state->vregs[insn->dst_reg].u64[0] = state->vregs[insn->src_reg1].u64[0] & state->vregs[insn->src_reg2].u64[0];
                    state->vregs[insn->dst_reg].u64[1] = state->vregs[insn->src_reg1].u64[1] & state->vregs[insn->src_reg2].u64[1];
                }
                break;
            }

            case OVIR_OP_VEC_ORR: {
                if (insn->dst_reg < OVIR_VEC_REG_COUNT &&
                    insn->src_reg1 < OVIR_VEC_REG_COUNT &&
                    insn->src_reg2 < OVIR_VEC_REG_COUNT) {
                    state->vregs[insn->dst_reg].u64[0] = state->vregs[insn->src_reg1].u64[0] | state->vregs[insn->src_reg2].u64[0];
                    state->vregs[insn->dst_reg].u64[1] = state->vregs[insn->src_reg1].u64[1] | state->vregs[insn->src_reg2].u64[1];
                }
                break;
            }

            case OVIR_OP_VEC_EOR: {
                if (insn->dst_reg < OVIR_VEC_REG_COUNT &&
                    insn->src_reg1 < OVIR_VEC_REG_COUNT &&
                    insn->src_reg2 < OVIR_VEC_REG_COUNT) {
                    state->vregs[insn->dst_reg].u64[0] = state->vregs[insn->src_reg1].u64[0] ^ state->vregs[insn->src_reg2].u64[0];
                    state->vregs[insn->dst_reg].u64[1] = state->vregs[insn->src_reg1].u64[1] ^ state->vregs[insn->src_reg2].u64[1];
                }
                break;
            }

            case OVIR_OP_VEC_DUP: {
                if (insn->dst_reg < OVIR_VEC_REG_COUNT && insn->src_reg1 < OVIR_CPU_REG_COUNT) {
                    uint32_t val = (uint32_t)state->registers[insn->src_reg1];
                    for (int l = 0; l < 4; l++) state->vregs[insn->dst_reg].u32[l] = val;
                }
                break;
            }

            case OVIR_OP_VEC_MOVI: {
                if (insn->dst_reg < OVIR_VEC_REG_COUNT) {
                    uint32_t val = (uint32_t)insn->imm_val;
                    for (int l = 0; l < 4; l++) state->vregs[insn->dst_reg].u32[l] = val;
                }
                break;
            }

            case OVIR_OP_BARRIER:
                // Barrier handled
                break;

            case OVIR_OP_SYSCALL:
                // Handled or routed to syscall table
                break;

            case OVIR_OP_RET:
                state->halted = true;
                break;

            default:
                break;
        }

        pc_idx++;
    }

    return OV_SUCCESS;
}
