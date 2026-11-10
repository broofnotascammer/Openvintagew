#include "ovir_gpu_ir.h"
#include <string.h>
#include <stdio.h>

const char *ovir_gpu_opcode_name(ovir_gpu_opcode_t op) {
    switch (op) {
        case OVIR_GPU_OP_NOP: return "NOP";
        case OVIR_GPU_OP_LOAD: return "LOAD";
        case OVIR_GPU_OP_STORE: return "STORE";
        case OVIR_GPU_OP_ASSIGN: return "ASSIGN";
        case OVIR_GPU_OP_VEC_CONSTRUCT: return "VEC_CONSTRUCT";
        case OVIR_GPU_OP_VEC_EXTRACT: return "VEC_EXTRACT";
        case OVIR_GPU_OP_VEC_SWIZZLE: return "VEC_SWIZZLE";
        case OVIR_GPU_OP_ADD: return "ADD";
        case OVIR_GPU_OP_SUB: return "SUB";
        case OVIR_GPU_OP_MUL: return "MUL";
        case OVIR_GPU_OP_DIV: return "DIV";
        case OVIR_GPU_OP_NEG: return "NEG";
        case OVIR_GPU_OP_DOT: return "DOT";
        case OVIR_GPU_OP_MAT_MUL_VEC: return "MAT_MUL_VEC";
        case OVIR_GPU_OP_LOGICAL_AND: return "LOGICAL_AND";
        case OVIR_GPU_OP_LOGICAL_OR: return "LOGICAL_OR";
        case OVIR_GPU_OP_LOGICAL_NOT: return "LOGICAL_NOT";
        case OVIR_GPU_OP_CMP_EQ: return "CMP_EQ";
        case OVIR_GPU_OP_CMP_NEQ: return "CMP_NEQ";
        case OVIR_GPU_OP_CMP_LT: return "CMP_LT";
        case OVIR_GPU_OP_CMP_GT: return "CMP_GT";
        case OVIR_GPU_OP_CMP_LTE: return "CMP_LTE";
        case OVIR_GPU_OP_CMP_GTE: return "CMP_GTE";
        case OVIR_GPU_OP_LABEL: return "LABEL";
        case OVIR_GPU_OP_BRANCH: return "BRANCH";
        case OVIR_GPU_OP_BRANCH_COND: return "BRANCH_COND";
        case OVIR_GPU_OP_RETURN: return "RETURN";
        case OVIR_GPU_OP_TEX_SAMPLE: return "TEX_SAMPLE";
        default: return "UNKNOWN_OP";
    }
}

const char *ovir_gpu_type_kind_name(ovir_gpu_type_kind_t kind) {
    switch (kind) {
        case OVIR_GPU_TYPE_VOID: return "void";
        case OVIR_GPU_TYPE_BOOL: return "bool";
        case OVIR_GPU_TYPE_INT32: return "int32";
        case OVIR_GPU_TYPE_UINT32: return "uint32";
        case OVIR_GPU_TYPE_FLOAT32: return "float32";
        case OVIR_GPU_TYPE_VEC2: return "vec2";
        case OVIR_GPU_TYPE_VEC3: return "vec3";
        case OVIR_GPU_TYPE_VEC4: return "vec4";
        case OVIR_GPU_TYPE_MAT4: return "mat4";
        case OVIR_GPU_TYPE_SAMPLER2D: return "sampler2D";
        default: return "unknown_type";
    }
}

ov_status_t ovir_gpu_ir_init(ovir_gpu_shader_ir_t *ir, ovir_spirv_exec_model_t stage, const char *entry_point) {
    if (!ir) return OV_ERR_INVALID_PARAM;
    memset(ir, 0, sizeof(ovir_gpu_shader_ir_t));
    ir->stage = stage;
    if (entry_point) {
        strncpy(ir->entry_point, entry_point, sizeof(ir->entry_point) - 1);
    } else {
        strncpy(ir->entry_point, "main", sizeof(ir->entry_point) - 1);
    }
    ir->is_valid = true;
    return OV_SUCCESS;
}

ov_status_t ovir_gpu_ir_add_type(ovir_gpu_shader_ir_t *ir, uint32_t type_id, ovir_gpu_type_kind_t kind, uint32_t vec_size, uint32_t bit_width) {
    if (!ir || ir->type_count >= OVIR_GPU_IR_MAX_TYPES) return OV_ERR_RESOURCE_EXHAUSTED;
    ovir_gpu_type_t *t = &ir->types[ir->type_count++];
    t->type_id = type_id;
    t->kind = kind;
    t->vector_size = vec_size > 0 ? vec_size : 1;
    t->bit_width = bit_width > 0 ? bit_width : 32;
    return OV_SUCCESS;
}

