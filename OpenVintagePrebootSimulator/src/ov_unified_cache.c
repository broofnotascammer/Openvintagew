/**
 * OpenVintage Pre-Boot Simulator - Unified Cache Subsystem Implementation (Phase 5)
 * Functional key-value multi-tier cache with LRU eviction and empirical telemetry.
 */

#include "ov_unified_cache.h"
#include "ov_memory.h"
#include "ov_logger.h"
#include <string.h>
#include <time.h>

typedef struct {
    uint64_t key_hash;
    uint8_t  data[OV_CACHE_MAX_PAYLOAD_BYTES];
    size_t   size;
    uint64_t access_counter;
    uint32_t access_count;
    bool     is_valid;
} ov_cache_slot_t;

typedef struct {
    ov_cache_slot_t slots[OV_CACHE_MAX_ENTRIES_PER_TIER];
    uint32_t        entry_count;
    uint64_t        committed_bytes;
    uint64_t        hits;
    uint64_t        misses;
    uint64_t        evictions;
} ov_cache_tier_state_t;

static ov_cache_tier_state_t g_tiers[4];
static uint64_t              g_lru_clock = 1;
static uint32_t              g_generation = 1;

static int tier_to_index(ov_cache_tier_t tier) {
    switch (tier) {
        case OV_CACHE_TIER_CPU_TRANSLATION: return 0;
        case OV_CACHE_TIER_SHADER:          return 1;
        case OV_CACHE_TIER_PIPELINE:        return 2;
        case OV_CACHE_TIER_COMPATIBILITY:   return 3;
        default:                            return -1;
    }
}

ov_status_t ov_unified_cache_init(void) {
    memset(g_tiers, 0, sizeof(g_tiers));
    g_lru_clock = 1;
    g_generation = 1;
    ov_log_info("Phase 5 Unified Cache Subsystem initialized (4 functional storage tiers ready)");
    return OV_SUCCESS;
}

void ov_unified_cache_cleanup(void) {
    memset(g_tiers, 0, sizeof(g_tiers));
    ov_log_info("Unified cache subsystem cleaned up (All entries evicted)");
}

ov_status_t ov_unified_cache_evict_lru(ov_cache_tier_t tier) {
    int idx = tier_to_index(tier);
    if (idx < 0) return OV_ERROR_INVALID_PARAM;

    ov_cache_tier_state_t *t = &g_tiers[idx];
    if (t->entry_count == 0) return OV_SUCCESS;

    uint64_t oldest_clock = UINT64_MAX;
    int lru_slot = -1;
    for (uint32_t i = 0; i < OV_CACHE_MAX_ENTRIES_PER_TIER; i++) {
        if (t->slots[i].is_valid && t->slots[i].access_counter < oldest_clock) {
            oldest_clock = t->slots[i].access_counter;
            lru_slot = (int)i;
        }
    }

    if (lru_slot >= 0) {
        t->committed_bytes -= t->slots[lru_slot].size;
        t->slots[lru_slot].is_valid = false;
        t->entry_count--;
        t->evictions++;
    }

    return OV_SUCCESS;
}

ov_status_t ov_unified_cache_store(ov_cache_tier_t tier, uint64_t key_hash, const void *data, size_t size) {
    int idx = tier_to_index(tier);
    if (idx < 0 || !data || size == 0 || size > OV_CACHE_MAX_PAYLOAD_BYTES) {
        return OV_ERROR_INVALID_PARAM;
    }

    ov_cache_tier_state_t *t = &g_tiers[idx];

    /* Check if key already exists, overwrite if so */
    for (uint32_t i = 0; i < OV_CACHE_MAX_ENTRIES_PER_TIER; i++) {
        if (t->slots[i].is_valid && t->slots[i].key_hash == key_hash) {
            t->committed_bytes -= t->slots[i].size;
            memcpy(t->slots[i].data, data, size);
            t->slots[i].size = size;
            t->slots[i].access_counter = ++g_lru_clock;
            t->committed_bytes += size;
            return OV_SUCCESS;
        }
    }

    /* Find an empty slot */
    int target_slot = -1;
    for (uint32_t i = 0; i < OV_CACHE_MAX_ENTRIES_PER_TIER; i++) {
        if (!t->slots[i].is_valid) {
            target_slot = (int)i;
            break;
        }
    }

    /* If full, evict least recently used slot */
    if (target_slot == -1) {
        ov_unified_cache_evict_lru(tier);
        for (uint32_t i = 0; i < OV_CACHE_MAX_ENTRIES_PER_TIER; i++) {
            if (!t->slots[i].is_valid) {
                target_slot = (int)i;
                break;
            }
        }
        if (target_slot == -1) target_slot = 0;
    }

    /* Insert */
    ov_cache_slot_t *slot = &t->slots[target_slot];
    slot->key_hash = key_hash;
    memcpy(slot->data, data, size);
    slot->size = size;
    slot->access_counter = ++g_lru_clock;
    slot->access_count = 1;
    slot->is_valid = true;

    t->entry_count++;
    t->committed_bytes += size;

    return OV_SUCCESS;
}

