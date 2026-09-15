/**
 * OpenVintage Pre-Boot Architecture Simulator - Native Linux Hardware Detection
 * Uses sysfs, procfs, CPUID with strict leaf validation, and PCI subsystem.
 */

#include "../common/ov_platform.h"
#include "ov_logger.h"
#include "ov_memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

/* CPUID validation */
static uint32_t linux_cpuid_max_leaf(void) {
#if defined(__x86_64__) || defined(_M_X64)
    cpuid_regs_t r = ov_cpuid(0, 0);
    return r.eax;
#else
    return 0;
#endif
}

static uint32_t linux_cpuid_max_ext_leaf(void) {
#if defined(__x86_64__) || defined(_M_X64)
    cpuid_regs_t r = ov_cpuid(0x80000000, 0);
    return r.eax;
#else
    return 0;
#endif
}

ov_status_t ov_linux_detect_cpu(ov_cpu_info_t *cpu) {
    if (!cpu) return OV_ERROR_INVALID_PARAM;
    memset(cpu, 0, sizeof(ov_cpu_info_t));

    cpu->type = OV_CPU_UNKNOWN;
    cpu->cores = 1;
    cpu->threads = 1;
    cpu->base_freq_mhz = 2000;
    cpu->max_freq_mhz = 2500;
    cpu->is_64bit = true;
    cpu->l1_cache_kb = 32;
    cpu->l2_cache_kb = 256;
    cpu->l3_cache_kb = 4096;
    snprintf(cpu->model_name, sizeof(cpu->model_name), "Linux Host CPU");

#if defined(__linux__)
    FILE *f = fopen("/proc/cpuinfo", "r");
    if (f) {
        char line[256];
        uint32_t processor_count = 0;
        /* Track unique physical socket & core ID combinations */
        int cur_phys_id = 0;
        int cur_core_id = 0;
        int seen_pairs[256][2];
        int unique_cores = 0;

        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "model name", 10) == 0) {
                char *colon = strchr(line, ':');
                if (colon && strlen(colon) > 2) {
                    colon += 2;
                    colon[strcspn(colon, "\r\n")] = 0;
                    snprintf(cpu->model_name, sizeof(cpu->model_name), "%s", colon);
                }
            } else if (strncmp(line, "processor", 9) == 0) {
                processor_count++;
            } else if (strncmp(line, "physical id", 11) == 0) {
                char *colon = strchr(line, ':');
                if (colon) cur_phys_id = atoi(colon + 1);
            } else if (strncmp(line, "core id", 7) == 0) {
                char *colon = strchr(line, ':');
                if (colon) {
                    cur_core_id = atoi(colon + 1);
                    bool found = false;
                    for (int k = 0; k < unique_cores; k++) {
                        if (seen_pairs[k][0] == cur_phys_id && seen_pairs[k][1] == cur_core_id) {
                            found = true;
                            break;
                        }
                    }
                    if (!found && unique_cores < 256) {
                        seen_pairs[unique_cores][0] = cur_phys_id;
                        seen_pairs[unique_cores][1] = cur_core_id;
                        unique_cores++;
                    }
                }
            } else if (strncmp(line, "cpu MHz", 7) == 0) {
                char *colon = strchr(line, ':');
                if (colon) {
                    float mhz = atof(colon + 1);
                    if (mhz > 100.0f) {
                        cpu->base_freq_mhz = (uint32_t)mhz;
                        cpu->max_freq_mhz = (uint32_t)(mhz * 1.15f);
                    }
                }
            }
        }
        fclose(f);

        if (processor_count > 0) cpu->threads = processor_count;
        if (unique_cores > 0) cpu->cores = (uint32_t)unique_cores;
        else cpu->cores = cpu->threads;
    }
#endif

    /* Safe CPUID Leaf-Validated Probing */
