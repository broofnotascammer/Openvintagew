#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../core/types/ov_types.h"
#include "../core/telemetry/ov_telemetry.h"
#include "../core/resolver/ov_resolver.h"
#include "../ovir/gpu/ovir_gpu.h"
#include "../ovir/gpu/ovir_spirv.h"
#include "../ovir/gpu/ovir_gpu_ir.h"
#include "../ovir/gpu/ovir_gpu_opt.h"
#include "../ovir/gpu/ovir_gpu_backend.h"

// Test sample SPIR-V binary words for a simple Vertex Shader:
// - OpCapability Shader
// - OpMemoryModel Logical Simple
// - OpEntryPoint Vertex %main "main" %in_pos %out_color
// - Types: void, float, vec4, ptr_input_vec4, ptr_output_vec4
// - Vars: in_pos (loc 0), out_color (loc 0)
// - Constants: vec4(1.0, 0.5, 0.2, 1.0)
// - Instructions: %1 = Load %in_pos; %2 = FAdd %1, const; Store %out_color, %2; Return
static const uint32_t s_sample_vert_spirv[] = {
    // Header
    0x07230203, // Magic
    0x00010300, // Version 1.3
    0x00080001, // Generator
    25,         // Bound
    0,          // Schema

    // OpCapability Shader (count=2, op=17, cap=1)
    (2 << 16) | 17, 1,

    // OpMemoryModel Logical Simple (count=3, op=14, addr=0, mem=0)
    (3 << 16) | 14, 0, 0,

    // OpEntryPoint Vertex %4 "main" %9 %10 (count=6, op=15)
    (6 << 16) | 15, 0, 4, 0x6e69616d, 0, 9,

    // OpExecutionMode %4 OriginUpperLeft (count=3, op=16)
    (3 << 16) | 16, 4, 7,

    // OpDecorate %9 Location 0 (count=4, op=71, target=9, dec=30, loc=0)
    (4 << 16) | 71, 9, 30, 0,

    // OpDecorate %10 Location 0 (count=4, op=71, target=10, dec=30, loc=0)
    (4 << 16) | 71, 10, 30, 0,

    // OpTypeVoid %1 (count=2, op=19)
    (2 << 16) | 19, 1,

    // OpTypeFunction %2 %1 (count=3, op=33)
    (3 << 16) | 33, 2, 1,

    // OpTypeFloat %5 32 (count=3, op=22)
    (3 << 16) | 22, 5, 32,

    // OpTypeVector %6 %5 4 (count=4, op=23)
    (4 << 16) | 23, 6, 5, 4,

    // OpTypePointer %7 Input %6 (count=4, op=32, target=7, storage=1, type=6)
    (4 << 16) | 32, 7, 1, 6,

    // OpTypePointer %8 Output %6 (count=4, op=32, target=8, storage=3, type=6)
    (4 << 16) | 32, 8, 3, 6,

    // OpVariable %7 %9 Input (count=4, op=59, type=7, res=9, storage=1)
    (4 << 16) | 59, 7, 9, 1,

    // OpVariable %8 %10 Output (count=4, op=59, type=8, res=10, storage=3)
    (4 << 16) | 59, 8, 10, 3,

    // OpConstant %5 %11 1.0f (0x3f800000) (count=4, op=43, type=5, res=11, val=1.0)
    (4 << 16) | 43, 5, 11, 0x3f800000,

    // OpConstant %5 %12 0.5f (0x3f000000) (count=4, op=43, type=5, res=12, val=0.5)
    (4 << 16) | 43, 5, 12, 0x3f000000,

    // OpFunction %1 %4 None %2 (count=5, op=54)
    (5 << 16) | 54, 1, 4, 0, 2,

    // OpLabel %13 (count=2, op=248)
    (2 << 16) | 248, 13,

    // OpLoad %6 %14 %9 (count=4, op=61, type=6, res=14, ptr=9)
    (4 << 16) | 61, 6, 14, 9,

    // OpFAdd %6 %15 %14 %11 (count=5, op=129, type=6, res=15, a=14, b=11)
    (5 << 16) | 129, 6, 15, 14, 11,

    // OpStore %10 %15 (count=3, op=62, ptr=10, obj=15)
    (3 << 16) | 62, 10, 15,

    // OpReturn (count=1, op=253)
    (1 << 16) | 253,

    // OpFunctionEnd (count=1, op=56)
    (1 << 16) | 56
};

