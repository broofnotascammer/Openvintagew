#include "ovir_gpu.h"
#include <string.h>

ov_status_t ovir_gpu_create_sample_workload(ovir_gpu_workload_t *out_workload, ov_gfx_api_t src_api, uint32_t required_features) {
    if (!out_workload) return OV_ERR_INVALID_PARAM;
    memset(out_workload, 0, sizeof(ovir_gpu_workload_t));

    // 1. Resource Creation: Vertex Buffer & Texture
    out_workload->resources[0] = (ovir_gpu_resource_desc_t){
        .resource_id = 1,
        .type = OVIR_RES_VERTEX_BUFFER,
        .size_bytes = 64 * 1024,
        .debug_name = "quad_vbo"
    };
    out_workload->resources[1] = (ovir_gpu_resource_desc_t){
        .resource_id = 2,
        .type = OVIR_RES_TEXTURE_2D,
        .width = 512,
        .height = 512,
        .size_bytes = 512 * 512 * 4,
        .debug_name = "albedo_tex"
    };
    out_workload->resource_count = 2;

    // 2. Shader Ingestion: Vertex + Fragment Shaders
    out_workload->shaders[0] = (ovir_gpu_shader_desc_t){
        .shader_id = 101,
        .stage = OVIR_SHADER_STAGE_VERTEX,
        .source_api = src_api,
        .bytecode_hash = 0x8a9b7c6d5e4f3a2bULL,
        .bytecode_size = 1024,
        .compiled = false
    };
    out_workload->shaders[1] = (ovir_gpu_shader_desc_t){
        .shader_id = 102,
        .stage = OVIR_SHADER_STAGE_FRAGMENT,
        .source_api = src_api,
        .bytecode_hash = 0x1f2e3d4c5b6a7988ULL,
        .bytecode_size = 2048,
        .compiled = false
    };
    out_workload->shader_count = 2;

    // 3. Pipeline Description
    out_workload->pipeline = (ovir_gpu_pipeline_desc_t){
        .pipeline_id = 501,
        .vertex_shader_id = 101,
        .fragment_shader_id = 102,
        .required_features = required_features,
        .pipeline_hash = 0xfeedfacecafebabeULL ^ (uint64_t)required_features,
        .is_compute = false
    };

    // 4. Draw Operation
    out_workload->draws[0] = (ovir_gpu_draw_cmd_t){
        .pipeline_id = 501,
        .vertex_count = 6,
        .instance_count = 1,
        .first_vertex = 0,
        .vbo_id = 1
    };
    out_workload->draw_count = 1;

    // 5. Synchronization
    out_workload->sync = (ovir_gpu_sync_cmd_t){
        .fence_id = 9001,
        .stage_mask = 0x3,
        .completed = false
    };

    out_workload->workload_hash = 0x4a5b6c7d8e9f0011ULL ^ (uint64_t)src_api;
    return OV_SUCCESS;
}

ov_gpu_route_t ovir_gpu_evaluate_route(const ovir_gpu_workload_t *workload, const ov_gpu_host_caps_t *caps, ov_reason_code_t *out_reason) {
    if (!workload || !caps) {
        if (out_reason) *out_reason = OV_REASON_UNSUPPORTED_OPERATION;
        return OV_GPU_ROUTE_UNSUPPORTED;
    }

    // Step 1: Does native API support it directly?
    ov_gfx_api_t src_api = (workload->shader_count > 0) ? workload->shaders[0].source_api : OV_GFX_API_NONE;
    if (src_api == OV_GFX_API_NONE) {
        if (out_reason) *out_reason = OV_REASON_NATIVE_SUPPORTED;
        return OV_GPU_ROUTE_NATIVE;
    }

    if (src_api == OV_GFX_API_METAL && caps->has_metal) {
        uint32_t needed = workload->pipeline.required_features;
        if ((needed & caps->supported_features) == needed) {
            if (out_reason) *out_reason = OV_REASON_NATIVE_SUPPORTED;
            return OV_GPU_ROUTE_NATIVE;
        }
    }

    // Step 2: Can another available graphics API reproduce it?
    // Check if host has translation backends (Metal, MoltenVK, OpenGL Core) that support all features
    bool has_any_backend = (caps->has_metal || caps->has_moltenvk || caps->has_opengl);
    uint32_t needed = workload->pipeline.required_features;
    if (has_any_backend && ((needed & caps->supported_features) == needed)) {
        if (out_reason) *out_reason = OV_REASON_GPU_API_MISMATCH_TRANSLATABLE;
        return OV_GPU_ROUTE_TRANSLATION;
    }

    // Step 3: Can available partial graphics features be combined to reproduce it?
    // (e.g. host has partial features that, combined with compute/shader emulation, reproduce the pipeline)
    if (has_any_backend && ((needed & (caps->supported_features | caps->partial_features)) == needed)) {
        if (out_reason) *out_reason = OV_REASON_GPU_PARTIAL_SUPPORT_RECONSTRUCTION;
        return OV_GPU_ROUTE_RECONSTRUCTION;
    }

    // Step 4: Can the operation be reconstructed/executed through CPU processing?
    if (caps->cpu_fallback_supported) {
        if (out_reason) *out_reason = OV_REASON_CPU_FALLBACK_REQUIRED;
        return OV_GPU_ROUTE_CPU_FALLBACK;
    }

    // Step 5: None of the above -> UNSUPPORTED
    if (out_reason) *out_reason = OV_REASON_UNSUPPORTED_OPERATION;
    return OV_GPU_ROUTE_UNSUPPORTED;
}

