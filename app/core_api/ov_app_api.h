/**
 * OpenVintage - High-Level Unified Application API (Phase 7)
 * Complete Application Contract bridging C Core to Native macOS (Swift/AppKit)
 * and Simulator / CLI / Web Bridge interfaces.
 */

#ifndef OV_APP_API_H
#define OV_APP_API_H

#include "ov_types.h"
#include "ov_hardware.h"
#include "ov_compatibility.h"
#include "ov_perf_profile.h"
#include "ov_boot_picker.h"
#include "ov_deployment.h"
#include "ov_oclp_adapter.h"
#include "ov_refind_adapter.h"
#include "ov_efi_installer.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char                app_version[32];
    char                build_id[32];
    ov_release_channel_t release_channel;
    char                channel_name[32];
    char                hardware_model[64];
    bool                is_real_hardware;
    char                hardware_source[32];
    uint32_t            gpu_count;
    char                active_gpu_name[64];
    uint32_t            active_gpu_index;
    bool                gmux_present;
    char                active_perf_profile[32];
    uint32_t            boot_target_count;
    int32_t             selected_boot_index;
    char                selected_boot_label[64];
    ov_deploy_state_t   deploy_state;
    ov_deploy_step_t    deploy_step;
    bool                developer_mode;
    bool                physical_test_mode;
    bool                backup_ready;
    bool                recovery_ready;
} ov_app_status_summary_t;

/* Core Lifecycle */
ov_status_t ov_app_init(void);
void        ov_app_cleanup(void);

/* Status & Channels */
ov_status_t ov_app_get_status_summary(ov_app_status_summary_t *out_summary);
ov_status_t ov_app_set_release_channel(ov_release_channel_t channel);
ov_release_channel_t ov_app_get_release_channel(void);

/* Physical Test Mode & Safety Controls */
ov_status_t ov_app_set_developer_mode(bool enabled);
bool        ov_app_get_developer_mode(void);
ov_status_t ov_app_enable_physical_test_mode(bool user_confirmed);
bool        ov_app_is_physical_test_mode(void);

/* Hardware Management */
ov_status_t ov_app_detect_hardware(bool force_native);
ov_status_t ov_app_load_simulated_profile(const char *model_id);
const ov_hardware_profile_t* ov_app_get_active_hardware_profile(void);
ov_status_t ov_app_switch_active_gpu(uint32_t gpu_index);

/* Compatibility Audit */
ov_status_t ov_app_check_os_compatibility(const char *target_os, ov_os_compat_result_t *out_result);

/* Performance Profiles */
ov_status_t ov_app_set_performance_profile(const char *profile_name);
const ov_perf_profile_config_t* ov_app_get_active_performance_profile(void);

/* Pre-Boot Picker Targets */
ov_status_t ov_app_refresh_boot_targets(void);
uint32_t    ov_app_get_boot_target_count(void);
const ov_boot_target_t* ov_app_get_boot_target(uint32_t index);
ov_status_t ov_app_select_boot_target(uint32_t index);

/* Integrations (OpenCore/OCLP & rEFInd) */
ov_status_t ov_app_get_integrations_status(bool *out_oc_detected, bool *out_refind_detected, char *out_rec_strategy, size_t max_len);

/* Safe Deployment Engine (Wizard, 12-Step, Backup & Rollback) */
const ov_deploy_plan_t* ov_app_deployment_get_plan(void);
ov_status_t ov_app_deployment_create_plan(const char *description);
ov_status_t ov_app_deployment_execute_step(uint32_t wizard_step, bool user_confirmed, char *out_message, size_t max_len);
ov_status_t ov_app_deployment_run_flow(bool user_approved, bool simulate_failure, char *out_log, size_t max_len);
ov_status_t ov_app_deployment_rollback(char *out_status, size_t max_len);

/* Recovery & Diagnostics */
ov_status_t ov_app_export_recovery_package(const char *dest_dir, char *out_pkg_path, size_t max_len);
ov_status_t ov_app_export_diagnostics(int format /* 0=text, 1=json, 2=html */, const char *dest_path);

/* Phase 8: Safe Real-Hardware EFI Installer & Verified Backup/Restore */
ov_status_t ov_app_installer_detect_storage(ov_efi_storage_inventory_t *out_inv);
ov_status_t ov_app_installer_select_usb(uint32_t index, bool user_confirmed);
ov_status_t ov_app_installer_confirm_esp(uint32_t index, bool user_confirmed);
ov_status_t ov_app_installer_audit_artifacts(ov_efi_artifact_t *out_artifacts, uint32_t *out_count, uint32_t max_count);
ov_status_t ov_app_installer_create_backup(const char *usb_mount_path, ov_recovery_package_t *out_pkg);
ov_status_t ov_app_installer_verify_backup(bool simulate_tamper);
ov_status_t ov_app_installer_generate_dry_run(ov_efi_dry_run_t *out_dry_run);
ov_status_t ov_app_installer_check_safety_gates(ov_efi_safety_gates_t *out_gates, bool *out_all_pass);
ov_status_t ov_app_installer_install(bool user_final_confirmed, ov_efi_install_report_t *out_report);
ov_status_t ov_app_installer_rollback(ov_efi_rollback_report_t *out_report);

#ifdef __cplusplus
}
#endif

#endif /* OV_APP_API_H */
