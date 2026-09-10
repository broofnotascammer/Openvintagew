/**
 * OpenVintage Pre-Boot Simulator - Hardware Detection & Simulation Profiles
 * Real host discovery (read-only) and complete Mac target profiles database.
 */

#include "ov_hardware.h"
#include "ov_logger.h"
#include "ov_memory.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>

#if defined(__linux__)
#include <pci/pci.h>
static struct pci_access *pci_access = NULL;
#endif

static ov_hw_profile_id_t active_profile_id = OV_HW_PROFILE_MBP91_IVY_BRIDGE;
static ov_hardware_profile_t active_profile = {0};
static ov_hardware_profile_t host_profile = {0};
static ov_hardware_profile_t custom_profile = {0};
static bool host_detected = false;

/* CPUID implementation for x86_64 */
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

bool ov_cpu_supports_sse(void) {
    cpuid_regs_t regs = ov_cpuid(1, 0);
    return (regs.edx & (1 << 25)) != 0;
}

bool ov_cpu_supports_sse2(void) {
    cpuid_regs_t regs = ov_cpuid(1, 0);
    return (regs.edx & (1 << 26)) != 0;
}

bool ov_cpu_supports_sse3(void) {
    cpuid_regs_t regs = ov_cpuid(1, 0);
    return (regs.ecx & (1 << 0)) != 0;
}

bool ov_cpu_supports_ssse3(void) {
    cpuid_regs_t regs = ov_cpuid(1, 0);
    return (regs.ecx & (1 << 9)) != 0;
}

bool ov_cpu_supports_sse41(void) {
    cpuid_regs_t regs = ov_cpuid(1, 0);
    return (regs.ecx & (1 << 19)) != 0;
}

bool ov_cpu_supports_sse42(void) {
    cpuid_regs_t regs = ov_cpuid(1, 0);
    return (regs.ecx & (1 << 20)) != 0;
}

uint32_t ov_cpuid_max_leaf(void) {
#if defined(__x86_64__) || defined(_M_X64)
    cpuid_regs_t regs = ov_cpuid(0, 0);
    return regs.eax;
#else
    return 0;
#endif
}

uint32_t ov_cpuid_max_ext_leaf(void) {
#if defined(__x86_64__) || defined(_M_X64)
    cpuid_regs_t regs = ov_cpuid(0x80000000, 0);
    return regs.eax;
#else
    return 0;
#endif
}

bool ov_cpu_supports_avx(void) {
    if (ov_cpuid_max_leaf() < 1) return false;
    cpuid_regs_t regs = ov_cpuid(1, 0);
    return (regs.ecx & (1 << 28)) != 0;
}

bool ov_cpu_supports_avx2(void) {
    if (ov_cpuid_max_leaf() < 7) return false;
    cpuid_regs_t regs = ov_cpuid(7, 0);
    return (regs.ebx & (1 << 5)) != 0;
}

bool ov_cpu_supports_avx512(void) {
    if (ov_cpuid_max_leaf() < 7) return false;
    cpuid_regs_t regs = ov_cpuid(7, 0);
    return (regs.ebx & (1 << 16)) != 0;
}

bool ov_cpu_supports_fma(void) {
    if (ov_cpuid_max_leaf() < 1) return false;
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

/* ========================================================================= */
/* Host Hardware Discovery (Safe, Read-Only)                                  */
/* ========================================================================= */

ov_status_t ov_hardware_detect_cpu(ov_cpu_info_t *cpu_info) {
    if (!cpu_info) return OV_ERROR_INVALID_PARAM;
    memset(cpu_info, 0, sizeof(ov_cpu_info_t));

    cpu_info->type = OV_CPU_UNKNOWN;
    cpu_info->cores = 1;
    cpu_info->threads = 1;
    cpu_info->base_freq_mhz = 2000;
    cpu_info->max_freq_mhz = 2500;
    cpu_info->is_64bit = true;
    cpu_info->l1_cache_kb = 32;
    cpu_info->l2_cache_kb = 256;
    cpu_info->l3_cache_kb = 4096;
    snprintf(cpu_info->model_name, sizeof(cpu_info->model_name), "Generic Host CPU");

#if defined(__linux__)
    FILE *f = fopen("/proc/cpuinfo", "r");
    if (f) {
        char line[256];
        uint32_t processor_count = 0;
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "model name", 10) == 0) {
                char *colon = strchr(line, ':');
                if (colon && strlen(colon) > 2) {
                    colon += 2;
                    colon[strcspn(colon, "\r\n")] = 0;
                    snprintf(cpu_info->model_name, sizeof(cpu_info->model_name), "%s", colon);
                }
            } else if (strncmp(line, "processor", 9) == 0) {
                processor_count++;
            } else if (strncmp(line, "cpu MHz", 7) == 0) {
                char *colon = strchr(line, ':');
                if (colon) {
                    float mhz = atof(colon + 1);
                    if (mhz > 100.0f) {
                        cpu_info->base_freq_mhz = (uint32_t)mhz;
                        cpu_info->max_freq_mhz = (uint32_t)(mhz * 1.1f);
                    }
                }
            } else if (strncmp(line, "flags", 5) == 0) {
                if (strstr(line, " sse "))   cpu_info->has_sse = true;
                if (strstr(line, " sse2 "))  cpu_info->has_sse2 = true;
                if (strstr(line, " pni ") || strstr(line, " sse3 ")) cpu_info->has_sse3 = true;
                if (strstr(line, " ssse3 ")) cpu_info->has_ssse3 = true;
                if (strstr(line, " sse4_1 ")) cpu_info->has_sse41 = true;
                if (strstr(line, " sse4_2 ")) cpu_info->has_sse42 = true;
                if (strstr(line, " avx "))   cpu_info->has_avx = true;
                if (strstr(line, " avx2 "))  cpu_info->has_avx2 = true;
                if (strstr(line, " avx512f ")) cpu_info->has_avx512 = true;
                if (strstr(line, " fma "))   cpu_info->has_fma = true;
                if (strstr(line, " aes "))   cpu_info->has_aesni = true;
                if (strstr(line, " tsc "))   cpu_info->has_tsc = true;
            }
        }
        fclose(f);
        if (processor_count > 0) {
            cpu_info->threads = processor_count;
            cpu_info->cores = (processor_count > 1) ? (processor_count / 2) : 1;
            if (cpu_info->cores == 0) cpu_info->cores = 1;
        }
    }
#endif

    /* Check CPUID registers if running on x86_64 */
#if defined(__x86_64__) || defined(_M_X64)
    cpu_info->has_sse   = ov_cpu_supports_sse();
    cpu_info->has_sse2  = ov_cpu_supports_sse2();
    cpu_info->has_sse3  = ov_cpu_supports_sse3();
    cpu_info->has_ssse3 = ov_cpu_supports_ssse3();
    cpu_info->has_sse41 = ov_cpu_supports_sse41();
    cpu_info->has_sse42 = ov_cpu_supports_sse42();
    cpu_info->has_avx   = ov_cpu_supports_avx();
    cpu_info->has_avx2  = ov_cpu_supports_avx2();
    cpu_info->has_avx512= ov_cpu_supports_avx512();
    cpu_info->has_fma   = ov_cpu_supports_fma();
    cpu_info->has_aesni = ov_cpu_supports_aesni();
    cpu_info->has_tsc   = ov_cpu_supports_tsc();

    if (strstr(cpu_info->model_name, "AMD") != NULL || strstr(cpu_info->model_name, "Ryzen") != NULL) {
        cpu_info->type = OV_CPU_AMD_ZEN;
    } else if (strstr(cpu_info->model_name, "Intel") != NULL) {
        if (cpu_info->has_avx2) {
            cpu_info->type = OV_CPU_INTEL_HASWELL;
        } else if (cpu_info->has_avx) {
            cpu_info->type = OV_CPU_INTEL_IVY_BRIDGE;
        } else {
            cpu_info->type = OV_CPU_INTEL_CORE2_DUO;
        }
    }
#endif

    return OV_SUCCESS;
}

ov_status_t ov_hardware_detect_memory(ov_memory_info_t *mem_info) {
    if (!mem_info) return OV_ERROR_INVALID_PARAM;
    memset(mem_info, 0, sizeof(ov_memory_info_t));

    mem_info->total_bytes = 4ULL * 1024 * 1024 * 1024;
    mem_info->available_bytes = 2ULL * 1024 * 1024 * 1024;
    mem_info->channels = 2;
    mem_info->frequency_mhz = 2400;
    snprintf(mem_info->memory_type, sizeof(mem_info->memory_type), "DDR4 / System RAM");

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
        if (total_kb > 0) mem_info->total_bytes = total_kb * 1024ULL;
        if (avail_kb > 0) mem_info->available_bytes = avail_kb * 1024ULL;
        else mem_info->available_bytes = mem_info->total_bytes / 2;
    }
#endif

    mem_info->reserved_bytes = mem_info->total_bytes - mem_info->available_bytes;
    return OV_SUCCESS;
}

ov_status_t ov_hardware_detect_gpu(ov_gpu_info_t *gpu_info) {
    if (!gpu_info) return OV_ERROR_INVALID_PARAM;
    memset(gpu_info, 0, sizeof(ov_gpu_info_t));

    /* Safe default host display adapter */
    gpu_info->type = OV_GPU_SOFTWARE_RASTERIZER;
    gpu_info->arch_gen = OV_GPU_ARCH_SOFTWARE_FALLBACK;
    snprintf(gpu_info->model_name, sizeof(gpu_info->model_name), "Host Software / Virtual Adapter");
    gpu_info->vendor_id = 0x0000;
    gpu_info->device_id = 0x0000;
    gpu_info->vram_mb = 1024;
    gpu_info->vram_bytes = 1024ULL * 1024 * 1024;
    snprintf(gpu_info->vram_type, sizeof(gpu_info->vram_type), "System Shared");
    gpu_info->max_texture_dimension = 8192;
    gpu_info->has_graphics = true;
    gpu_info->has_compute = false;
    gpu_info->supports_opengl_core = true;
    gpu_info->opengl_major = 3;
    gpu_info->opengl_minor = 3;
    gpu_info->metal_level = OV_METAL_NONE;
    gpu_info->supports_metal = false;
    gpu_info->supports_vulkan = false;
    gpu_info->supports_directx = false;
    snprintf(gpu_info->driver_version, sizeof(gpu_info->driver_version), "Mesa / Host Graphics Emulation");

    return OV_SUCCESS;
}

ov_status_t ov_hardware_detect_pci(ov_pci_device_t **devices, uint32_t *device_count) {
    if (!devices || !device_count) return OV_ERROR_INVALID_PARAM;

#if defined(__linux__)
    if (pci_access) {
        pci_scan_bus(pci_access);
        uint32_t count = 0;
        for (struct pci_dev *dev = pci_access->devices; dev; dev = dev->next) {
            count++;
        }
        if (count > 0) {
            ov_pci_device_t *list = ov_calloc(count, sizeof(ov_pci_device_t));
            if (list) {
                uint32_t idx = 0;
                for (struct pci_dev *dev = pci_access->devices; dev && idx < count; dev = dev->next, idx++) {
                    list[idx].vendor_id = dev->vendor_id;
                    list[idx].device_id = dev->device_id;
                    list[idx].bus = dev->bus;
                    list[idx].slot = dev->dev;
                    list[idx].func = dev->func;
                    snprintf(list[idx].class_name, sizeof(list[idx].class_name), "PCI Device (Class 0x%04x)", dev->device_class);
                    pci_fill_info(dev, PCI_FILL_IDENT | PCI_FILL_CLASS);
                    char buf[256];
                    pci_lookup_name(pci_access, buf, sizeof(buf), PCI_LOOKUP_VENDOR, dev->vendor_id);
                    snprintf(list[idx].vendor_name, sizeof(list[idx].vendor_name), "%s", buf);
                    pci_lookup_name(pci_access, buf, sizeof(buf), PCI_LOOKUP_DEVICE, dev->vendor_id, dev->device_id);
                    snprintf(list[idx].device_name, sizeof(list[idx].device_name), "%s", buf);
                }
                *devices = list;
                *device_count = count;
                return OV_SUCCESS;
            }
        }
    }
#endif

    /* If raw PCI bus is not accessible (e.g. sandboxed container/VM), provide structured host devices */
    uint32_t count = 3;
    ov_pci_device_t *list = ov_calloc(count, sizeof(ov_pci_device_t));
    if (!list) {
        *devices = NULL;
        *device_count = 0;
        return OV_ERROR_MEMORY;
    }

    /* Host Bridge */
    list[0].vendor_id = 0x8086;
    list[0].device_id = 0x1237;
    snprintf(list[0].class_name, sizeof(list[0].class_name), "Bridge: Host Bridge");
    snprintf(list[0].vendor_name, sizeof(list[0].vendor_name), "Intel Corporation / Host Virtual Bridge");
    snprintf(list[0].device_name, sizeof(list[0].device_name), "440FX - 82441FX Host Bridge");

    /* Host Display Controller */
    list[1].vendor_id = 0x1234;
    list[1].device_id = 0x1111;
    list[1].slot = 1;
    snprintf(list[1].class_name, sizeof(list[1].class_name), "Display controller: VGA Compatible");
    snprintf(list[1].vendor_name, sizeof(list[1].vendor_name), "QEMU / Virtual Host Graphics");
    snprintf(list[1].device_name, sizeof(list[1].device_name), "Standard VGA Graphics Adapter");

    /* Host Storage Controller */
    list[2].vendor_id = 0x8086;
    list[2].device_id = 0x7010;
    list[2].slot = 2;
    snprintf(list[2].class_name, sizeof(list[2].class_name), "Mass Storage Controller: IDE/SATA");
    snprintf(list[2].vendor_name, sizeof(list[2].vendor_name), "Intel Corporation / Virtual Host Storage");
    snprintf(list[2].device_name, sizeof(list[2].device_name), "PIIX3 IDE / Virtual Host Storage Controller");

    *devices = list;
    *device_count = count;
    return OV_SUCCESS;
}

