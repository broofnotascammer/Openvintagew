/**
 * OpenVintage Pre-Boot Simulator - Main Entry Point
 */

#include "ov_core.h"
#include "ov_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char *prog_name) {
    printf("OpenVintage Pre-Boot Architecture Simulator (Phases 1-5)\n\n");
    printf("Usage: %s [options]\n\n", prog_name);
    printf("Options:\n");
    printf("  -c, --cli               Run in CLI mode without launching GUI\n");
    printf("  -g, --gui               Force GUI mode (requires DISPLAY/Wayland)\n");
    printf("  -b, --bench             Execute and print Phase 5 benchmark suite\n");
    printf("  -r, --report <file>     Export text diagnostic report to <file>\n");
    printf("  --html <file>           Export HTML diagnostic report to <file>\n");
    printf("  --json <file>           Export JSON diagnostic report to <file>\n");
    printf("  --profile <profile>     Resource profile: balanced, perf, max, lowpower\n");
    printf("  -h, --help              Show this help message\n");
}

int main(int argc, char *argv[]) {
    bool force_cli = false;
    bool force_gui = false;
    bool run_bench = false;
    const char *report_txt = NULL;
    const char *report_html = NULL;
    const char *report_json = NULL;
    const char *profile_arg = NULL;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-c") || !strcmp(argv[i], "--cli")) {
            force_cli = true;
        } else if (!strcmp(argv[i], "-g") || !strcmp(argv[i], "--gui")) {
            force_gui = true;
        } else if (!strcmp(argv[i], "-b") || !strcmp(argv[i], "--bench")) {
            run_bench = true;
        } else if ((!strcmp(argv[i], "-r") || !strcmp(argv[i], "--report")) && i + 1 < argc) {
            report_txt = argv[++i];
        } else if (!strcmp(argv[i], "--html") && i + 1 < argc) {
            report_html = argv[++i];
        } else if (!strcmp(argv[i], "--json") && i + 1 < argc) {
            report_json = argv[++i];
        } else if (!strcmp(argv[i], "--profile") && i + 1 < argc) {
            profile_arg = argv[++i];
        } else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            print_usage(argv[0]);
            return 0;
        }
    }

    /* 1. Initialize Simulator Core */
    ov_status_t status = ov_core_init();
    if (status != OV_SUCCESS) {
        fprintf(stderr, "Fatal: Core initialization failed: %s\n", ov_status_to_string(status));
        return 1;
    }

    /* 2. Execute Full Pre-Boot Sequence (Phases 1-5) */
    status = ov_core_execute_preboot();
    if (status != OV_SUCCESS) {
        fprintf(stderr, "Fatal: Pre-boot sequence execution failed: %s\n", ov_status_to_string(status));
        ov_core_cleanup();
        return 1;
    }

    /* Apply custom resource profile if specified */
    if (profile_arg) {
        if (!strcasecmp(profile_arg, "perf") || !strcasecmp(profile_arg, "performance")) {
            ov_resource_manager_set_profile(OV_PROFILE_PERFORMANCE);
        } else if (!strcasecmp(profile_arg, "max")) {
            ov_resource_manager_set_profile(OV_PROFILE_MAX_PERFORMANCE);
        } else if (!strcasecmp(profile_arg, "lowpower") || !strcasecmp(profile_arg, "battery")) {
            ov_resource_manager_set_profile(OV_PROFILE_BATTERY_LOW_POWER);
        } else {
            ov_resource_manager_set_profile(OV_PROFILE_BALANCED);
        }
    }

    /* 3. Handle File Exports */
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

    /* 4. Determine UI Mode */
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
        /* CLI Mode Output */
        char text_summary[4096];
        ov_diagnostics_format_text(&ov_core_instance.diagnostic_report, text_summary, sizeof(text_summary));
        printf("\n%s\n", text_summary);

        if (run_bench) {
            char bench_summary[2048];
            ov_benchmark_format_summary(&ov_core_instance.benchmark_results, bench_summary, sizeof(bench_summary));
            printf("%s\n", bench_summary);
        }
    }

    /* 5. Clean up */
    ov_core_cleanup();
    return result;
}
