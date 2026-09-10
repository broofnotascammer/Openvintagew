/**
 * OpenVintage Pre-Boot Simulator - OVIR-CPU Translation Engine Implementation
 * Architecture-agnostic intermediate representation, optimizer passes, execution, and JIT cache.
 */

#include "ov_cpu_engine.h"
#include "ov_logger.h"
#include <stdio.h>
#include <string.h>

static ov_cpu_jit_cache_t jit_cache = {0};

ov_status_t ov_cpu_engine_init(void) {
    memset(&jit_cache, 0, sizeof(jit_cache));
    ov_log_info("OVIR-CPU Translation Engine initialized (JIT cache reset)");
    return OV_SUCCESS;
}

void ov_cpu_engine_cleanup(void) {
    memset(&jit_cache, 0, sizeof(jit_cache));
    ov_log_info("OVIR-CPU Translation Engine cleaned up");
}

/* Build Sample Programs */
ov_status_t ov_cpu_build_sample_program(int sample_id, ov_cpu_program_t *out_program) {
    if (!out_program) return OV_ERROR_INVALID_PARAM;
    memset(out_program, 0, sizeof(ov_cpu_program_t));

    if (sample_id == 0) {
        /* Sample 0: ARM64 Compute Kernel -> OVIR IR */
        out_program->program_id = 1;
        snprintf(out_program->program_name, sizeof(out_program->program_name), "arm64_matrix_multiply_kernel");
        out_program->source_arch = 1; /* ARM64 */
        out_program->target_arch = 2; /* x86_64 */
        out_program->block_count = 2;

        /* Block 0: Initialization & Loop Header */
        ov_cpu_basic_block_t *b0 = &out_program->blocks[0];
        b0->block_id = 0;
        b0->start_pc = 0x00401000;
        b0->successors[0] = 1;
        b0->successor_count = 1;

        int i = 0;
        /* Constant values */
        b0->instructions[i].opcode = OV_CPU_OP_MOV;
        b0->instructions[i].bit_width = 64;
        b0->instructions[i].dst = (ov_cpu_operand_t){OV_CPU_OPND_REG, 1, 0, 0, 0};
        b0->instructions[i].src1 = (ov_cpu_operand_t){OV_CPU_OPND_IMM, 0, 10, 0, 0};
        i++;

        b0->instructions[i].opcode = OV_CPU_OP_MOV;
        b0->instructions[i].bit_width = 64;
        b0->instructions[i].dst = (ov_cpu_operand_t){OV_CPU_OPND_REG, 2, 0, 0, 0};
        b0->instructions[i].src1 = (ov_cpu_operand_t){OV_CPU_OPND_IMM, 0, 20, 0, 0};
        i++;

        /* Constant fold candidate: ADD r3 = r1 + r2 (10 + 20 = 30) */
        b0->instructions[i].opcode = OV_CPU_OP_ADD;
        b0->instructions[i].bit_width = 64;
        b0->instructions[i].dst = (ov_cpu_operand_t){OV_CPU_OPND_REG, 3, 0, 0, 0};
        b0->instructions[i].src1 = (ov_cpu_operand_t){OV_CPU_OPND_REG, 1, 0, 0, 0};
        b0->instructions[i].src2 = (ov_cpu_operand_t){OV_CPU_OPND_REG, 2, 0, 0, 0};
        i++;

        /* Redundant Move: MOV r4 = r4 */
        b0->instructions[i].opcode = OV_CPU_OP_MOV;
        b0->instructions[i].bit_width = 64;
        b0->instructions[i].dst = (ov_cpu_operand_t){OV_CPU_OPND_REG, 4, 0, 0, 0};
        b0->instructions[i].src1 = (ov_cpu_operand_t){OV_CPU_OPND_REG, 4, 0, 0, 0};
        i++;

        /* Dead instruction: MUL r5 = r3 * 2 (r5 overwritten right after without read) */
        b0->instructions[i].opcode = OV_CPU_OP_MUL;
        b0->instructions[i].bit_width = 64;
        b0->instructions[i].dst = (ov_cpu_operand_t){OV_CPU_OPND_REG, 5, 0, 0, 0};
        b0->instructions[i].src1 = (ov_cpu_operand_t){OV_CPU_OPND_REG, 3, 0, 0, 0};
        b0->instructions[i].src2 = (ov_cpu_operand_t){OV_CPU_OPND_IMM, 0, 2, 0, 0};
        i++;

        /* Overwrite r5 */
        b0->instructions[i].opcode = OV_CPU_OP_MOV;
        b0->instructions[i].bit_width = 64;
        b0->instructions[i].dst = (ov_cpu_operand_t){OV_CPU_OPND_REG, 5, 0, 0, 0};
        b0->instructions[i].src1 = (ov_cpu_operand_t){OV_CPU_OPND_IMM, 0, 100, 0, 0};
        i++;

        b0->instructions[i].opcode = OV_CPU_OP_JMP;
        b0->instructions[i].dst = (ov_cpu_operand_t){OV_CPU_OPND_LABEL, 0, 0, 0, 1};
        i++;
        b0->instr_count = i;

        /* Block 1: Compute Body */
        ov_cpu_basic_block_t *b1 = &out_program->blocks[1];
        b1->block_id = 1;
        b1->start_pc = 0x00401040;
        b1->successor_count = 0;

        i = 0;
        b1->instructions[i].opcode = OV_CPU_OP_ADD;
        b1->instructions[i].bit_width = 64;
        b1->instructions[i].dst = (ov_cpu_operand_t){OV_CPU_OPND_REG, 6, 0, 0, 0};
        b1->instructions[i].src1 = (ov_cpu_operand_t){OV_CPU_OPND_REG, 3, 0, 0, 0};
        b1->instructions[i].src2 = (ov_cpu_operand_t){OV_CPU_OPND_REG, 5, 0, 0, 0};
        i++;

        b1->instructions[i].opcode = OV_CPU_OP_RET;
        i++;
        b1->instr_count = i;

        out_program->total_instructions = b0->instr_count + b1->instr_count;
    } else {
        /* Sample 1: AVX2 SIMD Filter Kernel */
        out_program->program_id = 2;
        snprintf(out_program->program_name, sizeof(out_program->program_name), "avx2_video_filter_vectorized");
        out_program->source_arch = 2; /* x86_64 */
        out_program->target_arch = 2;
        out_program->block_count = 1;

        ov_cpu_basic_block_t *b = &out_program->blocks[0];
        b->block_id = 0;
        b->start_pc = 0x00502000;
        b->successor_count = 0;

        int i = 0;
        b->instructions[i].opcode = OV_CPU_OP_MOV;
        b->instructions[i].bit_width = 64;
        b->instructions[i].dst = (ov_cpu_operand_t){OV_CPU_OPND_REG, 1, 0, 0, 0};
        b->instructions[i].src1 = (ov_cpu_operand_t){OV_CPU_OPND_IMM, 0, 255, 0, 0};
        i++;

        b->instructions[i].opcode = OV_CPU_OP_VEC_MUL;
        b->instructions[i].bit_width = 256;
        b->instructions[i].dst = (ov_cpu_operand_t){OV_CPU_OPND_REG, 2, 0, 0, 0};
        b->instructions[i].src1 = (ov_cpu_operand_t){OV_CPU_OPND_REG, 1, 0, 0, 0};
        b->instructions[i].src2 = (ov_cpu_operand_t){OV_CPU_OPND_REG, 1, 0, 0, 0};
        i++;

        b->instructions[i].opcode = OV_CPU_OP_RET;
        i++;
        b->instr_count = i;
        out_program->total_instructions = i;
    }

    return OV_SUCCESS;
}

