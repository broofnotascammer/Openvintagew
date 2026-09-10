/**
 * OpenVintage Pre-Boot Simulator - Resource Manager & OVScheduler Implementation (Phase 5)
 * Dynamic resource allocation, platform constraints, and performance profiles.
 */

#include "ov_resource_manager.h"
#include "ov_hardware.h"
#include "ov_logger.h"
#include <string.h>

static ov_resource_status_t current_status = {0};

ov_status_t ov_resource_manager_init(void) {
    const ov_cpu_info_t *cpu = ov_hardware_get_cpu();
    const ov_gpu_info_t *gpu = ov_hardware_get_gpu();
    const ov_memory_info_t *mem = ov_hardware_get_memory();

    memset(&current_status, 0, sizeof(current_status));

    current_status.caps.supported_profiles_mask = 0x0F;
    current_status.caps.max_worker_threads = cpu->threads > 0 ? cpu->threads : 1;
    current_status.caps.max_memory_budget_bytes = mem->total_bytes > 0 ? mem->total_bytes : (4ULL * 1024 * 1024 * 1024);
    current_status.caps.max_vram_budget_bytes = gpu->vram_bytes > 0 ? gpu->vram_bytes : (1024ULL * 1024 * 1024);
    current_status.caps.simd_avx2_supported = cpu->has_avx2;
    current_status.caps.battery_power_supported = true;
    current_status.caps.dynamic_core_affinity = true;

    /* Set initial profile to Balanced */
    ov_resource_manager_set_profile(OV_PROFILE_BALANCED);
    ov_log_info("Resource Manager initialized (Max RAM: %llu MB, Max VRAM: %llu MB)",
                (unsigned long long)(current_status.caps.max_memory_budget_bytes / (1024 * 1024)),
                (unsigned long long)(current_status.caps.max_vram_budget_bytes / (1024 * 1024)));

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

    uint64_t total_ram = mem->total_bytes > 0 ? mem->total_bytes : (4ULL * 1024 * 1024 * 1024);
    uint64_t total_vram = gpu->vram_bytes > 0 ? gpu->vram_bytes : (1024ULL * 1024 * 1024);
    uint32_t threads = cpu->threads > 0 ? cpu->threads : 1;

    current_status.active_profile = profile;

    switch (profile) {
        case OV_PROFILE_BALANCED:
            current_status.allocated_worker_threads = (threads > 2) ? (threads - 1) : threads;
            current_status.committed_memory_bytes = total_ram / 4;
            current_status.available_memory_bytes = total_ram - current_status.committed_memory_bytes;
            current_status.committed_vram_bytes = total_vram / 4;
            current_status.available_vram_bytes = total_vram - current_status.committed_vram_bytes;
            current_status.active_tasks_count = 3;
            current_status.thermal_throttling_active = false;
            break;

        case OV_PROFILE_PERFORMANCE:
            current_status.allocated_worker_threads = threads;
            current_status.committed_memory_bytes = total_ram / 2;
            current_status.available_memory_bytes = total_ram - current_status.committed_memory_bytes;
            current_status.committed_vram_bytes = total_vram / 2;
            current_status.available_vram_bytes = total_vram - current_status.committed_vram_bytes;
            current_status.active_tasks_count = 6;
            current_status.thermal_throttling_active = false;
            break;

        case OV_PROFILE_MAX_PERFORMANCE:
            current_status.allocated_worker_threads = threads;
            current_status.committed_memory_bytes = (total_ram * 3) / 4;
            current_status.available_memory_bytes = total_ram - current_status.committed_memory_bytes;
            current_status.committed_vram_bytes = (total_vram * 3) / 4;
            current_status.available_vram_bytes = total_vram - current_status.committed_vram_bytes;
            current_status.active_tasks_count = 10;
            current_status.thermal_throttling_active = false;
            break;

        case OV_PROFILE_BATTERY_LOW_POWER:
            current_status.allocated_worker_threads = (threads > 2) ? 2 : 1;
            current_status.committed_memory_bytes = total_ram / 8;
            current_status.available_memory_bytes = total_ram - current_status.committed_memory_bytes;
            current_status.committed_vram_bytes = total_vram / 8;
            current_status.available_vram_bytes = total_vram - current_status.committed_vram_bytes;
            current_status.active_tasks_count = 1;
            current_status.thermal_throttling_active = true;
            break;
    }

    current_status.pressure_level = ov_resource_manager_get_pressure();

    ov_log_info("Active Resource Profile set to: %s (Workers: %u, VRAM: %llu MB)",
                ov_resource_profile_to_string(profile),
                current_status.allocated_worker_threads,
                (unsigned long long)(current_status.committed_vram_bytes / (1024 * 1024)));

    return OV_SUCCESS;
}

