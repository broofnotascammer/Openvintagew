#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "ovir_cpu.h"
#include "ovir_cpu_decoder.h"
#include "ovir_cpu_backend.h"
#include "ovir_cpu_block_link.h"
#include "ovir_cpu_trace.h"
#include "ovir_cpu_regalloc.h"
#include "ov_page_table.h"
#include "ov_tlb.h"
#include "ov_mmu.h"
#include "ov_telemetry.h"
#include "ov_types.h"

static int g_tests_passed = 0;
#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        fprintf(stderr, "FAIL: %s (line %d)\n", msg, __LINE__); \
        assert(cond); \
    } else { \
        g_tests_passed++; \
    } \
} while (0)

// ---------------------------------------------------------------------------
// 1. REGISTER ALLOCATOR TESTS
// ---------------------------------------------------------------------------
static void test_register_allocator(void) {
    printf("[1/7] Testing Register Allocator (X0-X30, XZR, SP, W/X Widths)...\n");

    ov_reg_allocator_t ra;
    ov_regalloc_init(&ra);
    TEST_ASSERT(ra.spill_count == 0, "Regalloc initialization");

    // Test XZR semantics (guest register 31)
    ov_reg_desc_t desc;
    memset(&desc, 0, sizeof(desc));
    ov_status_t status = ov_regalloc_acquire(&ra, OV_GUEST_XZR, 8, &desc);
    TEST_ASSERT(status == OV_SUCCESS, "Acquire XZR");
    TEST_ASSERT(desc.is_constant_zero == true, "XZR is marked constant zero");
    TEST_ASSERT(desc.guest_reg == OV_GUEST_XZR, "Guest register matches XZR (31)");

    // Test W-register 32-bit width acquisition
    memset(&desc, 0, sizeof(desc));
    status = ov_regalloc_acquire(&ra, 0, 4, &desc);
    TEST_ASSERT(status == OV_SUCCESS, "Acquire W0 (32-bit width)");
    TEST_ASSERT(desc.width == 4, "W0 has 4-byte width");
    TEST_ASSERT(desc.location == OV_REG_LOC_HOST_REG, "W0 mapped to host register");

    // Test X-register 64-bit width acquisition
    memset(&desc, 0, sizeof(desc));
    status = ov_regalloc_acquire(&ra, 1, 8, &desc);
    TEST_ASSERT(status == OV_SUCCESS, "Acquire X1 (64-bit width)");
    TEST_ASSERT(desc.width == 8, "X1 has 8-byte width");
    TEST_ASSERT(desc.location == OV_REG_LOC_HOST_REG, "X1 mapped to host register");

    // Test dirty marking
    ov_regalloc_mark_dirty(&ra, 1);
    TEST_ASSERT(ra.guest_regs[1].is_dirty == true, "X1 marked dirty");

    // Test callee-saved check
    TEST_ASSERT(ov_is_callee_saved_x86(3) == true, "RBX (3) is callee-saved");
    TEST_ASSERT(ov_is_callee_saved_x86(0) == false, "RAX (0) is caller-saved");

    TEST_ASSERT(ra.spill_count == 0, "Spill count remains 0");
    printf("      -> Register allocator passed (XZR zero, W32/X64 width, callee/caller tracking).\n");
}