/* Optimizer Passes */
ov_status_t ov_cpu_optimize_program(
    ov_cpu_program_t         *program,
    bool                      enable_folding,
    bool                      enable_dce,
    bool                      enable_move_elim,
    ov_cpu_optimizer_stats_t *out_stats
) {
    if (!program || !out_stats) return OV_ERROR_INVALID_PARAM;
    memset(out_stats, 0, sizeof(ov_cpu_optimizer_stats_t));

    out_stats->enable_constant_folding = enable_folding;
    out_stats->enable_dce = enable_dce;
    out_stats->enable_move_elimination = enable_move_elim;
    out_stats->original_instr_count = program->total_instructions;

    int total_instr = 0;

    for (uint32_t b = 0; b < program->block_count; b++) {
        ov_cpu_basic_block_t *block = &program->blocks[b];

        int64_t reg_imm_map[32];
        bool    reg_is_const[32];
        memset(reg_is_const, 0, sizeof(reg_is_const));

        for (uint32_t i = 0; i < block->instr_count; i++) {
            ov_cpu_instruction_t *instr = &block->instructions[i];
            if (instr->is_dead) continue;

            /* Track constants from MOV rX, imm */
            if (instr->opcode == OV_CPU_OP_MOV &&
                instr->dst.kind == OV_CPU_OPND_REG &&
                instr->src1.kind == OV_CPU_OPND_IMM &&
                instr->dst.reg_index < 32) {
                reg_imm_map[instr->dst.reg_index] = instr->src1.imm_value;
                reg_is_const[instr->dst.reg_index] = true;
            }

            /* 1. Constant Folding Pass */
            if (enable_folding) {
                if (instr->opcode == OV_CPU_OP_ADD &&
                    instr->dst.kind == OV_CPU_OPND_REG &&
                    instr->src1.kind == OV_CPU_OPND_REG &&
                    instr->src2.kind == OV_CPU_OPND_REG) {

                    uint16_t r1 = instr->src1.reg_index;
                    uint16_t r2 = instr->src2.reg_index;

                    if (r1 < 32 && r2 < 32 && reg_is_const[r1] && reg_is_const[r2]) {
                        int64_t sum = reg_imm_map[r1] + reg_imm_map[r2];
                        instr->opcode = OV_CPU_OP_MOV;
                        instr->src1.kind = OV_CPU_OPND_IMM;
                        instr->src1.imm_value = sum;
                        instr->src2.kind = OV_CPU_OPND_NONE;

                        if (instr->dst.reg_index < 32) {
                            reg_imm_map[instr->dst.reg_index] = sum;
                            reg_is_const[instr->dst.reg_index] = true;
                        }
                        out_stats->constants_folded++;
                    }
                }
            }

            /* 2. Redundant Move Elimination: MOV rX, rX */
            if (enable_move_elim) {
                if (instr->opcode == OV_CPU_OP_MOV &&
                    instr->dst.kind == OV_CPU_OPND_REG &&
                    instr->src1.kind == OV_CPU_OPND_REG &&
                    instr->dst.reg_index == instr->src1.reg_index) {
                    instr->is_dead = true;
                    out_stats->moves_eliminated++;
                    continue;
                }
            }

            /* 3. Dead Code Elimination: Look ahead for overwrites */
            if (enable_dce) {
                if (instr->dst.kind == OV_CPU_OPND_REG &&
                    instr->opcode != OV_CPU_OP_JMP &&
                    instr->opcode != OV_CPU_OP_JCC &&
                    instr->opcode != OV_CPU_OP_RET) {

                    uint16_t target_reg = instr->dst.reg_index;
                    bool read_before_overwrite = false;

                    for (uint32_t j = i + 1; j < block->instr_count; j++) {
                        ov_cpu_instruction_t *next = &block->instructions[j];
                        if (next->is_dead) continue;

                        if ((next->src1.kind == OV_CPU_OPND_REG && next->src1.reg_index == target_reg) ||
                            (next->src2.kind == OV_CPU_OPND_REG && next->src2.reg_index == target_reg)) {
                            read_before_overwrite = true;
                            break;
                        }

                        if (next->dst.kind == OV_CPU_OPND_REG && next->dst.reg_index == target_reg) {
                            break;
                        }
                    }

                    if (!read_before_overwrite && i + 1 < block->instr_count) {
                        ov_cpu_instruction_t *next = &block->instructions[i + 1];
                        if (next->dst.kind == OV_CPU_OPND_REG && next->dst.reg_index == target_reg) {
                            instr->is_dead = true;
                            out_stats->dead_instr_removed++;
                            continue;
                        }
                    }
                }
            }

            if (!instr->is_dead) total_instr++;
        }
    }

    out_stats->optimized_instr_count = total_instr;
    program->total_instructions = total_instr;
    if (out_stats->original_instr_count > 0) {
        out_stats->reduction_ratio = 1.0f - ((float)total_instr / (float)out_stats->original_instr_count);
    }

    ov_log_info("OVIR-CPU Optimization: %u -> %u instructions (folded: %u, dce: %u, move_elim: %u)",
                out_stats->original_instr_count, total_instr,
                out_stats->constants_folded, out_stats->dead_instr_removed, out_stats->moves_eliminated);

    return OV_SUCCESS;
}

