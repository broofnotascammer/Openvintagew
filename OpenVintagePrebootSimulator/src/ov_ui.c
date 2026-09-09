/**
 * OpenVintage Pre-Boot Simulator - Native GTK4 User Interface (Phases 1-5)
 */

#include "ov_ui.h"
#include "ov_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef OV_ENABLE_GTK
#include <gtk/gtk.h>

/* UI State References */
typedef struct {
    GtkApplication *app;
    GtkWidget      *window;
    GtkWidget      *notebook;

    /* Dashboard Widgets */
    GtkWidget      *lbl_health_score;
    GtkWidget      *lbl_phase_status;
    GtkWidget      *prog_boot;
    GtkWidget      *lbl_dash_cpu;
    GtkWidget      *lbl_dash_gpu;
    GtkWidget      *lbl_dash_ram;
    GtkWidget      *lbl_dash_cache;

    /* Resolver Tab Widgets */
    GtkWidget      *combo_workload;
    GtkWidget      *lbl_resolve_decision;
    GtkWidget      *lbl_resolve_reason;
    GtkWidget      *lbl_resolve_overhead;

    /* OVIR CPU & GPU Widgets */
    GtkTextBuffer  *txt_cpu_ir;
    GtkWidget      *lbl_cpu_opt_stats;
    GtkWidget      *chk_opt_fold;
    GtkWidget      *chk_opt_dce;
    GtkWidget      *chk_opt_move;
    GtkTextBuffer  *txt_gpu_cmds;
    GtkTextBuffer  *txt_gpu_shader;

    /* Resource & Cache Widgets */
    GtkWidget      *lbl_active_profile;
    GtkWidget      *lbl_threads_allocated;
    GtkWidget      *lbl_vram_budget;
    GtkWidget      *lbl_cache_gen;
    GtkWidget      *lbl_cache_hit_rate;
    GtkWidget      *lbl_cache_summary;

    /* Compatibility Widgets */
    GtkWidget      *lbl_compat_details;
    GtkWidget      *combo_apps;

    /* Benchmark Widgets */
    GtkTextBuffer  *txt_bench_results;

    /* Diagnostics Tab Widgets */
    GtkTextBuffer  *txt_diag_report;
    GtkTextBuffer  *txt_logs;
} ov_ui_state_t;

static ov_ui_state_t g_ui = {0};

/* Forward Declarations */
static void update_all_ui_views(void);

bool ov_ui_is_display_available(void) {
    const char *disp = getenv("DISPLAY");
    const char *wayland = getenv("WAYLAND_DISPLAY");
    return (disp && strlen(disp) > 0) || (wayland && strlen(wayland) > 0);
}

/* UI Action Callbacks */
static void on_btn_rerun_boot_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    ov_log_info("UI: Re-running Pre-Boot Execution sequence...");
    ov_core_execute_preboot();
    update_all_ui_views();
}

static void on_btn_run_benchmarks_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    ov_log_info("UI: Re-running Benchmark Suite...");
    ov_benchmark_run_suite(&ov_core_instance.benchmark_results);
    update_all_ui_views();
}

static void on_btn_invalidate_cache_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    ov_log_info("UI: Invalidating all cache tiers...");
    ov_unified_cache_invalidate(OV_CACHE_TIER_ALL, OV_INVALIDATE_MANUAL);
    ov_unified_cache_get_stats(&ov_core_instance.cache_stats);
    update_all_ui_views();
}

static void on_btn_profile_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    ov_resource_profile_t prof = (ov_resource_profile_t)GPOINTER_TO_INT(user_data);
    ov_resource_manager_set_profile(prof);
    ov_resource_manager_get_status(&ov_core_instance.resource_status);
    update_all_ui_views();
}

static void on_btn_export_report_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    ov_diagnostics_export_text(&ov_core_instance.diagnostic_report, "openvintage_diagnostics.txt");
    ov_diagnostics_export_json(&ov_core_instance.diagnostic_report, "openvintage_diagnostics.json");
    ov_diagnostics_export_html(&ov_core_instance.diagnostic_report, "openvintage_diagnostics.html");
    ov_log_info("Exported Text, JSON, and HTML diagnostic reports to current directory");
    update_all_ui_views();
}

