/**
 * OpenVintage - High-Level Unified Application API Implementation (Phase 6)
 */

#include "ov_app_api.h"
#include <string.h>
#include <stdio.h>

ov_status_t ov_app_init(void) {
    ov_hardware_init();
    ov_compatibility_init();
    ov_perf_profile_init();
    ov_boot_picker_init();
    ov_boot_picker_scan_targets();
    ov_deployment_init();
    return OV_SUCCESS;
}

void ov_app_cleanup(void) {
    ov_deployment_cleanup();
    ov_boot_picker_cleanup();
    ov_perf_profile_cleanup();
    ov_compatibility_cleanup();
    ov_hardware_cleanup();
}

ov_status_t ov_app_get_status_summary(ov_app_status_summary_t *out_summary) {
    if (!out_summary) return OV_ERROR_INVALID_PARAM;
    memset(out_summary, 0, sizeof(ov_app_status_summary_t));

    strncpy(out_summary->app_version, "OpenVintage 6.0.0 (Phase 6)", sizeof(out_summary->app_version) - 1);

    const ov_hardware_profile_t *hw = ov_hardware_get_active_profile();
    if (hw) {
        strncpy(out_summary->hardware_model, hw->model_identifier, sizeof(out_summary->hardware_model) - 1);
        out_summary->is_real_hardware = (!hw->is_simulated && hw->source == OV_HW_SOURCE_NATIVE);
        strncpy(out_summary->hardware_source, out_summary->is_real_hardware ? "REAL HARDWARE" : "SIMULATED HARDWARE",
                sizeof(out_summary->hardware_source) - 1);
    }

    const ov_gpu_topology_t *topo = ov_hardware_get_gpu_topology();
    if (topo) {
        out_summary->gpu_count = topo->gpu_count;
        out_summary->active_gpu_index = topo->active_gpu_index;
        const ov_gpu_info_t *act_gpu = ov_hardware_get_active_gpu();
        if (act_gpu) {
            strncpy(out_summary->active_gpu_name, act_gpu->model_name, sizeof(out_summary->active_gpu_name) - 1);
        }
    }

    const ov_perf_profile_config_t *perf = ov_perf_profile_get_active();
    if (perf) {
        strncpy(out_summary->active_perf_profile, perf->name, sizeof(out_summary->active_perf_profile) - 1);
    }

    out_summary->boot_target_count = ov_boot_picker_get_target_count();
    out_summary->selected_boot_index = ov_boot_picker_get_selected_index();
    const ov_boot_target_t *tgt = ov_boot_picker_get_target((uint32_t)out_summary->selected_boot_index);
    if (tgt) {
        strncpy(out_summary->selected_boot_label, tgt->label, sizeof(out_summary->selected_boot_label) - 1);
    }

    out_summary->deploy_state = ov_deployment_get_current_state();
    out_summary->deploy_step = ov_deployment_get_current_step();

    return OV_SUCCESS;
}

ov_status_t ov_app_detect_hardware(bool force_native) {
    if (force_native) {
        return ov_hardware_set_mode(OV_HW_MODE_NATIVE);
    }
    return ov_hardware_init();
}

ov_status_t ov_app_check_os_compatibility(const char *target_os, ov_os_compat_result_t *out_result) {
    return ov_compatibility_evaluate_active_os(target_os, out_result);
}

ov_status_t ov_app_set_performance_profile(const char *profile_name) {
    return ov_perf_profile_apply_by_name(profile_name);
}

ov_status_t ov_app_refresh_boot_targets(void) {
    return ov_boot_picker_scan_targets();
}

ov_status_t ov_app_select_boot_target(uint32_t index) {
    return ov_boot_picker_select_index(index);
}

ov_status_t ov_app_deployment_run_flow(bool user_approved, bool simulate_failure, char *out_log, size_t max_len) {
    const ov_hardware_profile_t *hw = ov_hardware_get_active_profile();

    /* Step 1: Discover */
    ov_status_t st = ov_deployment_step_discover(hw);
    if (st != OV_SUCCESS) return st;

    /* Step 2: Simulate */
    st = ov_deployment_step_simulate();
    if (st != OV_SUCCESS) return st;

    /* Step 3: Plan */
    st = ov_deployment_step_create_plan("Full OpenVintage EFI Deployment");
    if (st != OV_SUCCESS) return st;

    /* Step 4: Show Changes */
    char changes[256];
    st = ov_deployment_show_changes(changes, sizeof(changes));
    if (st != OV_SUCCESS) return st;

    /* Step 5: User Approval Guard */
    st = ov_deployment_require_user_approval();
    if (st != OV_SUCCESS) return st;

    if (!user_approved) {
        if (out_log && max_len > 0) {
            snprintf(out_log, max_len, "Deployment aborted: User approval not granted.");
        }
        return OV_ERROR_PERMISSION_DENIED;
    }

    st = ov_deployment_grant_user_approval();
    if (st != OV_SUCCESS) return st;

    /* Step 6: Backup */
    st = ov_deployment_step_backup();
    if (st != OV_SUCCESS) return st;

    /* Step 7: Apply */
    st = ov_deployment_step_apply();
    if (st != OV_SUCCESS) return st;

    /* Step 8: Verify */
    st = ov_deployment_step_verify(simulate_failure);
    if (st != OV_SUCCESS) {
        /* Step 9: Rollback on failure */
        ov_deployment_step_rollback();
        if (out_log && max_len > 0) {
            snprintf(out_log, max_len, "Verification failed! Safe rollback executed automatically.");
        }
        return OV_ERROR_GENERIC;
    }

    if (out_log && max_len > 0) {
        snprintf(out_log, max_len, "Deployment fully verified and complete.");
    }
    return OV_SUCCESS;
}
