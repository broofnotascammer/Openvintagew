/**
 * OpenVintage Pre-Boot Simulator - Hardware Detection & Simulation Profiles
 * Supports host hardware discovery (safe, read-only) and comprehensive Mac target profiles.
 */

#ifndef OV_HARDWARE_H
#define OV_HARDWARE_H

#include "ov_types.h"

/* Mac Hardware Simulation Profiles */
typedef enum {
    OV_HW_PROFILE_HOST = 0,               /* Actual Host System (read-only detected) */
    OV_HW_PROFILE_MBP11_CORE_DUO = 1,     /* MacBookPro1,1 (Early 2006, 32-bit Yonah, ATI X1600) */
    OV_HW_PROFILE_MBP31_CORE2_DUO = 2,    /* MacBookPro3,1 (Mid 2007, Merom, GeForce 8600M GT) */
    OV_HW_PROFILE_MB51_PENRYN = 3,        /* MacBook5,1 (Late 2008, Penryn, GeForce 9400M) */
    OV_HW_PROFILE_MP11_WOODCREST = 4,     /* MacPro1,1 (2006, Woodcrest Xeon, 32-bit EFI) */
    OV_HW_PROFILE_MP31_HARPERTOWN = 5,    /* MacPro3,1 (2008, Harpertown Xeon, 64-bit EFI) */
    OV_HW_PROFILE_MP41_NEHALEM = 6,       /* MacPro4,1 (2009, Nehalem Xeon, Radeon HD 4870) */
    OV_HW_PROFILE_MP51_WESTMERE = 7,      /* MacPro5,1 (2010/2012, Westmere Xeon, Radeon HD 5770) */
    OV_HW_PROFILE_MBP81_SANDY_BRIDGE = 8, /* MacBookPro8,1 (13" 2011, Core i5-2415M, HD 3000) */
    OV_HW_PROFILE_MBP82_SANDY_BRIDGE = 9, /* MacBookPro8,2 (15" 2011, Core i7, HD 3000 + HD 6750M) */
    OV_HW_PROFILE_MBP91_IVY_BRIDGE = 10,  /* MacBookPro9,1 (15" Mid 2012, i7-3615QM, HD 4000 + GT 650M) */
    OV_HW_PROFILE_MBA52_IVY_BRIDGE = 11,  /* MacBookAir5,2 (13" Mid 2012, i5-3427U, HD 4000) */
    OV_HW_PROFILE_IMAC132_IVY_BRIDGE = 12,/* iMac13,2 (27" Late 2012, i5-3470S, GTX 675MX) */
    OV_HW_PROFILE_MM62_IVY_BRIDGE = 13,   /* Macmini6,2 (Late 2012, i7-3720QM, HD 4000) */
    OV_HW_PROFILE_MP61_IVY_BRIDGE_EP = 14,/* MacPro6,1 (Late 2013, Xeon E5-1650 v2, Dual FirePro D300) */
    OV_HW_PROFILE_MBA62_HASWELL = 15,     /* MacBookAir6,2 (13" 2013-2014, i5-4250U, HD 5000) */
    OV_HW_PROFILE_MBP113_HASWELL = 16,    /* MacBookPro11,3 (15" Late 2013, i7-4850HQ, Iris Pro 5200 + GT 750M) */
    OV_HW_PROFILE_MBP121_BROADWELL = 17,  /* MacBookPro12,1 (13" Early 2015, i5-5257U, Iris 6100) */
    OV_HW_PROFILE_MB91_SKYLAKE = 18,      /* MacBook9,1 (12" Early 2016, Core m3-6Y30, HD 515) */
    OV_HW_PROFILE_MBP133_SKYLAKE = 19,    /* MacBookPro13,3 (15" Late 2016, i7-6700HQ, Iris 550 + Radeon Pro 455) */
    OV_HW_PROFILE_MBP143_KABY_LAKE = 20,  /* MacBookPro14,3 (15" Mid 2017, i7-7700HQ, HD 630 + Radeon Pro 560) */
    OV_HW_PROFILE_MBP161_COFFEE_LAKE = 21,/* MacBookPro16,1 (16" 2019, i9-9880H, UHD 630 + Radeon Pro 5500M) */
    OV_HW_PROFILE_IMAC201_COMET_LAKE = 22,/* iMac20,1 (27" 2020, i7-10700K, Radeon Pro 5500 XT) */
    OV_HW_PROFILE_MP71_CASCADE_LAKE = 23, /* MacPro7,1 (2019, Xeon W-3235, Radeon Pro 580X) */
    OV_HW_PROFILE_MBA101_M1 = 24,         /* MacBookAir10,1 (M1 2020, 8-Core CPU / 8-Core GPU) */
    OV_HW_PROFILE_MBP181_M1_PRO = 25,     /* MacBookPro18,1 (16" 2021 M1 Pro, 10-Core CPU / 16-Core GPU) */
    OV_HW_PROFILE_MAC131_M1_ULTRA = 26,   /* Mac13,1 (Mac Studio 2022 M1 Ultra, 20-Core CPU / 64-Core GPU) */
    OV_HW_PROFILE_MAC142_M2 = 27,         /* Mac14,2 (MacBook Air M2 2022, 8-Core CPU / 10-Core GPU) */
    OV_HW_PROFILE_MAC146_M2_MAX = 28,     /* Mac14,6 (MacBook Pro 16" M2 Max 2023, 12-Core CPU / 38-Core GPU) */
    OV_HW_PROFILE_MAC1414_M2_ULTRA = 29,  /* Mac14,14 (Mac Pro M2 Ultra 2023, 24-Core CPU / 76-Core GPU) */
    OV_HW_PROFILE_MAC153_M3 = 30,         /* Mac15,3 (MacBook Pro 14" M3 2023, 8-Core CPU / 10-Core GPU) */
    OV_HW_PROFILE_MAC158_M3_MAX = 31,     /* Mac15,8 (MacBook Pro 16" M3 Max 2023, 16-Core CPU / 40-Core GPU) */
    OV_HW_PROFILE_CUSTOM = 32,            /* User-defined Custom Hardware Profile */
    OV_HW_PROFILE_COUNT
} ov_hw_profile_id_t;

