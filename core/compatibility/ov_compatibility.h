/**
 * OpenVintage Pre-Boot Simulator - Compatibility & Quirks Framework (Phase 5)
 * Known application catalog, silicon evaluation, and hardware quirk workarounds.
 */

#ifndef OV_COMPATIBILITY_H
#define OV_COMPATIBILITY_H

#include "ov_types.h"
#include "ov_hardware.h"

#define OV_MAX_COMPAT_RECORDS 32

/* Application Compatibility Profile */
typedef struct {
    char                app_name[64];
    char                target_os[32];
    uint32_t            bitness;            /* 32 or 64 */
    uint32_t            required_cpu_arch;  /* 1=ARM64, 2=x86_64, 3=x86_32 */
    uint32_t            min_cpu_cores;
    bool                requires_sse42;
    bool                requires_avx;
    bool                requires_avx2;

    ov_api_type_t       required_api;
    uint32_t            api_version_major;
    uint32_t            api_version_minor;
    bool                requires_compute;
    bool                requires_tessellation;
    uint32_t            max_texture_size;
    uint64_t            min_vram_bytes;
    uint64_t            min_system_ram_bytes;

    char                known_limitations[128];
    ov_compat_mode_t    recommended_mode;
} ov_compat_record_t;

/* Evaluation Result Against Silicon */
typedef struct {
    bool                can_execute;
    ov_compat_mode_t    selected_mode;
    char                execution_plan[128];
    char                clamped_features[128];
    uint32_t            expected_overhead_factor; /* 100 = 1.0x */
    bool                cpu_translation_required;
    bool                gpu_translation_required;
    bool                texture_clamp_applied;
    bool                software_compute_fallback;
} ov_compat_eval_t;

/* Silicon Quirk Entry */
typedef struct {
    char target_silicon[64];
    char component[32];
    char issue_description[128];
    char workaround_applied[128];
    bool is_active_for_current_hw;
} ov_silicon_quirk_t;

/* Dedicated Hardware + OS + Firmware + GPU + Boot Compatibility Evaluation Result */
typedef struct {
    char                 target_os[64];
    ov_compat_category_t category;
    bool                 is_real_hardware;          /* Strictly distinguishes REAL vs SIMULATED */
    char                 hardware_source_label[32]; /* "REAL HARDWARE" or "SIMULATED HARDWARE" */
    char                 recommended_integration[32];/* "Native", "OCLP", "rEFInd", "OpenCore", "None" */
    char                 rationale[256];
    bool                 requires_legacy_gpu_patch; /* e.g. Kepler Metal bundle on macOS 12+ */
    bool                 requires_avx_emulation;    /* e.g. AVX2 Cryptex requirements on Ivy Bridge */
    bool                 requires_efi_boot_shim;    /* 32-bit to 64-bit EFI translation */
    bool                 cryptex_bypass_needed;
    uint32_t             confidence_percent;
} ov_os_compat_result_t;

/* Subsystem APIs */
ov_status_t ov_compatibility_init(void);
void        ov_compatibility_cleanup(void);

uint32_t                  ov_compatibility_get_app_count(void);
const ov_compat_record_t* ov_compatibility_get_app(uint32_t index);
ov_status_t               ov_compatibility_evaluate(const ov_compat_record_t *record, ov_compat_eval_t *out_eval);

/* Dedicated Hardware + OS Compatibility Evaluation */
ov_status_t ov_compatibility_evaluate_os(
    const ov_hardware_profile_t *hw,
    const char                  *target_os,
    ov_os_compat_result_t       *out_result
);

ov_status_t ov_compatibility_evaluate_active_os(
    const char                  *target_os,
    ov_os_compat_result_t       *out_result
);

uint32_t                    ov_compatibility_get_quirk_count(void);
const ov_silicon_quirk_t*   ov_compatibility_get_quirk(uint32_t index);

const char* ov_compat_mode_to_string(ov_compat_mode_t mode);

#endif /* OV_COMPATIBILITY_H */
