/**
 * OpenVintage Pre-Boot Simulator - Compatibility & Quirks Framework (Phase 5)
 */

#include "ov_compatibility.h"
#include "ov_hardware.h"
#include "ov_logger.h"
#include <string.h>

static ov_compat_record_t app_catalog[] = {
    {
        .app_name = "Metal Odyssey (Adventure RPG)",
        .target_os = "macOS / iOS",
        .bitness = 64,
        .required_cpu_arch = 1, /* ARM64 */
        .min_cpu_cores = 4,
        .requires_sse42 = false,
        .requires_avx = false,
        .requires_avx2 = false,
        .required_api = OV_API_METAL,
        .api_version_major = 2,
        .api_version_minor = 0,
        .requires_compute = true,
        .requires_tessellation = true,
        .max_texture_size = 8192,
        .min_vram_bytes = 1024ULL * 1024 * 1024,
        .min_system_ram_bytes = 4ULL * 1024 * 1024 * 1024,
        .known_limitations = "Requires MSL 2.0 to GLSL 4.0 translation on Intel Gen7 silicon.",
        .recommended_mode = OV_COMPAT_MODE_TRANSLATED_BOTH
    },
    {
        .app_name = "CyberVulkan 2077 (Sci-Fi Shooter)",
        .target_os = "Linux / Windows",
        .bitness = 64,
        .required_cpu_arch = 2, /* x86_64 */
        .min_cpu_cores = 4,
        .requires_sse42 = true,
        .requires_avx = true,
        .requires_avx2 = true,
        .required_api = OV_API_VULKAN,
        .api_version_major = 1,
        .api_version_minor = 2,
        .requires_compute = true,
        .requires_tessellation = true,
        .max_texture_size = 8192,
        .min_vram_bytes = 2048ULL * 1024 * 1024,
        .min_system_ram_bytes = 8ULL * 1024 * 1024 * 1024,
        .known_limitations = "SPIR-V to GLSL 4.3 shader conversion required on Gen7; AVX2 emulated if host lacks AVX2.",
        .recommended_mode = OV_COMPAT_MODE_TRANSLATED_GPU
    },
    {
        .app_name = "DirectX Classic Flight Simulator",
        .target_os = "Windows XP / 7",
        .bitness = 32,
        .required_cpu_arch = 3, /* x86_32 */
        .min_cpu_cores = 2,
        .requires_sse42 = false,
        .requires_avx = false,
        .requires_avx2 = false,
        .required_api = OV_API_DIRECTX,
        .api_version_major = 9,
        .api_version_minor = 0,
        .requires_compute = false,
        .requires_tessellation = false,
        .max_texture_size = 4096,
        .min_vram_bytes = 512ULL * 1024 * 1024,
        .min_system_ram_bytes = 2ULL * 1024 * 1024 * 1024,
        .known_limitations = "Legacy D3D9 fixed-function pipeline emulated via OpenGL 3.3 programmable shaders.",
        .recommended_mode = OV_COMPAT_MODE_TRANSLATED_GPU
    },
    {
        .app_name = "ARM64 Scientific Matrix Kernel",
        .target_os = "Linux AArch64",
        .bitness = 64,
        .required_cpu_arch = 1, /* ARM64 */
        .min_cpu_cores = 4,
        .requires_sse42 = false,
        .requires_avx = false,
        .requires_avx2 = false,
        .required_api = OV_API_OPENGL,
        .api_version_major = 3,
        .api_version_minor = 3,
        .requires_compute = false,
        .requires_tessellation = false,
        .max_texture_size = 2048,
        .min_vram_bytes = 256ULL * 1024 * 1024,
        .min_system_ram_bytes = 4ULL * 1024 * 1024 * 1024,
        .known_limitations = "ARM64 NEON vector instructions translated to SSE4.2 / AVX on x86_64 host.",
        .recommended_mode = OV_COMPAT_MODE_TRANSLATED_CPU
    },
    {
        .app_name = "AVX2 Heavy Video Filter",
        .target_os = "Linux / Windows",
        .bitness = 64,
        .required_cpu_arch = 2, /* x86_64 */
        .min_cpu_cores = 4,
        .requires_sse42 = true,
        .requires_avx = true,
        .requires_avx2 = true,
        .required_api = OV_API_OPENGL,
        .api_version_major = 3,
        .api_version_minor = 3,
        .requires_compute = false,
        .requires_tessellation = false,
        .max_texture_size = 4096,
        .min_vram_bytes = 512ULL * 1024 * 1024,
        .min_system_ram_bytes = 4ULL * 1024 * 1024 * 1024,
        .known_limitations = "AVX2 SIMD requires decomposition on Intel Ivy Bridge (AVX-only).",
        .recommended_mode = OV_COMPAT_MODE_NATIVE
    },
    {
        .app_name = "Retro 3D Platformer (OpenGL 3.3)",
        .target_os = "Cross-Platform",
        .bitness = 64,
        .required_cpu_arch = 2, /* x86_64 */
        .min_cpu_cores = 2,
        .requires_sse42 = false,
        .requires_avx = false,
        .requires_avx2 = false,
        .required_api = OV_API_OPENGL,
        .api_version_major = 3,
        .api_version_minor = 3,
        .requires_compute = false,
        .requires_tessellation = false,
        .max_texture_size = 4096,
        .min_vram_bytes = 256ULL * 1024 * 1024,
        .min_system_ram_bytes = 2ULL * 1024 * 1024 * 1024,
        .known_limitations = "None. Runs natively on all supported OpenVintage hardware platforms.",
        .recommended_mode = OV_COMPAT_MODE_NATIVE
    }
};

