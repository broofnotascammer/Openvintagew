/**
 * OpenVintage Pre-Boot Subsystem - OS Handoff Protocol Implementation (Phase 6)
 */

#include "ov_handoff.h"
#include <string.h>

ov_status_t ov_handoff_prepare(
    const char *boot_args,
    const char *perf_profile,
    uint32_t active_gpu_vendor,
    uint32_t active_gpu_device,
    ov_boot_handoff_block_t *out_block
) {
    if (!out_block) return OV_ERROR_INVALID_PARAM;
    memset(out_block, 0, sizeof(ov_boot_handoff_block_t));

    out_block->version = 1;
    out_block->revision = 0x00060000; /* OpenVintage Phase 6 */

    if (boot_args) {
        strncpy(out_block->command_line, boot_args, sizeof(out_block->command_line) - 1);
    } else {
        strncpy(out_block->command_line, "-v keepsyms=1", sizeof(out_block->command_line) - 1);
    }

    if (perf_profile) {
        strncpy(out_block->active_perf_profile, perf_profile, sizeof(out_block->active_perf_profile) - 1);
    } else {
        strncpy(out_block->active_perf_profile, "Balanced", sizeof(out_block->active_perf_profile) - 1);
    }

    out_block->active_gpu_pci_vendor = active_gpu_vendor;
    out_block->active_gpu_pci_device = active_gpu_device;

    /* Standard high-res pre-boot framebuffer handoff (1920x1080 32bpp) */
    out_block->video_base = 0xE0000000ULL;
    out_block->video_width = 1920;
    out_block->video_height = 1080;
    out_block->video_depth = 32;
    out_block->video_stride = 1920 * 4;

    out_block->memory_map_addr = 0x00500000ULL;
    out_block->memory_map_size = 4096;
    out_block->memory_map_desc_size = 48;
    out_block->memory_map_desc_version = 1;

    out_block->acpi_rsdp_ptr = 0x000F0000ULL;
    out_block->smbios_ptr = 0x000F8000ULL;

    out_block->is_handoff_valid = true;
    return OV_SUCCESS;
}

ov_status_t ov_handoff_validate(const ov_boot_handoff_block_t *block) {
    if (!block) return OV_ERROR_INVALID_PARAM;
    if (!block->is_handoff_valid) return OV_ERROR_INTEGRITY;
    if (block->version != 1) return OV_ERROR_UNSUPPORTED;
    if (block->video_width == 0 || block->video_height == 0) return OV_ERROR_HARDWARE;
    return OV_SUCCESS;
}
