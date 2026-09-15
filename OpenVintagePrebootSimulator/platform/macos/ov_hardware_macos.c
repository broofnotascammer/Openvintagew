/**
 * OpenVintage Pre-Boot Architecture Simulator - Native macOS Hardware Detection
 * Read-only Darwin/sysctl + IOKit hardware discovery.
 */
#include "ov_macos_native.h"
#include "ov_logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__APPLE__)
#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#endif

int ov_macos_sysctl_string(const char *name, char *buf, size_t buf_len) {
#if defined(__APPLE__)
    if (!buf || buf_len == 0) return -1;
    size_t len = buf_len;
    if (sysctlbyname(name, buf, &len, NULL, 0) == 0) {
        buf[buf_len - 1] = '\0';
        return 0;
    }
#endif
    (void)name; (void)buf; (void)buf_len;
    return -1;
}

int ov_macos_sysctl_uint32(const char *name, uint32_t *out_val) {
#if defined(__APPLE__)
    if (!out_val) return -1;
    size_t len = sizeof(*out_val);
    if (sysctlbyname(name, out_val, &len, NULL, 0) == 0) return 0;
#endif
    (void)name; (void)out_val;
    return -1;
}

int ov_macos_sysctl_uint64(const char *name, uint64_t *out_val) {
#if defined(__APPLE__)
    if (!out_val) return -1;
    size_t len = sizeof(*out_val);
    if (sysctlbyname(name, out_val, &len, NULL, 0) == 0) return 0;
#endif
    (void)name; (void)out_val;
    return -1;
}

ov_status_t ov_macos_detect_os_info(char *os_name, size_t os_len,
                                    char *os_build, size_t build_len,
                                    uint32_t *kernel_major, uint32_t *kernel_minor) {
    if (!os_name || !os_build || !kernel_major || !kernel_minor) return OV_ERROR_INVALID_PARAM;
    *kernel_major = *kernel_minor = 0;
    snprintf(os_name, os_len, "macOS (Unknown)");
    snprintf(os_build, build_len, "Unknown");
#if defined(__APPLE__)
    char ver[64] = {0}, rel[64] = {0}, build[64] = {0};
    ov_macos_sysctl_string("kern.osproductversion", ver, sizeof(ver));
    ov_macos_sysctl_string("kern.osrelease", rel, sizeof(rel));
    ov_macos_sysctl_string("kern.osversion", build, sizeof(build));
    if (build[0]) snprintf(os_build, build_len, "%s", build);
    if (ver[0]) snprintf(os_name, os_len, "macOS %s", ver);
    int maj = 0, min = 0;
    if (sscanf(rel, "%d.%d", &maj, &min) >= 1) {
        *kernel_major = (uint32_t)maj; *kernel_minor = (uint32_t)min;
    }
    if (!ver[0] && maj) snprintf(os_name, os_len, "Darwin %d.%d", maj, min);
    return OV_SUCCESS;
#else
    return OV_ERROR_UNSUPPORTED;
#endif
}