void ov_hardware_free_pci(ov_pci_device_t *devices) {
    if (devices) {
        ov_free(devices);
    }
}

ov_status_t ov_hardware_detect_host(ov_hardware_profile_t *out_host) {
    if (!out_host) return OV_ERROR_INVALID_PARAM;
    memset(out_host, 0, sizeof(ov_hardware_profile_t));

    out_host->profile_id = OV_HW_PROFILE_HOST;
    out_host->source = OV_HW_SOURCE_HOST_DETECTED;
    out_host->is_simulated = false;

    snprintf(out_host->profile_name, sizeof(out_host->profile_name), "Host System");
    snprintf(out_host->model_identifier, sizeof(out_host->model_identifier), "HostSystem-x86_64");
    snprintf(out_host->marketing_name, sizeof(out_host->marketing_name), "Local Host Machine");

    ov_hardware_detect_cpu(&out_host->cpu);
    ov_hardware_detect_gpu(&out_host->gpu);
    ov_hardware_detect_memory(&out_host->mem);

    /* Detect if host is a genuine Apple Mac via DMI */
    out_host->is_mac_host = false;
#if defined(__linux__)
    FILE *fdmi = fopen("/sys/class/dmi/id/product_name", "r");
    if (fdmi) {
        char dmi_buf[128];
        if (fgets(dmi_buf, sizeof(dmi_buf), fdmi)) {
            dmi_buf[strcspn(dmi_buf, "\r\n")] = 0;
            if (strstr(dmi_buf, "Mac") != NULL || strstr(dmi_buf, "Apple") != NULL) {
                out_host->is_mac_host = true;
                snprintf(out_host->model_identifier, sizeof(out_host->model_identifier), "%s", dmi_buf);
            }
        }
        fclose(fdmi);
    }
#endif

    if (out_host->is_mac_host) {
        snprintf(out_host->description, sizeof(out_host->description),
                 "Physical Apple Mac Host: %s (%u Cores @ %u MHz)",
                 out_host->model_identifier, out_host->cpu.cores, out_host->cpu.base_freq_mhz);
    } else {
        snprintf(out_host->description, sizeof(out_host->description),
                 "Linux x86_64 Virtualized/Container Environment (Non-Mac Host)");
    }

    snprintf(out_host->firmware_type, sizeof(out_host->firmware_type), "Host Firmware");
    snprintf(out_host->storage_interface, sizeof(out_host->storage_interface), "Host Storage Subsystem");
    snprintf(out_host->native_macos_min, sizeof(out_host->native_macos_min), "N/A (Host Machine)");
    snprintf(out_host->native_macos_max, sizeof(out_host->native_macos_max), "N/A (Host Machine)");
    snprintf(out_host->silicon_quirks, sizeof(out_host->silicon_quirks), "None (Host Native)");

    host_profile = *out_host;
    host_detected = true;
    return OV_SUCCESS;
}

/* ========================================================================= */
/* Comprehensive Mac Target Profiles Database                                 */
/* ========================================================================= */

