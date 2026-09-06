#include "ov_hardware.h"
#include "ov_logger.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pci/pci.h>

static struct pci_access *pci_access = NULL;

/* CPUID implementation for Linux x86_64 */
cpuid_regs_t ov_cpuid(uint32_t leaf, uint32_t subleaf) {
    cpuid_regs_t regs = {0};
#ifdef __x86_64__
    asm volatile(
        "cpuid"
        : "=a"(regs.eax), "=b"(regs.ebx), "=c"(regs.ecx), "=d"(regs.edx)
        : "a"(leaf), "c"(subleaf)
    );
#endif
    return regs;
}

bool ov_cpu_supports_sse42(void) {
    cpuid_regs_t regs = ov_cpuid(1, 0);
    return (regs.ecx & (1 << 20)) != 0;
}

bool ov_cpu_supports_avx(void) {
    cpuid_regs_t regs = ov_cpuid(1, 0);
    return (regs.ecx & (1 << 28)) != 0;
}

bool ov_cpu_supports_avx2(void) {
    cpuid_regs_t regs = ov_cpuid(7, 0);
    return (regs.ebx & (1 << 5)) != 0;
}

bool ov_cpu_supports_tsc(void) {
    cpuid_regs_t regs = ov_cpuid(1, 0);
    return (regs.edx & (1 << 4)) != 0;
}

static ov_cpu_type_t ov_detect_cpu_type(const char *model_name) {
    if (strstr(model_name, "Intel")) {
        if (strstr(model_name, "Ivy Bridge") || strstr(model_name, "Sandy Bridge") ||
            strstr(model_name, "Haswell") || strstr(model_name, "Broadwell")) {
            return OV_CPU_INTEL_LEGACY;
        }
        return OV_CPU_INTEL_MODERN;
    } else if (strstr(model_name, "AMD")) {
        return OV_CPU_AMD;
    } else if (strstr(model_name, "ARM")) {
        return OV_CPU_ARM;
    }
    return OV_CPU_UNKNOWN;
}

ov_status_t ov_hardware_init(void) {
    ov_log_info("Initializing hardware detection subsystem...");
    
    pci_access = pci_alloc();
    if (!pci_access) {
        ov_log_error("Failed to initialize PCI access");
        return OV_ERROR_HARDWARE;
    }
    
    pci_init(pci_access);
    ov_log_info("Hardware detection initialized");
    return OV_SUCCESS;
}

ov_status_t ov_hardware_detect_cpu(ov_cpu_info_t *cpu_info) {
    if (!cpu_info) return OV_ERROR_INVALID_PARAM;
    
    ov_log_info("Detecting CPU hardware...");
    memset(cpu_info, 0, sizeof(ov_cpu_info_t));
    
    FILE *cpuinfo = fopen("/proc/cpuinfo", "r");
    if (!cpuinfo) {
        ov_log_error("Failed to open /proc/cpuinfo");
        return OV_ERROR_HARDWARE;
    }
    
    char line[512];
    int cpu_count = 0;
    uint32_t base_freq = 0;
    
    while (fgets(line, sizeof(line), cpuinfo)) {
        if (sscanf(line, "processor\t: %d", &cpu_count) == 1) {
            cpu_info->cores++;
        } else if (sscanf(line, "model name\t: %255[^\n]", cpu_info->model_name) == 1) {
            cpu_info->type = ov_detect_cpu_type(cpu_info->model_name);
        } else if (sscanf(line, "cpu cores\t: %u", &cpu_info->cores) == 1) {
            // cores already set
        } else if (sscanf(line, "siblings\t: %u", &cpu_info->threads) == 1) {
            // threads set
        } else if (strstr(line, "flags")) {
            cpu_info->has_sse42 = ov_cpu_supports_sse42();
            cpu_info->has_avx = ov_cpu_supports_avx();
            cpu_info->has_avx2 = ov_cpu_supports_avx2();
            cpu_info->has_tsc = ov_cpu_supports_tsc();
        }
    }
    
    fclose(cpuinfo);
    
    if (cpu_info->cores == 0) cpu_info->cores = 1;
    if (cpu_info->threads == 0) cpu_info->threads = cpu_info->cores;
    
    ov_log_info("Detected CPU: %s (%u cores, %u threads)", 
                cpu_info->model_name, cpu_info->cores, cpu_info->threads);
    
    return OV_SUCCESS;
}

