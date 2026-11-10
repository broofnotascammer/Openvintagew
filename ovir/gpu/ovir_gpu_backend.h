#ifndef OVIR_GPU_BACKEND_H
#define OVIR_GPU_BACKEND_H

#include "ovir_gpu_ir.h"

#define OVIR_GLSL_MAX_SOURCE_LEN 4096

typedef struct {
    char source_code[OVIR_GLSL_MAX_SOURCE_LEN];
    size_t source_length;
    ov_gfx_backend_t target_backend;
    uint32_t glsl_version;
    bool compilation_passed;
    char compiler_log[256];
} ovir_gpu_compiled_shader_t;

// Backend Translation & Compiler APIs
ov_status_t ovir_gpu_translate_to_glsl(const ovir_gpu_shader_ir_t *ir, ovir_gpu_compiled_shader_t *out_compiled);
ov_status_t ovir_gpu_backend_compile(ovir_gpu_compiled_shader_t *shader, bool simulate_driver);
ov_status_t ovir_gpu_reconstruct_operation(ovir_gpu_shader_ir_t *ir, ovir_gpu_opcode_t unsupported_op, uint32_t *out_reconstructed);

#endif /* OVIR_GPU_BACKEND_H */
