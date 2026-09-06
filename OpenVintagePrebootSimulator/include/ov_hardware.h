#ifndef OV_HARDWARE_H
#define OV_HARDWARE_H

#include "ov_types.h"

/* Hardware detection functions */
ov_status_t ov_hardware_init(void);
ov_status_t ov_hardware_detect_cpu(ov_cpu_info_t *cpu_info);
ov_status_t ov_hardware_detect_gpu(ov_gpu_info_t *gpu_info);
ov_status_t ov_hardware_detect_memory(ov_memory_info_t *mem_info);
ov_status_t ov_hardware_detect_pci(ov_pci_device_t **devices, uint32_t *device_count);
ovid ov_hardware_cleanup(void);

/* CPUID wrapper for Intel detection */
typedef struct {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
} cpuid_regs_t;

cpuid_regs_t ov_cpuid(uint32_t leaf, uint32_t subleaf);

/* CPU feature detection */
bool ov_cpu_supports_sse42(void);
bool ov_cpu_supports_avx(void);
bool ov_cpu_supports_avx2(void);
bool ov_cpu_supports_tsc(void);

#endif /* OV_HARDWARE_H */
