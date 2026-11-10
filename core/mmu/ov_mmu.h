#ifndef OV_MMU_H
#define OV_MMU_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../types/ov_types.h"
#include "../runtime/ov_os_scale.h"
#include "ov_page_table.h"
#include "ov_tlb.h"

typedef enum {
    OV_MMU_FAULT_NONE = 0,
    OV_MMU_FAULT_TRANSLATION, // Unmapped address
    OV_MMU_FAULT_PERMISSION,  // Read-only write or non-exec fetch
    OV_MMU_FAULT_ALIGNMENT,   // Unaligned access
    OV_MMU_FAULT_PRIVILEGE    // User mode access to supervisor-only page
} ov_mmu_fault_type_t;

typedef enum {
    OV_MMU_ACCESS_READ  = (1 << 0),
    OV_MMU_ACCESS_WRITE = (1 << 1),
    OV_MMU_ACCESS_EXEC  = (1 << 2)
} ov_mmu_access_type_t;

typedef struct {
    ov_mmu_fault_type_t fault_type;
    uint64_t fault_addr;
    ov_mmu_access_type_t access_type;
    uint32_t asid;
    char description[96];
} ov_mmu_fault_info_t;

typedef struct {
    ov_page_table_t page_table;
    ov_tlb_t tlb;
    uint32_t asid;
    ov_privilege_ring_t privilege_ring;
    uint8_t *phys_backing;
    size_t phys_backing_size;
    uint64_t total_translations;
    uint64_t page_faults;
    uint64_t permission_faults;
    uint64_t privilege_faults;
} ov_mmu_t;

ov_status_t ov_mmu_init(ov_mmu_t *mmu, uint32_t asid, size_t phys_ram_size);
void ov_mmu_destroy(ov_mmu_t *mmu);
void ov_mmu_set_privilege(ov_mmu_t *mmu, ov_privilege_ring_t ring);

ov_status_t ov_mmu_map_page(
    ov_mmu_t *mmu,
    uint64_t gva,
    uint64_t gpa,
    uint32_t flags
);

ov_status_t ov_mmu_unmap_page(
    ov_mmu_t *mmu,
    uint64_t gva
);

ov_status_t ov_mmu_translate(
    ov_mmu_t *mmu,
    uint64_t gva,
    ov_mmu_access_type_t access,
    uint64_t *out_gpa,
    ov_mmu_fault_info_t *out_fault
);

ov_status_t ov_mmu_read(
    ov_mmu_t *mmu,
    uint64_t gva,
    void *buf,
    size_t size,
    ov_mmu_fault_info_t *out_fault
);

ov_status_t ov_mmu_write(
    ov_mmu_t *mmu,
    uint64_t gva,
    const void *buf,
    size_t size,
    ov_mmu_fault_info_t *out_fault
);

const char *ov_mmu_fault_name(ov_mmu_fault_type_t fault);

#endif /* OV_MMU_H */
