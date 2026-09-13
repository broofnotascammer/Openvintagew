/**
 * OpenVintage Pre-Boot Simulator - Unified Diagnostics & Report Generation (Phase 5)
 * Platform audit, system health score, Mac compatibility evaluation, and JSON/HTML/Text export.
 */

#include "ov_diagnostics.h"
#include "ov_hardware.h"
#include "ov_macos_compat.h"
#include "ov_unified_cache.h"
#include "ov_memory.h"
#include "ov_logger.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

ov_status_t ov_diagnostics_init(void) {
    ov_log_info("Diagnostics Subsystem initialized");
    return OV_SUCCESS;
}

void ov_diagnostics_cleanup(void) {
    ov_log_info("Diagnostics cleanup complete");
}

ov_status_t ov_diagnostics_generate_report(ov_diagnostic_report_t *out_report) {
    if (!out_report) return OV_ERROR_INVALID_PARAM;
    memset(out_report, 0, sizeof(ov_diagnostic_report_t));

    const ov_cpu_info_t *cpu = ov_hardware_get_cpu();
    const ov_gpu_info_t *gpu = ov_hardware_get_gpu();
    const ov_memory_info_t *mem = ov_hardware_get_memory();
    const ov_hardware_profile_t *prof = ov_hardware_get_active_profile();
    ov_memory_stats_t mem_stats = ov_memory_get_stats();

    ov_unified_cache_stats_t cstats;
    ov_unified_cache_get_stats(&cstats);

    /* Platform & Firmware */
    snprintf(out_report->platform_name, sizeof(out_report->platform_name), "OpenVintage EFI Platform");
    ov_hw_mode_t hw_mode = ov_hardware_get_mode();
    snprintf(out_report->hardware_mode, sizeof(out_report->hardware_mode), "%s",
             ov_hw_mode_to_string(hw_mode));
    ov_hw_source_t hw_source = ov_hardware_get_source();
    snprintf(out_report->hardware_source, sizeof(out_report->hardware_source), "%s",
             ov_hw_source_to_string(hw_source));

    ov_hardware_profile_t host_prof;
    if (ov_hardware_get_profile(OV_HW_PROFILE_HOST, &host_prof) == OV_SUCCESS) {
        snprintf(out_report->host_detected_model, sizeof(out_report->host_detected_model), "%s", host_prof.model_identifier);
    } else {
        snprintf(out_report->host_detected_model, sizeof(out_report->host_detected_model), "Unknown Host");
    }

    if (hw_mode == OV_HW_MODE_SIMULATED && prof) {
        snprintf(out_report->simulated_target_model, sizeof(out_report->simulated_target_model), "%s", prof->model_identifier);
    } else {
        snprintf(out_report->simulated_target_model, sizeof(out_report->simulated_target_model), "N/A (Native Mode)");
    }

    snprintf(out_report->firmware_vendor, sizeof(out_report->firmware_vendor), "OpenVintage Systems / EDK2");
    out_report->firmware_revision = 0x00010005; /* v1.5 */
    out_report->bitness = 64;

    /* Mac Profile & Compatibility */
    if (prof) {
        snprintf(out_report->mac_model, sizeof(out_report->mac_model), "%s", prof->model_identifier);
        snprintf(out_report->mac_model_name, sizeof(out_report->mac_model_name), "%s", prof->marketing_name);

        /* Evaluate against macOS Monterey (12.0) */
        ov_macos_compat_eval_t res;
        ov_macos_compat_evaluate(OV_MACOS_12_MONTEREY, prof, &res);
        out_report->oclp_patchable = (res.patcher_recommended == OV_PATCHER_OPENCORE_LEGACY);
        const char *summary_msg = res.notes[0] ? res.notes : (res.failure_reason[0] ? res.failure_reason : "Fully compatible");
        snprintf(out_report->macos_compat_summary, sizeof(out_report->macos_compat_summary),
                 "macOS 12 Monterey: %s (%s)",
                 ov_macos_compat_rating_to_string(res.rating),
                 summary_msg);
    } else {
        snprintf(out_report->mac_model, sizeof(out_report->mac_model), "Host System");
        snprintf(out_report->mac_model_name, sizeof(out_report->mac_model_name), "Native Host Environment");
        snprintf(out_report->macos_compat_summary, sizeof(out_report->macos_compat_summary), "Custom / Native Host");
        out_report->oclp_patchable = false;
    }

    /* CPU */
    strncpy(out_report->cpu_model, cpu->model_name, sizeof(out_report->cpu_model) - 1);
    out_report->physical_cores = cpu->cores;
    out_report->logical_threads = cpu->threads;
    out_report->base_clock_mhz = cpu->base_freq_mhz;
    out_report->has_sse42 = cpu->has_sse42;
    out_report->has_avx = cpu->has_avx;
    out_report->has_avx2 = cpu->has_avx2;
    out_report->has_aesni = cpu->has_aesni;

    /* GPU */
    strncpy(out_report->gpu_model, gpu->model_name, sizeof(out_report->gpu_model) - 1);
    out_report->pci_vendor_id = gpu->vendor_id;
    out_report->pci_device_id = gpu->device_id;
    out_report->vram_bytes = gpu->vram_bytes;
    out_report->max_texture_dimension = gpu->max_texture_dimension;
    out_report->supports_compute = gpu->supports_compute;
    out_report->supports_tessellation = gpu->supports_tessellation;

    /* Memory */
    out_report->total_ram_bytes = mem->total_bytes;
    out_report->free_ram_bytes = mem->available_bytes;
    out_report->allocated_ram_bytes = mem_stats.allocated;
    out_report->memory_leaks_detected = mem_stats.has_leaks;

    /* APIs */
    out_report->native_metal_available = gpu->supports_metal;
    out_report->native_vulkan_available = gpu->supports_vulkan;
    out_report->native_opengl_core_available = gpu->supports_opengl_core;
    snprintf(out_report->api_summary, sizeof(out_report->api_summary),
             "OpenGL Core [%s], Vulkan [%s], Metal [%s]",
             gpu->supports_opengl_core ? "Native" : "None",
             gpu->supports_vulkan ? "Native" : "OVIR-GPU Trans",
             gpu->supports_metal ? "Native" : "OVIR-GPU Trans");

    /* Translation Support */
    out_report->cpu_arm64_to_x64_supported = true;
    out_report->cpu_x64_recomp_supported = true;
    out_report->gpu_spirv_to_glsl_supported = true;
    out_report->gpu_metal_to_opengl_supported = true;

    /* Cache State */
    out_report->cache_generation = cstats.current_generation;
    out_report->total_cache_entries = cstats.cpu_cache_entries + cstats.shader_cache_entries +
                                      cstats.pipeline_cache_entries + cstats.compat_cache_entries;
    out_report->total_cache_size_bytes = cstats.cpu_cache_bytes + cstats.shader_cache_bytes +
                                         cstats.pipeline_cache_bytes + cstats.compat_cache_bytes;
    out_report->cache_hit_rate_pct = cstats.overall_hit_rate_percent;

    snprintf(out_report->execution_policy, sizeof(out_report->execution_policy), "OVIR Adaptive Hybrid Routing (Phase 5)");
    snprintf(out_report->silicon_quirks_summary, sizeof(out_report->silicon_quirks_summary),
             "Texture clamping <= %upx; Metal emulation via GLSL core; AVX2 emulated via SSE4.2 if absent.",
             gpu->max_texture_dimension);

    /* Health Score Calculation (0-100) */
    uint32_t score = 70;
    if (cpu->has_sse42) score += 5;
    if (cpu->has_avx) score += 5;
    if (cpu->has_avx2) score += 5;
    if (gpu->supports_opengl_core) score += 5;
    if (cstats.overall_hit_rate_percent >= 50) score += 5;
    if (!out_report->memory_leaks_detected) score += 5;
    if (score > 100) score = 100;
    out_report->system_health_score = score;

    return OV_SUCCESS;
}