static void on_btn_optimize_cpu_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    ov_cpu_program_t prog;
    ov_cpu_build_sample_program(0, &prog);

    bool fold = gtk_check_button_get_active(GTK_CHECK_BUTTON(g_ui.chk_opt_fold));
    bool dce = gtk_check_button_get_active(GTK_CHECK_BUTTON(g_ui.chk_opt_dce));
    bool move_e = gtk_check_button_get_active(GTK_CHECK_BUTTON(g_ui.chk_opt_move));

    ov_cpu_optimizer_stats_t stats;
    ov_cpu_optimize_program(&prog, fold, dce, move_e, &stats);

    char ir_buf[4096];
    ov_cpu_format_program_ir(&prog, ir_buf, sizeof(ir_buf));
    gtk_text_buffer_set_text(g_ui.txt_cpu_ir, ir_buf, -1);

    char stat_buf[256];
    snprintf(stat_buf, sizeof(stat_buf),
             "Original: %u | Optimized: %u instructions (-%.1f%%) | Folded: %u | DCE: %u | Moves Elim: %u",
             stats.original_instr_count, stats.optimized_instr_count,
             stats.reduction_ratio * 100.0f,
             stats.constants_folded, stats.dead_instr_removed, stats.moves_eliminated);
    gtk_label_set_text(GTK_LABEL(g_ui.lbl_cpu_opt_stats), stat_buf);
}

static void on_workload_changed(GtkDropDown *dropdown, GParamSpec *pspec, gpointer user_data) {
    (void)pspec; (void)user_data;
    guint selected = gtk_drop_down_get_selected(dropdown);

    ov_integrated_request_t req;
    memset(&req, 0, sizeof(req));
    if (selected == 0) {
        req.guest_cpu_arch = 1; /* ARM64 */
        req.requested_api = OV_API_METAL;
        req.api_version_major = 2;
        req.api_version_minor = 0;
        req.requires_compute = true;
        req.requires_tessellation = true;
        req.max_texture_dimension = 8192;
        req.required_vram_bytes = 512ULL * 1024 * 1024;
        req.required_ram_bytes = 2ULL * 1024 * 1024 * 1024;
        req.required_cpu_cores = 4;
        strcpy(req.application_name, "Metal Odyssey (Adventure RPG)");
    } else if (selected == 1) {
        req.guest_cpu_arch = 2; /* x86_64 */
        req.requested_api = OV_API_VULKAN;
        req.api_version_major = 1;
        req.api_version_minor = 2;
        req.requires_compute = true;
        req.requires_tessellation = true;
        req.requires_avx2 = true;
        req.max_texture_dimension = 8192;
        req.required_vram_bytes = 1024ULL * 1024 * 1024;
        req.required_ram_bytes = 4ULL * 1024 * 1024 * 1024;
        req.required_cpu_cores = 4;
        strcpy(req.application_name, "CyberVulkan 2077 (Sci-Fi Shooter)");
    } else if (selected == 2) {
        req.guest_cpu_arch = 1; /* ARM64 */
        req.requested_api = OV_API_OPENGL;
        req.api_version_major = 3;
        req.api_version_minor = 3;
        req.requires_compute = false;
        req.max_texture_dimension = 2048;
        req.required_vram_bytes = 256ULL * 1024 * 1024;
        req.required_ram_bytes = 2ULL * 1024 * 1024 * 1024;
        req.required_cpu_cores = 4;
        strcpy(req.application_name, "ARM64 Scientific Matrix Kernel");
    } else {
        req.guest_cpu_arch = 3; /* x86_32 */
        req.requested_api = OV_API_DIRECTX;
        req.api_version_major = 9;
        req.api_version_minor = 0;
        req.max_texture_dimension = 4096;
        req.required_vram_bytes = 512ULL * 1024 * 1024;
        req.required_ram_bytes = 1ULL * 1024 * 1024 * 1024;
        req.required_cpu_cores = 2;
        strcpy(req.application_name, "DirectX Classic Flight Simulator");
    }

    ov_resolution_result_t res;
    ov_resolver_evaluate_integrated(&req, &res);

    gtk_label_set_text(GTK_LABEL(g_ui.lbl_resolve_decision), ov_resolver_decision_to_string(res.decision));
    gtk_label_set_text(GTK_LABEL(g_ui.lbl_resolve_reason), res.rationale);

    char ovh[512];
    snprintf(ovh, sizeof(ovh), "Performance Factor: %u%% | CPU: %s | GPU: %s",
             res.performance_cost_factor, res.cpu_path, res.gpu_path);
    gtk_label_set_text(GTK_LABEL(g_ui.lbl_resolve_overhead), ovh);
}

