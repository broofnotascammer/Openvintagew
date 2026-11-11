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

static ov_discovery_mode_t s_discovery_mode = OV_DISCOVERY_MODE_REAL;
static char s_mock_profile_name[64] = "MacBookPro9,1";

const char *ov_discovery_mode_name(ov_discovery_mode_t mode) {
    switch (mode) {
        case OV_DISCOVERY_MODE_REAL: return "REAL_HARDWARE";
        case OV_DISCOVERY_MODE_MOCK_PROFILE: return "MOCK_PROFILE";
        default: return "UNKNOWN_MODE";
    }
}

const char *ov_cap_state_name(ov_cap_state_t state) {
    switch (state) {
        case OV_CAP_STATE_UNKNOWN: return "UNKNOWN";
        case OV_CAP_STATE_DETECTED: return "DETECTED";
        case OV_CAP_STATE_SUPPORTED: return "SUPPORTED";
        case OV_CAP_STATE_AVAILABLE: return "AVAILABLE";
        case OV_CAP_STATE_INITIALIZED: return "INITIALIZED";
        case OV_CAP_STATE_VERIFIED: return "VERIFIED";
        case OV_CAP_STATE_UNAVAILABLE: return "UNAVAILABLE";
        default: return "UNKNOWN";
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

ov_status_t ov_platform_apply_mock_profile(const char *profile_name, ov_platform_info_t *out_info) {
    if (!out_info) return OV_ERR_INVALID_PARAM;
    memset(out_info, 0, sizeof(ov_platform_info_t));

    const char *pname = (profile_name && strlen(profile_name) > 0) ? profile_name : "MacBookPro9,1";
    strncpy(out_info->profile_name, pname, sizeof(out_info->profile_name) - 1);
    out_info->discovery_mode = OV_DISCOVERY_MODE_MOCK_PROFILE;
    out_info->is_simulated_profile = true;

    // Standard MacBookPro9,1 validated development profile
    out_info->os_type = OV_OS_MACOS;
    strncpy(out_info->os_name, "macOS", sizeof(out_info->os_name) - 1);
    strncpy(out_info->os_version, "macOS Catalina 10.15.8 (Build 19H2036)", sizeof(out_info->os_version) - 1);
    strncpy(out_info->kernel_version, "Darwin 19.6.0 (x86_64)", sizeof(out_info->kernel_version) - 1);
    out_info->total_ram_mb = 8192;
    out_info->available_ram_mb = 5840;
    out_info->page_size = 4096;
    out_info->init_state = OV_PLATFORM_INIT_READY;

    // CPU specs
    strncpy(out_info->cpu.vendor, "GenuineIntel", sizeof(out_info->cpu.vendor) - 1);
    strncpy(out_info->cpu.model_name, "Intel(R) Core(TM) i7-3615QM CPU @ 2.30GHz", sizeof(out_info->cpu.model_name) - 1);
    out_info->cpu.arch = OV_ARCH_X86_64;
    out_info->cpu.physical_cores = 4;
    out_info->cpu.logical_threads = 8;
    out_info->cpu.sse_supported = true;
    out_info->cpu.sse2_supported = true;
    out_info->cpu.sse3_supported = true;
    out_info->cpu.ssse3_supported = true;
    out_info->cpu.sse4_1_supported = true;
    out_info->cpu.sse4_2_supported = true;
    out_info->cpu.avx_supported = true;
    out_info->cpu.avx2_supported = false; // Ivy Bridge lacks AVX2
    out_info->cpu.neon_supported = false;
    out_info->cpu.osxsave_supported = true;
    out_info->cpu.os_vector_enabled = true;
    out_info->cpu.supported_features = OV_CPU_FEAT_AVX | OV_CPU_FEAT_FMA | OV_CPU_FEAT_ATOMICS;

    // Dual GPU: Integrated Intel HD 4000 + Discrete NVIDIA GT 650M Kepler
    out_info->gpu_device_count = 2;

    // Device 0: Integrated
    out_info->gpu_devices[0].device_index = 0;
    strncpy(out_info->gpu_devices[0].vendor_name, "Intel", sizeof(out_info->gpu_devices[0].vendor_name) - 1);
    strncpy(out_info->gpu_devices[0].device_name, "Intel HD Graphics 4000", sizeof(out_info->gpu_devices[0].device_name) - 1);
    out_info->gpu_devices[0].pci_vendor_id = 0x8086;
    out_info->gpu_devices[0].pci_device_id = 0x0166;
    out_info->gpu_devices[0].dedicated_vram_mb = 0;
    out_info->gpu_devices[0].shared_vram_mb = 1536;
    out_info->gpu_devices[0].is_discrete = false;
    out_info->gpu_devices[0].runtime_state = OV_GPU_STATE_CONTEXT_READY;
    out_info->gpu_devices[0].cap_state = OV_CAP_STATE_AVAILABLE;
    out_info->gpu_devices[0].supports_opengl_33 = true;
    out_info->gpu_devices[0].supports_metal = true;
    out_info->gpu_devices[0].supports_vulkan = true;

    // Device 1: Discrete
    out_info->gpu_devices[1].device_index = 1;
    strncpy(out_info->gpu_devices[1].vendor_name, "NVIDIA", sizeof(out_info->gpu_devices[1].vendor_name) - 1);
    strncpy(out_info->gpu_devices[1].device_name, "NVIDIA GeForce GT 650M", sizeof(out_info->gpu_devices[1].device_name) - 1);
    out_info->gpu_devices[1].pci_vendor_id = 0x10de;
    out_info->gpu_devices[1].pci_device_id = 0x0fd5;
    out_info->gpu_devices[1].dedicated_vram_mb = 1024;
    out_info->gpu_devices[1].shared_vram_mb = 0;
    out_info->gpu_devices[1].is_discrete = true;
    out_info->gpu_devices[1].runtime_state = OV_GPU_STATE_CONTEXT_READY;
    out_info->gpu_devices[1].cap_state = OV_CAP_STATE_AVAILABLE;
    out_info->gpu_devices[1].supports_opengl_33 = true;
    out_info->gpu_devices[1].supports_metal = true;
    out_info->gpu_devices[1].supports_vulkan = true;

    // Caps
    out_info->gpu_caps.stage_mask = 0x1F;
    out_info->gpu_caps.glsl_version = 330;
    out_info->gpu_caps.compute_supported = true;
    out_info->gpu_caps.tessellation_supported = true;
    out_info->gpu_caps.geometry_supported = true;
    strncpy(out_info->gpu_caps.driver_vendor, "Apple Inc. / NVIDIA Corporation", sizeof(out_info->gpu_caps.driver_vendor) - 1);
    strncpy(out_info->gpu_caps.driver_renderer, "NVIDIA GeForce GT 650M OpenGL Engine", sizeof(out_info->gpu_caps.driver_renderer) - 1);
    strncpy(out_info->gpu_caps.driver_version, "2.1 NVIDIA-14.0.32", sizeof(out_info->gpu_caps.driver_version) - 1);
    out_info->gpu_caps.detection_complete = true;

    // Backends
    const ov_platform_adapter_t *mac_adapter = ov_adapter_macos_get();
    if (mac_adapter) {
        mac_adapter->register_backends(out_info->backends, &out_info->backend_count);
    }

    return OV_SUCCESS;
}

ov_status_t ov_platform_set_discovery_mode(ov_discovery_mode_t mode, const char *profile_name) {
    s_discovery_mode = mode;
    if (profile_name && strlen(profile_name) > 0) {
        strncpy(s_mock_profile_name, profile_name, sizeof(s_mock_profile_name) - 1);
    }
    // Re-initialize platform with requested mode
    s_platform_initialized = false;
    return ov_platform_init();
}

ov_discovery_mode_t ov_platform_get_discovery_mode(void) {
    return s_discovery_mode;
}

ov_status_t ov_platform_init(void) {
    if (s_platform_initialized) return OV_SUCCESS;
    memset(&s_platform_info, 0, sizeof(ov_platform_info_t));

    if (s_discovery_mode == OV_DISCOVERY_MODE_MOCK_PROFILE) {
        ov_status_t mst = ov_platform_apply_mock_profile(s_mock_profile_name, &s_platform_info);
        if (mst == OV_SUCCESS) {
            s_platform_initialized = true;
            return OV_SUCCESS;
        }
    }

    select_default_adapter();
    if (!s_active_adapter) return OV_ERR_UNSUPPORTED;

    s_platform_info.discovery_mode = OV_DISCOVERY_MODE_REAL;
    s_platform_info.is_simulated_profile = false;
    strncpy(s_platform_info.profile_name, "Native Host Hardware", sizeof(s_platform_info.profile_name) - 1);

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

    // Intelligent multi-GPU selection: prefer discrete GPU with highest dedicated VRAM, else integrated with shared
    uint64_t best_vram = 0;
    if (s_platform_info.gpu_device_count > 0) {
        int best_idx = -1;
        // Priority 1: discrete GPU
        for (uint32_t i = 0; i < s_platform_info.gpu_device_count; i++) {
            if (s_platform_info.gpu_devices[i].is_discrete && s_platform_info.gpu_devices[i].dedicated_vram_mb > best_vram) {
                best_vram = s_platform_info.gpu_devices[i].dedicated_vram_mb;
                best_idx = (int)i;
            }
        }
        // Priority 2: integrated GPU if no discrete
        if (best_idx < 0) {
            for (uint32_t i = 0; i < s_platform_info.gpu_device_count; i++) {
                uint64_t vram = s_platform_info.gpu_devices[i].dedicated_vram_mb > 0
                    ? s_platform_info.gpu_devices[i].dedicated_vram_mb
                    : s_platform_info.gpu_devices[i].shared_vram_mb;
                if (vram > best_vram) {
                    best_vram = vram;
                    best_idx = (int)i;
                }
            }
        }
        out_host->gpu_caps.available_vram_mb = (best_vram > 0) ? best_vram : 1024;
    } else {
        out_host->gpu_caps.available_vram_mb = 1024;
    }

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