const char *ov_gpu_route_name(ov_gpu_route_t route) {
    switch (route) {
        case OV_GPU_ROUTE_NATIVE: return "Native GPU Execution";
        case OV_GPU_ROUTE_TRANSLATION: return "OVIR-GPU Translation";
        case OV_GPU_ROUTE_RECONSTRUCTION: return "OVIR-GPU Partial Feature Reconstruction";
        case OV_GPU_ROUTE_CPU_FALLBACK: return "CPU Fallback Software Reconstruction";
        case OV_GPU_ROUTE_UNSUPPORTED: return "Unsupported Graphics Operation";
        default: return "Unknown Route";
    }
}

ov_status_t ovir_gpu_validate_features(const ovir_gpu_workload_t *workload, const ov_gpu_host_caps_t *caps) {
    if (!workload || !caps) return OV_ERR_INVALID_PARAM;

    uint32_t needed = workload->pipeline.required_features;
    if ((needed & caps->supported_features) != needed) {
        return OV_ERR_UNSUPPORTED; // Hardware missing required GPU features
    }
    return OV_SUCCESS;
}

ov_status_t ovir_gpu_translate_workload(
    const ovir_gpu_workload_t *workload,
    const ov_gpu_host_caps_t *caps,
    ovir_gpu_dispatch_result_t *out_result
) {
    if (!workload || !caps || !out_result) return OV_ERR_INVALID_PARAM;

    memset(out_result, 0, sizeof(ovir_gpu_dispatch_result_t));

    ov_reason_code_t reason = OV_REASON_NONE;
    ov_gpu_route_t route = ovir_gpu_evaluate_route(workload, caps, &reason);
    out_result->route = route;

    if (route == OV_GPU_ROUTE_UNSUPPORTED) {
        return OV_ERR_UNSUPPORTED;
    }

    if (route == OV_GPU_ROUTE_CPU_FALLBACK) {
        // CPU Fallback Foundation
        out_result->selected_backend = OV_GFX_BACKEND_NONE;
        out_result->is_reconstructed = true;
        out_result->cpu_fallback.cpu_fallback_engaged = true;
        strncpy(out_result->cpu_fallback.fallback_reason,
                "GPU feature unsupported by host hardware; falling back to CPU software reconstruction",
                sizeof(out_result->cpu_fallback.fallback_reason) - 1);
        strncpy(out_result->cpu_fallback.operation_name,
                workload->pipeline.is_compute ? "Software Compute Pipeline" : "Software Rasterizer / Shader Emulation",
                sizeof(out_result->cpu_fallback.operation_name) - 1);
        out_result->cpu_fallback.estimated_cycle_cost = 450000;
        strncpy(out_result->cpu_fallback.cpu_execution_path, "Host SIMD CPU Rasterizer", sizeof(out_result->cpu_fallback.cpu_execution_path) - 1);
        out_result->cpu_fallback.result_status = OV_SUCCESS;
        out_result->sync_passed = true;
        out_result->draw_calls_emitted = workload->draw_count;
        return OV_SUCCESS;
    }

    // Select suitable backend
    if (caps->has_metal) {
        out_result->selected_backend = OV_GFX_BACKEND_METAL_NATIVE;
    } else if (caps->has_moltenvk) {
        out_result->selected_backend = OV_GFX_BACKEND_VULKAN_MVK;
    } else if (caps->has_opengl) {
        out_result->selected_backend = OV_GFX_BACKEND_OPENGL_CORE;
    } else {
        out_result->selected_backend = OV_GFX_BACKEND_OVIR_TRANSLATED;
    }

    if (route == OV_GPU_ROUTE_RECONSTRUCTION) {
        out_result->is_reconstructed = true;
    }

    // Translated shader hash derived deterministically from bytecode hash & target backend
    out_result->translated_shader_hash = workload->shaders[0].bytecode_hash ^
                                         workload->shaders[1].bytecode_hash ^
                                         (uint64_t)out_result->selected_backend;

    out_result->draw_calls_emitted = workload->draw_count;
    out_result->sync_passed = true;

    return OV_SUCCESS;
}