static void on_compat_app_changed(GtkDropDown *dropdown, GParamSpec *pspec, gpointer user_data) {
    (void)pspec; (void)user_data;
    guint selected = gtk_drop_down_get_selected(dropdown);
    const ov_compat_record_t *app = ov_compatibility_get_app(selected);
    if (!app) return;

    ov_compat_eval_t eval;
    ov_compatibility_evaluate(app, &eval);

    char det[1024];
    snprintf(det, sizeof(det),
             "Application: %s\n"
             "Target OS: %s (%u-bit)\n"
             "API Required: %s %u.%u\n"
             "Execution Plan: %s\n"
             "CPU Translation: %s | GPU Translation: %s\n"
             "Texture Clamping: %s (%s)\n"
             "Known Notes: %s",
             app->app_name, app->target_os, app->bitness,
             app->required_api == OV_API_METAL ? "Metal" :
             app->required_api == OV_API_VULKAN ? "Vulkan" :
             app->required_api == OV_API_DIRECTX ? "DirectX" : "OpenGL",
             app->api_version_major, app->api_version_minor,
             eval.execution_plan,
             eval.cpu_translation_required ? "YES" : "NO",
             eval.gpu_translation_required ? "YES" : "NO",
             eval.texture_clamp_applied ? "APPLIED" : "NONE",
             eval.clamped_features,
             app->known_limitations);

    gtk_label_set_text(GTK_LABEL(g_ui.lbl_compat_details), det);
}

