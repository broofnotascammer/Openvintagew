/**
 * OpenVintage - Hardware-Aware Performance Profile Subsystem (Phase 6)
 * Selects optimal configurations for detected hardware and workload profiles:
 * Maximum Performance, Gaming, Balanced, Battery/Efficiency, Compatibility, Custom.
 */

#ifndef OV_PERF_PROFILE_H
#define OV_PERF_PROFILE_H

#include "ov_types.h"
#include "ov_hardware.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    ov_perf_profile_id_t id;
    char                 name[32];
    char                 description[128];
    bool                 prefer_discrete_gpu;      /* Prefer discrete GPU where supported */
    uint32_t             cpu_power_policy;         /* 0=PowerSave, 1=Balanced, 2=Performance */
    char                 cpu_governor_str[32];     /* "Performance", "Balanced", "PowerSave" */
    char                 graphics_policy[64];      /* e.g. "Low Latency Pacing", "Maximum Throughput" */
    bool                 low_latency_mode;         /* Low latency command dispatch */
    bool                 reduce_effects;           /* Reduce cosmetic rendering effects for battery */
    char                 backend_preference[32];   /* "Native Metal", "OpenGL Core", "Conservative Safe" */
    uint32_t             frame_rate_cap;           /* 0=uncapped, 30, 60 */
    uint32_t             max_texture_clamp;        /* 0=native max, 4096, 8192 */
    bool                 conservative_instructions;/* Conservative CPU/GPU instruction set subset */
} ov_perf_profile_config_t;

ov_status_t ov_perf_profile_init(void);
void        ov_perf_profile_cleanup(void);

uint32_t    ov_perf_profile_get_count(void);
ov_status_t ov_perf_profile_get_by_id(ov_perf_profile_id_t id, ov_perf_profile_config_t *out_config);
ov_status_t ov_perf_profile_get_by_name(const char *name, ov_perf_profile_config_t *out_config);

/* Applies performance profile to the active hardware state (GPU selection, governors) */
ov_status_t ov_perf_profile_apply(ov_perf_profile_id_t id);
ov_status_t ov_perf_profile_apply_by_name(const char *name);

ov_perf_profile_id_t             ov_perf_profile_get_active_id(void);
const ov_perf_profile_config_t*  ov_perf_profile_get_active(void);

ov_status_t ov_perf_profile_update_custom(const ov_perf_profile_config_t *custom);

#endif /* OV_PERF_PROFILE_H */
