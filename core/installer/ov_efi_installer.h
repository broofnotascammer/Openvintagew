#ifndef OV_EFI_INSTALLER_H
#define OV_EFI_INSTALLER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

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

void ov_installer_init(ov_installer_safety_context_t *ctx);
bool ov_installer_verify_safety_gates(ov_installer_safety_context_t *ctx);
int ov_installer_stage_external_usb(const char *usb_mount_point, char *out_log, size_t log_size);
int ov_installer_dry_run_audit(char *out_report, size_t report_size);

#endif /* OV_EFI_INSTALLER_H */
