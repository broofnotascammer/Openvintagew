/**
 * OpenVintage Pre-Boot Simulator - Hardware Detection & Simulation Profiles
 * Supports host hardware discovery (safe, read-only) and simulated target profiles.
 */

#ifndef OV_HARDWARE_H
#define OV_HARDWARE_H

#include "ov_types.h"

/* Pre-defined Hardware Simulation Profiles */
typedef enum {
    OV_HW_PROFILE_HOST = 0,             /* Actual Host System (read-only detected) */
    OV_HW_PROFILE_INTEL_GEN7 = 1,       /* Intel Ivy Bridge + HD 4000 (Gen7) */
    OV_HW_PROFILE_INTEL_GEN75 = 2,      /* Intel Haswell + HD 4600 / Iris (Gen7.5) */
    OV_HW_PROFILE_MODERN_INTEL = 3,     /* Modern Intel Core + Iris Xe */
    OV_HW_PROFILE_AMD_ZEN = 4,          /* AMD Ryzen Zen + Radeon */
    OV_HW_PROFILE_ARM64_GUEST = 5,      /* Simulated ARM64 Platform */
    OV_HW_PROFILE_CUSTOM = 6,           /* User-defined Custom Hardware Profile */
    OV_HW_PROFILE_COUNT
} ov_hw_profile_id_t;

/* CPU Hardware State */
typedef struct {
    ov_cpu_type_t   type;
    char            model_name[128];
    uint32_t        cores;
    uint32_t        threads;
    uint32_t        base_freq_mhz;
    uint32_t        max_freq_mhz;
    uint64_t        features_mask;
    bool            has_sse41;
    bool            has_sse42;
    bool            has_avx;
    bool            has_avx2;
    bool            has_avx512;
    bool            has_fma;
    bool            has_aesni;
    bool            has_tsc;
} ov_cpu_info_t;

/* GPU Hardware State */
typedef struct {
    ov_gpu_type_t   type;
    char            model_name[128];
    uint16_t        vendor_id;
    uint16_t        device_id;
    uint32_t        vram_mb;
    uint64_t        vram_bytes;
    uint32_t        eu_count;               /* Execution Units / Compute Units */
    uint32_t        max_texture_dimension;  /* e.g. 4096, 8192, 16384 */
    bool            has_compute;
    bool            has_graphics;
    bool            supports_compute;
    bool            supports_tessellation;
    bool            supports_metal;
    bool            supports_vulkan;
    bool            supports_opengl_core;
    bool            supports_directx;
    char            driver_version[64];
} ov_gpu_info_t;

/* System Memory State */
typedef struct {
    uint64_t        total_bytes;
    uint64_t        available_bytes;
    uint64_t        reserved_bytes;
    uint32_t        numa_nodes;
    uint32_t        channels;
    uint32_t        frequency_mhz;
} ov_memory_info_t;

/* PCI Device Entry */
typedef struct {
    uint16_t        vendor_id;
    uint16_t        device_id;
    uint8_t         bus;
    uint8_t         slot;
    uint8_t         func;
    char            class_name[128];
    char            vendor_name[128];
    char            device_name[256];
} ov_pci_device_t;

/* Hardware Profile Snapshot */
typedef struct {
    ov_hw_profile_id_t  profile_id;
    char                profile_name[64];
    char                description[256];
    ov_cpu_info_t       cpu;
    ov_gpu_info_t       gpu;
    ov_memory_info_t    mem;
    bool                is_simulated;       /* True if simulated, false if real host */
} ov_hardware_profile_t;

/* Hardware detection and management */
ov_status_t ov_hardware_init(void);
void        ov_hardware_cleanup(void);

/* Host Detection (Safe, Read-Only) */
ov_status_t ov_hardware_detect_host(ov_hardware_profile_t *out_host);
ov_status_t ov_hardware_detect_cpu(ov_cpu_info_t *cpu_info);
ov_status_t ov_hardware_detect_gpu(ov_gpu_info_t *gpu_info);
ov_status_t ov_hardware_detect_memory(ov_memory_info_t *mem_info);
ov_status_t ov_hardware_detect_pci(ov_pci_device_t **devices, uint32_t *device_count);
void        ov_hardware_free_pci(ov_pci_device_t *devices);

/* Hardware Profile Management */
ov_status_t ov_hardware_get_profile(ov_hw_profile_id_t profile_id, ov_hardware_profile_t *out_profile);
ov_status_t ov_hardware_set_active_profile(ov_hw_profile_id_t profile_id);
ov_hw_profile_id_t ov_hardware_get_active_profile_id(void);
const ov_hardware_profile_t* ov_hardware_get_active_profile(void);
ov_status_t ov_hardware_update_custom_profile(const ov_hardware_profile_t *custom);

/* Query Active Hardware */
const ov_cpu_info_t*    ov_hardware_get_cpu(void);
const ov_gpu_info_t*    ov_hardware_get_gpu(void);
const ov_memory_info_t* ov_hardware_get_memory(void);

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
bool ov_cpu_supports_sse41(void);
bool ov_cpu_supports_sse42(void);
bool ov_cpu_supports_avx(void);
bool ov_cpu_supports_avx2(void);
bool ov_cpu_supports_avx512(void);
bool ov_cpu_supports_fma(void);
bool ov_cpu_supports_aesni(void);
bool ov_cpu_supports_tsc(void);

#endif /* OV_HARDWARE_H */
