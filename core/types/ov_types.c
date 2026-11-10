#include "ov_types.h"

const char *ov_arch_name(ov_arch_t arch) {
    switch (arch) {
        case OV_ARCH_ARM64: return "ARM64";
        case OV_ARCH_X86_64: return "x86-64";
        default: return "Unknown Arch";
    }
}

const char *ov_gfx_api_name(ov_gfx_api_t api) {
    switch (api) {
        case OV_GFX_API_NONE: return "None";
        case OV_GFX_API_METAL: return "Metal";
        case OV_GFX_API_VULKAN: return "Vulkan";
        case OV_GFX_API_OPENGL: return "OpenGL";
        case OV_GFX_API_D3D11: return "Direct3D 11";
        case OV_GFX_API_D3D12: return "Direct3D 12";
        default: return "Unknown API";
    }
}

const char *ov_gfx_backend_name(ov_gfx_backend_t backend) {
    switch (backend) {
        case OV_GFX_BACKEND_NONE: return "None";
        case OV_GFX_BACKEND_METAL_NATIVE: return "Metal Native";
        case OV_GFX_BACKEND_VULKAN_MVK: return "MoltenVK / Vulkan";
        case OV_GFX_BACKEND_OPENGL_CORE: return "OpenGL Core Profile";
        case OV_GFX_BACKEND_OVIR_TRANSLATED: return "OVIR-GPU Translated";
        default: return "Unknown Backend";
    }
}

const char *ov_decision_name(ov_execution_decision_t decision) {
    switch (decision) {
        case OV_DECISION_NATIVE: return "NATIVE";
        case OV_DECISION_CPU_TRANSLATION: return "CPU_TRANSLATION";
        case OV_DECISION_GPU_TRANSLATION: return "GPU_TRANSLATION";
        case OV_DECISION_CPU_AND_GPU_TRANSLATION: return "CPU_AND_GPU_TRANSLATION";
        case OV_DECISION_GPU_RECONSTRUCTION: return "GPU_RECONSTRUCTION";
        case OV_DECISION_CPU_FALLBACK: return "CPU_FALLBACK";
        case OV_DECISION_UNSUPPORTED: return "UNSUPPORTED";
        default: return "UNKNOWN";
    }
}

const char *ov_reason_name(ov_reason_code_t reason) {
    switch (reason) {
        case OV_REASON_NONE: return "None";
        case OV_REASON_NATIVE_SUPPORTED: return "Workload matches host architecture natively";
        case OV_REASON_CPU_ARCH_MISMATCH_TRANSLATABLE: return "CPU architecture mismatch; translatable via OVIR-CPU";
        case OV_REASON_GPU_API_MISMATCH_TRANSLATABLE: return "Graphics API mismatch; translatable via OVIR-GPU";
        case OV_REASON_CPU_AND_GPU_MISMATCH_TRANSLATABLE: return "Both CPU and GPU translations required; translatable";
        case OV_REASON_GPU_PARTIAL_SUPPORT_RECONSTRUCTION: return "Graphics operation reconstructed from partial GPU feature support";
        case OV_REASON_GPU_API_UNAVAILABLE: return "Requested graphics API unavailable on host GPU";
        case OV_REASON_CPU_FALLBACK_AVAILABLE: return "GPU cannot represent operation; CPU fallback execution path available";
        case OV_REASON_CPU_FALLBACK_REQUIRED: return "Graphics pipeline executing via software CPU fallback reconstruction";
        case OV_REASON_NO_COMPATIBLE_BACKEND: return "No compatible GPU or CPU translation backend available";
        case OV_REASON_RESOURCE_LIMIT: return "Workload exceeds available host system resource limits";
        case OV_REASON_UNSUPPORTED_OPERATION: return "Operation unsupported on both GPU and CPU execution paths";
        case OV_REASON_UNSUPPORTED_CPU_FEATURE: return "Required CPU feature missing on host and cannot be emulated";
        case OV_REASON_UNSUPPORTED_GPU_FEATURE: return "Required GPU feature missing on host backend";
        case OV_REASON_INSUFFICIENT_RAM: return "Host system has insufficient RAM for workload";
        case OV_REASON_INSUFFICIENT_VRAM: return "Host GPU has insufficient VRAM for workload";
        case OV_REASON_INSUFFICIENT_CORES: return "Host has fewer CPU cores than strictly required";
        case OV_REASON_UNSUPPORTED_TRANSLATION_PAIR: return "Translation between requested architecture pair is unsupported";
        case OV_REASON_VALIDATION_METADATA_MISMATCH: return "Cache validation metadata mismatch; invalidation triggered";
        default: return "Unknown Reason";
    }
}


const char *ov_profile_name(ov_scheduler_profile_t profile) {
    switch (profile) {
        case OV_PROFILE_ENERGY_SAVER: return "Energy Saver";
        case OV_PROFILE_BALANCED: return "Balanced";
        case OV_PROFILE_PERFORMANCE: return "Performance";
        case OV_PROFILE_MAX_THROUGHPUT: return "Max Throughput";
        default: return "Unknown Profile";
    }
}
