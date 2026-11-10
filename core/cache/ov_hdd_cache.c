#include "ov_hdd_cache.h"
#include <string.h>
#include <time.h>

static uint32_t compute_crc32(const uint8_t *data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) crc = (crc >> 1) ^ 0xEDB88320;
            else crc >>= 1;
        }
    }
    return ~crc;
}

void ov_hdd_cache_init(ov_hdd_cache_t *cache, size_t max_ram_bytes) {
    if (!cache) return;
    memset(cache, 0, sizeof(ov_hdd_cache_t));
    cache->max_ram_bytes = (max_ram_bytes > 0) ? max_ram_bytes : (64 * 1024); // default 64KB RAM limit
}

static void evict_lru_ram(ov_hdd_cache_t *cache) {
    if (cache->ram_count == 0) return;

    size_t lru_idx = 0;
    uint64_t oldest_ts = cache->ram_entries[0].last_access_timestamp;
    for (size_t i = 1; i < cache->ram_count; i++) {
        if (cache->ram_entries[i].last_access_timestamp < oldest_ts) {
            oldest_ts = cache->ram_entries[i].last_access_timestamp;
            lru_idx = i;
        }
    }

    cache->current_ram_bytes -= cache->ram_entries[lru_idx].header.code_size;
    cache->evictions++;

    // Shift entries left to keep array contiguous
    for (size_t i = lru_idx; i + 1 < cache->ram_count; i++) {
        cache->ram_entries[i] = cache->ram_entries[i + 1];
    }
    cache->ram_count--;
}

ov_status_t ov_hdd_cache_lookup(
    ov_hdd_cache_t *cache,
    uint64_t code_hash,
    ov_arch_t src_arch,
    ov_arch_t tgt_arch,
    ovir_compiled_block_t *out_block,
    ov_cache_residency_t *out_residency
) {
    if (!cache) return OV_ERR_INVALID_PARAM;

    // 1. Check RAM Hot Tier first (Zero disk I/O)
    for (size_t i = 0; i < cache->ram_count; i++) {
        ov_hdd_cache_entry_t *entry = &cache->ram_entries[i];
        if (entry->header.code_identity_hash == code_hash &&
            entry->header.source_arch == src_arch &&
            entry->header.target_arch == tgt_arch) {
            entry->access_count++;
            entry->last_access_timestamp = (uint64_t)time(NULL);
            cache->ram_hits++;

            if (out_block) {
                out_block->target_arch = tgt_arch;
                out_block->code_size = entry->header.code_size;
                memcpy(out_block->code_bytes, entry->code_bytes, entry->header.code_size);
                out_block->verified = true;
            }
            if (out_residency) *out_residency = OV_RESIDENCY_RAM_HOT;
            return OV_SUCCESS; // RAM HOT HIT
        }
    }

    // 2. Check Persistent Storage (Warm Tier, Lazy Load)
    for (size_t i = 0; i < cache->disk_count; i++) {
        ov_hdd_cache_entry_t *entry = &cache->disk_store[i];
        if (entry->header.code_identity_hash == code_hash &&
            entry->header.source_arch == src_arch &&
            entry->header.target_arch == tgt_arch) {
            
            cache->persistent_reads++;

            // Integrity verification: Check Magic and CRC32
            if (entry->header.magic != OV_HDD_MAGIC) {
                cache->corruption_count++;
                cache->cache_misses++;
                return OV_ERR_CACHE_INVALID;
            }

            uint32_t calc_crc = compute_crc32(entry->code_bytes, entry->header.code_size);
            if (calc_crc != entry->header.crc32) {
                cache->corruption_count++;
                cache->cache_misses++;
                return OV_ERR_CACHE_INVALID; // Corruption detected on persistent storage!
            }

            cache->persistent_hits++;

            // Lazy load into RAM Hot Tier (check memory bounds)
            while (cache->current_ram_bytes + entry->header.code_size > cache->max_ram_bytes && cache->ram_count > 0) {
                evict_lru_ram(cache);
            }

            if (cache->ram_count < OV_HDD_MAX_RAM_ENTRIES) {
                cache->ram_entries[cache->ram_count] = *entry;
                cache->ram_entries[cache->ram_count].in_ram = true;
                cache->ram_entries[cache->ram_count].access_count = 1;
                cache->ram_entries[cache->ram_count].last_access_timestamp = (uint64_t)time(NULL);
                cache->current_ram_bytes += entry->header.code_size;
                cache->ram_count++;
            }

            if (out_block) {
                out_block->target_arch = tgt_arch;
                out_block->code_size = entry->header.code_size;
                memcpy(out_block->code_bytes, entry->code_bytes, entry->header.code_size);
                out_block->verified = true;
            }
            if (out_residency) *out_residency = OV_RESIDENCY_PERSISTENT_WARM;
            return OV_SUCCESS; // PERSISTENT WARM HIT
        }
    }

    cache->cache_misses++;
    if (out_residency) *out_residency = OV_RESIDENCY_COLD;
    return OV_ERR_CACHE_MISS;
}