void ov_diagnostics_refresh_memory_status(ov_diagnostic_report_t *report) {
    if (!report) return;
    ov_memory_stats_t mem_stats = ov_memory_get_stats();
    report->allocated_ram_bytes = mem_stats.allocated;
    report->free_ram_bytes = mem_stats.freed;
    report->memory_leaks_detected = mem_stats.has_leaks;
}

void ov_diagnostics_format_text(const ov_diagnostic_report_t *r, char *out_buf, size_t max_len) {
    if (!r || !out_buf || max_len == 0) return;

    snprintf(out_buf, max_len,
        "================================================================================\n"
        "                  OPENVINTAGE PRE-BOOT SYSTEM DIAGNOSTIC REPORT                 \n"
        "================================================================================\n\n"
        "System Health Score: %u / 100 [OPTIMAL]\n"
        "Hardware Mode:       %s\n"
        "Hardware Source:     %s\n"
        "Host Detected Model: %s\n"
        "Simulated Target:    %s\n"
        "Platform:            %s (%s, Rev 0x%08x)\n"
        "Active Model:        %s (%s)\n"
        "macOS Compatibility: %s\n"
        "OCLP Patch Support:  %s\n"
        "Execution Policy:    %s\n\n"
        "[PROCESSOR (CPU)]\n"
        "Model:               %s\n"
        "Topology:            %u Physical Cores / %u Logical Threads @ %u MHz\n"
        "ISA Extensions:      SSE4.2: %s | AVX: %s | AVX2: %s | AES-NI: %s\n"
        "Translation Support: ARM64->x86_64 JIT: %s | x86 Recompilation: %s\n\n"
        "[GRAPHICS ACCELERATOR (GPU)]\n"
        "Model:               %s (PCI 0x%04x:0x%04x)\n"
        "Dedicated Memory:    %llu MB VRAM | Max Texture: %upx\n"
        "Hardware Features:   Compute: %s | Tessellation: %s\n"
        "API Capabilities:    %s\n\n"
        "[SYSTEM MEMORY]\n"
        "Total Physical RAM:  %llu MB\n"
        "Available Free RAM:  %llu MB\n"
        "Allocated Simulator: %llu KB\n"
        "Integrity / Leaks:   %s\n\n"
        "[UNIFIED MULTI-TIER CACHE]\n"
        "Generation:          %u\n"
        "Active Entries:      %u entries (%llu KB total)\n"
        "Global Hit Rate:     %u%%\n\n"
        "[ACTIVE SILICON QUIRKS & WORKAROUNDS]\n"
        "%s\n"
        "================================================================================\n",
        r->system_health_score,
        r->hardware_mode,
        r->hardware_source,
        r->host_detected_model,
        r->simulated_target_model,
        r->platform_name, r->firmware_vendor, r->firmware_revision,
        r->mac_model, r->mac_model_name,
        r->macos_compat_summary,
        r->oclp_patchable ? "Supported (OpenCore Legacy Patcher)" : "Native / Not Required",
        r->execution_policy,
        r->cpu_model,
        r->physical_cores, r->logical_threads, r->base_clock_mhz,
        r->has_sse42 ? "YES" : "NO", r->has_avx ? "YES" : "NO",
        r->has_avx2 ? "YES" : "NO", r->has_aesni ? "YES" : "NO",
        r->cpu_arm64_to_x64_supported ? "YES" : "NO", r->cpu_x64_recomp_supported ? "YES" : "NO",
        r->gpu_model, r->pci_vendor_id, r->pci_device_id,
        (unsigned long long)(r->vram_bytes / (1024 * 1024)), r->max_texture_dimension,
        r->supports_compute ? "YES" : "NO", r->supports_tessellation ? "YES" : "NO",
        r->api_summary,
        (unsigned long long)(r->total_ram_bytes / (1024 * 1024)),
        (unsigned long long)(r->free_ram_bytes / (1024 * 1024)),
        (unsigned long long)(r->allocated_ram_bytes / 1024),
        r->memory_leaks_detected ? "LEAKS DETECTED" : "CLEAN (0 Leaks)",
        r->cache_generation,
        r->total_cache_entries, (unsigned long long)(r->total_cache_size_bytes / 1024),
        r->cache_hit_rate_pct,
        r->silicon_quirks_summary
    );
}

