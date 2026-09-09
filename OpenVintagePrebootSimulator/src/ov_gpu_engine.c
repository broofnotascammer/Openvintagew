/**
 * OpenVintage Pre-Boot Simulator - OVIR-GPU Graphics Intermediate Layer (Phase 3)
 */

#include "ov_gpu_engine.h"
#include "ov_logger.h"
#include <stdio.h>
#include <string.h>

ov_status_t ov_gpu_engine_init(void) {
    ov_log_info("Initializing OVIR-GPU Intermediate Graphics Layer...");
    return OV_SUCCESS;
}

void ov_gpu_engine_cleanup(void) {
    ov_log_info("OVIR-GPU cleanup complete");
}

ov_status_t ov_gpu_record_sample_frame(int scene_preset, ov_gpu_command_list_t *out_list) {
    if (!out_list) return OV_ERROR_INVALID_PARAM;
    memset(out_list, 0, sizeof(ov_gpu_command_list_t));

    uint32_t seq = 1;
    int idx = 0;

    /* 1. Begin Render Pass */
    out_list->commands[idx].type = OV_GPU_CMD_BEGIN_RENDER_PASS;
    out_list->commands[idx].sequence_id = seq++;
    out_list->commands[idx].as.begin_pass.render_target_id = 1;
    out_list->commands[idx].as.begin_pass.clear_color[0] = 0.12f;
    out_list->commands[idx].as.begin_pass.clear_color[1] = 0.15f;
    out_list->commands[idx].as.begin_pass.clear_color[2] = 0.20f;
    out_list->commands[idx].as.begin_pass.clear_color[3] = 1.0f;
    out_list->commands[idx].as.begin_pass.clear_depth = 1.0f;
    idx++;

    /* 2. Set Pipeline */
    out_list->commands[idx].type = OV_GPU_CMD_SET_PIPELINE;
    out_list->commands[idx].sequence_id = seq++;
    out_list->commands[idx].as.set_pipeline.pipeline_id = 101;
    strcpy(out_list->commands[idx].as.set_pipeline.pipeline_name, "GBuffer_Opaque_Metal_Trans");
    idx++;

    /* 3. Set Viewport */
    out_list->commands[idx].type = OV_GPU_CMD_SET_VIEWPORT;
    out_list->commands[idx].sequence_id = seq++;
    out_list->commands[idx].as.set_viewport.x = 0;
    out_list->commands[idx].as.set_viewport.y = 0;
    out_list->commands[idx].as.set_viewport.width = 1920.0f;
    out_list->commands[idx].as.set_viewport.height = 1080.0f;
    idx++;

    /* 4. Set Scissor */
    out_list->commands[idx].type = OV_GPU_CMD_SET_SCISSOR;
    out_list->commands[idx].sequence_id = seq++;
    out_list->commands[idx].as.set_scissor.x = 0;
    out_list->commands[idx].as.set_scissor.y = 0;
    out_list->commands[idx].as.set_scissor.width = 1920;
    out_list->commands[idx].as.set_scissor.height = 1080;
    idx++;

    /* 5. Bind Vertex Buffer */
    out_list->commands[idx].type = OV_GPU_CMD_BIND_VERTEX_BUFFER;
    out_list->commands[idx].sequence_id = seq++;
    out_list->commands[idx].as.bind_vb.slot = 0;
    out_list->commands[idx].as.bind_vb.buffer_id = 501;
    out_list->commands[idx].as.bind_vb.stride = 32;
    idx++;

    /* 6. Draw Indexed Geometry */
    out_list->commands[idx].type = OV_GPU_CMD_DRAW_INDEXED;
    out_list->commands[idx].sequence_id = seq++;
    out_list->commands[idx].as.draw_indexed.index_count = (scene_preset == 0) ? 36840 : 12450;
    out_list->commands[idx].as.draw_indexed.instance_count = 1;
    out_list->commands[idx].as.draw_indexed.first_index = 0;
    out_list->total_draw_calls++;
    idx++;

    /* 7. Pipeline Barrier */
    out_list->commands[idx].type = OV_GPU_CMD_PIPELINE_BARRIER;
    out_list->commands[idx].sequence_id = seq++;
    out_list->commands[idx].as.barrier.old_state = 1; /* RenderTarget */
    out_list->commands[idx].as.barrier.new_state = 2; /* ShaderResource */
    out_list->total_barriers++;
    idx++;

    /* 8. Compute Post-Process Dispatch */
    if (scene_preset == 0) {
        out_list->commands[idx].type = OV_GPU_CMD_DISPATCH;
        out_list->commands[idx].sequence_id = seq++;
        out_list->commands[idx].as.dispatch.gx = 120;
        out_list->commands[idx].as.dispatch.gy = 68;
        out_list->commands[idx].as.dispatch.gz = 1;
        out_list->total_dispatches++;
        idx++;
    }

    /* 9. End Render Pass */
    out_list->commands[idx].type = OV_GPU_CMD_END_RENDER_PASS;
    out_list->commands[idx].sequence_id = seq++;
    idx++;

    /* 10. Present */
    out_list->commands[idx].type = OV_GPU_CMD_PRESENT;
    out_list->commands[idx].sequence_id = seq++;
    out_list->commands[idx].as.present.sync_interval = 1;
    idx++;

    out_list->count = idx;
    out_list->is_validated = true;
    out_list->estimated_vram_usage = 128ULL * 1024 * 1024;

    ov_log_info("OVIR-GPU recorded frame: %u commands, %u draws, %u dispatches",
                out_list->count, out_list->total_draw_calls, out_list->total_dispatches);

    return OV_SUCCESS;
}