ov_status_t ov_hdd_cache_insert(
    ov_hdd_cache_t *cache,
    uint64_t code_hash,
    ov_arch_t src_arch,
    ov_arch_t tgt_arch,
    const ovir_compiled_block_t *block
) {
    if (!cache || !block || block->code_size > OVIR_CPU_MAX_CODE_SIZE) return OV_ERR_INVALID_PARAM;

    // Enforce bounded RAM usage
    while (cache->current_ram_bytes + block->code_size > cache->max_ram_bytes && cache->ram_count > 0) {
        evict_lru_ram(cache);
    }

    ov_hdd_cache_entry_t new_entry;
    memset(&new_entry, 0, sizeof(ov_hdd_cache_entry_t));
    new_entry.header.magic = OV_HDD_MAGIC;
    new_entry.header.version_id = OV_TRANSLATOR_VERSION_ID;
    new_entry.header.code_identity_hash = code_hash;
    new_entry.header.source_arch = src_arch;
    new_entry.header.target_arch = tgt_arch;
    new_entry.header.code_size = block->code_size;
    new_entry.header.crc32 = compute_crc32(block->code_bytes, block->code_size);
    memcpy(new_entry.code_bytes, block->code_bytes, block->code_size);
    new_entry.in_ram = true;
    new_entry.access_count = 1;
    new_entry.last_access_timestamp = (uint64_t)time(NULL);

    // Insert into RAM Hot
    if (cache->ram_count < OV_HDD_MAX_RAM_ENTRIES) {
        cache->ram_entries[cache->ram_count++] = new_entry;
        cache->current_ram_bytes += block->code_size;
    }

    // Buffer for asynchronous / batched persistent write (never synchronous block-by-block on HDD)
    if (cache->write_buffer_count < OV_HDD_MAX_WRITE_BUFFER) {
        cache->write_buffer[cache->write_buffer_count++] = new_entry;
    } else {
        // Buffer full, flush batch to disk
        ov_hdd_cache_flush_buffer(cache);
        cache->write_buffer[cache->write_buffer_count++] = new_entry;
    }

    return OV_SUCCESS;
}

ov_status_t ov_hdd_cache_flush_buffer(ov_hdd_cache_t *cache) {
    if (!cache) return OV_ERR_INVALID_PARAM;
    if (cache->write_buffer_count == 0) return OV_SUCCESS;

    for (size_t i = 0; i < cache->write_buffer_count; i++) {
        // Find existing or add new slot in disk_store
        int slot = -1;
        for (size_t d = 0; d < cache->disk_count; d++) {
            if (cache->disk_store[d].header.code_identity_hash == cache->write_buffer[i].header.code_identity_hash) {
                slot = (int)d;
                break;
            }
        }
        if (slot < 0) {
            if (cache->disk_count < OV_HDD_MAX_PERSISTENT_ENTRIES) {
                slot = (int)cache->disk_count++;
            } else {
                slot = 0; // Replace oldest
            }
        }
        cache->disk_store[slot] = cache->write_buffer[i];
        cache->disk_store[slot].in_ram = false;
        cache->persistent_writes++;
    }

    cache->write_buffer_count = 0;
    return OV_SUCCESS;
}

ov_status_t ov_hdd_cache_invalidate(ov_hdd_cache_t *cache, uint64_t code_hash) {
    if (!cache) return OV_ERR_INVALID_PARAM;
    uint32_t count = 0;

    for (size_t i = 0; i < cache->ram_count; i++) {
        if (cache->ram_entries[i].header.code_identity_hash == code_hash) {
            cache->current_ram_bytes -= cache->ram_entries[i].header.code_size;
            for (size_t j = i; j + 1 < cache->ram_count; j++) {
                cache->ram_entries[j] = cache->ram_entries[j + 1];
            }
            cache->ram_count--;
            count++;
            break;
        }
    }

    for (size_t i = 0; i < cache->disk_count; i++) {
        if (cache->disk_store[i].header.code_identity_hash == code_hash) {
            for (size_t j = i; j + 1 < cache->disk_count; j++) {
                cache->disk_store[j] = cache->disk_store[j + 1];
            }
            cache->disk_count--;
            count++;
            break;
        }
    }

    cache->invalidations += count;
    return OV_SUCCESS;
}

void ov_hdd_cache_inject_corruption(ov_hdd_cache_t *cache, uint64_t code_hash) {
    if (!cache) return;
    for (size_t i = 0; i < cache->disk_count; i++) {
        if (cache->disk_store[i].header.code_identity_hash == code_hash) {
            cache->disk_store[i].code_bytes[0] ^= 0xFF; // Invert byte to corrupt CRC
            break;
        }
    }
}
