/**
 * OpenVintage Pre-Boot Architecture Simulator - Platform Abstraction Interface
 * Isolates Linux, macOS, and profile-based hardware probing behind clean interfaces.
 */

#ifndef OV_PLATFORM_H
#define OV_PLATFORM_H

#include "ov_types.h"
#include "ov_hardware.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    OV_PLATFORM_UNKNOWN = 0,
    OV_PLATFORM_LINUX   = 1,
    OV_PLATFORM_MACOS   = 2,
    OV_PLATFORM_BSD     = 3,
    OV_PLATFORM_WINDOWS = 4
} ov_platform_type_t;

ov_platform_type_t ov_platform_get_current(void);
const char*        ov_platform_get_name(void);

/* Common Hardware Probing Dispatch */
ov_status_t ov_platform_detect_host(ov_hardware_profile_t *out_host);
ov_status_t ov_platform_detect_cpu(ov_cpu_info_t *out_cpu);
ov_status_t ov_platform_detect_memory(ov_memory_info_t *out_mem);
ov_status_t ov_platform_detect_gpus(ov_gpu_topology_t *out_topo);
ov_status_t ov_platform_detect_pci(ov_pci_device_t **out_devs, uint32_t *out_count);
ov_status_t ov_platform_detect_os_info(char *os_name, size_t os_len,
                                      char *os_build, size_t build_len,
                                      uint32_t *kernel_major, uint32_t *kernel_minor);

/* Linux specific native probing */
ov_status_t ov_linux_detect_host(ov_hardware_profile_t *out_host);
ov_status_t ov_linux_detect_cpu(ov_cpu_info_t *out_cpu);
ov_status_t ov_linux_detect_memory(ov_memory_info_t *out_mem);
ov_status_t ov_linux_detect_gpus(ov_gpu_topology_t *out_topo);
ov_status_t ov_linux_detect_pci(ov_pci_device_t **out_devs, uint32_t *out_count);

/* macOS specific native probing */
ov_status_t ov_macos_detect_host(ov_hardware_profile_t *out_host);
ov_status_t ov_macos_detect_cpu(ov_cpu_info_t *out_cpu);
ov_status_t ov_macos_detect_memory(ov_memory_info_t *out_mem);
ov_status_t ov_macos_detect_gpus(ov_gpu_topology_t *out_topo);
ov_status_t ov_macos_detect_os_info(char *os_name, size_t os_len,
                                    char *os_build, size_t build_len,
                                    uint32_t *kernel_major, uint32_t *kernel_minor);

#ifdef __cplusplus
}
#endif

#endif /* OV_PLATFORM_H */