static ov_silicon_quirk_t silicon_quirks[] = {
    {
        .target_silicon = "Intel HD Graphics 4000 (Gen7)",
        .component = "OVIR-GPU Texture Unit",
        .issue_description = "Hardware texture size limit 8192px. 16k textures cause hardware hang.",
        .workaround_applied = "OVIR clamps textures > 8192 down to 8192 with bilinear downsampling.",
        .is_active_for_current_hw = true
    },
    {
        .target_silicon = "Intel HD Graphics 4000 (Gen7)",
        .component = "Compute Engine",
        .issue_description = "Gen7 EU lacks hardware thread group shared memory barriers for high dispatch counts.",
        .workaround_applied = "Compute dispatched across worker CPU thread pool when group count > 1024.",
        .is_active_for_current_hw = true
    },
    {
        .target_silicon = "Intel Core i7-3770 (Ivy Bridge)",
        .component = "OVIR-CPU JIT",
        .issue_description = "Lacks AVX2 256-bit integer SIMD instructions.",
        .workaround_applied = "Split 256-bit vector operations into two 128-bit SSE4.2 instruction pairs.",
        .is_active_for_current_hw = true
    },
    {
        .target_silicon = "Intel HD Graphics 4600 (Gen7.5)",
        .component = "Register Allocator",
        .issue_description = "Register pressure in complex pixel shaders triggers EU spill to shared memory.",
        .workaround_applied = "Enable temporary variable reuse pass in OVIR-GPU optimizer.",
        .is_active_for_current_hw = false
    },
    {
        .target_silicon = "ARM64 Emulation Target",
        .component = "Memory Model",
        .issue_description = "ARM64 weak memory ordering vs x86_64 strong Total Store Order (TSO).",
        .workaround_applied = "Omit redundant MFENCE instructions since x86 TSO provides strict ordering.",
        .is_active_for_current_hw = true
    }
};

ov_status_t ov_compatibility_init(void) {
    ov_log_info("Initializing Phase 5 Compatibility & Quirks Framework...");

    const ov_gpu_info_t *gpu = ov_hardware_get_gpu();
    const ov_cpu_info_t *cpu = ov_hardware_get_cpu();

    /* Update quirk activity based on active hardware */
    for (uint32_t i = 0; i < sizeof(silicon_quirks) / sizeof(silicon_quirks[0]); i++) {
        if (strstr(silicon_quirks[i].target_silicon, "Gen7") && gpu->type == OV_GPU_INTEL_GEN7_HD4000) {
            silicon_quirks[i].is_active_for_current_hw = true;
        } else if (strstr(silicon_quirks[i].target_silicon, "Gen7.5") && gpu->type == OV_GPU_INTEL_GEN75_HD4600) {
            silicon_quirks[i].is_active_for_current_hw = true;
        } else if (strstr(silicon_quirks[i].target_silicon, "Ivy Bridge") && !cpu->has_avx2) {
            silicon_quirks[i].is_active_for_current_hw = true;
        } else {
            silicon_quirks[i].is_active_for_current_hw = false;
        }
    }

    return OV_SUCCESS;
}

