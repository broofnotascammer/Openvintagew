/**
 * OpenVintage Pre-Boot Simulator - Core Types & Enumerations
 * Bridges host simulator environment with OpenVintage Architecture.
 */

#ifndef OV_TYPES_H
#define OV_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/* OpenVintage Phase 7 Architecture Versioning */
#define OPENVINTAGE_VERSION_MAJOR 7
#define OPENVINTAGE_VERSION_MINOR 0
#define OPENVINTAGE_VERSION_PATCH 0
#define OPENVINTAGE_VERSION_STRING "7.0.0"
#define OPENVINTAGE_BUILD_ID "OV7-RELEASE-PROD"

/* Release Channels */
typedef enum {
    OV_CHANNEL_DEVELOPMENT = 0,
    OV_CHANNEL_CANARY      = 1,
    OV_CHANNEL_BETA        = 2,
    OV_CHANNEL_STABLE      = 3
} ov_release_channel_t;

const char* ov_release_channel_to_string(ov_release_channel_t channel);

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
    OV_ERROR_INTEGRITY = 9,
    OV_ERROR_PERMISSION_DENIED = 10,
    OV_ERROR_GENERIC = 11
} ov_status_t;

const char* ov_status_to_string(ov_status_t status);

/* Hardware Detection / Simulation Operating Mode */
typedef enum {
    OV_HW_MODE_NATIVE = 0,     /* Physical Mac / host platform native hardware detection */
    OV_HW_MODE_SIMULATED = 1   /* Simulated hardware profile target */
} ov_hw_mode_t;

const char* ov_hw_mode_to_string(ov_hw_mode_t mode);

/* Hardware Detection / Simulation Origin */
typedef enum {
    OV_HW_SOURCE_UNKNOWN = 0,
    OV_HW_SOURCE_NATIVE = 1,            /* Real physical Mac / host hardware detected */
    OV_HW_SOURCE_HOST_DETECTED = 1,     /* Backward-compatible alias */
    OV_HW_SOURCE_SIMULATED = 2,         /* Simulated hardware profile */
    OV_HW_SOURCE_SIMULATED_PROFILE = 2, /* Backward-compatible alias */
    OV_HW_SOURCE_INFERRED = 3,          /* Heuristic/inferred value */
    OV_HW_SOURCE_UNAVAILABLE = 4,       /* Feature/hardware probe unavailable */
    OV_HW_SOURCE_FALLBACK = 5,          /* Safe software/virtual fallback */
    OV_HW_SOURCE_INFERRED_FALLBACK = 5  /* Backward compatibility alias */
} ov_hw_source_t;

const char* ov_hw_source_to_string(ov_hw_source_t source);

/* CPU Family Architectures */
typedef enum {
    OV_CPU_INTEL_CORE_DUO = 0,
    OV_CPU_INTEL_CORE2_DUO = 1,
    OV_CPU_INTEL_XEON_CORE = 2,
    OV_CPU_INTEL_NEHALEM = 3,
    OV_CPU_INTEL_SANDY_BRIDGE = 4,
    OV_CPU_INTEL_IVY_BRIDGE = 5,
    OV_CPU_INTEL_HASWELL = 6,
    OV_CPU_INTEL_BROADWELL = 7,
    OV_CPU_INTEL_SKYLAKE = 8,
    OV_CPU_INTEL_KABY_LAKE = 9,
    OV_CPU_INTEL_COFFEE_LAKE = 10,
    OV_CPU_INTEL_COMET_LAKE = 11,
    OV_CPU_INTEL_CASCADE_LAKE = 12,
    OV_CPU_AMD_ZEN = 13,
    OV_CPU_ARM64_M1 = 14,
    OV_CPU_ARM64_M2 = 15,
    OV_CPU_ARM64_M3 = 16,
    OV_CPU_ARM64_GENERIC = 17,
    OV_CPU_UNKNOWN = 18
} ov_cpu_type_t;

const char* ov_cpu_type_to_string(ov_cpu_type_t type);