static const ov_hardware_profile_t g_mac_profiles[] = {
    /* 0: HOST (Filled dynamically) */
    {
        .profile_id = OV_HW_PROFILE_HOST,
        .source = OV_HW_SOURCE_HOST_DETECTED,
        .model_identifier = "Host",
        .profile_name = "Host",
        .marketing_name = "Host System",
        .description = "Real Host System",
        .is_simulated = false
    },

    /* 1: MacBookPro1,1 (Early 2006, 32-bit Core Duo Yonah, ATI X1600) */
    {
        .profile_id = OV_HW_PROFILE_MBP11_CORE_DUO,
        .source = OV_HW_SOURCE_SIMULATED_PROFILE,
        .model_identifier = "MacBookPro1,1",
        .profile_name = "MacBookPro1,1",
        .marketing_name = "MacBook Pro (15-inch, Early 2006)",
        .description = "First Intel Mac laptop. 32-bit Core Duo Yonah processor, ATI Radeon X1600 GPU.",
        .cpu = {
            .type = OV_CPU_INTEL_CORE_DUO,
            .model_name = "Intel Core Duo T2600",
            .cores = 2, .threads = 2,
            .base_freq_mhz = 2160, .max_freq_mhz = 2160,
            .is_64bit = false, /* 32-bit only! */
            .l1_cache_kb = 32, .l2_cache_kb = 2048, .l3_cache_kb = 0,
            .has_sse = true, .has_sse2 = true, .has_sse3 = true,
            .has_ssse3 = false, .has_sse41 = false, .has_sse42 = false,
            .has_avx = false, .has_avx2 = false, .has_aesni = false
        },
        .gpu = {
            .type = OV_GPU_AMD_RADEON,
            .arch_gen = OV_GPU_ARCH_AMD_TERASCALE_1,
            .model_name = "ATI Mobility Radeon X1600",
            .vendor_id = 0x1002, .device_id = 0x71C5,
            .vram_mb = 128, .vram_bytes = 128ULL * 1024 * 1024,
            .vram_type = "GDDR3",
            .eu_count = 12, .max_texture_dimension = 2048,
            .has_graphics = true, .has_compute = false,
            .supports_opengl_core = true, .opengl_major = 2, .opengl_minor = 1,
            .metal_level = OV_METAL_NONE
        },
        .mem = { .total_bytes = 2ULL * 1024 * 1024 * 1024, .memory_type = "DDR2-667", .channels = 2 },
        .display_info = "15.4-inch Matte/Glossy (1440x900)",
        .storage_interface = "SATA 1.5 Gb/s",
        .firmware_type = "Apple EFI 32-bit (v1.10)",
        .efi_is_64bit = false,
        .apfs_supported_in_firmware = false,
        .native_macos_min = "Mac OS X 10.4.4 Tiger",
        .native_macos_max = "Mac OS X 10.6.8 Snow Leopard",
        .silicon_quirks = "32-bit Yonah CPU limits OS support to Snow Leopard; lacks SSSE3 and 64-bit instructions.",
        .is_simulated = true
    },

    /* 2: MacBookPro3,1 (Mid 2007, Merom Core 2 Duo, GeForce 8600M GT) */
    {
        .profile_id = OV_HW_PROFILE_MBP31_CORE2_DUO,
        .source = OV_HW_SOURCE_SIMULATED_PROFILE,
        .model_identifier = "MacBookPro3,1",
        .profile_name = "MacBookPro3,1",
        .marketing_name = "MacBook Pro (15-inch, Mid 2007)",
        .description = "Santa Rosa platform. 64-bit Core 2 Duo Merom, NVIDIA GeForce 8600M GT.",
        .cpu = {
            .type = OV_CPU_INTEL_CORE2_DUO,
            .model_name = "Intel Core 2 Duo T7700",
            .cores = 2, .threads = 2,
            .base_freq_mhz = 2400, .max_freq_mhz = 2400,
            .is_64bit = true,
            .l1_cache_kb = 32, .l2_cache_kb = 4096, .l3_cache_kb = 0,
            .has_sse = true, .has_sse2 = true, .has_sse3 = true, .has_ssse3 = true,
            .has_sse41 = false, .has_sse42 = false, .has_avx = false, .has_avx2 = false
        },
        .gpu = {
            .type = OV_GPU_NVIDIA_GEFORCE,
            .arch_gen = OV_GPU_ARCH_NVIDIA_TESLA,
            .model_name = "NVIDIA GeForce 8600M GT",
            .vendor_id = 0x10DE, .device_id = 0x0407,
            .vram_mb = 256, .vram_bytes = 256ULL * 1024 * 1024,
            .vram_type = "GDDR3",
            .eu_count = 32, .max_texture_dimension = 8192,
            .supports_opengl_core = true, .opengl_major = 3, .opengl_minor = 3,
            .metal_level = OV_METAL_NONE
        },
        .mem = { .total_bytes = 4ULL * 1024 * 1024 * 1024, .memory_type = "DDR2-667", .channels = 2 },
        .display_info = "15.4-inch LED (1440x900)",
        .storage_interface = "SATA 1.5 Gb/s",
        .firmware_type = "Apple EFI 64-bit",
        .efi_is_64bit = true,
        .native_macos_min = "Mac OS X 10.4.9 Tiger",
        .native_macos_max = "OS X 10.11.6 El Capitan",
        .silicon_quirks = "GeForce 8600M GT thermal defect quirk; lacks SSE4.1 requiring emulation on Sierra+.",
        .is_simulated = true
    },

    /* 3: MacBook5,1 (Late 2008, Penryn, GeForce 9400M) */
    {
        .profile_id = OV_HW_PROFILE_MB51_PENRYN,
        .source = OV_HW_SOURCE_SIMULATED_PROFILE,
        .model_identifier = "MacBook5,1",
        .profile_name = "MacBook5,1",
        .marketing_name = "MacBook (13-inch, Aluminum, Late 2008)",
        .description = "Unibody aluminum MacBook with integrated NVIDIA MCP79 chipset graphics.",
        .cpu = {
            .type = OV_CPU_INTEL_CORE2_DUO,
            .model_name = "Intel Core 2 Duo P8600",
            .cores = 2, .threads = 2,
            .base_freq_mhz = 2400, .max_freq_mhz = 2400,
            .is_64bit = true,
            .l1_cache_kb = 32, .l2_cache_kb = 3072, .l3_cache_kb = 0,
            .has_sse = true, .has_sse2 = true, .has_sse3 = true, .has_ssse3 = true, .has_sse41 = true
        },
        .gpu = {
            .type = OV_GPU_NVIDIA_GEFORCE,
            .arch_gen = OV_GPU_ARCH_NVIDIA_TESLA,
            .model_name = "NVIDIA GeForce 9400M",
            .vendor_id = 0x10DE, .device_id = 0x0863,
            .vram_mb = 256, .vram_bytes = 256ULL * 1024 * 1024,
            .vram_type = "DDR3 Shared",
            .eu_count = 16, .max_texture_dimension = 8192,
            .supports_opengl_core = true, .opengl_major = 3, .opengl_minor = 3,
            .metal_level = OV_METAL_NONE
        },
        .mem = { .total_bytes = 4ULL * 1024 * 1024 * 1024, .memory_type = "DDR3-1066", .channels = 2 },
        .display_info = "13.3-inch Glossy (1280x800)",
        .storage_interface = "SATA 3.0 Gb/s",
        .firmware_type = "Apple EFI 64-bit",
        .efi_is_64bit = true,
        .native_macos_min = "Mac OS X 10.5.5 Leopard",
        .native_macos_max = "OS X 10.11.6 El Capitan",
        .silicon_quirks = "MCP79 AHCI SATA link negotiation bug; Penryn SSE4.1 supported.",
        .is_simulated = true
    },

    /* 4: MacPro1,1 (2006, Woodcrest Xeon, 32-bit EFI with 64-bit CPU) */
    {
        .profile_id = OV_HW_PROFILE_MP11_WOODCREST,
        .source = OV_HW_SOURCE_SIMULATED_PROFILE,
        .model_identifier = "MacPro1,1",
        .profile_name = "MacPro1,1",
        .marketing_name = "Mac Pro (First Generation, 2006)",
        .description = "Dual dual-core Woodcrest Xeons. 64-bit CPUs paired with 32-bit EFI firmware.",
        .cpu = {
            .type = OV_CPU_INTEL_XEON_CORE,
            .model_name = "Dual Intel Xeon 5150",
            .cores = 4, .threads = 4,
            .base_freq_mhz = 2660, .max_freq_mhz = 2660,
            .is_64bit = true,
            .has_sse = true, .has_sse2 = true, .has_sse3 = true, .has_ssse3 = true
        },
        .gpu = {
            .type = OV_GPU_NVIDIA_GEFORCE,
            .arch_gen = OV_GPU_ARCH_NVIDIA_TESLA,
            .model_name = "NVIDIA GeForce 7300 GT",
            .vendor_id = 0x10DE, .device_id = 0x0393,
            .vram_mb = 256, .vram_bytes = 256ULL * 1024 * 1024,
            .vram_type = "GDDR2",
            .eu_count = 8, .max_texture_dimension = 4096,
            .supports_opengl_core = true, .opengl_major = 2, .opengl_minor = 1,
            .metal_level = OV_METAL_NONE
        },
        .mem = { .total_bytes = 8ULL * 1024 * 1024 * 1024, .memory_type = "FB-DIMM DDR2-667 ECC", .has_ecc = true },
        .storage_interface = "SATA 3.0 Gb/s (4 Bays)",
        .firmware_type = "Apple EFI 32-bit (v1.10)",
        .efi_is_64bit = false, /* 32-bit EFI bottleneck! */
        .native_macos_min = "Mac OS X 10.4.7 Tiger",
        .native_macos_max = "Mac OS X 10.7.5 Lion",
        .silicon_quirks = "32-bit EFI block on 64-bit OS kernels; requires custom 32-to-64 bit bootloader (PikeYolu/OpenCore).",
        .is_simulated = true
    },

    /* 5: MacPro3,1 (2008, Harpertown Xeon, 64-bit EFI) */
    {
        .profile_id = OV_HW_PROFILE_MP31_HARPERTOWN,
        .source = OV_HW_SOURCE_SIMULATED_PROFILE,
        .model_identifier = "MacPro3,1",
        .profile_name = "MacPro3,1",
        .marketing_name = "Mac Pro (Early 2008)",
        .description = "Dual quad-core Harpertown Xeons (8 cores total), genuine 64-bit EFI firmware.",
        .cpu = {
            .type = OV_CPU_INTEL_XEON_CORE,
            .model_name = "Dual Intel Xeon E5462",
            .cores = 8, .threads = 8,
            .base_freq_mhz = 2800, .max_freq_mhz = 2800,
            .is_64bit = true,
            .has_sse = true, .has_sse2 = true, .has_sse3 = true, .has_ssse3 = true, .has_sse41 = true
        },
        .gpu = {
            .type = OV_GPU_AMD_RADEON,
            .arch_gen = OV_GPU_ARCH_AMD_TERASCALE_1,
            .model_name = "ATI Radeon HD 2600 XT",
            .vendor_id = 0x1002, .device_id = 0x9588,
            .vram_mb = 256, .vram_bytes = 256ULL * 1024 * 1024,
            .vram_type = "GDDR3",
            .eu_count = 24, .max_texture_dimension = 8192,
            .supports_opengl_core = true, .opengl_major = 3, .opengl_minor = 3,
            .metal_level = OV_METAL_NONE
        },
        .mem = { .total_bytes = 16ULL * 1024 * 1024 * 1024, .memory_type = "FB-DIMM DDR2-800 ECC", .has_ecc = true },
        .storage_interface = "SATA 3.0 Gb/s (4 Bays)",
        .firmware_type = "Apple EFI 64-bit",
        .efi_is_64bit = true,
        .native_macos_min = "Mac OS X 10.5.1 Leopard",
        .native_macos_max = "OS X 10.11.6 El Capitan",
        .silicon_quirks = "Lacks SSE4.2 and AVX; FB-DIMM high thermal footprint; upgraded GPUs unlock Metal.",
        .is_simulated = true
    },

    /* 6: MacPro4,1 (2009, Nehalem Xeon, Radeon HD 4870) */
    {
        .profile_id = OV_HW_PROFILE_MP41_NEHALEM,
        .source = OV_HW_SOURCE_SIMULATED_PROFILE,
        .model_identifier = "MacPro4,1",
        .profile_name = "MacPro4,1",
        .marketing_name = "Mac Pro (Early 2009)",
        .description = "Nehalem architecture with QuickPath Interconnect (QPI) and integrated memory controller.",
        .cpu = {
            .type = OV_CPU_INTEL_NEHALEM,
            .model_name = "Dual Intel Xeon E5520",
            .cores = 8, .threads = 16,
            .base_freq_mhz = 2260, .max_freq_mhz = 2530,
            .is_64bit = true,
            .has_sse = true, .has_sse2 = true, .has_sse3 = true, .has_ssse3 = true,
            .has_sse41 = true, .has_sse42 = true
        },
        .gpu = {
            .type = OV_GPU_AMD_RADEON,
            .arch_gen = OV_GPU_ARCH_AMD_TERASCALE_1,
            .model_name = "ATI Radeon HD 4870",
            .vendor_id = 0x1002, .device_id = 0x9440,
            .vram_mb = 512, .vram_bytes = 512ULL * 1024 * 1024,
            .vram_type = "GDDR5",
            .eu_count = 160, .max_texture_dimension = 8192,
            .supports_opengl_core = true, .opengl_major = 3, .opengl_minor = 3,
            .metal_level = OV_METAL_NONE
        },
        .mem = { .total_bytes = 16ULL * 1024 * 1024 * 1024, .memory_type = "DDR3-1066 ECC", .has_ecc = true },
        .storage_interface = "SATA 3.0 Gb/s",
        .firmware_type = "Apple EFI 64-bit",
        .efi_is_64bit = true,
        .native_macos_min = "Mac OS X 10.5.6 Leopard",
        .native_macos_max = "OS X 10.11.6 El Capitan",
        .silicon_quirks = "Upgradeable to 5,1 firmware; lacks AVX; TeraScale 1 lacks Metal support.",
        .is_simulated = true
    },

    /* 7: MacPro5,1 (2010/2012, Westmere Xeon, Radeon HD 5770) */
    {
        .profile_id = OV_HW_PROFILE_MP51_WESTMERE,
        .source = OV_HW_SOURCE_SIMULATED_PROFILE,
        .model_identifier = "MacPro5,1",
        .profile_name = "MacPro5,1",
        .marketing_name = "Mac Pro (Mid 2010 / Mid 2012)",
        .description = "Westmere 6-core / 12-core workstation with PCIe 2.0 expansion and triple-channel memory.",
        .cpu = {
            .type = OV_CPU_INTEL_NEHALEM,
            .model_name = "Dual Intel Xeon X5680",
            .cores = 12, .threads = 24,
            .base_freq_mhz = 3330, .max_freq_mhz = 3600,
            .is_64bit = true,
            .has_sse = true, .has_sse2 = true, .has_sse3 = true, .has_ssse3 = true,
            .has_sse41 = true, .has_sse42 = true, .has_aesni = true
        },
        .gpu = {
            .type = OV_GPU_AMD_RADEON,
            .arch_gen = OV_GPU_ARCH_AMD_TERASCALE_2,
            .model_name = "ATI Radeon HD 5770",
            .vendor_id = 0x1002, .device_id = 0x68B8,
            .vram_mb = 1024, .vram_bytes = 1024ULL * 1024 * 1024,
            .vram_type = "GDDR5",
            .eu_count = 160, .max_texture_dimension = 16384,
            .supports_opengl_core = true, .opengl_major = 4, .opengl_minor = 1,
            .metal_level = OV_METAL_NONE
        },
        .mem = { .total_bytes = 32ULL * 1024 * 1024 * 1024, .memory_type = "DDR3-1333 ECC", .has_ecc = true },
        .storage_interface = "SATA 3.0 Gb/s",
        .firmware_type = "Apple EFI 64-bit (144.0.0.0.0 APFS)",
        .efi_is_64bit = true,
        .apfs_supported_in_firmware = true,
        .native_macos_min = "Mac OS X 10.6.4 Snow Leopard",
        .native_macos_max = "macOS 10.14.6 Mojave",
        .silicon_quirks = "Native Mojave support requires Metal-capable GPU (e.g. RX 580); lacks AVX and AVX2.",
        .is_simulated = true
    },

    /* 8: MacBookPro8,1 (Sandy Bridge 13" 2011, Intel HD 3000) */
    {
        .profile_id = OV_HW_PROFILE_MBP81_SANDY_BRIDGE,
        .source = OV_HW_SOURCE_SIMULATED_PROFILE,
        .model_identifier = "MacBookPro8,1",
        .profile_name = "MacBookPro8,1",
        .marketing_name = "MacBook Pro (13-inch, Early/Late 2011)",
        .description = "Sandy Bridge 2nd gen Core i5 with Intel HD Graphics 3000.",
        .cpu = {
            .type = OV_CPU_INTEL_SANDY_BRIDGE,
            .model_name = "Intel Core i5-2415M",
            .cores = 2, .threads = 4,
            .base_freq_mhz = 2300, .max_freq_mhz = 2900,
            .is_64bit = true,
            .has_sse = true, .has_sse2 = true, .has_sse3 = true, .has_ssse3 = true,
            .has_sse41 = true, .has_sse42 = true, .has_avx = true, .has_aesni = true
        },
        .gpu = {
            .type = OV_GPU_INTEL_GEN7_HD4000,
            .arch_gen = OV_GPU_ARCH_INTEL_GEN6,
            .model_name = "Intel HD Graphics 3000",
            .vendor_id = 0x8086, .device_id = 0x0126,
            .vram_mb = 512, .vram_bytes = 512ULL * 1024 * 1024,
            .vram_type = "DDR3 Shared",
            .eu_count = 12, .max_texture_dimension = 8192,
            .supports_opengl_core = true, .opengl_major = 3, .opengl_minor = 3,
            .metal_level = OV_METAL_NONE
        },
        .mem = { .total_bytes = 8ULL * 1024 * 1024 * 1024, .memory_type = "DDR3-1333", .channels = 2 },
        .display_info = "13.3-inch (1280x800)",
        .storage_interface = "SATA 6.0 Gb/s",
        .firmware_type = "Apple EFI 64-bit",
        .efi_is_64bit = true,
        .native_macos_min = "Mac OS X 10.6.6 Snow Leopard",
        .native_macos_max = "macOS 10.13.6 High Sierra",
        .silicon_quirks = "HD 3000 lacks Metal hardware support; dropped in Mojave (requires non-Metal patch).",
        .is_simulated = true
    },

    /* 9: MacBookPro8,2 (Sandy Bridge 15" 2011, Intel HD 3000 + AMD Radeon HD 6750M) */
    {
        .profile_id = OV_HW_PROFILE_MBP82_SANDY_BRIDGE,
        .source = OV_HW_SOURCE_SIMULATED_PROFILE,
        .model_identifier = "MacBookPro8,2",
        .profile_name = "MacBookPro8,2",
        .marketing_name = "MacBook Pro (15-inch, Early/Late 2011)",
        .description = "Sandy Bridge quad-core i7 with dual switchable graphics (HD 3000 + Radeon HD 6750M).",
        .cpu = {
            .type = OV_CPU_INTEL_SANDY_BRIDGE,
            .model_name = "Intel Core i7-2720QM",
            .cores = 4, .threads = 8,
            .base_freq_mhz = 2200, .max_freq_mhz = 3300,
            .is_64bit = true,
            .has_sse = true, .has_sse2 = true, .has_sse3 = true, .has_ssse3 = true,
            .has_sse41 = true, .has_sse42 = true, .has_avx = true, .has_aesni = true
        },
        .gpu = {
            .type = OV_GPU_INTEL_GEN7_HD4000,
            .arch_gen = OV_GPU_ARCH_INTEL_GEN6,
            .model_name = "Intel HD Graphics 3000 (Integrated)",
            .vendor_id = 0x8086, .device_id = 0x0126,
            .vram_mb = 512, .vram_bytes = 512ULL * 1024 * 1024,
            .vram_type = "DDR3 Shared",
            .eu_count = 12, .max_texture_dimension = 8192,
            .supports_opengl_core = true, .opengl_major = 3, .opengl_minor = 3,
            .metal_level = OV_METAL_NONE
        },
        .has_discrete_gpu = true,
        .is_switchable_graphics = true,
        .secondary_gpu = {
            .type = OV_GPU_AMD_RADEON,
            .arch_gen = OV_GPU_ARCH_AMD_TERASCALE_2,
            .model_name = "AMD Radeon HD 6750M (Discrete)",
            .vendor_id = 0x1002, .device_id = 0x6741,
            .vram_mb = 1024, .vram_bytes = 1024ULL * 1024 * 1024,
            .vram_type = "GDDR5",
            .eu_count = 480, .max_texture_dimension = 16384,
            .supports_opengl_core = true, .opengl_major = 4, .opengl_minor = 1,
            .metal_level = OV_METAL_NONE
        },
        .mem = { .total_bytes = 8ULL * 1024 * 1024 * 1024, .memory_type = "DDR3-1333", .channels = 2 },
        .display_info = "15.4-inch (1440x900 / 1680x1050)",
        .storage_interface = "SATA 6.0 Gb/s",
        .firmware_type = "Apple EFI 64-bit",
        .efi_is_64bit = true,
        .native_macos_min = "Mac OS X 10.6.6 Snow Leopard",
        .native_macos_max = "macOS 10.13.6 High Sierra",
        .silicon_quirks = "Radeon HD 6750M GPU failure/desoldering defect; GMUX power switching workaround required.",
        .is_simulated = true
    },

    /* 10: MacBookPro9,1 (CRITICAL: Ivy Bridge 15" Mid 2012, Core i7, HD 4000 + GeForce GT 650M) */
    {
        .profile_id = OV_HW_PROFILE_MBP91_IVY_BRIDGE,
        .source = OV_HW_SOURCE_SIMULATED_PROFILE,
        .model_identifier = "MacBookPro9,1",
        .profile_name = "MacBookPro9,1",
        .marketing_name = "MacBook Pro (15-inch, Mid 2012)",
        .description = "Unibody 15-inch Ivy Bridge quad-core i7 with dual switchable graphics (Intel HD 4000 + NVIDIA GeForce GT 650M Kepler).",
        .cpu = {
            .type = OV_CPU_INTEL_IVY_BRIDGE,
            .model_name = "Intel Core i7-3615QM",
            .cores = 4, .threads = 8,
            .base_freq_mhz = 2300, .max_freq_mhz = 3300,
            .is_64bit = true,
            .l1_cache_kb = 32, .l2_cache_kb = 256, .l3_cache_kb = 6144,
            .has_sse = true, .has_sse2 = true, .has_sse3 = true, .has_ssse3 = true,
            .has_sse41 = true, .has_sse42 = true,
            .has_avx = true,
            .has_avx2 = false, /* Ivy Bridge has AVX, NO AVX2 */
            .has_avx512 = false,
            .has_fma = false,  /* FMA3 was introduced in Haswell */
            .has_aesni = true,
            .has_tsc = true
        },
        .gpu = {
            .type = OV_GPU_INTEL_GEN7_HD4000,
            .arch_gen = OV_GPU_ARCH_INTEL_GEN7,
            .model_name = "Intel HD Graphics 4000 (Integrated)",
            .vendor_id = 0x8086, .device_id = 0x0166,
            .subvendor_id = 0x106B, .subdevice_id = 0x00F8,
            .vram_mb = 1536, .vram_bytes = 1536ULL * 1024 * 1024,
            .vram_type = "DDR3 Shared",
            .eu_count = 16,
            .max_texture_dimension = 8192,
            .has_graphics = true, .has_compute = true,
            .supports_compute = true, .supports_tessellation = true,
            .supports_opengl_core = true, .opengl_major = 3, .opengl_minor = 3,
            .supports_metal = true,
            .metal_level = OV_METAL_1,
            .supports_vulkan = false,
            .supports_directx = false,
            .driver_version = "AppleIntelHD4000Graphics 14.0.0"
        },
        .has_discrete_gpu = true,
        .is_switchable_graphics = true,
        .secondary_gpu = {
            .type = OV_GPU_NVIDIA_GEFORCE,
            .arch_gen = OV_GPU_ARCH_NVIDIA_KEPLER,
            .model_name = "NVIDIA GeForce GT 650M (Discrete GK107)",
            .vendor_id = 0x10DE, .device_id = 0x0FD5,
            .subvendor_id = 0x106B, .subdevice_id = 0x00F8,
            .vram_mb = 1024, .vram_bytes = 1024ULL * 1024 * 1024,
            .vram_type = "GDDR5",
            .eu_count = 384, /* 384 CUDA Kepler GK107 cores */
            .max_texture_dimension = 16384,
            .has_graphics = true, .has_compute = true,
            .supports_compute = true, .supports_tessellation = true,
            .supports_opengl_core = true, .opengl_major = 4, .opengl_minor = 1,
            .supports_metal = true,
            .metal_level = OV_METAL_2,
            .supports_vulkan = true,
            .supports_directx = true,
            .driver_version = "GeForce.kext 355.11.10.50 (Kepler)"
        },
        .mem = {
            .total_bytes = 8ULL * 1024 * 1024 * 1024,
            .available_bytes = 7ULL * 1024 * 1024 * 1024,
            .channels = 2,
            .frequency_mhz = 1600,
            .memory_type = "DDR3-1600 (PC3-12800)",
            .has_ecc = false
        },
        .display_info = "15.4-inch LED-backlit TFT (1440x900 / 1680x1050 Hi-Res)",
        .storage_interface = "SATA 6.0 Gb/s (Upgradable 2.5\" SSD/HDD)",
        .firmware_type = "Apple EFI 64-bit (MBP91.88Z.00D7.B00)",
        .efi_is_64bit = true,
        .apfs_supported_in_firmware = true,
        .native_macos_min = "OS X 10.7.4 Lion",
        .native_macos_max = "macOS 10.15.7 Catalina",
        .silicon_quirks = "Apple GMUX switchable graphics; Kepler GPU native drivers removed in macOS 12 Monterey (patchable via OCLP); Ventura+ drops HD 4000 graphics acceleration without legacy root patches.",
        .is_simulated = true
    },

    /* 11: MacBookAir5,2 (Ivy Bridge 13" Mid 2012, HD 4000) */
    {
        .profile_id = OV_HW_PROFILE_MBA52_IVY_BRIDGE,
        .source = OV_HW_SOURCE_SIMULATED_PROFILE,
        .model_identifier = "MacBookAir5,2",
        .profile_name = "MacBookAir5,2",
        .marketing_name = "MacBook Air (13-inch, Mid 2012)",
        .description = "Ultraportable Ivy Bridge dual-core i5 with integrated Intel HD Graphics 4000.",
        .cpu = {
            .type = OV_CPU_INTEL_IVY_BRIDGE,
            .model_name = "Intel Core i5-3427U",
            .cores = 2, .threads = 4,
            .base_freq_mhz = 1800, .max_freq_mhz = 2800,
            .is_64bit = true,
            .has_sse = true, .has_sse2 = true, .has_sse3 = true, .has_ssse3 = true,
            .has_sse41 = true, .has_sse42 = true, .has_avx = true, .has_aesni = true
        },
        .gpu = {
            .type = OV_GPU_INTEL_GEN7_HD4000,
            .arch_gen = OV_GPU_ARCH_INTEL_GEN7,
            .model_name = "Intel HD Graphics 4000",
            .vendor_id = 0x8086, .device_id = 0x0166,
            .vram_mb = 1536, .vram_bytes = 1536ULL * 1024 * 1024,
            .vram_type = "DDR3L Shared",
            .eu_count = 16, .max_texture_dimension = 8192,
            .supports_opengl_core = true, .opengl_major = 3, .opengl_minor = 3,
            .metal_level = OV_METAL_1
        },
        .mem = { .total_bytes = 4ULL * 1024 * 1024 * 1024, .memory_type = "DDR3L-1600", .channels = 2 },
        .display_info = "13.3-inch (1440x900)",
        .storage_interface = "mSATA Proprietary Apple Blade SSD",
        .firmware_type = "Apple EFI 64-bit",
        .efi_is_64bit = true,
        .native_macos_min = "OS X 10.7.4 Lion",
        .native_macos_max = "macOS 10.15.7 Catalina",
        .silicon_quirks = "Lacks AVX2; HD 4000 Metal 1 only; requires OCLP for Big Sur and newer.",
        .is_simulated = true
    },

    /* 12: iMac13,2 (Ivy Bridge 27" Late 2012, GTX 675MX) */
    {
        .profile_id = OV_HW_PROFILE_IMAC132_IVY_BRIDGE,
        .source = OV_HW_SOURCE_SIMULATED_PROFILE,
        .model_identifier = "iMac13,2",
        .profile_name = "iMac13,2",
        .marketing_name = "iMac (27-inch, Late 2012)",
        .description = "Slim tapered iMac 27-inch with quad-core desktop Ivy Bridge and NVIDIA Kepler graphics.",
        .cpu = {
            .type = OV_CPU_INTEL_IVY_BRIDGE,
            .model_name = "Intel Core i5-3470S",
            .cores = 4, .threads = 4,
            .base_freq_mhz = 2900, .max_freq_mhz = 3600,
            .is_64bit = true,
            .has_sse = true, .has_sse2 = true, .has_sse3 = true, .has_ssse3 = true,
            .has_sse41 = true, .has_sse42 = true, .has_avx = true, .has_aesni = true
        },
        .gpu = {
            .type = OV_GPU_NVIDIA_GEFORCE,
            .arch_gen = OV_GPU_ARCH_NVIDIA_KEPLER,
            .model_name = "NVIDIA GeForce GTX 675MX",
            .vendor_id = 0x10DE, .device_id = 0x11A2,
            .vram_mb = 1024, .vram_bytes = 1024ULL * 1024 * 1024,
            .vram_type = "GDDR5",
            .eu_count = 960, .max_texture_dimension = 16384,
            .supports_opengl_core = true, .opengl_major = 4, .opengl_minor = 1,
            .metal_level = OV_METAL_2
        },
        .mem = { .total_bytes = 16ULL * 1024 * 1024 * 1024, .memory_type = "DDR3-1600", .channels = 2 },
        .display_info = "27-inch IPS (2560x1440)",
        .storage_interface = "SATA 6.0 Gb/s + Blade PCIe",
        .firmware_type = "Apple EFI 64-bit",
        .efi_is_64bit = true,
        .native_macos_min = "OS X 10.8.2 Mountain Lion",
        .native_macos_max = "macOS 10.15.7 Catalina",
        .silicon_quirks = "MXM-B form factor GPU; Kepler GPU requires OCLP patch in Monterey.",
        .is_simulated = true
    },

    /* 13: Macmini6,2 (Ivy Bridge Server Late 2012, HD 4000) */
    {
        .profile_id = OV_HW_PROFILE_MM62_IVY_BRIDGE,
        .source = OV_HW_SOURCE_SIMULATED_PROFILE,
        .model_identifier = "Macmini6,2",
        .profile_name = "Macmini6,2",
        .marketing_name = "Mac mini (Late 2012 Quad-Core)",
        .description = "Compact desktop quad-core Ivy Bridge with dual SATA drive bays.",
        .cpu = {
            .type = OV_CPU_INTEL_IVY_BRIDGE,
            .model_name = "Intel Core i7-3720QM",
            .cores = 4, .threads = 8,
            .base_freq_mhz = 2600, .max_freq_mhz = 3600,
            .is_64bit = true,
            .has_sse = true, .has_sse2 = true, .has_sse3 = true, .has_ssse3 = true,
            .has_sse41 = true, .has_sse42 = true, .has_avx = true, .has_aesni = true
        },
        .gpu = {
            .type = OV_GPU_INTEL_GEN7_HD4000,
            .arch_gen = OV_GPU_ARCH_INTEL_GEN7,
            .model_name = "Intel HD Graphics 4000",
            .vendor_id = 0x8086, .device_id = 0x0166,
            .vram_mb = 1536, .vram_bytes = 1536ULL * 1024 * 1024,
            .vram_type = "DDR3 Shared",
            .eu_count = 16, .max_texture_dimension = 8192,
            .supports_opengl_core = true, .opengl_major = 3, .opengl_minor = 3,
            .metal_level = OV_METAL_1
        },
        .mem = { .total_bytes = 16ULL * 1024 * 1024 * 1024, .memory_type = "DDR3-1600", .channels = 2 },
        .display_info = "HDMI + Thunderbolt (2560x1600)",
        .storage_interface = "Dual SATA 6.0 Gb/s",
        .firmware_type = "Apple EFI 64-bit",
        .efi_is_64bit = true,
        .native_macos_min = "OS X 10.8.1 Mountain Lion",
        .native_macos_max = "macOS 10.15.7 Catalina",
        .silicon_quirks = "Lacks discrete GPU; HD 4000 supported up to Catalina natively.",
        .is_simulated = true
    },

    /* 14: MacPro6,1 (Ivy Bridge-EP Late 2013 Trashcan, Dual FirePro D300) */
    {
        .profile_id = OV_HW_PROFILE_MP61_IVY_BRIDGE_EP,
        .source = OV_HW_SOURCE_SIMULATED_PROFILE,
        .model_identifier = "MacPro6,1",
        .profile_name = "MacPro6,1",
        .marketing_name = "Mac Pro (Late 2013, Cylinder)",
        .description = "Cylindrical Mac Pro with Ivy Bridge-EP Xeon E5-1650 v2 and dual AMD FirePro workstation GPUs.",
        .cpu = {
            .type = OV_CPU_INTEL_IVY_BRIDGE,
            .model_name = "Intel Xeon E5-1650 v2",
            .cores = 6, .threads = 12,
            .base_freq_mhz = 3500, .max_freq_mhz = 3900,
            .is_64bit = true,
            .has_sse = true, .has_sse2 = true, .has_sse3 = true, .has_ssse3 = true,
            .has_sse41 = true, .has_sse42 = true, .has_avx = true, .has_aesni = true
        },
        .gpu = {
            .type = OV_GPU_AMD_RADEON,
            .arch_gen = OV_GPU_ARCH_AMD_GCN_1_4,
            .model_name = "Dual AMD FirePro D300 (Pitcairn)",
            .vendor_id = 0x1002, .device_id = 0x6810,
            .vram_mb = 2048, .vram_bytes = 2048ULL * 1024 * 1024,
            .vram_type = "GDDR5",
            .eu_count = 1280, .max_texture_dimension = 16384,
            .supports_opengl_core = true, .opengl_major = 4, .opengl_minor = 1,
            .metal_level = OV_METAL_2
        },
        .mem = { .total_bytes = 32ULL * 1024 * 1024 * 1024, .memory_type = "DDR3-1866 ECC", .has_ecc = true },
        .storage_interface = "PCIe 2.0 x4 Apple Proprietary Blade SSD",
        .firmware_type = "Apple EFI 64-bit",
        .efi_is_64bit = true,
        .native_macos_min = "OS X 10.9.1 Mavericks",
        .native_macos_max = "macOS 12.7.4 Monterey",
        .silicon_quirks = "FirePro D500/D700 GPU VRM thermal degradation quirk; lacks AVX2 requiring OCLP on Ventura+.",
        .is_simulated = true
    },

    /* 15: MacBookAir6,2 (Haswell 13" 2013-2014, HD 5000) */
    {
        .profile_id = OV_HW_PROFILE_MBA62_HASWELL,
        .source = OV_HW_SOURCE_SIMULATED_PROFILE,
        .model_identifier = "MacBookAir6,2",
        .profile_name = "MacBookAir6,2",
        .marketing_name = "MacBook Air (13-inch, Mid 2013 / Early 2014)",
        .description = "Haswell ultraportable with AVX2, FMA, and Intel HD Graphics 5000.",
        .cpu = {
            .type = OV_CPU_INTEL_HASWELL,
            .model_name = "Intel Core i5-4250U",
            .cores = 2, .threads = 4,
            .base_freq_mhz = 1300, .max_freq_mhz = 2600,
            .is_64bit = true,
            .has_sse = true, .has_sse2 = true, .has_sse3 = true, .has_ssse3 = true,
            .has_sse41 = true, .has_sse42 = true, .has_avx = true, .has_avx2 = true,
            .has_fma = true, .has_aesni = true
        },
        .gpu = {
            .type = OV_GPU_INTEL_GEN75_HD4600,
            .arch_gen = OV_GPU_ARCH_INTEL_GEN75,
            .model_name = "Intel HD Graphics 5000",
            .vendor_id = 0x8086, .device_id = 0x0A26,
            .vram_mb = 1536, .vram_bytes = 1536ULL * 1024 * 1024,
            .vram_type = "LPDDR3 Shared",
            .eu_count = 40, .max_texture_dimension = 16384,
            .supports_opengl_core = true, .opengl_major = 4, .opengl_minor = 1,
            .metal_level = OV_METAL_1
        },
        .mem = { .total_bytes = 8ULL * 1024 * 1024 * 1024, .memory_type = "LPDDR3-1600", .channels = 2 },
        .display_info = "13.3-inch (1440x900)",
        .storage_interface = "PCIe 2.0 x2 Apple SSD",
        .firmware_type = "Apple EFI 64-bit",
        .efi_is_64bit = true,
        .native_macos_min = "OS X 10.8.4 Mountain Lion",
        .native_macos_max = "macOS 11.7.10 Big Sur",
        .silicon_quirks = "Haswell graphics dropped in Monterey; AVX2 supported natively.",
        .is_simulated = true
    },

    /* 16: MacBookPro11,3 (Haswell 15" Late 2013 / Mid 2014, Iris Pro 5200 + GeForce GT 750M) */
    {
        .profile_id = OV_HW_PROFILE_MBP113_HASWELL,
        .source = OV_HW_SOURCE_SIMULATED_PROFILE,
        .model_identifier = "MacBookPro11,3",
        .profile_name = "MacBookPro11,3",
        .marketing_name = "MacBook Pro (Retina, 15-inch, Late 2013 / Mid 2014)",
        .description = "Haswell quad-core i7 with 128MB eDRAM Crystalwell cache and discrete NVIDIA GT 750M Kepler.",
        .cpu = {
            .type = OV_CPU_INTEL_HASWELL,
            .model_name = "Intel Core i7-4850HQ",
            .cores = 4, .threads = 8,
            .base_freq_mhz = 2300, .max_freq_mhz = 3500,
            .is_64bit = true,
            .has_sse = true, .has_sse2 = true, .has_sse3 = true, .has_ssse3 = true,
            .has_sse41 = true, .has_sse42 = true, .has_avx = true, .has_avx2 = true,
            .has_fma = true, .has_aesni = true
        },
        .gpu = {
            .type = OV_GPU_INTEL_GEN75_HD4600,
            .arch_gen = OV_GPU_ARCH_INTEL_GEN75,
            .model_name = "Intel Iris Pro 5200 (128MB eDRAM)",
            .vendor_id = 0x8086, .device_id = 0x0D26,
            .vram_mb = 1536, .vram_bytes = 1536ULL * 1024 * 1024,
            .vram_type = "DDR3L Shared + 128MB eDRAM",
            .eu_count = 40, .max_texture_dimension = 16384,
            .supports_opengl_core = true, .opengl_major = 4, .opengl_minor = 1,
            .metal_level = OV_METAL_1
        },
        .has_discrete_gpu = true,
        .is_switchable_graphics = true,
        .secondary_gpu = {
            .type = OV_GPU_NVIDIA_GEFORCE,
            .arch_gen = OV_GPU_ARCH_NVIDIA_KEPLER,
            .model_name = "NVIDIA GeForce GT 750M (Discrete GK107)",
            .vendor_id = 0x10DE, .device_id = 0x0FE9,
            .vram_mb = 2048, .vram_bytes = 2048ULL * 1024 * 1024,
            .vram_type = "GDDR5",
            .eu_count = 384, .max_texture_dimension = 16384,
            .supports_opengl_core = true, .opengl_major = 4, .opengl_minor = 1,
            .metal_level = OV_METAL_2
        },
        .mem = { .total_bytes = 16ULL * 1024 * 1024 * 1024, .memory_type = "DDR3L-1600", .channels = 2 },
        .display_info = "15.4-inch Retina (2880x1800)",
        .storage_interface = "PCIe 2.0 x4 Apple Blade SSD",
        .firmware_type = "Apple EFI 64-bit",
        .efi_is_64bit = true,
        .native_macos_min = "OS X 10.9 Mavericks",
        .native_macos_max = "macOS 11.7.10 Big Sur",
        .silicon_quirks = "Kepler GPU dropped in macOS 12; AVX2 native support allows smooth Ventura OCLP.",
        .is_simulated = true
    },

    /* 17: MacBookPro12,1 (Broadwell 13" Early 2015, Iris 6100) */
    {
        .profile_id = OV_HW_PROFILE_MBP121_BROADWELL,
        .source = OV_HW_SOURCE_SIMULATED_PROFILE,
        .model_identifier = "MacBookPro12,1",
        .profile_name = "MacBookPro12,1",
        .marketing_name = "MacBook Pro (Retina, 13-inch, Early 2015)",
        .description = "Broadwell 14nm 5th gen dual-core i5 with Force Touch trackpad and Intel Iris Graphics 6100.",
        .cpu = {
            .type = OV_CPU_INTEL_BROADWELL,
            .model_name = "Intel Core i5-5257U",
            .cores = 2, .threads = 4,
            .base_freq_mhz = 2700, .max_freq_mhz = 3100,
            .is_64bit = true,
            .has_sse = true, .has_sse2 = true, .has_sse3 = true, .has_ssse3 = true,
            .has_sse41 = true, .has_sse42 = true, .has_avx = true, .has_avx2 = true,
            .has_fma = true, .has_aesni = true
        },
        .gpu = {
            .type = OV_GPU_INTEL_GEN75_HD4600,
            .arch_gen = OV_GPU_ARCH_INTEL_GEN8,
            .model_name = "Intel Iris Graphics 6100",
            .vendor_id = 0x8086, .device_id = 0x162B,
            .vram_mb = 1536, .vram_bytes = 1536ULL * 1024 * 1024,
            .vram_type = "LPDDR3 Shared",
            .eu_count = 48, .max_texture_dimension = 16384,
            .supports_opengl_core = true, .opengl_major = 4, .opengl_minor = 1,
            .metal_level = OV_METAL_2
        },
        .mem = { .total_bytes = 8ULL * 1024 * 1024 * 1024, .memory_type = "LPDDR3-1866", .channels = 2 },
        .display_info = "13.3-inch Retina (2560x1600)",
        .storage_interface = "PCIe 2.0 x4 Apple Blade SSD",
        .firmware_type = "Apple EFI 64-bit",
        .efi_is_64bit = true,
        .native_macos_min = "OS X 10.10.2 Yosemite",
        .native_macos_max = "macOS 12.7.4 Monterey",
        .silicon_quirks = "Broadwell Gen8 Metal 2 support native through Monterey; dropped in Ventura.",
        .is_simulated = true
    },

    /* 18: MacBook9,1 (Skylake 12" Early 2016, HD 515) */
    {
        .profile_id = OV_HW_PROFILE_MB91_SKYLAKE,
        .source = OV_HW_SOURCE_SIMULATED_PROFILE,
        .model_identifier = "MacBook9,1",
        .profile_name = "MacBook9,1",
        .marketing_name = "MacBook (Retina, 12-inch, Early 2016)",
        .description = "Fanless ultra-slim MacBook with Skylake Core m3 and USB-C.",
        .cpu = {
            .type = OV_CPU_INTEL_SKYLAKE,
            .model_name = "Intel Core m3-6Y30",
            .cores = 2, .threads = 4,
            .base_freq_mhz = 1100, .max_freq_mhz = 2200,
            .is_64bit = true,
            .has_sse = true, .has_sse2 = true, .has_sse3 = true, .has_ssse3 = true,
            .has_sse41 = true, .has_sse42 = true, .has_avx = true, .has_avx2 = true,
            .has_fma = true, .has_aesni = true
        },
        .gpu = {
            .type = OV_GPU_INTEL_GEN75_HD4600,
            .arch_gen = OV_GPU_ARCH_INTEL_GEN9,
            .model_name = "Intel HD Graphics 515",
            .vendor_id = 0x8086, .device_id = 0x191E,
            .vram_mb = 1536, .vram_bytes = 1536ULL * 1024 * 1024,
            .vram_type = "LPDDR3 Shared",
            .eu_count = 24, .max_texture_dimension = 16384,
            .supports_opengl_core = true, .opengl_major = 4, .opengl_minor = 1,
            .metal_level = OV_METAL_2
        },
        .mem = { .total_bytes = 8ULL * 1024 * 1024 * 1024, .memory_type = "LPDDR3-1866", .channels = 2 },
        .display_info = "12.0-inch Retina (2304x1440)",
        .storage_interface = "NVMe PCIe 3.0 x2 On-Board SSD",
        .firmware_type = "Apple EFI 64-bit",
        .efi_is_64bit = true,
        .native_macos_min = "OS X 10.11.4 El Capitan",
        .native_macos_max = "macOS 12.7.4 Monterey",
        .silicon_quirks = "Passively cooled; sustained thermal throttling under heavy CPU/GPU load.",
        .is_simulated = true
    },

    /* 19: MacBookPro13,3 (Skylake 15" Late 2016, Iris 550 + Radeon Pro 455) */
    {
        .profile_id = OV_HW_PROFILE_MBP133_SKYLAKE,
        .source = OV_HW_SOURCE_SIMULATED_PROFILE,
        .model_identifier = "MacBookPro13,3",
        .profile_name = "MacBookPro13,3",
        .marketing_name = "MacBook Pro (15-inch, Late 2016, Touch Bar)",
        .description = "First Touch Bar MBP. Skylake quad-core i7 with AMD Polaris Radeon Pro 455 discrete graphics.",
        .cpu = {
            .type = OV_CPU_INTEL_SKYLAKE,
            .model_name = "Intel Core i7-6700HQ",
            .cores = 4, .threads = 8,
            .base_freq_mhz = 2600, .max_freq_mhz = 3500,
            .is_64bit = true,
            .has_sse = true, .has_sse2 = true, .has_sse3 = true, .has_ssse3 = true,
            .has_sse41 = true, .has_sse42 = true, .has_avx = true, .has_avx2 = true,
            .has_fma = true, .has_aesni = true
        },
        .gpu = {
            .type = OV_GPU_INTEL_GEN75_HD4600,
            .arch_gen = OV_GPU_ARCH_INTEL_GEN9,
            .model_name = "Intel HD Graphics 530 (Integrated)",
            .vendor_id = 0x8086, .device_id = 0x191B,
            .vram_mb = 1536, .vram_bytes = 1536ULL * 1024 * 1024,
            .vram_type = "LPDDR3 Shared",
            .eu_count = 24, .max_texture_dimension = 16384,
            .supports_opengl_core = true, .opengl_major = 4, .opengl_minor = 1,
            .metal_level = OV_METAL_2
        },
        .has_discrete_gpu = true,
        .is_switchable_graphics = true,
        .secondary_gpu = {
            .type = OV_GPU_AMD_RADEON,
            .arch_gen = OV_GPU_ARCH_AMD_GCN_1_4,
            .model_name = "AMD Radeon Pro 455 (Polaris)",
            .vendor_id = 0x1002, .device_id = 0x67EF,
            .vram_mb = 2048, .vram_bytes = 2048ULL * 1024 * 1024,
            .vram_type = "GDDR5",
            .eu_count = 768, .max_texture_dimension = 16384,
            .supports_opengl_core = true, .opengl_major = 4, .opengl_minor = 1,
            .metal_level = OV_METAL_2
        },
        .mem = { .total_bytes = 16ULL * 1024 * 1024 * 1024, .memory_type = "LPDDR3-2133", .channels = 2 },
        .display_info = "15.4-inch Retina DCI-P3 (2880x1800)",
        .storage_interface = "NVMe PCIe 3.0 x4",
        .firmware_type = "Apple EFI 64-bit + Apple T1 Security",
        .efi_is_64bit = true,
        .native_macos_min = "macOS 10.12.1 Sierra",
        .native_macos_max = "macOS 12.7.4 Monterey",
        .silicon_quirks = "Skylake dropped in Ventura; Polaris GPU retains native Metal 2/3 compatibility.",
        .is_simulated = true
    },

    /* 20: MacBookPro14,3 (Kaby Lake 15" Mid 2017, Radeon Pro 560) */
    {
        .profile_id = OV_HW_PROFILE_MBP143_KABY_LAKE,
        .source = OV_HW_SOURCE_SIMULATED_PROFILE,
        .model_identifier = "MacBookPro14,3",
        .profile_name = "MacBookPro14,3",
        .marketing_name = "MacBook Pro (15-inch, Mid 2017)",
        .description = "Kaby Lake quad-core i7 with 4GB AMD Radeon Pro 560 discrete graphics.",
        .cpu = {
            .type = OV_CPU_INTEL_KABY_LAKE,
            .model_name = "Intel Core i7-7700HQ",
            .cores = 4, .threads = 8,
            .base_freq_mhz = 2800, .max_freq_mhz = 3800,
            .is_64bit = true,
            .has_sse = true, .has_sse2 = true, .has_sse3 = true, .has_ssse3 = true,
            .has_sse41 = true, .has_sse42 = true, .has_avx = true, .has_avx2 = true,
            .has_fma = true, .has_aesni = true
        },
        .gpu = {
            .type = OV_GPU_INTEL_GEN75_HD4600,
            .arch_gen = OV_GPU_ARCH_INTEL_GEN95,
            .model_name = "Intel HD Graphics 630",
            .vendor_id = 0x8086, .device_id = 0x591B,
            .vram_mb = 1536, .vram_bytes = 1536ULL * 1024 * 1024,
            .vram_type = "LPDDR3 Shared",
            .eu_count = 24, .max_texture_dimension = 16384,
            .metal_level = OV_METAL_2
        },
        .has_discrete_gpu = true,
        .is_switchable_graphics = true,
        .secondary_gpu = {
            .type = OV_GPU_AMD_RADEON,
            .arch_gen = OV_GPU_ARCH_AMD_GCN_1_4,
            .model_name = "AMD Radeon Pro 560",
            .vendor_id = 0x1002, .device_id = 0x67EF,
            .vram_mb = 4096, .vram_bytes = 4096ULL * 1024 * 1024,
            .vram_type = "GDDR5",
            .eu_count = 1024, .max_texture_dimension = 16384,
            .metal_level = OV_METAL_2
        },
        .mem = { .total_bytes = 16ULL * 1024 * 1024 * 1024, .memory_type = "LPDDR3-2133", .channels = 2 },
        .display_info = "15.4-inch Retina (2880x1800)",
        .storage_interface = "NVMe PCIe 3.0 x4",
        .firmware_type = "Apple EFI 64-bit + Apple T1",
        .efi_is_64bit = true,
        .native_macos_min = "macOS 10.12.5 Sierra",
        .native_macos_max = "macOS 13.6.5 Ventura",
        .silicon_quirks = "Kaby Lake dropped in Sonoma; runs Ventura natively with Metal 2.",
        .is_simulated = true
    },

    /* 21: MacBookPro16,1 (Coffee Lake 16" 2019, 8-core i9, Radeon Pro 5500M) */
    {
        .profile_id = OV_HW_PROFILE_MBP161_COFFEE_LAKE,
        .source = OV_HW_SOURCE_SIMULATED_PROFILE,
        .model_identifier = "MacBookPro16,1",
        .profile_name = "MacBookPro16,1",
        .marketing_name = "MacBook Pro (16-inch, 2019)",
        .description = "Last flagship 16-inch Intel MacBook Pro. 8-core Core i9-9880H with AMD Navi RDNA 1 Radeon Pro 5500M and Apple T2.",
        .cpu = {
            .type = OV_CPU_INTEL_COFFEE_LAKE,
            .model_name = "Intel Core i9-9880H",
            .cores = 8, .threads = 16,
            .base_freq_mhz = 2300, .max_freq_mhz = 4800,
            .is_64bit = true,
            .l1_cache_kb = 32, .l2_cache_kb = 256, .l3_cache_kb = 16384,
            .has_sse = true, .has_sse2 = true, .has_sse3 = true, .has_ssse3 = true,
            .has_sse41 = true, .has_sse42 = true, .has_avx = true, .has_avx2 = true,
            .has_fma = true, .has_aesni = true
        },
        .gpu = {
            .type = OV_GPU_INTEL_GEN75_HD4600,
            .arch_gen = OV_GPU_ARCH_INTEL_GEN95,
            .model_name = "Intel UHD Graphics 630",
            .vendor_id = 0x8086, .device_id = 0x3E9B,
            .vram_mb = 1536, .vram_bytes = 1536ULL * 1024 * 1024,
            .vram_type = "DDR4 Shared",
            .eu_count = 24, .max_texture_dimension = 16384,
            .metal_level = OV_METAL_2
        },
        .has_discrete_gpu = true,
        .is_switchable_graphics = true,
        .secondary_gpu = {
            .type = OV_GPU_AMD_RADEON,
            .arch_gen = OV_GPU_ARCH_AMD_RDNA_1_3,
            .model_name = "AMD Radeon Pro 5500M (Navi 14 RDNA)",
            .vendor_id = 0x1002, .device_id = 0x7340,
            .vram_mb = 8192, .vram_bytes = 8192ULL * 1024 * 1024,
            .vram_type = "GDDR6",
            .eu_count = 1536, .max_texture_dimension = 16384,
            .metal_level = OV_METAL_3
        },
        .mem = { .total_bytes = 32ULL * 1024 * 1024 * 1024, .memory_type = "DDR4-2666", .channels = 2 },
        .display_info = "16.0-inch Retina (3072x1920)",
        .storage_interface = "Apple Fabric Encrypted NVMe SSD (T2)",
        .firmware_type = "Apple T2 BridgeOS + UEFI 64-bit",
        .efi_is_64bit = true,
        .native_macos_min = "macOS 10.15.1 Catalina",
        .native_macos_max = "macOS 15 Sequoia (Supported)",
        .silicon_quirks = "Apple T2 Secure Boot controller; VRM power spikes under combined AVX2 + discrete GPU workload.",
        .is_simulated = true
    },

    /* 22: iMac20,1 (Comet Lake 27" 2020, 10th Gen Core i7, Radeon Pro 5500 XT) */
    {
        .profile_id = OV_HW_PROFILE_IMAC201_COMET_LAKE,
        .source = OV_HW_SOURCE_SIMULATED_PROFILE,
        .model_identifier = "iMac20,1",
        .profile_name = "iMac20,1",
        .marketing_name = "iMac (Retina 5K, 27-inch, 2020)",
        .description = "Final Intel 27-inch 5K iMac with 10th-gen 8-core CPU, Radeon Pro 5500 XT RDNA graphics, and T2 chip.",
        .cpu = {
            .type = OV_CPU_INTEL_COMET_LAKE,
            .model_name = "Intel Core i7-10700K",
            .cores = 8, .threads = 16,
            .base_freq_mhz = 3800, .max_freq_mhz = 5100,
            .is_64bit = true,
            .has_sse = true, .has_sse2 = true, .has_sse3 = true, .has_ssse3 = true,
            .has_sse41 = true, .has_sse42 = true, .has_avx = true, .has_avx2 = true,
            .has_fma = true, .has_aesni = true
        },
        .gpu = {
            .type = OV_GPU_AMD_RADEON,
            .arch_gen = OV_GPU_ARCH_AMD_RDNA_1_3,
            .model_name = "AMD Radeon Pro 5500 XT (Navi 14)",
            .vendor_id = 0x1002, .device_id = 0x7340,
            .vram_mb = 8192, .vram_bytes = 8192ULL * 1024 * 1024,
            .vram_type = "GDDR6",
            .eu_count = 1536, .max_texture_dimension = 16384,
            .metal_level = OV_METAL_3
        },
        .mem = { .total_bytes = 32ULL * 1024 * 1024 * 1024, .memory_type = "DDR4-2666", .channels = 2 },
        .display_info = "27-inch Retina 5K (5120x2880 True Tone)",
        .storage_interface = "NVMe PCIe 3.0 x4 (T2)",
        .firmware_type = "Apple T2 + UEFI 64-bit",
        .efi_is_64bit = true,
        .native_macos_min = "macOS 10.15.6 Catalina",
        .native_macos_max = "macOS 15 Sequoia (Supported)",
        .silicon_quirks = "Apple T2 chip requires hardware cryptoprocessor authentication during boot.",
        .is_simulated = true
    },

    /* 23: MacPro7,1 (Cascade Lake Mac Pro 2019, Xeon W-3235, Radeon Pro 580X / Vega II) */
    {
        .profile_id = OV_HW_PROFILE_MP71_CASCADE_LAKE,
        .source = OV_HW_SOURCE_SIMULATED_PROFILE,
        .model_identifier = "MacPro7,1",
        .profile_name = "MacPro7,1",
        .marketing_name = "Mac Pro (2019 Tower/Rack)",
        .description = "Modular workstation with 12-core Cascade Lake Xeon W, 6-channel ECC memory, 8 PCIe slots, and MPX modules.",
        .cpu = {
            .type = OV_CPU_INTEL_CASCADE_LAKE,
            .model_name = "Intel Xeon W-3235",
            .cores = 12, .threads = 24,
            .base_freq_mhz = 3300, .max_freq_mhz = 4400,
            .is_64bit = true,
            .has_sse = true, .has_sse2 = true, .has_sse3 = true, .has_ssse3 = true,
            .has_sse41 = true, .has_sse42 = true, .has_avx = true, .has_avx2 = true,
            .has_avx512 = true, .has_fma = true, .has_aesni = true
        },
        .gpu = {
            .type = OV_GPU_AMD_RADEON,
            .arch_gen = OV_GPU_ARCH_AMD_GCN_1_4,
            .model_name = "AMD Radeon Pro 580X (MPX Module)",
            .vendor_id = 0x1002, .device_id = 0x67DF,
            .vram_mb = 8192, .vram_bytes = 8192ULL * 1024 * 1024,
            .vram_type = "GDDR5",
            .eu_count = 2304, .max_texture_dimension = 16384,
            .metal_level = OV_METAL_3
        },
        .mem = { .total_bytes = 48ULL * 1024 * 1024 * 1024, .memory_type = "DDR4-2933 ECC 6-Channel", .has_ecc = true },
        .storage_interface = "Apple T2 Encrypted NVMe (Dual Blade)",
        .firmware_type = "Apple T2 + UEFI 64-bit",
        .efi_is_64bit = true,
        .native_macos_min = "macOS 10.15.1 Catalina",
        .native_macos_max = "macOS 15 Sequoia (Supported)",
        .silicon_quirks = "AVX-512 frequency downclocking behavior; MPX module PCIe power architecture.",
        .is_simulated = true
    },

    /* 24: MacBookAir10,1 (Apple Silicon M1 2020) */
    {
        .profile_id = OV_HW_PROFILE_MBA101_M1,
        .source = OV_HW_SOURCE_SIMULATED_PROFILE,
        .model_identifier = "MacBookAir10,1",
        .profile_name = "MacBookAir10,1",
        .marketing_name = "MacBook Air (M1, 2020)",
        .description = "First Apple Silicon Mac. 8-core CPU (4P + 4E), 8-core GPU, 16-core Neural Engine, Unified Memory Architecture.",
        .cpu = {
            .type = OV_CPU_ARM64_M1,
            .model_name = "Apple M1 (4 Firestorm + 4 Icestorm)",
            .cores = 8, .threads = 8,
            .base_freq_mhz = 3200, .max_freq_mhz = 3200,
            .is_64bit = true,
            .has_neon = true,
            .has_sse = false, .has_sse2 = false, .has_avx = false, .has_avx2 = false
        },
        .gpu = {
            .type = OV_GPU_APPLE_SILICON,
            .arch_gen = OV_GPU_ARCH_APPLE_SILICON_M1,
            .model_name = "Apple M1 GPU (8 Cores, TBDR)",
            .vendor_id = 0x106B, .device_id = 0x0001,
            .vram_mb = 16384, .vram_bytes = 16384ULL * 1024 * 1024,
            .vram_type = "Unified LPDDR4X-4266",
            .eu_count = 128, .max_texture_dimension = 16384,
            .supports_metal = true,
            .metal_level = OV_METAL_3,
            .supports_opengl_core = true, .opengl_major = 4, .opengl_minor = 1
        },
        .mem = { .total_bytes = 16ULL * 1024 * 1024 * 1024, .memory_type = "Unified LPDDR4X-4266", .channels = 2 },
        .display_info = "13.3-inch Retina True Tone (2560x1600)",
        .storage_interface = "Apple Fabric NAND Controller",
        .firmware_type = "Apple Silicon iBoot / Secure Enclave",
        .efi_is_64bit = true,
        .native_macos_min = "macOS 11.0 Big Sur",
        .native_macos_max = "macOS 15 Sequoia (Supported)",
        .silicon_quirks = "ARM64 pure ISA; executes x86_64 binaries via Rosetta 2 runtime translation.",
        .is_simulated = true
    },

    /* 25: MacBookPro18,1 (M1 Pro 16" 2021) */
    {
        .profile_id = OV_HW_PROFILE_MBP181_M1_PRO,
        .source = OV_HW_SOURCE_SIMULATED_PROFILE,
        .model_identifier = "MacBookPro18,1",
        .profile_name = "MacBookPro18,1",
        .marketing_name = "MacBook Pro (16-inch, 2021 M1 Pro)",
        .description = "10-core CPU (8P + 2E), 16-core GPU, Liquid Retina XDR mini-LED display, 200GB/s memory bandwidth.",
        .cpu = {
            .type = OV_CPU_ARM64_M1,
            .model_name = "Apple M1 Pro (8 Firestorm + 2 Icestorm)",
            .cores = 10, .threads = 10,
            .base_freq_mhz = 3220, .max_freq_mhz = 3220,
            .is_64bit = true,
            .has_neon = true
        },
        .gpu = {
            .type = OV_GPU_APPLE_SILICON,
            .arch_gen = OV_GPU_ARCH_APPLE_SILICON_M1,
            .model_name = "Apple M1 Pro GPU (16 Cores)",
            .vendor_id = 0x106B, .device_id = 0x0002,
            .vram_mb = 32768, .vram_bytes = 32768ULL * 1024 * 1024,
            .vram_type = "Unified LPDDR5-6400 (200 GB/s)",
            .eu_count = 256, .max_texture_dimension = 16384,
            .metal_level = OV_METAL_3
        },
        .mem = { .total_bytes = 32ULL * 1024 * 1024 * 1024, .memory_type = "Unified LPDDR5-6400", .channels = 4 },
        .display_info = "16.2-inch Liquid Retina XDR ProMotion 120Hz (3456x2234)",
        .storage_interface = "Apple Fabric NAND",
        .firmware_type = "Apple iBoot",
        .efi_is_64bit = true,
        .native_macos_min = "macOS 12.0 Monterey",
        .native_macos_max = "macOS 15 Sequoia (Supported)",
        .silicon_quirks = "Unified Memory Architecture zero-copy buffer sharing between CPU and GPU.",
        .is_simulated = true
    },

    /* 26: Mac13,1 (Mac Studio 2022 M1 Ultra) */
    {
        .profile_id = OV_HW_PROFILE_MAC131_M1_ULTRA,
        .source = OV_HW_SOURCE_SIMULATED_PROFILE,
        .model_identifier = "Mac13,1",
        .profile_name = "Mac13,1",
        .marketing_name = "Mac Studio (2022 M1 Ultra)",
        .description = "UltraFusion dual-die interconnect. 20-core CPU, 64-core GPU, 800GB/s memory bandwidth.",
        .cpu = {
            .type = OV_CPU_ARM64_M1,
            .model_name = "Apple M1 Ultra (16 Firestorm + 4 Icestorm)",
            .cores = 20, .threads = 20,
            .base_freq_mhz = 3220, .max_freq_mhz = 3220,
            .is_64bit = true,
            .has_neon = true
        },
        .gpu = {
            .type = OV_GPU_APPLE_SILICON,
            .arch_gen = OV_GPU_ARCH_APPLE_SILICON_M1,
            .model_name = "Apple M1 Ultra GPU (64 Cores)",
            .vendor_id = 0x106B, .device_id = 0x0004,
            .vram_mb = 131072, .vram_bytes = 131072ULL * 1024 * 1024,
            .vram_type = "Unified LPDDR5-6400 (800 GB/s)",
            .eu_count = 1024, .max_texture_dimension = 16384,
            .metal_level = OV_METAL_3
        },
        .mem = { .total_bytes = 128ULL * 1024 * 1024 * 1024, .memory_type = "Unified LPDDR5-6400 800GB/s", .channels = 8 },
        .storage_interface = "Dual Apple Fabric NAND",
        .firmware_type = "Apple iBoot",
        .efi_is_64bit = true,
        .native_macos_min = "macOS 12.3 Monterey",
        .native_macos_max = "macOS 15 Sequoia (Supported)",
        .silicon_quirks = "UltraFusion 2.5TB/s inter-die latency characteristics.",
        .is_simulated = true
    },

    /* 27: Mac14,2 (MacBook Air M2 2022) */
    {
        .profile_id = OV_HW_PROFILE_MAC142_M2,
        .source = OV_HW_SOURCE_SIMULATED_PROFILE,
        .model_identifier = "Mac14,2",
        .profile_name = "Mac14,2",
        .marketing_name = "MacBook Air (M2, 2022)",
        .description = "Redesigned M2 MacBook Air with 10-core GPU, ProRes video engine, and MagSafe 3.",
        .cpu = {
            .type = OV_CPU_ARM64_M2,
            .model_name = "Apple M2 (4 Avalanche + 4 Blizzard)",
            .cores = 8, .threads = 8,
            .base_freq_mhz = 3490, .max_freq_mhz = 3490,
            .is_64bit = true,
            .has_neon = true
        },
        .gpu = {
            .type = OV_GPU_APPLE_SILICON,
            .arch_gen = OV_GPU_ARCH_APPLE_SILICON_M2,
            .model_name = "Apple M2 GPU (10 Cores)",
            .vendor_id = 0x106B, .device_id = 0x0010,
            .vram_mb = 16384, .vram_bytes = 16384ULL * 1024 * 1024,
            .vram_type = "Unified LPDDR5-6400 (100 GB/s)",
            .eu_count = 160, .max_texture_dimension = 16384,
            .metal_level = OV_METAL_3
        },
        .mem = { .total_bytes = 16ULL * 1024 * 1024 * 1024, .memory_type = "Unified LPDDR5-6400", .channels = 2 },
        .display_info = "13.6-inch Liquid Retina (2560x1664)",
        .storage_interface = "Apple Fabric NAND",
        .firmware_type = "Apple iBoot",
        .efi_is_64bit = true,
        .native_macos_min = "macOS 12.4 Monterey",
        .native_macos_max = "macOS 15 Sequoia (Supported)",
        .silicon_quirks = "Base 256GB model single-NAND read/write bandwidth constraint.",
        .is_simulated = true
    },

    /* 28: Mac14,6 (MacBook Pro 16" M2 Max 2023) */
    {
        .profile_id = OV_HW_PROFILE_MAC146_M2_MAX,
        .source = OV_HW_SOURCE_SIMULATED_PROFILE,
        .model_identifier = "Mac14,6",
        .profile_name = "Mac14,6",
        .marketing_name = "MacBook Pro (16-inch, 2023 M2 Max)",
        .description = "12-core CPU, 38-core GPU, 400GB/s unified memory bandwidth, dual ProRes encode engines.",
        .cpu = {
            .type = OV_CPU_ARM64_M2,
            .model_name = "Apple M2 Max (8 Avalanche + 4 Blizzard)",
            .cores = 12, .threads = 12,
            .base_freq_mhz = 3680, .max_freq_mhz = 3680,
            .is_64bit = true,
            .has_neon = true
        },
        .gpu = {
            .type = OV_GPU_APPLE_SILICON,
            .arch_gen = OV_GPU_ARCH_APPLE_SILICON_M2,
            .model_name = "Apple M2 Max GPU (38 Cores)",
            .vendor_id = 0x106B, .device_id = 0x0012,
            .vram_mb = 65536, .vram_bytes = 65536ULL * 1024 * 1024,
            .vram_type = "Unified LPDDR5-6400 (400 GB/s)",
            .eu_count = 608, .max_texture_dimension = 16384,
            .metal_level = OV_METAL_3
        },
        .mem = { .total_bytes = 64ULL * 1024 * 1024 * 1024, .memory_type = "Unified LPDDR5-6400", .channels = 8 },
        .display_info = "16.2-inch Liquid Retina XDR (3456x2234)",
        .storage_interface = "Apple Fabric NAND",
        .firmware_type = "Apple iBoot",
        .efi_is_64bit = true,
        .native_macos_min = "macOS 13.2 Ventura",
        .native_macos_max = "macOS 15 Sequoia (Supported)",
        .silicon_quirks = "Metal 3 Fast Resource Loading and Mesh Shaders accelerated.",
        .is_simulated = true
    },

    /* 29: Mac14,14 (Mac Pro M2 Ultra 2023) */
    {
        .profile_id = OV_HW_PROFILE_MAC1414_M2_ULTRA,
        .source = OV_HW_SOURCE_SIMULATED_PROFILE,
        .model_identifier = "Mac14,14",
        .profile_name = "Mac14,14",
        .marketing_name = "Mac Pro (2023 M2 Ultra)",
        .description = "PCIe Gen4 expansion workstation powered by 24-core M2 Ultra with 76-core GPU.",
        .cpu = {
            .type = OV_CPU_ARM64_M2,
            .model_name = "Apple M2 Ultra (16 Avalanche + 8 Blizzard)",
            .cores = 24, .threads = 24,
            .base_freq_mhz = 3680, .max_freq_mhz = 3680,
            .is_64bit = true,
            .has_neon = true
        },
        .gpu = {
            .type = OV_GPU_APPLE_SILICON,
            .arch_gen = OV_GPU_ARCH_APPLE_SILICON_M2,
            .model_name = "Apple M2 Ultra GPU (76 Cores)",
            .vendor_id = 0x106B, .device_id = 0x0014,
            .vram_mb = 196608, .vram_bytes = 196608ULL * 1024 * 1024,
            .vram_type = "Unified LPDDR5-6400 (800 GB/s)",
            .eu_count = 1216, .max_texture_dimension = 16384,
            .metal_level = OV_METAL_3
        },
        .mem = { .total_bytes = 192ULL * 1024 * 1024 * 1024, .memory_type = "Unified LPDDR5-6400", .channels = 16 },
        .storage_interface = "PCIe Gen 4 + Apple Fabric NAND",
        .firmware_type = "Apple iBoot",
        .efi_is_64bit = true,
        .native_macos_min = "macOS 13.4 Ventura",
        .native_macos_max = "macOS 15 Sequoia (Supported)",
        .silicon_quirks = "PCIe expansion slots do not support external discrete GPUs; DSP/Audio/Capture cards only.",
        .is_simulated = true
    },

    /* 30: Mac15,3 (MacBook Pro 14" M3 2023) */
    {
        .profile_id = OV_HW_PROFILE_MAC153_M3,
        .source = OV_HW_SOURCE_SIMULATED_PROFILE,
        .model_identifier = "Mac15,3",
        .profile_name = "Mac15,3",
        .marketing_name = "MacBook Pro (14-inch, Nov 2023 M3)",
        .description = "3nm Apple M3 chip with hardware-accelerated ray tracing, mesh shading, and Dynamic Caching GPU.",
        .cpu = {
            .type = OV_CPU_ARM64_M3,
            .model_name = "Apple M3 (4 Everest + 4 Sawtooth)",
            .cores = 8, .threads = 8,
            .base_freq_mhz = 4050, .max_freq_mhz = 4050,
            .is_64bit = true,
            .has_neon = true
        },
        .gpu = {
            .type = OV_GPU_APPLE_SILICON,
            .arch_gen = OV_GPU_ARCH_APPLE_SILICON_M3,
            .model_name = "Apple M3 GPU (10 Cores, Dynamic Caching + RT)",
            .vendor_id = 0x106B, .device_id = 0x0020,
            .vram_mb = 16384, .vram_bytes = 16384ULL * 1024 * 1024,
            .vram_type = "Unified LPDDR5-6400 (100 GB/s)",
            .eu_count = 160, .max_texture_dimension = 16384,
            .metal_level = OV_METAL_3
        },
        .mem = { .total_bytes = 16ULL * 1024 * 1024 * 1024, .memory_type = "Unified LPDDR5-6400", .channels = 2 },
        .display_info = "14.2-inch Liquid Retina XDR (3024x1964)",
        .storage_interface = "Apple Fabric NAND",
        .firmware_type = "Apple iBoot",
        .efi_is_64bit = true,
        .native_macos_min = "macOS 14.1 Sonoma",
        .native_macos_max = "macOS 15 Sequoia (Supported)",
        .silicon_quirks = "Dynamic Caching architecture allocates local memory in hardware in real time; hardware ray-tracing.",
        .is_simulated = true
    },

    /* 31: Mac15,8 (MacBook Pro 16" M3 Max 2023) */
    {
        .profile_id = OV_HW_PROFILE_MAC158_M3_MAX,
        .source = OV_HW_SOURCE_SIMULATED_PROFILE,
        .model_identifier = "Mac15,8",
        .profile_name = "Mac15,8",
        .marketing_name = "MacBook Pro (16-inch, Nov 2023 M3 Max)",
        .description = "Flagship 3nm Apple Silicon with 16-core CPU (12P + 4E), 40-core GPU, Hardware Ray Tracing, and AV1 decode.",
        .cpu = {
            .type = OV_CPU_ARM64_M3,
            .model_name = "Apple M3 Max (12 Everest + 4 Sawtooth)",
            .cores = 16, .threads = 16,
            .base_freq_mhz = 4050, .max_freq_mhz = 4050,
            .is_64bit = true,
            .has_neon = true
        },
        .gpu = {
            .type = OV_GPU_APPLE_SILICON,
            .arch_gen = OV_GPU_ARCH_APPLE_SILICON_M3,
            .model_name = "Apple M3 Max GPU (40 Cores, Dynamic Caching + RT)",
            .vendor_id = 0x106B, .device_id = 0x0024,
            .vram_mb = 131072, .vram_bytes = 131072ULL * 1024 * 1024,
            .vram_type = "Unified LPDDR5-6400 (400 GB/s)",
            .eu_count = 640, .max_texture_dimension = 16384,
            .metal_level = OV_METAL_3
        },
        .mem = { .total_bytes = 128ULL * 1024 * 1024 * 1024, .memory_type = "Unified LPDDR5-6400 (400 GB/s)", .channels = 8 },
        .display_info = "16.2-inch Liquid Retina XDR 120Hz (3456x2234)",
        .storage_interface = "Apple Fabric NAND",
        .firmware_type = "Apple iBoot",
        .efi_is_64bit = true,
        .native_macos_min = "macOS 14.1 Sonoma",
        .native_macos_max = "macOS 15 Sequoia (Supported)",
        .silicon_quirks = "Apple Intelligence neural acceleration native; Hardware AV1 decode engine.",
        .is_simulated = true
    }
};

