#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "ovir_cpu.h"
#include "ovir_cpu_decoder.h"
#include "ovir_cpu_backend.h"
#include "ov_os_scale.h"
#include "ov_runtime.h"

static int g_tests_run = 0;
static int g_tests_passed = 0;

#define TEST_ASSERT(cond, msg) do { \
    g_tests_run++; \
    if (cond) { \
        g_tests_passed++; \
        printf("  [PASS] %s\n", msg); \
    } else { \
        printf("  [FAIL] %s (Line %d: %s)\n", msg, __LINE__, #cond); \
    } \
} while(0)

// 1. Decoder Documented Subset
static void test_decoder_documented_subset(void) {
    printf("--- Test: ARM64 Instruction Decoder Documented Subset ---\n");

    ov_decoded_insn_t insn;
    ov_cpu_decode_diag_t diag;

    // NOP: 0xd503201f
    ov_decode_status_t st = ovir_cpu_decode_single(0xd503201f, 0x1000, &insn, &diag);
    TEST_ASSERT(st == OV_DECODE_SUCCESS && insn.opcode == OVIR_OP_NOP, "Decode NOP");

    // RET: 0xd65f03c0
    st = ovir_cpu_decode_single(0xd65f03c0, 0x1004, &insn, &diag);
    TEST_ASSERT(st == OV_DECODE_SUCCESS && insn.opcode == OVIR_OP_RET && insn.src_reg1 == 30, "Decode RET X30");

    // MOVZ W0, #42 -> 0x52800540 (42 << 5 = 0x540)
    st = ovir_cpu_decode_single(0x52800540, 0x1008, &insn, &diag);
    TEST_ASSERT(st == OV_DECODE_SUCCESS && insn.opcode == OVIR_OP_MOV_IMM && insn.dst_reg == 0 && insn.imm_val == 42, "Decode MOVZ W0, #42");

    // MOV X0, X1 (ORR X0, XZR, X1) -> 0xaa0103e0
    st = ovir_cpu_decode_single(0xaa0103e0, 0x100c, &insn, &diag);
    TEST_ASSERT(st == OV_DECODE_SUCCESS && insn.opcode == OVIR_OP_MOV_REG && insn.dst_reg == 0 && insn.src_reg1 == 1, "Decode MOV X0, X1");

    // ADD X0, X1, X2 -> 0x8b020020
    st = ovir_cpu_decode_single(0x8b020020, 0x1010, &insn, &diag);
    TEST_ASSERT(st == OV_DECODE_SUCCESS && insn.opcode == OVIR_OP_ADD && insn.dst_reg == 0 && insn.src_reg1 == 1 && insn.src_reg2 == 2, "Decode ADD X0, X1, X2");

    // SUB X3, X4, X5 -> 0x4b050083
    st = ovir_cpu_decode_single(0x4b050083, 0x1014, &insn, &diag);
    TEST_ASSERT(st == OV_DECODE_SUCCESS && insn.opcode == OVIR_OP_SUB && insn.dst_reg == 3 && insn.src_reg1 == 4 && insn.src_reg2 == 5, "Decode SUB X3, X4, X5");

    // MUL X0, X1, X2 -> 0x9b027c20
    st = ovir_cpu_decode_single(0x9b027c20, 0x1018, &insn, &diag);
    TEST_ASSERT(st == OV_DECODE_SUCCESS && insn.opcode == OVIR_OP_MUL && insn.dst_reg == 0, "Decode MUL X0, X1, X2");

    // Logical AND X0, X1, X2 -> 0x8a020020
    st = ovir_cpu_decode_single(0x8a020020, 0x101c, &insn, &diag);
    TEST_ASSERT(st == OV_DECODE_SUCCESS && insn.opcode == OVIR_OP_AND, "Decode AND X0, X1, X2");

    // Logical ORR X0, X1, X2 -> 0xaa020020
    st = ovir_cpu_decode_single(0xaa020020, 0x1020, &insn, &diag);
    TEST_ASSERT(st == OV_DECODE_SUCCESS && insn.opcode == OVIR_OP_ORR, "Decode ORR X0, X1, X2");

    // Logical EOR X0, X1, X2 -> 0xca020020
    st = ovir_cpu_decode_single(0xca020020, 0x1024, &insn, &diag);
    TEST_ASSERT(st == OV_DECODE_SUCCESS && insn.opcode == OVIR_OP_EOR, "Decode EOR X0, X1, X2");

    // Compare CMP X0, X1 (SUBS XZR, X0, X1) -> 0xeb01001f
    st = ovir_cpu_decode_single(0xeb01001f, 0x1028, &insn, &diag);
    TEST_ASSERT(st == OV_DECODE_SUCCESS && insn.opcode == OVIR_OP_CMP && insn.sets_flags, "Decode CMP X0, X1");

    // LDR X0, [X1, #16] -> 0xf9400820 (imm12 = 2, 2 * 8 = 16)
    st = ovir_cpu_decode_single(0xf9400820, 0x102c, &insn, &diag);
    TEST_ASSERT(st == OV_DECODE_SUCCESS && insn.opcode == OVIR_OP_LOAD && insn.dst_reg == 0 && insn.src_reg1 == 1 && insn.mem_offset == 16, "Decode LDR X0, [X1, #16]");

    // STR X2, [X3, #8] -> 0xf9000462 (imm12 = 1, 1 * 8 = 8)
    st = ovir_cpu_decode_single(0xf9000462, 0x1030, &insn, &diag);
    TEST_ASSERT(st == OV_DECODE_SUCCESS && insn.opcode == OVIR_OP_STORE && insn.dst_reg == 2 && insn.src_reg1 == 3 && insn.mem_offset == 8, "Decode STR X2, [X3, #8]");

    // Branch B.EQ #+16 -> 0x54000080 (imm19 = 4, 4 * 4 = 16, cond = 0 EQ)
    st = ovir_cpu_decode_single(0x54000080, 0x1034, &insn, &diag);
    TEST_ASSERT(st == OV_DECODE_SUCCESS && insn.opcode == OVIR_OP_JMP_COND && insn.cond == OVIR_CC_EQ && insn.branch_target_disp == 16, "Decode B.EQ #+16");

    // Unconditional Branch B #+32 -> 0x14000008 (imm26 = 8, 8 * 4 = 32)
    st = ovir_cpu_decode_single(0x14000008, 0x1038, &insn, &diag);
    TEST_ASSERT(st == OV_DECODE_SUCCESS && insn.opcode == OVIR_OP_JMP && insn.branch_target_disp == 32, "Decode B #+32");

    // System call SVC #0 -> 0xd4000001
    st = ovir_cpu_decode_single(0xd4000001, 0x103c, &insn, &diag);
    TEST_ASSERT(st == OV_DECODE_SUCCESS && insn.opcode == OVIR_OP_SYSCALL, "Decode SVC #0");
}