/* GPU Hardware Architecture Generations */
typedef enum {
    OV_GPU_ARCH_UNKNOWN = 0,
    OV_GPU_ARCH_INTEL_GEN3 = 1,
    OV_GPU_ARCH_INTEL_GEN6 = 2,
    OV_GPU_ARCH_INTEL_GEN7 = 3,
    OV_GPU_ARCH_INTEL_GEN75 = 4,
    OV_GPU_ARCH_INTEL_GEN8 = 5,
    OV_GPU_ARCH_INTEL_GEN9 = 6,
    OV_GPU_ARCH_INTEL_GEN95 = 7,
    OV_GPU_ARCH_INTEL_GEN11 = 8,
    OV_GPU_ARCH_INTEL_GEN12_IRIS_XE = 9,
    OV_GPU_ARCH_NVIDIA_TESLA = 10,
    OV_GPU_ARCH_NVIDIA_FERMI = 11,
    OV_GPU_ARCH_NVIDIA_KEPLER = 12,
    OV_GPU_ARCH_NVIDIA_MAXWELL = 13,
    OV_GPU_ARCH_NVIDIA_PASCAL = 14,
    OV_GPU_ARCH_AMD_TERASCALE_1 = 15,
    OV_GPU_ARCH_AMD_TERASCALE_2 = 16,
    OV_GPU_ARCH_AMD_GCN_1_4 = 17,
    OV_GPU_ARCH_AMD_GCN_5_VEGA = 18,
    OV_GPU_ARCH_AMD_RDNA_1_3 = 19,
    OV_GPU_ARCH_APPLE_SILICON_M1 = 20,
    OV_GPU_ARCH_APPLE_SILICON_M2 = 21,
    OV_GPU_ARCH_APPLE_SILICON_M3 = 22,
    OV_GPU_ARCH_SOFTWARE_FALLBACK = 23
} ov_gpu_arch_t;

const char* ov_gpu_arch_to_string(ov_gpu_arch_t arch);

/* GPU Family Legacy Types */
typedef enum {
    OV_GPU_INTEL_GEN7_HD4000 = 0,
    OV_GPU_INTEL_GEN75_HD4600 = 1,
    OV_GPU_INTEL_IRIS_XE = 2,
    OV_GPU_NVIDIA_GEFORCE = 3,
    OV_GPU_AMD_RADEON = 4,
    OV_GPU_APPLE_SILICON = 5,
    OV_GPU_SOFTWARE_RASTERIZER = 6,
    OV_GPU_NONE = 7,
    OV_GPU_UNKNOWN = 8
} ov_gpu_type_t;

/* Metal Support Level */
typedef enum {
    OV_METAL_NONE = 0,
    OV_METAL_1 = 1,
    OV_METAL_2 = 2,
    OV_METAL_3 = 3
} ov_metal_support_t;

const char* ov_metal_support_to_string(ov_metal_support_t level);

typedef enum {
    OV_API_OPENGL = 1,
    OV_API_VULKAN = 2,
    OV_API_METAL = 3,
    OV_API_DIRECTX = 4
} ov_api_type_t;

typedef enum {
    OV_RESOLUTION_NATIVE = 1,
    OV_RESOLUTION_JIT_TRANSLATED = 2,
    OV_RESOLUTION_RECOMPILED = 3,
    OV_RESOLUTION_SIMPLIFIED = 4,
    OV_RESOLUTION_FALLBACK = 5,
    OV_RESOLUTION_UNSUPPORTED = 6
} ov_resolution_decision_t;

typedef enum {
    OV_EXEC_NATIVE = 0,
    OV_EXEC_TRANSLATED = 1,
    OV_EXEC_FALLBACK = 2
} ov_exec_mode_t;

typedef enum {
    OV_PROFILE_BALANCED = 1,
    OV_PROFILE_PERFORMANCE = 2,
    OV_PROFILE_MAX_PERFORMANCE = 3,
    OV_PROFILE_BATTERY_LOW_POWER = 4
} ov_resource_profile_t;

typedef enum {
    OV_RESOURCE_PRESSURE_NORMAL = 0,
    OV_RESOURCE_PRESSURE_MODERATE = 1,
    OV_RESOURCE_PRESSURE_CRITICAL = 2
} ov_resource_pressure_t;

typedef enum {
    OV_CACHE_TIER_CPU_TRANSLATION = 1,
    OV_CACHE_TIER_SHADER = 2,
    OV_CACHE_TIER_PIPELINE = 3,
    OV_CACHE_TIER_COMPATIBILITY = 4,
    OV_CACHE_TIER_ALL = 0xFF
} ov_cache_tier_t;

typedef enum {
    OV_INVALIDATE_MANUAL = 1,
    OV_INVALIDATE_VERSION_CHANGE = 2,
    OV_INVALIDATE_HARDWARE_CHANGE = 3,
    OV_INVALIDATE_MEMORY_PRESSURE = 4,
    OV_INVALIDATE_INTEGRITY_FAILURE = 5
} ov_invalidate_reason_t;

typedef enum {
    OV_MACOS_SUPPORTED_NATIVE = 1,
    OV_MACOS_SUPPORTED_WITH_PATCHES = 2,
    OV_MACOS_SUPPORTED_SIMULATED = 3,
    OV_MACOS_UNSUPPORTED = 4
} ov_macos_compat_rating_t;

const char* ov_macos_compat_rating_to_string(ov_macos_compat_rating_t rating);