void ov_compatibility_cleanup(void) {
    ov_log_info("Compatibility framework cleanup");
}

uint32_t ov_compatibility_get_app_count(void) {
    return sizeof(app_catalog) / sizeof(app_catalog[0]);
}

const ov_compat_record_t* ov_compatibility_get_app(uint32_t index) {
    if (index >= ov_compatibility_get_app_count()) return NULL;
    return &app_catalog[index];
}

ov_status_t ov_compatibility_evaluate(const ov_compat_record_t *record, ov_compat_eval_t *out_eval) {
    if (!record || !out_eval) return OV_ERROR_INVALID_PARAM;
    memset(out_eval, 0, sizeof(ov_compat_eval_t));

    const ov_cpu_info_t *cpu = ov_hardware_get_cpu();
    const ov_gpu_info_t *gpu = ov_hardware_get_gpu();

    out_eval->can_execute = true;
    out_eval->expected_overhead_factor = 100;
    out_eval->selected_mode = OV_COMPAT_MODE_NATIVE;

    /* Check CPU architecture */
    if (record->required_cpu_arch == 1) {
        /* ARM64 required */
        if (cpu->type < OV_CPU_ARM64_M1 || cpu->type > OV_CPU_ARM64_GENERIC) {
            out_eval->cpu_translation_required = true;
            out_eval->selected_mode = OV_COMPAT_MODE_TRANSLATED_CPU;
            out_eval->expected_overhead_factor += 40;
        }
    } else if (record->requires_avx2 && !cpu->has_avx2) {
        out_eval->cpu_translation_required = true;
        out_eval->selected_mode = OV_COMPAT_MODE_TRANSLATED_CPU;
        out_eval->expected_overhead_factor += 30;
    }

    /* Check GPU API */
    if (record->required_api == OV_API_METAL && !gpu->supports_metal) {
        out_eval->gpu_translation_required = true;
        if (out_eval->cpu_translation_required) {
            out_eval->selected_mode = OV_COMPAT_MODE_TRANSLATED_BOTH;
        } else {
            out_eval->selected_mode = OV_COMPAT_MODE_TRANSLATED_GPU;
        }
        out_eval->expected_overhead_factor += 30;
    } else if (record->required_api == OV_API_VULKAN && !gpu->supports_vulkan) {
        out_eval->gpu_translation_required = true;
        out_eval->selected_mode = OV_COMPAT_MODE_TRANSLATED_GPU;
        out_eval->expected_overhead_factor += 35;
    }

    /* Check Texture Clamp */
    if (record->max_texture_size > gpu->max_texture_dimension) {
        out_eval->texture_clamp_applied = true;
        out_eval->selected_mode = OV_COMPAT_MODE_SIMPLIFIED_TEXTURES;
        snprintf(out_eval->clamped_features, sizeof(out_eval->clamped_features),
                 "Textures clamped from %upx to %upx",
                 record->max_texture_size, gpu->max_texture_dimension);
    }

    /* Check Compute Fallback */
    if (record->requires_compute && !gpu->supports_compute) {
        out_eval->software_compute_fallback = true;
        out_eval->selected_mode = OV_COMPAT_MODE_CPU_SOFTWARE_FALLBACK;
        out_eval->expected_overhead_factor += 80;
    }

    snprintf(out_eval->execution_plan, sizeof(out_eval->execution_plan),
             "Plan: %s (Overhead: ~%u%%)",
             ov_compat_mode_to_string(out_eval->selected_mode),
             out_eval->expected_overhead_factor - 100);

    return OV_SUCCESS;
}

uint32_t ov_compatibility_get_quirk_count(void) {
    return sizeof(silicon_quirks) / sizeof(silicon_quirks[0]);
}

const ov_silicon_quirk_t* ov_compatibility_get_quirk(uint32_t index) {
    if (index >= ov_compatibility_get_quirk_count()) return NULL;
    return &silicon_quirks[index];
}

