/**
 * OpenVintage Pre-Boot Simulator - macOS Compatibility Engine Implementation
 * Evaluates architectural constraints, kernel requirements, Metal levels, and OCLP patchability.
 */

#include "ov_macos_compat.h"
#include "ov_hardware.h"
#include "ov_logger.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    ov_macos_version_t version;
    const char *marketing_name;
    const char *code_name;
    uint32_t darwin_major;
    bool requires_64bit_cpu;
    bool requires_64bit_efi;
    bool requires_sse41;
    bool requires_sse42;
    bool requires_avx2;
    ov_metal_support_t min_metal;
    uint64_t min_ram_bytes;
} macos_version_info_t;

static const macos_version_info_t g_macos_versions[] = {
    { OV_MACOS_10_4_TIGER,       "Mac OS X 10.4", "Tiger",         8,  false, false, false, false, false, OV_METAL_NONE, 512ULL * 1024 * 1024 },
    { OV_MACOS_10_5_LEOPARD,     "Mac OS X 10.5", "Leopard",       9,  false, false, false, false, false, OV_METAL_NONE, 512ULL * 1024 * 1024 },
    { OV_MACOS_10_6_SNOW_LEOPARD,"Mac OS X 10.6", "Snow Leopard", 10,  false, false, false, false, false, OV_METAL_NONE, 1ULL * 1024 * 1024 * 1024 },
    { OV_MACOS_10_7_LION,        "Mac OS X 10.7", "Lion",         11,  true,  false, false, false, false, OV_METAL_NONE, 2ULL * 1024 * 1024 * 1024 },
    { OV_MACOS_10_8_MOUNTAIN_LION,"OS X 10.8",    "Mountain Lion", 12,  true,  true,  false, false, false, OV_METAL_NONE, 2ULL * 1024 * 1024 * 1024 },
    { OV_MACOS_10_9_MAVERICKS,   "OS X 10.9",     "Mavericks",     13,  true,  true,  false, false, false, OV_METAL_NONE, 2ULL * 1024 * 1024 * 1024 },
    { OV_MACOS_10_10_YOSEMITE,   "OS X 10.10",    "Yosemite",      14,  true,  true,  false, false, false, OV_METAL_NONE, 2ULL * 1024 * 1024 * 1024 },
    { OV_MACOS_10_11_EL_CAPITAN, "OS X 10.11",    "El Capitan",    15,  true,  true,  false, false, false, OV_METAL_NONE, 2ULL * 1024 * 1024 * 1024 },
    { OV_MACOS_10_12_SIERRA,     "macOS 10.12",   "Sierra",        16,  true,  true,  true,  false, false, OV_METAL_NONE, 2ULL * 1024 * 1024 * 1024 },
    { OV_MACOS_10_13_HIGH_SIERRA,"macOS 10.13",   "High Sierra",   17,  true,  true,  true,  false, false, OV_METAL_NONE, 2ULL * 1024 * 1024 * 1024 },
    { OV_MACOS_10_14_MOJAVE,     "macOS 10.14",   "Mojave",        18,  true,  true,  true,  false, false, OV_METAL_1,    4ULL * 1024 * 1024 * 1024 },
    { OV_MACOS_10_15_CATALINA,   "macOS 10.15",   "Catalina",      19,  true,  true,  true,  false, false, OV_METAL_1,    4ULL * 1024 * 1024 * 1024 },
    { OV_MACOS_11_BIG_SUR,       "macOS 11",      "Big Sur",       20,  true,  true,  true,  true,  false, OV_METAL_1,    4ULL * 1024 * 1024 * 1024 },
    { OV_MACOS_12_MONTEREY,      "macOS 12",      "Monterey",      21,  true,  true,  true,  true,  false, OV_METAL_2,    4ULL * 1024 * 1024 * 1024 },
    { OV_MACOS_13_VENTURA,       "macOS 13",      "Ventura",       22,  true,  true,  true,  true,  true,  OV_METAL_2,    4ULL * 1024 * 1024 * 1024 },
    { OV_MACOS_14_SONOMA,        "macOS 14",      "Sonoma",        23,  true,  true,  true,  true,  true,  OV_METAL_2,    8ULL * 1024 * 1024 * 1024 },
    { OV_MACOS_15_SEQUOIA,       "macOS 15",      "Sequoia",       24,  true,  true,  true,  true,  true,  OV_METAL_2,    8ULL * 1024 * 1024 * 1024 }
};

static const uint32_t g_macos_count = sizeof(g_macos_versions) / sizeof(g_macos_versions[0]);