static const uint32_t g_mac_profile_count = sizeof(g_mac_profiles) / sizeof(g_mac_profiles[0]);

/* ========================================================================= */
/* Subsystem Management                                                      */
/* ========================================================================= */

ov_status_t ov_hardware_init(void) {
#if defined(__linux__)
    if (access("/sys/bus/pci", F_OK) == 0 || access("/proc/bus/pci", F_OK) == 0) {
        pci_access = pci_alloc();
        if (pci_access) {
            pci_init(pci_access);
        }
    } else {
        pci_access = NULL;
    }
#endif

    /* Detect host hardware automatically on initialization */
    ov_hardware_detect_host(&host_profile);

    /* Default active profile is MacBookPro9,1 (Ivy Bridge + GT 650M) */
    ov_hardware_set_active_profile(OV_HW_PROFILE_MBP91_IVY_BRIDGE);

    ov_log_info("Hardware subsystem initialized (Profile: %s)", active_profile.model_identifier);
    return OV_SUCCESS;
}

void ov_hardware_cleanup(void) {
#if defined(__linux__)
    if (pci_access) {
        pci_cleanup(pci_access);
        pci_access = NULL;
    }
#endif
    ov_log_info("Hardware subsystem cleaned up");
}

uint32_t ov_hardware_get_profile_count(void) {
    return g_mac_profile_count;
}

