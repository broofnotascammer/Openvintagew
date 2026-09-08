/**
 * OpenVintage Pre-Boot Simulator - Core Types & Enumerations
 * Bridges host simulator environment with OpenVintage Phase 1-5 Architecture.
 */

#ifndef OV_TYPES_H
#define OV_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/* OpenVintage Standard Status Codes */
typedef enum {
    OV_SUCCESS = 0,
    OV_ERROR_INIT = 1,
    OV_ERROR_HARDWARE = 2,
    OV_ERROR_MEMORY = 3,
    OV_ERROR_CONFIG = 4,
    OV_ERROR_INVALID_PARAM = 5,
    OV_ERROR_NOT_FOUND = 6,
    OV_ERROR_OUT_OF_RESOURCES = 7,
    OV_ERROR_UNSUPPORTED = 8,
    OV_ERROR_INTEGRITY = 9
} ov_status_t;

/* CPU Architectures */
typedef enum {
    OV_CPU_INTEL_IVY_BRIDGE = 0,   /* Gen7 host (e.g. Core i7-3770, SSE4.2, AVX) */
    OV_CPU_INTEL_HASWELL = 1,      /* Gen7.5 host (e.g. Core i7-4770, AVX2, FMA) */
    OV_CPU_INTEL_MODERN = 2,       /* Modern Intel (Skylake/Tiger Lake/etc.) */
    OV_CPU_AMD_ZEN = 3,            /* AMD Zen architecture */
    OV_CPU_ARM64 = 4,              /* ARM64 (Apple Silicon / Cortex-A7x) */
    OV_CPU_UNKNOWN = 5
} ov_cpu_type_t;

/* GPU Hardware Families */
typedef enum {
    OV_GPU_INTEL_GEN7_HD4000 = 0,  /* Intel Gen7 HD 4000 (Ivy Bridge, 16 EUs) */
    OV_GPU_INTEL_GEN75_HD4600 = 1, /* Intel Gen7.5 HD 4600 / Iris 5100/5200 (Haswell, 20-40 EUs) */
    OV_GPU_INTEL_IRIS_XE = 2,      /* Modern Intel Iris Xe (96 EUs) */
    OV_GPU_NVIDIA_GEFORCE = 3,     /* Nvidia Kepler/Maxwell/Ampere */
    OV_GPU_AMD_RADEON = 4,         /* AMD GCN / RDNA */
    OV_GPU_APPLE_SILICON = 5,      /* Apple M-series GPU */
    OV_GPU_SOFTWARE_RASTERIZER = 6,/* CPU SoftPipe / llvmpipe fallback */
    OV_GPU_NONE = 7
} ov_gpu_type_t;

/* Graphics APIs */
typedef enum {
    OV_API_OPENGL = 1,
    OV_API_VULKAN = 2,
    OV_API_METAL = 3,
    OV_API_DIRECTX = 4
} ov_api_type_t;

/* Resolver Decisions (Phase 2 & Phase 5) */
typedef enum {
    OV_RESOLUTION_NATIVE = 1,       /* Target silicon natively satisfies workload */
    OV_RESOLUTION_TRANSLATED = 2,   /* Workload transformed through OVIR layer */
    OV_RESOLUTION_SIMPLIFIED = 3,   /* Clamped texture/shader for VRAM compatibility */
    OV_RESOLUTION_FALLBACK = 4,     /* Workload executed on CPU software emulation */
    OV_RESOLUTION_UNSUPPORTED = 5   /* Workload cannot be executed */
} ov_resolution_decision_t;

/* Execution Modes (Phase 2 legacy mapping) */
typedef enum {
    OV_EXEC_NATIVE = 0,
    OV_EXEC_TRANSLATED = 1,
    OV_EXEC_FALLBACK = 2
} ov_exec_mode_t;

/* Resource Performance Profiles (Phase 5) */
typedef enum {
    OV_PROFILE_BALANCED = 1,
    OV_PROFILE_PERFORMANCE = 2,
    OV_PROFILE_MAX_PERFORMANCE = 3,
    OV_PROFILE_BATTERY_LOW_POWER = 4
} ov_resource_profile_t;

/* Cache Tiers (Phase 5) */
typedef enum {
    OV_CACHE_TIER_CPU_TRANSLATION = 1,
    OV_CACHE_TIER_SHADER = 2,
    OV_CACHE_TIER_PIPELINE = 3,
    OV_CACHE_TIER_COMPATIBILITY = 4,
    OV_CACHE_TIER_ALL = 0xFF
} ov_cache_tier_t;

/* Cache Invalidation Reasons */
typedef enum {
    OV_INVALIDATE_MANUAL = 1,
    OV_INVALIDATE_VERSION_CHANGE = 2,
    OV_INVALIDATE_HARDWARE_CHANGE = 3,
    OV_INVALIDATE_MEMORY_PRESSURE = 4,
    OV_INVALIDATE_INTEGRITY_FAILURE = 5
} ov_invalidate_reason_t;

/* Compatibility Modes */
typedef enum {
    OV_COMPAT_MODE_NATIVE = 1,
    OV_COMPAT_MODE_TRANSLATED_GPU = 2,
    OV_COMPAT_MODE_TRANSLATED_CPU = 3,
    OV_COMPAT_MODE_TRANSLATED_BOTH = 4,
    OV_COMPAT_MODE_SIMPLIFIED_TEXTURES = 5,
    OV_COMPAT_MODE_CPU_SOFTWARE_FALLBACK = 6,
    OV_COMPAT_MODE_UNSUPPORTED = 7
} ov_compat_mode_t;

/* Workload Class */
typedef enum {
    OV_WORKLOAD_CLASS_GPU_RENDERING = 1,
    OV_WORKLOAD_CLASS_GPU_COMPUTE = 2,
    OV_WORKLOAD_CLASS_CPU_SIMD = 3,
    OV_WORKLOAD_CLASS_MEMORY_OPS = 4
} ov_workload_class_t;

/* Workload descriptor (Phase 2 legacy) */
typedef enum {
    OV_WORKLOAD_COMPUTE = 0,
    OV_WORKLOAD_GRAPHICS = 1,
    OV_WORKLOAD_MEMORY = 2,
    OV_WORKLOAD_IO = 3,
    OV_WORKLOAD_MIXED = 4
} ov_workload_type_t;

typedef struct {
    ov_workload_type_t type;
    uint32_t priority;
    uint32_t timeout_ms;
    bool requires_gpu;
    bool requires_sync;
} ov_workload_t;

/* Resolution decision (Phase 2 legacy) */
typedef struct {
    ov_exec_mode_t mode;
    uint32_t estimated_cost;
    char reason[256];
} ov_resolution_t;

#endif /* OV_TYPES_H */