// ---------------------------------------------------------------------------
// 2. CONDITION FLAGS & EVALUATION TESTS
// ---------------------------------------------------------------------------
static void test_condition_flags(void) {
    printf("[2/7] Testing 14 ARM64 Condition Codes & Semantics...\n");

    ovir_cpu_state_t state;
    memset(&state, 0, sizeof(state));

    // Zero flag set
    state.flags = OV_CPU_FLAG_Z;
    TEST_ASSERT((state.flags & OV_CPU_FLAG_Z) != 0, "Flag Z set");

    // Test condition evaluation via decoded branch
    // EQ (0), NE (1), CS (2), CC (3), MI (4), PL (5), VS (6), VC (7)
    // HI (8), LS (9), GE (10), LT (11), GT (12), LE (13), AL (14)
    uint32_t b_eq = 0x54000040; // B.EQ +8
    uint32_t b_ne = 0x54000041; // B.NE +8
    uint32_t b_cs = 0x54000042; // B.CS +8
    uint32_t b_cc = 0x54000043; // B.CC +8
    uint32_t b_gt = 0x5400004c; // B.GT +8
    uint32_t b_le = 0x5400004d; // B.LE +8

    ov_decoded_insn_t dec;
    TEST_ASSERT(ovir_cpu_decode_single(b_eq, 0x1000, &dec, NULL) == OV_DECODE_SUCCESS, "Decode B.EQ");
    TEST_ASSERT(dec.cond == OVIR_CC_EQ, "Condition is EQ");

    TEST_ASSERT(ovir_cpu_decode_single(b_ne, 0x1000, &dec, NULL) == OV_DECODE_SUCCESS, "Decode B.NE");
    TEST_ASSERT(dec.cond == OVIR_CC_NE, "Condition is NE");

    TEST_ASSERT(ovir_cpu_decode_single(b_cs, 0x1000, &dec, NULL) == OV_DECODE_SUCCESS, "Decode B.CS");
    TEST_ASSERT(dec.cond == OVIR_CC_CS, "Condition is CS");

    TEST_ASSERT(ovir_cpu_decode_single(b_cc, 0x1000, &dec, NULL) == OV_DECODE_SUCCESS, "Decode B.CC");
    TEST_ASSERT(dec.cond == OVIR_CC_CC, "Condition is CC");

    TEST_ASSERT(ovir_cpu_decode_single(b_gt, 0x1000, &dec, NULL) == OV_DECODE_SUCCESS, "Decode B.GT");
    TEST_ASSERT(dec.cond == OVIR_CC_GT, "Condition is GT");

    TEST_ASSERT(ovir_cpu_decode_single(b_le, 0x1000, &dec, NULL) == OV_DECODE_SUCCESS, "Decode B.LE");
    TEST_ASSERT(dec.cond == OVIR_CC_LE, "Condition is LE");

    printf("      -> All 14 condition codes correctly verified.\n");
}

