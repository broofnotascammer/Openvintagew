#include "ovir_gpu_opt.h"
#include <string.h>

static const ovir_gpu_constant_t *find_constant(const ovir_gpu_shader_ir_t *ir, uint32_t const_id) {
    for (uint32_t i = 0; i < ir->constant_count; i++) {
        if (ir->constants[i].const_id == const_id) {
            return &ir->constants[i];
        }
    }
    return NULL;
}

ov_status_t ovir_gpu_opt_constant_fold(ovir_gpu_shader_ir_t *ir, uint32_t *out_folded_count) {
    if (!ir) return OV_ERR_INVALID_PARAM;
    uint32_t folded = 0;

    for (uint32_t i = 0; i < ir->instruction_count; i++) {
        ovir_gpu_insn_t *insn = &ir->instructions[i];

        // Fold float arithmetic operations if both operands are known constants
        if (insn->opcode == OVIR_GPU_OP_ADD || insn->opcode == OVIR_GPU_OP_SUB || insn->opcode == OVIR_GPU_OP_MUL) {
            const ovir_gpu_constant_t *c1 = find_constant(ir, insn->src1_id);
            const ovir_gpu_constant_t *c2 = find_constant(ir, insn->src2_id);

            if (c1 && c2 && !c1->is_composite && !c2->is_composite) {
                float res = 0.0f;
                if (insn->opcode == OVIR_GPU_OP_ADD) res = c1->value.f32 + c2->value.f32;
                else if (insn->opcode == OVIR_GPU_OP_SUB) res = c1->value.f32 - c2->value.f32;
                else if (insn->opcode == OVIR_GPU_OP_MUL) res = c1->value.f32 * c2->value.f32;

                // Add folded constant to IR constant table
                if (ir->constant_count < OVIR_GPU_IR_MAX_CONSTANTS) {
                    uint32_t new_cid = insn->result_id;
                    ovir_gpu_ir_add_constant_f32(ir, new_cid, insn->type_id, res);

                    // Transform instruction to assignment from constant
                    insn->opcode = OVIR_GPU_OP_ASSIGN;
                    insn->src1_id = new_cid;
                    insn->src2_id = 0;
                    folded++;
                }
            }
        }
    }

    if (out_folded_count) *out_folded_count = folded;
    return OV_SUCCESS;
}

ov_status_t ovir_gpu_opt_dead_code_elimination(ovir_gpu_shader_ir_t *ir, uint32_t *out_dce_count) {
    if (!ir) return OV_ERR_INVALID_PARAM;
    uint32_t eliminated = 0;

    bool return_encountered = false;
    for (uint32_t i = 0; i < ir->instruction_count; i++) {
        ovir_gpu_insn_t *insn = &ir->instructions[i];

        if (return_encountered) {
            // Unreachable code after RETURN
            if (insn->opcode != OVIR_GPU_OP_NOP) {
                insn->opcode = OVIR_GPU_OP_NOP;
                insn->result_id = 0;
                eliminated++;
            }
        } else if (insn->opcode == OVIR_GPU_OP_RETURN) {
            return_encountered = true;
        }
    }

    // Compact out NOPs to actually shrink instruction stream
    if (eliminated > 0) {
        uint32_t write_idx = 0;
        for (uint32_t i = 0; i < ir->instruction_count; i++) {
            if (ir->instructions[i].opcode != OVIR_GPU_OP_NOP) {
                if (write_idx != i) {
                    ir->instructions[write_idx] = ir->instructions[i];
                }
                write_idx++;
            }
        }
        ir->instruction_count = write_idx;
    }

    if (out_dce_count) *out_dce_count = eliminated;
    return OV_SUCCESS;
}

ov_status_t ovir_gpu_optimize(ovir_gpu_shader_ir_t *ir, ovir_gpu_opt_stats_t *out_stats) {
    if (!ir) return OV_ERR_INVALID_PARAM;
    ovir_gpu_opt_stats_t stats = {0};

    ovir_gpu_opt_constant_fold(ir, &stats.constant_folds_applied);
    ovir_gpu_opt_dead_code_elimination(ir, &stats.dead_instructions_eliminated);

    if (out_stats) *out_stats = stats;
    return OV_SUCCESS;
}
