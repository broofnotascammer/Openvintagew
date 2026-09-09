/**
 * OpenVintage Pre-Boot Simulator - Hardware Detection & Simulation Profiles
 */

#include "ov_hardware.h"
#include "ov_logger.h"
#include "ov_memory.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pci/pci.h>

static struct pci_access *pci_access = NULL;
static ov_hw_profile_id_t active_profile_id = OV_HW_PROFILE_INTEL_GEN7;
static ov_hardware_profile_t active_profile = {0};
static ov_hardware_profile_t host_profile = {0};
static ov_hardware_profile_t custom_profile = {0};
static bool host_detected = false;

/* CPUID implementation for Linux x86_64 */
cpuid_regs_t ov_cpuid(uint32_t leaf, uint32_t subleaf) {
    cpuid_regs_t regs = {0};
#if defined(__x86_64__) || defined(_M_X64)
    asm volatile(
        "cpuid"
        : "=a"(regs.eax), "=b"(regs.ebx), "=c"(regs.ecx), "=d"(regs.edx)
        : "a"(leaf), "c"(subleaf)
    );
#else
    (void)leaf;
    (void)subleaf;
#endif
    return regs;
}

bool ov_cpu_supports_sse41(void) {
    cpuid_regs_t regs = ov_cpuid(1, 0);
    return (regs.ecx & (1 << 19)) != 0;
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

bool ov_cpu_supports_avx512(void) {
    cpuid_regs_t regs = ov_cpuid(7, 0);
    return (regs.ebx & (1 << 16)) != 0;
}

bool ov_cpu_supports_fma(void) {
    cpuid_regs_t regs = ov_cpuid(1, 0);
    return (regs.ecx & (1 << 12)) != 0;
}

bool ov_cpu_supports_aesni(void) {
    cpuid_regs_t regs = ov_cpuid(1, 0);
    return (regs.ecx & (1 << 25)) != 0;
}

bool ov_cpu_supports_tsc(void) {
    cpuid_regs_t regs = ov_cpuid(1, 0);
    return (regs.edx & (1 << 4)) != 0;
}

static ov_cpu_type_t ov_detect_cpu_type(const char *model_name) {
    if (strstr(model_name, "Intel")) {
        if (strstr(model_name, "Ivy Bridge") || strstr(model_name, "Core(TM) i7-3") || strstr(model_name, "Core(TM) i5-3")) {
            return OV_CPU_INTEL_IVY_BRIDGE;
        } else if (strstr(model_name, "Haswell") || strstr(model_name, "Core(TM) i7-4") || strstr(model_name, "Core(TM) i5-4")) {
            return OV_CPU_INTEL_HASWELL;
        }
        return OV_CPU_INTEL_MODERN;
    } else if (strstr(model_name, "AMD") || strstr(model_name, "Ryzen") || strstr(model_name, "EPYC")) {
        return OV_CPU_AMD_ZEN;
    } else if (strstr(model_name, "ARM") || strstr(model_name, "aarch64") || strstr(model_name, "Apple")) {
        return OV_CPU_ARM64;
    }
    return OV_CPU_UNKNOWN;
}

/* Host CPU Detection */
ov_status_t ov_hardware_detect_cpu(ov_cpu_info_t *cpu_info) {
    if (!cpu_info) return OV_ERROR_INVALID_PARAM;
    
    memset(cpu_info, 0, sizeof(ov_cpu_info_t));
    cpu_info->type = OV_CPU_UNKNOWN;
    strcpy(cpu_info->model_name, "Generic x86_64 Processor");
    cpu_info->cores = 1;
    cpu_info->threads = 1;
    cpu_info->base_freq_mhz = 2400;
    cpu_info->max_freq_mhz = 3200;

    FILE *cpuinfo = fopen("/proc/cpuinfo", "r");
    if (cpuinfo) {
        char line[512];
        int proc_count = 0;
        while (fgets(line, sizeof(line), cpuinfo)) {
            int p;
            if (sscanf(line, "processor\t: %d", &p) == 1) {
                proc_count++;
            } else if (sscanf(line, "model name\t: %127[^\n]", cpu_info->model_name) == 1) {
                cpu_info->type = ov_detect_cpu_type(cpu_info->model_name);
            } else if (sscanf(line, "cpu cores\t: %u", &cpu_info->cores) == 1) {
                /* parsed */
            } else if (sscanf(line, "siblings\t: %u", &cpu_info->threads) == 1) {
                /* parsed */
            }
        }
        fclose(cpuinfo);
        if (proc_count > 0 && cpu_info->threads < (uint32_t)proc_count) {
            cpu_info->threads = (uint32_t)proc_count;
        }
        if (cpu_info->cores == 0) cpu_info->cores = (cpu_info->threads > 0) ? cpu_info->threads : 1;
    }

    cpu_info->has_sse41 = ov_cpu_supports_sse41();
    cpu_info->has_sse42 = ov_cpu_supports_sse42();
    cpu_info->has_avx = ov_cpu_supports_avx();
    cpu_info->has_avx2 = ov_cpu_supports_avx2();
    cpu_info->has_avx512 = ov_cpu_supports_avx512();
    cpu_info->has_fma = ov_cpu_supports_fma();
    cpu_info->has_aesni = ov_cpu_supports_aesni();
    cpu_info->has_tsc = ov_cpu_supports_tsc();

    return OV_SUCCESS;
}

/* Host GPU Detection */
ov_status_t ov_hardware_detect_gpu(ov_gpu_info_t *gpu_info) {
    if (!gpu_info) return OV_ERROR_INVALID_PARAM;
    memset(gpu_info, 0, sizeof(ov_gpu_info_t));

    gpu_info->type = OV_GPU_INTEL_GEN7_HD4000;
    strcpy(gpu_info->model_name, "Intel HD Graphics 4000 (Gen7)");
    gpu_info->vendor_id = 0x8086;
    gpu_info->device_id = 0x0166;
    gpu_info->eu_count = 16;
    gpu_info->vram_mb = 1536;
    gpu_info->vram_bytes = 1536ULL * 1024 * 1024;
    gpu_info->max_texture_dimension = 8192;
    gpu_info->has_graphics = true;
    gpu_info->has_compute = true;
    gpu_info->supports_compute = true;
    gpu_info->supports_opengl_core = true;
    gpu_info->supports_vulkan = false;
    gpu_info->supports_metal = false;
    strcpy(gpu_info->driver_version, "OpenVintage Gen7 Mesa 22.0-ov");

    FILE *pci_file = popen("lspci -nn 2>/dev/null | grep -E 'VGA|3D|Display'", "r");
    if (pci_file) {
        char line[512];
        if (fgets(line, sizeof(line), pci_file)) {
            if (strstr(line, "8086")) {
                gpu_info->vendor_id = 0x8086;
                if (strstr(line, "Iris") || strstr(line, "Xe")) {
                    gpu_info->type = OV_GPU_INTEL_IRIS_XE;
                    strcpy(gpu_info->model_name, "Intel Iris Xe Graphics");
                    gpu_info->eu_count = 96;
                    gpu_info->vram_mb = 4096;
                    gpu_info->supports_vulkan = true;
                } else if (strstr(line, "HD Graphics 4600") || strstr(line, "0412") || strstr(line, "0416")) {
                    gpu_info->type = OV_GPU_INTEL_GEN75_HD4600;
                    strcpy(gpu_info->model_name, "Intel HD Graphics 4600 (Gen7.5)");
                    gpu_info->eu_count = 20;
                    gpu_info->vram_mb = 2048;
                } else {
                    gpu_info->type = OV_GPU_INTEL_GEN7_HD4000;
                    strcpy(gpu_info->model_name, "Intel HD Graphics (Gen7/Integrated)");
                }
            } else if (strstr(line, "10de") || strstr(line, "NVIDIA")) {
                gpu_info->type = OV_GPU_NVIDIA_GEFORCE;
                gpu_info->vendor_id = 0x10de;
                strcpy(gpu_info->model_name, "NVIDIA GeForce Graphics");
                gpu_info->eu_count = 128;
                gpu_info->vram_mb = 4096;
                gpu_info->supports_vulkan = true;
            } else if (strstr(line, "1002") || strstr(line, "AMD")) {
                gpu_info->type = OV_GPU_AMD_RADEON;
                gpu_info->vendor_id = 0x1002;
                strcpy(gpu_info->model_name, "AMD Radeon Graphics");
                gpu_info->eu_count = 64;
                gpu_info->vram_mb = 4096;
                gpu_info->supports_vulkan = true;
            }
        }
        pclose(pci_file);
    }
    gpu_info->vram_bytes = (uint64_t)gpu_info->vram_mb * 1024 * 1024;
    return OV_SUCCESS;
}

/* Host Memory Detection */
ov_status_t ov_hardware_detect_memory(ov_memory_info_t *mem_info) {
    if (!mem_info) return OV_ERROR_INVALID_PARAM;
    memset(mem_info, 0, sizeof(ov_memory_info_t));
    mem_info->numa_nodes = 1;
    mem_info->channels = 2;
    mem_info->frequency_mhz = 1600;
    mem_info->total_bytes = 16ULL * 1024 * 1024 * 1024;
    mem_info->available_bytes = 12ULL * 1024 * 1024 * 1024;

    FILE *meminfo = fopen("/proc/meminfo", "r");
    if (meminfo) {
        char line[256];
        while (fgets(line, sizeof(line), meminfo)) {
            uint64_t kb = 0;
            if (sscanf(line, "MemTotal: %lu kB", &kb) == 1) {
                mem_info->total_bytes = kb * 1024;
            } else if (sscanf(line, "MemAvailable: %lu kB", &kb) == 1) {
                mem_info->available_bytes = kb * 1024;
            }
        }
        fclose(meminfo);
    }
    return OV_SUCCESS;
}

/* Host PCI Scan */
ov_status_t ov_hardware_detect_pci(ov_pci_device_t **devices, uint32_t *device_count) {
    if (!devices || !device_count) return OV_ERROR_INVALID_PARAM;
    *devices = NULL;
    *device_count = 0;

    if (!pci_access) {
        /* Fallback synthetic PCI devices if libpci init failed */
        uint32_t count = 4;
        ov_pci_device_t *list = (ov_pci_device_t*)malloc(count * sizeof(ov_pci_device_t));
        if (!list) return OV_ERROR_MEMORY;

        list[0].vendor_id = 0x8086; list[0].device_id = 0x0150; list[0].bus = 0; list[0].slot = 0; list[0].func = 0;
        strcpy(list[0].class_name, "Host Bridge"); strcpy(list[0].vendor_name, "Intel Corp"); strcpy(list[0].device_name, "Ivy Bridge Host Bridge");

        list[1].vendor_id = 0x8086; list[1].device_id = 0x0166; list[1].bus = 0; list[1].slot = 2; list[1].func = 0;
        strcpy(list[1].class_name, "VGA Compatible Controller"); strcpy(list[1].vendor_name, "Intel Corp"); strcpy(list[1].device_name, "HD Graphics 4000 (Gen7)");

        list[2].vendor_id = 0x8086; list[2].device_id = 0x1e20; list[2].bus = 0; list[2].slot = 27; list[2].func = 0;
        strcpy(list[2].class_name, "Audio Controller"); strcpy(list[2].vendor_name, "Intel Corp"); strcpy(list[2].device_name, "7 Series/C216 HD Audio");

        list[3].vendor_id = 0x8086; list[3].device_id = 0x1e10; list[3].bus = 0; list[3].slot = 28; list[3].func = 0;
        strcpy(list[3].class_name, "PCI Express Root Port"); strcpy(list[3].vendor_name, "Intel Corp"); strcpy(list[3].device_name, "7 Series PCI Express Port 1");

        *devices = list;
        *device_count = count;
        return OV_SUCCESS;
    }

    struct pci_dev *dev;
    uint32_t count = 0;
    for (dev = pci_access->devices; dev; dev = dev->next) {
        count++;
    }

    if (count == 0) {
        pci_scan_bus(pci_access);
        for (dev = pci_access->devices; dev; dev = dev->next) count++;
    }

    if (count == 0) count = 1;
    ov_pci_device_t *list = (ov_pci_device_t*)malloc(count * sizeof(ov_pci_device_t));
    if (!list) return OV_ERROR_MEMORY;

    uint32_t i = 0;
    for (dev = pci_access->devices; dev && i < count; dev = dev->next) {
        pci_fill_info(dev, PCI_FILL_IDENT | PCI_FILL_CLASS);
        list[i].vendor_id = dev->vendor_id;
        list[i].device_id = dev->device_id;
        list[i].bus = dev->bus;
        list[i].slot = dev->dev;
        list[i].func = dev->func;
        snprintf(list[i].class_name, sizeof(list[i].class_name), "Class 0x%04x", dev->device_class);
        snprintf(list[i].vendor_name, sizeof(list[i].vendor_name), "Vendor 0x%04x", dev->vendor_id);
        snprintf(list[i].device_name, sizeof(list[i].device_name), "PCI %02x:%02x.%d [0x%04x:0x%04x]",
                 dev->bus, dev->dev, dev->func, dev->vendor_id, dev->device_id);
        i++;
    }
    *devices = list;
    *device_count = i;
    return OV_SUCCESS;
}

void ov_hardware_free_pci(ov_pci_device_t *devices) {
    if (devices) free(devices);
}

/* Hardware Profile Catalog Initialization */
static void ov_init_builtin_profiles(void) {
    /* Profile 1: Intel Gen7 (Ivy Bridge HD 4000) */
    ov_hardware_profile_t *p = &active_profile;
    p->profile_id = OV_HW_PROFILE_INTEL_GEN7;
    strcpy(p->profile_name, "Intel Gen7 HD 4000 (Ivy Bridge)");
    strcpy(p->description, "Target Reference Platform: Intel Core i7-3770, HD 4000 (16 EUs), 16GB DDR3-1600. AVX, SSE4.2, OpenGL 4.0.");
    p->is_simulated = true;

    p->cpu.type = OV_CPU_INTEL_IVY_BRIDGE;
    strcpy(p->cpu.model_name, "Intel(R) Core(TM) i7-3770 CPU @ 3.40GHz");
    p->cpu.cores = 4;
    p->cpu.threads = 8;
    p->cpu.base_freq_mhz = 3400;
    p->cpu.max_freq_mhz = 3900;
    p->cpu.has_sse41 = true;
    p->cpu.has_sse42 = true;
    p->cpu.has_avx = true;
    p->cpu.has_avx2 = false;
    p->cpu.has_avx512 = false;
    p->cpu.has_fma = false;
    p->cpu.has_aesni = true;
    p->cpu.has_tsc = true;

    p->gpu.type = OV_GPU_INTEL_GEN7_HD4000;
    strcpy(p->gpu.model_name, "Intel(R) HD Graphics 4000 (Gen7 GT2)");
    p->gpu.vendor_id = 0x8086;
    p->gpu.device_id = 0x0166;
    p->gpu.vram_mb = 1536;
    p->gpu.vram_bytes = 1536ULL * 1024 * 1024;
    p->gpu.eu_count = 16;
    p->gpu.max_texture_dimension = 8192;
    p->gpu.has_graphics = true;
    p->gpu.has_compute = true;
    p->gpu.supports_compute = true;
    p->gpu.supports_opengl_core = true;
    p->gpu.supports_vulkan = false;
    p->gpu.supports_metal = false;
    strcpy(p->gpu.driver_version, "OpenVintage Gen7 Mesa 22.0");

    p->mem.total_bytes = 16ULL * 1024 * 1024 * 1024;
    p->mem.available_bytes = 14ULL * 1024 * 1024 * 1024;
    p->mem.numa_nodes = 1;
    p->mem.channels = 2;
    p->mem.frequency_mhz = 1600;

    /* Custom profile default */
    custom_profile = *p;
    custom_profile.profile_id = OV_HW_PROFILE_CUSTOM;
    strcpy(custom_profile.profile_name, "Custom Simulated Platform");
    strcpy(custom_profile.description, "User-configured custom CPU/GPU parameters.");
}

ov_status_t ov_hardware_get_profile(ov_hw_profile_id_t profile_id, ov_hardware_profile_t *out_profile) {
    if (!out_profile) return OV_ERROR_INVALID_PARAM;
    memset(out_profile, 0, sizeof(ov_hardware_profile_t));

    switch (profile_id) {
        case OV_HW_PROFILE_HOST:
            if (!host_detected) {
                ov_hardware_detect_host(&host_profile);
            }
            *out_profile = host_profile;
            return OV_SUCCESS;

        case OV_HW_PROFILE_INTEL_GEN7:
            ov_init_builtin_profiles();
            *out_profile = active_profile;
            out_profile->profile_id = OV_HW_PROFILE_INTEL_GEN7;
            return OV_SUCCESS;

        case OV_HW_PROFILE_INTEL_GEN75:
            out_profile->profile_id = OV_HW_PROFILE_INTEL_GEN75;
            strcpy(out_profile->profile_name, "Intel Gen7.5 HD 4600 / Iris (Haswell)");
            strcpy(out_profile->description, "Intel Core i7-4770, HD 4600 (20 EUs), 16GB DDR3. AVX2, FMA, Full Gen7.5 execution pipeline.");
            out_profile->is_simulated = true;
            out_profile->cpu.type = OV_CPU_INTEL_HASWELL;
            strcpy(out_profile->cpu.model_name, "Intel(R) Core(TM) i7-4770 CPU @ 3.40GHz");
            out_profile->cpu.cores = 4;
            out_profile->cpu.threads = 8;
            out_profile->cpu.base_freq_mhz = 3400;
            out_profile->cpu.max_freq_mhz = 3900;
            out_profile->cpu.has_sse41 = true;
            out_profile->cpu.has_sse42 = true;
            out_profile->cpu.has_avx = true;
            out_profile->cpu.has_avx2 = true;
            out_profile->cpu.has_fma = true;
            out_profile->cpu.has_aesni = true;
            out_profile->cpu.has_tsc = true;

            out_profile->gpu.type = OV_GPU_INTEL_GEN75_HD4600;
            strcpy(out_profile->gpu.model_name, "Intel(R) HD Graphics 4600 (Gen7.5 GT2)");
            out_profile->gpu.vendor_id = 0x8086;
            out_profile->gpu.device_id = 0x0412;
            out_profile->gpu.vram_mb = 2048;
            out_profile->gpu.vram_bytes = 2048ULL * 1024 * 1024;
            out_profile->gpu.eu_count = 20;
            out_profile->gpu.max_texture_dimension = 8192;
            out_profile->gpu.has_graphics = true;
            out_profile->gpu.has_compute = true;
            out_profile->gpu.supports_compute = true;
            out_profile->gpu.supports_opengl_core = true;
            out_profile->gpu.supports_vulkan = true;
            out_profile->gpu.supports_metal = false;

            out_profile->mem.total_bytes = 16ULL * 1024 * 1024 * 1024;
            out_profile->mem.available_bytes = 14ULL * 1024 * 1024 * 1024;
            out_profile->mem.numa_nodes = 1;
            out_profile->mem.channels = 2;
            out_profile->mem.frequency_mhz = 1600;
            return OV_SUCCESS;

        case OV_HW_PROFILE_MODERN_INTEL:
            out_profile->profile_id = OV_HW_PROFILE_MODERN_INTEL;
            strcpy(out_profile->profile_name, "Modern Intel Core + Iris Xe (Gen12)");
            strcpy(out_profile->description, "Modern Intel Core i7-1185G7, Iris Xe (96 EUs), 32GB LPDDR4x. AVX-512, Vulkan 1.3 native.");
            out_profile->is_simulated = true;
            out_profile->cpu.type = OV_CPU_INTEL_MODERN;
            strcpy(out_profile->cpu.model_name, "11th Gen Intel(R) Core(TM) i7-1185G7 @ 3.00GHz");
            out_profile->cpu.cores = 4;
            out_profile->cpu.threads = 8;
            out_profile->cpu.base_freq_mhz = 3000;
            out_profile->cpu.max_freq_mhz = 4800;
            out_profile->cpu.has_sse41 = true;
            out_profile->cpu.has_sse42 = true;
            out_profile->cpu.has_avx = true;
            out_profile->cpu.has_avx2 = true;
            out_profile->cpu.has_avx512 = true;
            out_profile->cpu.has_fma = true;
            out_profile->cpu.has_aesni = true;
            out_profile->cpu.has_tsc = true;

            out_profile->gpu.type = OV_GPU_INTEL_IRIS_XE;
            strcpy(out_profile->gpu.model_name, "Intel(R) Iris(R) Xe Graphics (Gen12)");
            out_profile->gpu.vendor_id = 0x8086;
            out_profile->gpu.device_id = 0x9a49;
            out_profile->gpu.vram_mb = 8192;
            out_profile->gpu.vram_bytes = 8192ULL * 1024 * 1024;
            out_profile->gpu.eu_count = 96;
            out_profile->gpu.max_texture_dimension = 16384;
            out_profile->gpu.has_graphics = true;
            out_profile->gpu.has_compute = true;
            out_profile->gpu.supports_compute = true;
            out_profile->gpu.supports_opengl_core = true;
            out_profile->gpu.supports_vulkan = true;
            out_profile->gpu.supports_metal = false;

            out_profile->mem.total_bytes = 32ULL * 1024 * 1024 * 1024;
            out_profile->mem.available_bytes = 28ULL * 1024 * 1024 * 1024;
            out_profile->mem.numa_nodes = 1;
            out_profile->mem.channels = 2;
            out_profile->mem.frequency_mhz = 4266;
            return OV_SUCCESS;

        case OV_HW_PROFILE_AMD_ZEN:
            out_profile->profile_id = OV_HW_PROFILE_AMD_ZEN;
            strcpy(out_profile->profile_name, "AMD Ryzen 7 + Radeon Graphics");
            strcpy(out_profile->description, "AMD Ryzen 7 5800X (8C/16T) + Radeon RX 6700 XT, 32GB DDR4-3600.");
            out_profile->is_simulated = true;
            out_profile->cpu.type = OV_CPU_AMD_ZEN;
            strcpy(out_profile->cpu.model_name, "AMD Ryzen 7 5800X 8-Core Processor");
            out_profile->cpu.cores = 8;
            out_profile->cpu.threads = 16;
            out_profile->cpu.base_freq_mhz = 3800;
            out_profile->cpu.max_freq_mhz = 4700;
            out_profile->cpu.has_sse41 = true;
            out_profile->cpu.has_sse42 = true;
            out_profile->cpu.has_avx = true;
            out_profile->cpu.has_avx2 = true;
            out_profile->cpu.has_fma = true;
            out_profile->cpu.has_aesni = true;
            out_profile->cpu.has_tsc = true;

            out_profile->gpu.type = OV_GPU_AMD_RADEON;
            strcpy(out_profile->gpu.model_name, "AMD Radeon RX 6700 XT");
            out_profile->gpu.vendor_id = 0x1002;
            out_profile->gpu.device_id = 0x73df;
            out_profile->gpu.vram_mb = 12288;
            out_profile->gpu.vram_bytes = 12288ULL * 1024 * 1024;
            out_profile->gpu.eu_count = 40;
            out_profile->gpu.max_texture_dimension = 16384;
            out_profile->gpu.has_graphics = true;
            out_profile->gpu.has_compute = true;
            out_profile->gpu.supports_compute = true;
            out_profile->gpu.supports_opengl_core = true;
            out_profile->gpu.supports_vulkan = true;

            out_profile->mem.total_bytes = 32ULL * 1024 * 1024 * 1024;
            out_profile->mem.available_bytes = 28ULL * 1024 * 1024 * 1024;
            out_profile->mem.numa_nodes = 1;
            out_profile->mem.channels = 2;
            out_profile->mem.frequency_mhz = 3600;
            return OV_SUCCESS;

        case OV_HW_PROFILE_ARM64_GUEST:
            out_profile->profile_id = OV_HW_PROFILE_ARM64_GUEST;
            strcpy(out_profile->profile_name, "ARM64 Platform (Apple Silicon M-Series)");
            strcpy(out_profile->description, "Simulated Apple Silicon ARM64 CPU + Metal Native GPU, 16GB Unified Memory.");
            out_profile->is_simulated = true;
            out_profile->cpu.type = OV_CPU_ARM64;
            strcpy(out_profile->cpu.model_name, "Apple M1 / ARM64 Unified SoC");
            out_profile->cpu.cores = 8;
            out_profile->cpu.threads = 8;
            out_profile->cpu.base_freq_mhz = 3200;
            out_profile->cpu.max_freq_mhz = 3200;
            out_profile->cpu.has_sse41 = false;
            out_profile->cpu.has_sse42 = false;
            out_profile->cpu.has_avx = false;
            out_profile->cpu.has_avx2 = false;
            out_profile->cpu.has_aesni = true;
            out_profile->cpu.has_tsc = true;

            out_profile->gpu.type = OV_GPU_APPLE_SILICON;
            strcpy(out_profile->gpu.model_name, "Apple M1 GPU (8-core)");
            out_profile->gpu.vendor_id = 0x106b;
            out_profile->gpu.device_id = 0x0001;
            out_profile->gpu.vram_mb = 8192;
            out_profile->gpu.vram_bytes = 8192ULL * 1024 * 1024;
            out_profile->gpu.eu_count = 128;
            out_profile->gpu.max_texture_dimension = 16384;
            out_profile->gpu.has_graphics = true;
            out_profile->gpu.has_compute = true;
            out_profile->gpu.supports_compute = true;
            out_profile->gpu.supports_opengl_core = true;
            out_profile->gpu.supports_vulkan = false;
            out_profile->gpu.supports_metal = true;

            out_profile->mem.total_bytes = 16ULL * 1024 * 1024 * 1024;
            out_profile->mem.available_bytes = 14ULL * 1024 * 1024 * 1024;
            out_profile->mem.numa_nodes = 1;
            out_profile->mem.channels = 4;
            out_profile->mem.frequency_mhz = 4266;
            return OV_SUCCESS;

        case OV_HW_PROFILE_CUSTOM:
            *out_profile = custom_profile;
            return OV_SUCCESS;

        default:
            return OV_ERROR_NOT_FOUND;
    }
}

ov_status_t ov_hardware_detect_host(ov_hardware_profile_t *out_host) {
    if (!out_host) return OV_ERROR_INVALID_PARAM;
    memset(out_host, 0, sizeof(ov_hardware_profile_t));
    out_host->profile_id = OV_HW_PROFILE_HOST;
    strcpy(out_host->profile_name, "Host System (Real Detection)");
    strcpy(out_host->description, "Actual physical/virtual hardware detected on host machine.");
    out_host->is_simulated = false;

    ov_hardware_detect_cpu(&out_host->cpu);
    ov_hardware_detect_gpu(&out_host->gpu);
    ov_hardware_detect_memory(&out_host->mem);

    host_profile = *out_host;
    host_detected = true;
    return OV_SUCCESS;
}

ov_status_t ov_hardware_set_active_profile(ov_hw_profile_id_t profile_id) {
    ov_hardware_profile_t prof;
    ov_status_t status = ov_hardware_get_profile(profile_id, &prof);
    if (status != OV_SUCCESS) return status;

    active_profile_id = profile_id;
    active_profile = prof;
    ov_log_info("Active hardware profile switched to: %s", prof.profile_name);
    return OV_SUCCESS;
}

ov_hw_profile_id_t ov_hardware_get_active_profile_id(void) {
    return active_profile_id;
}

const ov_hardware_profile_t* ov_hardware_get_active_profile(void) {
    return &active_profile;
}

ov_status_t ov_hardware_update_custom_profile(const ov_hardware_profile_t *custom) {
    if (!custom) return OV_ERROR_INVALID_PARAM;
    custom_profile = *custom;
    custom_profile.profile_id = OV_HW_PROFILE_CUSTOM;
    if (active_profile_id == OV_HW_PROFILE_CUSTOM) {
        active_profile = custom_profile;
    }
    return OV_SUCCESS;
}

const ov_cpu_info_t* ov_hardware_get_cpu(void) {
    return &active_profile.cpu;
}

const ov_gpu_info_t* ov_hardware_get_gpu(void) {
    return &active_profile.gpu;
}

const ov_memory_info_t* ov_hardware_get_memory(void) {
    return &active_profile.mem;
}

ov_status_t ov_hardware_init(void) {
    ov_log_info("Initializing hardware detection subsystem...");
    
    bool has_pci_bus = (access("/sys/bus/pci", F_OK) == 0 || access("/proc/bus/pci", F_OK) == 0);
    if (has_pci_bus) {
        pci_access = pci_alloc();
        if (pci_access) {
            pci_init(pci_access);
            ov_log_info("PCI access initialized via libpci");
        }
    } else {
        ov_log_info("Host raw PCI bus not present in environment; using simulated OpenVintage PCI bus architecture");
        pci_access = NULL;
    }

    /* Detect host hardware once */
    ov_hardware_detect_host(&host_profile);

    /* Initialize profiles and default to Intel Gen7 target */
    ov_init_builtin_profiles();
    ov_hardware_set_active_profile(OV_HW_PROFILE_INTEL_GEN7);

    return OV_SUCCESS;
}

void ov_hardware_cleanup(void) {
    if (pci_access) {
        pci_cleanup(pci_access);
        pci_access = NULL;
    }
    ov_log_info("Hardware detection subsystem cleanup complete");
}