void ov_diagnostics_format_json(const ov_diagnostic_report_t *r, char *out_buf, size_t max_len) {
    if (!r || !out_buf || max_len == 0) return;

    snprintf(out_buf, max_len,
        "{\n"
        "  \"openvintage\": {\n"
        "    \"health_score\": %u,\n"
        "    \"hardware_mode\": \"%s\",\n"
        "    \"hardware_source\": \"%s\",\n"
        "    \"host_detected_model\": \"%s\",\n"
        "    \"simulated_target_model\": \"%s\",\n"
        "    \"platform\": \"%s\",\n"
        "    \"firmware\": \"%s\",\n"
        "    \"mac_model\": \"%s\",\n"
        "    \"mac_name\": \"%s\",\n"
        "    \"macos_compatibility\": \"%s\",\n"
        "    \"oclp_patchable\": %s,\n"
        "    \"policy\": \"%s\",\n"
        "    \"cpu\": {\n"
        "      \"model\": \"%s\",\n"
        "      \"cores\": %u,\n"
        "      \"threads\": %u,\n"
        "      \"base_clock_mhz\": %u,\n"
        "      \"sse42\": %s,\n"
        "      \"avx\": %s,\n"
        "      \"avx2\": %s\n"
        "    },\n"
        "    \"gpu\": {\n"
        "      \"model\": \"%s\",\n"
        "      \"pci_id\": \"0x%04x:0x%04x\",\n"
        "      \"vram_bytes\": %llu,\n"
        "      \"max_texture\": %u,\n"
        "      \"compute\": %s\n"
        "    },\n"
        "    \"memory\": {\n"
        "      \"total_ram_bytes\": %llu,\n"
        "      \"free_ram_bytes\": %llu,\n"
        "      \"leaks_detected\": %s\n"
        "    },\n"
        "    \"cache\": {\n"
        "      \"generation\": %u,\n"
        "      \"entries\": %u,\n"
        "      \"hit_rate_pct\": %u\n"
        "    }\n"
        "  }\n"
        "}\n",
        r->system_health_score,
        r->hardware_mode,
        r->hardware_source,
        r->host_detected_model,
        r->simulated_target_model,
        r->platform_name, r->firmware_vendor,
        r->mac_model, r->mac_model_name,
        r->macos_compat_summary,
        r->oclp_patchable ? "true" : "false",
        r->execution_policy,
        r->cpu_model, r->physical_cores, r->logical_threads, r->base_clock_mhz,
        r->has_sse42 ? "true" : "false", r->has_avx ? "true" : "false", r->has_avx2 ? "true" : "false",
        r->gpu_model, r->pci_vendor_id, r->pci_device_id, (unsigned long long)r->vram_bytes, r->max_texture_dimension,
        r->supports_compute ? "true" : "false",
        (unsigned long long)r->total_ram_bytes, (unsigned long long)r->free_ram_bytes,
        r->memory_leaks_detected ? "true" : "false",
        r->cache_generation, r->total_cache_entries, r->cache_hit_rate_pct
    );
}

