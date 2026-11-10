#include "ovir_spirv.h"
#include <string.h>
#include <stdio.h>

const char *ovir_spirv_exec_model_name(ovir_spirv_exec_model_t model) {
    switch (model) {
        case OVIR_SPIRV_EXEC_VERTEX: return "Vertex";
        case OVIR_SPIRV_EXEC_TESSELLATION_CONTROL: return "TessellationControl";
        case OVIR_SPIRV_EXEC_TESSELLATION_EVALUATION: return "TessellationEvaluation";
        case OVIR_SPIRV_EXEC_GEOMETRY: return "Geometry";
        case OVIR_SPIRV_EXEC_FRAGMENT: return "Fragment";
        case OVIR_SPIRV_EXEC_GLCOMPUTE: return "GLCompute";
        case OVIR_SPIRV_EXEC_KERNEL: return "Kernel";
        default: return "Unknown";
    }
}

const char *ovir_spirv_storage_class_name(ovir_spirv_storage_class_t sc) {
    switch (sc) {
        case OVIR_SPIRV_STORAGE_UNIFORM_CONSTANT: return "UniformConstant";
        case OVIR_SPIRV_STORAGE_INPUT: return "Input";
        case OVIR_SPIRV_STORAGE_UNIFORM: return "Uniform";
        case OVIR_SPIRV_STORAGE_OUTPUT: return "Output";
        case OVIR_SPIRV_STORAGE_WORKGROUP: return "Workgroup";
        case OVIR_SPIRV_STORAGE_CROSS_WORKGROUP: return "CrossWorkgroup";
        case OVIR_SPIRV_STORAGE_PRIVATE: return "Private";
        case OVIR_SPIRV_STORAGE_FUNCTION: return "Function";
        case OVIR_SPIRV_STORAGE_GENERIC: return "Generic";
        case OVIR_SPIRV_STORAGE_PUSH_CONSTANT: return "PushConstant";
        case OVIR_SPIRV_STORAGE_IMAGE: return "Image";
        case OVIR_SPIRV_STORAGE_STORAGE_BUFFER: return "StorageBuffer";
        default: return "Unknown";
    }
}

