/**
 * OpenVintage Pre-Boot Simulator - Core Orchestration Engine
 * Coordinates hardware discovery, capability analysis, workload resolution,
 * benchmark runs, and report generation across Phases 1-5.
 */

#ifndef OV_CORE_H
#define OV_CORE_H

#include "ov_types.h"
#include "ov_hardware.h"
#include "ov_resolver.h"
#include "ov_cpu_engine.h"
#include "ov_gpu_engine.h"
#include "ov_resource_manager.h"
#include "ov_unified_cache.h"
#include "ov_compatibility.h"
#include "ov_benchmark.h"
#include "ov_diagnostics.h"
#include "ov_memory.h"
#include "ov_logger.h"

/* Pre-Boot Execution Phase Tracking */
typedef enum {
    OV_PHASE_UNINITIALIZED = 0,
    OV_PHASE_MEMORY_INIT = 1,
    OV_PHASE_HARDWARE_DISCOVERY = 2,
    OV_PHASE_CAPABILITY_ANALYSIS = 3,
    OV_PHASE_OVIR_SUBSYSTEMS_INIT = 4,
    OV_PHASE_UNIFIED_RESOLVER_EVAL = 5,
    OV_PHASE_RESOURCE_ALLOCATION = 6,
    OV_PHASE_CACHE_WARMUP = 7,
    OV_PHASE_COMPATIBILITY_AUDIT = 8,
    OV_PHASE_BENCHMARK_VALIDATION = 9,
    OV_PHASE_READY = 10,
    OV_PHASE_ERROR = 11
} ov_boot_phase_t;

/* Core PreBoot Simulator State */
typedef struct {
    ov_boot_phase_t         current_phase;
    uint32_t                phase_progress_percent;
    char                    phase_message[128];
    bool                    initialized;
    bool                    boot_successful;

    ov_hardware_profile_t   active_hw_profile;
    ov_pci_device_t        *pci_devices;
    uint32_t                pci_device_count;

    ov_resolution_result_t  last_resolution;
    ov_resource_status_t    resource_status;
    ov_unified_cache_stats_t cache_stats;
    ov_benchmark_results_t  benchmark_results;
    ov_diagnostic_report_t  diagnostic_report;
} ov_core_t;

extern ov_core_t ov_core_instance;

/* Core Lifecycle APIs */
ov_status_t ov_core_init(void);
ov_status_t ov_core_execute_preboot(void);
void        ov_core_generate_report(const char *output_file);
void        ov_core_cleanup(void);

/* Individual Execution Phases */
ov_status_t ov_core_phase_detect_hardware(void);
ov_status_t ov_core_phase_analyze_capabilities(void);
ov_status_t ov_core_phase_resolve_workloads(void);
ov_status_t ov_core_phase_init_cache_and_resources(void);
ov_status_t ov_core_phase_run_benchmarks(void);
ov_status_t ov_core_phase_generate_report(void);

/* Helpers */
const char* ov_boot_phase_to_string(ov_boot_phase_t phase);
ov_core_t*  ov_core_get_instance(void);

#endif /* OV_CORE_H */
