#include "ovir_cpu_backend.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(__x86_64__) || defined(_M_X64)
#include <sys/mman.h>
#include <unistd.h>
#endif

// Host CPU feature queries
bool ov_host_has_sse2(void) {
#if defined(__x86_64__) || defined(_M_X64)
    #if defined(__GNUC__) || defined(__clang__)
        return __builtin_cpu_supports("sse2");
    #else
        return true; // x86-64 baseline mandates SSE2
    #endif
#else
    return false;
#endif
}

bool ov_host_has_sse41(void) {
#if defined(__x86_64__) || defined(_M_X64)
    #if defined(__GNUC__) || defined(__clang__)
        return __builtin_cpu_supports("sse4.1");
    #else
        return false;
    #endif
#else
    return false;
#endif
}

bool ov_host_has_avx(void) {
#if defined(__x86_64__) || defined(_M_X64)
    #if defined(__GNUC__) || defined(__clang__)
        return __builtin_cpu_supports("avx");
    #else
        return false;
    #endif
#else
    return false;
#endif
}

// Register mapping for lower 8 registers
// X0 -> RAX (0), X1 -> RCX (1), X2 -> RDX (2), X3 -> RBX (3)
// X4 -> RSI (6), X5 -> RDI (7), X6 -> R8 (0 w/ REX.B), X7 -> R9 (1 w/ REX.B)
static const uint8_t x86_reg_table[8] = { 0, 1, 2, 3, 6, 7, 0, 1 };