ov_status_t ov_macos_compat_init(void) {
    ov_log_info("macOS Compatibility Subsystem initialized (%u OS versions indexed)", g_macos_count);
    return OV_SUCCESS;
}

void ov_macos_compat_cleanup(void) {
    ov_log_info("macOS Compatibility Subsystem cleaned up");
}

const char* ov_macos_version_name(ov_macos_version_t version) {
    for (uint32_t i = 0; i < g_macos_count; i++) {
        if (g_macos_versions[i].version == version) {
            return g_macos_versions[i].marketing_name;
        }
    }
    return "Unknown macOS Version";
}

const char* ov_macos_code_name(ov_macos_version_t version) {
    for (uint32_t i = 0; i < g_macos_count; i++) {
        if (g_macos_versions[i].version == version) {
            return g_macos_versions[i].code_name;
        }
    }
    return "Unknown";
}

static const macos_version_info_t* find_version_info(ov_macos_version_t version) {
    for (uint32_t i = 0; i < g_macos_count; i++) {
        if (g_macos_versions[i].version == version) {
            return &g_macos_versions[i];
        }
    }
    return NULL;
}

ov_status_t ov_macos_compat_evaluate(ov_macos_version_t target_os,
                                     const ov_hardware_profile_t *hw,
                                     ov_macos_compat_eval_t *out_eval) {
    if (!hw || !out_eval) return OV_ERROR_INVALID_PARAM;
    memset(out_eval, 0, sizeof(ov_macos_compat_eval_t));

    const macos_version_info_t *os_info = find_version_info(target_os);
    if (!os_info) return OV_ERROR_NOT_FOUND;

    out_eval->target_os = target_os;
    snprintf(out_eval->target_os_name, sizeof(out_eval->target_os_name), "%s (%s)",
             os_info->marketing_name, os_info->code_name);
    out_eval->darwin_kernel_version = os_info->darwin_major;
    out_eval->rating = OV_MACOS_SUPPORTED_NATIVE;

    bool is_apple_silicon_os = (target_os >= OV_MACOS_11_BIG_SUR);
    bool is_apple_silicon_hw = (hw->cpu.type >= OV_CPU_ARM64_M1 && hw->cpu.type <= OV_CPU_ARM64_GENERIC);

    /* 1. CPU Architecture Compatibility */
    if (!is_apple_silicon_hw && is_apple_silicon_os && target_os > OV_MACOS_15_SEQUOIA) {
        out_eval->rating = OV_MACOS_UNSUPPORTED;
        snprintf(out_eval->failure_reason, sizeof(out_eval->failure_reason),
                 "Target OS requires Apple Silicon ARM64; host CPU is x86_64 Intel/AMD.");
        return OV_SUCCESS;
    }

    if (is_apple_silicon_hw && target_os < OV_MACOS_11_BIG_SUR) {
        out_eval->rating = OV_MACOS_SUPPORTED_SIMULATED;
        out_eval->requires_dynamic_binary_translation = true;
        snprintf(out_eval->notes, sizeof(out_eval->notes),
                 "Legacy x86_64 macOS on Apple Silicon requires full CPU virtualization or emulated QEMU environment.");
        return OV_SUCCESS;
    }

    /* 2. 64-bit CPU Requirement */
    if (os_info->requires_64bit_cpu && !hw->cpu.is_64bit) {
        out_eval->rating = OV_MACOS_UNSUPPORTED;
        snprintf(out_eval->failure_reason, sizeof(out_eval->failure_reason),
                 "macOS %s requires 64-bit CPU architecture; CPU '%s' is 32-bit only.",
                 os_info->code_name, hw->cpu.model_name);
        return OV_SUCCESS;
    }

    /* 3. 64-bit EFI Firmware Requirement */
    if (os_info->requires_64bit_efi && !hw->efi_is_64bit) {
        out_eval->rating = OV_MACOS_SUPPORTED_WITH_PATCHES;
        out_eval->patcher_recommended = OV_PATCHER_OPENCORE_LEGACY;
        snprintf(out_eval->firmware_quirks, sizeof(out_eval->firmware_quirks),
                 "32-bit EFI firmware detected; 64-bit kernel booting requires boot.efi replacement / OpenCore emulation.");
    }

    /* 4. SSE4.1 Instruction Requirement (Sierra+) */
    if (os_info->requires_sse41 && !hw->cpu.has_sse41) {
        out_eval->rating = OV_MACOS_SUPPORTED_WITH_PATCHES;
        out_eval->patcher_recommended = OV_PATCHER_OPENCORE_LEGACY;
        out_eval->requires_dynamic_binary_translation = true;
        snprintf(out_eval->cpu_features_missing, sizeof(out_eval->cpu_features_missing),
                 "Missing SSE4.1 instruction set; kernel relies on SSE4.1 emulation patch.");
    }

    /* 5. AVX2 Instruction Requirement (Ventura+) */
    if (os_info->requires_avx2 && !hw->cpu.has_avx2 && !is_apple_silicon_hw) {
        out_eval->rating = OV_MACOS_SUPPORTED_WITH_PATCHES;
        out_eval->patcher_recommended = OV_PATCHER_OPENCORE_LEGACY;
        out_eval->requires_cryptex_patches = true;
        snprintf(out_eval->cpu_features_missing, sizeof(out_eval->cpu_features_missing),
                 "Missing AVX2 instructions; requires Ventura+ Cryptex patch and Rosetta/x86_64 fallback libraries.");
    }

    /* 6. GPU & Metal Evaluation */
    const ov_gpu_info_t *effective_gpu = hw->has_discrete_gpu ? &hw->secondary_gpu : &hw->gpu;

    if (os_info->min_metal > OV_METAL_NONE) {
        if (!effective_gpu->supports_metal || effective_gpu->metal_level < os_info->min_metal) {
            out_eval->requires_legacy_graphics_acceleration = true;
            if (out_eval->rating == OV_MACOS_SUPPORTED_NATIVE) {
                out_eval->rating = OV_MACOS_SUPPORTED_WITH_PATCHES;
                out_eval->patcher_recommended = OV_PATCHER_OPENCORE_LEGACY;
            }
            snprintf(out_eval->gpu_compatibility_notes, sizeof(out_eval->gpu_compatibility_notes),
                     "GPU '%s' does not natively satisfy Metal %s; requires legacy OpenGL/Metal shim patching.",
                     effective_gpu->model_name, ov_metal_support_to_string(os_info->min_metal));
        } else {
            snprintf(out_eval->gpu_compatibility_notes, sizeof(out_eval->gpu_compatibility_notes),
                     "GPU '%s' satisfies Metal requirement (%s).",
                     effective_gpu->model_name, ov_metal_support_to_string(effective_gpu->metal_level));
        }
    }

    /* 7. Special Profile Evaluations: MacBookPro9,1 Specific Logic */
    if (hw->profile_id == OV_HW_PROFILE_MBP91_IVY_BRIDGE) {
        if (target_os <= OV_MACOS_10_15_CATALINA) {
            out_eval->rating = OV_MACOS_SUPPORTED_NATIVE;
            snprintf(out_eval->notes, sizeof(out_eval->notes),
                     "Native support. Intel HD 4000 and NVIDIA GT 650M switchable graphics function with factory drivers.");
        } else if (target_os == OV_MACOS_11_BIG_SUR) {
            out_eval->rating = OV_MACOS_SUPPORTED_WITH_PATCHES;
            out_eval->patcher_recommended = OV_PATCHER_OPENCORE_LEGACY;
            snprintf(out_eval->notes, sizeof(out_eval->notes),
                     "Supported via OCLP. Native Kepler GT 650M drivers exist in Big Sur; HD 4000 requires SIP relaxation.");
        } else if (target_os == OV_MACOS_12_MONTEREY) {
            out_eval->rating = OV_MACOS_SUPPORTED_WITH_PATCHES;
            out_eval->patcher_recommended = OV_PATCHER_OPENCORE_LEGACY;
            snprintf(out_eval->notes, sizeof(out_eval->notes),
                     "Supported via OCLP. Apple removed Kepler NVIDIA drivers; OCLP reinstalls Kepler Metal bundle.");
        } else if (target_os >= OV_MACOS_13_VENTURA) {
            out_eval->rating = OV_MACOS_SUPPORTED_WITH_PATCHES;
            out_eval->patcher_recommended = OV_PATCHER_OPENCORE_LEGACY;
            out_eval->requires_cryptex_patches = true;
            out_eval->requires_legacy_graphics_acceleration = true;
            snprintf(out_eval->notes, sizeof(out_eval->notes),
                     "Supported via OCLP. Requires Ivy Bridge non-AVX2 Cryptex patch and legacy Metal 3802 graphics drivers.");
        }
    }

    /* 8. Memory Check */
    if (hw->mem.total_bytes < os_info->min_ram_bytes) {
        if (out_eval->rating != OV_MACOS_UNSUPPORTED) {
            out_eval->rating = OV_MACOS_SUPPORTED_WITH_PATCHES;
            char ram_note[128];
            snprintf(ram_note, sizeof(ram_note), " (Warning: Installed RAM %llu MB is below minimum recommended %llu MB)",
                     (unsigned long long)(hw->mem.total_bytes / (1024 * 1024)),
                     (unsigned long long)(os_info->min_ram_bytes / (1024 * 1024)));
            strncat(out_eval->notes, ram_note, sizeof(out_eval->notes) - strlen(out_eval->notes) - 1);
        }
    }

    return OV_SUCCESS;
}

