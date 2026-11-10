#include "ov_resolver.h"
#include <stdio.h>
#include <string.h>

ov_status_t ov_resolver_create_plan(
    const ov_workload_desc_t *workload,
    const ov_host_env_t *host,
    ov_unified_cache_t *cache,
    ov_execution_plan_t *out_plan
) {
    if (!workload || !host || !out_plan) return OV_ERR_INVALID_PARAM;
    memset(out_plan, 0, sizeof(ov_execution_plan_t));

    // 1. Strict Resource Boundary Validation
    if (workload->ram_requirement_mb > host->host_ram_mb) {
        out_plan->decision = OV_DECISION_UNSUPPORTED;
        out_plan->reason_code = OV_REASON_INSUFFICIENT_RAM;
        out_plan->is_supported = false;
        snprintf(out_plan->limitations, sizeof(out_plan->limitations),
                 "Workload requires %llu MB RAM, host only has %llu MB",
                 (unsigned long long)workload->ram_requirement_mb,
                 (unsigned long long)host->host_ram_mb);
        return OV_ERR_UNSUPPORTED;
    }

    if (workload->vram_requirement_mb > host->gpu_caps.available_vram_mb) {
        out_plan->decision = OV_DECISION_UNSUPPORTED;
        out_plan->reason_code = OV_REASON_INSUFFICIENT_VRAM;
        out_plan->is_supported = false;
        snprintf(out_plan->limitations, sizeof(out_plan->limitations),
                 "Workload requires %llu MB VRAM, GPU only has %llu MB",
                 (unsigned long long)workload->vram_requirement_mb,
                 (unsigned long long)host->gpu_caps.available_vram_mb);
        return OV_ERR_UNSUPPORTED;
    }

    if (workload->required_cpu_cores > host->host_cpu_cores) {
        out_plan->decision = OV_DECISION_UNSUPPORTED;
        out_plan->reason_code = OV_REASON_INSUFFICIENT_CORES;
        out_plan->is_supported = false;
        snprintf(out_plan->limitations, sizeof(out_plan->limitations),
                 "Workload requires %u CPU cores, host only has %u cores",
                 workload->required_cpu_cores, host->host_cpu_cores);
        return OV_ERR_UNSUPPORTED;
    }

    // 2. GPU Hardware Feature Validation
    uint32_t gpu_needed = workload->required_gpu_features;
    bool gpu_cpu_fallback_needed = false;
    bool gpu_reconstruction_needed = false;
    if ((gpu_needed & host->gpu_caps.supported_features) != gpu_needed) {
        if ((gpu_needed & (host->gpu_caps.supported_features | host->gpu_caps.partial_features)) == gpu_needed) {
            gpu_reconstruction_needed = true;
        } else if (host->gpu_caps.cpu_fallback_supported) {
            gpu_cpu_fallback_needed = true;
        } else {
            out_plan->decision = OV_DECISION_UNSUPPORTED;
            out_plan->reason_code = OV_REASON_UNSUPPORTED_GPU_FEATURE;
            out_plan->is_supported = false;
            snprintf(out_plan->limitations, sizeof(out_plan->limitations),
                     "Missing hardware GPU feature mask: 0x%04x (Host supports: 0x%04x)",
                     gpu_needed & ~host->gpu_caps.supported_features,
                     host->gpu_caps.supported_features);
            return OV_ERR_UNSUPPORTED;
        }
    }

    // 3. CPU Architecture & Feature Validation
    bool cpu_translation_required = false;
    if (workload->native_execution_available && workload->source_cpu_arch == host->host_cpu_arch) {
        // Native path: host must have required CPU features directly
        uint32_t cpu_needed = workload->required_cpu_features;
        if ((cpu_needed & host->host_cpu_features) != cpu_needed) {
            out_plan->decision = OV_DECISION_UNSUPPORTED;
            out_plan->reason_code = OV_REASON_UNSUPPORTED_CPU_FEATURE;
            out_plan->is_supported = false;
            snprintf(out_plan->limitations, sizeof(out_plan->limitations),
                     "Native CPU feature mask missing: 0x%04x",
                     cpu_needed & ~host->host_cpu_features);
            return OV_ERR_UNSUPPORTED;
        }
        cpu_translation_required = false;
        out_plan->selected_cpu_target = host->host_cpu_arch;
    } else {
        // Translation path: check supported translation pairs
        if (workload->source_cpu_arch == OV_ARCH_ARM64 && host->host_cpu_arch == OV_ARCH_X86_64) {
            cpu_translation_required = true;
            out_plan->selected_cpu_target = OV_ARCH_X86_64;
            if (workload->required_cpu_features & OV_CPU_FEAT_NEON) {
                snprintf(out_plan->limitations, sizeof(out_plan->limitations),
                         "ARM64 JIT active: GPR scalar + NEON vector execution pipeline (4S/2D/16B)");
            } else {
                snprintf(out_plan->limitations, sizeof(out_plan->limitations),
                         "ARM64 JIT active: GPR scalar execution pipeline");
            }
        } else {
            out_plan->decision = OV_DECISION_UNSUPPORTED;
            out_plan->reason_code = OV_REASON_UNSUPPORTED_TRANSLATION_PAIR;
            out_plan->is_supported = false;
            snprintf(out_plan->limitations, sizeof(out_plan->limitations),
                     "No translator available from %s to %s",
                     ov_arch_name(workload->source_cpu_arch),
                     ov_arch_name(host->host_cpu_arch));
            return OV_ERR_UNSUPPORTED;
        }
    }

    // 4. GPU Path Determination
    bool gpu_translation_required = false;
    if (gpu_cpu_fallback_needed) {
        out_plan->selected_gfx_backend = OV_GFX_BACKEND_NONE;
        out_plan->cpu_fallback_engaged = true;
        strncpy(out_plan->cpu_fallback_path, "Host SIMD CPU Rasterizer", sizeof(out_plan->cpu_fallback_path) - 1);
        gpu_translation_required = true;
    } else if (gpu_reconstruction_needed) {
        out_plan->gpu_reconstruction_engaged = true;
        gpu_translation_required = true;
        if (host->gpu_caps.has_metal) {
            out_plan->selected_gfx_backend = OV_GFX_BACKEND_METAL_NATIVE;
        } else if (host->gpu_caps.has_moltenvk) {
            out_plan->selected_gfx_backend = OV_GFX_BACKEND_VULKAN_MVK;
        } else {
            out_plan->selected_gfx_backend = OV_GFX_BACKEND_OPENGL_CORE;
        }
    } else if (workload->source_gfx_api == OV_GFX_API_NONE) {
        out_plan->selected_gfx_backend = OV_GFX_BACKEND_NONE;
    } else if (workload->source_gfx_api == OV_GFX_API_METAL && host->gpu_caps.has_metal) {
        out_plan->selected_gfx_backend = OV_GFX_BACKEND_METAL_NATIVE;
    } else {
        // Translation required
        gpu_translation_required = true;
        if (host->gpu_caps.has_metal) {
            out_plan->selected_gfx_backend = OV_GFX_BACKEND_METAL_NATIVE;
        } else if (host->gpu_caps.has_moltenvk) {
            out_plan->selected_gfx_backend = OV_GFX_BACKEND_VULKAN_MVK;
        } else if (host->gpu_caps.has_opengl) {
            out_plan->selected_gfx_backend = OV_GFX_BACKEND_OPENGL_CORE;
        } else {
            out_plan->selected_gfx_backend = OV_GFX_BACKEND_OVIR_TRANSLATED;
        }
    }

    // 5. Synthesize Execution Decision
    out_plan->cpu_translation_required = cpu_translation_required;
    out_plan->gpu_translation_required = gpu_translation_required;
    out_plan->is_supported = true;

    if (gpu_cpu_fallback_needed) {
        out_plan->decision = OV_DECISION_CPU_FALLBACK;
        out_plan->reason_code = OV_REASON_CPU_FALLBACK_REQUIRED;
        snprintf(out_plan->limitations, sizeof(out_plan->limitations),
                 "Hardware GPU features unavailable; executing via CPU fallback software reconstruction");
    } else if (gpu_reconstruction_needed) {
        out_plan->decision = OV_DECISION_GPU_RECONSTRUCTION;
        out_plan->reason_code = OV_REASON_GPU_PARTIAL_SUPPORT_RECONSTRUCTION;
        snprintf(out_plan->limitations, sizeof(out_plan->limitations),
                 "Partial hardware GPU support; reconstructing missing stages via multi-pass shaders");
    } else if (!cpu_translation_required && !gpu_translation_required) {
        out_plan->decision = OV_DECISION_NATIVE;
        out_plan->reason_code = OV_REASON_NATIVE_SUPPORTED;
    } else if (cpu_translation_required && !gpu_translation_required) {
        out_plan->decision = OV_DECISION_CPU_TRANSLATION;
        out_plan->reason_code = OV_REASON_CPU_ARCH_MISMATCH_TRANSLATABLE;
    } else if (!cpu_translation_required && gpu_translation_required) {
        out_plan->decision = OV_DECISION_GPU_TRANSLATION;
        out_plan->reason_code = OV_REASON_GPU_API_MISMATCH_TRANSLATABLE;
    } else {
        out_plan->decision = OV_DECISION_CPU_AND_GPU_TRANSLATION;
        out_plan->reason_code = OV_REASON_CPU_AND_GPU_MISMATCH_TRANSLATABLE;
    }

    // 6. Scheduler Profile Selection
    if (workload->priority == OV_PRIORITY_REALTIME) {
        out_plan->selected_profile = OV_PROFILE_MAX_THROUGHPUT;
    } else if (workload->priority == OV_PRIORITY_HIGH || workload->workload_type == OV_WORKLOAD_COMPUTE) {
        out_plan->selected_profile = OV_PROFILE_PERFORMANCE;
    } else if (workload->priority == OV_PRIORITY_LOW) {
        out_plan->selected_profile = OV_PROFILE_ENERGY_SAVER;
    } else {
        out_plan->selected_profile = OV_PROFILE_BALANCED;
    }

    // 7. Probe Cache if provided
    if (cache) {
        if (cpu_translation_required && workload->code_identity_hash != 0) {
            ov_cache_cpu_metadata_t req_meta = {
                .source_arch = workload->source_cpu_arch,
                .target_arch = out_plan->selected_cpu_target,
                .code_identity_hash = workload->code_identity_hash,
                .translator_version_id = OV_TRANSLATOR_VERSION_ID,
                .required_cpu_features = workload->required_cpu_features,
                .optimization_level = 1
            };
            ovir_compiled_block_t dummy;
            ov_status_t c_st = ov_cache_lookup_cpu(cache, &req_meta, &dummy);
            out_plan->cpu_cache_hit = (c_st == OV_SUCCESS);
        }

        if (gpu_translation_required && workload->shader_identity_hash != 0) {
            ov_cache_gpu_metadata_t req_meta = {
                .source_api = workload->source_gfx_api,
                .target_backend = out_plan->selected_gfx_backend,
                .shader_hash = workload->shader_identity_hash,
                .pipeline_hash = 0,
                .required_gpu_features = workload->required_gpu_features,
                .translator_version_id = OV_TRANSLATOR_VERSION_ID
            };
            uint64_t dummy_hash;
            ov_status_t g_st = ov_cache_lookup_gpu(cache, &req_meta, &dummy_hash);
            out_plan->gpu_cache_hit = (g_st == OV_SUCCESS);
        }
    }

    return OV_SUCCESS;
}
