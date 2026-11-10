#ifndef OVIR_CPU_BACKEND_H
#define OVIR_CPU_BACKEND_H

#include "ovir_cpu.h"
#include "../../core/types/ov_types.h"

typedef struct ovir_backend_s {
    const char *name;
    ov_arch_t guest_arch;
    ov_arch_t host_arch;
    uint32_t capabilities_mask;
    ov_status_t (*emit)(const ovir_ir_block_t *ir, ovir_compiled_block_t *out);
    ov_status_t (*verify)(const ovir_compiled_block_t *compiled);
} ovir_backend_t;

// Backend capabilities
#define OVIR_BACKEND_CAP_ARITHMETIC (1U << 0)
#define OVIR_BACKEND_CAP_LOGICAL    (1U << 1)
#define OVIR_BACKEND_CAP_MEMORY     (1U << 2)
#define OVIR_BACKEND_CAP_BRANCHES   (1U << 3)
#define OVIR_BACKEND_CAP_CALLS      (1U << 4)
#define OVIR_BACKEND_CAP_BARRIERS   (1U << 5)
#define OVIR_BACKEND_CAP_VECTOR_SSE2 (1U << 6)
#define OVIR_BACKEND_CAP_VECTOR_SSE4 (1U << 7)
#define OVIR_BACKEND_CAP_VECTOR_AVX  (1U << 8)

// Host CPU feature queries
bool ov_host_has_sse2(void);
bool ov_host_has_sse41(void);
bool ov_host_has_avx(void);

// Get registered backend
const ovir_backend_t *ovir_cpu_get_backend(ov_arch_t guest, ov_arch_t host);

// Direct x86-64 code emitter supporting extended instruction set & NEON
ov_status_t ovir_cpu_emit_x86_64_extended(const ovir_ir_block_t *ir, ovir_compiled_block_t *out_compiled);

// Real W^X execution for scalar & vector workloads
ov_status_t ovir_cpu_execute_native_vector(
    const ovir_compiled_block_t *compiled,
    ovir_cpu_state_t *state
);

#endif /* OVIR_CPU_BACKEND_H */
