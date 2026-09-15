/**
 * OpenVintage - Safe Real-Hardware EFI Installer & Verified Backup/Restore (Phase 8)
 *
 * CRITICAL ARCHITECTURE RULE:
 * OpenVintage does NOT replace, rewrite, or flash the Mac's physical firmware.
 * Physical installation consists ONLY of the required OpenVintage EFI binaries
 * in EFI/OpenVintage/ and minimum boot configuration.
 * Firmware modification = NONE, ROM modification = NONE.
 */

#ifndef OV_EFI_INSTALLER_H
#define OV_EFI_INSTALLER_H

#include "ov_types.h"
#include "ov_hardware.h"
#include "ov_security.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OV_MAX_STORAGE_DEVICES       8
#define OV_MAX_INSTALL_ARTIFACTS     8
#define OV_MAX_BACKUP_FILES          32
#define OV_SHA256_STRING_LEN         65

/* Storage Target Representation */
typedef struct {
    char        device_node[64];      /* e.g. "/dev/disk0s1" or "/dev/disk2s1" */
    char        mount_point[256];     /* e.g. "/Volumes/EFI" or "/Volumes/OV_USB_RECOVERY" */
    char        volume_label[64];     /* e.g. "EFI" or "OV_RECOVERY" */
    char        filesystem_type[32];   /* e.g. "FAT32" */
    uint64_t    capacity_bytes;
    uint64_t    free_bytes;
    bool        is_removable;         /* true for USB drive, false for internal */
    bool        is_internal_esp;      /* true for internal EFI System Partition */
    bool        is_detected;
    bool        is_selected;
    bool        is_user_confirmed;
} ov_efi_storage_target_t;

typedef struct {
    uint32_t                device_count;
    ov_efi_storage_target_t devices[OV_MAX_STORAGE_DEVICES];
    int32_t                 internal_esp_index;
    int32_t                 selected_usb_index;
} ov_efi_storage_inventory_t;

/* Authoritative EFI Release Artifact Representation */
typedef enum {
    OV_ARTIFACT_BOOT_APP      = 0,  /* OpenVintageBootApp.efi - boot picker & pre-boot execution */
    OV_ARTIFACT_HAL_DXE       = 1,  /* OpenVintageHalDxe.efi - pre-boot HAL & hardware inspection */
    OV_ARTIFACT_CONFIG        = 2,  /* config.plist - pre-boot configuration */
    OV_ARTIFACT_EXCLUDED_TEST = 3,  /* OvSelfTestApp.efi - development test suite, EXCLUDED */
    OV_ARTIFACT_FORBIDDEN_FW  = 4   /* OPENVINTAGE.fd - SPI/ROM firmware image, STRICTLY FORBIDDEN */
} ov_artifact_role_t;

typedef struct {
    char                filename[64];
    char                source_path[256];
    char                target_esp_path[256];
    size_t              file_size;
    char                sha256[OV_SHA256_STRING_LEN];
    ov_artifact_role_t  role;
    bool                is_required_for_physical_install;
    bool                is_rejected_forbidden;
    char                rejection_reason[128];
} ov_efi_artifact_t;

/* Recovery Package Item & Manifest */
typedef struct {
    char        relative_path[256];   /* e.g. "EFI-BACKUP/APPLE/EXTENSIONS/Firmware.scap" */
    char        source_esp_path[256];
    size_t      size_bytes;
    char        sha256[OV_SHA256_STRING_LEN];
    bool        backed_up;
    bool        verified_on_usb;
} ov_recovery_item_t;

typedef struct {
    char                package_root[256];    /* e.g. "/Volumes/OV_USB_RECOVERY/OPENVINTAGE-RECOVERY" */
    char                target_model[64];     /* "MacBookPro9,1" */
    char                timestamp[32];
    uint32_t            item_count;
    ov_recovery_item_t  items[OV_MAX_BACKUP_FILES];
    bool                has_opencore_backup;
    bool                has_oclp_backup;
    bool                has_refind_backup;
    bool                manifest_generated;
    bool                checksums_written;
    bool                all_checksums_verified;
    bool                is_recovery_verified;
} ov_recovery_package_t;