ov_status_t ovir_gpu_ir_add_constant_f32(ovir_gpu_shader_ir_t *ir, uint32_t const_id, uint32_t type_id, float val) {
    if (!ir || ir->constant_count >= OVIR_GPU_IR_MAX_CONSTANTS) return OV_ERR_RESOURCE_EXHAUSTED;
    ovir_gpu_constant_t *c = &ir->constants[ir->constant_count++];
    c->const_id = const_id;
    c->type_id = type_id;
    c->value.f32 = val;
    c->is_composite = false;
    return OV_SUCCESS;
}

ov_status_t ovir_gpu_ir_add_constant_vec4(ovir_gpu_shader_ir_t *ir, uint32_t const_id, uint32_t type_id, float x, float y, float z, float w) {
    if (!ir || ir->constant_count >= OVIR_GPU_IR_MAX_CONSTANTS) return OV_ERR_RESOURCE_EXHAUSTED;
    ovir_gpu_constant_t *c = &ir->constants[ir->constant_count++];
    c->const_id = const_id;
    c->type_id = type_id;
    c->value.vec4[0] = x;
    c->value.vec4[1] = y;
    c->value.vec4[2] = z;
    c->value.vec4[3] = w;
    c->is_composite = true;
    return OV_SUCCESS;
}

ov_status_t ovir_gpu_ir_add_variable(ovir_gpu_shader_ir_t *ir, uint32_t var_id, uint32_t type_id, ovir_spirv_storage_class_t sc, const char *name) {
    if (!ir || ir->variable_count >= OVIR_GPU_IR_MAX_VARS) return OV_ERR_RESOURCE_EXHAUSTED;
    ovir_gpu_var_t *v = &ir->variables[ir->variable_count++];
    v->var_id = var_id;
    v->type_id = type_id;
    v->storage_class = sc;
    if (name) {
        strncpy(v->name, name, sizeof(v->name) - 1);
    }
    return OV_SUCCESS;
}

ov_status_t ovir_gpu_ir_add_instruction(ovir_gpu_shader_ir_t *ir, ovir_gpu_opcode_t op, uint32_t res_id, uint32_t type_id, uint32_t s1, uint32_t s2) {
    if (!ir || ir->instruction_count >= OVIR_GPU_IR_MAX_INSN) return OV_ERR_RESOURCE_EXHAUSTED;
    ovir_gpu_insn_t *insn = &ir->instructions[ir->instruction_count++];
    insn->opcode = op;
    insn->result_id = res_id;
    insn->type_id = type_id;
    insn->src1_id = s1;
    insn->src2_id = s2;
    insn->has_side_effects = (op == OVIR_GPU_OP_STORE || op == OVIR_GPU_OP_BRANCH || op == OVIR_GPU_OP_BRANCH_COND || op == OVIR_GPU_OP_RETURN);
    return OV_SUCCESS;
}

ov_status_t ovir_gpu_ir_validate(const ovir_gpu_shader_ir_t *ir, char *out_err, size_t err_size) {
    if (!ir) return OV_ERR_INVALID_PARAM;
    if (ir->instruction_count == 0) {
        if (out_err && err_size > 0) snprintf(out_err, err_size, "Empty GPU IR: 0 instructions");
        return OV_ERR_VERIFICATION_FAILED;
    }

    // Check entry point
    if (strlen(ir->entry_point) == 0) {
        if (out_err && err_size > 0) snprintf(out_err, err_size, "Missing shader entry point");
        return OV_ERR_VERIFICATION_FAILED;
    }

    // Check last instruction is a return
    ovir_gpu_opcode_t last_op = ir->instructions[ir->instruction_count - 1].opcode;
    if (last_op != OVIR_GPU_OP_RETURN) {
        if (out_err && err_size > 0) snprintf(out_err, err_size, "Control flow must terminate with RETURN");
        return OV_ERR_VERIFICATION_FAILED;
    }

    return OV_SUCCESS;
}

