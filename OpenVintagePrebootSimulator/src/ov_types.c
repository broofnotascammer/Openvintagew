/**
 * OpenVintage Pre-Boot Simulator - Type & Enum String Converters
 */

#include "ov_types.h"

const char* ov_status_to_string(ov_status_t status) {
    switch (status) {
        case OV_SUCCESS:                   return "OV_SUCCESS";
        case OV_ERROR_INIT:                 return "OV_ERROR_INIT";
        case OV_ERROR_HARDWARE:             return "OV_ERROR_HARDWARE";
        case OV_ERROR_MEMORY:               return "OV_ERROR_MEMORY";
        case OV_ERROR_CONFIG:               return "OV_ERROR_CONFIG";
        case OV_ERROR_INVALID_PARAM:        return "OV_ERROR_INVALID_PARAM";
        case OV_ERROR_NOT_FOUND:            return "OV_ERROR_NOT_FOUND";
        case OV_ERROR_OUT_OF_RESOURCES:     return "OV_ERROR_OUT_OF_RESOURCES";
        case OV_ERROR_UNSUPPORTED:          return "OV_ERROR_UNSUPPORTED";
        case OV_ERROR_INTEGRITY:            return "OV_ERROR_INTEGRITY";
        case OV_ERROR_PERMISSION_DENIED:    return "OV_ERROR_PERMISSION_DENIED";
        case OV_ERROR_GENERIC:              return "OV_ERROR_GENERIC";
        default:                            return "OV_ERROR_UNKNOWN";
    }
}

const char* ov_hw_mode_to_string(ov_hw_mode_t mode) {
    switch (mode) {
        case OV_HW_MODE_NATIVE:    return "NATIVE";
        case OV_HW_MODE_SIMULATED: return "SIMULATED";
        default:                   return "UNKNOWN";
    }
}

const char* ov_hw_source_to_string(ov_hw_source_t source) {
    switch (source) {
        case OV_HW_SOURCE_NATIVE:    return "NATIVE";
        case OV_HW_SOURCE_SIMULATED: return "SIMULATED";
        case OV_HW_SOURCE_INFERRED:  return "INFERRED";
        case OV_HW_SOURCE_UNAVAILABLE: return "UNAVAILABLE";
        case OV_HW_SOURCE_FALLBACK:  return "FALLBACK";
        case OV_HW_SOURCE_UNKNOWN:
        default:                     return "UNKNOWN";
    }
}

const char* ov_cpu_type_to_string(ov_cpu_type_t type) {
    switch (type) {
        case OV_CPU_INTEL_CORE_DUO:    return "Intel Core Duo (Yonah 32-bit)";
        case OV_CPU_INTEL_CORE2_DUO:   return "Intel Core 2 Duo (Merom/Penryn 64-bit)";
        case OV_CPU_INTEL_XEON_CORE:   return "Intel Xeon (Woodcrest/Harpertown)";
        case OV_CPU_INTEL_NEHALEM:     return "Intel Nehalem / Westmere";
        case OV_CPU_INTEL_SANDY_BRIDGE:return "Intel Sandy Bridge (2nd Gen)";
        case OV_CPU_INTEL_IVY_BRIDGE:  return "Intel Ivy Bridge (3rd Gen)";
        case OV_CPU_INTEL_HASWELL:     return "Intel Haswell (4th Gen)";
        case OV_CPU_INTEL_BROADWELL:   return "Intel Broadwell (5th Gen)";
        case OV_CPU_INTEL_SKYLAKE:     return "Intel Skylake (6th Gen)";
        case OV_CPU_INTEL_KABY_LAKE:   return "Intel Kaby Lake (7th Gen)";
        case OV_CPU_INTEL_COFFEE_LAKE: return "Intel Coffee Lake (8th/9th Gen)";
        case OV_CPU_INTEL_COMET_LAKE:  return "Intel Comet Lake (10th Gen)";
        case OV_CPU_INTEL_CASCADE_LAKE:return "Intel Cascade Lake Xeon W";
        case OV_CPU_AMD_ZEN:           return "AMD Zen Architecture";
        case OV_CPU_ARM64_M1:          return "Apple Silicon M1";
        case OV_CPU_ARM64_M2:          return "Apple Silicon M2";
        case OV_CPU_ARM64_M3:          return "Apple Silicon M3";
        case OV_CPU_ARM64_GENERIC:     return "ARM64 Generic";
        case OV_CPU_UNKNOWN:
        default:                       return "Unknown CPU Architecture";
    }
}