const char *ovir_spirv_opcode_name(ovir_spirv_opcode_t op) {
    switch (op) {
        case OVIR_SPIRV_OP_NOP: return "OpNop";
        case OVIR_SPIRV_OP_SOURCE: return "OpSource";
        case OVIR_SPIRV_OP_NAME: return "OpName";
        case OVIR_SPIRV_OP_MEMBER_NAME: return "OpMemberName";
        case OVIR_SPIRV_OP_STRING: return "OpString";
        case OVIR_SPIRV_OP_EXTENSION: return "OpExtension";
        case OVIR_SPIRV_OP_EXT_INST_IMPORT: return "OpExtInstImport";
        case OVIR_SPIRV_OP_MEMORY_MODEL: return "OpMemoryModel";
        case OVIR_SPIRV_OP_ENTRY_POINT: return "OpEntryPoint";
        case OVIR_SPIRV_OP_EXECUTION_MODE: return "OpExecutionMode";
        case OVIR_SPIRV_OP_CAPABILITY: return "OpCapability";
        case OVIR_SPIRV_OP_TYPE_VOID: return "OpTypeVoid";
        case OVIR_SPIRV_OP_TYPE_BOOL: return "OpTypeBool";
        case OVIR_SPIRV_OP_TYPE_INT: return "OpTypeInt";
        case OVIR_SPIRV_OP_TYPE_FLOAT: return "OpTypeFloat";
        case OVIR_SPIRV_OP_TYPE_VECTOR: return "OpTypeVector";
        case OVIR_SPIRV_OP_TYPE_MATRIX: return "OpTypeMatrix";
        case OVIR_SPIRV_OP_TYPE_IMAGE: return "OpTypeImage";
        case OVIR_SPIRV_OP_TYPE_SAMPLER: return "OpTypeSampler";
        case OVIR_SPIRV_OP_TYPE_SAMPLED_IMAGE: return "OpTypeSampledImage";
        case OVIR_SPIRV_OP_TYPE_ARRAY: return "OpTypeArray";
        case OVIR_SPIRV_OP_TYPE_STRUCT: return "OpTypeStruct";
        case OVIR_SPIRV_OP_TYPE_POINTER: return "OpTypePointer";
        case OVIR_SPIRV_OP_TYPE_FUNCTION: return "OpTypeFunction";
        case OVIR_SPIRV_OP_CONSTANT_TRUE: return "OpConstantTrue";
        case OVIR_SPIRV_OP_CONSTANT_FALSE: return "OpConstantFalse";
        case OVIR_SPIRV_OP_CONSTANT: return "OpConstant";
        case OVIR_SPIRV_OP_CONSTANT_COMPOSITE: return "OpConstantComposite";
        case OVIR_SPIRV_OP_FUNCTION: return "OpFunction";
        case OVIR_SPIRV_OP_FUNCTION_PARAMETER: return "OpFunctionParameter";
        case OVIR_SPIRV_OP_FUNCTION_END: return "OpFunctionEnd";
        case OVIR_SPIRV_OP_VARIABLE: return "OpVariable";
        case OVIR_SPIRV_OP_LOAD: return "OpLoad";
        case OVIR_SPIRV_OP_STORE: return "OpStore";
        case OVIR_SPIRV_OP_ACCESS_CHAIN: return "OpAccessChain";
        case OVIR_SPIRV_OP_DECORATE: return "OpDecorate";
        case OVIR_SPIRV_OP_MEMBER_DECORATE: return "OpMemberDecorate";
        case OVIR_SPIRV_OP_VECTOR_SHUFFLE: return "OpVectorShuffle";
        case OVIR_SPIRV_OP_COMPOSITE_CONSTRUCT: return "OpCompositeConstruct";
        case OVIR_SPIRV_OP_COMPOSITE_EXTRACT: return "OpCompositeExtract";
        case OVIR_SPIRV_OP_COMPOSITE_INSERT: return "OpCompositeInsert";
        case OVIR_SPIRV_OP_LABEL: return "OpLabel";
        case OVIR_SPIRV_OP_BRANCH: return "OpBranch";
        case OVIR_SPIRV_OP_BRANCH_CONDITIONAL: return "OpBranchConditional";
        case OVIR_SPIRV_OP_RETURN: return "OpReturn";
        case OVIR_SPIRV_OP_RETURN_VALUE: return "OpReturnValue";
        case OVIR_SPIRV_OP_FADD: return "OpFAdd";
        case OVIR_SPIRV_OP_FSUB: return "OpFSub";
        case OVIR_SPIRV_OP_FMUL: return "OpFMul";
        case OVIR_SPIRV_OP_FDIV: return "OpFDiv";
        case OVIR_SPIRV_OP_FNEGATE: return "OpFNegate";
        case OVIR_SPIRV_OP_DOT: return "OpDot";
        case OVIR_SPIRV_OP_VECTOR_TIMES_SCALAR: return "OpVectorTimesScalar";
        case OVIR_SPIRV_OP_MATRIX_TIMES_VECTOR: return "OpMatrixTimesVector";
        case OVIR_SPIRV_OP_IADD: return "OpIAdd";
        case OVIR_SPIRV_OP_ISUB: return "OpISub";
        case OVIR_SPIRV_OP_IMUL: return "OpIMul";
        case OVIR_SPIRV_OP_UDIV: return "OpUDiv";
        case OVIR_SPIRV_OP_SDIV: return "OpSDiv";
        default: return "OpOther";
    }
}

ov_status_t ovir_spirv_validate_header(const uint32_t *words, size_t word_count, ovir_spirv_module_t *out_module) {
    if (!words || !out_module) return OV_ERR_INVALID_PARAM;
    memset(out_module, 0, sizeof(ovir_spirv_module_t));

    // A valid SPIR-V module header has at least 5 32-bit words
    if (word_count < 5) {
        snprintf(out_module->validation_error, sizeof(out_module->validation_error),
                 "SPIR-V header truncated: word count %zu < 5", word_count);
        out_module->is_valid = false;
        return OV_ERR_VERIFICATION_FAILED;
    }

    out_module->magic = words[0];
    out_module->version = words[1];
    out_module->generator_id = words[2];
    out_module->id_bound = words[3];
    out_module->schema = words[4];
    out_module->word_count = (uint32_t)word_count;

    if (out_module->magic != OVIR_SPIRV_MAGIC) {
        snprintf(out_module->validation_error, sizeof(out_module->validation_error),
                 "Invalid SPIR-V magic: 0x%08x (expected 0x%08x)", out_module->magic, OVIR_SPIRV_MAGIC);
        out_module->is_valid = false;
        return OV_ERR_VERIFICATION_FAILED;
    }

    if (out_module->id_bound == 0) {
        snprintf(out_module->validation_error, sizeof(out_module->validation_error),
                 "Invalid SPIR-V ID bound: 0");
        out_module->is_valid = false;
        return OV_ERR_VERIFICATION_FAILED;
    }

    out_module->is_valid = true;
    return OV_SUCCESS;
}