static int64_t resolve_val(const ov_cpu_operand_t *opnd, const int64_t *regs, size_t num_regs) {
    if (!opnd) return 0;
    if (opnd->kind == OV_CPU_OPND_IMM) return opnd->imm_value;
    if (opnd->kind == OV_CPU_OPND_REG && opnd->reg_index < num_regs) return regs[opnd->reg_index];
    return 0;
}

ov_status_t ov_cpu_execute_program(
    const ov_cpu_program_t *program,
    int64_t                *registers,
    size_t                  num_regs,
    uint64_t               *out_cycles
) {
    if (!program || !registers || num_regs == 0) return OV_ERROR_INVALID_PARAM;

    uint64_t cycles = 0;
    uint32_t current_block = 0;

    while (current_block < program->block_count) {
        const ov_cpu_basic_block_t *bb = &program->blocks[current_block];
        bool jumped = false;

        for (uint32_t i = 0; i < bb->instr_count; i++) {
            const ov_cpu_instruction_t *ins = &bb->instructions[i];
            if (ins->is_dead) continue;

            uint16_t dst_reg = (ins->dst.kind == OV_CPU_OPND_REG) ? ins->dst.reg_index : 0;
            int64_t v1 = resolve_val(&ins->src1, registers, num_regs);
            int64_t v2 = resolve_val(&ins->src2, registers, num_regs);

            switch (ins->opcode) {
                case OV_CPU_OP_MOV:
                    if (dst_reg < num_regs) registers[dst_reg] = v1;
                    cycles += 1;
                    break;
                case OV_CPU_OP_ADD:
                    if (dst_reg < num_regs) registers[dst_reg] = v1 + v2;
                    cycles += 1;
                    break;
                case OV_CPU_OP_SUB:
                    if (dst_reg < num_regs) registers[dst_reg] = v1 - v2;
                    cycles += 1;
                    break;
                case OV_CPU_OP_MUL:
                    if (dst_reg < num_regs) registers[dst_reg] = v1 * v2;
                    cycles += 3;
                    break;
                case OV_CPU_OP_DIV:
                    if (dst_reg < num_regs) registers[dst_reg] = (v2 != 0) ? (v1 / v2) : 0;
                    cycles += 15;
                    break;
                case OV_CPU_OP_AND:
                    if (dst_reg < num_regs) registers[dst_reg] = v1 & v2;
                    cycles += 1;
                    break;
                case OV_CPU_OP_OR:
                    if (dst_reg < num_regs) registers[dst_reg] = v1 | v2;
                    cycles += 1;
                    break;
                case OV_CPU_OP_XOR:
                    if (dst_reg < num_regs) registers[dst_reg] = v1 ^ v2;
                    cycles += 1;
                    break;
                case OV_CPU_OP_SHL:
                    if (dst_reg < num_regs) registers[dst_reg] = v1 << (v2 & 63);
                    cycles += 1;
                    break;
                case OV_CPU_OP_SHR:
                    if (dst_reg < num_regs) registers[dst_reg] = v1 >> (v2 & 63);
                    cycles += 1;
                    break;
                case OV_CPU_OP_VEC_ADD:
                    if (dst_reg < num_regs) registers[dst_reg] = v1 + v2;
                    cycles += 2;
                    break;
                case OV_CPU_OP_VEC_MUL:
                    if (dst_reg < num_regs) registers[dst_reg] = v1 * v2;
                    cycles += 4;
                    break;
                case OV_CPU_OP_JMP:
                    if (bb->successor_count > 0) {
                        current_block = bb->successors[0];
                        jumped = true;
                    }
                    cycles += 1;
                    break;
                case OV_CPU_OP_RET:
                    cycles += 1;
                    if (out_cycles) *out_cycles = cycles;
                    return OV_SUCCESS;
                default:
                    cycles += 1;
                    break;
            }

            if (jumped) break;
        }

        if (!jumped) {
            current_block++;
        }
    }

    if (out_cycles) *out_cycles = cycles;
    return OV_SUCCESS;
}

