/**
 * OpenVintage - OVIR-CPU Instruction Decode Implementation (Phase 6)
 */

#include "ov_cpu_decode.h"
#include <stdio.h>
#include <string.h>

ov_status_t ov_cpu_decode_x86_instruction(const uint8_t *bytes, size_t len, char *out_disasm, size_t max_len) {
    if (!bytes || len == 0 || !out_disasm || max_len == 0) return OV_ERROR_INVALID_PARAM;
    if (bytes[0] == 0x90) {
        snprintf(out_disasm, max_len, "nop");
    } else if (bytes[0] == 0xC3) {
        snprintf(out_disasm, max_len, "ret");
    } else {
        snprintf(out_disasm, max_len, "raw_op 0x%02X (%zu bytes)", bytes[0], len);
    }
    return OV_SUCCESS;
}

ov_status_t ov_cpu_decode_arm64_instruction(uint32_t raw_inst, char *out_disasm, size_t max_len) {
    if (!out_disasm || max_len == 0) return OV_ERROR_INVALID_PARAM;
    if (raw_inst == 0xD503201F) {
        snprintf(out_disasm, max_len, "nop");
    } else if (raw_inst == 0xD65F03C0) {
        snprintf(out_disasm, max_len, "ret");
    } else {
        snprintf(out_disasm, max_len, "a64_op 0x%08X", raw_inst);
    }
    return OV_SUCCESS;
}