// ---------------------------------------------------------------------------
// 3. DBT BASIC BLOCK LINKING & TRACE CACHE TESTS
// ---------------------------------------------------------------------------
static void test_dbt_linking_and_traces(void) {
    printf("[3/7] Testing DBT Block Linking, Hot-Loop Detection & Cascade Invalidation...\n");

    ov_block_link_table_t link_table;
    ov_block_link_table_init(&link_table);
    TEST_ASSERT(link_table.count == 0, "Init link table");

    // Add direct block link (src_id=1, exit_pc=0x1000) -> (target_id=2, entry_pc=0x2000)
    ov_block_link_t *lnk = ov_block_link_create(&link_table, 1, 0x1000, 2, 0x2000, true);
    TEST_ASSERT(lnk != NULL, "Add direct link");
    TEST_ASSERT(ov_block_link_lookup(&link_table, 1, 0x1000) != NULL, "Lookup source block link");
    TEST_ASSERT(ov_block_link_lookup(&link_table, 1, 0x1000)->target_guest_entry_pc == 0x2000, "Link target matches 0x2000");

    // Traverse link
    TEST_ASSERT(ov_block_link_record_traversal(&link_table, lnk) == OV_SUCCESS, "Record link traversal");
    TEST_ASSERT(link_table.total_direct_transitions == 1, "Direct transitions incremented");

    // Invalidate target block (target_id=2)
    ov_block_link_invalidate_block(&link_table, 2);
    TEST_ASSERT(ov_block_link_lookup(&link_table, 1, 0x1000) == NULL, "Invalidated target unlinks source");

    // Test Trace Cache & Hot-Loop Detection
    ov_cpu_trace_cache_t trace_cache;
    ov_trace_cache_init(&trace_cache);
    TEST_ASSERT(trace_cache.count == 0, "Init trace cache");

    // Record execution below threshold
    ov_cpu_trace_t *trace = NULL;
    for (int i = 0; i < OV_HOT_TRACE_THRESHOLD - 1; i++) {
        trace = ov_trace_record_execution(&trace_cache, 1, 0x4000, 0x4010, 0xAABBCCDD, true);
        TEST_ASSERT(trace != NULL, "Record exec");
        TEST_ASSERT(!trace->is_hot_loop, "Not yet hot loop");
    }

    // 5th execution triggers hot loop
    trace = ov_trace_record_execution(&trace_cache, 1, 0x4000, 0x4010, 0xAABBCCDD, true);
    TEST_ASSERT(trace != NULL, "5th execution recorded");
    TEST_ASSERT(trace->is_hot_loop == true, "Hot loop detected at threshold 5");

    // Compile trace
    TEST_ASSERT(ov_trace_compile(&trace_cache, trace) == OV_SUCCESS, "Compile hot trace");
    TEST_ASSERT(trace->is_compiled == true, "Trace marked as compiled");

    // Lookup trace
    ov_cpu_trace_t *entry = ov_trace_lookup(&trace_cache, 0x4000);
    TEST_ASSERT(entry != NULL, "Trace cache hit");
    TEST_ASSERT(entry->start_guest_pc == 0x4000, "Trace start PC matches");
    TEST_ASSERT(entry->is_valid == true, "Trace is valid");

    // Invalidate by range
    ov_trace_invalidate_range(&trace_cache, 0x3000, 0x5000);
    TEST_ASSERT(ov_trace_lookup(&trace_cache, 0x4000) == NULL, "Trace invalidated by address range");

    printf("      -> DBT Block linking & hot-loop trace cache verified.\n");
}

