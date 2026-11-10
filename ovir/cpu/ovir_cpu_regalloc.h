#ifndef OVIR_CPU_REGALLOC_H
#define OVIR_CPU_REGALLOC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../../core/types/ov_types.h"
#include "ovir_cpu.h"

// Host x86-64 GPR indices:
// 0: RAX, 1: RCX, 2: RDX, 3: RBX, 4: RSP, 5: RBP, 6: RSI, 7: RDI
// 8: R8,  9: R9, 10: R10, 11: R11, 12: R12, 13: R13, 14: R14, 15: R15

#define OV_GUEST_REG_COUNT 32
#define OV_GUEST_XZR 31

typedef enum {
    OV_REG_LOC_UNMAPPED = 0,
    OV_REG_LOC_HOST_REG,
    OV_REG_LOC_STACK_SPILL,
    OV_REG_LOC_CONTEXT_FRAME
} ov_reg_location_t;

typedef struct {
    uint8_t guest_reg;
    ov_reg_location_t location;
    uint8_t host_reg;
    int32_t spill_offset;
    bool is_dirty;
    bool is_constant_zero;
    uint8_t width; // 4 bytes (W-reg) or 8 bytes (X-reg)
} ov_reg_desc_t;

typedef struct {
    ov_reg_desc_t guest_regs[OV_GUEST_REG_COUNT];
    uint32_t allocated_host_mask;
    uint32_t callee_saved_mask;
    uint32_t caller_saved_mask;
    int32_t current_stack_spill;
    uint64_t spill_count;
    uint64_t restore_count;
} ov_reg_allocator_t;

void ov_regalloc_init(ov_reg_allocator_t *ra);
void ov_regalloc_reset(ov_reg_allocator_t *ra);

ov_status_t ov_regalloc_acquire(
    ov_reg_allocator_t *ra,
    uint8_t guest_reg,
    uint8_t width,
    ov_reg_desc_t *out_desc
);

void ov_regalloc_mark_dirty(ov_reg_allocator_t *ra, uint8_t guest_reg);

bool ov_is_callee_saved_x86(uint8_t host_reg);

#endif /* OVIR_CPU_REGALLOC_H */