ov_resource_profile_t ov_resource_manager_get_profile(void) {
    return current_status.active_profile;
}

ov_status_t ov_resource_manager_get_status(ov_resource_status_t *out_status) {
    if (!out_status) return OV_ERROR_INVALID_PARAM;
    current_status.pressure_level = ov_resource_manager_get_pressure();
    *out_status = current_status;
    return OV_SUCCESS;
}

ov_status_t ov_resource_manager_allocate_memory(uint64_t bytes, bool is_vram) {
    if (is_vram) {
        if (current_status.available_vram_bytes < bytes) {
            return OV_ERROR_OUT_OF_RESOURCES;
        }
        current_status.committed_vram_bytes += bytes;
        current_status.available_vram_bytes -= bytes;
    } else {
        if (current_status.available_memory_bytes < bytes) {
            return OV_ERROR_OUT_OF_RESOURCES;
        }
        current_status.committed_memory_bytes += bytes;
        current_status.available_memory_bytes -= bytes;
    }
    current_status.pressure_level = ov_resource_manager_get_pressure();
    return OV_SUCCESS;
}

ov_status_t ov_resource_manager_free_memory(uint64_t bytes, bool is_vram) {
    if (is_vram) {
        if (current_status.committed_vram_bytes >= bytes) {
            current_status.committed_vram_bytes -= bytes;
            current_status.available_vram_bytes += bytes;
        } else {
            current_status.available_vram_bytes += current_status.committed_vram_bytes;
            current_status.committed_vram_bytes = 0;
        }
    } else {
        if (current_status.committed_memory_bytes >= bytes) {
            current_status.committed_memory_bytes -= bytes;
            current_status.available_memory_bytes += bytes;
        } else {
            current_status.available_memory_bytes += current_status.committed_memory_bytes;
            current_status.committed_memory_bytes = 0;
        }
    }
    current_status.pressure_level = ov_resource_manager_get_pressure();
    return OV_SUCCESS;
}

ov_resource_pressure_t ov_resource_manager_get_pressure(void) {
    uint64_t total_mem = current_status.committed_memory_bytes + current_status.available_memory_bytes;
    if (total_mem == 0) return OV_RESOURCE_PRESSURE_NORMAL;

    uint32_t pct = (uint32_t)((current_status.committed_memory_bytes * 100) / total_mem);
    if (pct > 80) return OV_RESOURCE_PRESSURE_CRITICAL;
    if (pct > 60) return OV_RESOURCE_PRESSURE_MODERATE;
    return OV_RESOURCE_PRESSURE_NORMAL;
}

bool ov_resource_manager_clamp_texture_for_vram(uint64_t required_vram, uint32_t *in_out_tex_dim) {
    if (!in_out_tex_dim) return false;
    bool clamped = false;

    while (required_vram > current_status.available_vram_bytes && *in_out_tex_dim > 512) {
        *in_out_tex_dim /= 2;
        required_vram /= 4; /* 2D texture area is divided by 4 when dimensions halve */
        clamped = true;
    }
    return clamped;
}

const char* ov_resource_profile_to_string(ov_resource_profile_t profile) {
    switch (profile) {
        case OV_PROFILE_BALANCED:            return "Balanced (Standard)";
        case OV_PROFILE_PERFORMANCE:         return "High Performance";
        case OV_PROFILE_MAX_PERFORMANCE:     return "Maximum Throughput / Unconstrained";
        case OV_PROFILE_BATTERY_LOW_POWER:   return "Battery / Low Power Mode";
        default:                             return "Unknown Profile";
    }
}

const char* ov_resource_pressure_to_string(ov_resource_pressure_t pressure) {
    switch (pressure) {
        case OV_RESOURCE_PRESSURE_NORMAL:   return "Normal (<60% Committed)";
        case OV_RESOURCE_PRESSURE_MODERATE: return "Moderate (60-80% Committed)";
        case OV_RESOURCE_PRESSURE_CRITICAL: return "Critical (>80% Committed)";
        default:                            return "Unknown Pressure";
    }
}
