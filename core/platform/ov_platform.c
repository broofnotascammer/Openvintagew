#include "ov_platform.h"
#include "ov_platform_adapter.h"
#include <string.h>
#include <stdio.h>

static ov_platform_info_t s_platform_info = {0};
static const ov_platform_adapter_t *s_active_adapter = NULL;
static bool s_platform_initialized = false;

const char *ov_platform_os_name(ov_os_type_t os) {
    switch (os) {
        case OV_OS_MACOS: return "macOS";
        case OV_OS_LINUX: return "Linux";
        case OV_OS_WINDOWS: return "Windows";
        default: return "Unknown OS";
    }
}

const char *ov_shader_exec_lifecycle_name(ov_shader_exec_lifecycle_state_t state) {
    switch (state) {
        case OV_SHADER_EXEC_STATE_UNINITIALIZED: return "UNINITIALIZED";
        case OV_SHADER_EXEC_STATE_PARSED: return "PARSED";
        case OV_SHADER_EXEC_STATE_VALIDATED: return "VALIDATED";
        case OV_SHADER_EXEC_STATE_IR_GENERATED: return "IR_GENERATED";
        case OV_SHADER_EXEC_STATE_OPTIMIZED: return "OPTIMIZED";
        case OV_SHADER_EXEC_STATE_TRANSLATED: return "TRANSLATED";
        case OV_SHADER_EXEC_STATE_DRIVER_COMPILED: return "DRIVER_COMPILED";
        case OV_SHADER_EXEC_STATE_LINKED: return "LINKED";
        case OV_SHADER_EXEC_STATE_EXECUTED: return "EXECUTED";
        case OV_SHADER_EXEC_STATE_FAILED: return "FAILED";
        default: return "UNKNOWN";
    }
}

const char *ov_platform_error_string(ov_platform_error_t err) {
    switch (err) {
        case OV_PLATFORM_OK: return "OK";
        case OV_PLATFORM_ERR_UNSUPPORTED: return "PLATFORM_UNSUPPORTED";
        case OV_PLATFORM_ERR_API_UNAVAILABLE: return "API_UNAVAILABLE";
        case OV_PLATFORM_ERR_DEVICE_NOT_FOUND: return "DEVICE_NOT_FOUND";
        case OV_PLATFORM_ERR_DRIVER_INIT_FAILED: return "DRIVER_INITIALIZATION_FAILED";
        case OV_PLATFORM_ERR_CAPABILITY_MISSING: return "CAPABILITY_MISSING";
        case OV_PLATFORM_ERR_MEM_ALLOC_FAILED: return "MEMORY_ALLOCATION_FAILED";
        case OV_PLATFORM_ERR_GRAPHICS_COMPILE_FAILED: return "GRAPHICS_COMPILATION_FAILED";
        case OV_PLATFORM_ERR_GRAPHICS_LINK_FAILED: return "GRAPHICS_LINK_FAILED";
        case OV_PLATFORM_ERR_EXECUTION_FAILED: return "EXECUTION_FAILED";
        case OV_PLATFORM_ERR_RESOURCE_LIMIT: return "RESOURCE_LIMIT_EXCEEDED";
        case OV_PLATFORM_ERR_BACKEND_INIT_FAILED: return "BACKEND_INITIALIZATION_FAILED";
        default: return "UNKNOWN_PLATFORM_ERROR";
    }
}

static void select_default_adapter(void) {
#if defined(__APPLE__)
    s_active_adapter = ov_adapter_macos_get();
#elif defined(_WIN32) || defined(_WIN64)
    s_active_adapter = ov_adapter_windows_get();
#elif defined(__linux__)
    s_active_adapter = ov_adapter_linux_get();
#else
    s_active_adapter = ov_adapter_linux_get();
#endif
}

ov_status_t ov_platform_init(void) {
    if (s_platform_initialized) return OV_SUCCESS;
    memset(&s_platform_info, 0, sizeof(ov_platform_info_t));

    select_default_adapter();
    if (!s_active_adapter) return OV_ERR_UNSUPPORTED;

    // 1. Detect OS & System
    s_active_adapter->detect_system(&s_platform_info);

    // 2. Detect CPU
    s_active_adapter->detect_cpu(&s_platform_info.cpu);

    // 3. Detect GPUs
    s_active_adapter->detect_gpus(s_platform_info.gpu_devices, &s_platform_info.gpu_device_count);

    // 4. Detect GPU Capabilities
    s_active_adapter->detect_gpu_caps(&s_platform_info.gpu_caps);

    // 5. Register Available Backends
    s_active_adapter->register_backends(s_platform_info.backends, &s_platform_info.backend_count);

    s_platform_initialized = true;
    return OV_SUCCESS;
}

const ov_platform_info_t *ov_platform_get_info(void) {
    if (!s_platform_initialized) {
        ov_platform_init();
    }
    return &s_platform_info;
}