// 2. Decoder Diagnostics on Invalid/Unsupported
static void test_decoder_invalid_and_diagnostics(void) {
    printf("--- Test: Decoder Structured Diagnostics & Invalid Encodings ---\n");

    ov_decoded_insn_t insn;
    ov_cpu_decode_diag_t diag;

    // Unknown instruction 0x00000000 (UDF)
    ov_decode_status_t st = ovir_cpu_decode_single(0x00000000, 0x4000, &insn, &diag);
    TEST_ASSERT(st == OV_DECODE_ERR_UNKNOWN_OPCODE, "Reject unknown opcode 0x00000000");
    TEST_ASSERT(diag.error_code == OV_DECODE_ERR_UNKNOWN_OPCODE, "Diagnostic reports unknown opcode error code");
    TEST_ASSERT(diag.guest_pc == 0x4000, "Diagnostic records accurate guest PC");
    TEST_ASSERT(diag.raw_insn == 0x00000000, "Diagnostic records raw instruction bits");
    TEST_ASSERT(strlen(diag.description) > 0, "Diagnostic provides readable description");
    TEST_ASSERT(strlen(diag.suggested_fallback) > 0, "Diagnostic provides suggested fallback route");
}

// 3. IR Validation & Reflection
static void test_ir_validation_and_reflection(void) {
    printf("--- Test: Architecture-Neutral IR Validation & Reflection ---\n");

    const uint32_t raw_seq[] = {
        0x52800540, // MOVZ W0, #42
        0x52800741, // MOVZ W1, #58
        0x8b010000, // ADD X0, X0, X1
        0xd65f03c0  // RET
    };

    ovir_ir_block_t ir;
    ov_cpu_decode_diag_t diag;
    ov_status_t status = ovir_cpu_decode_sequence(raw_seq, 4, 0x2000, &ir, &diag);
    TEST_ASSERT(status == OV_SUCCESS && ir.count == 4, "Decode sequence into IR block");

    char err_buf[128];
    status = ovir_cpu_ir_validate(&ir, err_buf, sizeof(err_buf));
    TEST_ASSERT(status == OV_SUCCESS, "IR validation succeeds for well-formed block");

    // Corrupt an instruction with invalid register > 31
    ir.instructions[0].dst_reg = 35;
    status = ovir_cpu_ir_validate(&ir, err_buf, sizeof(err_buf));
    TEST_ASSERT(status != OV_SUCCESS, "IR validation catches out-of-bounds register");
    TEST_ASSERT(strstr(err_buf, "out of bounds") != NULL, "IR validation produces detailed error message");
    ir.instructions[0].dst_reg = 0; // restore

    char dump_buf[512];
    int len = ovir_cpu_ir_dump(&ir, dump_buf, sizeof(dump_buf));
    TEST_ASSERT(len > 0, "IR disassembly dumped to buffer");
    TEST_ASSERT(strstr(dump_buf, "MOV_IMM") != NULL, "Dump contains MOV_IMM mnemonic");
    TEST_ASSERT(strstr(dump_buf, "RET") != NULL, "Dump contains RET mnemonic");
}