typedef enum {
    OV_COMPAT_MODE_NATIVE = 1,
    OV_COMPAT_MODE_TRANSLATED_GPU = 2,
    OV_COMPAT_MODE_TRANSLATED_CPU = 3,
    OV_COMPAT_MODE_TRANSLATED_BOTH = 4,
    OV_COMPAT_MODE_SIMPLIFIED_TEXTURES = 5,
    OV_COMPAT_MODE_CPU_SOFTWARE_FALLBACK = 6,
    OV_COMPAT_MODE_UNSUPPORTED = 7
} ov_compat_mode_t;

typedef enum {
    OV_WORKLOAD_CLASS_GPU_RENDERING = 1,
    OV_WORKLOAD_CLASS_GPU_COMPUTE = 2,
    OV_WORKLOAD_CLASS_CPU_SIMD = 3,
    OV_WORKLOAD_CLASS_MEMORY_OPS = 4
} ov_workload_class_t;

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

typedef struct {
    ov_exec_mode_t mode;
    uint32_t estimated_cost;
    char reason[256];
} ov_resolution_t;

typedef enum {
    OV_COMPAT_CAT_NATIVELY_SUPPORTED = 0,
    OV_COMPAT_CAT_SUPPORTED_WITH_CONFIG = 1,
    OV_COMPAT_CAT_SUPPORTED_WITH_OCLP = 2,
    OV_COMPAT_CAT_SUPPORTED_WITH_OPENCORE = 3,
    OV_COMPAT_CAT_SUPPORTED_WITH_REFIND = 4,
    OV_COMPAT_CAT_EXPERIMENTAL = 5,
    OV_COMPAT_CAT_UNSUPPORTED = 6,
    OV_COMPAT_CAT_SIMULATED_ONLY = 7,
    OV_COMPAT_CAT_COUNT
} ov_compat_category_t;

const char* ov_compat_category_to_string(ov_compat_category_t cat);

typedef enum {
    OV_PERF_PROFILE_MAX_PERFORMANCE = 0,
    OV_PERF_PROFILE_GAMING = 1,
    OV_PERF_PROFILE_BALANCED = 2,
    OV_PERF_PROFILE_BATTERY_EFFICIENCY = 3,
    OV_PERF_PROFILE_COMPATIBILITY = 4,
    OV_PERF_PROFILE_CUSTOM = 5,
    OV_PERF_PROFILE_COUNT
} ov_perf_profile_id_t;

const char* ov_perf_profile_to_string(ov_perf_profile_id_t profile);

typedef enum {
    OV_DEPLOY_STEP_DISCOVER = 0,
    OV_DEPLOY_STEP_SIMULATE = 1,
    OV_DEPLOY_STEP_PLAN = 2,
    OV_DEPLOY_STEP_SHOW_CHANGES = 3,
    OV_DEPLOY_STEP_USER_APPROVAL = 4,
    OV_DEPLOY_STEP_BACKUP = 5,
    OV_DEPLOY_STEP_APPLY = 6,
    OV_DEPLOY_STEP_VERIFY = 7,
    OV_DEPLOY_STEP_RECOVERY = 8,
    OV_DEPLOY_STEP_COMPLETE = 9
} ov_deploy_step_t;

const char* ov_deploy_step_to_string(ov_deploy_step_t step);

typedef enum {
    OV_DEPLOY_STATE_IDLE = 0,
    OV_DEPLOY_STATE_PLAN_READY = 1,
    OV_DEPLOY_STATE_AWAITING_APPROVAL = 2,
    OV_DEPLOY_STATE_BACKED_UP = 3,
    OV_DEPLOY_STATE_APPLIED = 4,
    OV_DEPLOY_STATE_VERIFIED = 5,
    OV_DEPLOY_STATE_ROLLED_BACK = 6,
    OV_DEPLOY_STATE_FAILED = 7
} ov_deploy_state_t;

const char* ov_deploy_state_to_string(ov_deploy_state_t state);

typedef enum {
    OV_BOOT_TARGET_MACOS = 0,
    OV_BOOT_TARGET_MACOS_RECOVERY = 1,
    OV_BOOT_TARGET_OPENCORE = 2,
    OV_BOOT_TARGET_LINUX = 3,
    OV_BOOT_TARGET_WINDOWS = 4,
    OV_BOOT_TARGET_EFI_APP = 5,
    OV_BOOT_TARGET_UNKNOWN = 6
} ov_boot_target_type_t;

const char* ov_boot_target_type_to_string(ov_boot_target_type_t target);

typedef enum {
    OV_INTEGRATION_OPENVINTAGE = 0,
    OV_INTEGRATION_OCLP = 1,
    OV_INTEGRATION_REFIND = 2
} ov_integration_type_t;

const char* ov_integration_type_to_string(ov_integration_type_t integ);

#endif /* OV_TYPES_H */