const char* ov_gpu_arch_to_string(ov_gpu_arch_t arch) {
    switch (arch) {
        case OV_GPU_ARCH_INTEL_GEN3:         return "Intel GMA Gen3 (950/X3100)";
        case OV_GPU_ARCH_INTEL_GEN6:         return "Intel Gen6 (HD 3000)";
        case OV_GPU_ARCH_INTEL_GEN7:         return "Intel Gen7 (HD 4000)";
        case OV_GPU_ARCH_INTEL_GEN75:        return "Intel Gen7.5 (HD 4600 / Iris 5100 / Iris Pro 5200)";
        case OV_GPU_ARCH_INTEL_GEN8:         return "Intel Gen8 (HD 6000 / Iris 6100)";
        case OV_GPU_ARCH_INTEL_GEN9:         return "Intel Gen9 (HD 515/530, Iris 540/550)";
        case OV_GPU_ARCH_INTEL_GEN95:        return "Intel Gen9.5 (UHD 630)";
        case OV_GPU_ARCH_INTEL_GEN11:        return "Intel Gen11 (Iris Plus Ice Lake)";
        case OV_GPU_ARCH_INTEL_GEN12_IRIS_XE:return "Intel Gen12 (Iris Xe)";
        case OV_GPU_ARCH_NVIDIA_TESLA:       return "NVIDIA Tesla (GeForce 8/9/GT 120)";
        case OV_GPU_ARCH_NVIDIA_FERMI:       return "NVIDIA Fermi (GeForce 300/400/500)";
        case OV_GPU_ARCH_NVIDIA_KEPLER:      return "NVIDIA Kepler (GK107 GT 650M / GT 750M)";
        case OV_GPU_ARCH_NVIDIA_MAXWELL:     return "NVIDIA Maxwell (GeForce 900)";
        case OV_GPU_ARCH_NVIDIA_PASCAL:      return "NVIDIA Pascal (GeForce 1000)";
        case OV_GPU_ARCH_AMD_TERASCALE_1:    return "AMD TeraScale 1 (Radeon X1600 / HD 2000-4000)";
        case OV_GPU_ARCH_AMD_TERASCALE_2:    return "AMD TeraScale 2 (Radeon HD 5000/6000)";
        case OV_GPU_ARCH_AMD_GCN_1_4:        return "AMD GCN 1-4 (FirePro D300/D500/D700 / Polaris)";
        case OV_GPU_ARCH_AMD_GCN_5_VEGA:     return "AMD GCN 5 (Vega 56/64 / Vega II)";
        case OV_GPU_ARCH_AMD_RDNA_1_3:       return "AMD RDNA 1-3 (Navi / W6800X)";
        case OV_GPU_ARCH_APPLE_SILICON_M1:   return "Apple Silicon M1 GPU";
        case OV_GPU_ARCH_APPLE_SILICON_M2:   return "Apple Silicon M2 GPU";
        case OV_GPU_ARCH_APPLE_SILICON_M3:   return "Apple Silicon M3 GPU (Hardware RT / Dynamic Caching)";
        case OV_GPU_ARCH_SOFTWARE_FALLBACK:  return "Software CPU Rasterizer (llvmpipe)";
        case OV_GPU_ARCH_UNKNOWN:
        default:                             return "Unknown GPU Architecture";
    }
}

const char* ov_metal_support_to_string(ov_metal_support_t level) {
    switch (level) {
        case OV_METAL_NONE: return "None (Pre-Metal)";
        case OV_METAL_1:    return "Metal 1 (macOS 10.11 - 10.14)";
        case OV_METAL_2:    return "Metal 2 (macOS 10.13 - 12)";
        case OV_METAL_3:    return "Metal 3 (macOS 13+ Modern)";
        default:            return "Unknown Metal Support";
    }
}

const char* ov_macos_compat_rating_to_string(ov_macos_compat_rating_t rating) {
    switch (rating) {
        case OV_MACOS_SUPPORTED_NATIVE:       return "NATIVELY SUPPORTED";
        case OV_MACOS_SUPPORTED_WITH_PATCHES: return "SUPPORTED WITH PATCHES (OCLP)";
        case OV_MACOS_SUPPORTED_SIMULATED:    return "SUPPORTED VIA EMULATION/TRANSLATION";
        case OV_MACOS_UNSUPPORTED:            return "UNSUPPORTED";
        default:                              return "UNKNOWN RATING";
    }
}

