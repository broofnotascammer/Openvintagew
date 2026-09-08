#include "ov_core.h"
#include <stdio.h>
#include <time.h>

ov_core_t ov_core_instance = {0};

ov_status_t ov_core_init(void) {
    ov_log_info("\n========================================");
    ov_log_info("OpenVintage PreBoot Simulator v1.0");
    ov_log_info("========================================\n");
    
    ov_status_t status;
    
    /* Initialize memory subsystem */
    status = ov_memory_init(16 * 1024 * 1024);
    if (status != OV_SUCCESS) {
        ov_log_error("Failed to initialize memory");
        return status;
    }
    
    /* Initialize hardware detection */
    status = ov_hardware_init();
    if (status != OV_SUCCESS) {
        ov_log_error("Failed to initialize hardware detection");
        return status;
    }
    
    ov_core_instance.initialized = true;
    return OV_SUCCESS;
}

ov_status_t ov_core_phase_detect_hardware(void) {
    ov_log_info("\n--- Phase 1: Hardware Detection ---");
    
    ov_status_t status;
    
    status = ov_hardware_detect_cpu(&ov_core_instance.cpu_info);
    if (status != OV_SUCCESS) return status;
    
    status = ov_hardware_detect_gpu(&ov_core_instance.gpu_info);
    if (status != OV_SUCCESS) return status;
    
    status = ov_hardware_detect_memory(&ov_core_instance.mem_info);
    if (status != OV_SUCCESS) return status;
    
    status = ov_hardware_detect_pci(&ov_core_instance.pci_devices,
                                    &ov_core_instance.pci_device_count);
    
    return OV_SUCCESS;
}

ov_status_t ov_core_phase_analyze_capabilities(void) {
    ov_log_info("\n--- Phase 2: Capability Analysis ---");
    
    ov_resolver_analyze_cpu(&ov_core_instance.cpu_info);
    ov_resolver_analyze_gpu(&ov_core_instance.gpu_info);
    
    ov_log_info("\n=== Memory Analysis ===");
    ov_log_info("Total Memory: %lu MB", 
               ov_core_instance.mem_info.total_bytes / (1024 * 1024));
    ov_log_info("Available Memory: %lu MB",
               ov_core_instance.mem_info.available_bytes / (1024 * 1024));
    ov_log_info("NUMA Nodes: %u, Channels: %u",
               ov_core_instance.mem_info.numa_nodes,
               ov_core_instance.mem_info.channels);
    
    return OV_SUCCESS;
}

ov_status_t ov_core_phase_resolve_workloads(void) {
    ov_log_info("\n--- Phase 3: Workload Resolution ---");
    
    /* Initialize resolver with detected hardware */
    ov_resolver_init(&ov_core_instance.cpu_info,
                     &ov_core_instance.gpu_info,
                     &ov_core_instance.mem_info);
    
    /* Test various workload types */
    ov_workload_t workloads[] = {
        {OV_WORKLOAD_COMPUTE, 1, 5000, false, true},
        {OV_WORKLOAD_GRAPHICS, 2, 5000, true, false},
        {OV_WORKLOAD_MEMORY, 1, 3000, false, true},
        {OV_WORKLOAD_IO, 1, 4000, false, false},
        {OV_WORKLOAD_MIXED, 2, 6000, true, true}
    };
    
    const char *workload_names[] = {"Compute", "Graphics", "Memory", "I/O", "Mixed"};
    
    for (int i = 0; i < 5; i++) {
        ov_log_info("\nTesting %s workload:", workload_names[i]);
        ov_resolution_t decision = ov_resolver_route(&workloads[i]);
        ov_resolver_print_decision(&decision);
    }
    
    return OV_SUCCESS;
}

ov_status_t ov_core_phase_generate_report(void) {
    ov_log_info("\n--- Phase 4: Report Generation ---");
    ov_memory_print_stats();
    return OV_SUCCESS;
}

ov_status_t ov_core_execute_preboot(void) {
    if (!ov_core_instance.initialized) {
        return OV_ERROR_INIT;
    }
    
    ov_status_t status;
    
    status = ov_core_phase_detect_hardware();
    if (status != OV_SUCCESS) return status;
    
    status = ov_core_phase_analyze_capabilities();
    if (status != OV_SUCCESS) return status;
    
    status = ov_core_phase_resolve_workloads();
    if (status != OV_SUCCESS) return status;
    
    status = ov_core_phase_generate_report();
    if (status != OV_SUCCESS) return status;
    
    ov_log_info("\n========================================");
    ov_log_info("PreBoot Simulation Complete");
    ov_log_info("========================================\n");
    
    return OV_SUCCESS;
}

void ov_core_generate_report(const char *output_file) {
    if (!output_file) return;
    
    FILE *report = fopen(output_file, "w");
    if (!report) {
        ov_log_error("Failed to create report file");
        return;
    }
    
    fprintf(report, "=== OpenVintage PreBoot Report ===\n\n");
    fprintf(report, "CPU: %s\n", ov_core_instance.cpu_info.model_name);
    fprintf(report, "GPU: %s\n", ov_core_instance.gpu_info.model_name);
    fprintf(report, "RAM: %lu MB\n\n",
           ov_core_instance.mem_info.total_bytes / (1024 * 1024));
    
    fclose(report);
    ov_log_info("Report written to: %s", output_file);
}

void ov_core_cleanup(void) {
    ov_log_info("Shutting down OpenVintage PreBoot Simulator...");
    
    ov_resolver_cleanup();
    ov_hardware_cleanup();
    ov_memory_cleanup();
    ov_logger_cleanup();
    
    if (ov_core_instance.pci_devices) {
        free(ov_core_instance.pci_devices);
        ov_core_instance.pci_devices = NULL;
    }
    
    ov_core_instance.initialized = false;
}
