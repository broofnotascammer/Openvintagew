#ifndef OV_PLATFORM_TYPES_H
#define OV_PLATFORM_TYPES_H

#include "../types/ov_types.h"

typedef enum {
    OV_OS_UNKNOWN = 0,
    OV_OS_MACOS,
    OV_OS_LINUX,
    OV_OS_WINDOWS
} ov_os_type_t;

typedef enum {
    OV_PLATFORM_INIT_UNINITIALIZED = 0,
    OV_PLATFORM_INIT_INITIALIZING,
    OV_PLATFORM_INIT_READY,
    OV_PLATFORM_INIT_FAILED
} ov_platform_init_state_t;

typedef enum {
    OV_DISCOVERY_MODE_REAL = 0,     // Genuine hardware detection (CPUID, /proc, sysctl, DRM)
    OV_DISCOVERY_MODE_MOCK_PROFILE  // Explicitly requested simulated profile (e.g. MacBookPro9,1)
} ov_discovery_mode_t;

typedef enum {
    OV_CAP_STATE_UNKNOWN = 0,
    OV_CAP_STATE_DETECTED,
    OV_CAP_STATE_SUPPORTED,
    OV_CAP_STATE_AVAILABLE,
    OV_CAP_STATE_INITIALIZED,
    OV_CAP_STATE_VERIFIED,
    OV_CAP_STATE_UNAVAILABLE
} ov_cap_state_t;

typedef enum {
    OV_GPU_STATE_UNKNOWN = 0,
    OV_GPU_STATE_DISCOVERED,
    OV_GPU_STATE_API_INITIALIZED,
    OV_GPU_STATE_CONTEXT_READY,
    OV_GPU_STATE_UNAVAILABLE
} ov_gpu_runtime_state_t;

typedef enum {
    OV_SHADER_EXEC_STATE_UNINITIALIZED = 0,
    OV_SHADER_EXEC_STATE_PARSED,
    OV_SHADER_EXEC_STATE_VALIDATED,
    OV_SHADER_EXEC_STATE_IR_GENERATED,
    OV_SHADER_EXEC_STATE_OPTIMIZED,
    OV_SHADER_EXEC_STATE_TRANSLATED,
    OV_SHADER_EXEC_STATE_DRIVER_COMPILED,
    OV_SHADER_EXEC_STATE_LINKED,
    OV_SHADER_EXEC_STATE_EXECUTED,
    OV_SHADER_EXEC_STATE_FAILED
} ov_shader_exec_lifecycle_state_t;

// Platform error codes
typedef enum {
    OV_PLATFORM_OK = 0,
    OV_PLATFORM_ERR_UNSUPPORTED = -101,
    OV_PLATFORM_ERR_API_UNAVAILABLE = -102,
    OV_PLATFORM_ERR_DEVICE_NOT_FOUND = -103,
    OV_PLATFORM_ERR_DRIVER_INIT_FAILED = -104,
    OV_PLATFORM_ERR_CAPABILITY_MISSING = -105,
    OV_PLATFORM_ERR_MEM_ALLOC_FAILED = -106,
    OV_PLATFORM_ERR_GRAPHICS_COMPILE_FAILED = -107,
    OV_PLATFORM_ERR_GRAPHICS_LINK_FAILED = -108,
    OV_PLATFORM_ERR_EXECUTION_FAILED = -109,
    OV_PLATFORM_ERR_RESOURCE_LIMIT = -110,
    OV_PLATFORM_ERR_BACKEND_INIT_FAILED = -111
} ov_platform_error_t;

#define OV_MAX_BACKENDS 8
#define OV_MAX_GPU_DEVICES 4

typedef struct {
    char backend_name[64];
    ov_os_type_t platform_os;
    ov_gfx_api_t api;
    ov_gfx_backend_t backend_id;
    bool is_available;
    bool device_detected;
    bool is_initialized;
    uint32_t supported_shader_stage_mask;
    char driver_version[64];
    char glsl_version[32];
    char known_limitations[128];
} ov_platform_backend_info_t;

typedef struct {
    uint32_t device_index;
    char vendor_name[64];
    char device_name[64];
    uint32_t pci_vendor_id;
    uint32_t pci_device_id;
    char drm_node_path[64];
    uint64_t dedicated_vram_mb;
    uint64_t shared_vram_mb;
    bool is_discrete;
    ov_gpu_runtime_state_t runtime_state;
    ov_cap_state_t cap_state;
    bool supports_opengl_33;
    bool supports_metal;
    bool supports_vulkan;
    bool supports_d3d;
} ov_platform_gpu_device_t;

typedef struct {
    char vendor[32];
    char model_name[64];
    ov_arch_t arch;
    uint32_t physical_cores;
    uint32_t logical_threads;
    uint32_t supported_features; // bitmask of OV_CPU_FEAT_*
    bool sse_supported;
    bool sse2_supported;
    bool sse3_supported;
    bool ssse3_supported;
    bool sse4_1_supported;
    bool sse4_2_supported;
    bool avx_supported;
    bool avx2_supported;
    bool neon_supported;
    bool osxsave_supported;   // CPUID leaf 1 ECX bit 27
    bool os_vector_enabled;    // Verified via XGETBV (XCR0[2:1] == 0x6) or OSXSAVE
} ov_platform_cpu_caps_t;

typedef struct {
    uint32_t stage_mask;
    uint32_t glsl_version;
    uint32_t max_workgroup_size[3];
    uint32_t max_uniform_buffer_bindings;
    uint32_t max_texture_units;
    bool compute_supported;
    bool tessellation_supported;
    bool geometry_supported;
    char driver_vendor[64];
    char driver_renderer[64];
    char driver_version[64];
    bool detection_complete;
} ov_platform_gpu_caps_t;

typedef struct {
    ov_os_type_t os_type;
    char os_name[64];
    char os_version[64];
    char kernel_version[64];
    ov_discovery_mode_t discovery_mode;
    bool is_simulated_profile;
    char profile_name[64];
    ov_platform_init_state_t init_state;
    ov_platform_cpu_caps_t cpu;
    ov_platform_gpu_device_t gpu_devices[OV_MAX_GPU_DEVICES];
    uint32_t gpu_device_count;
    ov_platform_gpu_caps_t gpu_caps;
    ov_platform_backend_info_t backends[OV_MAX_BACKENDS];
    uint32_t backend_count;
    uint64_t total_ram_mb;
    uint64_t available_ram_mb;
    uint64_t page_size;
} ov_platform_info_t;

const char *ov_platform_os_name(ov_os_type_t os);
const char *ov_discovery_mode_name(ov_discovery_mode_t mode);
const char *ov_cap_state_name(ov_cap_state_t state);
const char *ov_shader_exec_lifecycle_name(ov_shader_exec_lifecycle_state_t state);
const char *ov_platform_error_string(ov_platform_error_t err);

#endif /* OV_PLATFORM_TYPES_H */
