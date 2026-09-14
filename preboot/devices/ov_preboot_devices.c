/**
 * OpenVintage Pre-Boot Subsystem - Hardware Device Enumeration Implementation (Phase 6)
 */

#include "ov_preboot_devices.h"
#include <string.h>
#include <stdio.h>

static ov_preboot_device_table_t s_device_table;
static bool s_initialized = false;

ov_status_t ov_preboot_devices_init(void) {
    memset(&s_device_table, 0, sizeof(s_device_table));

    /* Initialize base architectural device abstractions */
    ov_preboot_device_t *dev = &s_device_table.devices[s_device_table.device_count++];
    dev->device_id = 1;
    dev->type = OV_PREBOOT_DEV_PCI_ROOT_BRIDGE;
    strncpy(dev->device_path, "PciRoot(0x0)", sizeof(dev->device_path) - 1);
    strncpy(dev->description, "Host PCI Express Root Complex", sizeof(dev->description) - 1);
    dev->is_active = true;

    dev = &s_device_table.devices[s_device_table.device_count++];
    dev->device_id = 2;
    dev->type = OV_PREBOOT_DEV_GRAPHICS_OUTPUT;
    strncpy(dev->device_path, "PciRoot(0x0)/Pci(0x2,0x0)", sizeof(dev->device_path) - 1);
    strncpy(dev->description, "EFI Graphics Output Protocol (GOP) Framebuffer", sizeof(dev->description) - 1);
    dev->vendor_id = 0x8086;
    dev->pci_device_id = 0x0166; /* HD 4000 */
    dev->is_active = true;
    s_device_table.gop_display_count++;

    dev = &s_device_table.devices[s_device_table.device_count++];
    dev->device_id = 3;
    dev->type = OV_PREBOOT_DEV_BLOCK_IO;
    strncpy(dev->device_path, "PciRoot(0x0)/Pci(0x1F,0x2)/Sata(0x0,0x0)", sizeof(dev->device_path) - 1);
    strncpy(dev->description, "AHCI SATA Primary Block I/O Controller", sizeof(dev->description) - 1);
    dev->is_boot_device = true;
    dev->is_active = true;
    s_device_table.block_io_count++;

    dev = &s_device_table.devices[s_device_table.device_count++];
    dev->device_id = 4;
    dev->type = OV_PREBOOT_DEV_SIMPLE_FILE_SYSTEM;
    strncpy(dev->device_path, "PciRoot(0x0)/Pci(0x1F,0x2)/Sata(0x0,0x0)/HD(1,GPT)", sizeof(dev->device_path) - 1);
    strncpy(dev->description, "EFI System Partition (FAT32 Volume)", sizeof(dev->description) - 1);
    dev->is_boot_device = true;
    dev->is_active = true;

    dev = &s_device_table.devices[s_device_table.device_count++];
    dev->device_id = 5;
    dev->type = OV_PREBOOT_DEV_NVRAM;
    strncpy(dev->device_path, "AppleEFINvramController", sizeof(dev->device_path) - 1);
    strncpy(dev->description, "Hardware Apple EFI / UEFI Variable Store (NVRAM)", sizeof(dev->description) - 1);
    dev->is_active = true;

    s_initialized = true;
    return OV_SUCCESS;
}

void ov_preboot_devices_cleanup(void) {
    memset(&s_device_table, 0, sizeof(s_device_table));
    s_initialized = false;
}

ov_status_t ov_preboot_devices_enumerate(ov_preboot_device_table_t *out_table) {
    if (!out_table) return OV_ERROR_INVALID_PARAM;
    if (!s_initialized) {
        ov_preboot_devices_init();
    }
    *out_table = s_device_table;
    return OV_SUCCESS;
}

uint32_t ov_preboot_devices_get_count(void) {
    if (!s_initialized) ov_preboot_devices_init();
    return s_device_table.device_count;
}

const ov_preboot_device_t* ov_preboot_devices_get_at(uint32_t index) {
    if (!s_initialized) ov_preboot_devices_init();
    if (index >= s_device_table.device_count) return NULL;
    return &s_device_table.devices[index];
}