/* Backward Compatibility Aliases */
#define OV_HW_PROFILE_INTEL_GEN7    OV_HW_PROFILE_MBP91_IVY_BRIDGE
#define OV_HW_PROFILE_INTEL_GEN75   OV_HW_PROFILE_MBP113_HASWELL
#define OV_HW_PROFILE_MODERN_INTEL  OV_HW_PROFILE_MBP161_COFFEE_LAKE
#define OV_HW_PROFILE_AMD_ZEN       OV_HW_PROFILE_HOST
#define OV_HW_PROFILE_ARM64_GUEST   OV_HW_PROFILE_MBA101_M1

/* CPU Hardware State */
typedef struct {
    ov_cpu_type_t   type;
    char            model_name[128];
    uint32_t        cores;
    uint32_t        threads;
    uint32_t        base_freq_mhz;
    uint32_t        max_freq_mhz;
    uint64_t        features_mask;
    bool            is_64bit;
    uint32_t        l1_cache_kb;
    uint32_t        l2_cache_kb;
    uint32_t        l3_cache_kb;
    bool            has_sse;
    bool            has_sse2;
    bool            has_sse3;
    bool            has_ssse3;
    bool            has_sse41;
    bool            has_sse42;
    bool            has_avx;
    bool            has_avx2;
    bool            has_avx512;
    bool            has_fma;
    bool            has_aesni;
    bool            has_tsc;
    bool            has_neon;
} ov_cpu_info_t;

/* GPU Hardware State */
typedef struct {
    ov_gpu_type_t       type;
    ov_gpu_arch_t       arch_gen;
    char                model_name[128];
    uint16_t            vendor_id;
    uint16_t            device_id;
    uint16_t            subvendor_id;
    uint16_t            subdevice_id;
    uint32_t            vram_mb;
    uint64_t            vram_bytes;
    bool                vram_is_detected;       /* True if VRAM value was directly exposed by hardware/IOKit */
    char                vram_description[64];   /* e.g. "512 MB (Dedicated GDDR5)", "UNKNOWN (Dynamic System Allocation / Shared RAM)" */
    char                vram_type[32];          /* e.g. "DDR3", "GDDR5", "Unified LPDDR5" */
    uint32_t            eu_count;               /* Execution Units / Compute Units / ALUs */
    uint32_t            max_texture_dimension;  /* e.g. 2048, 4096, 8192, 16384 */
    bool                has_compute;
    bool                has_graphics;
    bool                supports_compute;
    bool                supports_tessellation;
    ov_metal_support_t  metal_level;
    bool                supports_metal;
    char                metal_source[128];      /* e.g. "Detected via Metal Runtime API", "Derived from Architecture: NVIDIA GK107 Kepler" */
    bool                supports_vulkan;
    char                vulkan_source[128];     /* e.g. "Derived: Requires MoltenVK runtime translation" */
    bool                supports_opengl_core;
    uint32_t            opengl_major;
    uint32_t            opengl_minor;
    char                opengl_source[128];     /* e.g. "Derived from Driver Architecture: OpenGL 4.1 Core Profile" */
    bool                supports_directx;
    char                driver_version[64];
} ov_gpu_info_t;

