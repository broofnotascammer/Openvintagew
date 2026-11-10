#include "ov_mmu.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

const char *ov_mmu_fault_name(ov_mmu_fault_type_t fault) {
    switch (fault) {
        case OV_MMU_FAULT_NONE: return "NONE";
        case OV_MMU_FAULT_TRANSLATION: return "TRANSLATION_FAULT";
        case OV_MMU_FAULT_PERMISSION: return "PERMISSION_FAULT";
        case OV_MMU_FAULT_ALIGNMENT: return "ALIGNMENT_FAULT";
        case OV_MMU_FAULT_PRIVILEGE: return "PRIVILEGE_FAULT";
        default: return "UNKNOWN_FAULT";
    }
}

ov_status_t ov_mmu_init(ov_mmu_t *mmu, uint32_t asid, size_t phys_ram_size) {
    if (!mmu) return OV_ERR_INVALID_PARAM;
    memset(mmu, 0, sizeof(ov_mmu_t));

    mmu->asid = asid;
    mmu->privilege_ring = OV_RING_USER;
    ov_pt_init(&mmu->page_table, asid);
    ov_tlb_init(&mmu->tlb);

    if (phys_ram_size > 0) {
        mmu->phys_backing = (uint8_t *)calloc(1, phys_ram_size);
        if (!mmu->phys_backing) {
            return OV_ERR_OUT_OF_MEMORY;
        }
        mmu->phys_backing_size = phys_ram_size;
    }

    return OV_SUCCESS;
}

void ov_mmu_destroy(ov_mmu_t *mmu) {
    if (!mmu) return;
    ov_pt_destroy(&mmu->page_table);
    ov_tlb_flush_all(&mmu->tlb);
    if (mmu->phys_backing) {
        free(mmu->phys_backing);
        mmu->phys_backing = NULL;
    }
    mmu->phys_backing_size = 0;
}

void ov_mmu_set_privilege(ov_mmu_t *mmu, ov_privilege_ring_t ring) {
    if (mmu) {
        mmu->privilege_ring = ring;
    }
}

ov_status_t ov_mmu_map_page(
    ov_mmu_t *mmu,
    uint64_t gva,
    uint64_t gpa,
    uint32_t flags
) {
    if (!mmu) return OV_ERR_INVALID_PARAM;
    ov_status_t st = ov_pt_map(&mmu->page_table, gva, gpa, flags);
    if (st == OV_SUCCESS) {
        // Flush page from TLB to invalidate old translation
        ov_tlb_flush_page(&mmu->tlb, gva >> OV_PAGE_SHIFT, mmu->asid);
    }
    return st;
}

ov_status_t ov_mmu_unmap_page(
    ov_mmu_t *mmu,
    uint64_t gva
) {
    if (!mmu) return OV_ERR_INVALID_PARAM;
    ov_status_t st = ov_pt_unmap(&mmu->page_table, gva);
    if (st == OV_SUCCESS) {
        ov_tlb_flush_page(&mmu->tlb, gva >> OV_PAGE_SHIFT, mmu->asid);
    }
    return st;
}

