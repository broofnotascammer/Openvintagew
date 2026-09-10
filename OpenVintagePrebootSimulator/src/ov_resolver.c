/**
 * OpenVintage Pre-Boot Simulator - Phase 5 Unified Resolver Subsystem Implementation
 * Architecture Decision Matrix & Integrated Multi-Subsystem Evaluation.
 */

#include "ov_resolver.h"
#include "ov_hardware.h"
#include "ov_unified_cache.h"
#include "ov_logger.h"
#include <stdio.h>
#include <string.h>

static ov_cpu_info_t *resolver_cpu = NULL;
static ov_gpu_info_t *resolver_gpu = NULL;
static ov_memory_info_t *resolver_mem = NULL;

/* Preset catalog */
static const char *preset_names[] = {
    "Metal Odyssey (Metal 2.0 / Modern Compute)",
    "CyberVulkan 2077 (Vulkan 1.2 / Heavy Shaders)",
    "Legacy Direct3D 9/11 Title (DirectX)",
    "ARM64 Scientific Matrix Kernel (ARM64 NEON)",
    "AVX2 Heavy Video Processing Filter",
    "OpenGL 3.3 Classic Engine (Retro/Native)"
};

uint32_t ov_resolver_get_preset_count(void) {
    return OV_WORKLOAD_PRESET_COUNT;
}

const char* ov_resolver_get_preset_name(ov_workload_preset_id_t preset_id) {
    if (preset_id >= OV_WORKLOAD_PRESET_COUNT) return "Unknown Preset";
    return preset_names[preset_id];
}

ov_integrated_request_t ov_resolver_get_preset_request(ov_workload_preset_id_t preset_id) {
    ov_integrated_request_t req;
    memset(&req, 0, sizeof(req));

    switch (preset_id) {
        case OV_WORKLOAD_PRESET_METAL_GAME:
            snprintf(req.application_name, sizeof(req.application_name), "Metal Odyssey");
            req.guest_cpu_arch = 1; /* ARM64 */
            req.requested_api = OV_API_METAL;
            req.api_version_major = 2;
            req.api_version_minor = 0;
            req.requires_compute = true;
            req.requires_tessellation = true;
            req.requires_avx2 = false;
            req.max_texture_dimension = 8192;
            req.required_vram_bytes = 1024ULL * 1024 * 1024;
            req.required_ram_bytes = 4ULL * 1024 * 1024 * 1024;
            req.required_cpu_cores = 4;
            req.guest_code_hash = 0xAA77BB22CC114499ULL;
            req.shader_bytecode_hash = 0x1122334455667788ULL;
            break;

        case OV_WORKLOAD_PRESET_VULKAN_SHOOTER:
            snprintf(req.application_name, sizeof(req.application_name), "CyberVulkan 2077");
            req.guest_cpu_arch = 2; /* x86_64 */
            req.requested_api = OV_API_VULKAN;
            req.api_version_major = 1;
            req.api_version_minor = 2;
            req.requires_compute = true;
            req.requires_tessellation = true;
            req.requires_avx2 = true;
            req.max_texture_dimension = 8192;
            req.required_vram_bytes = 1536ULL * 1024 * 1024;
            req.required_ram_bytes = 8ULL * 1024 * 1024 * 1024;
            req.required_cpu_cores = 4;
            req.guest_code_hash = 0x5566778899AABBCCULL;
            req.shader_bytecode_hash = 0xDEADBEEFCAFEBABEULL;
            break;

        case OV_WORKLOAD_PRESET_DIRECTX_LEGACY:
            snprintf(req.application_name, sizeof(req.application_name), "DirectX Legacy App");
            req.guest_cpu_arch = 3; /* x86_32 */
            req.requested_api = OV_API_DIRECTX;
            req.api_version_major = 9;
            req.api_version_minor = 0;
            req.requires_compute = false;
            req.requires_tessellation = false;
            req.requires_avx2 = false;
            req.max_texture_dimension = 2048;
            req.required_vram_bytes = 256ULL * 1024 * 1024;
            req.required_ram_bytes = 2ULL * 1024 * 1024 * 1024;
            req.required_cpu_cores = 2;
            req.guest_code_hash = 0x1234567890ABCDEFULL;
            req.shader_bytecode_hash = 0xFEDCBA0987654321ULL;
            break;

        case OV_WORKLOAD_PRESET_ARM64_COMPUTE:
            snprintf(req.application_name, sizeof(req.application_name), "ARM64 Compute Kernel");
            req.guest_cpu_arch = 1; /* ARM64 */
            req.requested_api = OV_API_OPENGL;
            req.api_version_major = 4;
            req.api_version_minor = 0;
            req.requires_compute = true;
            req.requires_tessellation = false;
            req.requires_avx2 = false;
            req.max_texture_dimension = 4096;
            req.required_vram_bytes = 512ULL * 1024 * 1024;
            req.required_ram_bytes = 8ULL * 1024 * 1024 * 1024;
            req.required_cpu_cores = 8;
            req.guest_code_hash = 0x9988776655443322ULL;
            req.shader_bytecode_hash = 0;
            break;

        case OV_WORKLOAD_PRESET_AVX2_SIMD:
            snprintf(req.application_name, sizeof(req.application_name), "AVX2 SIMD Filter");
            req.guest_cpu_arch = 2; /* x86_64 */
            req.requested_api = OV_API_OPENGL;
            req.api_version_major = 4;
            req.api_version_minor = 0;
            req.requires_compute = false;
            req.requires_tessellation = false;
            req.requires_avx2 = true;
            req.max_texture_dimension = 4096;
            req.required_vram_bytes = 512ULL * 1024 * 1024;
            req.required_ram_bytes = 4ULL * 1024 * 1024 * 1024;
            req.required_cpu_cores = 4;
            req.guest_code_hash = 0x1111222233334444ULL;
            req.shader_bytecode_hash = 0;
            break;

        case OV_WORKLOAD_PRESET_OPENGL_CLASSIC:
        default:
            snprintf(req.application_name, sizeof(req.application_name), "OpenGL Classic 3.3 Engine");
            req.guest_cpu_arch = 2; /* x86_64 */
            req.requested_api = OV_API_OPENGL;
            req.api_version_major = 3;
            req.api_version_minor = 3;
            req.requires_compute = false;
            req.requires_tessellation = false;
            req.requires_avx2 = false;
            req.max_texture_dimension = 4096;
            req.required_vram_bytes = 256ULL * 1024 * 1024;
            req.required_ram_bytes = 2ULL * 1024 * 1024 * 1024;
            req.required_cpu_cores = 2;
            req.guest_code_hash = 0;
            req.shader_bytecode_hash = 0;
            break;
    }

    return req;
}