ov_status_t ov_macos_compat_evaluate_active(ov_macos_version_t target_os, ov_macos_compat_eval_t *out_eval) {
    const ov_hardware_profile_t *active = ov_hardware_get_active_profile();
    return ov_macos_compat_evaluate(target_os, active, out_eval);
}

ov_status_t ov_macos_compat_evaluate_matrix(const ov_hardware_profile_t *hw, ov_macos_compat_matrix_t *out_matrix) {
    if (!hw || !out_matrix) return OV_ERROR_INVALID_PARAM;
    memset(out_matrix, 0, sizeof(ov_macos_compat_matrix_t));

    out_matrix->target_count = g_macos_count;
    for (uint32_t i = 0; i < g_macos_count; i++) {
        ov_macos_compat_evaluate(g_macos_versions[i].version, hw, &out_matrix->evaluations[i]);
    }

    return OV_SUCCESS;
}

void ov_macos_compat_format_eval(const ov_macos_compat_eval_t *eval, char *out_buf, size_t max_len) {
    if (!eval || !out_buf || max_len == 0) return;

    const char *patcher_str = "None (Native)";
    if (eval->patcher_recommended == OV_PATCHER_OPENCORE_LEGACY) patcher_str = "OpenCore Legacy Patcher (OCLP)";
    else if (eval->patcher_recommended == OV_PATCHER_DOSDUDE1) patcher_str = "dosdude1 Patcher";

    snprintf(out_buf, max_len,
        "Target OS:             %s (Darwin %u)\n"
        "Compatibility Rating:  %s\n"
        "Recommended Patcher:   %s\n"
        "Graphics Acceleration: %s\n"
        "Cryptex Patches:       %s\n"
        "Binary Translation:    %s\n"
        "Status Notes:          %s\n",
        eval->target_os_name, eval->darwin_kernel_version,
        ov_macos_compat_rating_to_string(eval->rating),
        patcher_str,
        eval->requires_legacy_graphics_acceleration ? "Required (Legacy Metal / GL shim)" : "Native",
        eval->requires_cryptex_patches ? "Required (Non-AVX2 Cryptex OS.dmg)" : "Not Required",
        eval->requires_dynamic_binary_translation ? "Required" : "Native",
        eval->notes[0] ? eval->notes : (eval->failure_reason[0] ? eval->failure_reason : "Fully compatible")
    );
}

