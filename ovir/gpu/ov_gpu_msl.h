/**
 * OpenVintage - OVIR-GPU MSL / SPIR-V Shader Frontends (Phase 6)
 */

#ifndef OV_GPU_MSL_H
#define OV_GPU_MSL_H

#include "ov_types.h"
#include <stdint.h>
#include <stddef.h>

ov_status_t ov_gpu_msl_to_glsl(const char *msl_source, char *out_glsl, size_t max_len);
ov_status_t ov_gpu_spirv_to_glsl(const uint32_t *spirv_words, size_t word_count, char *out_glsl, size_t max_len);

#endif /* OV_GPU_MSL_H */
