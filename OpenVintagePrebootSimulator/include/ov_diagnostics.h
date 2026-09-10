/**
 * OpenVintage Pre-Boot Simulator - Unified Diagnostics & Report Generation (Phase 5)
 * Platform audit, system health score, Mac compatibility, and JSON/HTML/Text export.
 */

#ifndef OV_DIAGNOSTICS_H
#define OV_DIAGNOSTICS_H

#include "ov_types.h"
#include "ov_hardware.h"
#include "ov_unified_cache.h"
#include "ov_benchmark.h"

/* Comprehensive System Diagnostic Report */
typedef struct {
    /* Platform & Firmware */
    char     platform_name[64];
    char     firmware_vendor[32];
    uint32_t firmware_revision;
    uint32_t bitness;
    uint32_t system_health_score;        /* 0 - 100 */

    /* Mac Profile & Compatibility */
    char     mac_model[32];
    char     mac_model_name[64];
    char     macos_compat_summary[256];
    bool     oclp_patchable;

    /* CPU */
    char     cpu_model[128];
    uint32_t physical_cores;
    uint32_t logical_threads;
    uint32_t base_clock_mhz;
    bool     has_sse42;
    bool     has_avx;
    bool     has_avx2;
    bool     has_aesni;

    /* GPU */
    char     gpu_model[128];
    uint16_t pci_vendor_id;
    uint16_t pci_device_id;
    uint64_t vram_bytes;
    uint32_t max_texture_dimension;
    bool     supports_compute;
    bool     supports_tessellation;

    /* Memory */
    uint64_t total_ram_bytes;
    uint64_t free_ram_bytes;
    uint64_t allocated_ram_bytes;
    bool     memory_leaks_detected;

    /* Available APIs */
    bool     native_metal_available;
    bool     native_vulkan_available;
    bool     native_opengl_core_available;
    char     api_summary[128];

    /* Translation Support */
    bool     cpu_arm64_to_x64_supported;
    bool     cpu_x64_recomp_supported;
    bool     gpu_spirv_to_glsl_supported;
    bool     gpu_metal_to_opengl_supported;

    /* Cache State */
    uint32_t cache_generation;
    uint32_t total_cache_entries;
    uint64_t total_cache_size_bytes;
    uint32_t cache_hit_rate_pct;

    /* Policy */
    char     execution_policy[64];
    char     silicon_quirks_summary[256];
} ov_diagnostic_report_t;

/* Subsystem APIs */
ov_status_t ov_diagnostics_init(void);
void        ov_diagnostics_cleanup(void);

ov_status_t ov_diagnostics_generate_report(ov_diagnostic_report_t *out_report);

/* Exporters */
ov_status_t ov_diagnostics_export_text(const ov_diagnostic_report_t *report, const char *file_path);
ov_status_t ov_diagnostics_export_json(const ov_diagnostic_report_t *report, const char *file_path);
ov_status_t ov_diagnostics_export_html(const ov_diagnostic_report_t *report, const char *file_path);

/* Format in memory */
void ov_diagnostics_format_text(const ov_diagnostic_report_t *report, char *out_buf, size_t max_len);
void ov_diagnostics_format_json(const ov_diagnostic_report_t *report, char *out_buf, size_t max_len);

#endif /* OV_DIAGNOSTICS_H */
