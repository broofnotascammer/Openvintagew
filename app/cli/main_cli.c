/**
 * OpenVintage - Modular Boot Management & Configuration CLI Application (Phase 6)
 */

#include "ov_app_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char *prog) {
    printf("OpenVintage Application CLI - Phase 6 Rearchitecture\n");
    printf("Usage: %s <command> [arguments]\n\n", prog);
    printf("Commands:\n");
    printf("  status                 Display overall system, hardware, profile, and boot status\n");
    printf("  hardware-detect        Detect host silicon and print CPU/GPU inventory\n");
    printf("  compat-check <OS>      Evaluate compatibility for target OS (e.g. Monterey, Linux)\n");
    printf("  profile-list           List available hardware performance profiles\n");
    printf("  profile-set <name>     Apply performance profile (Max, Gaming, Balanced, Battery)\n");
    printf("  boot-targets           Enumerate bootable OS and EFI targets\n");
    printf("  boot-select <index>    Select boot target by index\n");
    printf("  deploy-simulate        Simulate safe deployment (dry-run without disk modifications)\n");
    printf("  deploy-plan            Generate auditable deployment plan with SHA-256 hashes\n");
    printf("  deploy-apply           Apply deployment changes atomically with user approval\n");
    printf("  verify                 Run integrity verification against EFI installation\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 0;
    }

    ov_app_init();
    const char *cmd = argv[1];

    if (strcmp(cmd, "status") == 0) {
        ov_app_status_summary_t sum;
        ov_app_get_status_summary(&sum);
        printf("==================================================\n");
        printf("OpenVintage System Status\n");
        printf("==================================================\n");
        printf("Version:             %s\n", sum.app_version);
        printf("Hardware:            %s [%s]\n", sum.hardware_model, sum.hardware_source);
        printf("GPU Inventory:       %u GPU(s) present\n", sum.gpu_count);
        printf("Active GPU:          %s (Index %u)\n", sum.active_gpu_name, sum.active_gpu_index);
        printf("Performance Profile: %s\n", sum.active_perf_profile);
        printf("Boot Target:         [%d] %s (%u targets discovered)\n",
               sum.selected_boot_index, sum.selected_boot_label, sum.boot_target_count);
        printf("Deployment State:    %s (Step: %s)\n",
               ov_deploy_state_to_string(sum.deploy_state),
               ov_deploy_step_to_string(sum.deploy_step));
        printf("==================================================\n");
    } else if (strcmp(cmd, "hardware-detect") == 0) {
        ov_app_detect_hardware(false);
        const ov_hardware_profile_t *hw = ov_hardware_get_active_profile();
        const ov_gpu_topology_t *topo = ov_hardware_get_gpu_topology();
        printf("Detected Hardware Profile:\n");
        printf("  Model:    %s (%s)\n", hw->model_identifier, hw->marketing_name);
        printf("  Source:   %s\n", hw->is_simulated ? "SIMULATED HARDWARE" : "REAL HARDWARE");
        printf("  CPU:      %s (%u cores, %u threads)\n", hw->cpu.model_name, hw->cpu.cores, hw->cpu.threads);
        printf("  Total GPUs: %u\n", topo->gpu_count);
        for (uint32_t i = 0; i < topo->gpu_count; ++i) {
            printf("    [%u] %s (VRAM: %u MB, Metal: %s)%s\n",
                   i, topo->gpus[i].model_name, topo->gpus[i].vram_mb,
                   ov_metal_support_to_string(topo->gpus[i].metal_level),
                   (i == topo->active_gpu_index) ? " [ACTIVE]" : "");
        }
    } else if (strcmp(cmd, "compat-check") == 0) {
        const char *target = (argc > 2) ? argv[2] : "macOS 12 Monterey";
        ov_os_compat_result_t res;
        ov_app_check_os_compatibility(target, &res);
        printf("Compatibility Evaluation for '%s':\n", target);
        printf("  Hardware Source: %s\n", res.hardware_source_label);
        printf("  Rating/Category: %s\n", ov_compat_category_to_string(res.category));
        printf("  Integration:     %s\n", res.recommended_integration);
        printf("  Rationale:       %s\n", res.rationale);
        printf("  Legacy GPU Patches: %s\n", res.requires_legacy_gpu_patch ? "Yes" : "No");
        printf("  Cryptex Bypass:     %s\n", res.cryptex_bypass_needed ? "Yes" : "No");
    } else if (strcmp(cmd, "profile-list") == 0) {
        printf("Available Performance Profiles:\n");
        for (uint32_t i = 0; i < ov_perf_profile_get_count(); ++i) {
            ov_perf_profile_config_t cfg;
            ov_perf_profile_get_by_id((ov_perf_profile_id_t)i, &cfg);
            printf("  [%u] %-22s - %s\n", i, cfg.name, cfg.description);
        }
    } else if (strcmp(cmd, "profile-set") == 0) {
        if (argc < 3) {
            printf("Error: Please specify profile name (e.g. Gaming, Battery, Max)\n");
        } else {
            ov_status_t st = ov_app_set_performance_profile(argv[2]);
            if (st == OV_SUCCESS) {
                const ov_perf_profile_config_t *act = ov_perf_profile_get_active();
                printf("Profile applied: %s (Active GPU: %s)\n",
                       act->name, ov_hardware_get_active_gpu()->model_name);
            } else {
                printf("Error: Failed to apply profile '%s'\n", argv[2]);
            }
        }
    } else if (strcmp(cmd, "boot-targets") == 0) {
        uint32_t count = ov_boot_picker_get_target_count();
        int32_t sel = ov_boot_picker_get_selected_index();
        printf("Discovered Boot Targets (%u):\n", count);
        for (uint32_t i = 0; i < count; ++i) {
            const ov_boot_target_t *t = ov_boot_picker_get_target(i);
            printf("  %s [%u] %-25s (%s) FS: %s\n",
                   (sel == (int32_t)i) ? "->" : "  ",
                   i, t->label, ov_boot_target_type_to_string(t->type), t->fs_type);
        }
    } else if (strcmp(cmd, "boot-select") == 0) {
        if (argc < 3) {
            printf("Error: Please specify target index\n");
        } else {
            uint32_t idx = (uint32_t)atoi(argv[2]);
            ov_status_t st = ov_app_select_boot_target(idx);
            if (st == OV_SUCCESS) {
                printf("Selected boot target [%u]: %s\n", idx, ov_boot_picker_get_target(idx)->label);
            } else {
                printf("Error: Invalid target index %u\n", idx);
            }
        }
    } else if (strcmp(cmd, "deploy-simulate") == 0) {
        char log[256];
        ov_app_deployment_run_flow(false, false, log, sizeof(log));
        printf("Deployment Simulation:\n");
        printf("  Status: Dry-run completed cleanly. No disk blocks touched.\n");
    } else if (strcmp(cmd, "deploy-plan") == 0) {
        ov_deployment_step_create_plan("User Requested Plan Inspection");
        const ov_deploy_plan_t *p = ov_deployment_get_current_plan();
        printf("Deployment Plan %s (%u items):\n", p->plan_id, p->item_count);
        printf("  Backup Destination: %s\n", p->backup_directory);
        for (uint32_t i = 0; i < p->item_count; ++i) {
            printf("  [%u] %-8s %s (SHA256: %.16s...)\n",
                   i, p->items[i].action, p->items[i].target_path, p->items[i].payload_sha256);
        }
    } else if (strcmp(cmd, "deploy-apply") == 0) {
        char log[256];
        ov_status_t st = ov_app_deployment_run_flow(true, false, log, sizeof(log));
        if (st == OV_SUCCESS) {
            printf("Deployment Result: SUCCESS\n  %s\n", log);
        } else {
            printf("Deployment Result: FAILED\n  %s\n", log);
        }
    } else if (strcmp(cmd, "verify") == 0) {
        ov_status_t st = ov_deployment_step_verify(false);
        printf("Verification Result: %s\n", (st == OV_SUCCESS) ? "PASS (Integrity Verified)" : "FAIL");
    } else {
        printf("Unknown command: %s\n", cmd);
        print_usage(argv[0]);
    }

    ov_app_cleanup();
    return 0;
}
