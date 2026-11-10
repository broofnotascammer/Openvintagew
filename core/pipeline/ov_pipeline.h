#ifndef OV_PIPELINE_H
#define OV_PIPELINE_H

#include "../types/ov_types.h"
#include "../resolver/ov_resolver.h"
#include "../cache/ov_unified_cache.h"
#include "../scheduler/ov_scheduler.h"
#include "../telemetry/ov_telemetry.h"
#include "../diagnostics/ov_diagnostics.h"
#include "../../ovir/cpu/ovir_cpu.h"
#include "../../ovir/gpu/ovir_gpu.h"

typedef struct {
    ov_host_env_t *host;
    ov_unified_cache_t *cache;
    ov_scheduler_t *scheduler;
} ov_pipeline_context_t;

typedef struct {
    ov_execution_plan_t plan;
    ov_schedule_ticket_t ticket;
    ov_execution_telemetry_t telemetry;
    ovir_compiled_block_t cpu_compiled;
    ovir_gpu_dispatch_result_t gpu_dispatch;
    ovir_cpu_state_t cpu_sim_state;
    bool execution_dispatched;
    char diagnostic_report[2048];
} ov_execution_result_t;

void ov_pipeline_init(
    ov_pipeline_context_t *ctx,
    ov_host_env_t *host,
    ov_unified_cache_t *cache,
    ov_scheduler_t *scheduler
);

ov_status_t ov_pipeline_execute(
    ov_pipeline_context_t *ctx,
    const ov_workload_desc_t *workload,
    const uint32_t *raw_code,
    uint32_t raw_code_count,
    const ovir_gpu_workload_t *gpu_workload,
    ov_execution_result_t *out_result
);

#endif /* OV_PIPELINE_H */
