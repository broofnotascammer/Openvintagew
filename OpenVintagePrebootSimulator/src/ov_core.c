/**
 * OpenVintage Pre-Boot Simulator - Core Orchestration Engine Implementation (Phases 1-5)
 */

#include "ov_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ov_core_t ov_core_instance = {0};

ov_core_t* ov_core_get_instance(void) {
    return &ov_core_instance;
}

const char* ov_boot_phase_to_string(ov_boot_phase_t phase) {
    switch (phase) {
        case OV_PHASE_UNINITIALIZED: return "Uninitialized";
        case OV_PHASE_MEMORY_INIT: return "Phase 1: Memory & Logging Init";
        case OV_PHASE_HARDWARE_DISCOVERY: return "Phase 1: Hardware & PCI Discovery";
        case OV_PHASE_CAPABILITY_ANALYSIS: return "Phase 2: Capability & Feature Analysis";
        case OV_PHASE_OVIR_SUBSYSTEMS_INIT: return "Phase 3/4: OVIR CPU/GPU Subsystems Init";
        case OV_PHASE_UNIFIED_RESOLVER_EVAL: return "Phase 3: Workload Resolution Evaluation";
        case OV_PHASE_RESOURCE_ALLOCATION: return "Phase 5: Resource & Scheduler Provisioning";
        case OV_PHASE_CACHE_WARMUP: return "Phase 5: Unified Multi-Tier Cache Warmup";
        case OV_PHASE_COMPATIBILITY_AUDIT: return "Phase 5: Compatibility & Silicon Quirks Audit";
        case OV_PHASE_BENCHMARK_VALIDATION: return "Phase 5: Empirical Benchmark Validation";
        case OV_PHASE_READY: return "Simulation Complete (Ready)";
        case OV_PHASE_ERROR: return "Simulation Error";
        default: return "Unknown Phase";
    }
}

ov_status_t ov_core_init(void) {
    ov_logger_init("openvintage_preboot.log");
    ov_log_info("================================================================");
    ov_log_info(" OpenVintage Pre-Boot Architecture Simulator (Phases 1-5)");
    ov_log_info("================================================================");

    memset(&ov_core_instance, 0, sizeof(ov_core_instance));
    ov_core_instance.current_phase = OV_PHASE_MEMORY_INIT;
    ov_core_instance.phase_progress_percent = 5;
    strcpy(ov_core_instance.phase_message, "Initializing Memory Subsystem (32MB Pool)...");

    /* 1. Memory Subsystem */
    ov_status_t status = ov_memory_init(32 * 1024 * 1024);
    if (status != OV_SUCCESS) {
        ov_log_error("Failed to initialize memory pool: %s", ov_status_to_string(status));
        ov_core_instance.current_phase = OV_PHASE_ERROR;
        return status;
    }

    /* 2. Hardware Subsystem */
    status = ov_hardware_init();
    if (status != OV_SUCCESS) {
        ov_log_error("Failed to initialize hardware discovery: %s", ov_status_to_string(status));
        ov_core_instance.current_phase = OV_PHASE_ERROR;
        return status;
    }

    ov_core_instance.initialized = true;
    return OV_SUCCESS;
}

ov_status_t ov_core_phase_detect_hardware(void) {
    ov_core_instance.current_phase = OV_PHASE_HARDWARE_DISCOVERY;
    ov_core_instance.phase_progress_percent = 20;
    strcpy(ov_core_instance.phase_message, "Scanning CPUID, PCI Bus, and Memory Layout...");
    ov_log_info("--- Phase 1: Hardware Discovery ---");

    ov_cpu_info_t cpu;
    ov_gpu_info_t gpu;
    ov_memory_info_t mem;

    ov_status_t st = ov_hardware_detect_cpu(&cpu);
    if (st != OV_SUCCESS) {
        ov_log_warn("CPU detection warning: %s", ov_status_to_string(st));
    }

    st = ov_hardware_detect_gpu(&gpu);
    if (st != OV_SUCCESS) {
        ov_log_warn("GPU detection warning: %s", ov_status_to_string(st));
    }

    st = ov_hardware_detect_memory(&mem);
    if (st != OV_SUCCESS) {
        ov_log_warn("Memory detection warning: %s", ov_status_to_string(st));
    }

    /* Query PCI Devices */
    st = ov_hardware_detect_pci(&ov_core_instance.pci_devices, &ov_core_instance.pci_device_count);
    if (st != OV_SUCCESS) {
        ov_log_warn("PCI detection returned warning/error: %s", ov_status_to_string(st));
    }

    const ov_hardware_profile_t *prof = ov_hardware_get_active_profile();
    if (prof) {
        ov_core_instance.active_hw_profile = *prof;
    }

    return OV_SUCCESS;
}