ov_status_t ov_mmu_translate(
    ov_mmu_t *mmu,
    uint64_t gva,
    ov_mmu_access_type_t access,
    uint64_t *out_gpa,
    ov_mmu_fault_info_t *out_fault
) {
    if (!mmu) return OV_ERR_INVALID_PARAM;

    mmu->total_translations++;
    uint64_t vpn = gva >> OV_PAGE_SHIFT;
    uint64_t offset = gva & (OV_PAGE_SIZE - 1);
    uint64_t ppn = 0;
    uint32_t flags = 0;

    // 1. Check TLB
    bool tlb_hit = ov_tlb_lookup(&mmu->tlb, vpn, mmu->asid, mmu->page_table.generation, &ppn, &flags);

    // 2. If TLB miss, perform page table walk
    if (!tlb_hit) {
        uint64_t walked_gpa = 0;
        ov_status_t walk_st = ov_pt_walk(&mmu->page_table, gva, &walked_gpa, &flags);
        if (walk_st != OV_SUCCESS) {
            mmu->page_faults++;
            if (out_fault) {
                out_fault->fault_type = OV_MMU_FAULT_TRANSLATION;
                out_fault->fault_addr = gva;
                out_fault->access_type = access;
                out_fault->asid = mmu->asid;
                snprintf(out_fault->description, sizeof(out_fault->description),
                         "Unmapped guest virtual address 0x%llx", (unsigned long long)gva);
            }
            return OV_ERR_FAULT;
        }
        ppn = walked_gpa >> OV_PAGE_SHIFT;
        // Insert into TLB
        ov_tlb_insert(&mmu->tlb, vpn, ppn, mmu->asid, flags, mmu->page_table.generation);
    }

    // 3. Permission checks
    if ((access & OV_MMU_ACCESS_WRITE) && !(flags & OV_PTE_WRITE)) {
        mmu->permission_faults++;
        if (out_fault) {
            out_fault->fault_type = OV_MMU_FAULT_PERMISSION;
            out_fault->fault_addr = gva;
            out_fault->access_type = access;
            out_fault->asid = mmu->asid;
            snprintf(out_fault->description, sizeof(out_fault->description),
                     "Write permission denied on page 0x%llx", (unsigned long long)(vpn << OV_PAGE_SHIFT));
        }
        return OV_ERR_SECURITY_VIOLATION;
    }

    if ((access & OV_MMU_ACCESS_EXEC) && !(flags & OV_PTE_EXECUTE)) {
        mmu->permission_faults++;
        if (out_fault) {
            out_fault->fault_type = OV_MMU_FAULT_PERMISSION;
            out_fault->fault_addr = gva;
            out_fault->access_type = access;
            out_fault->asid = mmu->asid;
            snprintf(out_fault->description, sizeof(out_fault->description),
                     "Execute permission denied on page 0x%llx (W^X / No-Execute)", (unsigned long long)(vpn << OV_PAGE_SHIFT));
        }
        return OV_ERR_SECURITY_VIOLATION;
    }

    // 4. Privilege check: User mode accessing supervisor page
    if (mmu->privilege_ring == OV_RING_USER && !(flags & OV_PTE_USER)) {
        mmu->privilege_faults++;
        if (out_fault) {
            out_fault->fault_type = OV_MMU_FAULT_PRIVILEGE;
            out_fault->fault_addr = gva;
            out_fault->access_type = access;
            out_fault->asid = mmu->asid;
            snprintf(out_fault->description, sizeof(out_fault->description),
                     "Privilege violation: User mode access to supervisor page 0x%llx", (unsigned long long)(vpn << OV_PAGE_SHIFT));
        }
        return OV_ERR_SECURITY_VIOLATION;
    }

    uint64_t final_gpa = (ppn << OV_PAGE_SHIFT) | offset;
    if (out_gpa) *out_gpa = final_gpa;
    if (out_fault) out_fault->fault_type = OV_MMU_FAULT_NONE;

    return OV_SUCCESS;
}

ov_status_t ov_mmu_read(
    ov_mmu_t *mmu,
    uint64_t gva,
    void *buf,
    size_t size,
    ov_mmu_fault_info_t *out_fault
) {
    if (!mmu || !buf || size == 0) return OV_ERR_INVALID_PARAM;

    uint64_t gpa = 0;
    ov_status_t st = ov_mmu_translate(mmu, gva, OV_MMU_ACCESS_READ, &gpa, out_fault);
    if (st != OV_SUCCESS) return st;

    if (mmu->phys_backing) {
        if (gpa + size > mmu->phys_backing_size) {
            if (out_fault) {
                out_fault->fault_type = OV_MMU_FAULT_TRANSLATION;
                out_fault->fault_addr = gva;
                out_fault->access_type = OV_MMU_ACCESS_READ;
                snprintf(out_fault->description, sizeof(out_fault->description),
                         "GPA 0x%llx out of physical memory bounds", (unsigned long long)gpa);
            }
            return OV_ERR_FAULT;
        }
        memcpy(buf, &mmu->phys_backing[gpa], size);
    } else {
        memset(buf, 0, size);
    }

    return OV_SUCCESS;
}

ov_status_t ov_mmu_write(
    ov_mmu_t *mmu,
    uint64_t gva,
    const void *buf,
    size_t size,
    ov_mmu_fault_info_t *out_fault
) {
    if (!mmu || !buf || size == 0) return OV_ERR_INVALID_PARAM;

    uint64_t gpa = 0;
    ov_status_t st = ov_mmu_translate(mmu, gva, OV_MMU_ACCESS_WRITE, &gpa, out_fault);
    if (st != OV_SUCCESS) return st;

    if (mmu->phys_backing) {
        if (gpa + size > mmu->phys_backing_size) {
            if (out_fault) {
                out_fault->fault_type = OV_MMU_FAULT_TRANSLATION;
                out_fault->fault_addr = gva;
                out_fault->access_type = OV_MMU_ACCESS_WRITE;
                snprintf(out_fault->description, sizeof(out_fault->description),
                         "GPA 0x%llx out of physical memory bounds", (unsigned long long)gpa);
            }
            return OV_ERR_FAULT;
        }
        memcpy(&mmu->phys_backing[gpa], buf, size);
    }

    return OV_SUCCESS;
}