ov_status_t ov_resolver_init(ov_cpu_info_t *cpu, ov_gpu_info_t *gpu, ov_memory_info_t *mem) {
    ov_log_info("Initializing Phase 5 Unified Architecture Decision Resolver...");
    resolver_cpu = cpu;
    resolver_gpu = gpu;
    resolver_mem = mem;
    return OV_SUCCESS;
}

ov_status_t ov_resolver_bind_hardware(
    ov_cpu_info_t    *cpu,
    ov_gpu_info_t    *gpu,
    ov_memory_info_t *mem
) {
    resolver_cpu = cpu;
    resolver_gpu = gpu;
    resolver_mem = mem;
    ov_log_info("Resolver bound to custom hardware state");
    return OV_SUCCESS;
}

ov_status_t ov_resolver_evaluate_with_topology(
    const ov_integrated_request_t *request,
    const ov_gpu_topology_t       *topo,
    ov_resolution_result_t        *out_result
) {
    if (!request || !out_result) return OV_ERROR_INVALID_PARAM;
    memset(out_result, 0, sizeof(ov_resolution_result_t));

    const ov_cpu_info_t *cpu = resolver_cpu ? resolver_cpu : ov_hardware_get_cpu();
    const ov_memory_info_t *mem = resolver_mem ? resolver_mem : ov_hardware_get_memory();

    /* Intelligent GPU Workload Selection across Topology */
    uint32_t best_gpu_idx = 0;
    const ov_gpu_info_t *gpu = NULL;

    if (topo && topo->gpu_count > 1) {
        int best_score = -1000;
        char selection_reason[128] = {0};

        for (uint32_t i = 0; i < topo->gpu_count; i++) {
            const ov_gpu_info_t *cand = &topo->gpus[i];
            int score = 0;

            /* Native API capability score */
            if (request->requested_api == OV_API_METAL) {
                if (request->api_version_major >= 2) {
                    if (cand->supports_metal && cand->metal_level >= OV_METAL_2) score += 150;
                    else if (cand->supports_metal) score += 30;
                    else score -= 50;
                } else if (cand->supports_metal) {
                    score += 100;
                }
            } else if (request->requested_api == OV_API_VULKAN) {
                if (cand->supports_vulkan) score += 150;
                else score -= 40;
            } else if (request->requested_api == OV_API_DIRECTX) {
                if (cand->supports_directx) score += 100;
            } else if (request->requested_api == OV_API_OPENGL) {
                if (cand->supports_opengl_core) score += 80;
                /* If lightweight OpenGL, favor integrated GPU for zero-dGPU power overhead */
                if (request->required_vram_bytes <= 512ULL * 1024 * 1024 &&
                    (cand->type == OV_GPU_INTEL_GEN7_HD4000 || cand->vendor_id == 0x8086)) {
                    score += 40;
                }
            }

            /* VRAM Capacity */
            if (cand->vram_bytes >= request->required_vram_bytes) score += 50;
            else score -= 40;

            /* Compute Capability */
            if (request->requires_compute && cand->supports_compute) score += 40;

            /* Texture resolution support */
            if (cand->max_texture_dimension >= request->max_texture_dimension) score += 20;

            if (score > best_score) {
                best_score = score;
                best_gpu_idx = i;
            }
        }

        gpu = &topo->gpus[best_gpu_idx];
        if (best_gpu_idx == topo->discrete_gpu_index && topo->has_discrete_gpu) {
            snprintf(selection_reason, sizeof(selection_reason),
                     "Discrete dGPU chosen for optimal native API/compute throughput (%s)", gpu->model_name);
        } else {
            snprintf(selection_reason, sizeof(selection_reason),
                     "Integrated iGPU chosen for thermal/power efficiency and low overhead (%s)", gpu->model_name);
        }
        snprintf(out_result->gpu_selection_reason, sizeof(out_result->gpu_selection_reason), "%s", selection_reason);
    } else if (topo && topo->gpu_count == 1) {
        best_gpu_idx = 0;
        gpu = &topo->gpus[0];
        snprintf(out_result->gpu_selection_reason, sizeof(out_result->gpu_selection_reason),
                 "Single GPU available (%s)", gpu->model_name);
    } else {
        best_gpu_idx = 0;
        gpu = resolver_gpu ? resolver_gpu : ov_hardware_get_gpu();
        snprintf(out_result->gpu_selection_reason, sizeof(out_result->gpu_selection_reason),
                 "Default active GPU (%s)", gpu->model_name);
    }

    out_result->selected_gpu_index = best_gpu_idx;
    snprintf(out_result->selected_gpu_name, sizeof(out_result->selected_gpu_name), "%s", gpu->model_name);

    uint32_t cost = 100;
    out_result->decision = OV_RESOLUTION_NATIVE;

    /* 1. CPU Architecture Resolution */
    if (request->guest_cpu_arch == 1) {
        /* Guest is ARM64 */
        if (cpu->type >= OV_CPU_ARM64_M1 && cpu->type <= OV_CPU_ARM64_GENERIC) {
            snprintf(out_result->cpu_path, sizeof(out_result->cpu_path), "Native ARM64 Core Dispatch");
            out_result->cpu_translation_required = false;
        } else {
            snprintf(out_result->cpu_path, sizeof(out_result->cpu_path), "OVIR-CPU JIT (ARM64 -> x86_64)");
            out_result->cpu_translation_required = true;
            cost += 45;

            /* Check real Unified Cache */
            if (request->guest_code_hash != 0) {
                uint8_t dummy[64];
                size_t dlen = 0;
                ov_status_t cst = ov_unified_cache_lookup(OV_CACHE_TIER_CPU_TRANSLATION,
                                                         request->guest_code_hash,
                                                         dummy, sizeof(dummy), &dlen);
                if (cst == OV_SUCCESS) {
                    out_result->cpu_cache_hit = true;
                    cost -= 20; /* JIT cache hit reduces overhead */
                } else {
                    out_result->cpu_cache_hit = false;
                    /* Insert into cache for next lookup */
                    uint8_t payload[32] = {0x90, 0xCC};
                    ov_unified_cache_store(OV_CACHE_TIER_CPU_TRANSLATION,
                                          request->guest_code_hash,
                                          payload, sizeof(payload));
                }
            }
        }
    } else if (request->guest_cpu_arch == 2) {
        /* Guest is x86_64 */
        if (cpu->type >= OV_CPU_ARM64_M1 && cpu->type <= OV_CPU_ARM64_GENERIC) {
            snprintf(out_result->cpu_path, sizeof(out_result->cpu_path), "OVIR-CPU Dynamic Binary Translation (x86_64 -> ARM64)");
            out_result->cpu_translation_required = true;
            cost += 55;
        } else {
            if (request->requires_avx2 && !cpu->has_avx2) {
                snprintf(out_result->cpu_path, sizeof(out_result->cpu_path), "OVIR-CPU AVX2 SIMD Emulation via SSE4.2");
                out_result->cpu_translation_required = true;
                cost += 35;
            } else {
                snprintf(out_result->cpu_path, sizeof(out_result->cpu_path), "Native x86_64 Fast Path");
                out_result->cpu_translation_required = false;
            }
        }
    } else {
        /* x86_32 */
        snprintf(out_result->cpu_path, sizeof(out_result->cpu_path), "x86_32 Compatibility Mode");
        out_result->cpu_translation_required = false;
        cost += 10;
    }

    /* 2. GPU API Resolution */
    if (request->requested_api == OV_API_METAL) {
        bool metal_supported = gpu->supports_metal;
        if (request->api_version_major >= 2 && gpu->metal_level < OV_METAL_2) {
            metal_supported = false;
        }
        if (metal_supported) {
            snprintf(out_result->gpu_path, sizeof(out_result->gpu_path), "Native Metal 2.0 Command Stream (%s)", gpu->model_name);
            out_result->gpu_translation_required = false;
        } else {
            snprintf(out_result->gpu_path, sizeof(out_result->gpu_path), "OVIR-GPU Metal -> OpenGL 4.0 Core / Gen7 EU Bytecode");
            out_result->gpu_translation_required = true;
            cost += 30;

            /* Check real Unified Cache for shader */
            if (request->shader_bytecode_hash != 0) {
                uint8_t dummy[64];
                size_t dlen = 0;
                ov_status_t cst = ov_unified_cache_lookup(OV_CACHE_TIER_SHADER,
                                                         request->shader_bytecode_hash,
                                                         dummy, sizeof(dummy), &dlen);
                if (cst == OV_SUCCESS) {
                    out_result->shader_cache_hit = true;
                    cost -= 15;
                } else {
                    out_result->shader_cache_hit = false;
                    uint8_t payload[32] = {0x01, 0x02};
                    ov_unified_cache_store(OV_CACHE_TIER_SHADER,
                                          request->shader_bytecode_hash,
                                          payload, sizeof(payload));
                }
            }
        }
    } else if (request->requested_api == OV_API_VULKAN) {
        if (gpu->supports_vulkan) {
            snprintf(out_result->gpu_path, sizeof(out_result->gpu_path), "Native Vulkan 1.2 Pipeline (%s)", gpu->model_name);
            out_result->gpu_translation_required = false;
        } else {
            snprintf(out_result->gpu_path, sizeof(out_result->gpu_path), "OVIR-GPU SPIR-V -> OpenGL 4.0 GLSL Translation");
            out_result->gpu_translation_required = true;
            cost += 35;

            if (request->shader_bytecode_hash != 0) {
                uint8_t dummy[64];
                size_t dlen = 0;
                ov_status_t cst = ov_unified_cache_lookup(OV_CACHE_TIER_SHADER,
                                                         request->shader_bytecode_hash,
                                                         dummy, sizeof(dummy), &dlen);
                if (cst == OV_SUCCESS) {
                    out_result->shader_cache_hit = true;
                    cost -= 15;
                } else {
                    out_result->shader_cache_hit = false;
                    uint8_t payload[32] = {0x03, 0x04};
                    ov_unified_cache_store(OV_CACHE_TIER_SHADER,
                                          request->shader_bytecode_hash,
                                          payload, sizeof(payload));
                }
            }
        }
    } else if (request->requested_api == OV_API_DIRECTX) {
        snprintf(out_result->gpu_path, sizeof(out_result->gpu_path), "OVIR-GPU DXBC -> OpenGL 3.3 Core Shaders");
        out_result->gpu_translation_required = true;
        cost += 20;
    } else {
        snprintf(out_result->gpu_path, sizeof(out_result->gpu_path), "Native OpenGL 3.3/4.0 Core Profile (%s)", gpu->model_name);
        out_result->gpu_translation_required = false;
    }

    /* 3. Silicon Feature Clamping & Workarounds */
    if (request->max_texture_dimension > gpu->max_texture_dimension) {
        out_result->texture_clamp_applied = true;
        out_result->decision = OV_RESOLUTION_SIMPLIFIED;
        cost += 15;
    }

    if (request->requires_compute && !gpu->supports_compute) {
        out_result->software_fallback_used = true;
        out_result->decision = OV_RESOLUTION_FALLBACK;
        size_t cur_len = strlen(out_result->gpu_path);
        snprintf(out_result->gpu_path + cur_len, sizeof(out_result->gpu_path) - cur_len, " + SoftPipe Compute");
        cost += 60;
    }

    /* 4. Memory Quota Calculation */
    uint64_t vram_alloc = request->required_vram_bytes;
    if (vram_alloc > gpu->vram_bytes) {
        vram_alloc = gpu->vram_bytes;
        out_result->shader_simplification_applied = true;
        out_result->decision = OV_RESOLUTION_SIMPLIFIED;
    }
    out_result->allocated_vram_quota = vram_alloc;

    uint64_t ram_alloc = request->required_ram_bytes;
    if (ram_alloc > mem->available_bytes) {
        ram_alloc = mem->available_bytes;
    }
    out_result->allocated_ram_quota = ram_alloc;

    out_result->performance_cost_factor = cost;

    /* 5. Routing Path & Rationale */
    snprintf(out_result->routing_path, sizeof(out_result->routing_path),
             "[%s] -> [%s]", out_result->cpu_path, out_result->gpu_path);

    if (out_result->cpu_translation_required && out_result->gpu_translation_required) {
        out_result->decision = OV_RESOLUTION_JIT_TRANSLATED;
        snprintf(out_result->rationale, sizeof(out_result->rationale),
                 "Full OVIR translation active (CPU JIT + GPU Shader translation). Overhead ~%u%%.",
                 cost > 100 ? cost - 100 : 0);
    } else if (out_result->cpu_translation_required) {
        out_result->decision = OV_RESOLUTION_JIT_TRANSLATED;
        snprintf(out_result->rationale, sizeof(out_result->rationale),
                 "OVIR-CPU translation active, GPU dispatch native on %s. Overhead ~%u%%.",
                 gpu->model_name, cost > 100 ? cost - 100 : 0);
    } else if (out_result->gpu_translation_required) {
        out_result->decision = OV_RESOLUTION_JIT_TRANSLATED;
        snprintf(out_result->rationale, sizeof(out_result->rationale),
                 "GPU translation active (MSL/SPIR-V -> GLSL) on %s, CPU direct. Overhead ~%u%%.",
                 gpu->model_name, cost > 100 ? cost - 100 : 0);
    } else {
        out_result->decision = OV_RESOLUTION_NATIVE;
        snprintf(out_result->rationale, sizeof(out_result->rationale),
                 "Native execution path matched on silicon (%s + %s).",
                 cpu->model_name, gpu->model_name);
    }

    return OV_SUCCESS;
}

