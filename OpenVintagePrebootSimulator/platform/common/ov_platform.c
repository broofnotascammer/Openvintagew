/**
 * OpenVintage Pre-Boot Architecture Simulator - Platform Abstraction Dispatcher
 */

#include "ov_platform.h"
#include "ov_logger.h"
#include <string.h>

ov_platform_type_t ov_platform_get_current(void) {
#if defined(__APPLE__)
    return OV_PLATFORM_MACOS;
#elif defined(__linux__)
    return OV_PLATFORM_LINUX;
#elif defined(_WIN32) || defined(_WIN64)
    return OV_PLATFORM_WINDOWS;
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    return OV_PLATFORM_BSD;
#else
    return OV_PLATFORM_UNKNOWN;
#endif
}

const char* ov_platform_get_name(void) {
    switch (ov_platform_get_current()) {
        case OV_PLATFORM_MACOS:   return "macOS (Darwin)";
        case OV_PLATFORM_LINUX:   return "Linux";
        case OV_PLATFORM_WINDOWS: return "Windows";
        case OV_PLATFORM_BSD:     return "BSD";
        default:                  return "Unknown Platform";
    }
}

ov_status_t ov_platform_detect_host(ov_hardware_profile_t *out_host) {
#if defined(__APPLE__)
    return ov_macos_detect_host(out_host);
#elif defined(__linux__)
    return ov_linux_detect_host(out_host);
#else
    if (!out_host) return OV_ERROR_INVALID_PARAM;
    memset(out_host, 0, sizeof(ov_hardware_profile_t));
    out_host->source = OV_HW_SOURCE_UNKNOWN;
    snprintf(out_host->model_identifier, sizeof(out_host->model_identifier), "Generic Host");
    return OV_SUCCESS;
#endif
}

ov_status_t ov_platform_detect_cpu(ov_cpu_info_t *out_cpu) {
#if defined(__APPLE__)
    return ov_macos_detect_cpu(out_cpu);
#elif defined(__linux__)
    return ov_linux_detect_cpu(out_cpu);
#else
    if (!out_cpu) return OV_ERROR_INVALID_PARAM;
    memset(out_cpu, 0, sizeof(ov_cpu_info_t));
    out_cpu->cores = 1;
    out_cpu->threads = 1;
    snprintf(out_cpu->model_name, sizeof(out_cpu->model_name), "Generic Host CPU");
    return OV_SUCCESS;
#endif
}

ov_status_t ov_platform_detect_memory(ov_memory_info_t *out_mem) {
#if defined(__APPLE__)
    return ov_macos_detect_memory(out_mem);
#elif defined(__linux__)
    return ov_linux_detect_memory(out_mem);
#else
    if (!out_mem) return OV_ERROR_INVALID_PARAM;
    memset(out_mem, 0, sizeof(ov_memory_info_t));
    out_mem->total_bytes = 4ULL * 1024 * 1024 * 1024;
    out_mem->available_bytes = 2ULL * 1024 * 1024 * 1024;
    return OV_SUCCESS;
#endif
}

ov_status_t ov_platform_detect_gpus(ov_gpu_topology_t *out_topo) {
#if defined(__APPLE__)
    return ov_macos_iokit_probe_gpus(out_topo);
#elif defined(__linux__)
    return ov_linux_detect_gpus(out_topo);
#else
    if (!out_topo) return OV_ERROR_INVALID_PARAM;
    memset(out_topo, 0, sizeof(ov_gpu_topology_t));
    out_topo->gpu_count = 1;
    out_topo->gpus[0].type = OV_GPU_SOFTWARE_RASTERIZER;
    snprintf(out_topo->gpus[0].model_name, sizeof(out_topo->gpus[0].model_name), "Generic Virtual Display Adapter");
    return OV_SUCCESS;
#endif
}
