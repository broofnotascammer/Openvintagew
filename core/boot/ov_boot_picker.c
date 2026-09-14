/**
 * OpenVintage - Boot Picker & Boot Management Implementation (Phase 6)
 */

#include "ov_boot_picker.h"
#include <string.h>
#include <stdio.h>

static ov_boot_picker_state_t s_picker;

ov_status_t ov_boot_picker_init(void) {
    memset(&s_picker, 0, sizeof(s_picker));
    s_picker.selected_index = 0;
    s_picker.default_index = 0;
    s_picker.timeout_seconds = 5;
    s_picker.countdown_remaining = 5;
    s_picker.timeout_enabled = true;
    s_picker.auto_boot_triggered = false;
    return OV_SUCCESS;
}

void ov_boot_picker_cleanup(void) {
    memset(&s_picker, 0, sizeof(s_picker));
}

ov_status_t ov_boot_picker_scan_targets(void) {
    s_picker.target_count = 0;

    /* 1. macOS Main Partition */
    ov_boot_target_t mac_target = {
        .id = 1,
        .type = OV_BOOT_TARGET_MACOS,
        .is_default = true,
        .is_hidden = false,
        .boot_order = 1
    };
    strncpy(mac_target.label, "Macintosh HD (macOS)", sizeof(mac_target.label) - 1);
    strncpy(mac_target.device_path, "PciRoot(0x0)/Pci(0x1F,0x2)/Sata(0x0)/HD(2,GPT,C12A7328-F81F-11D2-BA4B-00A0C93EC93B)/System/Library/CoreServices/boot.efi", sizeof(mac_target.device_path) - 1);
    strncpy(mac_target.partition_uuid, "4C98234D-3182-4421-9AC1-EF332918BA01", sizeof(mac_target.partition_uuid) - 1);
    strncpy(mac_target.fs_type, "APFS", sizeof(mac_target.fs_type) - 1);
    strncpy(mac_target.custom_args, "-v keepsyms=1", sizeof(mac_target.custom_args) - 1);
    ov_boot_picker_add_target(&mac_target);

    /* 2. macOS Recovery */
    ov_boot_target_t rec_target = {
        .id = 2,
        .type = OV_BOOT_TARGET_MACOS_RECOVERY,
        .is_default = false,
        .is_hidden = false,
        .boot_order = 2
    };
    strncpy(rec_target.label, "macOS Recovery", sizeof(rec_target.label) - 1);
    strncpy(rec_target.device_path, "PciRoot(0x0)/Pci(0x1F,0x2)/Sata(0x0)/HD(3,GPT,C12A7328-F81F-11D2-BA4B-00A0C93EC93B)/com.apple.recovery.boot/boot.efi", sizeof(rec_target.device_path) - 1);
    strncpy(rec_target.partition_uuid, "4C98234D-3182-4421-9AC1-EF332918BA02", sizeof(rec_target.partition_uuid) - 1);
    strncpy(rec_target.fs_type, "APFS", sizeof(rec_target.fs_type) - 1);
    ov_boot_picker_add_target(&rec_target);

    /* 3. OpenCore Integration Target */
    ov_boot_target_t oc_target = {
        .id = 3,
        .type = OV_BOOT_TARGET_OPENCORE,
        .is_default = false,
        .is_hidden = false,
        .boot_order = 3
    };
    strncpy(oc_target.label, "OpenCore Bootloader", sizeof(oc_target.label) - 1);
    strncpy(oc_target.device_path, "PciRoot(0x0)/Pci(0x1F,0x2)/Sata(0x0)/HD(1,GPT,C12A7328-F81F-11D2-BA4B-00A0C93EC93B)/EFI/OC/OpenCore.efi", sizeof(oc_target.device_path) - 1);
    strncpy(oc_target.partition_uuid, "99B30211-1209-4C01-8172-DEABCF891230", sizeof(oc_target.partition_uuid) - 1);
    strncpy(oc_target.fs_type, "FAT32", sizeof(oc_target.fs_type) - 1);
    ov_boot_picker_add_target(&oc_target);

    /* 4. Linux EFI Stub Target */
    ov_boot_target_t lnx_target = {
        .id = 4,
        .type = OV_BOOT_TARGET_LINUX,
        .is_default = false,
        .is_hidden = false,
        .boot_order = 4
    };
    strncpy(lnx_target.label, "Ubuntu Linux 24.04 LTS", sizeof(lnx_target.label) - 1);
    strncpy(lnx_target.device_path, "PciRoot(0x0)/Pci(0x1F,0x2)/Sata(0x0)/HD(1,GPT,C12A7328-F81F-11D2-BA4B-00A0C93EC93B)/EFI/ubuntu/grubx64.efi", sizeof(lnx_target.device_path) - 1);
    strncpy(lnx_target.partition_uuid, "99B30211-1209-4C01-8172-DEABCF891230", sizeof(lnx_target.partition_uuid) - 1);
    strncpy(lnx_target.fs_type, "FAT32", sizeof(lnx_target.fs_type) - 1);
    strncpy(lnx_target.custom_args, "ro quiet splash", sizeof(lnx_target.custom_args) - 1);
    ov_boot_picker_add_target(&lnx_target);

    /* 5. Windows Boot Manager */
    ov_boot_target_t win_target = {
        .id = 5,
        .type = OV_BOOT_TARGET_WINDOWS,
        .is_default = false,
        .is_hidden = false,
        .boot_order = 5
    };
    strncpy(win_target.label, "Windows Boot Manager", sizeof(win_target.label) - 1);
    strncpy(win_target.device_path, "PciRoot(0x0)/Pci(0x1F,0x2)/Sata(0x0)/HD(1,GPT,C12A7328-F81F-11D2-BA4B-00A0C93EC93B)/EFI/Microsoft/Boot/bootmgfw.efi", sizeof(win_target.device_path) - 1);
    strncpy(win_target.partition_uuid, "99B30211-1209-4C01-8172-DEABCF891230", sizeof(win_target.partition_uuid) - 1);
    strncpy(win_target.fs_type, "FAT32", sizeof(win_target.fs_type) - 1);
    ov_boot_picker_add_target(&win_target);

    /* 6. OpenVintage Firmware Tool */
    ov_boot_target_t efi_app = {
        .id = 6,
        .type = OV_BOOT_TARGET_EFI_APP,
        .is_default = false,
        .is_hidden = false,
        .boot_order = 6
    };
    strncpy(efi_app.label, "OpenVintage EFI Self-Test Tool", sizeof(efi_app.label) - 1);
    strncpy(efi_app.device_path, "PciRoot(0x0)/Pci(0x1F,0x2)/Sata(0x0)/HD(1,GPT,C12A7328-F81F-11D2-BA4B-00A0C93EC93B)/EFI/OpenVintage/OvSelfTestApp.efi", sizeof(efi_app.device_path) - 1);
    strncpy(efi_app.partition_uuid, "99B30211-1209-4C01-8172-DEABCF891230", sizeof(efi_app.partition_uuid) - 1);
    strncpy(efi_app.fs_type, "FAT32", sizeof(efi_app.fs_type) - 1);
    ov_boot_picker_add_target(&efi_app);

    s_picker.selected_index = 0;
    s_picker.default_index = 0;
    s_picker.countdown_remaining = s_picker.timeout_seconds;
    return OV_SUCCESS;
}