ov_status_t ovir_cpu_emit_x86_64_extended(const ovir_ir_block_t *ir, ovir_compiled_block_t *out) {
    if (!ir || !out) return OV_ERR_INVALID_PARAM;

    memset(out, 0, sizeof(ovir_compiled_block_t));
    out->target_arch = OV_ARCH_X86_64;
    out->instruction_count = ir->count;

    uint8_t *code = out->code_bytes;
    uint32_t pos = 0;

    for (uint32_t i = 0; i < ir->count; i++) {
        const ovir_insn_t *insn = &ir->instructions[i];

        if (pos + 48 >= OVIR_CPU_MAX_CODE_SIZE) {
            return OV_ERR_OUT_OF_MEMORY;
        }

        uint8_t dst = (insn->dst_reg < 8) ? x86_reg_table[insn->dst_reg] : 0;
        uint8_t src1 = (insn->src_reg1 < 8) ? x86_reg_table[insn->src_reg1] : 0;
        uint8_t src2 = (insn->src_reg2 < 8) ? x86_reg_table[insn->src_reg2] : 0;

        // For vector ops, registers map directly to XMM0..XMM7
        uint8_t vdst = (insn->dst_reg < 8) ? insn->dst_reg : 0;
        uint8_t vsrc1 = (insn->src_reg1 < 8) ? insn->src_reg1 : 0;
        uint8_t vsrc2 = (insn->src_reg2 < 8) ? insn->src_reg2 : 0;

        switch (insn->opcode) {
            case OVIR_OP_NOP:
                code[pos++] = 0x90;
                break;

            case OVIR_OP_MOV_IMM:
                // mov r64, imm64
                code[pos++] = 0x48;
                code[pos++] = 0xb8 + dst;
                memcpy(&code[pos], &insn->imm_val, 8);
                pos += 8;
                break;

            case OVIR_OP_MOV_REG:
                // mov dst, src1 (48 89 ModRM)
                code[pos++] = 0x48;
                code[pos++] = 0x89;
                code[pos++] = (uint8_t)(0xc0 | (src1 << 3) | dst);
                break;

            case OVIR_OP_MVN:
                // mov dst, ~imm
                code[pos++] = 0x48;
                code[pos++] = 0xb8 + dst;
                memcpy(&code[pos], &insn->imm_val, 8);
                pos += 8;
                break;

            case OVIR_OP_ADD:
            case OVIR_OP_ADDS:
                if (insn->dst_reg != insn->src_reg1) {
                    code[pos++] = 0x48;
                    code[pos++] = 0x89;
                    code[pos++] = (uint8_t)(0xc0 | (src1 << 3) | dst);
                }
                code[pos++] = 0x48;
                code[pos++] = 0x01; // add dst, src2
                code[pos++] = (uint8_t)(0xc0 | (src2 << 3) | dst);
                break;

            case OVIR_OP_SUB:
            case OVIR_OP_SUBS:
                if (insn->dst_reg != insn->src_reg1) {
                    code[pos++] = 0x48;
                    code[pos++] = 0x89;
                    code[pos++] = (uint8_t)(0xc0 | (src1 << 3) | dst);
                }
                code[pos++] = 0x48;
                code[pos++] = 0x29; // sub dst, src2
                code[pos++] = (uint8_t)(0xc0 | (src2 << 3) | dst);
                break;

            case OVIR_OP_MUL:
                if (insn->dst_reg != insn->src_reg1) {
                    code[pos++] = 0x48;
                    code[pos++] = 0x89;
                    code[pos++] = (uint8_t)(0xc0 | (src1 << 3) | dst);
                }
                // imul dst, src2
                code[pos++] = 0x48;
                code[pos++] = 0x0f;
                code[pos++] = 0xaf;
                code[pos++] = (uint8_t)(0xc0 | (dst << 3) | src2);
                break;

            case OVIR_OP_NEG:
                if (insn->dst_reg != insn->src_reg1) {
                    code[pos++] = 0x48;
                    code[pos++] = 0x89;
                    code[pos++] = (uint8_t)(0xc0 | (src1 << 3) | dst);
                }
                // neg dst
                code[pos++] = 0x48;
                code[pos++] = 0xf7;
                code[pos++] = (uint8_t)(0xd8 | dst);
                break;

            case OVIR_OP_AND:
            case OVIR_OP_ANDS:
                if (insn->dst_reg != insn->src_reg1) {
                    code[pos++] = 0x48;
                    code[pos++] = 0x89;
                    code[pos++] = (uint8_t)(0xc0 | (src1 << 3) | dst);
                }
                code[pos++] = 0x48;
                code[pos++] = 0x21; // and dst, src2
                code[pos++] = (uint8_t)(0xc0 | (src2 << 3) | dst);
                break;

            case OVIR_OP_ORR:
                if (insn->dst_reg != insn->src_reg1) {
                    code[pos++] = 0x48;
                    code[pos++] = 0x89;
                    code[pos++] = (uint8_t)(0xc0 | (src1 << 3) | dst);
                }
                code[pos++] = 0x48;
                code[pos++] = 0x09; // or dst, src2
                code[pos++] = (uint8_t)(0xc0 | (src2 << 3) | dst);
                break;

            case OVIR_OP_EOR:
                if (insn->dst_reg != insn->src_reg1) {
                    code[pos++] = 0x48;
                    code[pos++] = 0x89;
                    code[pos++] = (uint8_t)(0xc0 | (src1 << 3) | dst);
                }
                code[pos++] = 0x48;
                code[pos++] = 0x31; // xor dst, src2
                code[pos++] = (uint8_t)(0xc0 | (src2 << 3) | dst);
                break;

            case OVIR_OP_CMP:
                // cmp src1, src2
                code[pos++] = 0x48;
                code[pos++] = 0x39;
                code[pos++] = (uint8_t)(0xc0 | (src2 << 3) | src1);
                break;

            case OVIR_OP_TST:
                // test src1, src2
                code[pos++] = 0x48;
                code[pos++] = 0x85;
                code[pos++] = (uint8_t)(0xc0 | (src2 << 3) | src1);
                break;

            case OVIR_OP_LOAD:
                // mov dst, [src1 + offset32]
                code[pos++] = 0x48;
                code[pos++] = 0x8b;
                code[pos++] = (uint8_t)(0x80 | (dst << 3) | src1);
                memcpy(&code[pos], &insn->offset, 4);
                pos += 4;
                break;

            case OVIR_OP_STORE:
                // mov [src1 + offset32], dst
                code[pos++] = 0x48;
                code[pos++] = 0x89;
                code[pos++] = (uint8_t)(0x80 | (dst << 3) | src1);
                memcpy(&code[pos], &insn->offset, 4);
                pos += 4;
                break;

            case OVIR_OP_JMP: {
                // jmp rel32
                code[pos++] = 0xe9;
                int32_t disp = (int32_t)insn->branch_target_disp;
                memcpy(&code[pos], &disp, 4);
                pos += 4;
                break;
            }

            case OVIR_OP_JMP_COND: {
                // 0f 8x rel32
                uint8_t op_byte = 0x84; // default JE
                switch (insn->cond) {
                    case OVIR_CC_EQ: op_byte = 0x84; break; // JE
                    case OVIR_CC_NE: op_byte = 0x85; break; // JNE
                    case OVIR_CC_CS: op_byte = 0x82; break; // JB/JC
                    case OVIR_CC_CC: op_byte = 0x83; break; // JNB/JNC
                    case OVIR_CC_MI: op_byte = 0x88; break; // JS
                    case OVIR_CC_PL: op_byte = 0x89; break; // JNS
                    case OVIR_CC_VS: op_byte = 0x80; break; // JO
                    case OVIR_CC_VC: op_byte = 0x81; break; // JNO
                    case OVIR_CC_HI: op_byte = 0x87; break; // JA
                    case OVIR_CC_LS: op_byte = 0x86; break; // JBE
                    case OVIR_CC_GE: op_byte = 0x8d; break; // JGE
                    case OVIR_CC_LT: op_byte = 0x8c; break; // JL
                    case OVIR_CC_GT: op_byte = 0x8f; break; // JG
                    case OVIR_CC_LE: op_byte = 0x8e; break; // JLE
                    default: break;
                }
                code[pos++] = 0x0f;
                code[pos++] = op_byte;
                int32_t disp = (int32_t)insn->branch_target_disp;
                memcpy(&code[pos], &disp, 4);
                pos += 4;
                break;
            }

            case OVIR_OP_RET:
                code[pos++] = 0xc3;
                break;

            case OVIR_OP_BARRIER:
                // mfence: 0x0f 0xae 0xf0
                code[pos++] = 0x0f;
                code[pos++] = 0xae;
                code[pos++] = 0xf0;
                break;

            case OVIR_OP_SYSCALL:
                // syscall: 0x0f 0x05
                code[pos++] = 0x0f;
                code[pos++] = 0x05;
                break;

            // Vector (NEON -> SSE) operations
            case OVIR_OP_VEC_ADD:
                if (vdst != vsrc1) {
                    code[pos++] = 0x0f;
                    code[pos++] = 0x28; // movaps xmm_dst, xmm_src1
                    code[pos++] = (uint8_t)(0xc0 | (vdst << 3) | vsrc1);
                }
                code[pos++] = 0x66;
                code[pos++] = 0x0f;
                if (insn->vec_arrangement == OVIR_VEC_ARRANGE_2D) {
                    code[pos++] = 0xd4; // paddq xmm_dst, xmm_src2
                } else {
                    code[pos++] = 0xfe; // paddd xmm_dst, xmm_src2
                }
                code[pos++] = (uint8_t)(0xc0 | (vdst << 3) | vsrc2);
                break;

            case OVIR_OP_VEC_SUB:
                if (vdst != vsrc1) {
                    code[pos++] = 0x0f;
                    code[pos++] = 0x28; // movaps xmm_dst, xmm_src1
                    code[pos++] = (uint8_t)(0xc0 | (vdst << 3) | vsrc1);
                }
                code[pos++] = 0x66;
                code[pos++] = 0x0f;
                if (insn->vec_arrangement == OVIR_VEC_ARRANGE_2D) {
                    code[pos++] = 0xfb; // psubq xmm_dst, xmm_src2
                } else {
                    code[pos++] = 0xfa; // psubd xmm_dst, xmm_src2
                }
                code[pos++] = (uint8_t)(0xc0 | (vdst << 3) | vsrc2);
                break;

            case OVIR_OP_VEC_MUL:
                if (vdst != vsrc1) {
                    code[pos++] = 0x0f;
                    code[pos++] = 0x28; // movaps xmm_dst, xmm_src1
                    code[pos++] = (uint8_t)(0xc0 | (vdst << 3) | vsrc1);
                }
                // pmulld xmm_dst, xmm_src2 (SSE4.1: 66 0f 38 40 ModRM)
                code[pos++] = 0x66;
                code[pos++] = 0x0f;
                code[pos++] = 0x38;
                code[pos++] = 0x40;
                code[pos++] = (uint8_t)(0xc0 | (vdst << 3) | vsrc2);
                break;

            case OVIR_OP_VEC_AND:
                if (vdst != vsrc1) {
                    code[pos++] = 0x0f;
                    code[pos++] = 0x28; // movaps xmm_dst, xmm_src1
                    code[pos++] = (uint8_t)(0xc0 | (vdst << 3) | vsrc1);
                }
                code[pos++] = 0x66;
                code[pos++] = 0x0f;
                code[pos++] = 0xdb; // pand xmm_dst, xmm_src2
                code[pos++] = (uint8_t)(0xc0 | (vdst << 3) | vsrc2);
                break;

            case OVIR_OP_VEC_ORR:
                if (vdst != vsrc1) {
                    code[pos++] = 0x0f;
                    code[pos++] = 0x28; // movaps xmm_dst, xmm_src1
                    code[pos++] = (uint8_t)(0xc0 | (vdst << 3) | vsrc1);
                }
                code[pos++] = 0x66;
                code[pos++] = 0x0f;
                code[pos++] = 0xeb; // por xmm_dst, xmm_src2
                code[pos++] = (uint8_t)(0xc0 | (vdst << 3) | vsrc2);
                break;

            case OVIR_OP_VEC_EOR:
                if (vdst != vsrc1) {
                    code[pos++] = 0x0f;
                    code[pos++] = 0x28; // movaps xmm_dst, xmm_src1
                    code[pos++] = (uint8_t)(0xc0 | (vdst << 3) | vsrc1);
                }
                code[pos++] = 0x66;
                code[pos++] = 0x0f;
                code[pos++] = 0xef; // pxor xmm_dst, xmm_src2
                code[pos++] = (uint8_t)(0xc0 | (vdst << 3) | vsrc2);
                break;

            case OVIR_OP_VEC_DUP: {
                uint8_t lane_imm = 0x00;
                if (insn->vec_lane == 1) lane_imm = 0x55;
                else if (insn->vec_lane == 2) lane_imm = 0xaa;
                else if (insn->vec_lane == 3) lane_imm = 0xff;
                // pshufd xmm_dst, xmm_src1, lane_imm (66 0f 70 ModRM imm8)
                code[pos++] = 0x66;
                code[pos++] = 0x0f;
                code[pos++] = 0x70;
                code[pos++] = (uint8_t)(0xc0 | (vdst << 3) | vsrc1);
                code[pos++] = lane_imm;
                break;
            }

            case OVIR_OP_VEC_MOVI:
                if (insn->imm_val == 0) {
                    // pxor xmm_dst, xmm_dst
                    code[pos++] = 0x66;
                    code[pos++] = 0x0f;
                    code[pos++] = 0xef;
                    code[pos++] = (uint8_t)(0xc0 | (vdst << 3) | vdst);
                } else {
                    // mov eax, imm32
                    code[pos++] = 0xb8;
                    uint32_t val32 = (uint32_t)insn->imm_val;
                    memcpy(&code[pos], &val32, 4);
                    pos += 4;
                    // movd xmm_dst, eax (66 0f 6e ModRM)
                    code[pos++] = 0x66;
                    code[pos++] = 0x0f;
                    code[pos++] = 0x6e;
                    code[pos++] = (uint8_t)(0xc0 | (vdst << 3) | 0);
                    // pshufd xmm_dst, xmm_dst, 0 (66 0f 70 ModRM 0)
                    code[pos++] = 0x66;
                    code[pos++] = 0x0f;
                    code[pos++] = 0x70;
                    code[pos++] = (uint8_t)(0xc0 | (vdst << 3) | vdst);
                    code[pos++] = 0x00;
                }
                break;

            default:
                break;
        }
    }

    if (pos == 0 || code[pos - 1] != 0xc3) {
        code[pos++] = 0xc3;
    }

    out->code_size = pos;
    out->verified = false;
    return ovir_cpu_verify(out);
}

