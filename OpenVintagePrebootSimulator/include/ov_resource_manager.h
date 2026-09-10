/**
 * OpenVintage Pre-Boot Simulator - Resource Manager & OVScheduler (Phase 5)
 * Dynamic resource allocation, platform constraints, and performance profiles.
 */

#ifndef OV_RESOURCE_MANAGER_H
#define OV_RESOURCE_MANAGER_H

#include "ov_types.h"

/* Platform Supported Resource Settings */
typedef struct {
    uint32_t supported_profiles_mask;
    uint32_t max_worker_threads;
    uint64_t max_memory_budget_bytes;
    uint64_t max_vram_budget_bytes;
    bool     simd_avx2_supported;
    bool     battery_power_supported;
    bool     dynamic_core_affinity;
} ov_resource_caps_t;

/* Active Resource Status */
typedef struct {
    ov_resource_profile_t   active_profile;
    ov_resource_pressure_t  pressure_level;
    uint32_t                allocated_worker_threads;
    uint64_t                committed_memory_bytes;
    uint64_t                available_memory_bytes;
    uint64_t                committed_vram_bytes;
    uint64_t                available_vram_bytes;
    uint32_t                active_tasks_count;
    bool                    thermal_throttling_active;
    ov_resource_caps_t      caps;
} ov_resource_status_t;

/* APIs */
ov_status_t             ov_resource_manager_init(void);
void                    ov_resource_manager_cleanup(void);

ov_status_t             ov_resource_manager_get_caps(ov_resource_caps_t *out_caps);
ov_status_t             ov_resource_manager_set_profile(ov_resource_profile_t profile);
ov_resource_profile_t   ov_resource_manager_get_profile(void);
ov_status_t             ov_resource_manager_get_status(ov_resource_status_t *out_status);

/* Allocation and VRAM Management */
ov_status_t             ov_resource_manager_allocate_memory(uint64_t bytes, bool is_vram);
ov_status_t             ov_resource_manager_free_memory(uint64_t bytes, bool is_vram);
ov_resource_pressure_t  ov_resource_manager_get_pressure(void);
bool                    ov_resource_manager_clamp_texture_for_vram(uint64_t required_vram, uint32_t *in_out_tex_dim);

const char*             ov_resource_profile_to_string(ov_resource_profile_t profile);
const char*             ov_resource_pressure_to_string(ov_resource_pressure_t pressure);

#endif /* OV_RESOURCE_MANAGER_H */