ov_status_t ov_core_phase_analyze_capabilities(void) {
    ov_core_instance.current_phase = OV_PHASE_CAPABILITY_ANALYSIS;
    ov_core_instance.phase_progress_percent = 35;
    strcpy(ov_core_instance.phase_message, "Analyzing Silicon Capabilities & Instruction Sets...");
    ov_log_info("--- Phase 2: Capability Analysis ---");

    const ov_cpu_info_t *cpu = ov_hardware_get_cpu();
    const ov_gpu_info_t *gpu = ov_hardware_get_gpu();
    const ov_memory_info_t *mem = ov_hardware_get_memory();

    ov_log_info("CPU: %s (%u Cores / %u Threads)", cpu->model_name, cpu->cores, cpu->threads);
    ov_log_info("     ISA: SSE4.2=%s, AVX=%s, AVX2=%s, AES-NI=%s",
                cpu->has_sse42 ? "Yes" : "No", cpu->has_avx ? "Yes" : "No",
                cpu->has_avx2 ? "Yes" : "No", cpu->has_aesni ? "Yes" : "No");

    ov_log_info("GPU: %s (PCI 0x%04x:0x%04x, %lu MB VRAM)",
                gpu->model_name, gpu->vendor_id, gpu->device_id, gpu->vram_bytes / (1024 * 1024));
    ov_log_info("     APIs: OpenGL Core=%s, Vulkan=%s, Metal=%s",
                gpu->supports_opengl_core ? "Native" : "No",
                gpu->supports_vulkan ? "Native" : "No",
                gpu->supports_metal ? "Native" : "No");

    ov_log_info("RAM: %lu MB Total, %lu MB Free",
                mem->total_bytes / (1024 * 1024), mem->available_bytes / (1024 * 1024));

    return OV_SUCCESS;
}

ov_status_t ov_core_phase_resolve_workloads(void) {
    ov_core_instance.current_phase = OV_PHASE_UNIFIED_RESOLVER_EVAL;
    ov_core_instance.phase_progress_percent = 50;
    strcpy(ov_core_instance.phase_message, "Initializing OVIR & Evaluating Workload Resolutions...");
    ov_log_info("--- Phase 3 & 4: OVIR Engines & Workload Resolution ---");

    ov_cpu_info_t *cpu = (ov_cpu_info_t*)ov_hardware_get_cpu();
    ov_gpu_info_t *gpu = (ov_gpu_info_t*)ov_hardware_get_gpu();
    ov_memory_info_t *mem = (ov_memory_info_t*)ov_hardware_get_memory();

    /* Initialize Subsystems */
    ov_cpu_engine_init();
    ov_gpu_engine_init();
    ov_resolver_init(cpu, gpu, mem);

    /* Test a typical modern macOS/iOS Metal App workload on this platform */
    ov_integrated_request_t test_req = {
        .guest_cpu_arch = 1, /* ARM64 */
        .requested_api = OV_API_METAL,
        .api_version_major = 2,
        .api_version_minor = 0,
        .requires_compute = true,
        .requires_tessellation = true,
        .requires_avx2 = false,
        .max_texture_dimension = 8192,
        .required_vram_bytes = 512ULL * 1024 * 1024,
        .required_ram_bytes = 2ULL * 1024 * 1024 * 1024,
        .required_cpu_cores = 4,
        .guest_code_hash = 0xAA77BB22CC114499ULL,
        .shader_bytecode_hash = 0x5566778899AABBCCULL
    };
    strcpy(test_req.application_name, "Metal Odyssey (Adventure RPG)");

    ov_status_t st = ov_resolver_evaluate_integrated(&test_req, &ov_core_instance.last_resolution);
    if (st == OV_SUCCESS) {
        ov_log_info("Resolved Metal Workload: %s (Overhead: ~%u%%)",
                    ov_resolver_decision_to_string(ov_core_instance.last_resolution.decision),
                    ov_core_instance.last_resolution.performance_cost_factor - 100);
    }

    return OV_SUCCESS;
}

