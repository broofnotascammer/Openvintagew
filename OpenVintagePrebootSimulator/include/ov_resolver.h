/**
 * OpenVintage Pre-Boot Simulator - Phase 5 Unified Resolver Subsystem
 * Architecture Decision Matrix & Integrated Multi-Subsystem Evaluation.
 */

#ifndef OV_RESOLVER_H
#define OV_RESOLVER_H

#include "ov_types.h"
#include "ov_hardware.h"

/* Integrated Workload Request (mirrors Phase 5 OvResolverLib.h) */
typedef struct {
    char            application_name[64];
    uint32_t        guest_cpu_arch;         /* 1 = ARM64, 2 = x86_64, 3 = x86_32 */
    ov_api_type_t   requested_api;          /* 1 = OpenGL, 2 = Vulkan, 3 = Metal, 4 = DirectX */
    uint32_t        api_version_major;
    uint32_t        api_version_minor;
    bool            requires_compute;
    bool            requires_tessellation;
    bool            requires_avx2;
    uint32_t        max_texture_dimension;  /* e.g. 4096, 8192, 16384 */
    uint64_t        required_vram_bytes;
    uint64_t        required_ram_bytes;
    uint32_t        required_cpu_cores;
    uint64_t        guest_code_hash;        /* For CPU translation cache */
    uint64_t        shader_bytecode_hash;   /* For GPU shader cache */
} ov_integrated_request_t;

/* Integrated Resolution Evaluation Result (mirrors Phase 5 OvResolverLib.h) */
typedef struct {
    ov_resolution_decision_t    decision;
    uint32_t                    performance_cost_factor; /* 100 = 1.0x baseline, 150 = 1.5x, etc. */
    bool                        cpu_translation_required;
    bool                        gpu_translation_required;
    bool                        cpu_cache_hit;
    bool                        shader_cache_hit;
    bool                        texture_clamp_applied;
    bool                        shader_simplification_applied;
    bool                        software_fallback_used;
    uint64_t                    allocated_vram_quota;
    uint64_t                    allocated_ram_quota;
    char                        cpu_path[128];
    char                        gpu_path[128];
    char                        routing_path[256];
    char                        rationale[256];
} ov_resolution_result_t;

/* Workload Preset ID */
typedef enum {
    OV_WORKLOAD_PRESET_METAL_GAME = 0,
    OV_WORKLOAD_PRESET_VULKAN_SHOOTER = 1,
    OV_WORKLOAD_PRESET_DIRECTX_LEGACY = 2,
    OV_WORKLOAD_PRESET_ARM64_COMPUTE = 3,
    OV_WORKLOAD_PRESET_AVX2_SIMD = 4,
    OV_WORKLOAD_PRESET_OPENGL_CLASSIC = 5,
    OV_WORKLOAD_PRESET_COUNT
} ov_workload_preset_id_t;

/* Subsystem APIs */
ov_status_t ov_resolver_init(ov_cpu_info_t *cpu, ov_gpu_info_t *gpu, ov_memory_info_t *mem);
void        ov_resolver_cleanup(void);

ov_status_t ov_resolver_evaluate_integrated(
    const ov_integrated_request_t *request,
    ov_resolution_result_t        *out_result
);

/* Phase 2 legacy route helper */
ov_resolution_t ov_resolver_route(ov_workload_t *workload);
void            ov_resolver_print_decision(ov_resolution_t *decision);
void            ov_resolver_analyze_cpu(ov_cpu_info_t *cpu);
void            ov_resolver_analyze_gpu(ov_gpu_info_t *gpu);

/* Workload Presets */
uint32_t                ov_resolver_get_preset_count(void);
const char*             ov_resolver_get_preset_name(ov_workload_preset_id_t preset_id);
ov_integrated_request_t ov_resolver_get_preset_request(ov_workload_preset_id_t preset_id);

/* Decision String Helper */
const char* ov_resolver_decision_to_string(ov_resolution_decision_t decision);
const char* ov_resolver_api_to_string(ov_api_type_t api);

#endif /* OV_RESOLVER_H */
