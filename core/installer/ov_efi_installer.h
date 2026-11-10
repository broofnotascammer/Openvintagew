#ifndef OV_EFI_INSTALLER_H
#define OV_EFI_INSTALLER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../types/ov_types.h"

#define MASTER_ARTIFACT_COUNT 4

typedef struct {
    const char *filename;
    const char *source_path;
    const char *target_esp_path;
    const char *sha256;
    const char *role;
    bool is_critical;
    bool is_isolated_usb_only;
} ov_efi_artifact_t;

typedef struct {
    bool esp_mount_verified;
    bool nvram_backup_created;
    bool spi_write_disabled;
    bool physical_esp_locked;
    bool usb_staging_active;
    uint32_t safety_gate_pass_count;
} ov_installer_safety_context_t;

typedef struct {
    char target_device_node[64];
    char esp_mount_point[128];
    bool dry_run;
    bool verify_sha256;
    bool backup_existing;
} ov_installation_config_t;

typedef struct {
    ov_efi_artifact_t artifacts[MASTER_ARTIFACT_COUNT];
    size_t artifact_count;
    char target_device[64];
    bool is_dry_run;
    bool safety_verified;
} ov_install_manifest_t;

void ov_installer_init(ov_installer_safety_context_t *ctx);
bool ov_installer_verify_safety_gates(ov_installer_safety_context_t *ctx);
int ov_installer_stage_external_usb(const char *usb_mount_point, char *out_log, size_t log_size);
int ov_installer_dry_run_audit(char *out_report, size_t report_size);
ov_status_t ov_installer_verify_target_disk(const ov_installation_config_t *config);
ov_status_t ov_installer_prepare_manifest(const ov_installation_config_t *config, ov_install_manifest_t *out_manifest);

#endif /* OV_EFI_INSTALLER_H */
