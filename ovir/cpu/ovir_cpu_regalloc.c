#include "ovir_cpu_regalloc.h"
#include <string.h>

// Standard System V AMD64 Callee-Saved Registers:
// RBX (3), RSP (4), RBP (5), R12 (12), R13 (13), R14 (14), R15 (15)
bool ov_is_callee_saved_x86(uint8_t host_reg) {
    return (host_reg == 3 || host_reg == 4 || host_reg == 5 ||
            (host_reg >= 12 && host_reg <= 15));
}

void ov_regalloc_init(ov_reg_allocator_t *ra) {
    if (!ra) return;
    memset(ra, 0, sizeof(ov_reg_allocator_t));

    // Register XZR is permanently constant zero
    ra->guest_regs[OV_GUEST_XZR].guest_reg = OV_GUEST_XZR;
    ra->guest_regs[OV_GUEST_XZR].is_constant_zero = true;
    ra->guest_regs[OV_GUEST_XZR].location = OV_REG_LOC_UNMAPPED;

    // Direct mapping for lower 8 registers X0-X7
    // Host mapping:
    // X0 -> RAX (0), X1 -> RCX (1), X2 -> RDX (2), X3 -> RBX (3)
    // X4 -> RSI (6), X5 -> RDI (7), X6 -> R8 (8), X7 -> R9 (9)
    static const uint8_t initial_host_regs[8] = { 0, 1, 2, 3, 6, 7, 8, 9 };
    for (uint8_t i = 0; i < 8; i++) {
        ra->guest_regs[i].guest_reg = i;
        ra->guest_regs[i].host_reg = initial_host_regs[i];
        ra->guest_regs[i].location = OV_REG_LOC_HOST_REG;
        ra->guest_regs[i].width = 8;
        ra->allocated_host_mask |= (1U << initial_host_regs[i]);
    }

    // Upper registers X8-X30 map to context frame offsets
    for (uint8_t i = 8; i < 31; i++) {
        ra->guest_regs[i].guest_reg = i;
        ra->guest_regs[i].location = OV_REG_LOC_CONTEXT_FRAME;
        ra->guest_regs[i].spill_offset = (int32_t)(i * 8); // byte offset in ovir_cpu_state_t
        ra->guest_regs[i].width = 8;
    }
}

void ov_regalloc_reset(ov_reg_allocator_t *ra) {
    ov_regalloc_init(ra);
}

ov_status_t ov_regalloc_acquire(
    ov_reg_allocator_t *ra,
    uint8_t guest_reg,
    uint8_t width,
    ov_reg_desc_t *out_desc
) {
    if (!ra || !out_desc) return OV_ERR_INVALID_PARAM;
    if (guest_reg >= OV_GUEST_REG_COUNT) return OV_ERR_INVALID_PARAM;

    ov_reg_desc_t *desc = &ra->guest_regs[guest_reg];
    desc->width = (width == 4) ? 4 : 8;

    if (desc->location == OV_REG_LOC_CONTEXT_FRAME) {
        ra->restore_count++;
    }

    *out_desc = *desc;
    return OV_SUCCESS;
}

void ov_regalloc_mark_dirty(ov_reg_allocator_t *ra, uint8_t guest_reg) {
    if (!ra || guest_reg >= OV_GUEST_REG_COUNT) return;
    if (guest_reg == OV_GUEST_XZR) return; // zero register cannot be modified
    ra->guest_regs[guest_reg].is_dirty = true;
    if (ra->guest_regs[guest_reg].location == OV_REG_LOC_CONTEXT_FRAME) {
        ra->spill_count++;
    }
}