ov_status_t ovir_spirv_parse(const uint32_t *words, size_t word_count, ovir_spirv_module_t *out_module) {
    ov_status_t st = ovir_spirv_validate_header(words, word_count, out_module);
    if (st != OV_SUCCESS) {
        return st;
    }

    size_t offset = 5;
    while (offset < word_count) {
        uint32_t first_word = words[offset];
        uint16_t length = (first_word >> 16) & 0xffff;
        uint16_t opcode = first_word & 0xffff;

        if (length == 0) {
            snprintf(out_module->validation_error, sizeof(out_module->validation_error),
                     "Malformed SPIR-V instruction at word %zu: word count length is 0", offset);
            out_module->is_valid = false;
            return OV_ERR_VERIFICATION_FAILED;
        }

        if (offset + length > word_count) {
            snprintf(out_module->validation_error, sizeof(out_module->validation_error),
                     "Truncated instruction opcode %u: required %u words, stream has %zu remaining",
                     opcode, length, word_count - offset);
            out_module->is_valid = false;
            return OV_ERR_VERIFICATION_FAILED;
        }

        // Parse key declarative opcodes
        switch ((ovir_spirv_opcode_t)opcode) {
            case OVIR_SPIRV_OP_CAPABILITY: {
                if (length >= 2 && out_module->capability_count < OVIR_SPIRV_MAX_CAPABILITIES) {
                    out_module->capabilities[out_module->capability_count++] = words[offset + 1];
                }
                break;
            }
            case OVIR_SPIRV_OP_ENTRY_POINT: {
                if (length >= 4 && out_module->entry_point_count < OVIR_SPIRV_MAX_ENTRY_POINTS) {
                    ovir_spirv_entry_point_t *ep = &out_module->entry_points[out_module->entry_point_count++];
                    ep->exec_model = (ovir_spirv_exec_model_t)words[offset + 1];
                    ep->id = words[offset + 2];
                    
                    // Extract null-terminated string name
                    const char *name_ptr = (const char *)&words[offset + 3];
                    size_t max_name = (length - 3) * 4;
                    snprintf(ep->name, sizeof(ep->name), "%.*s", (int)max_name, name_ptr);

                    // Skip words used by string name (including null terminator)
                    size_t name_len = strlen(ep->name);
                    size_t name_words = (name_len + 4) / 4;
                    size_t iface_idx = 3 + name_words;

                    ep->interface_count = 0;
                    while (iface_idx < length && ep->interface_count < 16) {
                        ep->interface_ids[ep->interface_count++] = words[offset + iface_idx];
                        iface_idx++;
                    }
                }
                break;
            }
            case OVIR_SPIRV_OP_VARIABLE: {
                if (length >= 4 && out_module->variable_count < OVIR_SPIRV_MAX_VARIABLES) {
                    ovir_spirv_variable_info_t *var = &out_module->variables[out_module->variable_count++];
                    var->type_id = words[offset + 1];
                    var->id = words[offset + 2];
                    var->storage_class = (ovir_spirv_storage_class_t)words[offset + 3];
                }
                break;
            }
            case OVIR_SPIRV_OP_NAME: {
                if (length >= 3) {
                    uint32_t target_id = words[offset + 1];
                    for (uint32_t i = 0; i < out_module->variable_count; i++) {
                        if (out_module->variables[i].id == target_id) {
                            const char *nptr = (const char *)&words[offset + 2];
                            size_t max_n = (length - 2) * 4;
                            snprintf(out_module->variables[i].name, sizeof(out_module->variables[i].name), "%.*s", (int)max_n, nptr);
                            break;
                        }
                    }
                }
                break;
            }
            case OVIR_SPIRV_OP_DECORATE: {
                if (length >= 3) {
                    uint32_t target_id = words[offset + 1];
                    uint32_t decoration = words[offset + 2];
                    for (uint32_t i = 0; i < out_module->variable_count; i++) {
                        if (out_module->variables[i].id == target_id) {
                            if (decoration == OVIR_SPIRV_DEC_LOCATION && length >= 4) {
                                out_module->variables[i].has_location = true;
                                out_module->variables[i].location = words[offset + 3];
                            } else if (decoration == OVIR_SPIRV_DEC_BINDING && length >= 4) {
                                out_module->variables[i].has_binding = true;
                                out_module->variables[i].binding = words[offset + 3];
                            } else if (decoration == OVIR_SPIRV_DEC_DESCRIPTOR_SET && length >= 4) {
                                out_module->variables[i].has_descriptor_set = true;
                                out_module->variables[i].descriptor_set = words[offset + 3];
                            } else if (decoration == OVIR_SPIRV_DEC_BUILTIN && length >= 4) {
                                out_module->variables[i].has_builtin = true;
                                out_module->variables[i].builtin = (ovir_spirv_builtin_t)words[offset + 3];
                            }
                            break;
                        }
                    }
                }
                break;
            }
            default:
                break;
        }

        offset += length;
    }

    out_module->is_valid = true;
    return OV_SUCCESS;
}