void ov_platform_populate_host_env(ov_host_env_t *out_host) {
    if (!out_host) return;
    if (!s_platform_initialized) {
        ov_platform_init();
    }

    memset(out_host, 0, sizeof(ov_host_env_t));
    out_host->host_cpu_arch = s_platform_info.cpu.arch;
    out_host->host_cpu_features = s_platform_info.cpu.supported_features;
    out_host->host_cpu_cores = s_platform_info.cpu.physical_cores;
    out_host->host_ram_mb = s_platform_info.total_ram_mb;

    // Select primary GPU backend and populate capabilities
    out_host->gpu_caps.backend = OV_GFX_BACKEND_OPENGL_CORE;
    for (uint32_t i = 0; i < s_platform_info.backend_count; i++) {
        if (s_platform_info.backends[i].api == OV_GFX_API_METAL && s_platform_info.backends[i].is_available) {
            out_host->gpu_caps.backend = OV_GFX_BACKEND_METAL_NATIVE;
            out_host->gpu_caps.has_metal = true;
        }
        if (s_platform_info.backends[i].api == OV_GFX_API_VULKAN && s_platform_info.backends[i].is_available) {
            out_host->gpu_caps.has_moltenvk = true;
        }
        if (s_platform_info.backends[i].api == OV_GFX_API_OPENGL && s_platform_info.backends[i].is_available) {
            out_host->gpu_caps.has_opengl = true;
        }
    }

    out_host->gpu_caps.supported_features = OV_GPU_FEAT_COMPUTE | OV_GPU_FEAT_TESSELLATION | OV_GPU_FEAT_BC_COMPRESSION;
    out_host->gpu_caps.partial_features = OV_GPU_FEAT_GEOMETRY_SHADER;
    out_host->gpu_caps.available_vram_mb = (s_platform_info.gpu_device_count > 0)
        ? (s_platform_info.gpu_devices[0].dedicated_vram_mb > 0
            ? s_platform_info.gpu_devices[0].dedicated_vram_mb
            : s_platform_info.gpu_devices[0].shared_vram_mb)
        : 1024;
    out_host->gpu_caps.cpu_fallback_supported = true;
}

void ov_platform_cleanup(void) {
    s_platform_initialized = false;
    memset(&s_platform_info, 0, sizeof(ov_platform_info_t));
}

ov_status_t ov_platform_detect_cpu(ov_platform_cpu_caps_t *out_cpu) {
    if (!out_cpu) return OV_ERR_INVALID_PARAM;
    if (!s_active_adapter) select_default_adapter();
    return s_active_adapter->detect_cpu(out_cpu);
}

ov_status_t ov_platform_detect_gpus(ov_platform_gpu_device_t *devices, uint32_t *out_count) {
    if (!devices || !out_count) return OV_ERR_INVALID_PARAM;
    if (!s_active_adapter) select_default_adapter();
    return s_active_adapter->detect_gpus(devices, out_count);
}

ov_status_t ov_platform_detect_gpu_capabilities(ov_platform_gpu_caps_t *out_caps) {
    if (!out_caps) return OV_ERR_INVALID_PARAM;
    if (!s_active_adapter) select_default_adapter();
    return s_active_adapter->detect_gpu_caps(out_caps);
}

ov_status_t ov_platform_register_backends(ov_platform_backend_info_t *backends, uint32_t *out_count) {
    if (!backends || !out_count) return OV_ERR_INVALID_PARAM;
    if (!s_active_adapter) select_default_adapter();
    return s_active_adapter->register_backends(backends, out_count);
}

ov_status_t ov_platform_alloc_wx(size_t size, void **out_rw_ptr, void **out_rx_ptr) {
    if (!s_active_adapter) select_default_adapter();
    return s_active_adapter->alloc_wx(size, out_rw_ptr, out_rx_ptr);
}

ov_status_t ov_platform_protect_wx(void *ptr, size_t size, bool executable) {
    if (!s_active_adapter) select_default_adapter();
    return s_active_adapter->protect_wx(ptr, size, executable);
}

ov_status_t ov_platform_free_wx(void *ptr, size_t size) {
    if (!s_active_adapter) select_default_adapter();
    return s_active_adapter->free_wx(ptr, size);
}

uint64_t ov_platform_get_time_ns(void) {
    if (!s_active_adapter) select_default_adapter();
    return s_active_adapter->get_time_ns();
}

void ov_platform_sleep_ms(uint32_t ms) {
    if (!s_active_adapter) select_default_adapter();
    s_active_adapter->sleep_ms(ms);
}

ov_status_t ov_platform_graphics_execute_glsl(
    const ovir_gpu_compiled_shader_t *glsl_shader,
    ov_platform_shader_execution_t *out_execution
) {
    if (!s_active_adapter) select_default_adapter();
    return s_active_adapter->compile_and_execute_glsl(glsl_shader, out_execution);
}
