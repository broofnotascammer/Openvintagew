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