ov_status_t ov_hardware_get_profile(ov_hw_profile_id_t profile_id, ov_hardware_profile_t *out_profile) {
    if (!out_profile) return OV_ERROR_INVALID_PARAM;

    if (profile_id == OV_HW_PROFILE_HOST) {
        if (!host_detected) {
            ov_hardware_detect_host(&host_profile);
        }
        *out_profile = host_profile;
        return OV_SUCCESS;
    }

    if (profile_id == OV_HW_PROFILE_CUSTOM) {
        *out_profile = custom_profile;
        return OV_SUCCESS;
    }

    for (uint32_t i = 0; i < g_mac_profile_count; i++) {
        if (g_mac_profiles[i].profile_id == profile_id) {
            *out_profile = g_mac_profiles[i];
            return OV_SUCCESS;
        }
    }

    return OV_ERROR_NOT_FOUND;
}

static bool string_contains_nocase(const char *haystack, const char *needle) {
    if (!haystack || !needle) return false;
    char h_lower[256] = {0};
    char n_lower[256] = {0};
    size_t hl = strlen(haystack) < 255 ? strlen(haystack) : 255;
    size_t nl = strlen(needle) < 255 ? strlen(needle) : 255;
    for (size_t i = 0; i < hl; i++) h_lower[i] = (char)tolower((unsigned char)haystack[i]);
    for (size_t i = 0; i < nl; i++) n_lower[i] = (char)tolower((unsigned char)needle[i]);
    return strstr(h_lower, n_lower) != NULL;
}

