#ifndef OVIR_GPU_IR_H
#define OVIR_GPU_IR_H

#include "../../core/types/ov_types.h"
#include "ovir_spirv.h"

#define OVIR_GPU_IR_MAX_INSN 128
#define OVIR_GPU_IR_MAX_TYPES 64
#define OVIR_GPU_IR_MAX_CONSTANTS 32
#define OVIR_GPU_IR_MAX_VARS 32
#define OVIR_GPU_IR_MAX_BLOCKS 16

// GPU IR Basic Primitive Types
typedef enum {
    OVIR_GPU_TYPE_VOID = 0,
    OVIR_GPU_TYPE_BOOL,
    OVIR_GPU_TYPE_INT32,
    OVIR_GPU_TYPE_UINT32,
    OVIR_GPU_TYPE_FLOAT32,
    OVIR_GPU_TYPE_VEC2,
    OVIR_GPU_TYPE_VEC3,
    OVIR_GPU_TYPE_VEC4,
    OVIR_GPU_TYPE_MAT4,
    OVIR_GPU_TYPE_SAMPLER2D
} ovir_gpu_type_kind_t;

typedef struct {
    uint32_t type_id;
    ovir_gpu_type_kind_t kind;
    uint32_t vector_size; // 1 for scalar, 2..4 for vector
    uint32_t bit_width;
} ovir_gpu_type_t;

// GPU IR Constant
typedef struct {
    uint32_t const_id;
    uint32_t type_id;
    union {
        int32_t i32;
        uint32_t u32;
        float f32;
        float vec4[4];
    } value;
    bool is_composite;
    uint32_t constituents[4];
} ovir_gpu_constant_t;

// GPU IR Variable / Interface Symbol
typedef struct {
    uint32_t var_id;
    uint32_t type_id;
    ovir_spirv_storage_class_t storage_class;
    char name[64];
    bool has_location;
    uint32_t location;
    bool has_binding;
    uint32_t binding;
    bool has_descriptor_set;
    uint32_t descriptor_set;
    bool has_builtin;
    ovir_spirv_builtin_t builtin;
} ovir_gpu_var_t;

// GPU IR Opcodes
typedef enum {
    OVIR_GPU_OP_NOP = 0,
    // Data movement & Memory
    OVIR_GPU_OP_LOAD,
    OVIR_GPU_OP_STORE,
    OVIR_GPU_OP_ASSIGN,
    // Vector & Composite
    OVIR_GPU_OP_VEC_CONSTRUCT,
    OVIR_GPU_OP_VEC_EXTRACT,
    OVIR_GPU_OP_VEC_SWIZZLE,
    // Arithmetic
    OVIR_GPU_OP_ADD,
    OVIR_GPU_OP_SUB,
    OVIR_GPU_OP_MUL,
    OVIR_GPU_OP_DIV,
    OVIR_GPU_OP_NEG,
    OVIR_GPU_OP_DOT,
    OVIR_GPU_OP_MAT_MUL_VEC,
    // Logical & Compare
    OVIR_GPU_OP_LOGICAL_AND,
    OVIR_GPU_OP_LOGICAL_OR,
    OVIR_GPU_OP_LOGICAL_NOT,
    OVIR_GPU_OP_CMP_EQ,
    OVIR_GPU_OP_CMP_NEQ,
    OVIR_GPU_OP_CMP_LT,
    OVIR_GPU_OP_CMP_GT,
    OVIR_GPU_OP_CMP_LTE,
    OVIR_GPU_OP_CMP_GTE,
    // Control Flow
    OVIR_GPU_OP_LABEL,
    OVIR_GPU_OP_BRANCH,
    OVIR_GPU_OP_BRANCH_COND,
    OVIR_GPU_OP_RETURN,
    // Specialized Texture
    OVIR_GPU_OP_TEX_SAMPLE
} ovir_gpu_opcode_t;

// Typed Instruction
typedef struct {
    ovir_gpu_opcode_t opcode;
    uint32_t result_id;
    uint32_t type_id;
    uint32_t src1_id;
    uint32_t src2_id;
    uint32_t src3_id;
    uint32_t swizzle_mask; // e.g. 0x0123 for xyzw
    bool has_side_effects;
} ovir_gpu_insn_t;

// GPU IR Shader Module
typedef struct {
    ovir_spirv_exec_model_t stage;
    char entry_point[64];
    
    ovir_gpu_type_t types[OVIR_GPU_IR_MAX_TYPES];
    uint32_t type_count;

    ovir_gpu_constant_t constants[OVIR_GPU_IR_MAX_CONSTANTS];
    uint32_t constant_count;

    ovir_gpu_var_t variables[OVIR_GPU_IR_MAX_VARS];
    uint32_t variable_count;

    ovir_gpu_insn_t instructions[OVIR_GPU_IR_MAX_INSN];
    uint32_t instruction_count;

    bool is_valid;
    char validation_error[128];
} ovir_gpu_shader_ir_t;

// IR APIs
ov_status_t ovir_gpu_ir_init(ovir_gpu_shader_ir_t *ir, ovir_spirv_exec_model_t stage, const char *entry_point);
ov_status_t ovir_gpu_ir_add_type(ovir_gpu_shader_ir_t *ir, uint32_t type_id, ovir_gpu_type_kind_t kind, uint32_t vec_size, uint32_t bit_width);
ov_status_t ovir_gpu_ir_add_constant_f32(ovir_gpu_shader_ir_t *ir, uint32_t const_id, uint32_t type_id, float val);
ov_status_t ovir_gpu_ir_add_constant_vec4(ovir_gpu_shader_ir_t *ir, uint32_t const_id, uint32_t type_id, float x, float y, float z, float w);
ov_status_t ovir_gpu_ir_add_variable(ovir_gpu_shader_ir_t *ir, uint32_t var_id, uint32_t type_id, ovir_spirv_storage_class_t sc, const char *name);
ov_status_t ovir_gpu_ir_add_instruction(ovir_gpu_shader_ir_t *ir, ovir_gpu_opcode_t op, uint32_t res_id, uint32_t type_id, uint32_t s1, uint32_t s2);
ov_status_t ovir_gpu_ir_validate(const ovir_gpu_shader_ir_t *ir, char *out_err, size_t err_size);
ov_status_t ovir_gpu_ir_dump(const ovir_gpu_shader_ir_t *ir, char *out_buf, size_t buf_size);

// Conversion from parsed SPIR-V to neutral GPU IR
ov_status_t ovir_gpu_ir_from_spirv(const uint32_t *words, size_t word_count, ovir_gpu_shader_ir_t *out_ir);

const char *ovir_gpu_opcode_name(ovir_gpu_opcode_t op);
const char *ovir_gpu_type_kind_name(ovir_gpu_type_kind_t kind);

#endif /* OVIR_GPU_IR_H */