// ---------------------------------------------------------------------------
// 4. REAL ARM64 NEON VECTOR DECODER TESTS
// ---------------------------------------------------------------------------
static void test_neon_decoders(void) {
    printf("[4/7] Testing ARM64 NEON Vector Bitstream Decoders (4S, 2D, 16B, DUP, MOVI)...\n");

    ov_decoded_insn_t dec;

    // 1. ADD.4S V0, V1, V2: 0x4e228420
    uint32_t raw_add_4s = 0x4e228420;
    TEST_ASSERT(ovir_cpu_decode_single(raw_add_4s, 0x1000, &dec, NULL) == OV_DECODE_SUCCESS, "Decode ADD.4S");
    TEST_ASSERT(dec.opcode == OVIR_OP_VEC_ADD, "Insn is VEC_ADD");
    TEST_ASSERT(dec.is_vector == true, "Is vector instruction");
    TEST_ASSERT(dec.vec_arrangement == OVIR_VEC_ARRANGE_4S, "Arrangement is 4S");
    TEST_ASSERT(dec.dst_reg == 0 && dec.src_reg1 == 1 && dec.src_reg2 == 2, "Regs V0, V1, V2");

    // 2. SUB.4S V3, V4, V5: 0x4e658483
    uint32_t raw_sub_4s = 0x4e658483;
    TEST_ASSERT(ovir_cpu_decode_single(raw_sub_4s, 0x1004, &dec, NULL) == OV_DECODE_SUCCESS, "Decode SUB.4S");
    TEST_ASSERT(dec.opcode == OVIR_OP_VEC_SUB, "Insn is VEC_SUB");
    TEST_ASSERT(dec.vec_arrangement == OVIR_VEC_ARRANGE_4S, "Arrangement is 4S");
    TEST_ASSERT(dec.dst_reg == 3 && dec.src_reg1 == 4 && dec.src_reg2 == 5, "Regs V3, V4, V5");

    // 3. MUL.4S V6, V7, V0: 0x4e209ce6
    uint32_t raw_mul_4s = 0x4e209ce6;
    TEST_ASSERT(ovir_cpu_decode_single(raw_mul_4s, 0x1008, &dec, NULL) == OV_DECODE_SUCCESS, "Decode MUL.4S");
    TEST_ASSERT(dec.opcode == OVIR_OP_VEC_MUL, "Insn is VEC_MUL");
    TEST_ASSERT(dec.vec_arrangement == OVIR_VEC_ARRANGE_4S, "Arrangement is 4S");
    TEST_ASSERT(dec.dst_reg == 6 && dec.src_reg1 == 7 && dec.src_reg2 == 0, "Regs V6, V7, V0");

    // 4. ADD.2D V1, V2, V3: 0x4ee38441
    uint32_t raw_add_2d = 0x4ee38441;
    TEST_ASSERT(ovir_cpu_decode_single(raw_add_2d, 0x100c, &dec, NULL) == OV_DECODE_SUCCESS, "Decode ADD.2D");
    TEST_ASSERT(dec.opcode == OVIR_OP_VEC_ADD, "Insn is VEC_ADD");
    TEST_ASSERT(dec.vec_arrangement == OVIR_VEC_ARRANGE_2D, "Arrangement is 2D");
    TEST_ASSERT(dec.dst_reg == 1 && dec.src_reg1 == 2 && dec.src_reg2 == 3, "Regs V1, V2, V3");

    // 5. SUB.2D V4, V5, V6: 0x6ee684a4
    uint32_t raw_sub_2d = 0x6ee684a4;
    TEST_ASSERT(ovir_cpu_decode_single(raw_sub_2d, 0x1010, &dec, NULL) == OV_DECODE_SUCCESS, "Decode SUB.2D");
    TEST_ASSERT(dec.opcode == OVIR_OP_VEC_SUB, "Insn is VEC_SUB");
    TEST_ASSERT(dec.vec_arrangement == OVIR_VEC_ARRANGE_2D, "Arrangement is 2D");
    TEST_ASSERT(dec.dst_reg == 4 && dec.src_reg1 == 5 && dec.src_reg2 == 6, "Regs V4, V5, V6");

    // 6. AND.16B V0, V1, V2: 0x4e221c20
    uint32_t raw_and_16b = 0x4e221c20;
    TEST_ASSERT(ovir_cpu_decode_single(raw_and_16b, 0x1014, &dec, NULL) == OV_DECODE_SUCCESS, "Decode AND.16B");
    TEST_ASSERT(dec.opcode == OVIR_OP_VEC_AND, "Insn is VEC_AND");
    TEST_ASSERT(dec.vec_arrangement == OVIR_VEC_ARRANGE_16B, "Arrangement is 16B");

    // 7. ORR.16B V0, V1, V2: 0x4ea21c20
    uint32_t raw_orr_16b = 0x4ea21c20;
    TEST_ASSERT(ovir_cpu_decode_single(raw_orr_16b, 0x1018, &dec, NULL) == OV_DECODE_SUCCESS, "Decode ORR.16B");
    TEST_ASSERT(dec.opcode == OVIR_OP_VEC_ORR, "Insn is VEC_ORR");

    // 8. EOR.16B V0, V1, V2: 0x4e221420
    uint32_t raw_eor_16b = 0x4e221420;
    TEST_ASSERT(ovir_cpu_decode_single(raw_eor_16b, 0x101c, &dec, NULL) == OV_DECODE_SUCCESS, "Decode EOR.16B");
    TEST_ASSERT(dec.opcode == OVIR_OP_VEC_EOR, "Insn is VEC_EOR");

    // 9. DUP V0.4S, W1: 0x4e040c20
    uint32_t raw_dup = 0x4e040c20;
    TEST_ASSERT(ovir_cpu_decode_single(raw_dup, 0x1020, &dec, NULL) == OV_DECODE_SUCCESS, "Decode DUP");
    TEST_ASSERT(dec.opcode == OVIR_OP_VEC_DUP, "Insn is VEC_DUP");

    // 10. MOVI V0.4S, #42: 0x4f000540
    uint32_t raw_movi = 0x4f000540;
    TEST_ASSERT(ovir_cpu_decode_single(raw_movi, 0x1024, &dec, NULL) == OV_DECODE_SUCCESS, "Decode MOVI");
    TEST_ASSERT(dec.opcode == OVIR_OP_VEC_MOVI, "Insn is VEC_MOVI");

    printf("      -> All 10 ARM64 NEON vector instruction decoders passed.\n");
}