#define OV_MAX_GPUS 4

/* Multi-GPU Topology Snapshot */
typedef struct {
    uint32_t            gpu_count;
    ov_gpu_info_t       gpus[OV_MAX_GPUS];
    uint32_t            primary_gpu_index;     /* Index 0: typically integrated */
    uint32_t            discrete_gpu_index;    /* Index of discrete GPU if present */
    uint32_t            active_gpu_index;      /* Index of currently active GPU in physical gpus[] inventory */
    bool                has_integrated_gpu;
    bool                has_discrete_gpu;
    bool                is_muxed_switchable;   /* e.g. Apple GMUX on MacBookPro9,1 */
    char                switch_policy[64];     /* e.g. "Apple GMUX Hardware Multiplexed" */
} ov_gpu_topology_t;

/* System Memory State */
typedef struct {
    uint64_t        total_bytes;
    uint64_t        available_bytes;
    uint64_t        reserved_bytes;
    uint32_t        numa_nodes;
    uint32_t        channels;
    uint32_t        frequency_mhz;
    char            memory_type[32];            /* e.g. "DDR3-1600", "LPDDR5-6400" */
    bool            has_ecc;
} ov_memory_info_t;

/* PCI Device Entry */
typedef struct {
    uint16_t        vendor_id;
    uint16_t        device_id;
    uint16_t        subvendor_id;
    uint16_t        subdevice_id;
    uint8_t         bus;
    uint8_t         slot;
    uint8_t         func;
    char            class_name[128];
    char            vendor_name[128];
    char            device_name[256];
} ov_pci_device_t;

/* Complete Hardware Profile Snapshot */
typedef struct {
    ov_hw_profile_id_t  profile_id;
    ov_hw_source_t      source;                 /* HOST_DETECTED, SIMULATED_PROFILE, etc. */
    char                model_identifier[64];   /* e.g. "MacBookPro9,1" */
    char                profile_name[64];       /* e.g. "MacBookPro9,1" */
    char                marketing_name[128];    /* e.g. "MacBook Pro (15-inch, Mid 2012)" */
    char                description[256];

    /* Processor */
    ov_cpu_info_t       cpu;

    /* Primary GPU */
    ov_gpu_info_t       gpu;                    /* Primary GPU (or integrated) */

    /* Secondary GPU (if switchable / discrete graphics present) */
    bool                has_discrete_gpu;
    bool                is_switchable_graphics;
    ov_gpu_info_t       secondary_gpu;

    /* Multi-GPU Topology */
    ov_gpu_topology_t   gpu_topology;

    /* Memory */
    ov_memory_info_t    mem;

    /* Display */
    char                display_info[128];      /* e.g. "15.4-inch (1440x900 / 1680x1050)" */

    /* Storage */
    char                storage_interface[64];  /* e.g. "SATA 6.0 Gb/s", "NVMe PCIe 3.0 x4" */

    /* Firmware */
    char                firmware_type[64];      /* e.g. "Apple EFI 64-bit", "Apple iBoot" */
    bool                efi_is_64bit;
    bool                apfs_supported_in_firmware;

    /* Operating System Compatibility Window */
    char                native_macos_min[32];   /* e.g. "OS X 10.7.4 Lion" */
    char                native_macos_max[32];   /* e.g. "macOS 10.15.7 Catalina" */

    /* Silicon Quirks & Errata */
    char                silicon_quirks[256];

    bool                is_simulated;           /* True if simulated, false if real physical host */
    bool                is_mac_host;            /* True if physical host is a genuine Apple Mac */
} ov_hardware_profile_t;

/* Hardware detection and management */
ov_status_t ov_hardware_init(void);
void        ov_hardware_cleanup(void);