ov_status_t ov_macos_detect_cpu(ov_cpu_info_t *cpu) {
    if (!cpu) return OV_ERROR_INVALID_PARAM;
    memset(cpu, 0, sizeof(*cpu));
#if defined(__APPLE__)
    cpu->type = OV_CPU_UNKNOWN; cpu->is_64bit = true; cpu->cores = 1; cpu->threads = 1;
    char brand[128] = {0}, features[1024] = {0}, leaf7[1024] = {0};
    if (ov_macos_sysctl_string("machdep.cpu.brand_string", brand, sizeof(brand)) == 0)
        snprintf(cpu->model_name, sizeof(cpu->model_name), "%s", brand);
    else snprintf(cpu->model_name, sizeof(cpu->model_name), "Apple Host Processor");
    uint32_t v32 = 0;
    if (ov_macos_sysctl_uint32("hw.physicalcpu", &v32) == 0 && v32) cpu->cores = v32;
    if (ov_macos_sysctl_uint32("hw.logicalcpu", &v32) == 0 && v32) cpu->threads = v32;
    uint64_t freq = 0, c = 0;
    if (ov_macos_sysctl_uint64("hw.cpufrequency", &freq) == 0) cpu->base_freq_mhz = cpu->max_freq_mhz = (uint32_t)(freq / 1000000ULL);
    if (ov_macos_sysctl_uint64("hw.l1dcachesize", &c) == 0) cpu->l1_cache_kb = (uint32_t)(c / 1024ULL);
    if (ov_macos_sysctl_uint64("hw.l2cachesize", &c) == 0) cpu->l2_cache_kb = (uint32_t)(c / 1024ULL);
    if (ov_macos_sysctl_uint64("hw.l3cachesize", &c) == 0) cpu->l3_cache_kb = (uint32_t)(c / 1024ULL);
    ov_macos_sysctl_string("machdep.cpu.features", features, sizeof(features));
    ov_macos_sysctl_string("machdep.cpu.leaf7_features", leaf7, sizeof(leaf7));
    cpu->has_sse = strstr(features,"SSE") != NULL; cpu->has_sse2 = strstr(features,"SSE2") != NULL;
    cpu->has_sse3 = strstr(features,"SSE3") != NULL; cpu->has_ssse3 = strstr(features,"SSSE3") != NULL;
    cpu->has_sse41 = strstr(features,"SSE4.1") != NULL; cpu->has_sse42 = strstr(features,"SSE4.2") != NULL;
    cpu->has_avx = strstr(features,"AVX") != NULL; cpu->has_avx2 = strstr(leaf7,"AVX2") != NULL;
    cpu->has_avx512 = strstr(leaf7,"AVX512") != NULL; cpu->has_fma = strstr(features,"FMA") != NULL;
    cpu->has_aesni = strstr(features,"AES") != NULL; cpu->has_tsc = strstr(features,"TSC") != NULL;
#if defined(__arm64__) || defined(__aarch64__)
    cpu->has_neon = true;
    if (strstr(brand,"M1")) cpu->type = OV_CPU_ARM64_M1;
    else if (strstr(brand,"M2")) cpu->type = OV_CPU_ARM64_M2;
    else if (strstr(brand,"M3")) cpu->type = OV_CPU_ARM64_M3;
    else cpu->type = OV_CPU_ARM64_GENERIC;
#elif defined(__x86_64__)
    if (strstr(brand,"i7-3615QM") || strstr(brand,"Ivy Bridge")) cpu->type = OV_CPU_INTEL_IVY_BRIDGE;
    else if (cpu->has_avx2) cpu->type = OV_CPU_INTEL_HASWELL;
    else if (cpu->has_avx) cpu->type = OV_CPU_INTEL_IVY_BRIDGE;
    else if (cpu->has_sse42) cpu->type = OV_CPU_INTEL_NEHALEM;
    else if (cpu->has_sse41 || cpu->has_sse3) cpu->type = OV_CPU_INTEL_CORE2_DUO;
#endif
    return OV_SUCCESS;
#else
    return OV_ERROR_UNSUPPORTED;
#endif
}

ov_status_t ov_macos_detect_memory(ov_memory_info_t *mem) {
    if (!mem) return OV_ERROR_INVALID_PARAM;
    memset(mem, 0, sizeof(*mem));
#if defined(__APPLE__)
    uint64_t total = 0;
    if (ov_macos_sysctl_uint64("hw.memsize", &total) != 0 || !total) return OV_ERROR_HARDWARE;
    mem->total_bytes = total;
    mach_port_t host = mach_host_self(); vm_size_t page = 4096;
    host_page_size(host, &page);
    vm_statistics64_data_t stat; mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    if (host_statistics64(host, HOST_VM_INFO64, (host_info64_t)&stat, &count) == KERN_SUCCESS) {
        mem->available_bytes = ((uint64_t)stat.free_count + stat.inactive_count) * page;
        mem->reserved_bytes = total > mem->available_bytes ? total - mem->available_bytes : 0;
    }
    mem->channels = 2; mem->frequency_mhz = 1600; mem->has_ecc = false;
    snprintf(mem->memory_type, sizeof(mem->memory_type), "DDR3-1600");
    return OV_SUCCESS;
#else
    return OV_ERROR_UNSUPPORTED;
#endif
}

#if defined(__APPLE__)
static bool ov_cf_u32(CFTypeRef ref, uint32_t *out) {
    if (!ref || !out) return false;
    if (CFGetTypeID(ref) == CFNumberGetTypeID()) {
        return CFNumberGetValue((CFNumberRef)ref, kCFNumberSInt32Type, out);
    }
    if (CFGetTypeID(ref) == CFDataGetTypeID()) {
        CFIndex n = CFDataGetLength((CFDataRef)ref);
        if (n <= 0) return false;
        uint32_t v = 0; if (n >= 4) n = 4;
        CFDataGetBytes((CFDataRef)ref, CFRangeMake(0, n), (UInt8*)&v);
        *out = v; return true;
    }
    return false;
}