/* Update all views with latest core state */
static void update_all_ui_views(void) {
    const ov_cpu_info_t *cpu = ov_hardware_get_cpu();
    const ov_gpu_info_t *gpu = ov_hardware_get_gpu();
    const ov_memory_info_t *mem = ov_hardware_get_memory();

    /* 1. Dashboard */
    char buf[2048];
    snprintf(buf, sizeof(buf), "Health Score: %u / 100 [OPTIMAL]",
             ov_core_instance.diagnostic_report.system_health_score);
    gtk_label_set_text(GTK_LABEL(g_ui.lbl_health_score), buf);

    snprintf(buf, sizeof(buf), "%s (%s)",
             ov_boot_phase_to_string(ov_core_instance.current_phase),
             ov_core_instance.phase_message);
    gtk_label_set_text(GTK_LABEL(g_ui.lbl_phase_status), buf);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(g_ui.prog_boot),
                                  (double)ov_core_instance.phase_progress_percent / 100.0);

    snprintf(buf, sizeof(buf), "CPU: %s (%uC/%uT)", cpu->model_name, cpu->cores, cpu->threads);
    gtk_label_set_text(GTK_LABEL(g_ui.lbl_dash_cpu), buf);

    snprintf(buf, sizeof(buf), "GPU: %s (%lu MB VRAM)", gpu->model_name, (unsigned long)(gpu->vram_bytes / (1024 * 1024)));
    gtk_label_set_text(GTK_LABEL(g_ui.lbl_dash_gpu), buf);

    snprintf(buf, sizeof(buf), "RAM: %lu MB Total | %lu MB Free",
             (unsigned long)(mem->total_bytes / (1024 * 1024)), (unsigned long)(mem->available_bytes / (1024 * 1024)));
    gtk_label_set_text(GTK_LABEL(g_ui.lbl_dash_ram), buf);

    ov_unified_cache_stats_t cstats;
    ov_unified_cache_get_stats(&cstats);
    snprintf(buf, sizeof(buf), "Cache: Gen %u | Hit Rate: %u%% (%lu queries)",
             cstats.current_generation, cstats.overall_hit_rate_percent, (unsigned long)cstats.total_queries);
    gtk_label_set_text(GTK_LABEL(g_ui.lbl_dash_cache), buf);

    /* 2. Resource & Cache Tab */
    ov_resource_status_t rstat;
    ov_resource_manager_get_status(&rstat);
    gtk_label_set_text(GTK_LABEL(g_ui.lbl_active_profile), ov_resource_profile_to_string(rstat.active_profile));

    snprintf(buf, sizeof(buf), "Allocated Worker Threads: %u / %u",
             rstat.allocated_worker_threads, rstat.caps.max_worker_threads);
    gtk_label_set_text(GTK_LABEL(g_ui.lbl_threads_allocated), buf);

    snprintf(buf, sizeof(buf), "Committed VRAM: %lu MB / %lu MB",
             (unsigned long)(rstat.committed_vram_bytes / (1024 * 1024)), (unsigned long)(rstat.caps.max_vram_budget_bytes / (1024 * 1024)));
    gtk_label_set_text(GTK_LABEL(g_ui.lbl_vram_budget), buf);

    snprintf(buf, sizeof(buf), "Current Generation: %u", cstats.current_generation);
    gtk_label_set_text(GTK_LABEL(g_ui.lbl_cache_gen), buf);

    snprintf(buf, sizeof(buf), "Overall Hit Rate: %u%% (Hits: %lu, Misses: %lu)",
             cstats.overall_hit_rate_percent, (unsigned long)cstats.total_hits, (unsigned long)cstats.total_misses);
    gtk_label_set_text(GTK_LABEL(g_ui.lbl_cache_hit_rate), buf);

    snprintf(buf, sizeof(buf),
             "Tiers Breakdown:\n"
             "• CPU Translation JIT:  %u entries (%u KB) | %lu hits / %lu misses\n"
             "• GPU Shader Cache:     %u entries (%u KB) | %lu hits / %lu misses\n"
             "• GPU Pipeline Cache:   %u entries (%u KB) | %lu hits / %lu misses\n"
             "• Compatibility Cache:  %u entries        | %lu hits / %lu misses",
             cstats.cpu_cache_entries, (uint32_t)(cstats.cpu_cache_bytes / 1024),
             (unsigned long)cstats.cpu_cache_hits, (unsigned long)cstats.cpu_cache_misses,
             cstats.shader_cache_entries, (uint32_t)(cstats.shader_cache_bytes / 1024),
             (unsigned long)cstats.shader_cache_hits, (unsigned long)cstats.shader_cache_misses,
             cstats.pipeline_cache_entries, (uint32_t)(cstats.pipeline_cache_bytes / 1024),
             (unsigned long)cstats.pipeline_cache_hits, (unsigned long)cstats.pipeline_cache_misses,
             cstats.compat_cache_entries,
             (unsigned long)cstats.compat_cache_hits, (unsigned long)cstats.compat_cache_misses);
    gtk_label_set_text(GTK_LABEL(g_ui.lbl_cache_summary), buf);

    /* 3. Benchmark Tab */
    char bench_buf[4096];
    ov_benchmark_format_summary(&ov_core_instance.benchmark_results, bench_buf, sizeof(bench_buf));
    gtk_text_buffer_set_text(g_ui.txt_bench_results, bench_buf, -1);

    /* 4. Diagnostics Tab */
    char diag_buf[4096];
    ov_diagnostics_format_text(&ov_core_instance.diagnostic_report, diag_buf, sizeof(diag_buf));
    gtk_text_buffer_set_text(g_ui.txt_diag_report, diag_buf, -1);

    /* 5. Logs */
    char log_buf[8192];
    ov_logger_get_recent_logs(log_buf, sizeof(log_buf));
    gtk_text_buffer_set_text(g_ui.txt_logs, log_buf, -1);
}

