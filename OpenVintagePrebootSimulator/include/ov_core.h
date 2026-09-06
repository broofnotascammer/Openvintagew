#ifndef OV_CORE_H
#define OV_CORE_H

#include "ov_types.h"
#include "ov_hardware.h"
#include "ov_resolver.h"
#include "ov_memory.h"
#include "ov_logger.h"

/* Core PreBoot Simulator */
typedef struct {
    ov_cpu_info_t cpu_info;
    ov_gpu_info_t gpu_info;
    ov_memory_info_t mem_info;
    ov_pci_device_t *pci_devices;
    uint32_t pci_device_count;
    bool initialized;
} ov_core_t;

extern ov_core_t ov_core_instance;

/* Core initialization and execution */
ov_status_t ov_core_init(void);
ov_status_t ov_core_execute_preboot(void);
ovoid ov_core_generate_report(const char *output_file);
ovoid ov_core_cleanup(void);

/* Preboot phases */
ov_status_t ov_core_phase_detect_hardware(void);
ov_status_t ov_core_phase_analyze_capabilities(void);
ov_status_t ov_core_phase_resolve_workloads(void);
ov_status_t ov_core_phase_generate_report(void);

#endif /* OV_CORE_H */
