/**
 * OpenVintage Pre-Boot Subsystem - Hardware Device Enumeration (Phase 6)
 * Real pre-boot abstractions for EFI handles, PCI device paths, and protocol interfaces.
 */

#ifndef OV_PREBOOT_DEVICES_H
#define OV_PREBOOT_DEVICES_H

#include "ov_types.h"
#include <stdint.h>
#include <stdbool.h>

#define OV_MAX_PREBOOT_DEVICES 64

typedef enum {
    OV_PREBOOT_DEV_PCI_ROOT_BRIDGE = 0,
    OV_PREBOOT_DEV_PCI_HOST = 1,
    OV_PREBOOT_DEV_GRAPHICS_OUTPUT = 2,
    OV_PREBOOT_DEV_BLOCK_IO = 3,
    OV_PREBOOT_DEV_SIMPLE_FILE_SYSTEM = 4,
    OV_PREBOOT_DEV_NVRAM = 5,
    OV_PREBOOT_DEV_TIMER = 6,
    OV_PREBOOT_DEV_UNKNOWN = 7
} ov_preboot_dev_type_t;

typedef struct {
    uint32_t              device_id;
    ov_preboot_dev_type_t type;
    char                  device_path[256];
    char                  description[128];
    uint16_t              vendor_id;
    uint16_t              pci_device_id;
    bool                  is_boot_device;
    bool                  is_active;
} ov_preboot_device_t;

typedef struct {
    uint32_t             device_count;
    ov_preboot_device_t  devices[OV_MAX_PREBOOT_DEVICES];
    uint32_t             block_io_count;
    uint32_t             gop_display_count;
} ov_preboot_device_table_t;

ov_status_t ov_preboot_devices_init(void);
void        ov_preboot_devices_cleanup(void);

ov_status_t ov_preboot_devices_enumerate(ov_preboot_device_table_t *out_table);
uint32_t    ov_preboot_devices_get_count(void);
const ov_preboot_device_t* ov_preboot_devices_get_at(uint32_t index);

#endif /* OV_PREBOOT_DEVICES_H */
