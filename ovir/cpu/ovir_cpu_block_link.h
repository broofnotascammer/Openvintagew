#ifndef OVIR_CPU_BLOCK_LINK_H
#define OVIR_CPU_BLOCK_LINK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../../core/types/ov_types.h"

#define OV_MAX_BLOCK_LINKS 256

typedef struct {
    uint32_t link_id;
    uint32_t source_block_id;
    uint64_t source_guest_exit_pc;
    uint32_t target_block_id;
    uint64_t target_guest_entry_pc;
    bool is_direct;         // direct block transition vs dispatcher
    bool is_conditional;
    bool is_valid;
    uint64_t traversal_count;
    uint32_t generation;
} ov_block_link_t;

typedef struct {
    ov_block_link_t links[OV_MAX_BLOCK_LINKS];
    size_t count;
    uint32_t current_generation;
    uint64_t total_direct_transitions;
    uint64_t total_dispatcher_fallbacks;
} ov_block_link_table_t;

void ov_block_link_table_init(ov_block_link_table_t *tbl);

ov_block_link_t *ov_block_link_create(
    ov_block_link_table_t *tbl,
    uint32_t src_id,
    uint64_t exit_pc,
    uint32_t target_id,
    uint64_t entry_pc,
    bool is_direct
);

ov_block_link_t *ov_block_link_lookup(
    ov_block_link_table_t *tbl,
    uint32_t src_id,
    uint64_t exit_pc
);

ov_status_t ov_block_link_record_traversal(
    ov_block_link_table_t *tbl,
    ov_block_link_t *link
);

ov_status_t ov_block_link_invalidate_block(
    ov_block_link_table_t *tbl,
    uint32_t block_id
);

ov_status_t ov_block_link_invalidate_all(
    ov_block_link_table_t *tbl
);

#endif /* OVIR_CPU_BLOCK_LINK_H */
