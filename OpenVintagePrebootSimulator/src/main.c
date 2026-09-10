/**
 * OpenVintage Pre-Boot Architecture Simulator - Main Entry Point
 * Comprehensive CLI / GUI options for host detection, Mac profile simulation,
 * macOS version compatibility evaluation, and empirical benchmarking.
 */

#include "ov_core.h"
#include "ov_hardware.h"
#include "ov_macos_compat.h"
#include "ov_benchmark.h"
#include "ov_diagnostics.h"
#include "ov_resource_manager.h"
#include "ov_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char *prog_name) {
    printf("================================================================================\n");
    printf("        OpenVintage Pre-Boot Architecture & Mac Compatibility Simulator        \n");
    printf("================================================================================\n\n");
    printf("Usage: %s [options]\n\n", prog_name);
    printf("Simulation & Hardware Profiles:\n");
    printf("  -d, --detect            Safely detect host hardware (read-only, non-invasive)\n");
    printf("  -m, --mac <model>       Select Mac hardware profile (e.g. MBP9,1, MacBookPro9,1,\n");
    printf("                          iMac14,2, MacPro5,1, Macmini6,2, MacBookPro11,5)\n");
    printf("  -l, --list-macs         List all supported Mac hardware profiles\n");
    printf("  -t, --target-os <ver>   Target macOS version for evaluation (e.g. 10.13, 11, 12, 13, 14, 15,\n");
    printf("                          monterey, ventura, sonoma, sequoia)\n");
    printf("  --compat-matrix         Display full macOS version compatibility matrix for profile\n\n");
    printf("Execution & Output Modes:\n");
    printf("  -c, --cli               Run in CLI mode without launching GUI\n");
    printf("  -g, --gui               Force GUI mode (requires DISPLAY / Wayland)\n");
    printf("  -b, --bench             Execute and display real empirical benchmark suite\n");
    printf("  -r, --report <file>     Export text diagnostic report to <file>\n");
    printf("  --html <file>           Export HTML diagnostic report to <file>\n");
    printf("  --json <file>           Export JSON diagnostic report to <file>\n");
    printf("  --profile-res <profile> Resource profile: balanced, perf, max, lowpower\n");
    printf("  -h, --help              Show this help message\n\n");
}

