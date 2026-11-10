#include "ov_page_table.h"
#include <stdlib.h>
#include <string.h>

void ov_pt_init(ov_page_table_t *pt, uint32_t asid) {
    if (!pt) return;
    memset(pt, 0, sizeof(ov_page_table_t));
    pt->asid = asid;
    pt->generation = 1;
}

void ov_pt_destroy(ov_page_table_t *pt) {
    if (!pt) return;
    for (int i = 0; i < OV_PGD_ENTRIES; i++) {
        if (pt->l2_tables[i]) {
            free(pt->l2_tables[i]);
            pt->l2_tables[i] = NULL;
        }
    }
    pt->mapped_pages_count = 0;
    pt->generation++;
}

ov_status_t ov_pt_map(
    ov_page_table_t *pt,
    uint64_t gva,
    uint64_t gpa,
    uint32_t flags
) {
    if (!pt) return OV_ERR_INVALID_PARAM;
    if ((gva & ~OV_PAGE_MASK) != 0 || (gpa & ~OV_PAGE_MASK) != 0) {
        return OV_ERR_INVALID_PARAM; // must be page aligned
    }

    uint32_t pgd_idx = (uint32_t)((gva >> OV_PGD_SHIFT) & OV_PGD_MASK);
    uint32_t pte_idx = (uint32_t)((gva >> OV_PAGE_SHIFT) & OV_PTE_MASK);

    if (!pt->l2_tables[pgd_idx]) {
        ov_page_table_level2_t *l2 = (ov_page_table_level2_t *)calloc(1, sizeof(ov_page_table_level2_t));
        if (!l2) return OV_ERR_OUT_OF_MEMORY;
        pt->l2_tables[pgd_idx] = l2;
    }

    ov_pte_t *entry = &pt->l2_tables[pgd_idx]->entries[pte_idx];
    if (!(entry->pte_raw & OV_PTE_VALID)) {
        pt->mapped_pages_count++;
    }

    entry->pte_raw = (gpa & OV_PAGE_MASK) | (flags & ~OV_PAGE_MASK) | OV_PTE_VALID;
    pt->generation++;

    return OV_SUCCESS;
}

ov_status_t ov_pt_unmap(
    ov_page_table_t *pt,
    uint64_t gva
) {
    if (!pt) return OV_ERR_INVALID_PARAM;
    if ((gva & ~OV_PAGE_MASK) != 0) return OV_ERR_INVALID_PARAM;

    uint32_t pgd_idx = (uint32_t)((gva >> OV_PGD_SHIFT) & OV_PGD_MASK);
    uint32_t pte_idx = (uint32_t)((gva >> OV_PAGE_SHIFT) & OV_PTE_MASK);

    if (!pt->l2_tables[pgd_idx]) {
        return OV_ERR_FAULT; // not mapped
    }

    ov_pte_t *entry = &pt->l2_tables[pgd_idx]->entries[pte_idx];
    if (!(entry->pte_raw & OV_PTE_VALID)) {
        return OV_ERR_FAULT;
    }

    entry->pte_raw = 0;
    if (pt->mapped_pages_count > 0) pt->mapped_pages_count--;
    pt->generation++;

    return OV_SUCCESS;
}

ov_status_t ov_pt_walk(
    const ov_page_table_t *pt,
    uint64_t gva,
    uint64_t *out_gpa,
    uint32_t *out_flags
) {
    if (!pt) return OV_ERR_INVALID_PARAM;

    uint32_t pgd_idx = (uint32_t)((gva >> OV_PGD_SHIFT) & OV_PGD_MASK);
    uint32_t pte_idx = (uint32_t)((gva >> OV_PAGE_SHIFT) & OV_PTE_MASK);
    uint64_t offset = gva & (OV_PAGE_SIZE - 1);

    if (!pt->l2_tables[pgd_idx]) {
        return OV_ERR_FAULT;
    }

    const ov_pte_t *entry = &pt->l2_tables[pgd_idx]->entries[pte_idx];
    if (!(entry->pte_raw & OV_PTE_VALID)) {
        return OV_ERR_FAULT;
    }

    uint64_t base_gpa = entry->pte_raw & OV_PAGE_MASK;
    uint32_t flags = (uint32_t)(entry->pte_raw & ~OV_PAGE_MASK);

    if (out_gpa) *out_gpa = base_gpa + offset;
    if (out_flags) *out_flags = flags;

    return OV_SUCCESS;
}
