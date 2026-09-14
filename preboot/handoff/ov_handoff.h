/**
 * OpenVintage Pre-Boot Subsystem - OS Handoff Protocol (Phase 6)
 * Pre-boot to kernel handoff blocks (boot_args, GOP framebuffer, device tree/ACPI pointers).
 */

#ifndef OV_HANDOFF_H
#define OV_HANDOFF_H

#include "ov_types.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t version;
    uint32_t revision;
    char     command_line[256];     /* e.g. "keepsyms=1 debug=0x100 -v ov_perf=max" */
    uint64_t memory_map_addr;
    uint32_t memory_map_size;
    uint32_t memory_map_desc_size;
    uint32_t memory_map_desc_version;

    /* Video Framebuffer */
    uint64_t video_base;
    uint32_t video_width;
    uint32_t video_height;
    uint32_t video_depth;
    uint32_t video_stride;

    /* Active GPU & Performance Configuration Passed to OS */
    uint32_t active_gpu_pci_vendor;
    uint32_t active_gpu_pci_device;
    char     active_perf_profile[32];

    /* ACPI & Device Tree */
    uint64_t acpi_rsdp_ptr;
    uint64_t smbios_ptr;
    uint64_t device_tree_ptr;

    bool     is_handoff_valid;
} ov_boot_handoff_block_t;

ov_status_t ov_handoff_prepare(
    const char *boot_args,
    const char *perf_profile,
    uint32_t active_gpu_vendor,
    uint32_t active_gpu_device,
    ov_boot_handoff_block_t *out_block
);

ov_status_t ov_handoff_validate(const ov_boot_handoff_block_t *block);

#endif /* OV_HANDOFF_H */