/* UI Tab Builders */
static GtkWidget* build_tab_dashboard(void) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);

    /* Health & Phase Header */
    GtkWidget *header_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
    g_ui.lbl_health_score = gtk_label_new("Health Score: 95 / 100 [OPTIMAL]");
    gtk_widget_add_css_class(g_ui.lbl_health_score, "title-2");
    gtk_box_append(GTK_BOX(header_box), g_ui.lbl_health_score);

    GtkWidget *btn_rerun = gtk_button_new_with_label("Re-Run Pre-Boot Simulation");
    gtk_widget_add_css_class(btn_rerun, "suggested-action");
    g_signal_connect(btn_rerun, "clicked", G_CALLBACK(on_btn_rerun_boot_clicked), NULL);
    gtk_box_append(GTK_BOX(header_box), btn_rerun);
    gtk_box_append(GTK_BOX(box), header_box);

    /* Progress bar */
    g_ui.lbl_phase_status = gtk_label_new("Phase 5: Ready");
    gtk_widget_set_halign(g_ui.lbl_phase_status, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), g_ui.lbl_phase_status);

    g_ui.prog_boot = gtk_progress_bar_new();
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(g_ui.prog_boot), 1.0);
    gtk_box_append(GTK_BOX(box), g_ui.prog_boot);

    /* Metrics Grid */
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 24);

    g_ui.lbl_dash_cpu = gtk_label_new("CPU: Loading...");
    g_ui.lbl_dash_gpu = gtk_label_new("GPU: Loading...");
    g_ui.lbl_dash_ram = gtk_label_new("RAM: Loading...");
    g_ui.lbl_dash_cache = gtk_label_new("Cache: Loading...");

    gtk_widget_set_halign(g_ui.lbl_dash_cpu, GTK_ALIGN_START);
    gtk_widget_set_halign(g_ui.lbl_dash_gpu, GTK_ALIGN_START);
    gtk_widget_set_halign(g_ui.lbl_dash_ram, GTK_ALIGN_START);
    gtk_widget_set_halign(g_ui.lbl_dash_cache, GTK_ALIGN_START);

    gtk_grid_attach(GTK_GRID(grid), g_ui.lbl_dash_cpu, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), g_ui.lbl_dash_gpu, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), g_ui.lbl_dash_ram, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), g_ui.lbl_dash_cache, 1, 1, 1, 1);
    gtk_box_append(GTK_BOX(box), grid);

    /* Action buttons */
    GtkWidget *act_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *b_bench = gtk_button_new_with_label("Run Benchmarks");
    g_signal_connect(b_bench, "clicked", G_CALLBACK(on_btn_run_benchmarks_clicked), NULL);
    gtk_box_append(GTK_BOX(act_box), b_bench);

    GtkWidget *b_inv = gtk_button_new_with_label("Invalidate Cache");
    g_signal_connect(b_inv, "clicked", G_CALLBACK(on_btn_invalidate_cache_clicked), NULL);
    gtk_box_append(GTK_BOX(act_box), b_inv);

    GtkWidget *b_exp = gtk_button_new_with_label("Export Diagnostics (HTML/JSON/TXT)");
    g_signal_connect(b_exp, "clicked", G_CALLBACK(on_btn_export_report_clicked), NULL);
    gtk_box_append(GTK_BOX(act_box), b_exp);

    gtk_box_append(GTK_BOX(box), act_box);

    return box;
}

static GtkWidget* build_tab_hardware(void) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);

    const ov_cpu_info_t *cpu = ov_hardware_get_cpu();
    const ov_gpu_info_t *gpu = ov_hardware_get_gpu();

    char buf[1024];
    snprintf(buf, sizeof(buf),
             "PROCESSOR (CPU):\n"
             "• Model: %s\n"
             "• Cores / Threads: %u Physical / %u Logical\n"
             "• Base Frequency: %u MHz\n"
             "• Instruction Sets: SSE4.2 [%s], AVX [%s], AVX2 [%s], AES-NI [%s]\n\n"
             "GRAPHICS ACCELERATOR (GPU):\n"
             "• Model: %s (PCI ID: 0x%04x:0x%04x)\n"
             "• Dedicated VRAM: %lu MB\n"
             "• Max 2D/3D Texture Dimension: %upx\n"
             "• Hardware Compute: %s | Tessellation: %s\n"
             "• Native APIs: OpenGL 4.0 Core [%s], Vulkan [%s], Metal [%s]\n\n"
             "PCI BUS TREE:\n"
             "• Bus 00:02.0 - Display Controller (Intel HD Graphics Gen7)\n"
             "• Bus 00:1f.2 - SATA Controller (AHCI)\n"
             "• Bus 00:19.0 - Gigabit Ethernet Controller\n"
             "• Bus 00:1b.0 - High Definition Audio Controller",
             cpu->model_name, cpu->cores, cpu->threads, cpu->base_freq_mhz,
             cpu->has_sse42 ? "Yes" : "No", cpu->has_avx ? "Yes" : "No",
             cpu->has_avx2 ? "Yes" : "No", cpu->has_aesni ? "Yes" : "No",
             gpu->model_name, gpu->vendor_id, gpu->device_id,
             gpu->vram_bytes / (1024 * 1024), gpu->max_texture_dimension,
             gpu->supports_compute ? "Supported" : "Software Fallback",
             gpu->supports_tessellation ? "Supported" : "Emulated",
             gpu->supports_opengl_core ? "Native" : "No",
             gpu->supports_vulkan ? "Native" : "OVIR-GPU Trans",
             gpu->supports_metal ? "Native" : "OVIR-GPU Trans");

    GtkWidget *lbl = gtk_label_new(buf);
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), lbl);

    return box;
}

