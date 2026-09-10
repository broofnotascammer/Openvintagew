/**
 * OpenVintage Pre-Boot Architecture Simulator - Native macOS Hardware Detection
 * Uses Darwin sysctl, Mach host APIs, IOKit registry, and Apple frameworks.
 * Targets macOS Catalina (10.15) through modern macOS (Big Sur, Monterey, Ventura, Sonoma, Sequoia).
 */

#include "ov_macos_native.h"
#include "ov_logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined(__APPLE__)
#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <mach/host_info.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <dlfcn.h>
#endif

int ov_macos_sysctl_string(const char *name, char *buf, size_t buf_len) {
#if defined(__APPLE__)
    size_t len = buf_len;
    if (sysctlbyname(name, buf, &len, NULL, 0) == 0) {
        if (len < buf_len) buf[len] = '\0';
        else buf[buf_len - 1] = '\0';
        return 0;
    }
    return -1;
#else
    (void)name; (void)buf; (void)buf_len;
    return -1;
#endif
}

int ov_macos_sysctl_uint32(const char *name, uint32_t *out_val) {
#if defined(__APPLE__)
    uint32_t val = 0;
    size_t len = sizeof(val);
    if (sysctlbyname(name, &val, &len, NULL, 0) == 0) {
        *out_val = val;
        return 0;
    }
    return -1;
#else
    (void)name; (void)out_val;
    return -1;
#endif
}

int ov_macos_sysctl_uint64(const char *name, uint64_t *out_val) {
#if defined(__APPLE__)
    uint64_t val = 0;
    size_t len = sizeof(val);
    if (sysctlbyname(name, &val, &len, NULL, 0) == 0) {
        *out_val = val;
        return 0;
    }
    return -1;
#else
    (void)name; (void)out_val;
    return -1;
#endif
}

ov_status_t ov_macos_detect_os_info(char *os_name, size_t os_len,
                                    char *os_build, size_t build_len,
                                    uint32_t *kernel_major, uint32_t *kernel_minor) {
    if (!os_name || !os_build || !kernel_major || !kernel_minor) {
        return OV_ERROR_INVALID_PARAM;
    }

    *kernel_major = 0;
    *kernel_minor = 0;
    snprintf(os_name, os_len, "macOS (Unknown)");
    snprintf(os_build, build_len, "Unknown");

#if defined(__APPLE__)
    char os_ver[64] = {0};
    char os_release[64] = {0};
    char build[64] = {0};

    ov_macos_sysctl_string("kern.osproductversion", os_ver, sizeof(os_ver));
    ov_macos_sysctl_string("kern.osrelease", os_release, sizeof(os_release));
    ov_macos_sysctl_string("kern.osversion", build, sizeof(build));

    if (os_release[0] != '\0') {
        int maj = 0, min = 0;
        if (sscanf(os_release, "%d.%d", &maj, &min) >= 1) {
            *kernel_major = (uint32_t)maj;
            *kernel_minor = (uint32_t)min;
        }
    }

    if (build[0] != '\0') {
        snprintf(os_build, build_len, "%s", build);
    }

    if (os_ver[0] != '\0') {
        snprintf(os_name, os_len, "macOS %s", os_ver);
    } else if (*kernel_major > 0) {
        /* Darwin to macOS mapping: Darwin 19 = 10.15 Catalina, Darwin 20 = 11 Big Sur */
        if (*kernel_major == 19) snprintf(os_name, os_len, "macOS 10.15 Catalina (Darwin %u.%u)", *kernel_major, *kernel_minor);
        else if (*kernel_major == 20) snprintf(os_name, os_len, "macOS 11 Big Sur (Darwin %u.%u)", *kernel_major, *kernel_minor);
        else if (*kernel_major == 21) snprintf(os_name, os_len, "macOS 12 Monterey (Darwin %u.%u)", *kernel_major, *kernel_minor);
        else if (*kernel_major == 22) snprintf(os_name, os_len, "macOS 13 Ventura (Darwin %u.%u)", *kernel_major, *kernel_minor);
        else if (*kernel_major == 23) snprintf(os_name, os_len, "macOS 14 Sonoma (Darwin %u.%u)", *kernel_major, *kernel_minor);
        else if (*kernel_major >= 24) snprintf(os_name, os_len, "macOS 15 Sequoia (Darwin %u.%u)", *kernel_major, *kernel_minor);
        else snprintf(os_name, os_len, "Darwin %u.%u", *kernel_major, *kernel_minor);
    }

    return OV_SUCCESS;
#else
    return OV_ERROR_UNSUPPORTED;
#endif
}