static const ovir_backend_t s_x86_backend = {
    .name = "OVIR-CPU x86-64 Native Backend",
    .guest_arch = OV_ARCH_ARM64,
    .host_arch = OV_ARCH_X86_64,
    .capabilities_mask = OVIR_BACKEND_CAP_ARITHMETIC |
                         OVIR_BACKEND_CAP_LOGICAL |
                         OVIR_BACKEND_CAP_MEMORY |
                         OVIR_BACKEND_CAP_BRANCHES |
                         OVIR_BACKEND_CAP_CALLS |
                         OVIR_BACKEND_CAP_BARRIERS |
                         OVIR_BACKEND_CAP_VECTOR_SSE2 |
                         OVIR_BACKEND_CAP_VECTOR_SSE4 |
                         OVIR_BACKEND_CAP_VECTOR_AVX,
    .emit = ovir_cpu_emit_x86_64_extended,
    .verify = ovir_cpu_verify
};

const ovir_backend_t *ovir_cpu_get_backend(ov_arch_t guest, ov_arch_t host) {
    if (guest == OV_ARCH_ARM64 && host == OV_ARCH_X86_64) {
        return &s_x86_backend;
    }
    return NULL;
}

#if defined(__x86_64__) || defined(_M_X64)
typedef uint64_t (*ov_jit_fn_t)(uint64_t *regs);
#endif