ov_status_t ov_gpu_validate_command_list(ov_gpu_command_list_t *list, char *out_error, size_t err_size) {
    if (!list) return OV_ERROR_INVALID_PARAM;
    bool in_pass = false;

    for (uint32_t i = 0; i < list->count; i++) {
        ov_gpu_command_t *cmd = &list->commands[i];

        if (cmd->type == OV_GPU_CMD_BEGIN_RENDER_PASS) {
            if (in_pass) {
                if (out_error) snprintf(out_error, err_size, "Nested BeginRenderPass at cmd %u", cmd->sequence_id);
                list->is_validated = false;
                return OV_ERROR_CONFIG;
            }
            in_pass = true;
        } else if (cmd->type == OV_GPU_CMD_END_RENDER_PASS) {
            if (!in_pass) {
                if (out_error) snprintf(out_error, err_size, "EndRenderPass called without active pass at cmd %u", cmd->sequence_id);
                list->is_validated = false;
                return OV_ERROR_CONFIG;
            }
            in_pass = false;
        } else if (cmd->type == OV_GPU_CMD_DRAW || cmd->type == OV_GPU_CMD_DRAW_INDEXED) {
            if (!in_pass) {
                if (out_error) snprintf(out_error, err_size, "Draw command outside render pass at cmd %u", cmd->sequence_id);
                list->is_validated = false;
                return OV_ERROR_CONFIG;
            }
        }
    }

    if (in_pass) {
        if (out_error) snprintf(out_error, err_size, "RenderPass unclosed at end of command stream");
        list->is_validated = false;
        return OV_ERROR_CONFIG;
    }

    list->is_validated = true;
    if (out_error) snprintf(out_error, err_size, "Valid OVIR-GPU command stream: %u commands", list->count);
    return OV_SUCCESS;
}