ov_status_t ov_hardware_detect_gpu(ov_gpu_info_t *gpu_info) {
    if (!gpu_info) return OV_ERROR_INVALID_PARAM;
    
    ov_log_info("Detecting GPU hardware...");
    memset(gpu_info, 0, sizeof(ov_gpu_info_t));
    
    FILE *gpu_info_file = popen("lspci -k | grep -A2 VGA", "r");
    if (!gpu_info_file) {
        ov_log_warn("Could not detect GPU via lspci");
        gpu_info->type = OV_GPU_NONE;
        strcpy(gpu_info->model_name, "None");
        return OV_SUCCESS;
    }
    
    char line[512];
    if (fgets(line, sizeof(line), gpu_info_file)) {
        if (strstr(line, "Intel")) {
            gpu_info->type = OV_GPU_INTEL_HD;
            if (strstr(line, "Iris")) gpu_info->type = OV_GPU_INTEL_IRIS;
        } else if (strstr(line, "NVIDIA")) {
            gpu_info->type = OV_GPU_NVIDIA;
        } else if (strstr(line, "AMD")) {
            gpu_info->type = OV_GPU_AMD;
        }
        strncpy(gpu_info->model_name, line, sizeof(gpu_info->model_name) - 1);
    }
    
    pclose(gpu_info_file);
    ov_log_info("Detected GPU: %s", gpu_info->model_name);
    
    return OV_SUCCESS;
}

ov_status_t ov_hardware_detect_memory(ov_memory_info_t *mem_info) {
    if (!mem_info) return OV_ERROR_INVALID_PARAM;
    
    ov_log_info("Detecting memory hardware...");
    memset(mem_info, 0, sizeof(ov_memory_info_t));
    
    FILE *meminfo = fopen("/proc/meminfo", "r");
    if (!meminfo) {
        ov_log_error("Failed to open /proc/meminfo");
        return OV_ERROR_HARDWARE;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), meminfo)) {
        uint64_t kb_value;
        if (sscanf(line, "MemTotal: %lu kB", &kb_value) == 1) {
            mem_info->total_bytes = kb_value * 1024;
        } else if (sscanf(line, "MemAvailable: %lu kB", &kb_value) == 1) {
            mem_info->available_bytes = kb_value * 1024;
        }
    }
    
    fclose(meminfo);
    
    mem_info->numa_nodes = 1;
    mem_info->channels = 2;
    
    ov_log_info("Detected Memory: %lu MB total, %lu MB available",
                mem_info->total_bytes / (1024 * 1024),
                mem_info->available_bytes / (1024 * 1024));
    
    return OV_SUCCESS;
}

ov_status_t ov_hardware_detect_pci(ov_pci_device_t **devices, uint32_t *device_count) {
    if (!devices || !device_count) return OV_ERROR_INVALID_PARAM;
    
    ov_log_info("Detecting PCI devices...");
    
    if (!pci_access) return OV_ERROR_HARDWARE;
    
    struct pci_dev *dev;
    uint32_t count = 0;
    
    for (dev = pci_access->devices; dev; dev = dev->next) {
        count++;
    }
    
    *devices = malloc(count * sizeof(ov_pci_device_t));
    if (!*devices) return OV_ERROR_MEMORY;
    
    uint32_t i = 0;
    for (dev = pci_access->devices; dev && i < count; dev = dev->next) {
        (*devices)[i].vendor_id = dev->vendor_id;
        (*devices)[i].device_id = dev->device_id;
        (*devices)[i].bus = dev->bus;
        (*devices)[i].slot = dev->dev;
        (*devices)[i].func = dev->func;
        i++;
    }
    
    *device_count = count;
    ov_log_info("Detected %u PCI devices", count);
    
    return OV_SUCCESS;
}

void ov_hardware_cleanup(void) {
    if (pci_access) {
        pci_cleanup(pci_access);
        pci_access = NULL;
    }
    ov_log_info("Hardware detection cleanup complete");
}