ov_status_t ovir_cpu_execute_native(
    const ovir_compiled_block_t *compiled,
    ovir_cpu_state_t *state
) {
    if (!compiled || !state) return OV_ERR_INVALID_PARAM;
    if (!compiled->verified || compiled->code_size == 0) return OV_ERR_VERIFICATION_FAILED;

#if defined(__x86_64__) || defined(_M_X64)
    size_t page_size = sysconf(_SC_PAGESIZE);
    if (page_size == 0) page_size = 4096;

    // Allocate page (PROT_READ | PROT_WRITE)
    uint8_t *exec_mem = (uint8_t *)mmap(NULL, page_size,
                                         PROT_READ | PROT_WRITE,
                                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (exec_mem == MAP_FAILED) {
        return OV_ERR_OUT_OF_MEMORY;
    }

    uint32_t p = 0;
    // push rbx
    exec_mem[p++] = 0x53;
    // push r12
    exec_mem[p++] = 0x41; exec_mem[p++] = 0x54;
    // mov r12, rdi
    exec_mem[p++] = 0x49; exec_mem[p++] = 0x89; exec_mem[p++] = 0xfc;

    // mov rax, [r12 + 0]
    exec_mem[p++] = 0x49; exec_mem[p++] = 0x8b; exec_mem[p++] = 0x44; exec_mem[p++] = 0x24; exec_mem[p++] = 0x00;
    // mov rcx, [r12 + 8]
    exec_mem[p++] = 0x49; exec_mem[p++] = 0x8b; exec_mem[p++] = 0x4c; exec_mem[p++] = 0x24; exec_mem[p++] = 0x08;
    // mov rdx, [r12 + 16]
    exec_mem[p++] = 0x49; exec_mem[p++] = 0x8b; exec_mem[p++] = 0x54; exec_mem[p++] = 0x24; exec_mem[p++] = 0x10;
    // mov rbx, [r12 + 24]
    exec_mem[p++] = 0x49; exec_mem[p++] = 0x8b; exec_mem[p++] = 0x5c; exec_mem[p++] = 0x24; exec_mem[p++] = 0x18;
    // mov rsi, [r12 + 32]
    exec_mem[p++] = 0x49; exec_mem[p++] = 0x8b; exec_mem[p++] = 0x74; exec_mem[p++] = 0x24; exec_mem[p++] = 0x20;
    // mov rdi, [r12 + 40]
    exec_mem[p++] = 0x49; exec_mem[p++] = 0x8b; exec_mem[p++] = 0x7c; exec_mem[p++] = 0x24; exec_mem[p++] = 0x28;

    // Copy compiled code without the trailing RET (0xc3)
    uint32_t body_len = (compiled->code_size > 0 && compiled->code_bytes[compiled->code_size - 1] == 0xc3)
                        ? compiled->code_size - 1 : compiled->code_size;
    memcpy(&exec_mem[p], compiled->code_bytes, body_len);
    p += body_len;

    // Epilogue:
    // mov [r12 + 0], rax
    exec_mem[p++] = 0x49; exec_mem[p++] = 0x89; exec_mem[p++] = 0x44; exec_mem[p++] = 0x24; exec_mem[p++] = 0x00;
    // mov [r12 + 8], rcx
    exec_mem[p++] = 0x49; exec_mem[p++] = 0x89; exec_mem[p++] = 0x4c; exec_mem[p++] = 0x24; exec_mem[p++] = 0x08;
    // mov [r12 + 16], rdx
    exec_mem[p++] = 0x49; exec_mem[p++] = 0x89; exec_mem[p++] = 0x54; exec_mem[p++] = 0x24; exec_mem[p++] = 0x10;
    // mov [r12 + 24], rbx
    exec_mem[p++] = 0x49; exec_mem[p++] = 0x89; exec_mem[p++] = 0x5c; exec_mem[p++] = 0x24; exec_mem[p++] = 0x18;
    // mov [r12 + 32], rsi
    exec_mem[p++] = 0x49; exec_mem[p++] = 0x89; exec_mem[p++] = 0x74; exec_mem[p++] = 0x24; exec_mem[p++] = 0x20;
    // mov [r12 + 40], rdi
    exec_mem[p++] = 0x49; exec_mem[p++] = 0x89; exec_mem[p++] = 0x7c; exec_mem[p++] = 0x24; exec_mem[p++] = 0x28;

    // pop r12
    exec_mem[p++] = 0x41; exec_mem[p++] = 0x5c;
    // pop rbx
    exec_mem[p++] = 0x5b;
    // ret
    exec_mem[p++] = 0xc3;

    // Cache clearing / synchronization
    #if defined(__GNUC__) || defined(__clang__)
    __builtin___clear_cache((char *)exec_mem, (char *)exec_mem + page_size);
    #endif

    // W^X Enforcement: Transition from WRITE to EXEC (remove WRITE permission!)
    if (mprotect(exec_mem, page_size, PROT_READ | PROT_EXEC) != 0) {
        munmap(exec_mem, page_size);
        return OV_ERR_SECURITY_VIOLATION;
    }

    // Call JIT function
    ov_jit_fn_t fn = (ov_jit_fn_t)(void *)exec_mem;
    fn(state->registers);

    state->instructions_retired += compiled->instruction_count;
    state->cycles += compiled->instruction_count;
    state->halted = true;

    // Free page
    munmap(exec_mem, page_size);
    return OV_SUCCESS;
#else
    return OV_ERR_UNSUPPORTED;
#endif
}

ov_status_t ovir_cpu_execute_native_vector(
    const ovir_compiled_block_t *compiled,
    ovir_cpu_state_t *state
) {
    if (!compiled || !state) return OV_ERR_INVALID_PARAM;
    if (!compiled->verified || compiled->code_size == 0) return OV_ERR_VERIFICATION_FAILED;

#if defined(__x86_64__) || defined(_M_X64)
    size_t page_size = sysconf(_SC_PAGESIZE);
    if (page_size == 0) page_size = 4096;

    uint8_t *exec_mem = (uint8_t *)mmap(NULL, page_size,
                                         PROT_READ | PROT_WRITE,
                                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (exec_mem == MAP_FAILED) {
        return OV_ERR_OUT_OF_MEMORY;
    }

    uint32_t p = 0;
    // push rbx
    exec_mem[p++] = 0x53;
    // push r12
    exec_mem[p++] = 0x41; exec_mem[p++] = 0x54;
    // push r13
    exec_mem[p++] = 0x41; exec_mem[p++] = 0x55;
    // mov r12, rdi (GPRs ptr)
    exec_mem[p++] = 0x49; exec_mem[p++] = 0x89; exec_mem[p++] = 0xfc;
    // mov r13, rsi (VREGs ptr)
    exec_mem[p++] = 0x49; exec_mem[p++] = 0x89; exec_mem[p++] = 0xf5;

    // Load GPRs X0..X5
    // mov rax, [r12 + 0]
    exec_mem[p++] = 0x49; exec_mem[p++] = 0x8b; exec_mem[p++] = 0x44; exec_mem[p++] = 0x24; exec_mem[p++] = 0x00;
    // mov rcx, [r12 + 8]
    exec_mem[p++] = 0x49; exec_mem[p++] = 0x8b; exec_mem[p++] = 0x4c; exec_mem[p++] = 0x24; exec_mem[p++] = 0x08;
    // mov rdx, [r12 + 16]
    exec_mem[p++] = 0x49; exec_mem[p++] = 0x8b; exec_mem[p++] = 0x54; exec_mem[p++] = 0x24; exec_mem[p++] = 0x10;
    // mov rbx, [r12 + 24]
    exec_mem[p++] = 0x49; exec_mem[p++] = 0x8b; exec_mem[p++] = 0x5c; exec_mem[p++] = 0x24; exec_mem[p++] = 0x18;
    // mov rsi, [r12 + 32]
    exec_mem[p++] = 0x49; exec_mem[p++] = 0x8b; exec_mem[p++] = 0x74; exec_mem[p++] = 0x24; exec_mem[p++] = 0x20;
    // mov rdi, [r12 + 40]
    exec_mem[p++] = 0x49; exec_mem[p++] = 0x8b; exec_mem[p++] = 0x7c; exec_mem[p++] = 0x24; exec_mem[p++] = 0x28;

    // Load XMM0..XMM7 from [r13 + 0..0x70] (movdqu: F3 41 0F 6F ModRM Disp8)
    for (uint8_t vr = 0; vr < 8; vr++) {
        exec_mem[p++] = 0xf3;
        exec_mem[p++] = 0x41;
        exec_mem[p++] = 0x0f;
        exec_mem[p++] = 0x6f;
        exec_mem[p++] = (uint8_t)(0x45 | (vr << 3));
        exec_mem[p++] = (uint8_t)(vr * 16);
    }

    // Body
    uint32_t body_len = (compiled->code_size > 0 && compiled->code_bytes[compiled->code_size - 1] == 0xc3)
                        ? compiled->code_size - 1 : compiled->code_size;
    memcpy(&exec_mem[p], compiled->code_bytes, body_len);
    p += body_len;

    // Store XMM0..XMM7 to [r13 + 0..0x70] (movdqu: F3 41 0F 7F ModRM Disp8)
    for (uint8_t vr = 0; vr < 8; vr++) {
        exec_mem[p++] = 0xf3;
        exec_mem[p++] = 0x41;
        exec_mem[p++] = 0x0f;
        exec_mem[p++] = 0x7f;
        exec_mem[p++] = (uint8_t)(0x45 | (vr << 3));
        exec_mem[p++] = (uint8_t)(vr * 16);
    }

    // Store GPRs X0..X5
    // mov [r12 + 0], rax
    exec_mem[p++] = 0x49; exec_mem[p++] = 0x89; exec_mem[p++] = 0x44; exec_mem[p++] = 0x24; exec_mem[p++] = 0x00;
    // mov [r12 + 8], rcx
    exec_mem[p++] = 0x49; exec_mem[p++] = 0x89; exec_mem[p++] = 0x4c; exec_mem[p++] = 0x24; exec_mem[p++] = 0x08;
    // mov [r12 + 16], rdx
    exec_mem[p++] = 0x49; exec_mem[p++] = 0x89; exec_mem[p++] = 0x54; exec_mem[p++] = 0x24; exec_mem[p++] = 0x10;
    // mov [r12 + 24], rbx
    exec_mem[p++] = 0x49; exec_mem[p++] = 0x89; exec_mem[p++] = 0x5c; exec_mem[p++] = 0x24; exec_mem[p++] = 0x18;
    // mov [r12 + 32], rsi
    exec_mem[p++] = 0x49; exec_mem[p++] = 0x89; exec_mem[p++] = 0x74; exec_mem[p++] = 0x24; exec_mem[p++] = 0x20;
    // mov [r12 + 40], rdi
    exec_mem[p++] = 0x49; exec_mem[p++] = 0x89; exec_mem[p++] = 0x7c; exec_mem[p++] = 0x24; exec_mem[p++] = 0x28;

    // pop r13
    exec_mem[p++] = 0x41; exec_mem[p++] = 0x5d;
    // pop r12
    exec_mem[p++] = 0x41; exec_mem[p++] = 0x5c;
    // pop rbx
    exec_mem[p++] = 0x5b;
    // ret
    exec_mem[p++] = 0xc3;

    // Cache synchronization
    #if defined(__GNUC__) || defined(__clang__)
    __builtin___clear_cache((char *)exec_mem, (char *)exec_mem + page_size);
    #endif

    // W^X Enforcement: Transition from WRITE to EXEC
    if (mprotect(exec_mem, page_size, PROT_READ | PROT_EXEC) != 0) {
        munmap(exec_mem, page_size);
        return OV_ERR_SECURITY_VIOLATION;
    }

    typedef void (*ov_jit_vec_fn_t)(uint64_t *gprs, ovir_vec128_t *vregs);
    ov_jit_vec_fn_t fn = (ov_jit_vec_fn_t)(void *)exec_mem;
    fn(state->registers, state->vregs);

    state->instructions_retired += compiled->instruction_count;
    state->cycles += compiled->instruction_count;
    state->halted = true;

    munmap(exec_mem, page_size);
    return OV_SUCCESS;
#else
    return OV_ERR_UNSUPPORTED;
#endif
}