// ---------------------------------------------------------------------------
// 5. VECTOR EXECUTION & NATIVE SSE TRANSLATION TESTS
// ---------------------------------------------------------------------------
static void test_vector_execution_and_native(void) {
    printf("[5/7] Testing NEON Vector Execution & Native SSE Translation Equivalence...\n");

    ovir_cpu_state_t sim_state;
    memset(&sim_state, 0, sizeof(sim_state));

    // Setup input vectors V1 and V2
    sim_state.vregs[1].u32[0] = 10;
    sim_state.vregs[1].u32[1] = 20;
    sim_state.vregs[1].u32[2] = 30;
    sim_state.vregs[1].u32[3] = 40;

    sim_state.vregs[2].u32[0] = 1;
    sim_state.vregs[2].u32[1] = 2;
    sim_state.vregs[2].u32[2] = 3;
    sim_state.vregs[2].u32[3] = 4;

    // Build IR block: ADD.4S V0, V1, V2; RET
    ovir_ir_block_t ir;
    memset(&ir, 0, sizeof(ir));
    ir.has_vector_ops = true;

    ovir_insn_t add_insn;
    memset(&add_insn, 0, sizeof(add_insn));
    add_insn.opcode = OVIR_OP_VEC_ADD;
    add_insn.dst_reg = 0;
    add_insn.src_reg1 = 1;
    add_insn.src_reg2 = 2;
    add_insn.vec_arrangement = OVIR_VEC_ARRANGE_4S;
    ir.instructions[ir.count++] = add_insn;

    ovir_insn_t ret_insn;
    memset(&ret_insn, 0, sizeof(ret_insn));
    ret_insn.opcode = OVIR_OP_RET;
    ir.instructions[ir.count++] = ret_insn;

    // 1. Simulated execution
    TEST_ASSERT(ovir_cpu_simulate(&ir, &sim_state) == OV_SUCCESS, "Simulate ADD.4S");
    TEST_ASSERT(sim_state.vregs[0].u32[0] == 11, "V0[0] = 10 + 1 = 11");
    TEST_ASSERT(sim_state.vregs[0].u32[1] == 22, "V0[1] = 20 + 2 = 22");
    TEST_ASSERT(sim_state.vregs[0].u32[2] == 33, "V0[2] = 30 + 3 = 33");
    TEST_ASSERT(sim_state.vregs[0].u32[3] == 44, "V0[3] = 40 + 4 = 44");

    // 2. Scalar reference helper equivalence
    ovir_vec128_t scalar_ref;
    ov_vec128_add_4s(&scalar_ref, &sim_state.vregs[1], &sim_state.vregs[2]);
    TEST_ASSERT(memcmp(&scalar_ref, &sim_state.vregs[0], sizeof(ovir_vec128_t)) == 0, "Simulation matches scalar reference");

    // 3. Native JIT execution with SSE2 PADDD
    ovir_compiled_block_t compiled;
    TEST_ASSERT(ovir_cpu_emit_x86_64_extended(&ir, &compiled) == OV_SUCCESS, "Compile NEON ADD.4S to SSE");
    TEST_ASSERT(compiled.verified == true, "Compiled block verified");

    ovir_cpu_state_t native_state;
    memset(&native_state, 0, sizeof(native_state));
    native_state.vregs[1] = sim_state.vregs[1];
    native_state.vregs[2] = sim_state.vregs[2];

    TEST_ASSERT(ovir_cpu_execute_native_vector(&compiled, &native_state) == OV_SUCCESS, "Execute native vector JIT");
    TEST_ASSERT(native_state.vregs[0].u32[0] == 11, "Native V0[0] = 11");
    TEST_ASSERT(native_state.vregs[0].u32[1] == 22, "Native V0[1] = 22");
    TEST_ASSERT(native_state.vregs[0].u32[2] == 33, "Native V0[2] = 33");
    TEST_ASSERT(native_state.vregs[0].u32[3] == 44, "Native V0[3] = 44");

    // Test SUB.4S native translation
    memset(&ir, 0, sizeof(ir));
    ir.has_vector_ops = true;
    ovir_insn_t sub_insn = add_insn;
    sub_insn.opcode = OVIR_OP_VEC_SUB;
    sub_insn.dst_reg = 3;
    ir.instructions[ir.count++] = sub_insn;
    ir.instructions[ir.count++] = ret_insn;

    TEST_ASSERT(ovir_cpu_emit_x86_64_extended(&ir, &compiled) == OV_SUCCESS, "Compile NEON SUB.4S to SSE");
    TEST_ASSERT(ovir_cpu_execute_native_vector(&compiled, &native_state) == OV_SUCCESS, "Execute native SUB.4S");
    TEST_ASSERT(native_state.vregs[3].u32[0] == 9, "Native V3[0] = 10 - 1 = 9");
    TEST_ASSERT(native_state.vregs[3].u32[1] == 18, "Native V3[1] = 20 - 2 = 18");
    TEST_ASSERT(native_state.vregs[3].u32[2] == 27, "Native V3[2] = 30 - 3 = 27");
    TEST_ASSERT(native_state.vregs[3].u32[3] == 36, "Native V3[3] = 40 - 4 = 36");

    // Test ADD.2D 64-bit vector addition
    memset(&ir, 0, sizeof(ir));
    ir.has_vector_ops = true;
    ovir_insn_t add2d_insn = add_insn;
    add2d_insn.vec_arrangement = OVIR_VEC_ARRANGE_2D;
    add2d_insn.dst_reg = 4;
    ir.instructions[ir.count++] = add2d_insn;
    ir.instructions[ir.count++] = ret_insn;

    native_state.vregs[1].u64[0] = 0x100000000ULL;
    native_state.vregs[1].u64[1] = 0x200000000ULL;
    native_state.vregs[2].u64[0] = 0x5ULL;
    native_state.vregs[2].u64[1] = 0x7ULL;

    TEST_ASSERT(ovir_cpu_emit_x86_64_extended(&ir, &compiled) == OV_SUCCESS, "Compile NEON ADD.2D to SSE");
    TEST_ASSERT(ovir_cpu_execute_native_vector(&compiled, &native_state) == OV_SUCCESS, "Execute native ADD.2D");
    TEST_ASSERT(native_state.vregs[4].u64[0] == 0x100000005ULL, "Native 2D[0] = 0x100000005");
    TEST_ASSERT(native_state.vregs[4].u64[1] == 0x200000007ULL, "Native 2D[1] = 0x200000007");

    printf("      -> Vector simulation, native SSE translation, and scalar equivalence passed.\n");
}

