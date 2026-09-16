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