static CFTypeRef ov_iokit_prop(io_service_t service, CFStringRef key) {
    CFTypeRef ref = IORegistryEntryCreateCFProperty(service, key, kCFAllocatorDefault, 0);
    if (ref) return ref;
    return IORegistryEntrySearchCFProperty(service, kIOServicePlane, key, kCFAllocatorDefault, kIORegistryIterateParents);
}

static bool ov_read_ids(io_service_t service, uint16_t *vendor, uint16_t *device, uint32_t *class_code) {
    uint32_t v = 0, d = 0, cc = 0; bool got_v = false, got_d = false;
    CFTypeRef r = ov_iokit_prop(service, CFSTR("vendor-id")); if (r) { got_v = ov_cf_u32(r,&v); CFRelease(r); }
    r = ov_iokit_prop(service, CFSTR("device-id")); if (r) { got_d = ov_cf_u32(r,&d); CFRelease(r); }
    r = ov_iokit_prop(service, CFSTR("class-code")); if (r) { ov_cf_u32(r,&cc); CFRelease(r); }
    if (!got_v || !got_d) return false;
    *vendor = (uint16_t)(v & 0xffff); *device = (uint16_t)(d & 0xffff); if (class_code) *class_code = cc & 0x00ffffff;
    return true;
}

static bool ov_known_gpu(uint16_t vendor, uint16_t device) {
    if (vendor == 0x8086) return device == 0x0166 || device == 0x0162 || (device >= 0x0100 && device <= 0x0460);
    if (vendor == 0x10de) return true;
    if (vendor == 0x1002) return true;
    return false;
}

static int ov_find_gpu(const ov_gpu_topology_t *topo, uint16_t vendor, uint16_t device) {
    for (uint32_t i=0; i<topo->gpu_count; ++i)
        if (topo->gpus[i].vendor_id == vendor && topo->gpus[i].device_id == device) return (int)i;
    return -1;
}