/* JIT Cache */
ov_status_t ov_cpu_jit_lookup(uint64_t guest_pc, uint64_t code_hash, bool *out_hit) {
    if (!out_hit) return OV_ERROR_INVALID_PARAM;
    *out_hit = false;

    for (uint32_t i = 0; i < jit_cache.entry_count; i++) {
        if (jit_cache.entries[i].is_valid &&
            (jit_cache.entries[i].guest_pc == guest_pc || jit_cache.entries[i].code_hash == code_hash)) {
            jit_cache.entries[i].execution_count++;
            jit_cache.cache_hits++;
            *out_hit = true;
            return OV_SUCCESS;
        }
    }

    /* Cache Miss -> Insert entry */
    if (jit_cache.entry_count < OV_CPU_JIT_CACHE_MAX) {
        uint32_t idx = jit_cache.entry_count++;
        jit_cache.entries[idx].guest_pc = guest_pc;
        jit_cache.entries[idx].code_hash = code_hash;
        jit_cache.entries[idx].native_code_size = 64;
        jit_cache.entries[idx].execution_count = 1;
        jit_cache.entries[idx].is_valid = true;
    }
    jit_cache.cache_misses++;
    return OV_SUCCESS;
}

void ov_cpu_jit_cache_clear(void) {
    memset(&jit_cache, 0, sizeof(jit_cache));
    ov_log_info("OVIR-CPU JIT Cache cleared");
}