ov_status_t ov_hardware_find_profile_by_name(const char *name, ov_hardware_profile_t *out_profile) {
    if (!name || !out_profile) return OV_ERROR_INVALID_PARAM;

    /* Check host profile */
    if (string_contains_nocase(name, "host") || string_contains_nocase(name, "detect")) {
        return ov_hardware_get_profile(OV_HW_PROFILE_HOST, out_profile);
    }

    /* Check custom profile */
    if (string_contains_nocase(name, "custom")) {
        return ov_hardware_get_profile(OV_HW_PROFILE_CUSTOM, out_profile);
    }

    char target[128];
    strncpy(target, name, sizeof(target) - 1);
    target[sizeof(target) - 1] = '\0';

    if (!strncasecmp(name, "MBP", 3)) {
        snprintf(target, sizeof(target), "MacBookPro%s", name + 3);
    } else if (!strncasecmp(name, "MBA", 3)) {
        snprintf(target, sizeof(target), "MacBookAir%s", name + 3);
    } else if (!strncasecmp(name, "MP", 2)) {
        snprintf(target, sizeof(target), "MacPro%s", name + 2);
    } else if (!strncasecmp(name, "MM", 2)) {
        snprintf(target, sizeof(target), "Macmini%s", name + 2);
    }

    /* Exact model match check */
    for (uint32_t i = 0; i < g_mac_profile_count; i++) {
        if (!strcasecmp(g_mac_profiles[i].model_identifier, target) ||
            !strcasecmp(g_mac_profiles[i].profile_name, target) ||
            !strcasecmp(g_mac_profiles[i].model_identifier, name) ||
            !strcasecmp(g_mac_profiles[i].profile_name, name)) {
            *out_profile = g_mac_profiles[i];
            return OV_SUCCESS;
        }
    }

    /* Substring / marketing name match */
    for (uint32_t i = 0; i < g_mac_profile_count; i++) {
        if (string_contains_nocase(g_mac_profiles[i].model_identifier, target) ||
            string_contains_nocase(g_mac_profiles[i].marketing_name, target) ||
            string_contains_nocase(g_mac_profiles[i].model_identifier, name) ||
            string_contains_nocase(g_mac_profiles[i].marketing_name, name)) {
            *out_profile = g_mac_profiles[i];
            return OV_SUCCESS;
        }
    }

    return OV_ERROR_NOT_FOUND;
}