#if defined(__x86_64__) || defined(_M_X64)
    uint32_t max_leaf = linux_cpuid_max_leaf();
    uint32_t max_ext_leaf = linux_cpuid_max_ext_leaf();

    if (max_leaf >= 1) {
        cpuid_regs_t r1 = ov_cpuid(1, 0);
        cpu->has_sse   = (r1.edx & (1 << 25)) != 0;
        cpu->has_sse2  = (r1.edx & (1 << 26)) != 0;
        cpu->has_tsc   = (r1.edx & (1 << 4))  != 0;
        cpu->has_sse3  = (r1.ecx & (1 << 0))  != 0;
        cpu->has_ssse3 = (r1.ecx & (1 << 9))  != 0;
        cpu->has_sse41 = (r1.ecx & (1 << 19)) != 0;
        cpu->has_sse42 = (r1.ecx & (1 << 20)) != 0;
        cpu->has_fma   = (r1.ecx & (1 << 12)) != 0;
        cpu->has_aesni = (r1.ecx & (1 << 25)) != 0;
        cpu->has_avx   = (r1.ecx & (1 << 28)) != 0;
    }

    if (max_leaf >= 7) {
        cpuid_regs_t r7 = ov_cpuid(7, 0);
        cpu->has_avx2   = (r7.ebx & (1 << 5))  != 0;
        cpu->has_avx512 = (r7.ebx & (1 << 16)) != 0;
    }

    /* Cache descriptors from leaf 0x80000006 if advertised */
    if (max_ext_leaf >= 0x80000006) {
        cpuid_regs_t r_cache = ov_cpuid(0x80000006, 0);
        uint32_t l2_size = (r_cache.ecx >> 16) & 0xFFFF;
        uint32_t l3_size = ((r_cache.edx >> 18) & 0x3FFF) * 512;
        if (l2_size > 0) cpu->l2_cache_kb = l2_size;
        if (l3_size > 0) cpu->l3_cache_kb = l3_size;
    }

    if (strstr(cpu->model_name, "AMD") != NULL || strstr(cpu->model_name, "Ryzen") != NULL) {
        cpu->type = OV_CPU_AMD_ZEN;
    } else if (strstr(cpu->model_name, "Intel") != NULL) {
        if (cpu->has_avx2) cpu->type = OV_CPU_INTEL_HASWELL;
        else if (cpu->has_avx) cpu->type = OV_CPU_INTEL_IVY_BRIDGE;
        else cpu->type = OV_CPU_INTEL_CORE2_DUO;
    }
#elif defined(__aarch64__) || defined(__arm64__)
    cpu->has_neon = true;
    cpu->type = OV_CPU_ARM64_GENERIC;
#endif

    return OV_SUCCESS;
}

ov_status_t ov_linux_detect_memory(ov_memory_info_t *mem) {
    if (!mem) return OV_ERROR_INVALID_PARAM;
    memset(mem, 0, sizeof(ov_memory_info_t));

    mem->total_bytes = 4ULL * 1024 * 1024 * 1024;
    mem->available_bytes = 2ULL * 1024 * 1024 * 1024;
    mem->channels = 2;
    mem->frequency_mhz = 2400;
    snprintf(mem->memory_type, sizeof(mem->memory_type), "System RAM");

#if defined(__linux__)
    FILE *f = fopen("/proc/meminfo", "r");
    if (f) {
        char line[256];
        uint64_t total_kb = 0, avail_kb = 0;
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "MemTotal:", 9) == 0) {
                total_kb = strtoull(line + 9, NULL, 10);
            } else if (strncmp(line, "MemAvailable:", 13) == 0) {
                avail_kb = strtoull(line + 13, NULL, 10);
            }
        }
        fclose(f);
        if (total_kb > 0) mem->total_bytes = total_kb * 1024ULL;
        if (avail_kb > 0) mem->available_bytes = avail_kb * 1024ULL;
        else mem->available_bytes = mem->total_bytes / 2;
    }
#endif

    mem->reserved_bytes = (mem->total_bytes > mem->available_bytes) ?
                          (mem->total_bytes - mem->available_bytes) : 0;
    return OV_SUCCESS;
}

