#ifndef OV_TYPES_H
#define OV_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define OV_VERSION_MAJOR 11
#define OV_VERSION_MINOR 0
#define OV_VERSION_PATCH 0
#define OV_VERSION_STRING "11.0.0-RC1"
#define OV_TRANSLATOR_VERSION_ID 0x000B0000

typedef enum {
    OV_SUCCESS = 0,
    OV_ERR_INVALID_PARAM = -1,
    OV_ERR_UNSUPPORTED = -2,
    OV_ERR_OUT_OF_MEMORY = -3,
    OV_ERR_CACHE_MISS = -4,
    OV_ERR_CACHE_INVALID = -5,
    OV_ERR_RESOURCE_EXHAUSTED = -6,
    OV_ERR_TRANSLATION_FAILED = -7,
    OV_ERR_VERIFICATION_FAILED = -8,
    OV_ERR_FAULT = -9,
    OV_ERR_SECURITY_VIOLATION = -10
} ov_status_t;

// 128-Bit SIMD / Vector Register Structure (ARM NEON / x86 SSE)
#ifndef OV_VEC128_DEFINED
#define OV_VEC128_DEFINED
typedef union {
    uint8_t  u8[16];
    uint16_t u16[8];
    uint32_t u32[4];
    uint64_t u64[2];
    int8_t   s8[16];
    int16_t  s16[8];
    int32_t  s32[4];
    int64_t  s64[2];
    float    f32[4];
    double   f64[2];
} ovir_vec128_t;
#endif

// Memory permission flags
#define OV_MEM_NONE     0
#define OV_MEM_READ     (1 << 0)
#define OV_MEM_WRITE    (1 << 1)
#define OV_MEM_EXECUTE  (1 << 2)

// Runtime exception types
typedef enum {
    OV_EXC_NONE = 0,
    OV_EXC_INVALID_MEMORY,
    OV_EXC_EXECUTE_VIOLATION,
    OV_EXC_INVALID_INSTRUCTION,
    OV_EXC_TRANSLATION_FAILURE,
    OV_EXC_INVALID_TRANSLATION_BLOCK,
    OV_EXC_GUEST_FAULT,
    OV_EXC_RESOURCE_LIMIT,
    OV_EXC_INTERRUPTED
} ov_exception_code_t;

typedef enum {
    OV_ARCH_UNKNOWN = 0,
    OV_ARCH_ARM64,
    OV_ARCH_X86_64
} ov_arch_t;

typedef enum {
    OV_GFX_API_NONE = 0,
    OV_GFX_API_METAL,
    OV_GFX_API_VULKAN,
    OV_GFX_API_OPENGL,
    OV_GFX_API_D3D11,
    OV_GFX_API_D3D12
} ov_gfx_api_t;

typedef enum {
    OV_GFX_BACKEND_NONE = 0,
    OV_GFX_BACKEND_METAL_NATIVE,
    OV_GFX_BACKEND_VULKAN_MVK,
    OV_GFX_BACKEND_OPENGL_CORE,
    OV_GFX_BACKEND_OVIR_TRANSLATED
} ov_gfx_backend_t;

// CPU feature flags
#define OV_CPU_FEAT_AVX      (1 << 0)
#define OV_CPU_FEAT_AVX2     (1 << 1)
#define OV_CPU_FEAT_AVX512   (1 << 2)
#define OV_CPU_FEAT_NEON     (1 << 3)
#define OV_CPU_FEAT_ATOMICS  (1 << 4)
#define OV_CPU_FEAT_FMA      (1 << 5)

// GPU feature flags
#define OV_GPU_FEAT_COMPUTE          (1 << 0)
#define OV_GPU_FEAT_TESSELLATION     (1 << 1)
#define OV_GPU_FEAT_GEOMETRY_SHADER  (1 << 2)
#define OV_GPU_FEAT_RAY_TRACING      (1 << 3)
#define OV_GPU_FEAT_MESH_SHADERS     (1 << 4)
#define OV_GPU_FEAT_BC_COMPRESSION   (1 << 5)