ov_status_t ov_gpu_simulate_shader_translation(
    int shader_preset,
    ov_gpu_shader_translation_t *out_trans
) {
    if (!out_trans) return OV_ERROR_INVALID_PARAM;
    memset(out_trans, 0, sizeof(ov_gpu_shader_translation_t));

    if (shader_preset == 0) {
        /* Metal MSL -> OpenGL 4.0 Core / Gen7 EU */
        strcpy(out_trans->source_api, "Metal Shading Language (MSL 2.0)");
        strcpy(out_trans->target_backend, "OpenGL GLSL 4.0 / Intel Gen7 EU");
        strcpy(out_trans->source_code,
               "#include <metal_stdlib>\n"
               "using namespace metal;\n\n"
               "struct VertexIn {\n"
               "    float4 position [[attribute(0)]];\n"
               "    float2 uv [[attribute(1)]];\n"
               "};\n\n"
               "fragment float4 fragmentMain(VertexIn in [[stage_in]],\n"
               "                             texture2d<float> tex [[texture(0)]],\n"
               "                             sampler smp [[sampler(0)]]) {\n"
               "    return tex.sample(smp, in.uv);\n"
               "}\n");

        strcpy(out_trans->translated_code,
               "#version 400 core\n"
               "// Translated by OpenVintage OVIR-GPU Translator\n"
               "// Target Silicon: Intel HD Graphics 4000 (Gen7)\n\n"
               "in vec2 v_uv;\n"
               "out vec4 o_fragColor;\n"
               "uniform sampler2D u_tex0;\n\n"
               "void main() {\n"
               "    o_fragColor = texture(u_tex0, v_uv);\n"
               "}\n\n"
               "// Gen7 Hardware EU Mapping:\n"
               "// mov (8) r2.0<1>:f  g1.0<8,8,1>:f\n"
               "// send.sampler (8) r4.0:f  r2.0:f  null  0x02114000\n"
               "// send.render_target (8) null  r4.0:f  0x05100000\n");

        out_trans->instruction_count = 14;
        out_trans->register_spill_count = 0;
        out_trans->requires_workaround = false;
        strcpy(out_trans->workaround_reason, "Direct translation successful; 0 spills.");
    } else {
        /* Vulkan SPIR-V -> OpenGL GLSL */
        strcpy(out_trans->source_api, "Vulkan SPIR-V 1.2");
        strcpy(out_trans->target_backend, "OpenGL GLSL 4.3 Core");
        strcpy(out_trans->source_code,
               "; SPIR-V\n"
               "; Version: 1.2\n"
               "; Generator: Khronos Glslang; 1\n"
               "OpCapability Shader\n"
               "OpMemoryModel Logical GLSL450\n"
               "OpEntryPoint Fragment %main \"main\" %inUV %outColor\n"
               "OpExecutionMode %main OriginUpperLeft\n"
               "%main = OpFunction %void None %3\n"
               "%label = OpLabel\n"
               "OpReturn\n"
               "OpFunctionEnd\n");

        strcpy(out_trans->translated_code,
               "#version 430 core\n"
               "// OVIR-GPU SPIR-V Decompiler to GLSL\n"
               "layout(location = 0) in vec2 inUV;\n"
               "layout(location = 0) out vec4 outColor;\n"
               "layout(binding = 0) uniform sampler2D diffuseMap;\n\n"
               "void main() {\n"
               "    outColor = texture(diffuseMap, inUV);\n"
               "}\n");

        out_trans->instruction_count = 18;
        out_trans->register_spill_count = 0;
        out_trans->requires_workaround = true;
        strcpy(out_trans->workaround_reason, "OriginUpperLeft inverted Y-axis workaround applied for Gen7 sampler.");
    }

    return OV_SUCCESS;
}