ov_status_t ov_macos_detect_cpu(ov_cpu_info_t *cpu) {
    if (!cpu) return OV_ERROR_INVALID_PARAM;
    memset(cpu, 0, sizeof(ov_cpu_info_t));

#if defined(__APPLE__)
    cpu->type = OV_CPU_UNKNOWN;
    cpu->is_64bit = true;
    cpu->cores = 1;
    cpu->threads = 1;

    /* Read Brand String */
    char brand[128] = {0};
    if (ov_macos_sysctl_string("machdep.cpu.brand_string", brand, sizeof(brand)) == 0) {
        snprintf(cpu->model_name, sizeof(cpu->model_name), "%s", brand);
    } else {
        snprintf(cpu->model_name, sizeof(cpu->model_name), "Apple Host Processor");
    }

    /* Read Core / Thread Topology directly from Kernel sysctl */
    uint32_t phys_cores = 0;
    if (ov_macos_sysctl_uint32("hw.physicalcpu", &phys_cores) == 0 && phys_cores > 0) {
        cpu->cores = phys_cores;
    }

    uint32_t logical_threads = 0;
    if (ov_macos_sysctl_uint32("hw.logicalcpu", &logical_threads) == 0 && logical_threads > 0) {
        cpu->threads = logical_threads;
    }

    /* Read CPU Frequencies */
    uint64_t freq_hz = 0;
    if (ov_macos_sysctl_uint64("hw.cpufrequency", &freq_hz) == 0 && freq_hz > 0) {
        cpu->base_freq_mhz = (uint32_t)(freq_hz / 1000000ULL);
        cpu->max_freq_mhz = (uint32_t)(freq_hz / 1000000ULL);
    } else {
        cpu->base_freq_mhz = 2400;
        cpu->max_freq_mhz = 3200;
    }

    /* Cache Lines */
    uint64_t l1_bytes = 0, l2_bytes = 0, l3_bytes = 0;
    if (ov_macos_sysctl_uint64("hw.l1dcachesize", &l1_bytes) == 0 && l1_bytes > 0) {
        cpu->l1_cache_kb = (uint32_t)(l1_bytes / 1024ULL);
    } else {
        cpu->l1_cache_kb = 32;
    }
    if (ov_macos_sysctl_uint64("hw.l2cachesize", &l2_bytes) == 0 && l2_bytes > 0) {
        cpu->l2_cache_kb = (uint32_t)(l2_bytes / 1024ULL);
    } else {
        cpu->l2_cache_kb = 256;
    }
    if (ov_macos_sysctl_uint64("hw.l3cachesize", &l3_bytes) == 0 && l3_bytes > 0) {
        cpu->l3_cache_kb = (uint32_t)(l3_bytes / 1024ULL);
    } else {
        cpu->l3_cache_kb = 6144;
    }

    /* Query CPU Feature Strings */
    char features[1024] = {0};
    ov_macos_sysctl_string("machdep.cpu.features", features, sizeof(features));

    char leaf7[1024] = {0};
    ov_macos_sysctl_string("machdep.cpu.leaf7_features", leaf7, sizeof(leaf7));

    if (strstr(features, "SSE") != NULL)    cpu->has_sse = true;
    if (strstr(features, "SSE2") != NULL)   cpu->has_sse2 = true;
    if (strstr(features, "SSE3") != NULL)   cpu->has_sse3 = true;
    if (strstr(features, "SSSE3") != NULL)  cpu->has_ssse3 = true;
    if (strstr(features, "SSE4.1") != NULL) cpu->has_sse41 = true;
    if (strstr(features, "SSE4.2") != NULL) cpu->has_sse42 = true;
    if (strstr(features, "AVX1.0") != NULL || strstr(features, "AVX ") != NULL) cpu->has_avx = true;
    if (strstr(leaf7, "AVX2") != NULL)      cpu->has_avx2 = true;
    if (strstr(leaf7, "AVX512") != NULL)    cpu->has_avx512 = true;
    if (strstr(features, "FMA") != NULL)    cpu->has_fma = true;
    if (strstr(features, "AES") != NULL)    cpu->has_aesni = true;
    if (strstr(features, "TSC") != NULL)    cpu->has_tsc = true;

#if defined(__arm64__) || defined(__aarch64__)
    cpu->has_neon = true;
    if (strstr(cpu->model_name, "M1") != NULL) cpu->type = OV_CPU_ARM64_M1;
    else if (strstr(cpu->model_name, "M2") != NULL) cpu->type = OV_CPU_ARM64_M2;
    else if (strstr(cpu->model_name, "M3") != NULL) cpu->type = OV_CPU_ARM64_M3;
    else cpu->type = OV_CPU_ARM64_GENERIC;
#elif defined(__x86_64__)
    if (strstr(cpu->model_name, "i7-3615QM") != NULL || strstr(cpu->model_name, "Ivy Bridge") != NULL) {
        cpu->type = OV_CPU_INTEL_IVY_BRIDGE;
    } else if (cpu->has_avx2) {
        cpu->type = OV_CPU_INTEL_HASWELL;
    } else if (cpu->has_avx) {
        cpu->type = OV_CPU_INTEL_IVY_BRIDGE;
    } else {
        cpu->type = OV_CPU_INTEL_CORE2_DUO;
    }
#endif

    return OV_SUCCESS;
#else
    return OV_ERROR_UNSUPPORTED;
#endif
}