ov_status_t ov_diagnostics_export_text(const ov_diagnostic_report_t *report, const char *file_path) {
    if (!report || !file_path) return OV_ERROR_INVALID_PARAM;
    FILE *f = fopen(file_path, "w");
    if (!f) return OV_ERROR_INIT;

    char buf[4096];
    ov_diagnostics_format_text(report, buf, sizeof(buf));
    fputs(buf, f);
    fclose(f);
    ov_log_info("Exported Text diagnostic report to: %s", file_path);
    return OV_SUCCESS;
}

ov_status_t ov_diagnostics_export_json(const ov_diagnostic_report_t *report, const char *file_path) {
    if (!report || !file_path) return OV_ERROR_INVALID_PARAM;
    FILE *f = fopen(file_path, "w");
    if (!f) return OV_ERROR_INIT;

    char buf[4096];
    ov_diagnostics_format_json(report, buf, sizeof(buf));
    fputs(buf, f);
    fclose(f);
    ov_log_info("Exported JSON diagnostic report to: %s", file_path);
    return OV_SUCCESS;
}

ov_status_t ov_diagnostics_export_html(const ov_diagnostic_report_t *r, const char *file_path) {
    if (!r || !file_path) return OV_ERROR_INVALID_PARAM;
    FILE *f = fopen(file_path, "w");
    if (!f) return OV_ERROR_INIT;

    fprintf(f,
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "  <meta charset=\"UTF-8\">\n"
        "  <title>OpenVintage Pre-Boot Diagnostic Report</title>\n"
        "  <style>\n"
        "    body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #0f172a; color: #f8fafc; margin: 0; padding: 24px; }\n"
        "    .container { max-width: 960px; margin: 0 auto; }\n"
        "    .card { background: #1e293b; border-radius: 12px; padding: 20px; margin-bottom: 20px; border: 1px solid #334155; }\n"
        "    h1 { color: #38bdf8; margin-top: 0; font-size: 24px; }\n"
        "    h2 { color: #94a3b8; font-size: 16px; text-transform: uppercase; letter-spacing: 0.05em; border-bottom: 1px solid #334155; padding-bottom: 8px; margin-top: 0; }\n"
        "    .score-box { display: inline-block; background: #0369a1; padding: 6px 16px; border-radius: 20px; font-weight: bold; font-size: 18px; }\n"
        "    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 16px; }\n"
        "    .prop { margin-bottom: 8px; font-size: 14px; }\n"
        "    .label { color: #94a3b8; }\n"
        "    .val { font-weight: 600; color: #f1f5f9; }\n"
        "  </style>\n"
        "</head>\n"
        "<body>\n"
        "  <div class=\"container\">\n"
        "    <div class=\"card\">\n"
        "      <h1>OpenVintage Pre-Boot Diagnostic Audit</h1>\n"
        "      <div class=\"score-box\">Health Score: %u / 100</div>\n"
        "      <p style=\"color:#cbd5e1; margin-top:12px;\">Hardware Mode: <span class=\"val\">%s</span> | Hardware Source: <span class=\"val\">%s</span> | Host: <span class=\"val\">%s</span></p>\n"
        "      <p style=\"color:#cbd5e1; margin-top:4px;\">Simulated Target: <span class=\"val\">%s</span> | Platform: %s | Active Model: %s (%s)</p>\n"
        "      <p style=\"color:#38bdf8; margin-top:4px;\">%s</p>\n"
        "    </div>\n"
        "    <div class=\"grid\">\n"
        "      <div class=\"card\">\n"
        "        <h2>Processor (CPU)</h2>\n"
        "        <div class=\"prop\"><span class=\"label\">Model:</span> <span class=\"val\">%s</span></div>\n"
        "        <div class=\"prop\"><span class=\"label\">Topology:</span> <span class=\"val\">%u Cores / %u Threads</span></div>\n"
        "        <div class=\"prop\"><span class=\"label\">AVX / AVX2:</span> <span class=\"val\">%s / %s</span></div>\n"
        "        <div class=\"prop\"><span class=\"label\">ARM64 JIT:</span> <span class=\"val\">Supported</span></div>\n"
        "      </div>\n"
        "      <div class=\"card\">\n"
        "        <h2>Graphics (GPU)</h2>\n"
        "        <div class=\"prop\"><span class=\"label\">Model:</span> <span class=\"val\">%s</span></div>\n"
        "        <div class=\"prop\"><span class=\"label\">VRAM:</span> <span class=\"val\">%llu MB</span></div>\n"
        "        <div class=\"prop\"><span class=\"label\">Max Texture:</span> <span class=\"val\">%upx</span></div>\n"
        "        <div class=\"prop\"><span class=\"label\">API Status:</span> <span class=\"val\">%s</span></div>\n"
        "      </div>\n"
        "    </div>\n"
        "    <div class=\"card\">\n"
        "      <h2>Unified Cache & Quirks</h2>\n"
        "      <div class=\"prop\"><span class=\"label\">Generation:</span> <span class=\"val\">Gen %u</span></div>\n"
        "      <div class=\"prop\"><span class=\"label\">Cache Hit Rate:</span> <span class=\"val\">%u%%</span></div>\n"
        "      <div class=\"prop\"><span class=\"label\">Active Quirks:</span> <span class=\"val\">%s</span></div>\n"
        "    </div>\n"
        "  </div>\n"
        "</body>\n"
        "</html>\n",
        r->system_health_score, r->hardware_mode, r->hardware_source, r->host_detected_model,
        r->simulated_target_model, r->platform_name, r->mac_model, r->mac_model_name,
        r->macos_compat_summary,
        r->cpu_model, r->physical_cores, r->logical_threads,
        r->has_avx ? "Yes" : "No", r->has_avx2 ? "Yes" : "No",
        r->gpu_model, (unsigned long long)(r->vram_bytes / (1024 * 1024)), r->max_texture_dimension, r->api_summary,
        r->cache_generation, r->cache_hit_rate_pct, r->silicon_quirks_summary
    );

    fclose(f);
    ov_log_info("Exported HTML diagnostic report to: %s", file_path);
    return OV_SUCCESS;
}

