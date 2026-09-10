/**
 * OpenVintage Pre-Boot Simulator - macOS Version Compatibility Engine Header
 * Evaluates architectural constraints, kernel requirements, Metal levels, and OCLP patchability.
 */

#ifndef OV_MACOS_COMPAT_H
#define OV_MACOS_COMPAT_H

#include "ov_types.h"
#include "ov_hardware.h"

/* macOS Version Enumeration */
typedef enum {
    OV_MACOS_UNKNOWN = 0,
    OV_MACOS_10_4_TIGER = 1,
    OV_MACOS_10_5_LEOPARD = 2,
    OV_MACOS_10_6_SNOW_LEOPARD = 3,
    OV_MACOS_10_7_LION = 4,
    OV_MACOS_10_8_MOUNTAIN_LION = 5,
    OV_MACOS_10_9_MAVERICKS = 6,
    OV_MACOS_10_10_YOSEMITE = 7,
    OV_MACOS_10_11_EL_CAPITAN = 8,
    OV_MACOS_10_12_SIERRA = 9,
    OV_MACOS_10_13_HIGH_SIERRA = 10,
    OV_MACOS_10_14_MOJAVE = 11,
    OV_MACOS_10_15_CATALINA = 12,
    OV_MACOS_11_BIG_SUR = 13,
    OV_MACOS_12_MONTEREY = 14,
    OV_MACOS_13_VENTURA = 15,
    OV_MACOS_14_SONOMA = 16,
    OV_MACOS_15_SEQUOIA = 17,
    OV_MACOS_COUNT
} ov_macos_version_t;

/* Patcher Recommendation */
typedef enum {
    OV_PATCHER_NONE = 0,
    OV_PATCHER_OPENCORE_LEGACY = 1,
    OV_PATCHER_DOSDUDE1 = 2
} ov_patcher_type_t;

/* Evaluation Result Structure */
typedef struct {
    ov_macos_version_t          target_os;
    char                        target_os_name[64];
    uint32_t                    darwin_kernel_version;
    ov_macos_compat_rating_t    rating;
    ov_patcher_type_t           patcher_recommended;
    bool                        requires_dynamic_binary_translation;
    bool                        requires_legacy_graphics_acceleration;
    bool                        requires_cryptex_patches;
    char                        failure_reason[256];
    char                        firmware_quirks[256];
    char                        cpu_features_missing[256];
    char                        gpu_compatibility_notes[256];
    char                        notes[512];
} ov_macos_compat_eval_t;

/* Matrix Structure */
typedef struct {
    uint32_t                    target_count;
    ov_macos_compat_eval_t      evaluations[32];
} ov_macos_compat_matrix_t;

/* Subsystem APIs */
ov_status_t ov_macos_compat_init(void);
void        ov_macos_compat_cleanup(void);

const char* ov_macos_version_name(ov_macos_version_t version);
const char* ov_macos_code_name(ov_macos_version_t version);

ov_status_t ov_macos_compat_evaluate(ov_macos_version_t target_os,
                                     const ov_hardware_profile_t *hw,
                                     ov_macos_compat_eval_t *out_eval);

ov_status_t ov_macos_compat_evaluate_active(ov_macos_version_t target_os,
                                            ov_macos_compat_eval_t *out_eval);

ov_status_t ov_macos_compat_evaluate_matrix(const ov_hardware_profile_t *hw,
                                            ov_macos_compat_matrix_t *out_matrix);

void ov_macos_compat_format_eval(const ov_macos_compat_eval_t *eval, char *out_buf, size_t max_len);
void ov_macos_compat_format_matrix(const ov_hardware_profile_t *hw, char *out_buf, size_t max_len);

#endif /* OV_MACOS_COMPAT_H */