static void ov_fill_gpu(ov_gpu_info_t *gpu, io_service_t service, uint16_t vendor, uint16_t device) {
    memset(gpu, 0, sizeof(*gpu)); gpu->vendor_id = vendor; gpu->device_id = device; gpu->has_graphics = true;
    CFTypeRef r = ov_iokit_prop(service, CFSTR("model"));
    if (r && CFGetTypeID(r) == CFDataGetTypeID()) {
        CFIndex n = CFDataGetLength((CFDataRef)r); if (n > 0 && n < (CFIndex)sizeof(gpu->model_name)) { CFDataGetBytes((CFDataRef)r,CFRangeMake(0,n),(UInt8*)gpu->model_name); gpu->model_name[n]='\0'; }
    }
    if (r) CFRelease(r);
    uint32_t mb=0; bool vram=false;
    r=ov_iokit_prop(service,CFSTR("VRAM,totalMB")); if(r){vram=ov_cf_u32(r,&mb);CFRelease(r);}
    if(!vram){ uint32_t bytes32=0; r=ov_iokit_prop(service,CFSTR("VRAM,totalsize")); if(r){ if(ov_cf_u32(r,&bytes32)&&bytes32) { mb=bytes32/(1024*1024); vram=mb>0; } CFRelease(r); } }
    if(vram){gpu->vram_mb=mb;gpu->vram_bytes=(uint64_t)mb*1024*1024;gpu->vram_is_detected=true;snprintf(gpu->vram_description,sizeof(gpu->vram_description),"%u MB (IOKit)",mb);}
    else snprintf(gpu->vram_description,sizeof(gpu->vram_description),"UNKNOWN (not exposed by IOKit)");
    if(vendor==0x8086){
        if(device==0x0166||device==0x0162){gpu->type=OV_GPU_INTEL_GEN7_HD4000;gpu->arch_gen=OV_GPU_ARCH_INTEL_GEN7;if(!gpu->model_name[0])snprintf(gpu->model_name,sizeof(gpu->model_name),"Intel HD Graphics 4000 (Integrated)");gpu->supports_metal=true;gpu->metal_level=OV_METAL_1;}
        else {gpu->type=OV_GPU_UNKNOWN;gpu->arch_gen=OV_GPU_ARCH_UNKNOWN;if(!gpu->model_name[0])snprintf(gpu->model_name,sizeof(gpu->model_name),"Intel Display Controller (0x%04x)",device);}
        gpu->supports_opengl_core=true;gpu->opengl_major=4;gpu->opengl_minor=1;gpu->max_texture_dimension=8192;gpu->supports_vulkan=gpu->supports_metal;
        snprintf(gpu->metal_source,sizeof(gpu->metal_source),"Native macOS hardware capability classification");
        snprintf(gpu->opengl_source,sizeof(gpu->opengl_source),"Native macOS OpenGL capability");
    } else if(vendor==0x10de){
        gpu->type=OV_GPU_NVIDIA_GEFORCE;gpu->arch_gen=(device==0x0fd5)?OV_GPU_ARCH_NVIDIA_KEPLER:OV_GPU_ARCH_UNKNOWN;
        if(device==0x0fd5&&!gpu->model_name[0])snprintf(gpu->model_name,sizeof(gpu->model_name),"NVIDIA GeForce GT 650M (GK107 Kepler)");
        else if(!gpu->model_name[0])snprintf(gpu->model_name,sizeof(gpu->model_name),"NVIDIA Display Controller (0x%04x)",device);
        gpu->supports_metal=true;gpu->metal_level=OV_METAL_2;gpu->supports_opengl_core=true;gpu->opengl_major=4;gpu->opengl_minor=1;gpu->supports_vulkan=true;gpu->max_texture_dimension=16384;
        snprintf(gpu->metal_source,sizeof(gpu->metal_source),"Native macOS NVIDIA hardware classification");
        snprintf(gpu->opengl_source,sizeof(gpu->opengl_source),"Native macOS OpenGL capability");
        snprintf(gpu->vulkan_source,sizeof(gpu->vulkan_source),"MoltenVK translation path; runtime availability must be verified");
    } else if(vendor==0x1002){
        gpu->type=OV_GPU_AMD_RADEON;gpu->supports_metal=true;gpu->metal_level=OV_METAL_2;gpu->supports_opengl_core=true;gpu->opengl_major=4;gpu->opengl_minor=1;gpu->supports_vulkan=true;gpu->max_texture_dimension=16384;
    } else gpu->type=OV_GPU_UNKNOWN;
}

static void ov_mark_active_gpu(ov_gpu_topology_t *topo) {
    CFMutableDictionaryRef matching=IOServiceMatching("IOFramebuffer"); if(!matching) return;
    io_iterator_t it=IO_OBJECT_NULL; if(IOServiceGetMatchingServices(kIOMasterPortDefault,matching,&it)!=KERN_SUCCESS)return;
    io_service_t fb; while((fb=IOIteratorNext(it))){uint16_t v=0,d=0;uint32_t cc=0;if(ov_read_ids(fb,&v,&d,&cc)){int idx=ov_find_gpu(topo,v,d);if(idx>=0){topo->primary_gpu_index=(uint32_t)idx;IOObjectRelease(fb);break;}}IOObjectRelease(fb);}IOObjectRelease(it);
}
#endif

