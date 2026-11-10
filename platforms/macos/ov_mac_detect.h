#ifndef OV_MAC_DETECT_H
#define OV_MAC_DETECT_H

#include "../../core/types/ov_types.h"
#include "../../core/resolver/ov_resolver.h"

typedef enum {
    OV_MAC_MODEL_UNKNOWN = 0,
    OV_MAC_MODEL_MBP9_1,       // MacBookPro9,1 (Mid 2012 15" i7-3615QM, HD4000 + GT 650M Kepler)
    OV_MAC_MODEL_MBP11_5,      // MacBookPro11,5 (Mid 2015 15" i7, R9 M370X)
    OV_MAC_MODEL_IMAC14_2,      // iMac14,2 (Late 2013 27" i5/i7, GTX 775M/780M)
    OV_MAC_MODEL_MACPRO5_1,    // MacPro5,1 (Mid 2010/2012 Dual Xeon, PCIe)
    OV_MAC_MODEL_APPLE_SILICON // Modern Apple Silicon (M1/M2/M3)
} ov_mac_model_t;

typedef enum {
    OV_STORAGE_MECHANICAL_HDD = 0,
    OV_STORAGE_SATA_SSD,
    OV_STORAGE_NVME_PCIE
} ov_storage_type_t;

typedef struct {
    ov_mac_model_t model;
    char model_identifier[32];
    char marketing_name[64];
    ov_arch_t cpu_arch;
    char cpu_brand[64];
    uint32_t cpu_cores;
    uint32_t cpu_threads;
    uint32_t cpu_features;
    uint64_t ram_size_mb;

    // GPUs
    char igpu_name[32];
    char dgpu_name[32];
    uint64_t dgpu_vram_mb;

    // Graphics APIs
    bool metal_supported;
    uint32_t metal_feature_family;
    bool opengl_supported;
    char opengl_version[16];
    bool moltenvk_supported;

    // OS & Storage
    char os_version[32];
    ov_storage_type_t primary_storage;
    bool is_legacy_hardware;
} ov_mac_profile_t;

void ov_mac_detect_profile(ov_mac_profile_t *out_profile);
void ov_mac_populate_host_env(const ov_mac_profile_t *profile, ov_host_env_t *out_host);
const char *ov_mac_model_name(ov_mac_model_t model);

#endif /* OV_MAC_DETECT_H */