#include "../../core/telemetry/ov_telemetry.h"

ov_status_t ovir_gpu_translate_spirv_pipeline(
    const uint32_t *spirv_words,
    size_t word_count,
    const ov_gpu_host_caps_t *caps,
    bool enable_optimization,
    ovir_gpu_compiled_shader_t *out_compiled,
    ovir_gpu_dispatch_result_t *out_result
) {
    if (!spirv_words || word_count == 0 || !caps || !out_compiled || !out_result) {
        return OV_ERR_INVALID_PARAM;
    }

    memset(out_compiled, 0, sizeof(ovir_gpu_compiled_shader_t));
    memset(out_result, 0, sizeof(ovir_gpu_dispatch_result_t));

    // 1. Parse and validate SPIR-V bitstream
    ovir_spirv_module_t spirv_mod;
    ov_status_t status = ovir_spirv_parse(spirv_words, word_count, &spirv_mod);
    if (status != OV_SUCCESS) {
        return status;
    }
    ov_telemetry_record_gpu_shader_parsed();

    // 2. Build Architecture-Neutral OVIR-GPU IR
    ovir_gpu_shader_ir_t ir;
    status = ovir_gpu_ir_from_spirv(spirv_words, word_count, &ir);
    if (status != OV_SUCCESS) {
        return status;
    }
    ov_telemetry_record_gpu_shader_translated();

    // 3. Optimization Pass (constant folding + DCE)
    if (enable_optimization) {
        ovir_gpu_opt_stats_t opt_stats;
        ovir_gpu_optimize(&ir, &opt_stats);
        if (opt_stats.constant_folds_applied > 0 || opt_stats.dead_instructions_eliminated > 0) {
            ov_telemetry_record_gpu_shader_optimized();
        }
    }

    // 4. Capability Resolution & Operation Reconstruction
    // Check if host backend supports native matrix multiply
    // In OpenGL 3.3 Core Profile GLSL 330, matrix-vector multiply is native, but if host lacks it or requested
    // reconstruction is triggered, we can decompose it.
    uint32_t reconstructed_ops = 0;
    if (caps->backend != OV_GFX_BACKEND_METAL_NATIVE && (caps->partial_features & 0x0008)) {
        // Feature 0x0008: simulated partial matrix feature -> reconstruct
        ovir_gpu_reconstruct_operation(&ir, OVIR_GPU_OP_MAT_MUL_VEC, &reconstructed_ops);
        if (reconstructed_ops > 0) {
            ov_telemetry_record_gpu_reconstruction();
            out_result->is_reconstructed = true;
            out_result->route = OV_GPU_ROUTE_RECONSTRUCTION;
        }
    }

    // 5. Backend Translation to GLSL 330 Core
    status = ovir_gpu_translate_to_glsl(&ir, out_compiled);
    if (status != OV_SUCCESS) {
        return status;
    }

    // 6. Backend Compilation Verification
    status = ovir_gpu_backend_compile(out_compiled, true);
    if (status == OV_SUCCESS) {
        ov_telemetry_record_gpu_shader_compiled();
    }

    // 7. Synthesize Dispatch Result
    out_result->selected_backend = out_compiled->target_backend;
    if (!out_result->is_reconstructed) {
        out_result->route = OV_GPU_ROUTE_TRANSLATION;
    }
    out_result->translated_shader_hash = (uint64_t)out_compiled->source_length ^ 0x07230203ULL;
    out_result->draw_calls_emitted = 1;
    out_result->sync_passed = true;

    return status;
}