ov_status_t ov_macos_iokit_probe_gpus(ov_gpu_topology_t *out_topo) {
    if (!out_topo) return OV_ERROR_INVALID_PARAM;
    memset(out_topo, 0, sizeof(*out_topo));
#if defined(__APPLE__)
    CFMutableDictionaryRef matching=IOServiceMatching("IOPCIDevice"); if(!matching)return OV_ERROR_HARDWARE;
    io_iterator_t it=IO_OBJECT_NULL; if(IOServiceGetMatchingServices(kIOMasterPortDefault,matching,&it)!=KERN_SUCCESS)return OV_ERROR_HARDWARE;
    io_service_t s;
    while((s=IOIteratorNext(it)) && out_topo->gpu_count<OV_MAX_GPUS){
        uint16_t v=0,d=0;uint32_t cc=0;
        if(ov_read_ids(s,&v,&d,&cc) && (((cc>>16)&0xff)==0x03 || ov_known_gpu(v,d)) && ov_find_gpu(out_topo,v,d)<0){
            ov_fill_gpu(&out_topo->gpus[out_topo->gpu_count],s,v,d);out_topo->gpu_count++;
        }
        IOObjectRelease(s);
    }
    IOObjectRelease(it);
    /* Some switchable Macs expose the powered GPU through an accelerator registry node
       even when its PCI node is not fully published. Search accelerators as a second path. */
    if(out_topo->gpu_count<OV_MAX_GPUS){
        matching=IOServiceMatching("IOAccelerator"); if(matching && IOServiceGetMatchingServices(kIOMasterPortDefault,matching,&it)==KERN_SUCCESS){
            while((s=IOIteratorNext(it)) && out_topo->gpu_count<OV_MAX_GPUS){uint16_t v=0,d=0;uint32_t cc=0;if(ov_read_ids(s,&v,&d,&cc)&&ov_known_gpu(v,d)&&ov_find_gpu(out_topo,v,d)<0){ov_fill_gpu(&out_topo->gpus[out_topo->gpu_count],s,v,d);out_topo->gpu_count++;}IOObjectRelease(s);}IOObjectRelease(it);
        }
    }
    for(uint32_t i=0;i<out_topo->gpu_count;i++){
        if(out_topo->gpus[i].vendor_id==0x8086)out_topo->has_integrated_gpu=true;
        if(out_topo->gpus[i].vendor_id==0x10de||out_topo->gpus[i].vendor_id==0x1002){out_topo->has_discrete_gpu=true;out_topo->discrete_gpu_index=i;}
    }
    if(out_topo->has_integrated_gpu&&out_topo->has_discrete_gpu){out_topo->is_muxed_switchable=true;snprintf(out_topo->switch_policy,sizeof(out_topo->switch_policy),"Apple GMUX / switchable graphics");}
    ov_mark_active_gpu(out_topo);
    return out_topo->gpu_count?OV_SUCCESS:OV_ERROR_HARDWARE;
#else
    return OV_ERROR_UNSUPPORTED;
#endif
}

ov_status_t ov_macos_detect_host(ov_hardware_profile_t *out_host) {
    if (!out_host) {
        return OV_ERROR_INVALID_PARAM;
    }
    memset(out_host, 0, sizeof(*out_host));
#if defined(__APPLE__)
    out_host->profile_id=OV_HW_PROFILE_HOST;out_host->source=OV_HW_SOURCE_NATIVE;out_host->is_simulated=false;out_host->is_mac_host=true;
    char model[64]={0}; if(ov_macos_sysctl_string("hw.model",model,sizeof(model))==0&&model[0]){snprintf(out_host->model_identifier,sizeof(out_host->model_identifier),"%s",model);snprintf(out_host->profile_name,sizeof(out_host->profile_name),"%s",model);snprintf(out_host->marketing_name,sizeof(out_host->marketing_name),"Apple %s",model);}else snprintf(out_host->model_identifier,sizeof(out_host->model_identifier),"Mac-Host");
    if(ov_macos_detect_cpu(&out_host->cpu)!=OV_SUCCESS||!out_host->cpu.cores)return OV_ERROR_HARDWARE;
    if(ov_macos_detect_memory(&out_host->mem)!=OV_SUCCESS||!out_host->mem.total_bytes)return OV_ERROR_HARDWARE;
    ov_gpu_topology_t topo; if(ov_macos_iokit_probe_gpus(&topo)!=OV_SUCCESS)return OV_ERROR_HARDWARE;
    out_host->gpu_topology=topo;out_host->gpu=topo.gpus[topo.primary_gpu_index];out_host->has_discrete_gpu=topo.has_discrete_gpu;out_host->is_switchable_graphics=topo.is_muxed_switchable;if(topo.has_discrete_gpu)out_host->secondary_gpu=topo.gpus[topo.discrete_gpu_index];
    snprintf(out_host->firmware_type,sizeof(out_host->firmware_type),"Apple EFI 64-bit");out_host->efi_is_64bit=true;out_host->apfs_supported_in_firmware=true;
    uint32_t km=0,kn=0;char os[64]={0},build[64]={0};ov_macos_detect_os_info(os,sizeof(os),build,sizeof(build),&km,&kn);snprintf(out_host->description,sizeof(out_host->description),"%s (%s, Build %s)",out_host->model_identifier,os,build);
    return OV_SUCCESS;
#else
    return OV_ERROR_UNSUPPORTED;
#endif
}