ov_status_t ov_macos_detect_memory(ov_memory_info_t *mem) {
    if (!mem) return OV_ERROR_INVALID_PARAM;
    memset(mem, 0, sizeof(ov_memory_info_t));

#if defined(__APPLE__)
    uint64_t memsize = 0;
    if (ov_macos_sysctl_uint64("hw.memsize", &memsize) == 0 && memsize > 0) {
        mem->total_bytes = memsize;
    } else {
        mem->total_bytes = 8ULL * 1024 * 1024 * 1024;
    }

    /* Mach Host VM Statistics for Available / Free RAM */
    mach_port_t host_port = mach_host_self();
    vm_size_t page_size = 4096;
    host_page_size(host_port, &page_size);

    vm_statistics64_data_t vm_stat;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    if (host_statistics64(host_port, HOST_VM_INFO64, (host_info64_t)&vm_stat, &count) == KERN_SUCCESS) {
        uint64_t free_bytes = ((uint64_t)vm_stat.free_count + (uint64_t)vm_stat.inactive_count) * (uint64_t)page_size;
        mem->available_bytes = free_bytes;
        mem->reserved_bytes = mem->total_bytes > free_bytes ? (mem->total_bytes - free_bytes) : 0;
    } else {
        mem->available_bytes = mem->total_bytes / 2;
        mem->reserved_bytes = mem->total_bytes / 2;
    }

    mem->channels = 2;
    mem->frequency_mhz = 1600;
    snprintf(mem->memory_type, sizeof(mem->memory_type), "Apple Unified / DDR Memory");
    mem->has_ecc = false;

    return OV_SUCCESS;
#else
    return OV_ERROR_UNSUPPORTED;
#endif
}