ov_status_t ov_linux_detect_gpus(ov_gpu_topology_t *out_topo) {
    if (!out_topo) return OV_ERROR_INVALID_PARAM;
    memset(out_topo, 0, sizeof(ov_gpu_topology_t));

    uint32_t count = 0;

#if defined(__linux__)
    /* Probe Linux DRM subsystem under /sys/class/drm/ */
    DIR *dir = opendir("/sys/class/drm");
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL && count < OV_MAX_GPUS) {
            if (strncmp(entry->d_name, "card", 4) == 0 && strchr(entry->d_name, '-') == NULL) {
                char vendor_path[512], device_path[512];
                snprintf(vendor_path, sizeof(vendor_path), "/sys/class/drm/%s/device/vendor", entry->d_name);
                snprintf(device_path, sizeof(device_path), "/sys/class/drm/%s/device/device", entry->d_name);

                FILE *fv = fopen(vendor_path, "r");
                FILE *fd = fopen(device_path, "r");
                if (fv && fd) {
                    char v_str[32] = {0}, d_str[32] = {0};
                    if (fgets(v_str, sizeof(v_str), fv) && fgets(d_str, sizeof(d_str), fd)) {
                        uint16_t vid = (uint16_t)strtoul(v_str, NULL, 0);
                        uint16_t did = (uint16_t)strtoul(d_str, NULL, 0);

                        ov_gpu_info_t *g = &out_topo->gpus[count];
                        g->vendor_id = vid;
                        g->device_id = did;
                        g->has_graphics = true;
                        g->supports_opengl_core = true;
                        g->opengl_major = 4;
                        g->opengl_minor = 1;
                        g->max_texture_dimension = 8192;
                        g->vram_mb = 1024;
                        g->vram_bytes = 1024ULL * 1024 * 1024;

                        if (vid == 0x8086) {
                            snprintf(g->model_name, sizeof(g->model_name), "Intel Graphics Controller (0x%04x)", did);
                            g->type = OV_GPU_INTEL_GEN7_HD4000;
                            g->arch_gen = OV_GPU_ARCH_INTEL_GEN7;
                            out_topo->has_integrated_gpu = true;
                        } else if (vid == 0x10de) {
                            snprintf(g->model_name, sizeof(g->model_name), "NVIDIA Graphics Controller (0x%04x)", did);
                            g->type = OV_GPU_NVIDIA_GEFORCE;
                            g->arch_gen = OV_GPU_ARCH_NVIDIA_KEPLER;
                            g->supports_vulkan = true;
                            out_topo->has_discrete_gpu = true;
                            out_topo->discrete_gpu_index = count;
                        } else if (vid == 0x1002) {
                            snprintf(g->model_name, sizeof(g->model_name), "AMD Radeon Controller (0x%04x)", did);
                            g->type = OV_GPU_AMD_RADEON;
                            g->arch_gen = OV_GPU_ARCH_AMD_GCN_1_4;
                            g->supports_vulkan = true;
                            out_topo->has_discrete_gpu = true;
                            out_topo->discrete_gpu_index = count;
                        } else {
                            snprintf(g->model_name, sizeof(g->model_name), "DRM Display Controller (0x%04x:0x%04x)", vid, did);
                            g->type = OV_GPU_SOFTWARE_RASTERIZER;
                            g->arch_gen = OV_GPU_ARCH_SOFTWARE_FALLBACK;
                        }
                        count++;
                    }
                }
                if (fv) fclose(fv);
                if (fd) fclose(fd);
            }
        }
        closedir(dir);
    }
#endif

    if (count == 0) {
        /* Virtual fallback adapter */
        ov_gpu_info_t *g = &out_topo->gpus[0];
        g->type = OV_GPU_SOFTWARE_RASTERIZER;
        g->arch_gen = OV_GPU_ARCH_SOFTWARE_FALLBACK;
        snprintf(g->model_name, sizeof(g->model_name), "Linux Host Graphics / Mesa Fallback");
        g->vendor_id = 0x0000;
        g->device_id = 0x0000;
        g->vram_mb = 512;
        g->vram_bytes = 512ULL * 1024 * 1024;
        g->supports_opengl_core = true;
        g->opengl_major = 3;
        g->opengl_minor = 3;
        g->max_texture_dimension = 8192;
        g->has_graphics = true;
        count = 1;
    }

    out_topo->gpu_count = count;
    out_topo->primary_gpu_index = 0;
    if (out_topo->has_integrated_gpu && out_topo->has_discrete_gpu) {
        out_topo->is_muxed_switchable = true;
        snprintf(out_topo->switch_policy, sizeof(out_topo->switch_policy), "Linux PRIME Switchable Dual-GPU");
    }

    return OV_SUCCESS;
}

ov_status_t ov_linux_detect_host(ov_hardware_profile_t *out_host) {
    if (!out_host) return OV_ERROR_INVALID_PARAM;
    memset(out_host, 0, sizeof(ov_hardware_profile_t));

    out_host->source = OV_HW_SOURCE_HOST_DETECTED;
    out_host->is_simulated = false;
    out_host->is_mac_host = false;

    snprintf(out_host->model_identifier, sizeof(out_host->model_identifier), "Linux x86_64 Host");
    snprintf(out_host->profile_name, sizeof(out_host->profile_name), "HostSystem");
    snprintf(out_host->marketing_name, sizeof(out_host->marketing_name), "Linux System Host");

    ov_linux_detect_cpu(&out_host->cpu);
    ov_linux_detect_memory(&out_host->mem);

    ov_gpu_topology_t topo;
    ov_linux_detect_gpus(&topo);
    out_host->gpu_topology = topo;
    out_host->gpu = topo.gpus[topo.primary_gpu_index];
    out_host->has_discrete_gpu = topo.has_discrete_gpu;
    out_host->is_switchable_graphics = topo.is_muxed_switchable;
    if (topo.has_discrete_gpu) {
        out_host->secondary_gpu = topo.gpus[topo.discrete_gpu_index];
    }

    snprintf(out_host->firmware_type, sizeof(out_host->firmware_type), "Standard PC UEFI / BIOS");
    out_host->efi_is_64bit = true;
    out_host->apfs_supported_in_firmware = false;
    snprintf(out_host->description, sizeof(out_host->description), "%s (%u cores, %llu MB RAM)",
             out_host->cpu.model_name, out_host->cpu.cores,
             (unsigned long long)(out_host->mem.total_bytes / (1024ULL * 1024ULL)));

    return OV_SUCCESS;
}