/* Host Detection (Safe, Read-Only, Never Modifies Firmware/Disks) */
ov_status_t ov_hardware_detect_host(ov_hardware_profile_t *out_host);
ov_status_t ov_hardware_detect_cpu(ov_cpu_info_t *cpu_info);
ov_status_t ov_hardware_detect_gpu(ov_gpu_info_t *gpu_info);
ov_status_t ov_hardware_detect_memory(ov_memory_info_t *mem_info);
ov_status_t ov_hardware_detect_pci(ov_pci_device_t **devices, uint32_t *device_count);
void        ov_hardware_free_pci(ov_pci_device_t *devices);

/* Hardware Profile Management */
uint32_t    ov_hardware_get_profile_count(void);
ov_status_t ov_hardware_get_profile(ov_hw_profile_id_t profile_id, ov_hardware_profile_t *out_profile);
ov_status_t ov_hardware_find_profile_by_name(const char *name, ov_hardware_profile_t *out_profile);
ov_status_t ov_hardware_set_active_profile(ov_hw_profile_id_t profile_id);
ov_status_t ov_hardware_set_active_profile_by_name(const char *name);
ov_hw_profile_id_t ov_hardware_get_active_profile_id(void);
const ov_hardware_profile_t* ov_hardware_get_active_profile(void);
ov_status_t ov_hardware_update_custom_profile(const ov_hardware_profile_t *custom);

/* Hardware Execution Mode & Source */
ov_status_t    ov_hardware_set_mode(ov_hw_mode_t mode);
ov_hw_mode_t   ov_hardware_get_mode(void);
ov_hw_source_t ov_hardware_get_source(void);
const char*    ov_hardware_get_source_string(void);
const char*    ov_hardware_get_mode_string(void);

/* Query Active Hardware & GPU Inventory */
const ov_cpu_info_t*     ov_hardware_get_cpu(void);
const ov_gpu_info_t*     ov_hardware_get_gpu(void);
const ov_gpu_info_t*     ov_hardware_get_secondary_gpu(void);
uint32_t                 ov_hardware_get_gpu_count(void);
const ov_gpu_info_t*     ov_hardware_get_gpu_at(uint32_t index);
const ov_gpu_topology_t* ov_hardware_get_gpu_topology(void);
const ov_memory_info_t*  ov_hardware_get_memory(void);

/* Separate Active GPU and Physical Inventory Management */
uint32_t                 ov_hardware_get_active_gpu_index(void);
ov_status_t              ov_hardware_set_active_gpu_index(uint32_t index);
const ov_gpu_info_t*     ov_hardware_get_active_gpu(void);
const ov_gpu_info_t*     ov_hardware_get_integrated_gpu(void);
const ov_gpu_info_t*     ov_hardware_get_discrete_gpu(void);

/* Dedicated Real Hardware (Native) Validation & Smoke Test */
ov_status_t ov_hardware_detect_native_strict(char *out_failure_subsystem, size_t max_len);
ov_status_t ov_hardware_validate_native_topology(char *out_err, size_t err_len);
void        ov_hardware_print_native_summary(void);
ov_status_t ov_hardware_run_smoke_test(void);
void        ov_hardware_set_native_detect_mock(ov_status_t (*mock_fn)(ov_hardware_profile_t *out_host));

/* CPUID Safe Bounds & Leaf Queries */
uint32_t ov_cpuid_max_leaf(void);
uint32_t ov_cpuid_max_ext_leaf(void);

/* CPUID Registers */
typedef struct {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
} cpuid_regs_t;

typedef cpuid_regs_t ov_cpuid_regs_t;

cpuid_regs_t ov_cpuid(uint32_t leaf, uint32_t subleaf);

/* CPU feature detection */
bool ov_cpu_supports_sse(void);
bool ov_cpu_supports_sse2(void);
bool ov_cpu_supports_sse3(void);
bool ov_cpu_supports_ssse3(void);
bool ov_cpu_supports_sse41(void);
bool ov_cpu_supports_sse42(void);
bool ov_cpu_supports_avx(void);
bool ov_cpu_supports_avx2(void);
bool ov_cpu_supports_avx512(void);
bool ov_cpu_supports_fma(void);
bool ov_cpu_supports_aesni(void);
bool ov_cpu_supports_tsc(void);

#endif /* OV_HARDWARE_H */