ov_status_t ov_macos_iokit_probe_gpus(ov_gpu_topology_t *out_topo) {
    if (!out_topo) return OV_ERROR_INVALID_PARAM;
    memset(out_topo, 0, sizeof(ov_gpu_topology_t));

#if defined(__APPLE__)
    CFMutableDictionaryRef matching = IOServiceMatching("IOPCIDevice");
    if (!matching) return OV_ERROR_HARDWARE;

    io_iterator_t iterator;
    kern_return_t kr = IOServiceGetMatchingServices(kIOMasterPortDefault, matching, &iterator);
    if (kr != KERN_SUCCESS) return OV_ERROR_HARDWARE;

    io_service_t service;
    uint32_t found = 0;

    while ((service = IOIteratorNext(iterator)) && found < OV_MAX_GPUS) {
        CFTypeRef class_code_ref = IORegistryEntryCreateCFProperty(service, CFSTR("class-code"), kCFAllocatorDefault, 0);
        if (class_code_ref) {
            uint32_t class_code = 0;
            if (CFGetTypeID(class_code_ref) == CFDataGetTypeID()) {
                CFDataGetBytes((CFDataRef)class_code_ref, CFRangeMake(0, sizeof(uint32_t)), (UInt8*)&class_code);
            }
            CFRelease(class_code_ref);

            /* Check Display Controller Class 0x030000 or 3D 0x030200 */
            uint32_t base_class = (class_code >> 16) & 0xFF;
            uint32_t sub_class = (class_code >> 8) & 0xFF;

            if (base_class == 0x03) {
                ov_gpu_info_t *gpu = &out_topo->gpus[found];
                memset(gpu, 0, sizeof(ov_gpu_info_t));

                /* Read Vendor and Device ID */
                CFTypeRef vendor_ref = IORegistryEntryCreateCFProperty(service, CFSTR("vendor-id"), kCFAllocatorDefault, 0);
                if (vendor_ref && CFGetTypeID(vendor_ref) == CFDataGetTypeID()) {
                    uint32_t vid = 0;
                    CFDataGetBytes((CFDataRef)vendor_ref, CFRangeMake(0, sizeof(uint32_t)), (UInt8*)&vid);
                    gpu->vendor_id = (uint16_t)vid;
                    CFRelease(vendor_ref);
                }

                CFTypeRef dev_ref = IORegistryEntryCreateCFProperty(service, CFSTR("device-id"), kCFAllocatorDefault, 0);
                if (dev_ref && CFGetTypeID(dev_ref) == CFDataGetTypeID()) {
                    uint32_t did = 0;
                    CFDataGetBytes((CFDataRef)dev_ref, CFRangeMake(0, sizeof(uint32_t)), (UInt8*)&did);
                    gpu->device_id = (uint16_t)did;
                    CFRelease(dev_ref);
                }

                /* Model Name from IORegistry */
                CFTypeRef model_ref = IORegistryEntryCreateCFProperty(service, CFSTR("model"), kCFAllocatorDefault, 0);
                if (model_ref && CFGetTypeID(model_ref) == CFDataGetTypeID()) {
                    CFIndex len = CFDataGetLength((CFDataRef)model_ref);
                    if (len > 0 && len < (CFIndex)sizeof(gpu->model_name)) {
                        CFDataGetBytes((CFDataRef)model_ref, CFRangeMake(0, len), (UInt8*)gpu->model_name);
                        gpu->model_name[len] = '\0';
                    }
                    CFRelease(model_ref);
                }

                /* VRAM Size */
                CFTypeRef vram_ref = IORegistryEntryCreateCFProperty(service, CFSTR("VRAM,totalMB"), kCFAllocatorDefault, 0);
                if (vram_ref && CFGetTypeID(vram_ref) == CFDataGetTypeID()) {
                    uint32_t mb = 0;
                    CFDataGetBytes((CFDataRef)vram_ref, CFRangeMake(0, sizeof(uint32_t)), (UInt8*)&mb);
                    gpu->vram_mb = mb;
                    gpu->vram_bytes = (uint64_t)mb * 1024ULL * 1024ULL;
                    CFRelease(vram_ref);
                }

                /* Categorize Architecture & Integrated vs Discrete */
                if (gpu->vendor_id == 0x8086) {
                    /* Intel Integrated */
                    gpu->type = OV_GPU_INTEL_GEN7_HD4000;
                    gpu->arch_gen = OV_GPU_ARCH_INTEL_GEN7;
                    if (gpu->model_name[0] == '\0') snprintf(gpu->model_name, sizeof(gpu->model_name), "Intel HD Graphics 4000 (Integrated)");
                    gpu->supports_metal = true;
                    gpu->metal_level = OV_METAL_1;
                    gpu->supports_opengl_core = true;
                    gpu->opengl_major = 4;
                    gpu->opengl_minor = 1;
                    gpu->supports_compute = true;
                    gpu->max_texture_dimension = 8192;
                    out_topo->has_integrated_gpu = true;
                } else if (gpu->vendor_id == 0x10DE) {
                    /* NVIDIA Discrete */
                    gpu->type = OV_GPU_NVIDIA_GEFORCE;
                    gpu->arch_gen = OV_GPU_ARCH_NVIDIA_KEPLER;
                    if (gpu->model_name[0] == '\0') snprintf(gpu->model_name, sizeof(gpu->model_name), "NVIDIA GeForce GT 650M (Discrete GK107)");
                    gpu->supports_metal = true;
                    gpu->metal_level = OV_METAL_2;
                    gpu->supports_vulkan = true;
                    gpu->supports_opengl_core = true;
                    gpu->opengl_major = 4;
                    gpu->opengl_minor = 1;
                    gpu->supports_compute = true;
                    gpu->max_texture_dimension = 16384;
                    out_topo->has_discrete_gpu = true;
                    out_topo->discrete_gpu_index = found;
                } else if (gpu->vendor_id == 0x1002) {
                    /* AMD Radeon Discrete */
                    gpu->type = OV_GPU_AMD_RADEON;
                    gpu->arch_gen = OV_GPU_ARCH_AMD_GCN_1_4;
                    gpu->supports_metal = true;
                    gpu->metal_level = OV_METAL_2;
                    gpu->supports_opengl_core = true;
                    gpu->opengl_major = 4;
                    gpu->opengl_minor = 1;
                    out_topo->has_discrete_gpu = true;
                    out_topo->discrete_gpu_index = found;
                }

                gpu->has_graphics = true;
                found++;
            }
        }
        IOObjectRelease(service);
    }
    IOObjectRelease(iterator);

    out_topo->gpu_count = found;
    if (out_topo->has_integrated_gpu && out_topo->has_discrete_gpu) {
        out_topo->is_muxed_switchable = true;
        snprintf(out_topo->switch_policy, sizeof(out_topo->switch_policy), "Apple GMUX Hardware Multiplexed / Dynamic Switchable");
    }

    return OV_SUCCESS;
#else
    return OV_ERROR_UNSUPPORTED;
#endif
}

