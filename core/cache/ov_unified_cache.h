#ifndef OV_UNIFIED_CACHE_H
#define OV_UNIFIED_CACHE_H

#include "../types/ov_types.h"
#include "../../ovir/cpu/ovir_cpu.h"

#define OV_CACHE_MAX_ENTRIES 64

typedef enum {
    OV_CACHE_ENTRY_CPU_TRANSLATION = 0,
    OV_CACHE_ENTRY_GPU_SHADER,
    OV_CACHE_ENTRY_GPU_PIPELINE
} ov_cache_entry_type_t;

typedef struct {
    ov_arch_t source_arch;
    ov_arch_t target_arch;
    uint64_t code_identity_hash;
    uint32_t translator_version_id;
    uint32_t required_cpu_features;
    uint32_t optimization_level;
} ov_cache_cpu_metadata_t;

typedef struct {
    ov_gfx_api_t source_api;
    ov_gfx_backend_t target_backend;
    uint64_t shader_hash;
    uint64_t pipeline_hash;
    uint32_t required_gpu_features;
    uint32_t translator_version_id;
} ov_cache_gpu_metadata_t;

typedef struct {
    bool valid;
    ov_cache_entry_type_t type;
    union {
        ov_cache_cpu_metadata_t cpu_meta;
        ov_cache_gpu_metadata_t gpu_meta;
    } meta;
    uint8_t payload[OVIR_CPU_MAX_CODE_SIZE];
    uint32_t payload_size;
    uint64_t access_count;
} ov_cache_entry_t;

typedef struct {
    ov_cache_entry_t entries[OV_CACHE_MAX_ENTRIES];
    uint32_t entry_count;
    uint64_t total_lookups;
    uint64_t hits;
    uint64_t misses;
    uint64_t invalidations;
} ov_unified_cache_t;

void ov_unified_cache_init(ov_unified_cache_t *cache);

// CPU Translation Cache APIs
ov_status_t ov_cache_lookup_cpu(
    ov_unified_cache_t *cache,
    const ov_cache_cpu_metadata_t *requested_meta,
    ovir_compiled_block_t *out_block
);

ov_status_t ov_cache_store_cpu(
    ov_unified_cache_t *cache,
    const ov_cache_cpu_metadata_t *meta,
    const ovir_compiled_block_t *block
);

// GPU Shader / Pipeline Cache APIs
ov_status_t ov_cache_lookup_gpu(
    ov_unified_cache_t *cache,
    const ov_cache_gpu_metadata_t *requested_meta,
    uint64_t *out_translated_hash
);

ov_status_t ov_cache_store_gpu(
    ov_unified_cache_t *cache,
    const ov_cache_gpu_metadata_t *meta,
    uint64_t translated_hash
);

ov_status_t ov_cache_invalidate_matching(
    ov_unified_cache_t *cache,
    uint64_t code_identity_hash
);

void ov_cache_get_stats(
    const ov_unified_cache_t *cache,
    uint64_t *out_lookups,
    uint64_t *out_hits,
    uint64_t *out_misses,
    uint64_t *out_invalidations
);

#endif /* OV_UNIFIED_CACHE_H */