int main(int argc, char *argv[]) {
    bool force_cli = false;
    bool force_gui = false;
    bool run_bench = false;
    bool just_detect = false;
    bool list_macs = false;
    bool show_matrix = false;
    const char *mac_profile_arg = NULL;
    const char *target_os_arg = NULL;
    const char *report_txt = NULL;
    const char *report_html = NULL;
    const char *report_json = NULL;
    const char *resource_arg = NULL;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-c") || !strcmp(argv[i], "--cli")) {
            force_cli = true;
        } else if (!strcmp(argv[i], "-g") || !strcmp(argv[i], "--gui")) {
            force_gui = true;
        } else if (!strcmp(argv[i], "-b") || !strcmp(argv[i], "--bench")) {
            run_bench = true;
        } else if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--detect")) {
            just_detect = true;
            force_cli = true;
        } else if (!strcmp(argv[i], "-l") || !strcmp(argv[i], "--list-macs")) {
            list_macs = true;
            force_cli = true;
        } else if (!strcmp(argv[i], "--compat-matrix")) {
            show_matrix = true;
            force_cli = true;
        } else if ((!strcmp(argv[i], "-m") || !strcmp(argv[i], "--mac") || !strcmp(argv[i], "--profile")) && i + 1 < argc) {
            mac_profile_arg = argv[++i];
        } else if ((!strcmp(argv[i], "-t") || !strcmp(argv[i], "--target-os")) && i + 1 < argc) {
            target_os_arg = argv[++i];
        } else if ((!strcmp(argv[i], "-r") || !strcmp(argv[i], "--report")) && i + 1 < argc) {
            report_txt = argv[++i];
        } else if (!strcmp(argv[i], "--html") && i + 1 < argc) {
            report_html = argv[++i];
        } else if (!strcmp(argv[i], "--json") && i + 1 < argc) {
            report_json = argv[++i];
        } else if (!strcmp(argv[i], "--profile-res") && i + 1 < argc) {
            resource_arg = argv[++i];
        } else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            print_usage(argv[0]);
            return 0;
        }
    }

    /* Handle quick informational commands */
    if (list_macs) {
        printf("\nOpenVintage Supported Mac Hardware Profiles:\n");
        printf("--------------------------------------------------------------------------------\n");
        printf(" %-16s | %-32s | %-20s\n", "Identifier", "Marketing Name", "CPU Architecture");
        printf("--------------------------------------------------------------------------------\n");
        uint32_t count = ov_hardware_get_profile_count();
        for (uint32_t i = 1; i < count; i++) {
            ov_hardware_profile_t p;
            if (ov_hardware_get_profile((ov_hw_profile_id_t)i, &p) == OV_SUCCESS) {
                printf(" %-16s | %-32s | %-20s\n",
                       p.model_identifier, p.marketing_name,
                       ov_cpu_type_to_string(p.cpu.type));
            }
        }
        printf("--------------------------------------------------------------------------------\n\n");
        return 0;
    }

    /* 1. Initialize Simulator Core */
    ov_status_t status = ov_core_init();
    if (status != OV_SUCCESS) {
        fprintf(stderr, "Fatal: Core initialization failed: %s\n", ov_status_to_string(status));
        return 1;
    }

    /* Handle safe host detection display */
    if (just_detect) {
        printf("\n================================================================================\n");
        printf("                  SAFE HOST HARDWARE DETECTION (READ-ONLY)                      \n");
        printf("================================================================================\n");
        const ov_cpu_info_t *c = ov_hardware_get_cpu();
        const ov_gpu_info_t *g = ov_hardware_get_gpu();
        const ov_memory_info_t *m = ov_hardware_get_memory();

        printf("Host CPU:       %s\n", c->model_name);
        printf("Topology:       %u Physical Cores / %u Logical Threads @ %u MHz\n", c->cores, c->threads, c->base_freq_mhz);
        printf("Extensions:     SSE4.2: %s | AVX: %s | AVX2: %s | AES-NI: %s\n",
               c->has_sse42 ? "Yes" : "No", c->has_avx ? "Yes" : "No",
               c->has_avx2 ? "Yes" : "No", c->has_aesni ? "Yes" : "No");
        printf("Host GPU:       %s (PCI 0x%04x:0x%04x)\n", g->model_name, g->vendor_id, g->device_id);
        printf("VRAM:           %llu MB | Max Texture: %upx\n",
               (unsigned long long)(g->vram_bytes / (1024 * 1024)), g->max_texture_dimension);
        printf("Graphics APIs:  OpenGL Core: %s | Vulkan: %s | Metal: %s\n",
               g->supports_opengl_core ? "Native" : "No",
               g->supports_vulkan ? "Native" : "No",
               g->supports_metal ? "Native" : "No");
        printf("System RAM:     %llu MB Total / %llu MB Free\n",
               (unsigned long long)(m->total_bytes / (1024 * 1024)),
               (unsigned long long)(m->available_bytes / (1024 * 1024)));
        printf("================================================================================\n\n");
        ov_core_cleanup();
        return 0;
    }

    /* 2. Switch to requested Mac profile if specified */
    if (mac_profile_arg) {
        status = ov_hardware_set_active_profile_by_name(mac_profile_arg);
        if (status != OV_SUCCESS) {
            fprintf(stderr, "Warning: Could not find Mac profile '%s'. Run --list-macs to see available.\n", mac_profile_arg);
        } else {
            const ov_hardware_profile_t *p = ov_hardware_get_active_profile();
            if (p) {
                printf("Switched simulated hardware profile to: %s (%s)\n",
                       p->model_identifier, p->marketing_name);
            }
        }
    }

    /* 3. Execute Full Pre-Boot Sequence */
    status = ov_core_execute_preboot();
    if (status != OV_SUCCESS) {
        fprintf(stderr, "Fatal: Pre-boot sequence execution failed: %s\n", ov_status_to_string(status));
        ov_core_cleanup();
        return 1;
    }

    /* Apply custom resource profile if specified */
    if (resource_arg) {
        if (!strcasecmp(resource_arg, "perf") || !strcasecmp(resource_arg, "performance")) {
            ov_resource_manager_set_profile(OV_PROFILE_PERFORMANCE);
        } else if (!strcasecmp(resource_arg, "max")) {
            ov_resource_manager_set_profile(OV_PROFILE_MAX_PERFORMANCE);
        } else if (!strcasecmp(resource_arg, "lowpower") || !strcasecmp(resource_arg, "battery")) {
            ov_resource_manager_set_profile(OV_PROFILE_BATTERY_LOW_POWER);
        } else {
            ov_resource_manager_set_profile(OV_PROFILE_BALANCED);
        }
    }

    /* 4. Display Compatibility Matrix if requested */
    if (show_matrix) {
        const ov_hardware_profile_t *p = ov_hardware_get_active_profile();
        if (p) {
            char matrix_buf[4096];
            ov_macos_compat_format_matrix(p, matrix_buf, sizeof(matrix_buf));
            printf("\n%s\n", matrix_buf);
        }
    }

    /* 5. Evaluate specific target macOS if requested */
    if (target_os_arg) {
        ov_macos_version_t target_ver = OV_MACOS_UNKNOWN;
        if (!strcasecmp(target_os_arg, "10.13") || !strcasecmp(target_os_arg, "high_sierra") || !strcasecmp(target_os_arg, "highsierra")) target_ver = OV_MACOS_10_13_HIGH_SIERRA;
        else if (!strcasecmp(target_os_arg, "10.14") || !strcasecmp(target_os_arg, "mojave")) target_ver = OV_MACOS_10_14_MOJAVE;
        else if (!strcasecmp(target_os_arg, "10.15") || !strcasecmp(target_os_arg, "catalina")) target_ver = OV_MACOS_10_15_CATALINA;
        else if (!strcasecmp(target_os_arg, "11") || !strcasecmp(target_os_arg, "big_sur") || !strcasecmp(target_os_arg, "bigsur")) target_ver = OV_MACOS_11_BIG_SUR;
        else if (!strcasecmp(target_os_arg, "12") || !strcasecmp(target_os_arg, "monterey")) target_ver = OV_MACOS_12_MONTEREY;
        else if (!strcasecmp(target_os_arg, "13") || !strcasecmp(target_os_arg, "ventura")) target_ver = OV_MACOS_13_VENTURA;
        else if (!strcasecmp(target_os_arg, "14") || !strcasecmp(target_os_arg, "sonoma")) target_ver = OV_MACOS_14_SONOMA;
        else if (!strcasecmp(target_os_arg, "15") || !strcasecmp(target_os_arg, "sequoia")) target_ver = OV_MACOS_15_SEQUOIA;

        if (target_ver != OV_MACOS_UNKNOWN) {
            const ov_hardware_profile_t *p = ov_hardware_get_active_profile();
            if (p) {
                ov_macos_compat_eval_t cres;
                ov_macos_compat_evaluate(target_ver, p, &cres);
                char cres_buf[1024];
                ov_macos_compat_format_eval(&cres, cres_buf, sizeof(cres_buf));
                printf("\n%s\n", cres_buf);
            }
        } else {
            fprintf(stderr, "Unknown target macOS version: %s\n", target_os_arg);
        }
    }

    /* 6. Handle File Exports */
    if (report_txt) {
        ov_diagnostics_export_text(&ov_core_instance.diagnostic_report, report_txt);
        printf("Diagnostics text report written to: %s\n", report_txt);
    }
    if (report_html) {
        ov_diagnostics_export_html(&ov_core_instance.diagnostic_report, report_html);
        printf("Diagnostics HTML report written to: %s\n", report_html);
    }
    if (report_json) {
        ov_diagnostics_export_json(&ov_core_instance.diagnostic_report, report_json);
        printf("Diagnostics JSON report written to: %s\n", report_json);
    }

    /* 7. Determine UI Mode */
    bool should_launch_gui = false;
    if (force_gui) {
        should_launch_gui = true;
    } else if (!force_cli && ov_ui_is_display_available()) {
        should_launch_gui = true;
    }

    int result = 0;
    if (should_launch_gui) {
        printf("Starting OpenVintage Pre-Boot Simulator GTK4 GUI...\n");
        result = ov_ui_run(argc, argv);
    } else {
        if (!show_matrix && !target_os_arg) {
            char text_summary[4096];
            ov_diagnostics_format_text(&ov_core_instance.diagnostic_report, text_summary, sizeof(text_summary));
            printf("\n%s\n", text_summary);
        }

        if (run_bench) {
            char bench_summary[2048];
            ov_benchmark_format_summary(&ov_core_instance.benchmark_results, bench_summary, sizeof(bench_summary));
            printf("%s\n", bench_summary);
        }
    }

    /* 8. Clean up */
    ov_core_cleanup();
    return result;
}