ov_status_t ov_resolver_evaluate_integrated(
    const ov_integrated_request_t *request,
    ov_resolution_result_t        *out_result
) {
    if (!request || !out_result) return OV_ERROR_INVALID_PARAM;

    if (resolver_gpu != NULL) {
        /* Bound to specific GPU target */
        ov_gpu_topology_t bound_topo;
        memset(&bound_topo, 0, sizeof(bound_topo));
        bound_topo.gpu_count = 1;
        bound_topo.gpus[0] = *resolver_gpu;
        bound_topo.primary_gpu_index = 0;
        return ov_resolver_evaluate_with_topology(request, &bound_topo, out_result);
    }

    /* Otherwise, dynamically evaluate against active hardware GPU topology */
    return ov_resolver_evaluate_with_topology(request, ov_hardware_get_gpu_topology(), out_result);
}

/* Phase 2 legacy route helper */
ov_resolution_t ov_resolver_route(ov_workload_t *workload) {
    ov_resolution_t resolution = {0};
    if (!workload) {
        resolution.mode = OV_EXEC_FALLBACK;
        snprintf(resolution.reason, sizeof(resolution.reason), "Invalid workload");
        return resolution;
    }

    ov_integrated_request_t req;
    memset(&req, 0, sizeof(req));
    snprintf(req.application_name, sizeof(req.application_name), "Workload");
    req.guest_cpu_arch = 2;

    switch (workload->type) {
        case OV_WORKLOAD_COMPUTE:
            req.requested_api = OV_API_OPENGL;
            req.requires_compute = true;
            break;
        case OV_WORKLOAD_GRAPHICS:
            req.requested_api = OV_API_METAL;
            req.requires_compute = false;
            break;
        case OV_WORKLOAD_MEMORY:
            req.requested_api = OV_API_OPENGL;
            break;
        case OV_WORKLOAD_IO:
            req.requested_api = OV_API_OPENGL;
            break;
        case OV_WORKLOAD_MIXED:
        default:
            req.requested_api = OV_API_VULKAN;
            req.requires_compute = true;
            break;
    }

    ov_resolution_result_t result;
    ov_resolver_evaluate_integrated(&req, &result);

    if (result.decision == OV_RESOLUTION_NATIVE) {
        resolution.mode = OV_EXEC_NATIVE;
    } else if (result.decision == OV_RESOLUTION_FALLBACK) {
        resolution.mode = OV_EXEC_FALLBACK;
    } else {
        resolution.mode = OV_EXEC_TRANSLATED;
    }
    resolution.estimated_cost = result.performance_cost_factor;
    strncpy(resolution.reason, result.rationale, sizeof(resolution.reason) - 1);

    return resolution;
}