ov_status_t ov_hardware_set_active_profile(ov_hw_profile_id_t profile_id) {
    ov_hardware_profile_t prof;
    ov_status_t status = ov_hardware_get_profile(profile_id, &prof);
    if (status != OV_SUCCESS) return status;

    active_profile_id = profile_id;
    active_profile = prof;

    /* Initialize GPU topology if not explicitly populated */
    if (active_profile.gpu_topology.gpu_count == 0) {
        if (active_profile.has_discrete_gpu) {
            active_profile.gpu_topology.gpu_count = 2;
            active_profile.gpu_topology.gpus[0] = active_profile.gpu;
            active_profile.gpu_topology.gpus[1] = active_profile.secondary_gpu;
            active_profile.gpu_topology.primary_gpu_index = 0;
            active_profile.gpu_topology.discrete_gpu_index = 1;
            active_profile.gpu_topology.has_integrated_gpu = true;
            active_profile.gpu_topology.has_discrete_gpu = true;
            active_profile.gpu_topology.is_muxed_switchable = active_profile.is_switchable_graphics;
            snprintf(active_profile.gpu_topology.switch_policy, sizeof(active_profile.gpu_topology.switch_policy),
                     "Apple GMUX Hardware Multiplexed / Dynamic Switchable");
        } else {
            active_profile.gpu_topology.gpu_count = 1;
            active_profile.gpu_topology.gpus[0] = active_profile.gpu;
            active_profile.gpu_topology.primary_gpu_index = 0;
            active_profile.gpu_topology.discrete_gpu_index = 0;
            active_profile.gpu_topology.has_integrated_gpu = (active_profile.gpu.vendor_id == 0x8086);
            active_profile.gpu_topology.has_discrete_gpu = false;
        }
    }

    ov_log_info("Active hardware profile set to: %s (%s, %u GPUs)",
                active_profile.model_identifier, active_profile.marketing_name,
                active_profile.gpu_topology.gpu_count);
    return OV_SUCCESS;
}

