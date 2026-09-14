/**
 * OpenVintage Pre-Boot Simulator - Native macOS Header
 * Platform detection using Darwin, Mach, sysctl, and IOKit.
 */

#ifndef OV_MACOS_NATIVE_H
#define OV_MACOS_NATIVE_H

#include "../common/ov_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Native macOS Sysctl Probing */
int ov_macos_sysctl_string(const char *name, char *buf, size_t buf_len);
int ov_macos_sysctl_uint32(const char *name, uint32_t *out_val);
int ov_macos_sysctl_uint64(const char *name, uint64_t *out_val);

/* Native IOKit GPU Enumeration */
ov_status_t ov_macos_iokit_probe_gpus(ov_gpu_topology_t *out_topo);

/* Native Metal Framework Capability Interrogation */
ov_status_t ov_macos_metal_probe(ov_gpu_info_t *gpu);

/* Native OpenGL Capability Interrogation */
ov_status_t ov_macos_opengl_probe(ov_gpu_info_t *gpu);

#ifdef __cplusplus
}
#endif

#endif /* OV_MACOS_NATIVE_H */
