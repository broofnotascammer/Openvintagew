#ifndef OVIR_GPU_OPT_H
#define OVIR_GPU_OPT_H

#include "ovir_gpu_ir.h"

typedef struct {
    uint32_t constant_folds_applied;
    uint32_t dead_instructions_eliminated;
    uint32_t unreachable_blocks_removed;
    uint32_t redundant_loads_removed;
} ovir_gpu_opt_stats_t;

// Optimization Passes
ov_status_t ovir_gpu_optimize(ovir_gpu_shader_ir_t *ir, ovir_gpu_opt_stats_t *out_stats);
ov_status_t ovir_gpu_opt_constant_fold(ovir_gpu_shader_ir_t *ir, uint32_t *out_folded_count);
ov_status_t ovir_gpu_opt_dead_code_elimination(ovir_gpu_shader_ir_t *ir, uint32_t *out_dce_count);

#endif /* OVIR_GPU_OPT_H */