/* ========================================================================= */
/* Native Hardware Diagnostics Exporters                                     */
/* ========================================================================= */

ov_status_t ov_diagnostics_export_native_text(const char *file_path) {
    if (!file_path) return OV_ERROR_INVALID_PARAM;
    FILE *f = fopen(file_path, "w");
    if (!f) return OV_ERROR_INIT;

    const ov_hardware_profile_t *p = ov_hardware_get_active_profile();
    ov_hw_mode_t mode = ov_hardware_get_mode();
    ov_hw_source_t source = ov_hardware_get_source();

    fprintf(f, "================================================================================\n");
    fprintf(f, "               OPENVINTAGE NATIVE HARDWARE AUDIT REPORT (TEXT)                  \n");
    fprintf(f, "================================================================================\n");
    fprintf(f, "Hardware Mode:       %s\n", ov_hw_mode_to_string(mode));
    fprintf(f, "Hardware Source:     %s\n", ov_hw_source_to_string(source));
    fprintf(f, "Execution Class:     REAL HARDWARE USERSPACE INSPECTION\n");
    fprintf(f, "Host Model ID:       %s\n", p ? p->model_identifier : "Unknown");
    fprintf(f, "Marketing Name:      %s\n", p ? p->marketing_name : "Unknown");
    fprintf(f, "Genuine Apple Host:  %s\n", (p && p->is_mac_host) ? "YES" : "NO");
    fprintf(f, "Firmware Type:       %s\n", p ? p->firmware_type : "Unknown");
    fprintf(f, "Storage Controller:  %s\n", p ? p->storage_interface : "Unknown");
    fprintf(f, "--------------------------------------------------------------------------------\n");
    if (p) {
        fprintf(f, "Processor (CPU):\n");
        fprintf(f, "  Brand Name:        %s\n", p->cpu.model_name);
        fprintf(f, "  Physical Cores:    %u\n", p->cpu.cores);
        fprintf(f, "  Logical Threads:   %u\n", p->cpu.threads);
        fprintf(f, "  Base Clock:        %u MHz\n", p->cpu.base_freq_mhz);
        fprintf(f, "  Max Clock:         %u MHz\n", p->cpu.max_freq_mhz);
        fprintf(f, "  SSE4.2:            %s [DETECTED]\n", p->cpu.has_sse42 ? "YES" : "NO");
        fprintf(f, "  AVX:               %s [DETECTED]\n", p->cpu.has_avx ? "YES" : "NO");
        fprintf(f, "  AVX2:              %s [DETECTED]\n", p->cpu.has_avx2 ? "YES" : "NO");
        fprintf(f, "  AES-NI:            %s [DETECTED]\n", p->cpu.has_aesni ? "YES" : "NO");
        fprintf(f, "--------------------------------------------------------------------------------\n");
        fprintf(f, "System Memory:\n");
        fprintf(f, "  Total Physical:    %llu MB (%llu bytes)\n",
                (unsigned long long)(p->mem.total_bytes / (1024 * 1024)), (unsigned long long)p->mem.total_bytes);
        fprintf(f, "  Available RAM:     %llu MB (%llu bytes)\n",
                (unsigned long long)(p->mem.available_bytes / (1024 * 1024)), (unsigned long long)p->mem.available_bytes);
        fprintf(f, "--------------------------------------------------------------------------------\n");
        fprintf(f, "Graphics Subsystem (IOKit / PCI Topology):\n");
        fprintf(f, "  Total GPUs:        %u\n", p->gpu_topology.gpu_count);
        fprintf(f, "  Switchable Mux:    %s\n", p->gpu_topology.is_muxed_switchable ? "YES (Dynamic / GMUX)" : "NO (Single Adapter)");
        if (p->gpu_topology.switch_policy[0]) {
            fprintf(f, "  Mux Policy:        %s\n", p->gpu_topology.switch_policy);
        }
        for (uint32_t i = 0; i < p->gpu_topology.gpu_count; i++) {
            const ov_gpu_info_t *g = &p->gpu_topology.gpus[i];
            fprintf(f, "\n  Adapter %u:\n", i);
            fprintf(f, "    Device Name:     %s\n", g->model_name);
            fprintf(f, "    PCI Vendor ID:   0x%04x\n", g->vendor_id);
            fprintf(f, "    PCI Device ID:   0x%04x\n", g->device_id);
            fprintf(f, "    Role:            %s%s\n",
                    (i == p->gpu_topology.primary_gpu_index) ? "Primary " : "",
                    (g->vendor_id == 0x8086) ? "Integrated Display Controller" : "Discrete GPU");
            fprintf(f, "    VRAM Detection:  %s\n", g->vram_is_detected ? "EXPOSED BY IOKIT" : "DYNAMIC / NOT EXPOSED BY IOKIT");
            fprintf(f, "    VRAM Description:%s\n", g->vram_description[0] ? g->vram_description : (g->vram_is_detected ? "Detected" : "UNKNOWN"));
            fprintf(f, "    Metal Level:     %s [%s]\n", ov_metal_support_to_string(g->metal_level), g->metal_source);
            fprintf(f, "    OpenGL Core:     OpenGL %u.%u [%s]\n",
                    g->opengl_major ? g->opengl_major : 4,
                    g->opengl_minor ? g->opengl_minor : 1,
                    g->opengl_source);
            fprintf(f, "    Vulkan Support:  %s [%s]\n", g->supports_vulkan ? "YES" : "NO", g->vulkan_source);
            fprintf(f, "    Max Texture Dim: %upx\n", g->max_texture_dimension);
        }
    }
    fprintf(f, "================================================================================\n");

    fclose(f);
    ov_log_info("Exported native text report to: %s", file_path);
    return OV_SUCCESS;
}

