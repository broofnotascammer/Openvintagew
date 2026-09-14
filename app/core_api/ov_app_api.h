/**
 * OpenVintage - High-Level Unified Application API (Phase 6)
 * Orchestrates Hardware, Compatibility, Performance Profiles, Boot Picker,
 * and Safe Deployment under a clean public API contract.
 */

#ifndef OV_APP_API_H
#define OV_APP_API_H

#include "ov_types.h"
#include "ov_hardware.h"
#include "ov_compatibility.h"
#include "ov_perf_profile.h"
#include "ov_boot_picker.h"
#include "ov_deployment.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    char                app_version[32];
    char                hardware_model[64];
    bool                is_real_hardware;
    char                hardware_source[32];
    uint32_t            gpu_count;
    char                active_gpu_name[64];
    uint32_t            active_gpu_index;
    char                active_perf_profile[32];
    uint32_t            boot_target_count;
    int32_t             selected_boot_index;
    char                selected_boot_label[64];
    ov_deploy_state_t   deploy_state;
    ov_deploy_step_t    deploy_step;
} ov_app_status_summary_t;

ov_status_t ov_app_init(void);
void        ov_app_cleanup(void);

ov_status_t ov_app_get_status_summary(ov_app_status_summary_t *out_summary);
ov_status_t ov_app_detect_hardware(bool force_native);
ov_status_t ov_app_check_os_compatibility(const char *target_os, ov_os_compat_result_t *out_result);
ov_status_t ov_app_set_performance_profile(const char *profile_name);
ov_status_t ov_app_refresh_boot_targets(void);
ov_status_t ov_app_select_boot_target(uint32_t index);
ov_status_t ov_app_deployment_run_flow(bool user_approved, bool simulate_failure, char *out_log, size_t max_len);

#endif /* OV_APP_API_H */
