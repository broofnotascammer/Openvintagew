#ifndef OV_BOOT_PICKER_H
#define OV_BOOT_PICKER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define OV_MAX_BOOT_TARGETS 16
#define OV_MAX_PATH_LEN 512
#define OV_MAX_NAME_LEN 64
#define OV_DEFAULT_TIMEOUT_SEC 5

typedef enum {
    OV_TARGET_MACOS = 0,
    OV_TARGET_OPENCORE,
    OV_TARGET_LINUX,
    OV_TARGET_WINDOWS,
    OV_TARGET_REFIND,
    OV_TARGET_EXTERNAL_USB,
    OV_TARGET_UNKNOWN
} ov_target_type_t;

typedef struct {
    char id[32];
    char name[OV_MAX_NAME_LEN];
    ov_target_type_t type;
    char device_path[OV_MAX_PATH_LEN];
    char esp_relative_path[OV_MAX_PATH_LEN];
    bool is_default;
    bool is_usb;
    bool verified;
    void *device_handle;
} ov_boot_target_t;

typedef struct {
    ov_boot_target_t targets[OV_MAX_BOOT_TARGETS];
    uint32_t count;
    int32_t selected_index;
    int32_t default_index;
    uint32_t timeout_seconds;
    bool timeout_armed;
    bool hal_loaded;
    bool esp_protection_active;
} ov_boot_picker_t;

void ov_boot_picker_init(ov_boot_picker_t *picker);
int ov_boot_picker_add_target(ov_boot_picker_t *picker, const ov_boot_target_t *target);
int ov_boot_picker_enumerate_all(ov_boot_picker_t *picker);
const ov_boot_target_t *ov_boot_picker_get_selected(const ov_boot_picker_t *picker);
int ov_boot_picker_select_by_index(ov_boot_picker_t *picker, uint32_t index);
int ov_boot_picker_render_ui(const ov_boot_picker_t *picker, uint32_t remaining_sec);
int ov_boot_picker_execute_handoff(ov_boot_picker_t *picker, uint32_t target_idx, char *out_error, size_t err_size);

#endif /* OV_BOOT_PICKER_H */