const char* ov_compat_mode_to_string(ov_compat_mode_t mode) {
    switch (mode) {
        case OV_COMPAT_MODE_NATIVE: return "Native Hardware";
        case OV_COMPAT_MODE_TRANSLATED_GPU: return "OVIR-GPU Translated";
        case OV_COMPAT_MODE_TRANSLATED_CPU: return "OVIR-CPU JIT Translated";
        case OV_COMPAT_MODE_TRANSLATED_BOTH: return "OVIR Dual (CPU+GPU) Translated";
        case OV_COMPAT_MODE_SIMPLIFIED_TEXTURES: return "Simplified / Clamped Textures";
        case OV_COMPAT_MODE_CPU_SOFTWARE_FALLBACK: return "CPU Software Rasterizer Fallback";
        case OV_COMPAT_MODE_UNSUPPORTED: return "Unsupported Workload";
        default: return "Unknown Mode";
    }
}

/* Helper to check case-insensitive substring */
static bool compat_strcasestr(const char *haystack, const char *needle) {
    if (!haystack || !needle) return false;
    size_t nlen = strlen(needle);
    if (nlen == 0) return true;
    size_t hlen = strlen(haystack);
    if (hlen < nlen) return false;
    for (size_t i = 0; i <= hlen - nlen; ++i) {
        if (strncasecmp(&haystack[i], needle, nlen) == 0) return true;
    }
    return false;
}

