/** @file
  OpenVintage ARM64 (AArch64) Machine Code Translation Backend.
  Phase 4 Architecture-Agnostic CPU Translation Framework.

  Emits native A64 32-bit instructions for OVIR-CPU IR, demonstrating
  multi-architecture re-targeting and reverse pipeline capabilities.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/OvirCpuBackendLib.h>

STATIC
UINT8
MapVRegToArm64 (
  IN CONST OVIR_VREG *Reg
  )
{
  if (Reg->Index == OVIR_VREG_SP) return 31;
  if (Reg->Index == OVIR_VREG_LR) return 30;
  if (Reg->Index == OVIR_VREG_FP) return 29;
  return (UINT8)(Reg->Index & 0x1F);
}

STATIC
EFI_STATUS
EFIAPI
Arm64EmitInstruction (
  IN     CONST OVIR_CPU_INSTRUCTION  *Instruction,
  IN OUT UINT8                       *CodeBuffer,
  IN     UINTN                       BufferSize,
  OUT    UINTN                       *BytesEmitted
  )
{
  UINT32 Word = 0;

  if (Instruction == NULL || CodeBuffer == NULL || BytesEmitted == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize < 4) {
    return EFI_BUFFER_TOO_SMALL;
  }

  // 1. NOP
  if (Instruction->Opcode == OvirOpNop) {
    Word = 0xD503201F;
    *(UINT32 *)CodeBuffer = Word;
    *BytesEmitted = 4;
    return EFI_SUCCESS;
  }

  // 2. RET
  if (Instruction->Opcode == OvirOpRet) {
    UINT8 Rn = 30; // Default LR
    if (Instruction->OperandCount >= 1 && Instruction->Operands[0].Kind == OvirOpKindReg) {
      Rn = MapVRegToArm64 (&Instruction->Operands[0].As.Reg);
    }
    Word = 0xD65F0000 | ((UINT32)Rn << 5);
    *(UINT32 *)CodeBuffer = Word;
    *BytesEmitted = 4;
    return EFI_SUCCESS;
  }

  // 3. MOV reg, imm (MOVZ) / MOV reg, reg (ORR Rd, XZR, Rm)
  if (Instruction->Opcode == OvirOpMov && Instruction->OperandCount == 2) {
    UINT8 Rd = MapVRegToArm64 (&Instruction->Operands[0].As.Reg);
    if (Instruction->Operands[1].Kind == OvirOpKindImm) {
      UINT16 Imm16 = (UINT16)(Instruction->Operands[1].As.Imm.Value.UintVal & 0xFFFF);
      // MOVZ Xd, #imm16
      Word = 0xD2800000 | ((UINT32)Imm16 << 5) | Rd;
      *(UINT32 *)CodeBuffer = Word;
      *BytesEmitted = 4;
      return EFI_SUCCESS;
    } else if (Instruction->Operands[1].Kind == OvirOpKindReg) {
      UINT8 Rm = MapVRegToArm64 (&Instruction->Operands[1].As.Reg);
      // ORR Xd, XZR, Rm
      Word = 0xAA0003E0 | ((UINT32)Rm << 16) | Rd;
      *(UINT32 *)CodeBuffer = Word;
      *BytesEmitted = 4;
      return EFI_SUCCESS;
    }
  }

  // 4. ADD / SUB (immediate or shifted register)
  if (Instruction->Opcode == OvirOpAdd || Instruction->Opcode == OvirOpSub) {
    UINT8 Rd = MapVRegToArm64 (&Instruction->Operands[0].As.Reg);
    UINT8 Rn = Rd;
    if (Instruction->OperandCount >= 2 && Instruction->Operands[1].Kind == OvirOpKindReg) {
      Rn = MapVRegToArm64 (&Instruction->Operands[1].As.Reg);
    }

    CONST OVIR_CPU_OPERAND *LastOp = &Instruction->Operands[Instruction->OperandCount - 1];
    if (LastOp->Kind == OvirOpKindImm) {
      // ADD/SUB immediate: sf=1, op=(1 for SUB), 100010
      UINT32 OpBit = (Instruction->Opcode == OvirOpSub) ? 0x40000000 : 0x00000000;
      UINT32 Imm12 = (UINT32)(LastOp->As.Imm.Value.UintVal & 0xFFF);
      Word = 0x91000000 | OpBit | (Imm12 << 10) | ((UINT32)Rn << 5) | Rd;
      *(UINT32 *)CodeBuffer = Word;
      *BytesEmitted = 4;
      return EFI_SUCCESS;
    } else if (LastOp->Kind == OvirOpKindReg) {
      UINT8 Rm = MapVRegToArm64 (&LastOp->As.Reg);
      UINT32 OpBit = (Instruction->Opcode == OvirOpSub) ? 0x40000000 : 0x00000000;
      Word = 0x8B000000 | OpBit | ((UINT32)Rm << 16) | ((UINT32)Rn << 5) | Rd;
      *(UINT32 *)CodeBuffer = Word;
      *BytesEmitted = 4;
      return EFI_SUCCESS;
    }
  }

  // 5. Unconditional Branch B imm26
  if (Instruction->Opcode == OvirOpJmp) {
    if (Instruction->OperandCount >= 1 && Instruction->Operands[0].Kind == OvirOpKindImm) {
      INT64 Rel = (INT64)Instruction->Operands[0].As.Imm.Value.UintVal - (INT64)Instruction->GuestPc;
      INT32 Imm26 = (INT32)(Rel >> 2) & 0x03FFFFFF;
      Word = 0x14000000 | (UINT32)Imm26;
      *(UINT32 *)CodeBuffer = Word;
      *BytesEmitted = 4;
      return EFI_SUCCESS;
    }
  }

  // 6. Call BL imm26
  if (Instruction->Opcode == OvirOpCall) {
    if (Instruction->OperandCount >= 1 && Instruction->Operands[0].Kind == OvirOpKindImm) {
      INT64 Rel = (INT64)Instruction->Operands[0].As.Imm.Value.UintVal - (INT64)Instruction->GuestPc;
      INT32 Imm26 = (INT32)(Rel >> 2) & 0x03FFFFFF;
      Word = 0x94000000 | (UINT32)Imm26;
      *(UINT32 *)CodeBuffer = Word;
      *BytesEmitted = 4;
      return EFI_SUCCESS;
    }
  }

  return EFI_UNSUPPORTED;
}

STATIC CONST OVIR_CPU_BACKEND_INTERFACE mArm64BackendInterface = {
  OvirCpuArchArm64,
  L"ARM64-NativeEmitter",
  Arm64EmitInstruction,
  NULL
};

CONST OVIR_CPU_BACKEND_INTERFACE *
EFIAPI
OvirCpuGetBackendArm64 (
  VOID
  )
{
  return &mArm64BackendInterface;
}