ov_status_t ov_macos_detect_host(ov_hardware_profile_t *out_host) {
    if (!out_host) return OV_ERROR_INVALID_PARAM;
    memset(out_host, 0, sizeof(ov_hardware_profile_t));

#if defined(__APPLE__)
    out_host->source = OV_HW_SOURCE_HOST_DETECTED;
    out_host->is_simulated = false;
    out_host->is_mac_host = true;

    /* Model Identifier (e.g. "MacBookPro9,1") */
    char model[64] = {0};
    if (ov_macos_sysctl_string("hw.model", model, sizeof(model)) == 0) {
        snprintf(out_host->model_identifier, sizeof(out_host->model_identifier), "%s", model);
        snprintf(out_host->profile_name, sizeof(out_host->profile_name), "%s", model);
    } else {
        snprintf(out_host->model_identifier, sizeof(out_host->model_identifier), "Apple Mac Host");
    }

    /* CPU */
    ov_macos_detect_cpu(&out_host->cpu);

    /* Memory */
    ov_macos_detect_memory(&out_host->mem);

    /* GPUs via IOKit */
    ov_gpu_topology_t topo;
    if (ov_macos_iokit_probe_gpus(&topo) == OV_SUCCESS && topo.gpu_count > 0) {
        out_host->gpu_topology = topo;
        out_host->gpu = topo.gpus[topo.primary_gpu_index];
        out_host->has_discrete_gpu = topo.has_discrete_gpu;
        out_host->is_switchable_graphics = topo.is_muxed_switchable;
        if (topo.has_discrete_gpu) {
            out_host->secondary_gpu = topo.gpus[topo.discrete_gpu_index];
        }
    } else {
        /* Fallback Host Virtual GPU */
        out_host->gpu.type = OV_GPU_SOFTWARE_RASTERIZER;
        snprintf(out_host->gpu.model_name, sizeof(out_host->gpu.model_name), "Apple Virtual/Host Display");
        out_host->gpu.supports_metal = true;
        out_host->gpu.metal_level = OV_METAL_1;
    }

    /* Firmware */
    snprintf(out_host->firmware_type, sizeof(out_host->firmware_type), "Apple EFI 64-bit");
    out_host->efi_is_64bit = true;
    out_host->apfs_supported_in_firmware = true;

    /* OS Info */
    uint32_t k_maj = 0, k_min = 0;
    char os_name[64], os_build[64];
    ov_macos_detect_os_info(os_name, sizeof(os_name), os_build, sizeof(os_build), &k_maj, &k_min);
    snprintf(out_host->description, sizeof(out_host->description), "%s (%s, Build %s)",
             out_host->model_identifier, os_name, os_build);

    return OV_SUCCESS;
#else
    return OV_ERROR_UNSUPPORTED;
#endif
}
