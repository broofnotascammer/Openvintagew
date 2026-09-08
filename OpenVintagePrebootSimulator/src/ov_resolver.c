#include "ov_resolver.h"
#include "ov_logger.h"
#include <stdio.h>

static ov_cpu_info_t *resolver_cpu = NULL;
static ov_gpu_info_t *resolver_gpu = NULL;
static ov_memory_info_t *resolver_mem = NULL;

ov_status_t ov_resolver_init(ov_cpu_info_t *cpu, ov_gpu_info_t *gpu, ov_memory_info_t *mem) {
    ov_log_info("Initializing OvResolver decision engine...");
    resolver_cpu = cpu;
    resolver_gpu = gpu;
    resolver_mem = mem;
    return OV_SUCCESS;
}

static ov_exec_mode_t ov_resolve_cpu_mode(ov_cpu_info_t *cpu) {
    if (!cpu) return OV_EXEC_FALLBACK;
    
    /* Intel modern processors prefer native execution */
    if (cpu->type == OV_CPU_INTEL_MODERN) {
        if (cpu->has_avx2) return OV_EXEC_NATIVE;
    }
    
    /* Legacy processors may need translation */
    if (cpu->type == OV_CPU_INTEL_LEGACY) {
        if (cpu->has_sse42 && cpu->has_avx) return OV_EXEC_NATIVE;
        return OV_EXEC_TRANSLATED;
    }
    
    return OV_EXEC_NATIVE;
}

static ov_exec_mode_t ov_resolve_gpu_mode(ov_gpu_info_t *gpu, ov_workload_t *workload) {
    if (!gpu || !workload) return OV_EXEC_FALLBACK;
    
    if (!workload->requires_gpu) return OV_EXEC_NATIVE;
    
    if (gpu->type == OV_GPU_NONE) return OV_EXEC_FALLBACK;
    if (gpu->has_compute) return OV_EXEC_NATIVE;
    
    return OV_EXEC_TRANSLATED;
}

ov_resolution_t ov_resolver_route(ov_workload_t *workload) {
    ov_resolution_t resolution = {0};
    
    if (!workload) {
        resolution.mode = OV_EXEC_FALLBACK;
        strcpy(resolution.reason, "Invalid workload");
        return resolution;
    }
    
    ov_log_info("Resolving workload: type=%u, priority=%u", workload->type, workload->priority);
    
    ov_exec_mode_t cpu_mode = ov_resolve_cpu_mode(resolver_cpu);
    ov_exec_mode_t gpu_mode = ov_resolve_gpu_mode(resolver_gpu, workload);
    
    /* Resolve execution mode based on workload type */
    switch (workload->type) {
        case OV_WORKLOAD_COMPUTE:
            resolution.mode = cpu_mode;
            resolution.estimated_cost = (cpu_mode == OV_EXEC_NATIVE) ? 10 : 50;
            snprintf(resolution.reason, sizeof(resolution.reason), 
                    "CPU compute workload, mode=%s", 
                    cpu_mode == OV_EXEC_NATIVE ? "NATIVE" : "TRANSLATED");
            break;
            
        case OV_WORKLOAD_GRAPHICS:
            resolution.mode = gpu_mode;
            resolution.estimated_cost = (gpu_mode == OV_EXEC_NATIVE) ? 15 : 100;
            snprintf(resolution.reason, sizeof(resolution.reason),
                    "GPU graphics workload, mode=%s",
                    gpu_mode == OV_EXEC_NATIVE ? "NATIVE" : "TRANSLATED");
            break;
            
        case OV_WORKLOAD_MEMORY:
            resolution.mode = OV_EXEC_NATIVE;
            resolution.estimated_cost = 5;
            strcpy(resolution.reason, "Memory-bound workload, using native execution");
            break;
            
        case OV_WORKLOAD_IO:
            resolution.mode = OV_EXEC_NATIVE;
            resolution.estimated_cost = 20;
            strcpy(resolution.reason, "I/O workload, using native execution");
            break;
            
        case OV_WORKLOAD_MIXED:
            resolution.mode = (cpu_mode == OV_EXEC_NATIVE && gpu_mode == OV_EXEC_NATIVE) 
                            ? OV_EXEC_NATIVE : OV_EXEC_TRANSLATED;
            resolution.estimated_cost = 40;
            strcpy(resolution.reason, "Mixed workload, hybrid execution");
            break;
            
        default:
            resolution.mode = OV_EXEC_FALLBACK;
            strcpy(resolution.reason, "Unknown workload type");
    }
    
    ov_log_info("Resolution decision: %s (cost=%u)", resolution.reason, resolution.estimated_cost);
    return resolution;
}

void ov_resolver_print_decision(ov_resolution_t *decision) {
    if (!decision) return;
    
    const char *mode_str[] = {"NATIVE", "TRANSLATED", "FALLBACK"};
    ov_log_info("\n=== Resolution Decision ===");
    ov_log_info("Mode: %s", mode_str[decision->mode]);
    ov_log_info("Estimated Cost: %u", decision->estimated_cost);
    ov_log_info("Reason: %s", decision->reason);
}

void ov_resolver_analyze_cpu(ov_cpu_info_t *cpu) {
    if (!cpu) return;
    ov_log_info("\n=== CPU Analysis ===");
    ov_log_info("Model: %s", cpu->model_name);
    ov_log_info("Cores: %u, Threads: %u", cpu->cores, cpu->threads);
    ov_log_info("Frequency: %u-%u MHz", cpu->base_freq_mhz, cpu->max_freq_mhz);
    ov_log_info("Features: SSE4.2=%d, AVX=%d, AVX2=%d, TSC=%d",
               cpu->has_sse42, cpu->has_avx, cpu->has_avx2, cpu->has_tsc);
}

void ov_resolver_analyze_gpu(ov_gpu_info_t *gpu) {
    if (!gpu) return;
    ov_log_info("\n=== GPU Analysis ===");
    ov_log_info("Model: %s", gpu->model_name);
    ov_log_info("VRAM: %u MB", gpu->vram_mb);
    ov_log_info("Capabilities: Compute=%d, Graphics=%d, EU=%u",
               gpu->has_compute, gpu->has_graphics, gpu->eu_count);
}

void ov_resolver_cleanup(void) {
    ov_log_info("OvResolver cleanup");
    resolver_cpu = NULL;
    resolver_gpu = NULL;
    resolver_mem = NULL;
}