void ov_resolver_print_decision(ov_resolution_t *decision) {
    if (!decision) return;
    const char *mode_str = "UNKNOWN";
    if (decision->mode == OV_EXEC_NATIVE) mode_str = "NATIVE";
    else if (decision->mode == OV_EXEC_TRANSLATED) mode_str = "TRANSLATED";
    else if (decision->mode == OV_EXEC_FALLBACK) mode_str = "FALLBACK";

    ov_log_info("  Mode: %s", mode_str);
    ov_log_info("  Estimated Cost: %u", decision->estimated_cost);
    ov_log_info("  Reason: %s", decision->reason);
}

void ov_resolver_analyze_cpu(ov_cpu_info_t *cpu) {
    if (!cpu) return;
    ov_log_info("CPU Analysis:");
    ov_log_info("  Model: %s", cpu->model_name);
    ov_log_info("  Cores: %u, Threads: %u", cpu->cores, cpu->threads);
    ov_log_info("  SSE4.2: %s, AVX: %s, AVX2: %s",
                cpu->has_sse42 ? "Yes" : "No",
                cpu->has_avx ? "Yes" : "No",
                cpu->has_avx2 ? "Yes" : "No");
}

void ov_resolver_analyze_gpu(ov_gpu_info_t *gpu) {
    if (!gpu) return;
    ov_log_info("GPU Analysis:");
    ov_log_info("  Model: %s", gpu->model_name);
    ov_log_info("  EUs: %u, VRAM: %u MB", gpu->eu_count, gpu->vram_mb);
    ov_log_info("  Compute: %s, OpenGL Core: %s, Vulkan: %s",
                gpu->supports_compute ? "Yes" : "No",
                gpu->supports_opengl_core ? "Yes" : "No",
                gpu->supports_vulkan ? "Yes" : "No");
}

const char* ov_resolver_decision_to_string(ov_resolution_decision_t decision) {
    switch (decision) {
        case OV_RESOLUTION_NATIVE: return "Native Hardware Path";
        case OV_RESOLUTION_JIT_TRANSLATED: return "OVIR Translated";
        case OV_RESOLUTION_RECOMPILED: return "Statically Recompiled";
        case OV_RESOLUTION_SIMPLIFIED: return "Simplified / Clamped";
        case OV_RESOLUTION_FALLBACK: return "Software CPU Fallback";
        case OV_RESOLUTION_UNSUPPORTED: return "Unsupported";
        default: return "Unknown";
    }
}

const char* ov_resolver_api_to_string(ov_api_type_t api) {
    switch (api) {
        case OV_API_OPENGL: return "OpenGL";
        case OV_API_VULKAN: return "Vulkan";
        case OV_API_METAL: return "Metal";
        case OV_API_DIRECTX: return "DirectX";
        default: return "Generic";
    }
}

void ov_resolver_cleanup(void) {
    resolver_cpu = NULL;
    resolver_gpu = NULL;
    resolver_mem = NULL;
}