/* Dry Run Plan */
typedef struct {
    char    source_summary[128];
    char    target_esp[128];
    char    backup_usb[128];
    char    files_to_install[512];
    char    files_to_modify[256];
    char    files_to_preserve[512];
    char    firmware_modification_status[64]; /* "NONE (Physical ROM/SPI Unaltered)" */
    char    rom_modification_status[64];      /* "NONE (Physical ROM/SPI Unaltered)" */
    char    full_text_preview[2048];
} ov_efi_dry_run_t;

/* The 17 Strict Safety Gates */
typedef struct {
    bool developer_mode_enabled;
    bool physical_test_mode_enabled;
    bool target_mac_identified_mbp91;
    bool usb_recovery_device_detected;
    bool correct_usb_device_confirmed;
    bool recovery_backup_created;
    bool recovery_backup_verified;
    bool required_efi_artifacts_identified;
    bool efi_artifacts_hash_verified;
    bool preboot_tests_pass;
    bool hardware_audit_passes;
    bool simulation_passes;
    bool deployment_plan_generated;
    bool exact_files_displayed;
    bool firmware_modification_none;
    bool rom_modification_none;
    bool user_explicitly_confirms_install;
} ov_efi_safety_gates_t;

/* Post-Install and Rollback Reports */
typedef struct {
    bool        binaries_exist;
    bool        hashes_match;
    bool        esp_filesystem_readable;
    bool        apple_files_intact;
    bool        existing_bootloaders_intact;
    bool        boot_config_valid;
    bool        no_unexpected_files_modified;
    bool        zero_rom_touched;
    bool        overall_success;
    char        report_summary[512];
} ov_efi_install_report_t;

typedef struct {
    bool        original_files_restored;
    bool        openvintage_files_removed;
    bool        checksums_match_original;
    bool        original_boot_config_restored;
    bool        rollback_verified;
    char        report_summary[512];
} ov_efi_rollback_report_t;

/* Core Subsystem Lifecycle */
ov_status_t ov_efi_installer_init(void);
void        ov_efi_installer_cleanup(void);

/* Storage Detection & Device Confirmation */
ov_status_t ov_efi_installer_detect_storage(ov_efi_storage_inventory_t *out_inv);
ov_status_t ov_efi_installer_select_usb_device(uint32_t device_index, bool user_confirmed);
ov_status_t ov_efi_installer_confirm_internal_esp(uint32_t esp_index, bool user_confirmed);
const ov_efi_storage_inventory_t* ov_efi_installer_get_storage_inventory(void);

/* EFI Artifact Resolution & Release Model */
ov_status_t ov_efi_installer_audit_artifacts(ov_efi_artifact_t *out_artifacts, uint32_t *out_count, uint32_t max_count);
ov_status_t ov_efi_installer_validate_payload_safety(const char *proposed_payload_path);

/* USB Recovery Package & Verification */
ov_status_t ov_efi_installer_create_usb_recovery(const char *usb_mount_path, ov_recovery_package_t *out_pkg);
ov_status_t ov_efi_installer_verify_usb_recovery(bool simulate_tamper);
const ov_recovery_package_t* ov_efi_installer_get_recovery_package(void);

/* Dry Run Plan */
ov_status_t ov_efi_installer_generate_dry_run(ov_efi_dry_run_t *out_dry_run);

/* Safety Gate Audit */
ov_status_t ov_efi_installer_evaluate_safety_gates(ov_efi_safety_gates_t *out_gates, bool *out_all_pass);

/* Execution & Post-Install Verification */
ov_status_t ov_efi_installer_execute_install(bool user_final_confirmed, ov_efi_install_report_t *out_report);

/* Rollback Execution & Verification */
ov_status_t ov_efi_installer_execute_rollback(ov_efi_rollback_report_t *out_report);

#ifdef __cplusplus
}
#endif

#endif /* OV_EFI_INSTALLER_H */
