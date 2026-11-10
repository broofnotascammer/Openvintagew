#ifndef OVIR_SPIRV_H
#define OVIR_SPIRV_H

#include "../../core/types/ov_types.h"

// SPIR-V 1.0 - 1.6 Magic number
#define OVIR_SPIRV_MAGIC 0x07230203

// Common SPIR-V Execution Models (OpEntryPoint)
typedef enum {
    OVIR_SPIRV_EXEC_VERTEX = 0,
    OVIR_SPIRV_EXEC_TESSELLATION_CONTROL = 1,
    OVIR_SPIRV_EXEC_TESSELLATION_EVALUATION = 2,
    OVIR_SPIRV_EXEC_GEOMETRY = 3,
    OVIR_SPIRV_EXEC_FRAGMENT = 4,
    OVIR_SPIRV_EXEC_GLCOMPUTE = 5,
    OVIR_SPIRV_EXEC_KERNEL = 6
} ovir_spirv_exec_model_t;

// Storage Classes (OpVariable)
typedef enum {
    OVIR_SPIRV_STORAGE_UNIFORM_CONSTANT = 0,
    OVIR_SPIRV_STORAGE_INPUT = 1,
    OVIR_SPIRV_STORAGE_UNIFORM = 2,
    OVIR_SPIRV_STORAGE_OUTPUT = 3,
    OVIR_SPIRV_STORAGE_WORKGROUP = 4,
    OVIR_SPIRV_STORAGE_CROSS_WORKGROUP = 5,
    OVIR_SPIRV_STORAGE_PRIVATE = 6,
    OVIR_SPIRV_STORAGE_FUNCTION = 7,
    OVIR_SPIRV_STORAGE_GENERIC = 8,
    OVIR_SPIRV_STORAGE_PUSH_CONSTANT = 9,
    OVIR_SPIRV_STORAGE_IMAGE = 11,
    OVIR_SPIRV_STORAGE_STORAGE_BUFFER = 12
} ovir_spirv_storage_class_t;

// SPIR-V Core Opcodes supported in Phase 14
typedef enum {
    OVIR_SPIRV_OP_NOP = 0,
    OVIR_SPIRV_OP_SOURCE = 3,
    OVIR_SPIRV_OP_NAME = 5,
    OVIR_SPIRV_OP_MEMBER_NAME = 6,
    OVIR_SPIRV_OP_STRING = 7,
    OVIR_SPIRV_OP_EXTENSION = 10,
    OVIR_SPIRV_OP_EXT_INST_IMPORT = 11,
    OVIR_SPIRV_OP_MEMORY_MODEL = 14,
    OVIR_SPIRV_OP_ENTRY_POINT = 15,
    OVIR_SPIRV_OP_EXECUTION_MODE = 16,
    OVIR_SPIRV_OP_CAPABILITY = 17,
    OVIR_SPIRV_OP_TYPE_VOID = 19,
    OVIR_SPIRV_OP_TYPE_BOOL = 20,
    OVIR_SPIRV_OP_TYPE_INT = 21,
    OVIR_SPIRV_OP_TYPE_FLOAT = 22,
    OVIR_SPIRV_OP_TYPE_VECTOR = 23,
    OVIR_SPIRV_OP_TYPE_MATRIX = 24,
    OVIR_SPIRV_OP_TYPE_IMAGE = 25,
    OVIR_SPIRV_OP_TYPE_SAMPLER = 26,
    OVIR_SPIRV_OP_TYPE_SAMPLED_IMAGE = 27,
    OVIR_SPIRV_OP_TYPE_ARRAY = 28,
    OVIR_SPIRV_OP_TYPE_STRUCT = 30,
    OVIR_SPIRV_OP_TYPE_POINTER = 32,
    OVIR_SPIRV_OP_TYPE_FUNCTION = 33,
    OVIR_SPIRV_OP_CONSTANT_TRUE = 41,
    OVIR_SPIRV_OP_CONSTANT_FALSE = 42,
    OVIR_SPIRV_OP_CONSTANT = 43,
    OVIR_SPIRV_OP_CONSTANT_COMPOSITE = 44,
    OVIR_SPIRV_OP_FUNCTION = 54,
    OVIR_SPIRV_OP_FUNCTION_PARAMETER = 55,
    OVIR_SPIRV_OP_FUNCTION_END = 56,
    OVIR_SPIRV_OP_VARIABLE = 59,
    OVIR_SPIRV_OP_LOAD = 61,
    OVIR_SPIRV_OP_STORE = 62,
    OVIR_SPIRV_OP_ACCESS_CHAIN = 65,
    OVIR_SPIRV_OP_DECORATE = 71,
    OVIR_SPIRV_OP_MEMBER_DECORATE = 72,
    OVIR_SPIRV_OP_VECTOR_SHUFFLE = 79,
    OVIR_SPIRV_OP_COMPOSITE_CONSTRUCT = 80,
    OVIR_SPIRV_OP_COMPOSITE_EXTRACT = 81,
    OVIR_SPIRV_OP_COMPOSITE_INSERT = 82,
    OVIR_SPIRV_OP_LABEL = 248,
    OVIR_SPIRV_OP_BRANCH = 249,
    OVIR_SPIRV_OP_BRANCH_CONDITIONAL = 250,
    OVIR_SPIRV_OP_RETURN = 253,
    OVIR_SPIRV_OP_RETURN_VALUE = 254,
    // Arithmetic & Bitwise
    OVIR_SPIRV_OP_SNODIV = 126,
    OVIR_SPIRV_OP_IADD = 128,
    OVIR_SPIRV_OP_FADD = 129,
    OVIR_SPIRV_OP_ISUB = 130,
    OVIR_SPIRV_OP_FSUB = 131,
    OVIR_SPIRV_OP_IMUL = 132,
    OVIR_SPIRV_OP_FMUL = 133,
    OVIR_SPIRV_OP_UDIV = 134,
    OVIR_SPIRV_OP_SDIV = 135,
    OVIR_SPIRV_OP_FDIV = 136,
    OVIR_SPIRV_OP_FNEGATE = 127,
    OVIR_SPIRV_OP_DOT = 148,
    OVIR_SPIRV_OP_VECTOR_TIMES_SCALAR = 142,
    OVIR_SPIRV_OP_MATRIX_TIMES_VECTOR = 145,
    // Comparisons
    OVIR_SPIRV_OP_LOGICAL_AND = 167,
    OVIR_SPIRV_OP_LOGICAL_OR = 168,
    OVIR_SPIRV_OP_LOGICAL_NOT = 169,
    OVIR_SPIRV_OP_I_EQUAL = 170,
    OVIR_SPIRV_OP_I_NOT_EQUAL = 171,
    OVIR_SPIRV_OP_F_ORD_EQUAL = 180,
    OVIR_SPIRV_OP_F_ORD_NOT_EQUAL = 181,
    OVIR_SPIRV_OP_F_ORD_LESS_THAN = 182,
    OVIR_SPIRV_OP_F_ORD_GREATER_THAN = 183,
    OVIR_SPIRV_OP_F_ORD_LESS_THAN_EQUAL = 184,
    OVIR_SPIRV_OP_F_ORD_GREATER_THAN_EQUAL = 185
} ovir_spirv_opcode_t;