ov_status_t ovir_gpu_ir_dump(const ovir_gpu_shader_ir_t *ir, char *out_buf, size_t buf_size) {
    if (!ir || !out_buf || buf_size == 0) return OV_ERR_INVALID_PARAM;
    size_t written = 0;

    written += snprintf(out_buf + written, buf_size - written,
                        "; OVIR-GPU Shader IR [Stage: %s, Entry: %s, Insns: %u]\n",
                        ovir_spirv_exec_model_name(ir->stage), ir->entry_point, ir->instruction_count);

    written += snprintf(out_buf + written, buf_size - written, "; Types (%u):\n", ir->type_count);
    for (uint32_t i = 0; i < ir->type_count && written < buf_size - 1; i++) {
        written += snprintf(out_buf + written, buf_size - written,
                            "  %%t%u = %s (vec: %u, bits: %u)\n",
                            ir->types[i].type_id, ovir_gpu_type_kind_name(ir->types[i].kind),
                            ir->types[i].vector_size, ir->types[i].bit_width);
    }

    written += snprintf(out_buf + written, buf_size - written, "; Variables (%u):\n", ir->variable_count);
    for (uint32_t i = 0; i < ir->variable_count && written < buf_size - 1; i++) {
        written += snprintf(out_buf + written, buf_size - written,
                            "  %%v%u = Var [Class: %s, Name: %s",
                            ir->variables[i].var_id,
                            ovir_spirv_storage_class_name(ir->variables[i].storage_class),
                            ir->variables[i].name[0] ? ir->variables[i].name : "unnamed");
        if (ir->variables[i].has_location) {
            written += snprintf(out_buf + written, buf_size - written, ", Loc: %u", ir->variables[i].location);
        }
        if (ir->variables[i].has_binding) {
            written += snprintf(out_buf + written, buf_size - written, ", Binding: %u", ir->variables[i].binding);
        }
        written += snprintf(out_buf + written, buf_size - written, "]\n");
    }

    written += snprintf(out_buf + written, buf_size - written, "; Instructions (%u):\n", ir->instruction_count);
    for (uint32_t i = 0; i < ir->instruction_count && written < buf_size - 1; i++) {
        const ovir_gpu_insn_t *insn = &ir->instructions[i];
        written += snprintf(out_buf + written, buf_size - written,
                            "  %%r%u = %-12s %%t%u, %%r%u, %%r%u\n",
                            insn->result_id, ovir_gpu_opcode_name(insn->opcode),
                            insn->type_id, insn->src1_id, insn->src2_id);
    }

    return OV_SUCCESS;
}