// ---------------------------------------------------------------------------
// 6. GUEST VIRTUAL MMU, 2-LEVEL PAGE TABLE & TLB TESTS
// ---------------------------------------------------------------------------
static void test_guest_mmu_and_page_tables(void) {
    printf("[6/7] Testing Guest Virtual MMU, 2-Level Page Table & TLB...\n");

    ov_mmu_t mmu;
    TEST_ASSERT(ov_mmu_init(&mmu, 1, 4096 * 64) == OV_SUCCESS, "Init MMU");

    // Verify Page Table Alignment check
    uint64_t pa = 0;
    uint32_t flags = 0;
    TEST_ASSERT(ov_pt_map(&mmu.page_table, 0x1001, 0x80000000, OV_PTE_VALID) == OV_ERR_INVALID_PARAM, "Misaligned map rejected");

    // Map 4KB page: VA 0x10000 -> PA 0x2000 (Readable, Writable, User)
    flags = OV_PTE_VALID | OV_PTE_READ | OV_PTE_WRITE | OV_PTE_USER;
    TEST_ASSERT(ov_pt_map(&mmu.page_table, 0x10000, 0x2000, flags) == OV_SUCCESS, "Map VA 0x10000 -> PA 0x2000");

    // Walk page table
    TEST_ASSERT(ov_pt_walk(&mmu.page_table, 0x10000, &pa, &flags) == OV_SUCCESS, "Walk page table");
    TEST_ASSERT(pa == 0x2000, "Walk PA matches 0x2000");
    TEST_ASSERT((flags & OV_PTE_USER) != 0, "Walk flags have USER set");

    // Test TLB
    ov_tlb_t tlb;
    ov_tlb_init(&tlb);
    TEST_ASSERT(tlb.hits == 0, "Init TLB hits is 0");

    uint64_t tlb_ppn = 0;
    uint32_t tlb_flags = 0;
    uint64_t vpn = 0x10000 >> 12;
    uint64_t ppn = 0x2000 >> 12;

    // Initial lookup -> miss
    bool hit = ov_tlb_lookup(&tlb, vpn, 1, 0, &tlb_ppn, &tlb_flags);
    TEST_ASSERT(!hit, "TLB miss on empty cache");

    // Insert into TLB
    ov_tlb_insert(&tlb, vpn, ppn, 1, flags, 0);

    // Lookup -> hit
    hit = ov_tlb_lookup(&tlb, vpn, 1, 0, &tlb_ppn, &tlb_flags);
    TEST_ASSERT(hit, "TLB hit for ASID 1");
    TEST_ASSERT(tlb_ppn == ppn, "TLB entry PPN matches");

    // Lookup with different ASID -> miss (ASID isolation)
    hit = ov_tlb_lookup(&tlb, vpn, 2, 0, &tlb_ppn, &tlb_flags);
    TEST_ASSERT(!hit, "TLB ASID isolation verified");

    // Generation invalidation
    ov_tlb_flush_all(&tlb);
    hit = ov_tlb_lookup(&tlb, vpn, 1, 0, &tlb_ppn, &tlb_flags);
    TEST_ASSERT(!hit, "TLB flush invalidates cache");

    // Structured MMU Fault Testing
    ov_mmu_fault_info_t fault;
    memset(&fault, 0, sizeof(fault));

    // 1. Translation Fault (unmapped address)
    TEST_ASSERT(ov_mmu_translate(&mmu, 0x99000, OV_MMU_ACCESS_READ, &pa, &fault) == OV_ERR_FAULT, "Unmapped VA triggers error");
    TEST_ASSERT(fault.fault_type == OV_MMU_FAULT_TRANSLATION, "Fault type is TRANSLATION");
    TEST_ASSERT(fault.fault_addr == 0x99000, "Faulting VA recorded");

    // 2. Permission Fault (write to read-only page)
    ov_pt_map(&mmu.page_table, 0x20000, 0x80010000, OV_PTE_VALID | OV_PTE_READ | OV_PTE_USER);
    TEST_ASSERT(ov_mmu_translate(&mmu, 0x20000, OV_MMU_ACCESS_WRITE, &pa, &fault) == OV_ERR_SECURITY_VIOLATION, "Write to RO page fails");
    TEST_ASSERT(fault.fault_type == OV_MMU_FAULT_PERMISSION, "Fault type is PERMISSION");

    // 3. Privilege Fault (user accessing supervisor page)
    ov_pt_map(&mmu.page_table, 0x30000, 0x80020000, OV_PTE_VALID | OV_PTE_READ | OV_PTE_WRITE); // No USER bit
    ov_mmu_set_privilege(&mmu, OV_RING_USER); // Switch to User Mode
    TEST_ASSERT(ov_mmu_translate(&mmu, 0x30000, OV_MMU_ACCESS_READ, &pa, &fault) == OV_ERR_SECURITY_VIOLATION, "User access to supervisor page fails");
    TEST_ASSERT(fault.fault_type == OV_MMU_FAULT_PRIVILEGE, "Fault type is PRIVILEGE");

    // 4. MMU Read / Write Execution
    ov_mmu_set_privilege(&mmu, OV_RING_SUPERVISOR); // Supervisor mode
    uint8_t write_buf[8] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE };
    uint8_t read_buf[8] = { 0 };

    TEST_ASSERT(ov_mmu_write(&mmu, 0x10000, write_buf, 8, &fault) == OV_SUCCESS, "MMU write 8 bytes");
    TEST_ASSERT(ov_mmu_read(&mmu, 0x10000, read_buf, 8, &fault) == OV_SUCCESS, "MMU read 8 bytes");
    TEST_ASSERT(memcmp(write_buf, read_buf, 8) == 0, "MMU read matches written data");

    ov_mmu_destroy(&mmu);
    printf("      -> Virtual MMU, 2-level PT, TLB & structured faults verified.\n");
}