// 4. IR Optimization
static void test_ir_optimization(void) {
    printf("--- Test: IR Optimization (Constant Folding & Dead Code Elimination) ---\n");

    const uint32_t raw_seq[] = {
        0x52800540, // MOVZ W0, #42
        0x52800741, // MOVZ W1, #58
        0x8b010000, // ADD X0, X0, X1 (42 + 58 = 100)
        0xd65f03c0, // RET
        0x52800142  // MOVZ W2, #10 (Dead code after RET!)
    };

    ovir_ir_block_t ir;
    ov_cpu_decode_diag_t diag;
    ov_status_t status = ovir_cpu_decode_sequence(raw_seq, 5, 0x1000, &ir, &diag);
    TEST_ASSERT(status == OV_SUCCESS, "Decode 5-instruction block");

    uint32_t opts = 0;
    status = ovir_cpu_optimize(&ir, &opts);
    TEST_ASSERT(status == OV_SUCCESS && opts >= 2, "Apply optimizations (folded constant + dead code)");
    TEST_ASSERT(ir.instructions[2].opcode == OVIR_OP_MOV_IMM && ir.instructions[2].imm_val == 100, "ADD folded to MOV_IMM 100");
    TEST_ASSERT(ir.instructions[4].opcode == OVIR_OP_NOP, "Instruction after RET converted to NOP (dead code)");
}

// 5. Backend Emitter & Machine Code Verification
static void test_backend_emitter_and_verification(void) {
    printf("--- Test: Modular Backend & x86-64 Machine Code Emitter ---\n");

    const ovir_backend_t *backend = ovir_cpu_get_backend(OV_ARCH_ARM64, OV_ARCH_X86_64);
    TEST_ASSERT(backend != NULL, "Query registered ARM64 -> x86-64 backend");
    TEST_ASSERT((backend->capabilities_mask & OVIR_BACKEND_CAP_ARITHMETIC) != 0, "Backend supports arithmetic");

    const uint32_t raw_seq[] = {
        0x52800540, // MOVZ W0, #42
        0x8b000000, // ADD X0, X0, X0
        0xd65f03c0  // RET
    };

    ovir_ir_block_t ir;
    ov_cpu_decode_diag_t diag;
    ovir_cpu_decode_sequence(raw_seq, 3, 0x1000, &ir, &diag);

    ovir_compiled_block_t compiled;
    ov_status_t status = backend->emit(&ir, &compiled);
    TEST_ASSERT(status == OV_SUCCESS, "Backend emit x86-64 machine code");
    TEST_ASSERT(compiled.verified, "Machine code is verified");
    TEST_ASSERT(compiled.code_size > 0, "Compiled code size > 0");
    TEST_ASSERT(compiled.code_bytes[compiled.code_size - 1] == 0xc3, "Code terminates with x86 RET (0xc3)");
}

