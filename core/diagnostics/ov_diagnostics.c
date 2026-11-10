#include "ov_diagnostics.h"
#include <stdio.h>
#include <string.h>

int ov_diagnostics_generate_report(
    const ov_workload_desc_t *workload,
    const ov_execution_plan_t *plan,
    const ov_schedule_ticket_t *ticket,
    const ov_execution_telemetry_t *telemetry,
    char *out_buffer,
    size_t buffer_size
) {
    if (!out_buffer || buffer_size == 0) return -1;

    char temp[2048];
    int offset = 0;

    offset += snprintf(temp + offset, sizeof(temp) - offset,
        "=================================================================\n"
        "           OPENVINTAGE PHASE 10 EXECUTION PLAN DIAGNOSTIC       \n"
        "=================================================================\n"
        "Source architecture: %s\n"
        "Target architecture: %s\n"
        "Native execution:    %s\n"
        "Execution decision:  %s (%s)\n"
        "Reason code:         %s\n"
        "-----------------------------------------------------------------\n"
        "CPU translation:     %s\n"
        "OVIR-CPU backend:    %s\n"
        "Translation cache:   %s\n"
        "GPU translation:     %s\n"
        "OVIR-GPU backend:    %s\n"
        "GPU cache:           %s\n"
        "Scheduler profile:   %s\n"
        "Scheduler status:    %s (Ticket #%u)\n"
        "Assigned resources:  %u Cores, %llu MB RAM, %llu MB VRAM\n"
        "-----------------------------------------------------------------\n"
        "Compatibility:       %s\n"
        "Limitations:         %s\n"
        "-----------------------------------------------------------------\n"
        "Pipeline Telemetry:\n"
        "  Resolver decision:  %llu us\n"
        "  Cache lookup:       %llu us\n"
        "  Translation time:   %llu us\n"
        "  Optimizer time:     %llu us (%u optimizations applied)\n"
        "  Emission time:      %llu us\n"
        "  Scheduler time:     %llu us\n"
        "  Total pipeline:     %llu us\n"
        "=================================================================\n",
        workload ? ov_arch_name(workload->source_cpu_arch) : "N/A",
        plan ? ov_arch_name(plan->selected_cpu_target) : "N/A",
        (workload && workload->native_execution_available) ? "available" : "unavailable",
        plan ? ov_decision_name(plan->decision) : "N/A",
        (plan && plan->is_supported) ? "Supported" : "Unsupported",
        plan ? ov_reason_name(plan->reason_code) : "N/A",
        (plan && plan->cpu_translation_required) ? "required" : "not required",
        plan ? ov_arch_name(plan->selected_cpu_target) : "None",
        (telemetry && telemetry->cache_hit) ? "HIT" : "MISS",
        (plan && plan->gpu_translation_required) ? "required" : "not required",
        plan ? ov_gfx_backend_name(plan->selected_gfx_backend) : "None",
        (plan && plan->gpu_cache_hit) ? "HIT" : "MISS",
        plan ? ov_profile_name(plan->selected_profile) : "Default",
        ticket ? ((ticket->status == OV_SCHED_ACCEPTED) ? "ACCEPTED" : "REJECTED") : "N/A",
        ticket ? ticket->ticket_id : 0,
        ticket ? ticket->assigned_cores : 0,
        ticket ? (unsigned long long)ticket->assigned_ram_mb : 0ULL,
        ticket ? (unsigned long long)ticket->assigned_vram_mb : 0ULL,
        (plan && plan->is_supported) ? "Supported for tested instruction subset" : "Unsupported / Rejected",
        (plan && strlen(plan->limitations) > 0) ? plan->limitations : "None",
        telemetry ? (unsigned long long)telemetry->resolver_time_us : 0ULL,
        telemetry ? (unsigned long long)telemetry->cache_lookup_time_us : 0ULL,
        telemetry ? (unsigned long long)telemetry->translation_time_us : 0ULL,
        telemetry ? (unsigned long long)telemetry->optimizer_time_us : 0ULL,
        telemetry ? telemetry->optimizations_applied : 0,
        telemetry ? (unsigned long long)telemetry->emission_time_us : 0ULL,
        telemetry ? (unsigned long long)telemetry->scheduler_time_us : 0ULL,
        telemetry ? (unsigned long long)telemetry->total_pipeline_time_us : 0ULL
    );

    strncpy(out_buffer, temp, buffer_size - 1);
    out_buffer[buffer_size - 1] = '\0';
    return offset;
}