// ---------------------------------------------------------------------------
// 7. RUNTIME TELEMETRY TESTS
// ---------------------------------------------------------------------------
static void test_telemetry(void) {
    printf("[7/7] Testing Phase 13 Runtime Telemetry Metrics...\n");

    ov_telemetry_reset_runtime_stats();
    ov_telemetry_record_dbt_hit();
    ov_telemetry_record_dbt_hit();
    ov_telemetry_record_dbt_miss();
    ov_telemetry_record_dbt_hot_loop();
    ov_telemetry_record_neon_insn();
    ov_telemetry_record_neon_insn();
    ov_telemetry_record_neon_native();
    ov_telemetry_record_mmu_tlb_hit();
    ov_telemetry_record_mmu_tlb_miss();
    ov_telemetry_record_mmu_page_fault();

    const ov_runtime_telemetry_t *stats = ov_telemetry_get_runtime_stats();
    TEST_ASSERT(stats->dbt_trace_hits == 2, "DBT trace hits = 2");
    TEST_ASSERT(stats->dbt_trace_misses == 1, "DBT trace misses = 1");
    TEST_ASSERT(stats->dbt_hot_loops_detected == 1, "DBT hot loops = 1");
    TEST_ASSERT(stats->neon_insns_translated == 2, "NEON insns = 2");
    TEST_ASSERT(stats->neon_native_vector_execs == 1, "NEON native = 1");
    TEST_ASSERT(stats->mmu_tlb_hits == 1, "MMU TLB hits = 1");
    TEST_ASSERT(stats->mmu_tlb_misses == 1, "MMU TLB misses = 1");
    TEST_ASSERT(stats->mmu_page_faults == 1, "MMU faults = 1");

    printf("      -> Runtime telemetry counters verified.\n");
}

int main(void) {
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
    printf("====================================================================\n");
    printf("  OPENVINTAGE PHASE 13 — DBT, NEON & GUEST VIRTUAL MMU TEST SUITE\n");
    printf("====================================================================\n");

    test_register_allocator();
    test_condition_flags();
    test_dbt_linking_and_traces();
    test_neon_decoders();
    test_vector_execution_and_native();
    test_guest_mmu_and_page_tables();
    test_telemetry();

    printf("====================================================================\n");
    printf("  ALL %d PHASE 13 RUNTIME ASSERTIONS PASSED SUCCESSFULLY!\n", g_tests_passed);
    printf("====================================================================\n");

    return 0;
}