// 6. Memory Load and Store Simulation
static void test_memory_load_store(void) {
    printf("--- Test: Memory Load and Store Translation & Simulation ---\n");

    ovir_ir_block_t ir;
    memset(&ir, 0, sizeof(ir));
    ir.source_arch = OV_ARCH_ARM64;

    // MOV X0, #0x100 (base address)
    ir.instructions[ir.count].opcode = OVIR_OP_MOV_IMM;
    ir.instructions[ir.count].dst_reg = 0;
    ir.instructions[ir.count++].imm_val = 0x100;

    // MOV X1, #0xdeadbeef
    ir.instructions[ir.count].opcode = OVIR_OP_MOV_IMM;
    ir.instructions[ir.count].dst_reg = 1;
    ir.instructions[ir.count++].imm_val = 0xdeadbeef;

    // STORE X1, [X0 + #8]
    ir.instructions[ir.count].opcode = OVIR_OP_STORE;
    ir.instructions[ir.count].dst_reg = 1;
    ir.instructions[ir.count].src_reg1 = 0;
    ir.instructions[ir.count].offset = 8;
    ir.instructions[ir.count++].op_size = 8;

    // LOAD X2, [X0 + #8]
    ir.instructions[ir.count].opcode = OVIR_OP_LOAD;
    ir.instructions[ir.count].dst_reg = 2;
    ir.instructions[ir.count].src_reg1 = 0;
    ir.instructions[ir.count].offset = 8;
    ir.instructions[ir.count++].op_size = 8;

    // RET
    ir.instructions[ir.count++].opcode = OVIR_OP_RET;

    ovir_cpu_state_t state;
    memset(&state, 0, sizeof(state));

    ov_status_t status = ovir_cpu_simulate(&ir, &state);
    TEST_ASSERT(status == OV_SUCCESS, "Simulate load/store block");
    TEST_ASSERT(state.registers[2] == 0xdeadbeef, "Value stored to memory correctly loaded into X2 (0xdeadbeef)");
}

// 7. Real W^X Native JIT Execution on Host x86-64
static void test_native_jit_execution_wx(void) {
    printf("--- Test: Real W^X Native JIT Execution on Host x86-64 ---\n");

    const uint32_t raw_seq[] = {
        0x52800540, // MOVZ W0, #42
        0x52800741, // MOVZ W1, #58
        0x8b010000, // ADD X0, X0, X1 (42 + 58 = 100)
        0xd65f03c0  // RET
    };

    ovir_ir_block_t ir;
    ov_cpu_decode_diag_t diag;
    ovir_cpu_decode_sequence(raw_seq, 4, 0x1000, &ir, &diag);

    ovir_compiled_block_t compiled;
    ov_status_t st = ovir_cpu_emit_x86_64(&ir, &compiled);
    TEST_ASSERT(st == OV_SUCCESS && compiled.verified, "Compile block for native execution");

    ovir_cpu_state_t state;
    memset(&state, 0, sizeof(state));

    st = ovir_cpu_execute_native(&compiled, &state);
#if defined(__x86_64__) || defined(_M_X64)
    TEST_ASSERT(st == OV_SUCCESS, "Native W^X JIT execution succeeded on x86-64");
    TEST_ASSERT(state.registers[0] == 100, "Native JIT produced expected register result X0 == 100");
    TEST_ASSERT(state.instructions_retired >= 4, "Instructions retired correctly tracked");
#else
    TEST_ASSERT(st == OV_ERR_UNSUPPORTED, "Non-x86 host gracefully signals unsupported for native JIT");
#endif
}

