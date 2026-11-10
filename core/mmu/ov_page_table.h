#ifndef OV_PAGE_TABLE_H
#define OV_PAGE_TABLE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../types/ov_types.h"

#define OV_PAGE_SIZE 4096ULL
#define OV_PAGE_MASK (~(OV_PAGE_SIZE - 1))
#define OV_PAGE_SHIFT 12

// 2-Level Page Table Constants
#define OV_PGD_BITS 9
#define OV_PTE_BITS 9
#define OV_PGD_ENTRIES (1 << OV_PGD_BITS) // 512 entries
#define OV_PTE_ENTRIES (1 << OV_PTE_BITS) // 512 entries

#define OV_PGD_SHIFT (OV_PAGE_SHIFT + OV_PTE_BITS) // 21
#define OV_PGD_MASK ((1ULL << OV_PGD_BITS) - 1)
#define OV_PTE_MASK ((1ULL << OV_PTE_BITS) - 1)

// Page Table Entry Flags
#define OV_PTE_VALID       (1U << 0)
#define OV_PTE_READ        (1U << 1)
#define OV_PTE_WRITE       (1U << 2)
#define OV_PTE_EXECUTE     (1U << 3)
#define OV_PTE_USER        (1U << 4) // 0 = Supervisor only, 1 = User accessible
#define OV_PTE_DIRTY       (1U << 5)
#define OV_PTE_ACCESSED    (1U << 6)

typedef struct {
    uint64_t pte_raw; // [63:12] = PFN, [11:0] = flags
} ov_pte_t;

typedef struct {
    ov_pte_t entries[OV_PTE_ENTRIES];
} ov_page_table_level2_t;

typedef struct {
    ov_page_table_level2_t *l2_tables[OV_PGD_ENTRIES];
    uint32_t asid;
    uint32_t generation;
    size_t mapped_pages_count;
} ov_page_table_t;

void ov_pt_init(ov_page_table_t *pt, uint32_t asid);
void ov_pt_destroy(ov_page_table_t *pt);

ov_status_t ov_pt_map(
    ov_page_table_t *pt,
    uint64_t gva,
    uint64_t gpa,
    uint32_t flags
);

ov_status_t ov_pt_unmap(
    ov_page_table_t *pt,
    uint64_t gva
);

ov_status_t ov_pt_walk(
    const ov_page_table_t *pt,
    uint64_t gva,
    uint64_t *out_gpa,
    uint32_t *out_flags
);

#endif /* OV_PAGE_TABLE_H */
