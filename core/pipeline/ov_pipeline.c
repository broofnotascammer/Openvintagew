#include "ov_pipeline.h"
#include <string.h>

void ov_pipeline_init(
    ov_pipeline_context_t *ctx,
    ov_host_env_t *host,
    ov_unified_cache_t *cache,
    ov_scheduler_t *scheduler
) {
    if (!ctx) return;
    ctx->host = host;
    ctx->cache = cache;
    ctx->scheduler = scheduler;
}

ov_status_t ov_pipeline_execute(
    ov_pipeline_context_t *ctx,
    const ov_workload_desc_t *workload,
    const uint32_t *raw_code,
    uint32_t raw_code_count,
    const ovir_gpu_workload_t *gpu_workload,
    ov_execution_result_t *out_result
) {
    if (!ctx || !workload || !out_result) return OV_ERR_INVALID_PARAM;
    memset(out_result, 0, sizeof(ov_execution_result_t));

    ov_timestamp_t t_total_start, t_total_end;
    ov_timestamp_t t_step_start, t_step_end;
    ov_timer_mark(&t_total_start);

    // 1. Resolver Decision Phase
    ov_timer_mark(&t_step_start);
    ov_status_t res_status = ov_resolver_create_plan(workload, ctx->host, ctx->cache, &out_result->plan);
    ov_timer_mark(&t_step_end);
    out_result->telemetry.resolver_time_us = ov_timer_elapsed_us(&t_step_start, &t_step_end);

    if (res_status != OV_SUCCESS || !out_result->plan.is_supported) {
        out_result->telemetry.execution_status = res_status;
        ov_timer_mark(&t_total_end);
        out_result->telemetry.total_pipeline_time_us = ov_timer_elapsed_us(&t_total_start, &t_total_end);

        ov_diagnostics_generate_report(workload, &out_result->plan, &out_result->ticket,
                                       &out_result->telemetry, out_result->diagnostic_report,
                                       sizeof(out_result->diagnostic_report));
        return res_status;
    }

    // 2. CPU Translation / Execution Path
    if (out_result->plan.cpu_translation_required) {
        ov_cache_cpu_metadata_t cpu_meta = {
            .source_arch = workload->source_cpu_arch,
            .target_arch = out_result->plan.selected_cpu_target,
            .code_identity_hash = workload->code_identity_hash,
            .translator_version_id = OV_TRANSLATOR_VERSION_ID,
            .required_cpu_features = workload->required_cpu_features,
            .optimization_level = 1
        };

        // Cache Probe
        ov_timer_mark(&t_step_start);
        ov_status_t c_st = OV_ERR_CACHE_MISS;
        if (ctx->cache) {
            c_st = ov_cache_lookup_cpu(ctx->cache, &cpu_meta, &out_result->cpu_compiled);
        }
        ov_timer_mark(&t_step_end);
        out_result->telemetry.cache_lookup_time_us = ov_timer_elapsed_us(&t_step_start, &t_step_end);

        ovir_ir_block_t ir;
        if (c_st == OV_SUCCESS) {
            out_result->telemetry.cache_hit = true;
            // Decode for simulation if needed
            if (raw_code && raw_code_count > 0) {
                ovir_cpu_decode(raw_code, raw_code_count, &ir);
            }
        } else {
            out_result->telemetry.cache_hit = false;

            // Cold translation: Decode -> Optimize -> Emit -> Verify -> Cache Store
            if (!raw_code || raw_code_count == 0) {
                return OV_ERR_INVALID_PARAM;
            }

            ov_timer_mark(&t_step_start);
            ov_status_t dec_st = ovir_cpu_decode(raw_code, raw_code_count, &ir);
            ov_timer_mark(&t_step_end);
            out_result->telemetry.translation_time_us = ov_timer_elapsed_us(&t_step_start, &t_step_end);
            if (dec_st != OV_SUCCESS) {
                return dec_st;
            }

            ov_timer_mark(&t_step_start);
            ovir_cpu_optimize(&ir, &out_result->telemetry.optimizations_applied);
            ov_timer_mark(&t_step_end);
            out_result->telemetry.optimizer_time_us = ov_timer_elapsed_us(&t_step_start, &t_step_end);

            ov_timer_mark(&t_step_start);
            ov_status_t emit_st = ovir_cpu_emit_x86_64(&ir, &out_result->cpu_compiled);
            ov_timer_mark(&t_step_end);
            out_result->telemetry.emission_time_us = ov_timer_elapsed_us(&t_step_start, &t_step_end);
            if (emit_st != OV_SUCCESS) {
                return emit_st;
            }

            if (ctx->cache) {
                ov_cache_store_cpu(ctx->cache, &cpu_meta, &out_result->cpu_compiled);
            }
        }

        // Simulate execution
        ovir_cpu_simulate(&ir, &out_result->cpu_sim_state);
    } else {
        // Native CPU path: simulate native execution
        if (raw_code && raw_code_count > 0) {
            ovir_ir_block_t ir;
            if (ovir_cpu_decode(raw_code, raw_code_count, &ir) == OV_SUCCESS) {
                ovir_cpu_simulate(&ir, &out_result->cpu_sim_state);
            }
        }
    }

    // 3. GPU Translation / Dispatch Path
    if (out_result->plan.gpu_translation_required && gpu_workload) {
        ov_status_t g_st = ovir_gpu_translate_workload(gpu_workload, &ctx->host->gpu_caps, &out_result->gpu_dispatch);
        if (g_st != OV_SUCCESS) {
            return g_st;
        }

        // Store translated shader in cache if available
        if (ctx->cache) {
            ov_cache_gpu_metadata_t gpu_meta = {
                .source_api = workload->source_gfx_api,
                .target_backend = out_result->plan.selected_gfx_backend,
                .shader_hash = workload->shader_identity_hash,
                .pipeline_hash = gpu_workload->pipeline.pipeline_hash,
                .required_gpu_features = workload->required_gpu_features,
                .translator_version_id = OV_TRANSLATOR_VERSION_ID
            };
            ov_cache_store_gpu(ctx->cache, &gpu_meta, out_result->gpu_dispatch.translated_shader_hash);
        }
    }

    // 4. Scheduler Resource Evaluation & Admission
    ov_timer_mark(&t_step_start);
    ov_status_t sched_st = ov_scheduler_evaluate(ctx->scheduler, workload, &out_result->plan, &out_result->ticket);
    ov_timer_mark(&t_step_end);
    out_result->telemetry.scheduler_time_us = ov_timer_elapsed_us(&t_step_start, &t_step_end);

    if (sched_st == OV_SUCCESS) {
        out_result->execution_dispatched = true;
        out_result->telemetry.allocated_cores = out_result->ticket.assigned_cores;
        out_result->telemetry.allocated_ram_mb = out_result->ticket.assigned_ram_mb;
        out_result->telemetry.execution_status = OV_SUCCESS;
    } else {
        out_result->execution_dispatched = false;
        out_result->telemetry.execution_status = sched_st;
    }

    ov_timer_mark(&t_total_end);
    out_result->telemetry.total_pipeline_time_us = ov_timer_elapsed_us(&t_total_start, &t_total_end);

    // 5. Diagnostics Generation
    ov_diagnostics_generate_report(workload, &out_result->plan, &out_result->ticket,
                                   &out_result->telemetry, out_result->diagnostic_report,
                                   sizeof(out_result->diagnostic_report));

    return sched_st;
}