ov_status_t ov_unified_cache_lookup(ov_cache_tier_t tier, uint64_t key_hash, void *out_data, size_t max_size, size_t *out_size) {
    int idx = tier_to_index(tier);
    if (idx < 0) return OV_ERROR_INVALID_PARAM;

    ov_cache_tier_state_t *t = &g_tiers[idx];

    for (uint32_t i = 0; i < OV_CACHE_MAX_ENTRIES_PER_TIER; i++) {
        if (t->slots[i].is_valid && t->slots[i].key_hash == key_hash) {
            /* Hit */
            t->slots[i].access_counter = ++g_lru_clock;
            t->slots[i].access_count++;
            t->hits++;

            if (out_data && max_size > 0) {
                size_t copy_bytes = (t->slots[i].size < max_size) ? t->slots[i].size : max_size;
                memcpy(out_data, t->slots[i].data, copy_bytes);
            }
            if (out_size) {
                *out_size = t->slots[i].size;
            }
            return OV_SUCCESS;
        }
    }

    /* Miss */
    t->misses++;
    return OV_ERROR_NOT_FOUND;
}

ov_status_t ov_unified_cache_clear_tier(ov_cache_tier_t tier) {
    int idx = tier_to_index(tier);
    if (idx < 0) return OV_ERROR_INVALID_PARAM;

    memset(&g_tiers[idx], 0, sizeof(ov_cache_tier_state_t));
    return OV_SUCCESS;
}

uint32_t ov_unified_cache_get_generation(void) {
    return g_generation;
}

void ov_unified_cache_record_query(ov_cache_tier_t tier, bool hit, uint64_t bytes) {
    (void)bytes;
    int idx = tier_to_index(tier);
    if (idx >= 0) {
        if (hit) g_tiers[idx].hits++;
        else g_tiers[idx].misses++;
    }
}

ov_status_t ov_unified_cache_verify_integrity(void) {
    for (int i = 0; i < 4; i++) {
        uint32_t count = 0;
        uint64_t bytes = 0;
        for (uint32_t s = 0; s < OV_CACHE_MAX_ENTRIES_PER_TIER; s++) {
            if (g_tiers[i].slots[s].is_valid) {
                count++;
                bytes += g_tiers[i].slots[s].size;
            }
        }
        if (count != g_tiers[i].entry_count || bytes != g_tiers[i].committed_bytes) {
            return OV_ERROR_INTEGRITY;
        }
    }
    return OV_SUCCESS;
}