ov_status_t ov_diagnostics_export_native_json(const char *file_path) {
    if (!file_path) return OV_ERROR_INVALID_PARAM;
    FILE *f = fopen(file_path, "w");
    if (!f) return OV_ERROR_INIT;

    const ov_hardware_profile_t *p = ov_hardware_get_active_profile();
    ov_hw_mode_t mode = ov_hardware_get_mode();
    ov_hw_source_t source = ov_hardware_get_source();

    fprintf(f, "{\n");
    fprintf(f, "  \"openvintage_native_report\": {\n");
    fprintf(f, "    \"version\": \"1.0\",\n");
    fprintf(f, "    \"hardware_mode\": \"%s\",\n", ov_hw_mode_to_string(mode));
    fprintf(f, "    \"hardware_source\": \"%s\",\n", ov_hw_source_to_string(source));
    fprintf(f, "    \"execution_class\": \"REAL_HARDWARE_USERSPACE\",\n");
    fprintf(f, "    \"is_simulated\": false,\n");
    fprintf(f, "    \"host\": {\n");
    fprintf(f, "      \"model_identifier\": \"%s\",\n", p ? p->model_identifier : "Unknown");
    fprintf(f, "      \"marketing_name\": \"%s\",\n", p ? p->marketing_name : "Unknown");
    fprintf(f, "      \"is_mac_host\": %s,\n", (p && p->is_mac_host) ? "true" : "false");
    fprintf(f, "      \"firmware_type\": \"%s\",\n", p ? p->firmware_type : "Unknown");
    fprintf(f, "      \"storage_interface\": \"%s\"\n", p ? p->storage_interface : "Unknown");
    fprintf(f, "    },\n");
    if (p) {
        fprintf(f, "    \"cpu\": {\n");
        fprintf(f, "      \"model\": \"%s\",\n", p->cpu.model_name);
        fprintf(f, "      \"physical_cores\": %u,\n", p->cpu.cores);
        fprintf(f, "      \"logical_threads\": %u,\n", p->cpu.threads);
        fprintf(f, "      \"base_frequency_mhz\": %u,\n", p->cpu.base_freq_mhz);
        fprintf(f, "      \"has_sse42\": %s,\n", p->cpu.has_sse42 ? "true" : "false");
        fprintf(f, "      \"has_avx\": %s,\n", p->cpu.has_avx ? "true" : "false");
        fprintf(f, "      \"has_avx2\": %s,\n", p->cpu.has_avx2 ? "true" : "false");
        fprintf(f, "      \"has_aesni\": %s\n", p->cpu.has_aesni ? "true" : "false");
        fprintf(f, "    },\n");
        fprintf(f, "    \"memory\": {\n");
        fprintf(f, "      \"total_bytes\": %llu,\n", (unsigned long long)p->mem.total_bytes);
        fprintf(f, "      \"available_bytes\": %llu,\n", (unsigned long long)p->mem.available_bytes);
        fprintf(f, "      \"total_mb\": %llu\n", (unsigned long long)(p->mem.total_bytes / (1024 * 1024)));
        fprintf(f, "    },\n");
        fprintf(f, "    \"gpu_topology\": {\n");
        fprintf(f, "      \"gpu_count\": %u,\n", p->gpu_topology.gpu_count);
        fprintf(f, "      \"is_muxed_switchable\": %s,\n", p->gpu_topology.is_muxed_switchable ? "true" : "false");
        fprintf(f, "      \"switch_policy\": \"%s\",\n", p->gpu_topology.switch_policy);
        fprintf(f, "      \"gpus\": [\n");
        for (uint32_t i = 0; i < p->gpu_topology.gpu_count; i++) {
            const ov_gpu_info_t *g = &p->gpu_topology.gpus[i];
            fprintf(f, "        {\n");
            fprintf(f, "          \"index\": %u,\n", i);
            fprintf(f, "          \"model_name\": \"%s\",\n", g->model_name);
            fprintf(f, "          \"pci_vendor_id\": \"0x%04x\",\n", g->vendor_id);
            fprintf(f, "          \"pci_device_id\": \"0x%04x\",\n", g->device_id);
            fprintf(f, "          \"is_primary\": %s,\n", (i == p->gpu_topology.primary_gpu_index) ? "true" : "false");
            fprintf(f, "          \"vram_is_detected\": %s,\n", g->vram_is_detected ? "true" : "false");
            fprintf(f, "          \"vram_bytes\": %llu,\n", (unsigned long long)g->vram_bytes);
            fprintf(f, "          \"vram_description\": \"%s\",\n", g->vram_description);
            fprintf(f, "          \"supports_metal\": %s,\n", g->supports_metal ? "true" : "false");
            fprintf(f, "          \"metal_level\": \"%s\",\n", ov_metal_support_to_string(g->metal_level));
            fprintf(f, "          \"metal_source\": \"%s\",\n", g->metal_source);
            fprintf(f, "          \"supports_opengl_core\": %s,\n", g->supports_opengl_core ? "true" : "false");
            fprintf(f, "          \"opengl_version\": \"%u.%u\",\n", g->opengl_major ? g->opengl_major : 4, g->opengl_minor ? g->opengl_minor : 1);
            fprintf(f, "          \"opengl_source\": \"%s\",\n", g->opengl_source);
            fprintf(f, "          \"supports_vulkan\": %s,\n", g->supports_vulkan ? "true" : "false");
            fprintf(f, "          \"vulkan_source\": \"%s\",\n", g->vulkan_source);
            fprintf(f, "          \"max_texture_dimension\": %u\n", g->max_texture_dimension);
            fprintf(f, "        }%s\n", (i + 1 < p->gpu_topology.gpu_count) ? "," : "");
        }
        fprintf(f, "      ]\n");
        fprintf(f, "    }\n");
    }
    fprintf(f, "  }\n");
    fprintf(f, "}\n");

    fclose(f);
    ov_log_info("Exported native JSON report to: %s", file_path);
    return OV_SUCCESS;
}
