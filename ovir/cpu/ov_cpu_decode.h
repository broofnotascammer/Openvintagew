/**
 * OpenVintage - OVIR-CPU Instruction Decode & Disassembly (Phase 6)
 */

#ifndef OV_CPU_DECODE_H
#define OV_CPU_DECODE_H

#include "ov_types.h"
#include <stdint.h>
#include <stddef.h>

ov_status_t ov_cpu_decode_x86_instruction(const uint8_t *bytes, size_t len, char *out_disasm, size_t max_len);
ov_status_t ov_cpu_decode_arm64_instruction(uint32_t raw_inst, char *out_disasm, size_t max_len);

#endif /* OV_CPU_DECODE_H */