ov_status_t ov_core_phase_init_cache_and_resources(void) {
    ov_core_instance.current_phase = OV_PHASE_RESOURCE_ALLOCATION;
    ov_core_instance.phase_progress_percent = 65;
    strcpy(ov_core_instance.phase_message, "Configuring OVScheduler & Warming Unified Cache...");
    ov_log_info("--- Phase 5: Resource Management & Multi-Tier Cache ---");

    ov_resource_manager_init();
    ov_resource_manager_set_profile(OV_PROFILE_BALANCED);
    ov_resource_manager_get_status(&ov_core_instance.resource_status);

    ov_unified_cache_init();
    ov_unified_cache_get_stats(&ov_core_instance.cache_stats);

    ov_compatibility_init();

    return OV_SUCCESS;
}

ov_status_t ov_core_phase_run_benchmarks(void) {
    ov_core_instance.current_phase = OV_PHASE_BENCHMARK_VALIDATION;
    ov_core_instance.phase_progress_percent = 85;
    strcpy(ov_core_instance.phase_message, "Executing Empirical Architecture Benchmark Suite...");
    ov_log_info("--- Phase 5: Empirical Benchmark Validation ---");

    ov_benchmark_init();
    ov_status_t st = ov_benchmark_run_suite(&ov_core_instance.benchmark_results);
    if (st != OV_SUCCESS) {
        ov_log_warn("Benchmark run encountered warning");
    }

    return OV_SUCCESS;
}

ov_status_t ov_core_phase_generate_report(void) {
    ov_core_instance.current_phase = OV_PHASE_READY;
    ov_core_instance.phase_progress_percent = 100;
    strcpy(ov_core_instance.phase_message, "Simulation Complete. Generating System Diagnostics...");
    ov_log_info("--- Phase 5: Diagnostic Report Generation ---");

    ov_diagnostics_init();
    ov_status_t st = ov_diagnostics_generate_report(&ov_core_instance.diagnostic_report);
    if (st != OV_SUCCESS) {
        ov_log_error("Diagnostic report generation failed");
        return st;
    }

    ov_core_instance.boot_successful = true;
    ov_log_info("OpenVintage Pre-Boot Simulation Completed Successfully (Health: %u/100)",
                ov_core_instance.diagnostic_report.system_health_score);

    return OV_SUCCESS;
}

ov_status_t ov_core_execute_preboot(void) {
    if (!ov_core_instance.initialized) {
        ov_status_t st = ov_core_init();
        if (st != OV_SUCCESS) return st;
    }

    ov_status_t st;
    st = ov_core_phase_detect_hardware();
    if (st != OV_SUCCESS) return st;

    st = ov_core_phase_analyze_capabilities();
    if (st != OV_SUCCESS) return st;

    st = ov_core_phase_resolve_workloads();
    if (st != OV_SUCCESS) return st;

    st = ov_core_phase_init_cache_and_resources();
    if (st != OV_SUCCESS) return st;

    st = ov_core_phase_run_benchmarks();
    if (st != OV_SUCCESS) return st;

    st = ov_core_phase_generate_report();
    if (st != OV_SUCCESS) return st;

    return OV_SUCCESS;
}

void ov_core_generate_report(const char *output_file) {
    if (!output_file) return;
    ov_diagnostics_export_text(&ov_core_instance.diagnostic_report, output_file);
}

void ov_core_cleanup(void) {
    ov_log_info("Cleaning up OpenVintage Pre-Boot Simulator...");

    ov_diagnostics_cleanup();
    ov_benchmark_cleanup();
    ov_compatibility_cleanup();
    ov_unified_cache_cleanup();
    ov_resource_manager_cleanup();
    ov_gpu_engine_cleanup();
    ov_cpu_engine_cleanup();
    ov_resolver_cleanup();
    ov_hardware_cleanup();
    ov_memory_cleanup();

    if (ov_core_instance.pci_devices) {
        free(ov_core_instance.pci_devices);
        ov_core_instance.pci_devices = NULL;
    }

    ov_core_instance.initialized = false;
    ov_logger_cleanup();
}
