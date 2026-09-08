/**
 * OpenVintage Pre-Boot Simulator - OVIR-GPU Graphics Intermediate Layer (Phase 3)
 * Command stream recording, pipeline state management, and shader translation.
 */

#ifndef OV_GPU_ENGINE_H
#define OV_GPU_ENGINE_H

#include "ov_types.h"

/* Command Types */
typedef enum {
    OV_GPU_CMD_BEGIN_RENDER_PASS = 1,
    OV_GPU_CMD_END_RENDER_PASS,
    OV_GPU_CMD_SET_PIPELINE,
    OV_GPU_CMD_SET_VIEWPORT,
    OV_GPU_CMD_SET_SCISSOR,
    OV_GPU_CMD_BIND_VERTEX_BUFFER,
    OV_GPU_CMD_BIND_INDEX_BUFFER,
    OV_GPU_CMD_DRAW,
    OV_GPU_CMD_DRAW_INDEXED,
    OV_GPU_CMD_DISPATCH,
    OV_GPU_CMD_COPY_BUFFER,
    OV_GPU_CMD_PIPELINE_BARRIER,
    OV_GPU_CMD_PRESENT
} ov_gpu_cmd_type_t;

/* Single GPU Command */
typedef struct {
    ov_gpu_cmd_type_t type;
    uint32_t          sequence_id;
    union {
        struct {
            uint32_t render_target_id;
            float    clear_color[4];
            float    clear_depth;
        } begin_pass;
        struct {
            uint32_t pipeline_id;
            char     pipeline_name[32];
        } set_pipeline;
        struct {
            float x, y, width, height;
        } set_viewport;
        struct {
            int32_t x, y;
            uint32_t width, height;
        } set_scissor;
        struct {
            uint32_t slot;
            uint32_t buffer_id;
            uint32_t stride;
        } bind_vb;
        struct {
            uint32_t vertex_count;
            uint32_t instance_count;
            uint32_t first_vertex;
        } draw;
        struct {
            uint32_t index_count;
            uint32_t instance_count;
            uint32_t first_index;
        } draw_indexed;
        struct {
            uint32_t gx, gy, gz;
        } dispatch;
        struct {
            uint32_t old_state;
            uint32_t new_state;
        } barrier;
        struct {
            uint32_t sync_interval;
        } present;
    } as;
} ov_gpu_command_t;

#define OV_GPU_MAX_COMMANDS 64

/* Command List Container */
typedef struct {
    uint32_t         count;
    ov_gpu_command_t commands[OV_GPU_MAX_COMMANDS];
    bool             is_validated;
    uint32_t         total_draw_calls;
    uint32_t         total_dispatches;
    uint32_t         total_barriers;
    uint64_t         estimated_vram_usage;
} ov_gpu_command_list_t;

/* Shader Translation Specification */
typedef struct {
    char source_api[32];        /* "Metal 2 MSL", "Vulkan SPIR-V", "DirectX DXBC" */
    char target_backend[32];    /* "OpenGL 3.3 GLSL", "Intel Gen7 EU Bytecode", "Direct Metal" */
    char source_code[1024];     /* High-level snippet */
    char translated_code[1024]; /* Transformed shader code */
    uint32_t instruction_count;
    uint32_t register_spill_count;
    bool     requires_workaround;
    char     workaround_reason[128];
} ov_gpu_shader_translation_t;

/* Subsystem APIs */
ov_status_t ov_gpu_engine_init(void);
void        ov_gpu_engine_cleanup(void);

/* Command Recording & Validation */
ov_status_t ov_gpu_record_sample_frame(int scene_preset, ov_gpu_command_list_t *out_list);
ov_status_t ov_gpu_validate_command_list(ov_gpu_command_list_t *list, char *out_error, size_t err_size);

/* Shader Translation Simulation */
ov_status_t ov_gpu_simulate_shader_translation(
    int shader_preset,
    ov_gpu_shader_translation_t *out_trans
);

/* Format String Helper */
const char* ov_gpu_cmd_type_to_string(ov_gpu_cmd_type_t type);
void        ov_gpu_format_command_stream(const ov_gpu_command_list_t *list, char *out_buf, size_t max_len);

#endif /* OV_GPU_ENGINE_H */
