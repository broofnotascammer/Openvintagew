#include "ov_tlb.h"
#include <string.h>

void ov_tlb_init(ov_tlb_t *tlb) {
    if (!tlb) return;
    memset(tlb, 0, sizeof(ov_tlb_t));
}

bool ov_tlb_lookup(
    ov_tlb_t *tlb,
    uint64_t vpn,
    uint32_t asid,
    uint32_t current_generation,
    uint64_t *out_ppn,
    uint32_t *out_flags
) {
    if (!tlb) return false;
    tlb->timer++;

    for (int i = 0; i < OV_TLB_ENTRIES; i++) {
        ov_tlb_entry_t *e = &tlb->entries[i];
        if (e->valid && e->vpn == vpn && (e->asid == asid || e->asid == 0)) {
            // Check generation match to prevent stale translations
            if (e->generation != current_generation) {
                e->valid = false;
                tlb->invalidations++;
                continue;
            }
            e->last_used = tlb->timer;
            if (out_ppn) *out_ppn = e->ppn;
            if (out_flags) *out_flags = e->flags;
            tlb->hits++;
            return true;
        }
    }

    tlb->misses++;
    return false;
}

void ov_tlb_insert(
    ov_tlb_t *tlb,
    uint64_t vpn,
    uint64_t ppn,
    uint32_t asid,
    uint32_t flags,
    uint32_t generation
) {
    if (!tlb) return;
    tlb->timer++;

    int victim_idx = 0;
    uint64_t oldest_time = UINT64_MAX;

    // First try finding an invalid entry
    for (int i = 0; i < OV_TLB_ENTRIES; i++) {
        if (!tlb->entries[i].valid) {
            victim_idx = i;
            break;
        }
        if (tlb->entries[i].last_used < oldest_time) {
            oldest_time = tlb->entries[i].last_used;
            victim_idx = i;
        }
    }

    ov_tlb_entry_t *entry = &tlb->entries[victim_idx];
    entry->vpn = vpn;
    entry->ppn = ppn;
    entry->asid = asid;
    entry->flags = flags;
    entry->generation = generation;
    entry->last_used = tlb->timer;
    entry->valid = true;
}

void ov_tlb_flush_page(
    ov_tlb_t *tlb,
    uint64_t vpn,
    uint32_t asid
) {
    if (!tlb) return;
    for (int i = 0; i < OV_TLB_ENTRIES; i++) {
        if (tlb->entries[i].valid && tlb->entries[i].vpn == vpn &&
            (tlb->entries[i].asid == asid || asid == 0)) {
            tlb->entries[i].valid = false;
            tlb->invalidations++;
        }
    }
}

void ov_tlb_flush_all(
    ov_tlb_t *tlb
) {
    if (!tlb) return;
    for (int i = 0; i < OV_TLB_ENTRIES; i++) {
        tlb->entries[i].valid = false;
    }
    tlb->flushes++;
}