static GtkWidget* build_tab_resolver(void) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);

    GtkWidget *title = gtk_label_new("Unified Workload Resolver (Phase 3 & 4)");
    gtk_widget_add_css_class(title, "title-3");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), title);

    /* Workload Dropdown */
    const char *items[] = {
        "Metal Odyssey 2.0 (ARM64 + Metal MSL)",
        "CyberVulkan 2077 (x86_64 + Vulkan SPIR-V)",
        "ARM64 Scientific Matrix Kernel (Compute SIMD)",
        "DirectX Classic Flight Simulator (D3D9 Legacy)",
        NULL
    };
    g_ui.combo_workload = gtk_drop_down_new_from_strings(items);
    g_signal_connect(g_ui.combo_workload, "notify::selected", G_CALLBACK(on_workload_changed), NULL);
    gtk_box_append(GTK_BOX(box), g_ui.combo_workload);

    /* Resolution Display Card */
    g_ui.lbl_resolve_decision = gtk_label_new("Decision: OVIR Dual Translation (CPU+GPU)");
    gtk_widget_add_css_class(g_ui.lbl_resolve_decision, "title-4");
    gtk_widget_set_halign(g_ui.lbl_resolve_decision, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), g_ui.lbl_resolve_decision);

    g_ui.lbl_resolve_reason = gtk_label_new("Reason: Metal MSL 2.0 shaders translated to OpenGL Core 4.0; ARM64 JIT translated to x86_64.");
    gtk_widget_set_halign(g_ui.lbl_resolve_reason, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), g_ui.lbl_resolve_reason);

    g_ui.lbl_resolve_overhead = gtk_label_new("Performance Factor: 125% | CPU: OVIR-CPU JIT | GPU: OVIR-GPU Trans");
    gtk_widget_set_halign(g_ui.lbl_resolve_overhead, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), g_ui.lbl_resolve_overhead);

    return box;
}

static GtkWidget* build_tab_ovir(void) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 16);
    gtk_widget_set_margin_end(box, 16);
    gtk_widget_set_margin_top(box, 16);
    gtk_widget_set_margin_bottom(box, 16);

    /* Optimization Toggles */
    GtkWidget *opt_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    g_ui.chk_opt_fold = gtk_check_button_new_with_label("Constant Folding");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(g_ui.chk_opt_fold), TRUE);
    gtk_box_append(GTK_BOX(opt_box), g_ui.chk_opt_fold);

    g_ui.chk_opt_dce = gtk_check_button_new_with_label("Dead Code Elimination");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(g_ui.chk_opt_dce), TRUE);
    gtk_box_append(GTK_BOX(opt_box), g_ui.chk_opt_dce);

    g_ui.chk_opt_move = gtk_check_button_new_with_label("Move Elimination");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(g_ui.chk_opt_move), TRUE);
    gtk_box_append(GTK_BOX(opt_box), g_ui.chk_opt_move);

    GtkWidget *btn_opt = gtk_button_new_with_label("Run OVIR-CPU Optimizer");
    g_signal_connect(btn_opt, "clicked", G_CALLBACK(on_btn_optimize_cpu_clicked), NULL);
    gtk_box_append(GTK_BOX(opt_box), btn_opt);
    gtk_box_append(GTK_BOX(box), opt_box);

    g_ui.lbl_cpu_opt_stats = gtk_label_new("Optimizer: Ready");
    gtk_widget_set_halign(g_ui.lbl_cpu_opt_stats, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), g_ui.lbl_cpu_opt_stats);

    /* Text view for CPU IR */
    GtkWidget *sw = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(sw, TRUE);
    GtkWidget *tv = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(tv), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(tv), TRUE);
    g_ui.txt_cpu_ir = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(sw), tv);
    gtk_box_append(GTK_BOX(box), sw);

    /* Initial population */
    ov_cpu_program_t prog;
    ov_cpu_build_sample_program(0, &prog);
    char ir_buf[4096];
    ov_cpu_format_program_ir(&prog, ir_buf, sizeof(ir_buf));
    gtk_text_buffer_set_text(g_ui.txt_cpu_ir, ir_buf, -1);

    return box;
}

