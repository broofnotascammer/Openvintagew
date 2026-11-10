#include "ov_mac_detect.h"
#include <string.h>

void ov_mac_detect_profile(ov_mac_profile_t *out_profile) {
    if (!out_profile) return;
    memset(out_profile, 0, sizeof(ov_mac_profile_t));

    // Baseline detection defaults to the primary legacy development target: MacBookPro9,1
    out_profile->model = OV_MAC_MODEL_MBP9_1;
    strncpy(out_profile->model_identifier, "MacBookPro9,1", sizeof(out_profile->model_identifier) - 1);
    strncpy(out_profile->marketing_name, "MacBook Pro (15-inch, Mid 2012)", sizeof(out_profile->marketing_name) - 1);
    out_profile->cpu_arch = OV_ARCH_X86_64;
    strncpy(out_profile->cpu_brand, "Intel(R) Core(TM) i7-3615QM CPU @ 2.30GHz", sizeof(out_profile->cpu_brand) - 1);
    out_profile->cpu_cores = 4;
    out_profile->cpu_threads = 8;
    out_profile->cpu_features = OV_CPU_FEAT_AVX | OV_CPU_FEAT_FMA; // Ivy Bridge supports AVX, FMA, but not AVX2
    out_profile->ram_size_mb = 8192; // 8 GB DDR3

    // Dual GPU configuration
    strncpy(out_profile->igpu_name, "Intel HD Graphics 4000", sizeof(out_profile->igpu_name) - 1);
    strncpy(out_profile->dgpu_name, "NVIDIA GeForce GT 650M", sizeof(out_profile->dgpu_name) - 1);
    out_profile->dgpu_vram_mb = 1024; // 1024 MB GDDR5

    // Graphics APIs: Legacy Metal 1 (GPUFamily1 v4), OpenGL 4.1 Core Profile, MoltenVK via Vulkan-1.1 subset
    out_profile->metal_supported = true;
    out_profile->metal_feature_family = 1;
    out_profile->opengl_supported = true;
    strncpy(out_profile->opengl_version, "4.1 Core Profile", sizeof(out_profile->opengl_version) - 1);
    out_profile->moltenvk_supported = true;

    // Operating System & Storage
    strncpy(out_profile->os_version, "macOS Catalina 10.15.8", sizeof(out_profile->os_version) - 1);
    out_profile->primary_storage = OV_STORAGE_MECHANICAL_HDD;
    out_profile->is_legacy_hardware = true;
}

void ov_mac_populate_host_env(const ov_mac_profile_t *profile, ov_host_env_t *out_host) {
    if (!profile || !out_host) return;
    memset(out_host, 0, sizeof(ov_host_env_t));

    out_host->host_cpu_arch = profile->cpu_arch;
    out_host->host_cpu_features = profile->cpu_features;
    out_host->host_cpu_cores = profile->cpu_cores;
    out_host->host_ram_mb = profile->ram_size_mb;

    out_host->gpu_caps.backend = profile->metal_supported ? OV_GFX_BACKEND_METAL_NATIVE : OV_GFX_BACKEND_OPENGL_CORE;
    // Kepler / HD4000 supports compute, tessellation, BC compression, but NOT mesh shaders or hardware ray tracing
    out_host->gpu_caps.supported_features = OV_GPU_FEAT_COMPUTE | OV_GPU_FEAT_TESSELLATION | OV_GPU_FEAT_BC_COMPRESSION;
    out_host->gpu_caps.partial_features = OV_GPU_FEAT_GEOMETRY_SHADER; // Can be reconstructed via compute shaders
    out_host->gpu_caps.available_vram_mb = profile->dgpu_vram_mb;
    out_host->gpu_caps.has_metal = profile->metal_supported;
    out_host->gpu_caps.has_moltenvk = profile->moltenvk_supported;
    out_host->gpu_caps.has_opengl = profile->opengl_supported;
    out_host->gpu_caps.cpu_fallback_supported = true; // Host CPU can perform software fallback reconstruction
}

const char *ov_mac_model_name(ov_mac_model_t model) {
    switch (model) {
        case OV_MAC_MODEL_MBP9_1: return "MacBookPro9,1 (Mid 2012)";
        case OV_MAC_MODEL_MBP11_5: return "MacBookPro11,5 (Mid 2015)";
        case OV_MAC_MODEL_IMAC14_2: return "iMac14,2 (Late 2013)";
        case OV_MAC_MODEL_MACPRO5_1: return "MacPro5,1 (Mid 2010/2012)";
        case OV_MAC_MODEL_APPLE_SILICON: return "Apple Silicon Mac";
        default: return "Generic / Unknown Mac";
    }
}