const char* ov_compat_category_to_string(ov_compat_category_t cat) {
    switch (cat) {
        case OV_COMPAT_CAT_NATIVELY_SUPPORTED:     return "Natively Supported";
        case OV_COMPAT_CAT_SUPPORTED_WITH_CONFIG:  return "Supported with Configuration";
        case OV_COMPAT_CAT_SUPPORTED_WITH_OCLP:    return "Supported with OCLP";
        case OV_COMPAT_CAT_SUPPORTED_WITH_OPENCORE:return "Supported with OpenCore";
        case OV_COMPAT_CAT_SUPPORTED_WITH_REFIND:  return "Supported with rEFInd";
        case OV_COMPAT_CAT_EXPERIMENTAL:           return "Experimental";
        case OV_COMPAT_CAT_UNSUPPORTED:            return "Unsupported";
        case OV_COMPAT_CAT_SIMULATED_ONLY:         return "Simulated Only";
        default:                                   return "Unknown Category";
    }
}

const char* ov_perf_profile_to_string(ov_perf_profile_id_t profile) {
    switch (profile) {
        case OV_PERF_PROFILE_MAX_PERFORMANCE:    return "Maximum Performance";
        case OV_PERF_PROFILE_GAMING:             return "Gaming";
        case OV_PERF_PROFILE_BALANCED:           return "Balanced";
        case OV_PERF_PROFILE_BATTERY_EFFICIENCY: return "Battery / Efficiency";
        case OV_PERF_PROFILE_COMPATIBILITY:      return "Compatibility";
        case OV_PERF_PROFILE_CUSTOM:             return "Custom";
        default:                                 return "Unknown Profile";
    }
}

const char* ov_deploy_step_to_string(ov_deploy_step_t step) {
    switch (step) {
        case OV_DEPLOY_STEP_DISCOVER:      return "DISCOVER";
        case OV_DEPLOY_STEP_SIMULATE:      return "SIMULATE";
        case OV_DEPLOY_STEP_PLAN:          return "PLAN";
        case OV_DEPLOY_STEP_SHOW_CHANGES:  return "SHOW CHANGES";
        case OV_DEPLOY_STEP_USER_APPROVAL: return "USER APPROVAL";
        case OV_DEPLOY_STEP_BACKUP:        return "BACKUP";
        case OV_DEPLOY_STEP_APPLY:         return "APPLY";
        case OV_DEPLOY_STEP_VERIFY:        return "VERIFY";
        case OV_DEPLOY_STEP_RECOVERY:      return "RECOVERY IF NEEDED";
        case OV_DEPLOY_STEP_COMPLETE:      return "COMPLETE";
        default:                           return "UNKNOWN STEP";
    }
}

const char* ov_deploy_state_to_string(ov_deploy_state_t state) {
    switch (state) {
        case OV_DEPLOY_STATE_IDLE:              return "Idle";
        case OV_DEPLOY_STATE_PLAN_READY:        return "Plan Ready";
        case OV_DEPLOY_STATE_AWAITING_APPROVAL: return "Awaiting User Approval";
        case OV_DEPLOY_STATE_BACKED_UP:         return "Backup Created";
        case OV_DEPLOY_STATE_APPLIED:           return "Changes Applied";
        case OV_DEPLOY_STATE_VERIFIED:          return "Verified";
        case OV_DEPLOY_STATE_ROLLED_BACK:       return "Rolled Back";
        case OV_DEPLOY_STATE_FAILED:            return "Failed";
        default:                                return "Unknown State";
    }
}

const char* ov_boot_target_type_to_string(ov_boot_target_type_t target) {
    switch (target) {
        case OV_BOOT_TARGET_MACOS:          return "macOS";
        case OV_BOOT_TARGET_MACOS_RECOVERY: return "macOS Recovery";
        case OV_BOOT_TARGET_OPENCORE:       return "OpenCore";
        case OV_BOOT_TARGET_LINUX:          return "Linux";
        case OV_BOOT_TARGET_WINDOWS:        return "Windows";
        case OV_BOOT_TARGET_EFI_APP:        return "EFI Application";
        case OV_BOOT_TARGET_UNKNOWN:
        default:                            return "Unknown Target";
    }
}

const char* ov_integration_type_to_string(ov_integration_type_t integ) {
    switch (integ) {
        case OV_INTEGRATION_OPENVINTAGE: return "OpenVintage";
        case OV_INTEGRATION_OCLP:        return "OpenCore Legacy Patcher (OCLP)";
        case OV_INTEGRATION_REFIND:      return "rEFInd Boot Manager";
        default:                         return "Unknown Integration";
    }
}