const ov_cpu_jit_cache_t* ov_cpu_get_jit_cache(void) {
    return &jit_cache;
}

const char* ov_cpu_opcode_to_string(ov_cpu_opcode_t opcode) {
    switch (opcode) {
        case OV_CPU_OP_NOP: return "NOP";
        case OV_CPU_OP_MOV: return "MOV";
        case OV_CPU_OP_LOAD: return "LOAD";
        case OV_CPU_OP_STORE: return "STORE";
        case OV_CPU_OP_ADD: return "ADD";
        case OV_CPU_OP_SUB: return "SUB";
        case OV_CPU_OP_MUL: return "MUL";
        case OV_CPU_OP_DIV: return "DIV";
        case OV_CPU_OP_AND: return "AND";
        case OV_CPU_OP_OR: return "OR";
        case OV_CPU_OP_XOR: return "XOR";
        case OV_CPU_OP_SHL: return "SHL";
        case OV_CPU_OP_SHR: return "SHR";
        case OV_CPU_OP_CMP: return "CMP";
        case OV_CPU_OP_JMP: return "JMP";
        case OV_CPU_OP_JCC: return "JCC";
        case OV_CPU_OP_CALL: return "CALL";
        case OV_CPU_OP_RET: return "RET";
        case OV_CPU_OP_VEC_ADD: return "VEC_ADD";
        case OV_CPU_OP_VEC_MUL: return "VEC_MUL";
        default: return "UNKNOWN";
    }
}

static void format_operand(const ov_cpu_operand_t *opnd, char *out, size_t sz) {
    switch (opnd->kind) {
        case OV_CPU_OPND_REG:
            snprintf(out, sz, "r%u", opnd->reg_index);
            break;
        case OV_CPU_OPND_IMM:
            snprintf(out, sz, "#%ld", (long)opnd->imm_value);
            break;
        case OV_CPU_OPND_MEM:
            snprintf(out, sz, "[r%u + %ld]", opnd->reg_index, (long)opnd->mem_disp);
            break;
        case OV_CPU_OPND_LABEL:
            snprintf(out, sz, "L%u", opnd->label_id);
            break;
        case OV_CPU_OPND_NONE:
        default:
            out[0] = '\0';
            break;
    }
}

void ov_cpu_format_program_ir(const ov_cpu_program_t *program, char *out_buf, size_t max_len) {
    if (!program || !out_buf || max_len == 0) return;
    size_t pos = 0;
    pos += snprintf(out_buf + pos, max_len - pos,
                    "; OVIR-CPU Program: %s (ID %u)\n; Arch: %s -> %s\n\n",
                    program->program_name, program->program_id,
                    program->source_arch == 1 ? "ARM64" : "x86_64",
                    program->target_arch == 2 ? "x86_64" : "Target");

    for (uint32_t b = 0; b < program->block_count; b++) {
        const ov_cpu_basic_block_t *bb = &program->blocks[b];
        pos += snprintf(out_buf + pos, max_len - pos, "BB_%u: ; 0x%08lx\n", bb->block_id, (unsigned long)bb->start_pc);

        for (uint32_t i = 0; i < bb->instr_count; i++) {
            const ov_cpu_instruction_t *ins = &bb->instructions[i];
            char d[32] = {0}, s1[32] = {0}, s2[32] = {0};
            format_operand(&ins->dst, d, sizeof(d));
            format_operand(&ins->src1, s1, sizeof(s1));
            format_operand(&ins->src2, s2, sizeof(s2));

            if (ins->is_dead) {
                pos += snprintf(out_buf + pos, max_len - pos, "  ; [DEAD] %s %s",
                                ov_cpu_opcode_to_string(ins->opcode), d);
            } else {
                pos += snprintf(out_buf + pos, max_len - pos, "  %-8s %s",
                                ov_cpu_opcode_to_string(ins->opcode), d);
            }
            if (s1[0]) pos += snprintf(out_buf + pos, max_len - pos, ", %s", s1);
            if (s2[0]) pos += snprintf(out_buf + pos, max_len - pos, ", %s", s2);
            pos += snprintf(out_buf + pos, max_len - pos, "\n");
        }
        pos += snprintf(out_buf + pos, max_len - pos, "\n");
    }
}
