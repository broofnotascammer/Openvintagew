#ifndef OV_RESOLVER_H
#define OV_RESOLVER_H

#include "../types/ov_types.h"
#include "../cache/ov_unified_cache.h"
#include "../../ovir/gpu/ovir_gpu.h"

#define OV_MAX_LIMITATION_LEN 256

typedef struct {
    ov_arch_t source_cpu_arch;
    ov_arch_t target_cpu_arch;
    ov_gfx_api_t source_gfx_api;
    ov_gfx_backend_t preferred_gfx_backend;
    uint32_t required_cpu_features;
    uint32_t required_gpu_features;
    uint64_t ram_requirement_mb;
    uint64_t vram_requirement_mb;
    uint32_t required_cpu_cores;
    uint64_t code_identity_hash;
    uint64_t shader_identity_hash;
    ov_priority_t priority;
    ov_workload_type_t workload_type;
    bool native_execution_available;
} ov_workload_desc_t;

typedef struct {
    ov_arch_t host_cpu_arch;
    uint32_t host_cpu_features;
    uint32_t host_cpu_cores;
    uint64_t host_ram_mb;
    ov_gpu_host_caps_t gpu_caps;
} ov_host_env_t;

typedef struct {
    ov_execution_decision_t decision;
    ov_reason_code_t reason_code;
    bool cpu_translation_required;
    bool gpu_translation_required;
    bool gpu_reconstruction_engaged;
    bool cpu_fallback_engaged;
    char cpu_fallback_path[64];
    ov_arch_t selected_cpu_target;
    ov_gfx_backend_t selected_gfx_backend;
    ov_scheduler_profile_t selected_profile;
    bool cpu_cache_hit;
    bool gpu_cache_hit;
    char limitations[OV_MAX_LIMITATION_LEN];
    bool is_supported;
} ov_execution_plan_t;

ov_status_t ov_resolver_create_plan(
    const ov_workload_desc_t *workload,
    const ov_host_env_t *host,
    ov_unified_cache_t *cache,
    ov_execution_plan_t *out_plan
);

#endif /* OV_RESOLVER_H */