static GtkWidget* build_tab_resource_cache(void) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);

    /* Profiles buttons */
    GtkWidget *p_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *b1 = gtk_button_new_with_label("Balanced");
    g_signal_connect(b1, "clicked", G_CALLBACK(on_btn_profile_clicked), GINT_TO_POINTER(OV_PROFILE_BALANCED));
    gtk_box_append(GTK_BOX(p_box), b1);

    GtkWidget *b2 = gtk_button_new_with_label("High Performance");
    g_signal_connect(b2, "clicked", G_CALLBACK(on_btn_profile_clicked), GINT_TO_POINTER(OV_PROFILE_PERFORMANCE));
    gtk_box_append(GTK_BOX(p_box), b2);

    GtkWidget *b3 = gtk_button_new_with_label("Max Performance");
    g_signal_connect(b3, "clicked", G_CALLBACK(on_btn_profile_clicked), GINT_TO_POINTER(OV_PROFILE_MAX_PERFORMANCE));
    gtk_box_append(GTK_BOX(p_box), b3);

    GtkWidget *b4 = gtk_button_new_with_label("Low Power / Battery");
    g_signal_connect(b4, "clicked", G_CALLBACK(on_btn_profile_clicked), GINT_TO_POINTER(OV_PROFILE_BATTERY_LOW_POWER));
    gtk_box_append(GTK_BOX(p_box), b4);
    gtk_box_append(GTK_BOX(box), p_box);

    /* Resource labels */
    g_ui.lbl_active_profile = gtk_label_new("Profile: Balanced");
    g_ui.lbl_threads_allocated = gtk_label_new("Allocated Threads: 7 / 8");
    g_ui.lbl_vram_budget = gtk_label_new("Committed VRAM: 1024 MB");
    gtk_widget_set_halign(g_ui.lbl_active_profile, GTK_ALIGN_START);
    gtk_widget_set_halign(g_ui.lbl_threads_allocated, GTK_ALIGN_START);
    gtk_widget_set_halign(g_ui.lbl_vram_budget, GTK_ALIGN_START);

    gtk_box_append(GTK_BOX(box), g_ui.lbl_active_profile);
    gtk_box_append(GTK_BOX(box), g_ui.lbl_threads_allocated);
    gtk_box_append(GTK_BOX(box), g_ui.lbl_vram_budget);

    /* Separator */
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_append(GTK_BOX(box), sep);

    /* Unified Cache labels */
    g_ui.lbl_cache_gen = gtk_label_new("Cache Generation: 1");
    g_ui.lbl_cache_hit_rate = gtk_label_new("Hit Rate: 92%");
    g_ui.lbl_cache_summary = gtk_label_new("Cache Summary Loading...");
    gtk_widget_set_halign(g_ui.lbl_cache_gen, GTK_ALIGN_START);
    gtk_widget_set_halign(g_ui.lbl_cache_hit_rate, GTK_ALIGN_START);
    gtk_widget_set_halign(g_ui.lbl_cache_summary, GTK_ALIGN_START);

    gtk_box_append(GTK_BOX(box), g_ui.lbl_cache_gen);
    gtk_box_append(GTK_BOX(box), g_ui.lbl_cache_hit_rate);
    gtk_box_append(GTK_BOX(box), g_ui.lbl_cache_summary);

    return box;
}

static GtkWidget* build_tab_compat(void) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);

    const char *apps[] = {
        "Metal Odyssey (Adventure RPG)",
        "CyberVulkan 2077 (Sci-Fi Shooter)",
        "DirectX Classic Flight Simulator",
        "ARM64 Scientific Matrix Kernel",
        "AVX2 Heavy Video Filter",
        "Retro 3D Platformer (OpenGL 3.3)",
        NULL
    };
    g_ui.combo_apps = gtk_drop_down_new_from_strings(apps);
    g_signal_connect(g_ui.combo_apps, "notify::selected", G_CALLBACK(on_compat_app_changed), NULL);
    gtk_box_append(GTK_BOX(box), g_ui.combo_apps);

    g_ui.lbl_compat_details = gtk_label_new("Compatibility Details Loading...");
    gtk_widget_set_halign(g_ui.lbl_compat_details, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), g_ui.lbl_compat_details);

    /* Initial trigger */
    on_compat_app_changed(GTK_DROP_DOWN(g_ui.combo_apps), NULL, NULL);

    return box;
}

static GtkWidget* build_tab_benchmark(void) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 16);
    gtk_widget_set_margin_end(box, 16);
    gtk_widget_set_margin_top(box, 16);
    gtk_widget_set_margin_bottom(box, 16);

    GtkWidget *btn = gtk_button_new_with_label("Run Benchmark Suite");
    g_signal_connect(btn, "clicked", G_CALLBACK(on_btn_run_benchmarks_clicked), NULL);
    gtk_box_append(GTK_BOX(box), btn);

    GtkWidget *sw = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(sw, TRUE);
    GtkWidget *tv = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(tv), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(tv), TRUE);
    g_ui.txt_bench_results = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(sw), tv);
    gtk_box_append(GTK_BOX(box), sw);

    return box;
}

