/**
 * OpenVintage - Hardware-Aware Performance Profile Subsystem Implementation (Phase 6)
 */

#include "ov_perf_profile.h"
#include <string.h>
#include <stdio.h>

static ov_perf_profile_config_t s_profiles[OV_PERF_PROFILE_COUNT] = {
    [OV_PERF_PROFILE_MAX_PERFORMANCE] = {
        .id = OV_PERF_PROFILE_MAX_PERFORMANCE,
        .name = "Maximum Performance",
        .description = "Discrete GPU preferred, maximum clock envelope, full texture and shader fidelity",
        .prefer_discrete_gpu = true,
        .cpu_power_policy = 2,
        .cpu_governor_str = "Performance",
        .graphics_policy = "Maximum Throughput / Uncapped",
        .low_latency_mode = false,
        .reduce_effects = false,
        .backend_preference = "Native Metal",
        .frame_rate_cap = 0,
        .max_texture_clamp = 0,
        .conservative_instructions = false
    },
    [OV_PERF_PROFILE_GAMING] = {
        .id = OV_PERF_PROFILE_GAMING,
        .name = "Gaming",
        .description = "Discrete GPU preferred, ultra-low dispatch latency, frame pacing, fast shader compilation",
        .prefer_discrete_gpu = true,
        .cpu_power_policy = 2,
        .cpu_governor_str = "Performance",
        .graphics_policy = "Low Latency Pacing / Game Optimized",
        .low_latency_mode = true,
        .reduce_effects = false,
        .backend_preference = "Native Metal",
        .frame_rate_cap = 0,
        .max_texture_clamp = 0,
        .conservative_instructions = false
    },
    [OV_PERF_PROFILE_BALANCED] = {
        .id = OV_PERF_PROFILE_BALANCED,
        .name = "Balanced",
        .description = "Intelligent dynamic switching, balanced thermals, optimal day-to-day responsiveness",
        .prefer_discrete_gpu = false,
        .cpu_power_policy = 1,
        .cpu_governor_str = "Balanced",
        .graphics_policy = "Dynamic Workload Adaptive",
        .low_latency_mode = false,
        .reduce_effects = false,
        .backend_preference = "Best Available",
        .frame_rate_cap = 60,
        .max_texture_clamp = 0,
        .conservative_instructions = false
    },
    [OV_PERF_PROFILE_BATTERY_EFFICIENCY] = {
        .id = OV_PERF_PROFILE_BATTERY_EFFICIENCY,
        .name = "Battery / Efficiency",
        .description = "Integrated GPU forced, power-saving clock policy, cosmetic effects throttled",
        .prefer_discrete_gpu = false,
        .cpu_power_policy = 0,
        .cpu_governor_str = "PowerSave",
        .graphics_policy = "Visual Efficiency / Throttled",
        .low_latency_mode = false,
        .reduce_effects = true,
        .backend_preference = "Power-Capped",
        .frame_rate_cap = 30,
        .max_texture_clamp = 4096,
        .conservative_instructions = false
    },
    [OV_PERF_PROFILE_COMPATIBILITY] = {
        .id = OV_PERF_PROFILE_COMPATIBILITY,
        .name = "Compatibility",
        .description = "Conservative instruction set fallback, safe texture clamping, maximum stability",
        .prefer_discrete_gpu = false,
        .cpu_power_policy = 1,
        .cpu_governor_str = "Balanced",
        .graphics_policy = "Conservative Safe Shaders",
        .low_latency_mode = false,
        .reduce_effects = false,
        .backend_preference = "Conservative Safe",
        .frame_rate_cap = 60,
        .max_texture_clamp = 4096,
        .conservative_instructions = true
    },
    [OV_PERF_PROFILE_CUSTOM] = {
        .id = OV_PERF_PROFILE_CUSTOM,
        .name = "Custom",
        .description = "User-customized hardware and runtime execution profile",
        .prefer_discrete_gpu = true,
        .cpu_power_policy = 1,
        .cpu_governor_str = "Custom",
        .graphics_policy = "User Defined",
        .low_latency_mode = false,
        .reduce_effects = false,
        .backend_preference = "Custom",
        .frame_rate_cap = 0,
        .max_texture_clamp = 0,
        .conservative_instructions = false
    }
};