const char* ov_gpu_cmd_type_to_string(ov_gpu_cmd_type_t type) {
    switch (type) {
        case OV_GPU_CMD_BEGIN_RENDER_PASS: return "BeginRenderPass";
        case OV_GPU_CMD_END_RENDER_PASS: return "EndRenderPass";
        case OV_GPU_CMD_SET_PIPELINE: return "SetPipeline";
        case OV_GPU_CMD_SET_VIEWPORT: return "SetViewport";
        case OV_GPU_CMD_SET_SCISSOR: return "SetScissor";
        case OV_GPU_CMD_BIND_VERTEX_BUFFER: return "BindVertexBuffer";
        case OV_GPU_CMD_BIND_INDEX_BUFFER: return "BindIndexBuffer";
        case OV_GPU_CMD_DRAW: return "Draw";
        case OV_GPU_CMD_DRAW_INDEXED: return "DrawIndexed";
        case OV_GPU_CMD_DISPATCH: return "DispatchCompute";
        case OV_GPU_CMD_COPY_BUFFER: return "CopyBuffer";
        case OV_GPU_CMD_PIPELINE_BARRIER: return "PipelineBarrier";
        case OV_GPU_CMD_PRESENT: return "Present";
        default: return "UnknownCmd";
    }
}

void ov_gpu_format_command_stream(const ov_gpu_command_list_t *list, char *out_buf, size_t max_len) {
    if (!list || !out_buf || max_len == 0) return;
    size_t pos = 0;
    pos += snprintf(out_buf + pos, max_len - pos,
                    "// OVIR-GPU Command Stream: %u commands (Validated: %s)\n\n",
                    list->count, list->is_validated ? "YES" : "NO");

    for (uint32_t i = 0; i < list->count; i++) {
        const ov_gpu_command_t *cmd = &list->commands[i];
        pos += snprintf(out_buf + pos, max_len - pos, "[%02u] %-18s ",
                        cmd->sequence_id, ov_gpu_cmd_type_to_string(cmd->type));

        switch (cmd->type) {
            case OV_GPU_CMD_BEGIN_RENDER_PASS:
                pos += snprintf(out_buf + pos, max_len - pos, "RT=%u Clear=(%.2f, %.2f, %.2f, %.2f)\n",
                                cmd->as.begin_pass.render_target_id,
                                cmd->as.begin_pass.clear_color[0], cmd->as.begin_pass.clear_color[1],
                                cmd->as.begin_pass.clear_color[2], cmd->as.begin_pass.clear_color[3]);
                break;
            case OV_GPU_CMD_SET_PIPELINE:
                pos += snprintf(out_buf + pos, max_len - pos, "ID=%u Name=\"%s\"\n",
                                cmd->as.set_pipeline.pipeline_id, cmd->as.set_pipeline.pipeline_name);
                break;
            case OV_GPU_CMD_SET_VIEWPORT:
                pos += snprintf(out_buf + pos, max_len - pos, "%.0fx%.0f @ (%.0f, %.0f)\n",
                                cmd->as.set_viewport.width, cmd->as.set_viewport.height,
                                cmd->as.set_viewport.x, cmd->as.set_viewport.y);
                break;
            case OV_GPU_CMD_DRAW_INDEXED:
                pos += snprintf(out_buf + pos, max_len - pos, "Indices=%u First=%u Inst=%u\n",
                                cmd->as.draw_indexed.index_count, cmd->as.draw_indexed.first_index,
                                cmd->as.draw_indexed.instance_count);
                break;
            case OV_GPU_CMD_DISPATCH:
                pos += snprintf(out_buf + pos, max_len - pos, "Groups=(%u, %u, %u)\n",
                                cmd->as.dispatch.gx, cmd->as.dispatch.gy, cmd->as.dispatch.gz);
                break;
            case OV_GPU_CMD_PIPELINE_BARRIER:
                pos += snprintf(out_buf + pos, max_len - pos, "Transition %u -> %u\n",
                                cmd->as.barrier.old_state, cmd->as.barrier.new_state);
                break;
            case OV_GPU_CMD_PRESENT:
                pos += snprintf(out_buf + pos, max_len - pos, "VSync Interval=%u\n",
                                cmd->as.present.sync_interval);
                break;
            default:
                pos += snprintf(out_buf + pos, max_len - pos, "\n");
                break;
        }
    }
}