ov_status_t ov_compatibility_evaluate_os(
    const ov_hardware_profile_t *hw,
    const char                  *target_os,
    ov_os_compat_result_t       *out_result
) {
    if (!hw || !target_os || !out_result) return OV_ERROR_INVALID_PARAM;
    memset(out_result, 0, sizeof(ov_os_compat_result_t));

    strncpy(out_result->target_os, target_os, sizeof(out_result->target_os) - 1);

    /* Real vs Simulated Hardware distinction */
    out_result->is_real_hardware = (!hw->is_simulated && hw->source == OV_HW_SOURCE_NATIVE);
    if (out_result->is_real_hardware) {
        strncpy(out_result->hardware_source_label, "REAL HARDWARE", sizeof(out_result->hardware_source_label) - 1);
    } else {
        strncpy(out_result->hardware_source_label, "SIMULATED HARDWARE", sizeof(out_result->hardware_source_label) - 1);
    }

    out_result->confidence_percent = 95;

    /* Check if target is a simulation-only scenario or explicitly requested simulated target */
    if (compat_strcasestr(target_os, "Simulator") || compat_strcasestr(target_os, "Simulated")) {
        out_result->category = OV_COMPAT_CAT_SIMULATED_ONLY;
        strncpy(out_result->recommended_integration, "None", sizeof(out_result->recommended_integration) - 1);
        snprintf(out_result->rationale, sizeof(out_result->rationale),
                 "Workload evaluated purely within OpenVintage simulator environment.");
        return OV_SUCCESS;
    }

    /* Check Linux targets */
    if (compat_strcasestr(target_os, "Linux") || compat_strcasestr(target_os, "Ubuntu") ||
        compat_strcasestr(target_os, "Debian") || compat_strcasestr(target_os, "Fedora") ||
        compat_strcasestr(target_os, "Arch")) {
        out_result->category = OV_COMPAT_CAT_SUPPORTED_WITH_REFIND;
        strncpy(out_result->recommended_integration, "rEFInd", sizeof(out_result->recommended_integration) - 1);
        snprintf(out_result->rationale, sizeof(out_result->rationale),
                 "Modern x86_64 Linux kernel bootable via rEFInd EFI chainloading with GMUX switch policy support.");
        return OV_SUCCESS;
    }

    /* Check Windows targets */
    if (compat_strcasestr(target_os, "Windows")) {
        if (compat_strcasestr(target_os, "11")) {
            out_result->category = OV_COMPAT_CAT_SUPPORTED_WITH_OPENCORE;
            strncpy(out_result->recommended_integration, "OpenCore", sizeof(out_result->recommended_integration) - 1);
            snprintf(out_result->rationale, sizeof(out_result->rationale),
                     "Windows 11 requires OpenCore EFI chainloading for TPM 2.0 and SecureBoot bypass on vintage Mac.");
        } else {
            out_result->category = OV_COMPAT_CAT_SUPPORTED_WITH_CONFIG;
            strncpy(out_result->recommended_integration, "Native", sizeof(out_result->recommended_integration) - 1);
            snprintf(out_result->rationale, sizeof(out_result->rationale),
                     "Windows 10/8.1 bootable natively via Apple EFI Boot Camp partition table configuration.");
        }
        return OV_SUCCESS;
    }

    /* Check macOS Targets */
    bool is_catalina_or_older = (compat_strcasestr(target_os, "Catalina") ||
                                 compat_strcasestr(target_os, "Mojave") ||
                                 compat_strcasestr(target_os, "High Sierra") ||
                                 compat_strcasestr(target_os, "Sierra") ||
                                 compat_strcasestr(target_os, "El Capitan") ||
                                 compat_strcasestr(target_os, "10.15") ||
                                 compat_strcasestr(target_os, "10.14") ||
                                 compat_strcasestr(target_os, "10.13"));

    bool is_monterey_or_bigsur = (compat_strcasestr(target_os, "Monterey") ||
                                  compat_strcasestr(target_os, "Big Sur") ||
                                  compat_strcasestr(target_os, "12.") ||
                                  compat_strcasestr(target_os, "11."));

    bool is_ventura_plus = (compat_strcasestr(target_os, "Ventura") ||
                            compat_strcasestr(target_os, "Sonoma") ||
                            compat_strcasestr(target_os, "Sequoia") ||
                            compat_strcasestr(target_os, "13.") ||
                            compat_strcasestr(target_os, "14.") ||
                            compat_strcasestr(target_os, "15."));

    /* Check MacBookPro9,1 or Ivy Bridge hardware capabilities */
    bool is_ivy_bridge = (hw->cpu.type == OV_CPU_INTEL_IVY_BRIDGE ||
                          compat_strcasestr(hw->model_identifier, "MacBookPro9,1") ||
                          compat_strcasestr(hw->cpu.model_name, "3615QM"));

    if (is_catalina_or_older) {
        if (is_ivy_bridge || hw->gpu.supports_metal) {
            out_result->category = OV_COMPAT_CAT_NATIVELY_SUPPORTED;
            strncpy(out_result->recommended_integration, "Native", sizeof(out_result->recommended_integration) - 1);
            snprintf(out_result->rationale, sizeof(out_result->rationale),
                     "Officially supported in Apple firmware and native OS compatibility window.");
        } else {
            out_result->category = OV_COMPAT_CAT_SUPPORTED_WITH_CONFIG;
            strncpy(out_result->recommended_integration, "OpenCore", sizeof(out_result->recommended_integration) - 1);
            snprintf(out_result->rationale, sizeof(out_result->rationale),
                     "Supported with legacy boot arguments and SIP configuration.");
        }
        return OV_SUCCESS;
    }

    if (is_monterey_or_bigsur) {
        out_result->category = OV_COMPAT_CAT_SUPPORTED_WITH_OCLP;
        out_result->requires_legacy_gpu_patch = true;
        strncpy(out_result->recommended_integration, "OCLP", sizeof(out_result->recommended_integration) - 1);
        snprintf(out_result->rationale, sizeof(out_result->rationale),
                 "Requires OpenCore Legacy Patcher (OCLP) root patches for Kepler / HD 4000 Metal graphics drivers.");
        return OV_SUCCESS;
    }

    if (is_ventura_plus) {
        out_result->category = OV_COMPAT_CAT_SUPPORTED_WITH_OCLP;
        out_result->requires_legacy_gpu_patch = true;
        out_result->requires_avx_emulation = !hw->cpu.has_avx2;
        out_result->cryptex_bypass_needed = true;
        strncpy(out_result->recommended_integration, "OCLP", sizeof(out_result->recommended_integration) - 1);
        snprintf(out_result->rationale, sizeof(out_result->rationale),
                 "Requires OCLP root patches, Cryptex bypass, and Rosetta/AVX2 instruction emulation on Ivy Bridge.");
        return OV_SUCCESS;
    }

    /* Fallback default */
    out_result->category = OV_COMPAT_CAT_EXPERIMENTAL;
    strncpy(out_result->recommended_integration, "OpenCore", sizeof(out_result->recommended_integration) - 1);
    snprintf(out_result->rationale, sizeof(out_result->rationale),
             "Experimental target; evaluation completed based on detected CPU/GPU instruction set capabilities.");
    return OV_SUCCESS;
}

ov_status_t ov_compatibility_evaluate_active_os(
    const char            *target_os,
    ov_os_compat_result_t *out_result
) {
    const ov_hardware_profile_t *hw = ov_hardware_get_active_profile();
    return ov_compatibility_evaluate_os(hw, target_os, out_result);
}

