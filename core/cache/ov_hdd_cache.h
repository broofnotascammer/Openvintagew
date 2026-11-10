#ifndef OV_HDD_CACHE_H
#define OV_HDD_CACHE_H

#include "../types/ov_types.h"
#include "../../ovir/cpu/ovir_cpu.h"
#include "ov_unified_cache.h"

#define OV_HDD_MAX_RAM_ENTRIES 32
#define OV_HDD_MAX_PERSISTENT_ENTRIES 128
#define OV_HDD_MAX_WRITE_BUFFER 8
#define OV_HDD_MAGIC 0x4F564844 // 'OVHD'

typedef struct {
    uint32_t magic;
    uint32_t crc32;
    uint32_t version_id;
    uint64_t code_identity_hash;
    ov_arch_t source_arch;
    ov_arch_t target_arch;
    uint32_t code_size;
    uint32_t flags;
} ov_hdd_entry_header_t;

typedef struct {
    ov_hdd_entry_header_t header;
    uint8_t code_bytes[OVIR_CPU_MAX_CODE_SIZE];
    bool in_ram;
    uint64_t access_count;
    uint64_t last_access_timestamp;
} ov_hdd_cache_entry_t;

typedef struct {
    // RAM Hot Tier
    ov_hdd_cache_entry_t ram_entries[OV_HDD_MAX_RAM_ENTRIES];
    size_t ram_count;
    size_t max_ram_bytes;
    size_t current_ram_bytes;

    // Persistent Storage Store (Simulated / Buffered Disk File)
    ov_hdd_cache_entry_t disk_store[OV_HDD_MAX_PERSISTENT_ENTRIES];
    size_t disk_count;

    // Write Buffer for Batched Sequential Flush (never synchronous on hot path)
    ov_hdd_cache_entry_t write_buffer[OV_HDD_MAX_WRITE_BUFFER];
    size_t write_buffer_count;

    // Metrics
    uint64_t ram_hits;
    uint64_t persistent_hits;
    uint64_t cache_misses;
    uint64_t retranslations;
    uint64_t invalidations;
    uint64_t evictions;
    uint64_t persistent_writes;
    uint64_t persistent_reads;
    uint64_t corruption_count;
} ov_hdd_cache_t;

void ov_hdd_cache_init(ov_hdd_cache_t *cache, size_t max_ram_bytes);

// Core 3-tier lookup: RAM Hot -> Persistent Warm (Lazy Load to RAM) -> Miss (Cold)
ov_status_t ov_hdd_cache_lookup(
    ov_hdd_cache_t *cache,
    uint64_t code_hash,
    ov_arch_t src_arch,
    ov_arch_t tgt_arch,
    ovir_compiled_block_t *out_block,
    ov_cache_residency_t *out_residency
);

// Stores into RAM Hot tier and buffers for asynchronous / batched persistent write
ov_status_t ov_hdd_cache_insert(
    ov_hdd_cache_t *cache,
    uint64_t code_hash,
    ov_arch_t src_arch,
    ov_arch_t tgt_arch,
    const ovir_compiled_block_t *block
);

// Batched flush to persistent disk storage (simulates HDD write buffer flush)
ov_status_t ov_hdd_cache_flush_buffer(ov_hdd_cache_t *cache);

// Invalidate entries by hash across all tiers
ov_status_t ov_hdd_cache_invalidate(ov_hdd_cache_t *cache, uint64_t code_hash);

// Corrupt an entry deliberately for test verification
void ov_hdd_cache_inject_corruption(ov_hdd_cache_t *cache, uint64_t code_hash);

#endif /* OV_HDD_CACHE_H */
