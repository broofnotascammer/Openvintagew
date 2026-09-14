/**
 * OpenVintage - OCLP (OpenCore Legacy Patcher) Integration Adapter (Phase 6)
 * Explicit integration contract: detects installation, inspects versions,
 * reads configuration, reports status, and coexists safely. Never impersonates OCLP.
 */

#ifndef OV_OCLP_ADAPTER_H
#define OV_OCLP_ADAPTER_H

#include "ov_types.h"
#include "ov_hardware.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    bool is_installed;
    char version[32];
    char install_path[256];
    bool root_patches_applied;
    char active_patches_summary[256];
    bool sip_status_supported;
    bool amfi_disabled;
    bool kext_collection_valid;
} ov_oclp_info_t;

ov_status_t ov_oclp_adapter_init(void);
void        ov_oclp_adapter_cleanup(void);

ov_status_t ov_oclp_adapter_detect(ov_oclp_info_t *out_info);
ov_status_t ov_oclp_adapter_read_config(const char *config_plist_path, char *out_boot_args, size_t max_len);
ov_status_t ov_oclp_adapter_get_compatibility_status(const ov_hardware_profile_t *hw, char *out_status, size_t max_len);

/* Allows deterministic verification in automated test environments */
void        ov_oclp_adapter_set_mock_state(bool installed, const char *version, bool root_patches_applied);

#endif /* OV_OCLP_ADAPTER_H */