static void test_spirv_parser_and_validation(void) {
    printf("[Phase 14 Test 1] SPIR-V Parser & Validation...\n");

    // Test 1a: Invalid Magic rejection
    uint32_t bad_magic[5] = {0x12345678, 0x00010000, 0, 10, 0};
    ovir_spirv_module_t bad_mod;
    ov_status_t st = ovir_spirv_validate_header(bad_magic, 5, &bad_mod);
    assert(st == OV_ERR_VERIFICATION_FAILED);
    assert(!bad_mod.is_valid);
    printf("  [PASS] Invalid magic rejected properly.\n");

    // Test 1b: Truncated header rejection
    st = ovir_spirv_validate_header(bad_magic, 3, &bad_mod);
    assert(st == OV_ERR_VERIFICATION_FAILED);
    printf("  [PASS] Truncated header rejected.\n");

    // Test 1c: Valid sample shader parsing
    ovir_spirv_module_t valid_mod;
    st = ovir_spirv_parse(s_sample_vert_spirv, sizeof(s_sample_vert_spirv)/sizeof(uint32_t), &valid_mod);
    assert(st == OV_SUCCESS);
    assert(valid_mod.is_valid);
    assert(valid_mod.magic == OVIR_SPIRV_MAGIC);
    assert(valid_mod.entry_point_count == 1);
    assert(valid_mod.entry_points[0].exec_model == OVIR_SPIRV_EXEC_VERTEX);
    assert(valid_mod.variable_count >= 2);
    printf("  [PASS] Valid SPIR-V header and declarations parsed: %u entry points, %u variables.\n",
           valid_mod.entry_point_count, valid_mod.variable_count);
}

static void test_neutral_gpu_ir_generation(void) {
    printf("[Phase 14 Test 2] Architecture-Neutral OVIR-GPU IR Generation...\n");

    ovir_gpu_shader_ir_t ir;
    ov_status_t st = ovir_gpu_ir_from_spirv(s_sample_vert_spirv, sizeof(s_sample_vert_spirv)/sizeof(uint32_t), &ir);
    assert(st == OV_SUCCESS);
    assert(ir.is_valid);
    assert(ir.stage == OVIR_SPIRV_EXEC_VERTEX);
    assert(ir.instruction_count > 0);

    // Verify IR dump functionality
    char ir_disasm[2048];
    memset(ir_disasm, 0, sizeof(ir_disasm));
    st = ovir_gpu_ir_dump(&ir, ir_disasm, sizeof(ir_disasm));
    assert(st == OV_SUCCESS);
    printf("DEBUG DUMP: [%s]\n", ir_disasm);
    assert(strlen(ir_disasm) > 0);
    assert(strstr(ir_disasm, "OVIR-GPU") != NULL);
    assert(strstr(ir_disasm, "LOAD") != NULL);
    assert(strstr(ir_disasm, "ADD") != NULL);
    assert(strstr(ir_disasm, "RETURN") != NULL);

    printf("  [PASS] OVIR-GPU IR constructed with %u instructions.\n", ir.instruction_count);
}

static void test_ir_optimization_passes(void) {
    printf("[Phase 14 Test 3] GPU IR Optimization Passes (Constant Folding & DCE)...\n");

    ovir_gpu_shader_ir_t ir;
    ovir_gpu_ir_init(&ir, OVIR_SPIRV_EXEC_FRAGMENT, "main");

    // Types
    ovir_gpu_ir_add_type(&ir, 1, OVIR_GPU_TYPE_FLOAT32, 1, 32);

    // Constants: %2 = 2.0, %3 = 3.0
    ovir_gpu_ir_add_constant_f32(&ir, 2, 1, 2.0f);
    ovir_gpu_ir_add_constant_f32(&ir, 3, 1, 3.0f);

    // Instructions:
    // %4 = ADD %2, %3 (Constant Foldable -> 5.0f)
    ovir_gpu_ir_add_instruction(&ir, OVIR_GPU_OP_ADD, 4, 1, 2, 3);
    // %5 = MUL %2, %3 (Unused, Dead Code)
    ovir_gpu_ir_add_instruction(&ir, OVIR_GPU_OP_MUL, 5, 1, 2, 3);
    // %6 = RETURN
    ovir_gpu_ir_add_instruction(&ir, OVIR_GPU_OP_RETURN, 0, 0, 0, 0);
    // %7 = ADD %4, %2 (Unreachable after return)
    ovir_gpu_ir_add_instruction(&ir, OVIR_GPU_OP_ADD, 7, 1, 4, 2);

    uint32_t initial_insn_count = ir.instruction_count;
    assert(initial_insn_count == 4);

    ovir_gpu_opt_stats_t opt_stats;
    ov_status_t st = ovir_gpu_optimize(&ir, &opt_stats);
    assert(st == OV_SUCCESS);

    assert(opt_stats.constant_folds_applied >= 1);
    assert(opt_stats.dead_instructions_eliminated >= 1);
    assert(ir.instruction_count < initial_insn_count);

    printf("  [PASS] Optimizer folded %u constant ops and eliminated %u dead instructions.\n",
           opt_stats.constant_folds_applied, opt_stats.dead_instructions_eliminated);
}