void ov_macos_compat_format_matrix(const ov_hardware_profile_t *hw, char *out_buf, size_t max_len) {
    if (!hw || !out_buf || max_len == 0) return;

    ov_macos_compat_matrix_t matrix;
    ov_macos_compat_evaluate_matrix(hw, &matrix);

    int offset = snprintf(out_buf, max_len,
        "================================================================================\n"
        " macOS Architecture Compatibility Matrix: %s (%s)\n"
        "================================================================================\n"
        " %-18s | %-22s | %-16s | %s\n"
        "--------------------------------------------------------------------------------\n",
        hw->model_identifier, hw->marketing_name,
        "macOS Release", "Rating", "Patcher Required", "Key Architectural Notes"
    );

    for (uint32_t i = 0; i < matrix.target_count && (size_t)offset < max_len - 128; i++) {
        const ov_macos_compat_eval_t *e = &matrix.evaluations[i];
        const char *patcher = "Native";
        if (e->patcher_recommended == OV_PATCHER_OPENCORE_LEGACY) patcher = "OCLP";
        else if (e->patcher_recommended == OV_PATCHER_DOSDUDE1) patcher = "dosdude1";
        else if (e->rating == OV_MACOS_UNSUPPORTED) patcher = "N/A";

        const char *short_note = e->notes;
        if (!short_note[0]) short_note = e->failure_reason;
        if (!short_note[0]) short_note = "Native Factory Support";

        char truncated_note[48];
        snprintf(truncated_note, sizeof(truncated_note), "%s", short_note);

        offset += snprintf(out_buf + offset, max_len - offset,
            " %-18s | %-22s | %-16s | %s\n",
            e->target_os_name,
            ov_macos_compat_rating_to_string(e->rating),
            patcher,
            truncated_note
        );
    }

    if ((size_t)offset < max_len - 82) {
        snprintf(out_buf + offset, max_len - offset,
            "================================================================================\n");
    }
}