static ov_perf_profile_id_t s_active_perf_id = OV_PERF_PROFILE_BALANCED;

ov_status_t ov_perf_profile_init(void) {
    s_active_perf_id = OV_PERF_PROFILE_BALANCED;
    return OV_SUCCESS;
}

void ov_perf_profile_cleanup(void) {
    s_active_perf_id = OV_PERF_PROFILE_BALANCED;
}

uint32_t ov_perf_profile_get_count(void) {
    return OV_PERF_PROFILE_COUNT;
}

ov_status_t ov_perf_profile_get_by_id(ov_perf_profile_id_t id, ov_perf_profile_config_t *out_config) {
    if (!out_config || id >= OV_PERF_PROFILE_COUNT) return OV_ERROR_INVALID_PARAM;
    *out_config = s_profiles[id];
    return OV_SUCCESS;
}

ov_status_t ov_perf_profile_get_by_name(const char *name, ov_perf_profile_config_t *out_config) {
    if (!name || !out_config) return OV_ERROR_INVALID_PARAM;
    for (uint32_t i = 0; i < OV_PERF_PROFILE_COUNT; ++i) {
        if (strcasecmp(s_profiles[i].name, name) == 0) {
            *out_config = s_profiles[i];
            return OV_SUCCESS;
        }
    }
    if (strcasecmp(name, "Max") == 0 || strcasecmp(name, "Performance") == 0) {
        *out_config = s_profiles[OV_PERF_PROFILE_MAX_PERFORMANCE];
        return OV_SUCCESS;
    }
    if (strcasecmp(name, "Battery") == 0 || strcasecmp(name, "Efficiency") == 0) {
        *out_config = s_profiles[OV_PERF_PROFILE_BATTERY_EFFICIENCY];
        return OV_SUCCESS;
    }
    return OV_ERROR_NOT_FOUND;
}

ov_status_t ov_perf_profile_apply(ov_perf_profile_id_t id) {
    if (id >= OV_PERF_PROFILE_COUNT) return OV_ERROR_INVALID_PARAM;
    s_active_perf_id = id;

    const ov_gpu_topology_t *topo = ov_hardware_get_gpu_topology();
    if (topo && topo->gpu_count > 1) {
        if (s_profiles[id].prefer_discrete_gpu && topo->has_discrete_gpu) {
            ov_hardware_set_active_gpu_index(topo->discrete_gpu_index);
        } else if (topo->has_integrated_gpu) {
            ov_hardware_set_active_gpu_index(topo->primary_gpu_index);
        }
    }

    return OV_SUCCESS;
}

ov_status_t ov_perf_profile_apply_by_name(const char *name) {
    ov_perf_profile_config_t conf;
    ov_status_t st = ov_perf_profile_get_by_name(name, &conf);
    if (st != OV_SUCCESS) return st;
    return ov_perf_profile_apply(conf.id);
}

ov_perf_profile_id_t ov_perf_profile_get_active_id(void) {
    return s_active_perf_id;
}

const ov_perf_profile_config_t* ov_perf_profile_get_active(void) {
    return &s_profiles[s_active_perf_id];
}

ov_status_t ov_perf_profile_update_custom(const ov_perf_profile_config_t *custom) {
    if (!custom) return OV_ERROR_INVALID_PARAM;
    s_profiles[OV_PERF_PROFILE_CUSTOM] = *custom;
    s_profiles[OV_PERF_PROFILE_CUSTOM].id = OV_PERF_PROFILE_CUSTOM;
    return OV_SUCCESS;
}
