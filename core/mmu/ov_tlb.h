#ifndef OV_TLB_H
#define OV_TLB_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../types/ov_types.h"

#define OV_TLB_ENTRIES 64

typedef struct {
    uint64_t vpn;        // Virtual Page Number (GVA >> 12)
    uint64_t ppn;        // Physical Page Number (GPA >> 12)
    uint32_t asid;       // Address Space ID
    uint32_t flags;      // Permissions & flags
    uint32_t generation; // Page table generation when cached
    uint64_t last_used;
    bool valid;
} ov_tlb_entry_t;

typedef struct {
    ov_tlb_entry_t entries[OV_TLB_ENTRIES];
    uint64_t hits;
    uint64_t misses;
    uint64_t flushes;
    uint64_t invalidations;
    uint64_t timer;
} ov_tlb_t;

void ov_tlb_init(ov_tlb_t *tlb);

bool ov_tlb_lookup(
    ov_tlb_t *tlb,
    uint64_t vpn,
    uint32_t asid,
    uint32_t current_generation,
    uint64_t *out_ppn,
    uint32_t *out_flags
);

void ov_tlb_insert(
    ov_tlb_t *tlb,
    uint64_t vpn,
    uint64_t ppn,
    uint32_t asid,
    uint32_t flags,
    uint32_t generation
);

void ov_tlb_flush_page(
    ov_tlb_t *tlb,
    uint64_t vpn,
    uint32_t asid
);

void ov_tlb_flush_all(
    ov_tlb_t *tlb
);

#endif /* OV_TLB_H */
