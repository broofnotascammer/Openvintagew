/**
 * OpenVintage Pre-Boot Subsystem - Memory Services (Phase 6)
 * UEFI-compatible memory type maps, page allocators, and runtime descriptors.
 */

#ifndef OV_PREBOOT_MEMORY_H
#define OV_PREBOOT_MEMORY_H

#include "ov_types.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    OV_EFI_RESERVED_MEMORY_TYPE = 0,
    OV_EFI_LOADER_CODE = 1,
    OV_EFI_LOADER_DATA = 2,
    OV_EFI_BOOT_SERVICES_CODE = 3,
    OV_EFI_BOOT_SERVICES_DATA = 4,
    OV_EFI_RUNTIME_SERVICES_CODE = 5,
    OV_EFI_RUNTIME_SERVICES_DATA = 6,
    OV_EFI_CONVENTIONAL_MEMORY = 7,
    OV_EFI_UNUSABLE_MEMORY = 8,
    OV_EFI_ACPI_RECLAIM_MEMORY = 9,
    OV_EFI_ACPI_MEMORY_NVS = 10,
    OV_EFI_MEMORY_MAPPED_IO = 11,
    OV_EFI_PAL_CODE = 12,
    OV_EFI_MAX_MEMORY_TYPE = 13
} ov_efi_memory_type_t;

typedef struct {
    ov_efi_memory_type_t type;
    uint64_t             physical_start;
    uint64_t             virtual_start;
    uint64_t             number_of_pages; /* 4KB pages */
    uint64_t             attribute;
} ov_efi_memory_descriptor_t;

#define OV_MAX_MEMORY_MAP_ENTRIES 128

typedef struct {
    uint32_t                    entry_count;
    ov_efi_memory_descriptor_t  entries[OV_MAX_MEMORY_MAP_ENTRIES];
    uint64_t                    total_conventional_bytes;
    uint64_t                    total_reserved_bytes;
    uint64_t                    total_runtime_bytes;
} ov_preboot_memory_map_t;

ov_status_t ov_preboot_memory_init(uint64_t total_system_ram_bytes);
void        ov_preboot_memory_cleanup(void);

ov_status_t ov_preboot_memory_get_map(ov_preboot_memory_map_t *out_map);
ov_status_t ov_preboot_allocate_pages(uint32_t pages, ov_efi_memory_type_t mem_type, uint64_t *out_addr);
ov_status_t ov_preboot_free_pages(uint64_t addr, uint32_t pages);

#endif /* OV_PREBOOT_MEMORY_H */