// 8. OS-Scale Extension Points
static void test_os_scale_extensions(void) {
    printf("--- Test: OS-Scale Extension Points (Privilege Rings, Syscalls, SIMD) ---\n");

    // Privilege rings
    TEST_ASSERT(ov_check_privilege(OV_RING_HYPERVISOR, OV_RING_USER), "Hypervisor has User privilege");
    TEST_ASSERT(ov_check_privilege(OV_RING_SUPERVISOR, OV_RING_USER), "Supervisor has User privilege");
    TEST_ASSERT(!ov_check_privilege(OV_RING_USER, OV_RING_SUPERVISOR), "User does NOT have Supervisor privilege");

    // Syscall table
    ov_syscall_table_t syscalls;
    ov_syscall_table_init(&syscalls, NULL);

    int64_t ret = ov_syscall_dispatch(&syscalls, OV_SYS_WRITE, 1, 0x1000, 64, 0);
    TEST_ASSERT(ret == 64, "Dispatch standard sys_write syscall");

    ret = ov_syscall_dispatch(&syscalls, OV_SYS_EXIT, 42, 0, 0, 0);
    TEST_ASSERT(ret == 42, "Dispatch standard sys_exit syscall");

    ret = ov_syscall_dispatch(&syscalls, 250, 0, 0, 0, 0);
    TEST_ASSERT(ret == -38, "Dispatch unregistered syscall returns -ENOSYS");

    // SIMD Vector Operations
    ovir_vec128_t v1, v2, v_res;
    for (int i = 0; i < 4; i++) {
        v1.u32[i] = 10 * (i + 1);
        v2.u32[i] = 5 * (i + 1);
    }

    ov_vec128_add_u32(&v_res, &v1, &v2);
    TEST_ASSERT(v_res.u32[0] == 15 && v_res.u32[1] == 30 && v_res.u32[2] == 45 && v_res.u32[3] == 60, "SIMD 128-bit 4x32 integer addition");

    ov_vec128_sub_u32(&v_res, &v1, &v2);
    TEST_ASSERT(v_res.u32[0] == 5 && v_res.u32[1] == 10 && v_res.u32[2] == 15 && v_res.u32[3] == 20, "SIMD 128-bit 4x32 integer subtraction");
}

// 9. Runtime TB Lifecycle & Invalidation Integration
static void test_cache_and_runtime_integration(void) {
    printf("--- Test: Runtime TB Lifecycle & Invalidation Integration ---\n");

    ov_runtime_t rt;
    ov_runtime_init(&rt);

    const uint32_t raw_seq[] = {
        0x52800540, // MOVZ W0, #42
        0xd65f03c0  // RET
    };

    ov_translation_block_t *tb = ov_runtime_create_tb(&rt, 0x1000, 0x1008, OV_ARCH_ARM64, OV_ARCH_X86_64, 0xabcdef1234567890ULL);
    TEST_ASSERT(tb != NULL, "Runtime creates translation block");
    TEST_ASSERT(tb->state == OV_TB_STATE_CREATED, "Initial TB state is CREATED");

    ov_status_t st = ov_runtime_compile_tb(tb, raw_seq, 2);
    TEST_ASSERT(st == OV_SUCCESS, "Compile TB into executable form");
    TEST_ASSERT(tb->state == OV_TB_STATE_VALID, "Compiled TB state is VALID");

    ov_cpu_context_t ctx;
    ov_cpu_context_init(&ctx, OV_ARCH_ARM64, 1);

    st = ov_runtime_execute_tb(&rt, tb, &ctx, NULL);
    TEST_ASSERT(st == OV_SUCCESS, "Execute TB through runtime dispatcher");
    TEST_ASSERT(ctx.gpr[0] == 42, "TB execution sets GPR0 = 42");
    TEST_ASSERT(tb->execution_count == 1, "TB execution count incremented");

    // Test invalidation
    st = ov_runtime_invalidate_by_hash(&rt, 0xabcdef1234567890ULL);
    TEST_ASSERT(st == OV_SUCCESS, "Invalidate TB by source hash");
    TEST_ASSERT(tb->state == OV_TB_STATE_INVALIDATED, "TB state transitions to INVALIDATED");
}

int main(void) {
    printf("====================================================\n");
    printf("OPENVINTAGE PHASE 12: REAL OVIR-CPU TEST SUITE\n");
    printf("====================================================\n");

    test_decoder_documented_subset();
    test_decoder_invalid_and_diagnostics();
    test_ir_validation_and_reflection();
    test_ir_optimization();
    test_backend_emitter_and_verification();
    test_memory_load_store();
    test_native_jit_execution_wx();
    test_os_scale_extensions();
    test_cache_and_runtime_integration();

    printf("\n====================================================\n");
    printf("Phase 12 Results: %d/%d assertions passed\n", g_tests_passed, g_tests_run);
    printf("====================================================\n");

    return (g_tests_passed == g_tests_run) ? 0 : 1;
}