static void test_glsl_translation_and_compilation(void) {
    printf("[Phase 14 Test 4] Target Translation: SPIR-V -> OVIR-GPU IR -> GLSL 330 Core...\n");

    ovir_gpu_shader_ir_t ir;
    ov_status_t st = ovir_gpu_ir_from_spirv(s_sample_vert_spirv, sizeof(s_sample_vert_spirv)/sizeof(uint32_t), &ir);
    assert(st == OV_SUCCESS);

    ovir_gpu_compiled_shader_t compiled;
    st = ovir_gpu_translate_to_glsl(&ir, &compiled);
    assert(st == OV_SUCCESS);
    assert(compiled.target_backend == OV_GFX_BACKEND_OPENGL_CORE);
    assert(compiled.glsl_version == 330);
    assert(strstr(compiled.source_code, "#version 330 core") != NULL);
    assert(strstr(compiled.source_code, "void main()") != NULL);

    // Verify simulated driver compilation
    st = ovir_gpu_backend_compile(&compiled, true);
    assert(st == OV_SUCCESS);
    assert(compiled.compilation_passed);
    assert(strstr(compiled.compiler_log, "compiled successfully") != NULL);

    printf("  [PASS] GLSL 330 emitted cleanly (%zu bytes) and passed compilation verification.\n", compiled.source_length);
}

static void test_operation_reconstruction(void) {
    printf("[Phase 14 Test 5] GPU Operation Reconstruction (Matrix-Vector Multiply)...\n");

    ovir_gpu_shader_ir_t ir;
    ovir_gpu_ir_init(&ir, OVIR_SPIRV_EXEC_VERTEX, "main");
    ovir_gpu_ir_add_type(&ir, 1, OVIR_GPU_TYPE_MAT4, 4, 32);
    ovir_gpu_ir_add_type(&ir, 2, OVIR_GPU_TYPE_VEC4, 4, 32);

    // Add MAT_MUL_VEC instruction
    ovir_gpu_ir_add_instruction(&ir, OVIR_GPU_OP_MAT_MUL_VEC, 10, 2, 1, 2);

    uint32_t reconstructed = 0;
    ov_status_t st = ovir_gpu_reconstruct_operation(&ir, OVIR_GPU_OP_MAT_MUL_VEC, &reconstructed);
    assert(st == OV_SUCCESS);
    assert(reconstructed == 1);

    // Verify that the instruction was reconstructed to dot-product decomposition
    assert(ir.instructions[0].opcode == OVIR_GPU_OP_DOT);
    printf("  [PASS] MAT_MUL_VEC decomposed into %u primitive reconstruction pass.\n", reconstructed);
}

static void test_pipeline_integration_and_telemetry(void) {
    printf("[Phase 14 Test 6] Full Pipeline Translation & Telemetry Tracking...\n");

    ov_telemetry_reset_runtime_stats();

    ov_gpu_host_caps_t caps = {
        .backend = OV_GFX_BACKEND_OPENGL_CORE,
        .supported_features = 0xFFFF,
        .partial_features = 0x0008, // Requests reconstruction
        .available_vram_mb = 1024,
        .has_opengl = true,
        .cpu_fallback_supported = true
    };

    ovir_gpu_compiled_shader_t compiled;
    ovir_gpu_dispatch_result_t result;

    ov_status_t st = ovir_gpu_translate_spirv_pipeline(
        s_sample_vert_spirv,
        sizeof(s_sample_vert_spirv)/sizeof(uint32_t),
        &caps,
        true,
        &compiled,
        &result
    );

    assert(st == OV_SUCCESS);
    assert(compiled.compilation_passed);
    assert(result.selected_backend == OV_GFX_BACKEND_OPENGL_CORE);
    assert(result.draw_calls_emitted == 1);

    const ov_runtime_telemetry_t *stats = ov_telemetry_get_runtime_stats();
    assert(stats->gpu_shaders_parsed == 1);
    assert(stats->gpu_shaders_translated == 1);
    assert(stats->gpu_shaders_compiled == 1);

    printf("  [PASS] Pipeline executed cleanly. Telemetry recorded: parsed=%llu, translated=%llu, compiled=%llu.\n",
           (unsigned long long)stats->gpu_shaders_parsed,
           (unsigned long long)stats->gpu_shaders_translated,
           (unsigned long long)stats->gpu_shaders_compiled);
}

int main(void) {
    printf("====================================================================\n");
    printf("  OPENVINTAGE PHASE 14: REAL OVIR-GPU SHADER TRANSLATION TEST SUITE \n");
    printf("====================================================================\n");

    test_spirv_parser_and_validation();
    test_neutral_gpu_ir_generation();
    test_ir_optimization_passes();
    test_glsl_translation_and_compilation();
    test_operation_reconstruction();
    test_pipeline_integration_and_telemetry();

    printf("\n>>> [ALL PHASE 14 GPU ASSERTIONS PASSED SUCCESSFULLY] <<<\n");
    return 0;
}