ov_status_t ov_hardware_set_active_profile_by_name(const char *name) {
    ov_hardware_profile_t prof;
    ov_status_t status = ov_hardware_find_profile_by_name(name, &prof);
    if (status != OV_SUCCESS) return status;

    active_profile_id = prof.profile_id;
    active_profile = prof;
    ov_log_info("Active hardware profile set to: %s (%s)", active_profile.model_identifier, active_profile.marketing_name);
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
    custom_profile.source = OV_HW_SOURCE_SIMULATED_PROFILE;
    custom_profile.is_simulated = true;
    return OV_SUCCESS;
}

const ov_cpu_info_t* ov_hardware_get_cpu(void) {
    return &active_profile.cpu;
}

const ov_gpu_info_t* ov_hardware_get_gpu(void) {
    return &active_profile.gpu;
}

const ov_gpu_info_t* ov_hardware_get_secondary_gpu(void) {
    if (active_profile.has_discrete_gpu) {
        return &active_profile.secondary_gpu;
    }
    return NULL;
}

const ov_memory_info_t* ov_hardware_get_memory(void) {
    return &active_profile.mem;
}

uint32_t ov_hardware_get_gpu_count(void) {
    if (active_profile.gpu_topology.gpu_count > 0) {
        return active_profile.gpu_topology.gpu_count;
    }
    return active_profile.has_discrete_gpu ? 2 : 1;
}

const ov_gpu_info_t* ov_hardware_get_gpu_at(uint32_t index) {
    if (active_profile.gpu_topology.gpu_count > 0 && index < active_profile.gpu_topology.gpu_count) {
        return &active_profile.gpu_topology.gpus[index];
    }
    if (index == 0) return &active_profile.gpu;
    if (index == 1 && active_profile.has_discrete_gpu) return &active_profile.secondary_gpu;
    return &active_profile.gpu;
}

const ov_gpu_topology_t* ov_hardware_get_gpu_topology(void) {
    return &active_profile.gpu_topology;
}
