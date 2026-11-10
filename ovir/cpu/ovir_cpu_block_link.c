#include "ovir_cpu_block_link.h"
#include <string.h>

void ov_block_link_table_init(ov_block_link_table_t *tbl) {
    if (!tbl) return;
    memset(tbl, 0, sizeof(ov_block_link_table_t));
    tbl->current_generation = 1;
}

ov_block_link_t *ov_block_link_create(
    ov_block_link_table_t *tbl,
    uint32_t src_id,
    uint64_t exit_pc,
    uint32_t target_id,
    uint64_t entry_pc,
    bool is_direct
) {
    if (!tbl) return NULL;

    // Check if link already exists
    for (size_t i = 0; i < tbl->count; i++) {
        if (tbl->links[i].source_block_id == src_id &&
            tbl->links[i].source_guest_exit_pc == exit_pc &&
            tbl->links[i].is_valid) {
            tbl->links[i].target_block_id = target_id;
            tbl->links[i].target_guest_entry_pc = entry_pc;
            tbl->links[i].is_direct = is_direct;
            return &tbl->links[i];
        }
    }

    if (tbl->count >= OV_MAX_BLOCK_LINKS) {
        // Reuse an invalid slot if available
        for (size_t i = 0; i < tbl->count; i++) {
            if (!tbl->links[i].is_valid) {
                ov_block_link_t *link = &tbl->links[i];
                link->source_block_id = src_id;
                link->source_guest_exit_pc = exit_pc;
                link->target_block_id = target_id;
                link->target_guest_entry_pc = entry_pc;
                link->is_direct = is_direct;
                link->is_valid = true;
                link->traversal_count = 0;
                link->generation = tbl->current_generation;
                return link;
            }
        }
        return NULL; // table exhausted
    }

    ov_block_link_t *link = &tbl->links[tbl->count];
    link->link_id = (uint32_t)(tbl->count + 1);
    link->source_block_id = src_id;
    link->source_guest_exit_pc = exit_pc;
    link->target_block_id = target_id;
    link->target_guest_entry_pc = entry_pc;
    link->is_direct = is_direct;
    link->is_valid = true;
    link->traversal_count = 0;
    link->generation = tbl->current_generation;
    tbl->count++;

    return link;
}

ov_block_link_t *ov_block_link_lookup(
    ov_block_link_table_t *tbl,
    uint32_t src_id,
    uint64_t exit_pc
) {
    if (!tbl) return NULL;
    for (size_t i = 0; i < tbl->count; i++) {
        if (tbl->links[i].is_valid &&
            tbl->links[i].source_block_id == src_id &&
            tbl->links[i].source_guest_exit_pc == exit_pc) {
            return &tbl->links[i];
        }
    }
    return NULL;
}

ov_status_t ov_block_link_record_traversal(
    ov_block_link_table_t *tbl,
    ov_block_link_t *link
) {
    if (!tbl || !link || !link->is_valid) return OV_ERR_INVALID_PARAM;
    link->traversal_count++;
    if (link->is_direct) {
        tbl->total_direct_transitions++;
    } else {
        tbl->total_dispatcher_fallbacks++;
    }
    return OV_SUCCESS;
}

ov_status_t ov_block_link_invalidate_block(
    ov_block_link_table_t *tbl,
    uint32_t block_id
) {
    if (!tbl) return OV_ERR_INVALID_PARAM;
    for (size_t i = 0; i < tbl->count; i++) {
        if (tbl->links[i].is_valid &&
            (tbl->links[i].source_block_id == block_id ||
             tbl->links[i].target_block_id == block_id)) {
            tbl->links[i].is_valid = false;
        }
    }
    return OV_SUCCESS;
}

ov_status_t ov_block_link_invalidate_all(
    ov_block_link_table_t *tbl
) {
    if (!tbl) return OV_ERR_INVALID_PARAM;
    for (size_t i = 0; i < tbl->count; i++) {
        tbl->links[i].is_valid = false;
    }
    tbl->current_generation++;
    return OV_SUCCESS;
}