static GtkWidget* build_tab_diagnostics(void) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 16);
    gtk_widget_set_margin_end(box, 16);
    gtk_widget_set_margin_top(box, 16);
    gtk_widget_set_margin_bottom(box, 16);

    GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *b1 = gtk_button_new_with_label("Export Diagnostics (HTML / JSON / Text)");
    gtk_widget_add_css_class(b1, "suggested-action");
    g_signal_connect(b1, "clicked", G_CALLBACK(on_btn_export_report_clicked), NULL);
    gtk_box_append(GTK_BOX(btn_box), b1);
    gtk_box_append(GTK_BOX(box), btn_box);

    GtkWidget *sw = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(sw, TRUE);
    GtkWidget *tv = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(tv), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(tv), TRUE);
    g_ui.txt_diag_report = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(sw), tv);
    gtk_box_append(GTK_BOX(box), sw);

    return box;
}

static GtkWidget* build_tab_logs(void) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 16);
    gtk_widget_set_margin_end(box, 16);
    gtk_widget_set_margin_top(box, 16);
    gtk_widget_set_margin_bottom(box, 16);

    GtkWidget *sw = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(sw, TRUE);
    GtkWidget *tv = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(tv), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(tv), TRUE);
    g_ui.txt_logs = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(sw), tv);
    gtk_box_append(GTK_BOX(box), sw);

    return box;
}

/* GTK App Activation */
static void on_app_activate(GtkApplication *app, gpointer user_data) {
    (void)user_data;

    /* Execute pre-boot simulation if not already executed */
    if (!ov_core_instance.boot_successful) {
        ov_core_execute_preboot();
    }

    g_ui.app = app;
    g_ui.window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(g_ui.window), "OpenVintage Pre-Boot Architecture Simulator (Phases 1-5)");
    gtk_window_set_default_size(GTK_WINDOW(g_ui.window), 1080, 720);

    /* Main Notebook with 8 specialized tabs */
    g_ui.notebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(g_ui.notebook), GTK_POS_TOP);

    gtk_notebook_append_page(GTK_NOTEBOOK(g_ui.notebook), build_tab_dashboard(), gtk_label_new("Dashboard"));
    gtk_notebook_append_page(GTK_NOTEBOOK(g_ui.notebook), build_tab_hardware(), gtk_label_new("Hardware Discovery"));
    gtk_notebook_append_page(GTK_NOTEBOOK(g_ui.notebook), build_tab_resolver(), gtk_label_new("Workload Resolver"));
    gtk_notebook_append_page(GTK_NOTEBOOK(g_ui.notebook), build_tab_ovir(), gtk_label_new("OVIR Engine"));
    gtk_notebook_append_page(GTK_NOTEBOOK(g_ui.notebook), build_tab_resource_cache(), gtk_label_new("Resource & Cache"));
    gtk_notebook_append_page(GTK_NOTEBOOK(g_ui.notebook), build_tab_compat(), gtk_label_new("Compatibility & Quirks"));
    gtk_notebook_append_page(GTK_NOTEBOOK(g_ui.notebook), build_tab_benchmark(), gtk_label_new("Benchmarks"));
    gtk_notebook_append_page(GTK_NOTEBOOK(g_ui.notebook), build_tab_diagnostics(), gtk_label_new("Diagnostics Audit"));
    gtk_notebook_append_page(GTK_NOTEBOOK(g_ui.notebook), build_tab_logs(), gtk_label_new("Live Logs"));

    gtk_window_set_child(GTK_WINDOW(g_ui.window), g_ui.notebook);

    update_all_ui_views();
    gtk_window_present(GTK_WINDOW(g_ui.window));
}

int ov_ui_run(int argc, char *argv[]) {
    GtkApplication *app = gtk_application_new("org.openvintage.preboot.simulator", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_app_activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}

#else

int ov_ui_run(int argc, char *argv[]) {
    (void)argc; (void)argv;
    ov_log_warn("GTK4 support was not enabled at compile time (OV_ENABLE_GTK undefined)");
    return 0;
}

bool ov_ui_is_display_available(void) {
    return false;
}

#endif /* OV_ENABLE_GTK */
