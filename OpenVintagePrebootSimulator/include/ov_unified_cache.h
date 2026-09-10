/**
 * OpenVintage Pre-Boot Simulator - Unified Cache Subsystem (Phase 5)
 * Multi-tier cache tracking, real key-value storage, stats, and reliable invalidation.
 */

#ifndef OV_UNIFIED_CACHE_H
#define OV_UNIFIED_CACHE_H

#include "ov_types.h"

#define OV_CACHE_MAX_ENTRIES_PER_TIER 64
#define OV_CACHE_MAX_PAYLOAD_BYTES    2048

/* Unified Cache Statistics */
typedef struct {
    uint32_t cpu_cache_entries;
    uint64_t cpu_cache_bytes;
    uint64_t cpu_cache_hits;
    uint64_t cpu_cache_misses;

    uint32_t shader_cache_entries;
    uint64_t shader_cache_bytes;
    uint64_t shader_cache_hits;
    uint64_t shader_cache_misses;

    uint32_t pipeline_cache_entries;
    uint64_t pipeline_cache_bytes;
    uint64_t pipeline_cache_hits;
    uint64_t pipeline_cache_misses;

    uint32_t compat_cache_entries;
    uint64_t compat_cache_bytes;
    uint64_t compat_cache_hits;
    uint64_t compat_cache_misses;

    uint64_t total_queries;
    uint64_t total_hits;
    uint64_t total_misses;
    uint32_t overall_hit_rate_percent;
    uint32_t current_generation;
} ov_unified_cache_stats_t;

/* Subsystem APIs */
ov_status_t ov_unified_cache_init(void);
void        ov_unified_cache_cleanup(void);

/* Functional In-Memory Cache Store & Lookup */
ov_status_t ov_unified_cache_store(ov_cache_tier_t tier, uint64_t key_hash, const void *data, size_t size);
ov_status_t ov_unified_cache_lookup(ov_cache_tier_t tier, uint64_t key_hash, void *out_data, size_t max_size, size_t *out_size);
ov_status_t ov_unified_cache_evict_lru(ov_cache_tier_t tier);
ov_status_t ov_unified_cache_clear_tier(ov_cache_tier_t tier);

ov_status_t ov_unified_cache_get_stats(ov_unified_cache_stats_t *out_stats);
ov_status_t ov_unified_cache_invalidate(ov_cache_tier_t tier, ov_invalidate_reason_t reason);
ov_status_t ov_unified_cache_verify_integrity(void);
uint32_t    ov_unified_cache_get_generation(void);

/* Simulation Telemetry Ingestion */
void ov_unified_cache_record_query(ov_cache_tier_t tier, bool hit, uint64_t bytes);

const char* ov_cache_tier_to_string(ov_cache_tier_t tier);
const char* ov_invalidate_reason_to_string(ov_invalidate_reason_t reason);

#endif /* OV_UNIFIED_CACHE_H */