typedef enum {
    OV_PRIORITY_LOW = 0,
    OV_PRIORITY_NORMAL,
    OV_PRIORITY_HIGH,
    OV_PRIORITY_REALTIME
} ov_priority_t;

typedef enum {
    OV_WORKLOAD_COMPUTE = 0,
    OV_WORKLOAD_GRAPHICS,
    OV_WORKLOAD_MIXED,
    OV_WORKLOAD_INTERACTIVE
} ov_workload_type_t;

typedef enum {
    OV_DECISION_NATIVE = 0,
    OV_DECISION_CPU_TRANSLATION,
    OV_DECISION_GPU_TRANSLATION,
    OV_DECISION_CPU_AND_GPU_TRANSLATION,
    OV_DECISION_GPU_RECONSTRUCTION,
    OV_DECISION_CPU_FALLBACK,
    OV_DECISION_UNSUPPORTED
} ov_execution_decision_t;

// Phase 11 Standard Decision Aliases
#define OV_EXEC_NATIVE                      OV_DECISION_NATIVE
#define OV_EXEC_CPU_TRANSLATION             OV_DECISION_CPU_TRANSLATION
#define OV_EXEC_GPU_TRANSLATION             OV_DECISION_GPU_TRANSLATION
#define OV_EXEC_CPU_AND_GPU_TRANSLATION     OV_DECISION_CPU_AND_GPU_TRANSLATION
#define OV_EXEC_GPU_RECONSTRUCTION          OV_DECISION_GPU_RECONSTRUCTION
#define OV_EXEC_CPU_FALLBACK                OV_DECISION_CPU_FALLBACK
#define OV_EXEC_UNSUPPORTED                 OV_DECISION_UNSUPPORTED

typedef enum {
    OV_PROFILE_ENERGY_SAVER = 0,
    OV_PROFILE_BALANCED,
    OV_PROFILE_PERFORMANCE,
    OV_PROFILE_MAX_THROUGHPUT
} ov_scheduler_profile_t;

typedef enum {
    OV_REASON_NONE = 0,
    OV_REASON_NATIVE_SUPPORTED,
    OV_REASON_CPU_ARCH_MISMATCH_TRANSLATABLE,
    OV_REASON_GPU_API_MISMATCH_TRANSLATABLE,
    OV_REASON_CPU_AND_GPU_MISMATCH_TRANSLATABLE,
    OV_REASON_GPU_PARTIAL_SUPPORT_RECONSTRUCTION,
    OV_REASON_GPU_API_UNAVAILABLE,
    OV_REASON_CPU_FALLBACK_AVAILABLE,
    OV_REASON_CPU_FALLBACK_REQUIRED,
    OV_REASON_NO_COMPATIBLE_BACKEND,
    OV_REASON_RESOURCE_LIMIT,
    OV_REASON_UNSUPPORTED_OPERATION,
    OV_REASON_UNSUPPORTED_CPU_FEATURE,
    OV_REASON_UNSUPPORTED_GPU_FEATURE,
    OV_REASON_INSUFFICIENT_RAM,
    OV_REASON_INSUFFICIENT_VRAM,
    OV_REASON_INSUFFICIENT_CORES,
    OV_REASON_UNSUPPORTED_TRANSLATION_PAIR,
    OV_REASON_VALIDATION_METADATA_MISMATCH
} ov_reason_code_t;

const char *ov_arch_name(ov_arch_t arch);
const char *ov_gfx_api_name(ov_gfx_api_t api);
const char *ov_gfx_backend_name(ov_gfx_backend_t backend);
const char *ov_decision_name(ov_execution_decision_t decision);
const char *ov_reason_name(ov_reason_code_t reason);
const char *ov_profile_name(ov_scheduler_profile_t profile);

#endif /* OV_TYPES_H */
