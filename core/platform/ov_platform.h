#ifndef OV_PLATFORM_H
#define OV_PLATFORM_H

#include "ov_platform_types.h"
#include "../resolver/ov_resolver.h"
#include "../../ovir/gpu/ovir_gpu_backend.h"

// Lifecycle & Detection
ov_status_t ov_platform_init(void);
const ov_platform_info_t *ov_platform_get_info(void);
void ov_platform_populate_host_env(ov_host_env_t *out_host);
void ov_platform_cleanup(void);

// Discovery Mode & Mock Profiles (Phase 15.1)
ov_status_t ov_platform_set_discovery_mode(ov_discovery_mode_t mode, const char *profile_name);
ov_discovery_mode_t ov_platform_get_discovery_mode(void);
ov_status_t ov_platform_apply_mock_profile(const char *profile_name, ov_platform_info_t *out_info);

// CPU Detection
ov_status_t ov_platform_detect_cpu(ov_platform_cpu_caps_t *out_cpu);

// GPU Detection & Backend Registry
ov_status_t ov_platform_detect_gpus(ov_platform_gpu_device_t *devices, uint32_t *out_count);
ov_status_t ov_platform_detect_gpu_capabilities(ov_platform_gpu_caps_t *out_caps);
ov_status_t ov_platform_register_backends(ov_platform_backend_info_t *backends, uint32_t *out_count);

// Memory Management Interface
ov_status_t ov_platform_alloc_wx(size_t size, void **out_rw_ptr, void **out_rx_ptr);
ov_status_t ov_platform_protect_wx(void *ptr, size_t size, bool executable);
ov_status_t ov_platform_free_wx(void *ptr, size_t size);

// Threading & Timing Interface
uint64_t ov_platform_get_time_ns(void);
void ov_platform_sleep_ms(uint32_t ms);

// Graphics Execution Interface (Phase 14 GLSL 330 Execution Integration)
typedef struct {
    ov_shader_exec_lifecycle_state_t lifecycle_state;
    bool driver_compiled;
    bool linked;
    bool executed;
    uint32_t shader_gl_handle;
    uint32_t program_gl_handle;
    char driver_compile_log[512];
    char driver_link_log[512];
    uint64_t driver_compile_time_ns;
    uint64_t driver_link_time_ns;
    uint64_t execution_time_ns;
} ov_platform_shader_execution_t;

ov_status_t ov_platform_graphics_execute_glsl(
    const ovir_gpu_compiled_shader_t *glsl_shader,
    ov_platform_shader_execution_t *out_execution
);

#endif /* OV_PLATFORM_H */
