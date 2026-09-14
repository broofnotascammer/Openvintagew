/**
 * OpenVintage Pre-Boot Subsystem - Memory Services Implementation (Phase 6)
 */

#include "ov_preboot_memory.h"
#include <string.h>
#include <stdlib.h>

static ov_preboot_memory_map_t s_mem_map;
static uint64_t s_next_alloc_addr = 0x40000000ULL; /* 1 GB boundary base for preboot allocs */
static bool s_initialized = false;

ov_status_t ov_preboot_memory_init(uint64_t total_system_ram_bytes) {
    if (total_system_ram_bytes == 0) {
        total_system_ram_bytes = 8ULL * 1024 * 1024 * 1024; /* 8 GB default */
    }

    memset(&s_mem_map, 0, sizeof(s_mem_map));
    s_next_alloc_addr = 0x40000000ULL;

    /* Entry 0: Low Memory (Real Mode IVT / BIOS reserved) 0x00000000 - 0x000A0000 (640KB) */
    s_mem_map.entries[0].type = OV_EFI_RESERVED_MEMORY_TYPE;
    s_mem_map.entries[0].physical_start = 0x00000000ULL;
    s_mem_map.entries[0].number_of_pages = 160; /* 640 KB */
    s_mem_map.entries[0].attribute = 0x1;

    /* Entry 1: OpenVintage Pre-Boot Loader Code 0x00100000 - 0x00500000 (4MB) */
    s_mem_map.entries[1].type = OV_EFI_LOADER_CODE;
    s_mem_map.entries[1].physical_start = 0x00100000ULL;
    s_mem_map.entries[1].number_of_pages = 1024; /* 4 MB */
    s_mem_map.entries[1].attribute = 0x8;

    /* Entry 2: OpenVintage Boot Services Data 0x00500000 - 0x02000000 (27MB) */
    s_mem_map.entries[2].type = OV_EFI_BOOT_SERVICES_DATA;
    s_mem_map.entries[2].physical_start = 0x00500000ULL;
    s_mem_map.entries[2].number_of_pages = 6912;
    s_mem_map.entries[2].attribute = 0x8;

    /* Entry 3: Conventional High Memory */
    uint64_t conv_start = 0x02000000ULL;
    uint64_t conv_bytes = (total_system_ram_bytes > conv_start) ? (total_system_ram_bytes - conv_start - 0x10000000ULL) : 0x20000000ULL;
    s_mem_map.entries[3].type = OV_EFI_CONVENTIONAL_MEMORY;
    s_mem_map.entries[3].physical_start = conv_start;
    s_mem_map.entries[3].number_of_pages = conv_bytes / 4096;
    s_mem_map.entries[3].attribute = 0xF;

    /* Entry 4: ACPI Tables & NVS */
    s_mem_map.entries[4].type = OV_EFI_ACPI_RECLAIM_MEMORY;
    s_mem_map.entries[4].physical_start = total_system_ram_bytes - 0x04000000ULL;
    s_mem_map.entries[4].number_of_pages = 1024;
    s_mem_map.entries[4].attribute = 0x1;

    /* Entry 5: PCI MMIO Space */
    s_mem_map.entries[5].type = OV_EFI_MEMORY_MAPPED_IO;
    s_mem_map.entries[5].physical_start = 0xF0000000ULL;
    s_mem_map.entries[5].number_of_pages = 65536; /* 256 MB */
    s_mem_map.entries[5].attribute = 0x1;

    s_mem_map.entry_count = 6;
    s_mem_map.total_conventional_bytes = conv_bytes;
    s_mem_map.total_reserved_bytes = 640 * 1024 + 4 * 1024 * 1024 + 27 * 1024 * 1024;
    s_mem_map.total_runtime_bytes = 4 * 1024 * 1024;

    s_initialized = true;
    return OV_SUCCESS;
}

void ov_preboot_memory_cleanup(void) {
    memset(&s_mem_map, 0, sizeof(s_mem_map));
    s_initialized = false;
}

ov_status_t ov_preboot_memory_get_map(ov_preboot_memory_map_t *out_map) {
    if (!out_map) return OV_ERROR_INVALID_PARAM;
    if (!s_initialized) ov_preboot_memory_init(0);
    *out_map = s_mem_map;
    return OV_SUCCESS;
}

ov_status_t ov_preboot_allocate_pages(uint32_t pages, ov_efi_memory_type_t mem_type, uint64_t *out_addr) {
    if (!out_addr || pages == 0) return OV_ERROR_INVALID_PARAM;
    if (!s_initialized) ov_preboot_memory_init(0);

    if (s_mem_map.entry_count >= OV_MAX_MEMORY_MAP_ENTRIES) {
        return OV_ERROR_OUT_OF_RESOURCES;
    }

    uint64_t alloc_addr = s_next_alloc_addr;
    uint64_t alloc_bytes = (uint64_t)pages * 4096;
    s_next_alloc_addr += alloc_bytes;

    ov_efi_memory_descriptor_t *entry = &s_mem_map.entries[s_mem_map.entry_count++];
    entry->type = mem_type;
    entry->physical_start = alloc_addr;
    entry->virtual_start = alloc_addr;
    entry->number_of_pages = pages;
    entry->attribute = 0xF;

    *out_addr = alloc_addr;
    return OV_SUCCESS;
}

ov_status_t ov_preboot_free_pages(uint64_t addr, uint32_t pages) {
    (void)addr;
    (void)pages;
    /* In preboot context, pages are freed on exit boot services */
    return OV_SUCCESS;
}
