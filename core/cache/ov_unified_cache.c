#include "ov_unified_cache.h"
#include <string.h>

void ov_unified_cache_init(ov_unified_cache_t *cache) {
    if (!cache) return;
    memset(cache, 0, sizeof(ov_unified_cache_t));
}

static bool validate_cpu_metadata(const ov_cache_cpu_metadata_t *cached, const ov_cache_cpu_metadata_t *req) {
    if (!cached || !req) return false;
    if (cached->code_identity_hash != req->code_identity_hash) return false;
    if (cached->source_arch != req->source_arch) return false;
    if (cached->target_arch != req->target_arch) return false;
    if (cached->translator_version_id != req->translator_version_id) return false;
    if (cached->required_cpu_features != req->required_cpu_features) return false;
    if (cached->optimization_level != req->optimization_level) return false;
    return true;
}

ov_status_t ov_cache_lookup_cpu(
    ov_unified_cache_t *cache,
    const ov_cache_cpu_metadata_t *requested_meta,
    ovir_compiled_block_t *out_block
) {
    if (!cache || !requested_meta) return OV_ERR_INVALID_PARAM;
    cache->total_lookups++;

    for (uint32_t i = 0; i < cache->entry_count; i++) {
        ov_cache_entry_t *entry = &cache->entries[i];
        if (!entry->valid || entry->type != OV_CACHE_ENTRY_CPU_TRANSLATION) {
            continue;
        }

        // Check if code hash matches
        if (entry->meta.cpu_meta.code_identity_hash == requested_meta->code_identity_hash) {
            // Validate all metadata fields strictly
            if (validate_cpu_metadata(&entry->meta.cpu_meta, requested_meta)) {
                entry->access_count++;
                cache->hits++;
                if (out_block) {
                    memset(out_block, 0, sizeof(ovir_compiled_block_t));
                    out_block->target_arch = entry->meta.cpu_meta.target_arch;
                    out_block->code_size = entry->payload_size;
                    memcpy(out_block->code_bytes, entry->payload, entry->payload_size);
                    out_block->verified = true;
                }
                return OV_SUCCESS; // Cache HIT
            } else {
                // Hash matched but metadata invalid/stale -> trigger invalidation
                entry->valid = false;
                cache->invalidations++;
                cache->misses++;
                return OV_ERR_CACHE_INVALID;
            }
        }
    }

    cache->misses++;
    return OV_ERR_CACHE_MISS;
}

ov_status_t ov_cache_store_cpu(
    ov_unified_cache_t *cache,
    const ov_cache_cpu_metadata_t *meta,
    const ovir_compiled_block_t *block
) {
    if (!cache || !meta || !block || block->code_size > OVIR_CPU_MAX_CODE_SIZE) {
        return OV_ERR_INVALID_PARAM;
    }

    // Check if slot with same hash already exists to update
    int target_idx = -1;
    for (uint32_t i = 0; i < cache->entry_count; i++) {
        if (cache->entries[i].meta.cpu_meta.code_identity_hash == meta->code_identity_hash) {
            target_idx = (int)i;
            break;
        }
    }

    if (target_idx < 0) {
        if (cache->entry_count < OV_CACHE_MAX_ENTRIES) {
            target_idx = (int)cache->entry_count++;
        } else {
            // Evict slot 0 for demonstration simplicity
            target_idx = 0;
        }
    }

    ov_cache_entry_t *entry = &cache->entries[target_idx];
    entry->valid = true;
    entry->type = OV_CACHE_ENTRY_CPU_TRANSLATION;
    entry->meta.cpu_meta = *meta;
    entry->payload_size = block->code_size;
    memcpy(entry->payload, block->code_bytes, block->code_size);
    entry->access_count = 1;

    return OV_SUCCESS;
}

static bool validate_gpu_metadata(const ov_cache_gpu_metadata_t *cached, const ov_cache_gpu_metadata_t *req) {
    if (!cached || !req) return false;
    if (cached->shader_hash != req->shader_hash) return false;
    if (cached->source_api != req->source_api) return false;
    if (cached->target_backend != req->target_backend) return false;
    if (cached->required_gpu_features != req->required_gpu_features) return false;
    if (cached->translator_version_id != req->translator_version_id) return false;
    return true;
}

ov_status_t ov_cache_lookup_gpu(
    ov_unified_cache_t *cache,
    const ov_cache_gpu_metadata_t *requested_meta,
    uint64_t *out_translated_hash
) {
    if (!cache || !requested_meta) return OV_ERR_INVALID_PARAM;
    cache->total_lookups++;

    for (uint32_t i = 0; i < cache->entry_count; i++) {
        ov_cache_entry_t *entry = &cache->entries[i];
        if (!entry->valid || entry->type != OV_CACHE_ENTRY_GPU_SHADER) {
            continue;
        }

        if (entry->meta.gpu_meta.shader_hash == requested_meta->shader_hash) {
            if (validate_gpu_metadata(&entry->meta.gpu_meta, requested_meta)) {
                entry->access_count++;
                cache->hits++;
                if (out_translated_hash) {
                    memcpy(out_translated_hash, entry->payload, sizeof(uint64_t));
                }
                return OV_SUCCESS;
            } else {
                entry->valid = false;
                cache->invalidations++;
                cache->misses++;
                return OV_ERR_CACHE_INVALID;
            }
        }
    }

    cache->misses++;
    return OV_ERR_CACHE_MISS;
}

ov_status_t ov_cache_store_gpu(
    ov_unified_cache_t *cache,
    const ov_cache_gpu_metadata_t *meta,
    uint64_t translated_hash
) {
    if (!cache || !meta) return OV_ERR_INVALID_PARAM;

    int target_idx = -1;
    for (uint32_t i = 0; i < cache->entry_count; i++) {
        if (cache->entries[i].meta.gpu_meta.shader_hash == meta->shader_hash) {
            target_idx = (int)i;
            break;
        }
    }

    if (target_idx < 0) {
        if (cache->entry_count < OV_CACHE_MAX_ENTRIES) {
            target_idx = (int)cache->entry_count++;
        } else {
            target_idx = 0;
        }
    }

    ov_cache_entry_t *entry = &cache->entries[target_idx];
    entry->valid = true;
    entry->type = OV_CACHE_ENTRY_GPU_SHADER;
    entry->meta.gpu_meta = *meta;
    entry->payload_size = sizeof(uint64_t);
    memcpy(entry->payload, &translated_hash, sizeof(uint64_t));
    entry->access_count = 1;

    return OV_SUCCESS;
}

ov_status_t ov_cache_invalidate_matching(
    ov_unified_cache_t *cache,
    uint64_t code_identity_hash
) {
    if (!cache) return OV_ERR_INVALID_PARAM;
    uint32_t invalidated = 0;
    for (uint32_t i = 0; i < cache->entry_count; i++) {
        ov_cache_entry_t *entry = &cache->entries[i];
        if (entry->valid && entry->meta.cpu_meta.code_identity_hash == code_identity_hash) {
            entry->valid = false;
            invalidated++;
        }
    }
    cache->invalidations += invalidated;
    return OV_SUCCESS;
}

void ov_cache_get_stats(
    const ov_unified_cache_t *cache,
    uint64_t *out_lookups,
    uint64_t *out_hits,
    uint64_t *out_misses,
    uint64_t *out_invalidations
) {
    if (!cache) return;
    if (out_lookups) *out_lookups = cache->total_lookups;
    if (out_hits) *out_hits = cache->hits;
    if (out_misses) *out_misses = cache->misses;
    if (out_invalidations) *out_invalidations = cache->invalidations;
}
