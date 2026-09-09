/**
 * OpenVintage Pre-Boot Simulator - Resource Manager & OVScheduler Implementation (Phase 5)
 */

#include "ov_resource_manager.h"
#include "ov_hardware.h"
#include "ov_logger.h"
#include <string.h>

static ov_resource_status_t current_status = {0};

ov_status_t ov_resource_manager_init(void) {
    ov_log_info("Initializing Phase 5 Resource Manager & OVScheduler...");

    const ov_cpu_info_t *cpu = ov_hardware_get_cpu();
    const ov_gpu_info_t *gpu = ov_hardware_get_gpu();
    const ov_memory_info_t *mem = ov_hardware_get_memory();

    memset(&current_status, 0, sizeof(current_status));

    current_status.caps.supported_profiles_mask = 0x0F;
    current_status.caps.max_worker_threads = cpu->threads;
    current_status.caps.max_memory_budget_bytes = mem->available_bytes;
    current_status.caps.max_vram_budget_bytes = gpu->vram_bytes;
    current_status.caps.simd_avx2_supported = cpu->has_avx2;
    current_status.caps.battery_power_supported = true;
    current_status.caps.dynamic_core_affinity = true;

    /* Set initial profile to Balanced */
    ov_resource_manager_set_profile(OV_PROFILE_BALANCED);

    return OV_SUCCESS;
}

void ov_resource_manager_cleanup(void) {
    ov_log_info("Resource manager cleanup complete");
}

ov_status_t ov_resource_manager_get_caps(ov_resource_caps_t *out_caps) {
    if (!out_caps) return OV_ERROR_INVALID_PARAM;
    *out_caps = current_status.caps;
    return OV_SUCCESS;
}

ov_status_t ov_resource_manager_set_profile(ov_resource_profile_t profile) {
    const ov_cpu_info_t *cpu = ov_hardware_get_cpu();
    const ov_gpu_info_t *gpu = ov_hardware_get_gpu();
    const ov_memory_info_t *mem = ov_hardware_get_memory();

    current_status.active_profile = profile;

    switch (profile) {
        case OV_PROFILE_BALANCED:
            current_status.allocated_worker_threads = (cpu->threads > 2) ? (cpu->threads - 1) : cpu->threads;
            current_status.committed_memory_bytes = mem->total_bytes / 2;
            current_status.available_memory_bytes = mem->total_bytes / 2;
            current_status.committed_vram_bytes = gpu->vram_bytes / 2;
            current_status.available_vram_bytes = gpu->vram_bytes / 2;
            current_status.active_tasks_count = 3;
            current_status.thermal_throttling_active = false;
            break;

        case OV_PROFILE_PERFORMANCE:
            current_status.allocated_worker_threads = cpu->threads;
            current_status.committed_memory_bytes = (mem->total_bytes * 3) / 4;
            current_status.available_memory_bytes = mem->total_bytes / 4;
            current_status.committed_vram_bytes = (gpu->vram_bytes * 3) / 4;
            current_status.available_vram_bytes = gpu->vram_bytes / 4;
            current_status.active_tasks_count = 6;
            current_status.thermal_throttling_active = false;
            break;

        case OV_PROFILE_MAX_PERFORMANCE:
            current_status.allocated_worker_threads = cpu->threads;
            current_status.committed_memory_bytes = (mem->total_bytes * 7) / 8;
            current_status.available_memory_bytes = mem->total_bytes / 8;
            current_status.committed_vram_bytes = (gpu->vram_bytes * 7) / 8;
            current_status.available_vram_bytes = gpu->vram_bytes / 8;
            current_status.active_tasks_count = 10;
            current_status.thermal_throttling_active = false;
            break;

        case OV_PROFILE_BATTERY_LOW_POWER:
            current_status.allocated_worker_threads = (cpu->threads > 2) ? 2 : 1;
            current_status.committed_memory_bytes = mem->total_bytes / 4;
            current_status.available_memory_bytes = (mem->total_bytes * 3) / 4;
            current_status.committed_vram_bytes = gpu->vram_bytes / 4;
            current_status.available_vram_bytes = (gpu->vram_bytes * 3) / 4;
            current_status.active_tasks_count = 1;
            current_status.thermal_throttling_active = true;
            break;
    }

    ov_log_info("Active Resource Profile set to: %s (Workers: %u, VRAM: %lu MB)",
                ov_resource_profile_to_string(profile),
                current_status.allocated_worker_threads,
                current_status.committed_vram_bytes / (1024 * 1024));

    return OV_SUCCESS;
}

ov_resource_profile_t ov_resource_manager_get_profile(void) {
    return current_status.active_profile;
}

ov_status_t ov_resource_manager_get_status(ov_resource_status_t *out_status) {
    if (!out_status) return OV_ERROR_INVALID_PARAM;
    *out_status = current_status;
    return OV_SUCCESS;
}

const char* ov_resource_profile_to_string(ov_resource_profile_t profile) {
    switch (profile) {
        case OV_PROFILE_BALANCED: return "Balanced (Recommended)";
        case OV_PROFILE_PERFORMANCE: return "High Performance";
        case OV_PROFILE_MAX_PERFORMANCE: return "Maximum Throughput / Unconstrained";
        case OV_PROFILE_BATTERY_LOW_POWER: return "Battery / Low Power Mode";
        default: return "Unknown Profile";
    }
}