ov_status_t ov_boot_picker_add_target(const ov_boot_target_t *target) {
    if (!target) return OV_ERROR_INVALID_PARAM;
    if (s_picker.target_count >= OV_MAX_BOOT_TARGETS) return OV_ERROR_OUT_OF_RESOURCES;

    s_picker.targets[s_picker.target_count] = *target;
    if (target->is_default) {
        s_picker.default_index = (int32_t)s_picker.target_count;
        s_picker.selected_index = (int32_t)s_picker.target_count;
    }
    s_picker.target_count++;
    return OV_SUCCESS;
}

uint32_t ov_boot_picker_get_target_count(void) {
    return s_picker.target_count;
}

const ov_boot_target_t* ov_boot_picker_get_target(uint32_t index) {
    if (index >= s_picker.target_count) return NULL;
    return &s_picker.targets[index];
}

ov_status_t ov_boot_picker_select_index(uint32_t index) {
    if (index >= s_picker.target_count) return OV_ERROR_INVALID_PARAM;
    s_picker.selected_index = (int32_t)index;
    /* Manual selection cancels auto-boot countdown */
    s_picker.timeout_enabled = false;
    return OV_SUCCESS;
}

int32_t ov_boot_picker_get_selected_index(void) {
    return s_picker.selected_index;
}

ov_status_t ov_boot_picker_set_default_target(uint32_t index) {
    if (index >= s_picker.target_count) return OV_ERROR_INVALID_PARAM;
    for (uint32_t i = 0; i < s_picker.target_count; ++i) {
        s_picker.targets[i].is_default = (i == index);
    }
    s_picker.default_index = (int32_t)index;
    return OV_SUCCESS;
}

int32_t ov_boot_picker_get_default_index(void) {
    return s_picker.default_index;
}

ov_status_t ov_boot_picker_set_timeout(uint32_t seconds, bool enabled) {
    s_picker.timeout_seconds = seconds;
    s_picker.countdown_remaining = seconds;
    s_picker.timeout_enabled = enabled;
    return OV_SUCCESS;
}

uint32_t ov_boot_picker_get_timeout(bool *out_enabled) {
    if (out_enabled) *out_enabled = s_picker.timeout_enabled;
    return s_picker.timeout_seconds;
}

ov_status_t ov_boot_picker_tick(uint32_t seconds_elapsed, bool *out_auto_booted) {
    if (out_auto_booted) *out_auto_booted = false;
    if (!s_picker.timeout_enabled || s_picker.auto_boot_triggered) {
        return OV_SUCCESS;
    }

    if (s_picker.countdown_remaining <= seconds_elapsed) {
        s_picker.countdown_remaining = 0;
        s_picker.auto_boot_triggered = true;
        if (out_auto_booted) *out_auto_booted = true;
    } else {
        s_picker.countdown_remaining -= seconds_elapsed;
    }
    return OV_SUCCESS;
}

ov_status_t ov_boot_picker_boot_selected(char *out_summary, size_t max_len) {
    if (s_picker.selected_index < 0 || (uint32_t)s_picker.selected_index >= s_picker.target_count) {
        return OV_ERROR_INVALID_PARAM;
    }
    const ov_boot_target_t *target = &s_picker.targets[s_picker.selected_index];
    if (out_summary && max_len > 0) {
        snprintf(out_summary, max_len,
                 "Booting '%s' [%s] via %s (UUID: %s, Args: %s)",
                 target->label,
                 ov_boot_target_type_to_string(target->type),
                 target->device_path,
                 target->partition_uuid,
                 target->custom_args[0] ? target->custom_args : "none");
    }
    return OV_SUCCESS;
}

const ov_boot_picker_state_t* ov_boot_picker_get_state(void) {
    return &s_picker;
}
