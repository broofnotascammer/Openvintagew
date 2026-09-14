/**
 * OpenVintage - OCLP Integration Adapter Implementation (Phase 6)
 */

#include "ov_oclp_adapter.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>

static bool s_mock_override = false;
static bool s_mock_installed = false;
static char s_mock_version[32] = "1.5.0";
static bool s_mock_root_patches = true;

ov_status_t ov_oclp_adapter_init(void) {
    s_mock_override = false;
    s_mock_installed = false;
    return OV_SUCCESS;
}

void ov_oclp_adapter_cleanup(void) {
    s_mock_override = false;
}

void ov_oclp_adapter_set_mock_state(bool installed, const char *version, bool root_patches_applied) {
    s_mock_override = true;
    s_mock_installed = installed;
    if (version) {
        strncpy(s_mock_version, version, sizeof(s_mock_version) - 1);
    }
    s_mock_root_patches = root_patches_applied;
}

ov_status_t ov_oclp_adapter_detect(ov_oclp_info_t *out_info) {
    if (!out_info) return OV_ERROR_INVALID_PARAM;
    memset(out_info, 0, sizeof(ov_oclp_info_t));

    if (s_mock_override) {
        out_info->is_installed = s_mock_installed;
        if (s_mock_installed) {
            strncpy(out_info->version, s_mock_version, sizeof(out_info->version) - 1);
            strncpy(out_info->install_path, "/Library/Application Support/OpenCore-Patcher", sizeof(out_info->install_path) - 1);
            out_info->root_patches_applied = s_mock_root_patches;
            strncpy(out_info->active_patches_summary,
                    "NVIDIA Kepler Metal driver, Legacy Wireless (BCM94331), Legacy USB 1.1",
                    sizeof(out_info->active_patches_summary) - 1);
            out_info->sip_status_supported = true;
            out_info->amfi_disabled = true;
            out_info->kext_collection_valid = true;
        }
        return OV_SUCCESS;
    }

    /* Probe filesystem for real OCLP installation */
    const char *test_paths[] = {
        "/Library/Application Support/OpenCore-Patcher/OpenCore-Patcher.app",
        "/Applications/OpenCore-Patcher.app",
        "/Volumes/EFI/EFI/OC/OpenCore.efi"
    };

    for (size_t i = 0; i < sizeof(test_paths)/sizeof(test_paths[0]); ++i) {
        if (access(test_paths[i], F_OK) == 0) {
            out_info->is_installed = true;
            strncpy(out_info->version, "Detected on System", sizeof(out_info->version) - 1);
            strncpy(out_info->install_path, test_paths[i], sizeof(out_info->install_path) - 1);
            out_info->root_patches_applied = true;
            out_info->sip_status_supported = true;
            out_info->kext_collection_valid = true;
            return OV_SUCCESS;
        }
    }

    out_info->is_installed = false;
    return OV_SUCCESS;
}

ov_status_t ov_oclp_adapter_read_config(const char *config_plist_path, char *out_boot_args, size_t max_len) {
    if (!out_boot_args || max_len == 0) return OV_ERROR_INVALID_PARAM;
    /* Default OpenCore boot arguments safely coexisting */
    snprintf(out_boot_args, max_len,
             "-v keepsyms=1 debug=0x100 ipc_control_port_options=0 amfi_get_out_of_my_way=1");
    return OV_SUCCESS;
}

ov_status_t ov_oclp_adapter_get_compatibility_status(const ov_hardware_profile_t *hw, char *out_status, size_t max_len) {
    if (!hw || !out_status || max_len == 0) return OV_ERROR_INVALID_PARAM;

    if (hw->has_discrete_gpu && hw->gpu.vendor_id == 0x10DE) {
        snprintf(out_status, max_len,
                 "OCLP Coexistence: Compatible with %s (Kepler root patches supported)", hw->model_identifier);
    } else {
        snprintf(out_status, max_len,
                 "OCLP Coexistence: Generic EFI boot supported for %s", hw->model_identifier);
    }
    return OV_SUCCESS;
}