ov_status_t ovir_gpu_ir_from_spirv(const uint32_t *words, size_t word_count, ovir_gpu_shader_ir_t *out_ir) {
    if (!words || !out_ir) return OV_ERR_INVALID_PARAM;

    ovir_spirv_module_t spv;
    ov_status_t st = ovir_spirv_parse(words, word_count, &spv);
    if (st != OV_SUCCESS) {
        return st;
    }

    ovir_spirv_exec_model_t stage = (spv.entry_point_count > 0) ? spv.entry_points[0].exec_model : OVIR_SPIRV_EXEC_FRAGMENT;
    const char *ep_name = (spv.entry_point_count > 0) ? spv.entry_points[0].name : "main";

    ovir_gpu_ir_init(out_ir, stage, ep_name);

    // Import parsed variables
    for (uint32_t i = 0; i < spv.variable_count; i++) {
        ovir_gpu_ir_add_variable(out_ir, spv.variables[i].id, spv.variables[i].type_id,
                                 spv.variables[i].storage_class, spv.variables[i].name);
        out_ir->variables[out_ir->variable_count - 1].has_location = spv.variables[i].has_location;
        out_ir->variables[out_ir->variable_count - 1].location = spv.variables[i].location;
        out_ir->variables[out_ir->variable_count - 1].has_binding = spv.variables[i].has_binding;
        out_ir->variables[out_ir->variable_count - 1].binding = spv.variables[i].binding;
        out_ir->variables[out_ir->variable_count - 1].has_descriptor_set = spv.variables[i].has_descriptor_set;
        out_ir->variables[out_ir->variable_count - 1].descriptor_set = spv.variables[i].descriptor_set;
        out_ir->variables[out_ir->variable_count - 1].has_builtin = spv.variables[i].has_builtin;
        out_ir->variables[out_ir->variable_count - 1].builtin = spv.variables[i].builtin;
    }

    // Second pass: Parse instructions into IR stream
    size_t offset = 5;
    while (offset < word_count) {
        uint32_t first_word = words[offset];
        uint16_t length = (first_word >> 16) & 0xffff;
        uint16_t opcode = first_word & 0xffff;

        if (length == 0 || offset + length > word_count) break;

        switch ((ovir_spirv_opcode_t)opcode) {
            case OVIR_SPIRV_OP_TYPE_VOID:
                ovir_gpu_ir_add_type(out_ir, words[offset + 1], OVIR_GPU_TYPE_VOID, 1, 0);
                break;
            case OVIR_SPIRV_OP_TYPE_FLOAT:
                ovir_gpu_ir_add_type(out_ir, words[offset + 1], OVIR_GPU_TYPE_FLOAT32, 1, words[offset + 2]);
                break;
            case OVIR_SPIRV_OP_TYPE_INT:
                ovir_gpu_ir_add_type(out_ir, words[offset + 1], OVIR_GPU_TYPE_INT32, 1, words[offset + 2]);
                break;
            case OVIR_SPIRV_OP_TYPE_VECTOR:
                ovir_gpu_ir_add_type(out_ir, words[offset + 1], OVIR_GPU_TYPE_VEC4, words[offset + 3], 32);
                break;
            case OVIR_SPIRV_OP_CONSTANT: {
                if (length >= 4) {
                    uint32_t type_id = words[offset + 1];
                    uint32_t const_id = words[offset + 2];
                    float fval;
                    memcpy(&fval, &words[offset + 3], 4);
                    ovir_gpu_ir_add_constant_f32(out_ir, const_id, type_id, fval);
                }
                break;
            }
            case OVIR_SPIRV_OP_CONSTANT_COMPOSITE: {
                if (length >= 7) {
                    uint32_t type_id = words[offset + 1];
                    uint32_t const_id = words[offset + 2];
                    float x, y, z, w;
                    memcpy(&x, &words[offset + 3], 4);
                    memcpy(&y, &words[offset + 4], 4);
                    memcpy(&z, &words[offset + 5], 4);
                    memcpy(&w, &words[offset + 6], 4);
                    ovir_gpu_ir_add_constant_vec4(out_ir, const_id, type_id, x, y, z, w);
                }
                break;
            }
            case OVIR_SPIRV_OP_LOAD:
                ovir_gpu_ir_add_instruction(out_ir, OVIR_GPU_OP_LOAD, words[offset + 2], words[offset + 1], words[offset + 3], 0);
                break;
            case OVIR_SPIRV_OP_STORE:
                ovir_gpu_ir_add_instruction(out_ir, OVIR_GPU_OP_STORE, 0, 0, words[offset + 1], words[offset + 2]);
                break;
            case OVIR_SPIRV_OP_FADD:
            case OVIR_SPIRV_OP_IADD:
                ovir_gpu_ir_add_instruction(out_ir, OVIR_GPU_OP_ADD, words[offset + 2], words[offset + 1], words[offset + 3], words[offset + 4]);
                break;
            case OVIR_SPIRV_OP_FSUB:
            case OVIR_SPIRV_OP_ISUB:
                ovir_gpu_ir_add_instruction(out_ir, OVIR_GPU_OP_SUB, words[offset + 2], words[offset + 1], words[offset + 3], words[offset + 4]);
                break;
            case OVIR_SPIRV_OP_FMUL:
            case OVIR_SPIRV_OP_IMUL:
                ovir_gpu_ir_add_instruction(out_ir, OVIR_GPU_OP_MUL, words[offset + 2], words[offset + 1], words[offset + 3], words[offset + 4]);
                break;
            case OVIR_SPIRV_OP_FDIV:
            case OVIR_SPIRV_OP_SDIV:
                ovir_gpu_ir_add_instruction(out_ir, OVIR_GPU_OP_DIV, words[offset + 2], words[offset + 1], words[offset + 3], words[offset + 4]);
                break;
            case OVIR_SPIRV_OP_DOT:
                ovir_gpu_ir_add_instruction(out_ir, OVIR_GPU_OP_DOT, words[offset + 2], words[offset + 1], words[offset + 3], words[offset + 4]);
                break;
            case OVIR_SPIRV_OP_RETURN:
            case OVIR_SPIRV_OP_RETURN_VALUE:
                ovir_gpu_ir_add_instruction(out_ir, OVIR_GPU_OP_RETURN, 0, 0, 0, 0);
                break;
            default:
                break;
        }

        offset += length;
    }

    return OV_SUCCESS;
}