// SPIR-V Decorations
typedef enum {
    OVIR_SPIRV_DEC_RELAXED_PRECISION = 0,
    OVIR_SPIRV_DEC_BLOCK = 2,
    OVIR_SPIRV_DEC_BUFFER_BLOCK = 3,
    OVIR_SPIRV_DEC_ARRAY_STRIDE = 6,
    OVIR_SPIRV_DEC_MATRIX_STRIDE = 7,
    OVIR_SPIRV_DEC_BUILTIN = 11,
    OVIR_SPIRV_DEC_LOCATION = 30,
    OVIR_SPIRV_DEC_BINDING = 33,
    OVIR_SPIRV_DEC_DESCRIPTOR_SET = 34,
    OVIR_SPIRV_DEC_OFFSET = 35
} ovir_spirv_decoration_t;

// BuiltIns
typedef enum {
    OVIR_SPIRV_BUILTIN_POSITION = 0,
    OVIR_SPIRV_BUILTIN_POINT_SIZE = 1,
    OVIR_SPIRV_BUILTIN_VERTEX_INDEX = 42,
    OVIR_SPIRV_BUILTIN_INSTANCE_INDEX = 43,
    OVIR_SPIRV_BUILTIN_FRAG_COORD = 15,
    OVIR_SPIRV_BUILTIN_FRAG_DEPTH = 22,
    OVIR_SPIRV_BUILTIN_GLOBAL_INVOCATION_ID = 28
} ovir_spirv_builtin_t;

#define OVIR_SPIRV_MAX_ENTRY_POINTS 8
#define OVIR_SPIRV_MAX_CAPABILITIES 16
#define OVIR_SPIRV_MAX_VARIABLES 64
#define OVIR_SPIRV_MAX_BINDINGS 32

typedef struct {
    uint32_t id;
    ovir_spirv_exec_model_t exec_model;
    char name[64];
    uint32_t interface_ids[16];
    uint32_t interface_count;
} ovir_spirv_entry_point_t;

typedef struct {
    uint32_t id;
    ovir_spirv_storage_class_t storage_class;
    uint32_t type_id;
    bool has_location;
    uint32_t location;
    bool has_binding;
    uint32_t binding;
    bool has_descriptor_set;
    uint32_t descriptor_set;
    bool has_builtin;
    ovir_spirv_builtin_t builtin;
    char name[64];
} ovir_spirv_variable_info_t;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t generator_id;
    uint32_t id_bound;
    uint32_t schema;
    uint32_t word_count;

    uint32_t capabilities[OVIR_SPIRV_MAX_CAPABILITIES];
    uint32_t capability_count;

    ovir_spirv_entry_point_t entry_points[OVIR_SPIRV_MAX_ENTRY_POINTS];
    uint32_t entry_point_count;

    ovir_spirv_variable_info_t variables[OVIR_SPIRV_MAX_VARIABLES];
    uint32_t variable_count;

    bool is_valid;
    char validation_error[128];
} ovir_spirv_module_t;

// Parser and Validator APIs
ov_status_t ovir_spirv_validate_header(const uint32_t *words, size_t word_count, ovir_spirv_module_t *out_module);
ov_status_t ovir_spirv_parse(const uint32_t *words, size_t word_count, ovir_spirv_module_t *out_module);
const char *ovir_spirv_exec_model_name(ovir_spirv_exec_model_t model);
const char *ovir_spirv_storage_class_name(ovir_spirv_storage_class_t sc);
const char *ovir_spirv_opcode_name(ovir_spirv_opcode_t op);

#endif /* OVIR_SPIRV_H */
