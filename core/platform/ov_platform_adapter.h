#ifndef OV_PLATFORM_ADAPTER_H
#define OV_PLATFORM_ADAPTER_H

#include "ov_platform_types.h"
#include "../../ovir/gpu/ovir_gpu_backend.h"
#include "ov_platform.h"

typedef struct {
    ov_os_type_t os_type;
    const char *adapter_name;
    ov_status_t (*detect_system)(ov_platform_info_t *info);
    ov_status_t (*detect_cpu)(ov_platform_cpu_caps_t *cpu);
    ov_status_t (*detect_gpus)(ov_platform_gpu_device_t *devices, uint32_t *count);
    ov_status_t (*detect_gpu_caps)(ov_platform_gpu_caps_t *caps);
    ov_status_t (*register_backends)(ov_platform_backend_info_t *backends, uint32_t *count);
    ov_status_t (*alloc_wx)(size_t size, void **out_rw, void **out_rx);
    ov_status_t (*protect_wx)(void *ptr, size_t size, bool executable);
    ov_status_t (*free_wx)(void *ptr, size_t size);
    uint64_t    (*get_time_ns)(void);
    void        (*sleep_ms)(uint32_t ms);
    ov_status_t (*compile_and_execute_glsl)(const ovir_gpu_compiled_shader_t *shader, ov_platform_shader_execution_t *exec);
} ov_platform_adapter_t;

// Adapter declarations
const ov_platform_adapter_t *ov_adapter_macos_get(void);
const ov_platform_adapter_t *ov_adapter_linux_get(void);
const ov_platform_adapter_t *ov_adapter_windows_get(void);

#endif /* OV_PLATFORM_ADAPTER_H */
