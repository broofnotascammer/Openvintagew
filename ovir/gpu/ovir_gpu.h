#ifndef OVIR_GPU_H
#define OVIR_GPU_H

#include "../../core/types/ov_types.h"

#define OVIR_GPU_MAX_RESOURCES 16
#define OVIR_GPU_MAX_DRAWS 32

typedef enum {
    OVIR_RES_VERTEX_BUFFER = 0,
    OVIR_RES_INDEX_BUFFER,
    OVIR_RES_UNIFORM_BUFFER,
    OVIR_RES_TEXTURE_2D
} ovir_resource_type_t;

typedef enum {
    OVIR_SHADER_STAGE_VERTEX = 0,
    OVIR_SHADER_STAGE_FRAGMENT,
    OVIR_SHADER_STAGE_COMPUTE
} ovir_shader_stage_t;

typedef struct {
    uint32_t resource_id;
    ovir_resource_type_t type;
    uint32_t size_bytes;
    uint32_t width;
    uint32_t height;
    const char *debug_name;
} ovir_gpu_resource_desc_t;

typedef struct {
    uint32_t shader_id;
    ovir_shader_stage_t stage;
    ov_gfx_api_t source_api;
    uint64_t bytecode_hash;
    uint32_t bytecode_size;
    bool compiled;
} ovir_gpu_shader_desc_t;

typedef struct {
    uint32_t pipeline_id;
    uint32_t vertex_shader_id;
    uint32_t fragment_shader_id;
    uint32_t required_features;
    uint64_t pipeline_hash;
    bool is_compute;
} ovir_gpu_pipeline_desc_t;

typedef struct {
    uint32_t pipeline_id;
    uint32_t vertex_count;
    uint32_t instance_count;
    uint32_t first_vertex;
    uint32_t vbo_id;
} ovir_gpu_draw_cmd_t;

typedef struct {
    uint32_t fence_id;
    uint32_t stage_mask;
    bool completed;
} ovir_gpu_sync_cmd_t;

typedef struct {
    ovir_gpu_resource_desc_t resources[OVIR_GPU_MAX_RESOURCES];
    uint32_t resource_count;
    ovir_gpu_shader_desc_t shaders[4];
    uint32_t shader_count;
    ovir_gpu_pipeline_desc_t pipeline;
    ovir_gpu_draw_cmd_t draws[OVIR_GPU_MAX_DRAWS];
    uint32_t draw_count;
    ovir_gpu_sync_cmd_t sync;
    uint64_t workload_hash;
} ovir_gpu_workload_t;

typedef enum {
    OV_GPU_ROUTE_NATIVE = 0,
    OV_GPU_ROUTE_TRANSLATION,
    OV_GPU_ROUTE_RECONSTRUCTION,
    OV_GPU_ROUTE_CPU_FALLBACK,
    OV_GPU_ROUTE_UNSUPPORTED
} ov_gpu_route_t;

typedef struct {
    bool cpu_fallback_engaged;
    char fallback_reason[128];
    char operation_name[64];
    uint32_t estimated_cycle_cost;
    char cpu_execution_path[64];
    ov_status_t result_status;
} ovir_cpu_fallback_desc_t;

typedef struct {
    ov_gfx_backend_t backend;
    uint32_t supported_features;
    uint32_t partial_features;
    uint64_t available_vram_mb;
    bool has_metal;
    bool has_moltenvk;
    bool has_opengl;
    bool cpu_fallback_supported;
} ov_gpu_host_caps_t;

typedef struct {
    ov_gfx_backend_t selected_backend;
    ov_gpu_route_t route;
    uint64_t translated_shader_hash;
    bool cached_shader_reused;
    bool cached_pipeline_reused;
    uint32_t draw_calls_emitted;
    bool sync_passed;
    bool is_reconstructed;
    ovir_cpu_fallback_desc_t cpu_fallback;
} ovir_gpu_dispatch_result_t;

ov_status_t ovir_gpu_create_sample_workload(ovir_gpu_workload_t *out_workload, ov_gfx_api_t src_api, uint32_t required_features);
ov_gpu_route_t ovir_gpu_evaluate_route(const ovir_gpu_workload_t *workload, const ov_gpu_host_caps_t *caps, ov_reason_code_t *out_reason);
ov_status_t ovir_gpu_validate_features(const ovir_gpu_workload_t *workload, const ov_gpu_host_caps_t *caps);
ov_status_t ovir_gpu_translate_workload(
    const ovir_gpu_workload_t *workload,
    const ov_gpu_host_caps_t *caps,
    ovir_gpu_dispatch_result_t *out_result
);

const char *ov_gpu_route_name(ov_gpu_route_t route);


#endif /* OVIR_GPU_H */
