/**
 * OpenVintage - Boot Picker & Boot Management Subsystem (Phase 6)
 * Hardware-aware boot target discovery, policy management, and boot delegation.
 */

#ifndef OV_BOOT_PICKER_H
#define OV_BOOT_PICKER_H

#include "ov_types.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define OV_MAX_BOOT_TARGETS 16

typedef struct {
    uint32_t             id;
    char                 label[64];
    ov_boot_target_type_t type;
    char                 device_path[256];
    char                 partition_uuid[64];
    char                 fs_type[32];
    bool                 is_default;
    bool                 is_hidden;
    uint32_t             boot_order;
    char                 custom_args[128];
} ov_boot_target_t;

typedef struct {
    uint32_t             target_count;
    ov_boot_target_t     targets[OV_MAX_BOOT_TARGETS];
    int32_t              selected_index;
    int32_t              default_index;
    uint32_t             timeout_seconds;
    uint32_t             countdown_remaining;
    bool                 timeout_enabled;
    bool                 auto_boot_triggered;
} ov_boot_picker_state_t;

ov_status_t            ov_boot_picker_init(void);
void                   ov_boot_picker_cleanup(void);

ov_status_t            ov_boot_picker_scan_targets(void);
ov_status_t            ov_boot_picker_add_target(const ov_boot_target_t *target);
uint32_t               ov_boot_picker_get_target_count(void);
const ov_boot_target_t* ov_boot_picker_get_target(uint32_t index);

ov_status_t            ov_boot_picker_select_index(uint32_t index);
int32_t                ov_boot_picker_get_selected_index(void);

ov_status_t            ov_boot_picker_set_default_target(uint32_t index);
int32_t                ov_boot_picker_get_default_index(void);

ov_status_t            ov_boot_picker_set_timeout(uint32_t seconds, bool enabled);
uint32_t               ov_boot_picker_get_timeout(bool *out_enabled);

ov_status_t            ov_boot_picker_tick(uint32_t seconds_elapsed, bool *out_auto_booted);
ov_status_t            ov_boot_picker_boot_selected(char *out_summary, size_t max_len);

const ov_boot_picker_state_t* ov_boot_picker_get_state(void);

#endif /* OV_BOOT_PICKER_H */