ov_status_t ov_unified_cache_get_stats(ov_unified_cache_stats_t *out_stats) {
    if (!out_stats) return OV_ERROR_INVALID_PARAM;
    memset(out_stats, 0, sizeof(ov_unified_cache_stats_t));

    out_stats->current_generation = g_generation;

    out_stats->cpu_cache_entries = g_tiers[0].entry_count;
    out_stats->cpu_cache_bytes   = g_tiers[0].committed_bytes;
    out_stats->cpu_cache_hits    = g_tiers[0].hits;
    out_stats->cpu_cache_misses  = g_tiers[0].misses;

    out_stats->shader_cache_entries = g_tiers[1].entry_count;
    out_stats->shader_cache_bytes   = g_tiers[1].committed_bytes;
    out_stats->shader_cache_hits    = g_tiers[1].hits;
    out_stats->shader_cache_misses  = g_tiers[1].misses;

    out_stats->pipeline_cache_entries = g_tiers[2].entry_count;
    out_stats->pipeline_cache_bytes   = g_tiers[2].committed_bytes;
    out_stats->pipeline_cache_hits    = g_tiers[2].hits;
    out_stats->pipeline_cache_misses  = g_tiers[2].misses;

    out_stats->compat_cache_entries = g_tiers[3].entry_count;
    out_stats->compat_cache_bytes   = g_tiers[3].committed_bytes;
    out_stats->compat_cache_hits    = g_tiers[3].hits;
    out_stats->compat_cache_misses  = g_tiers[3].misses;

    out_stats->total_hits = out_stats->cpu_cache_hits + out_stats->shader_cache_hits +
                            out_stats->pipeline_cache_hits + out_stats->compat_cache_hits;
    out_stats->total_misses = out_stats->cpu_cache_misses + out_stats->shader_cache_misses +
                              out_stats->pipeline_cache_misses + out_stats->compat_cache_misses;
    out_stats->total_queries = out_stats->total_hits + out_stats->total_misses;

    if (out_stats->total_queries > 0) {
        out_stats->overall_hit_rate_percent = (uint32_t)((out_stats->total_hits * 100) / out_stats->total_queries);
    } else {
        out_stats->overall_hit_rate_percent = 0;
    }

    return OV_SUCCESS;
}

ov_status_t ov_unified_cache_invalidate(ov_cache_tier_t tier, ov_invalidate_reason_t reason) {
    g_generation++;

    if (tier == OV_CACHE_TIER_ALL) {
        for (int i = 0; i < 4; i++) {
            memset(&g_tiers[i].slots, 0, sizeof(g_tiers[i].slots));
            g_tiers[i].entry_count = 0;
            g_tiers[i].committed_bytes = 0;
        }
        ov_log_info("Unified Cache: Invalidated ALL tiers (Reason: %s, Generation -> %u)",
                    ov_invalidate_reason_to_string(reason), g_generation);
        return OV_SUCCESS;
    }

    int idx = tier_to_index(tier);
    if (idx >= 0) {
        memset(&g_tiers[idx].slots, 0, sizeof(g_tiers[idx].slots));
        g_tiers[idx].entry_count = 0;
        g_tiers[idx].committed_bytes = 0;
        ov_log_info("Unified Cache: Invalidated tier %s (Reason: %s)",
                    ov_cache_tier_to_string(tier), ov_invalidate_reason_to_string(reason));
        return OV_SUCCESS;
    }

    return OV_ERROR_INVALID_PARAM;
}

const char* ov_cache_tier_to_string(ov_cache_tier_t tier) {
    switch (tier) {
        case OV_CACHE_TIER_CPU_TRANSLATION: return "CPU Translation Cache";
        case OV_CACHE_TIER_SHADER:          return "GPU Shader Cache";
        case OV_CACHE_TIER_PIPELINE:        return "Pipeline State Cache";
        case OV_CACHE_TIER_COMPATIBILITY:   return "Compatibility DB Cache";
        case OV_CACHE_TIER_ALL:             return "All Unified Tiers";
        default:                            return "Unknown Tier";
    }
}

const char* ov_invalidate_reason_to_string(ov_invalidate_reason_t reason) {
    switch (reason) {
        case OV_INVALIDATE_MANUAL:             return "Manual User Request";
        case OV_INVALIDATE_VERSION_CHANGE:     return "OS or Subsystem Version Changed";
        case OV_INVALIDATE_HARDWARE_CHANGE:    return "Hardware Profile Reconfigured";
        case OV_INVALIDATE_MEMORY_PRESSURE:    return "Memory Pressure Purge";
        case OV_INVALIDATE_INTEGRITY_FAILURE:  return "Integrity Verification Failure";
        default:                               return "Unknown Invalidation Reason";
    }
}
