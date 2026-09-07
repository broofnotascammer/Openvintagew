/** @file
  OpenVintage x86-64 Machine Code Translation Backend.
  Phase 4 Architecture-Agnostic CPU Translation Framework.

  Emits native x86-64 (AMD64) machine code for OVIR-CPU intermediate
  representation instructions and basic blocks.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/OvirCpuBackendLib.h>

STATIC
UINT8
MapVRegToX64 (
  IN CONST OVIR_VREG *Reg
  )
{
  if (Reg->Index == OVIR_VREG_SP) return 4; // RSP
  if (Reg->Index == OVIR_VREG_FP) return 5; // RBP
  return (UINT8)(Reg->Index & 0x0F);
}

STATIC
UINT8
OvirCondToX64 (
  IN OVIR_COND Cond
  )
{
  switch (Cond) {
    case OvirCondEq:    return 0x4; // E / Z
    case OvirCondNe:    return 0x5; // NE / NZ
    case OvirCondCs:    return 0x3; // AE / NC
    case OvirCondCc:    return 0x2; // B / C
    case OvirCondMi:    return 0x8; // S
    case OvirCondPl:    return 0x9; // NS
    case OvirCondVs:    return 0x0; // O
    case OvirCondVc:    return 0x1; // NO
    case OvirCondHi:    return 0x7; // A
    case OvirCondLs:    return 0x6; // BE
    case OvirCondGe:    return 0xD; // GE
    case OvirCondLt:    return 0xC; // L
    case OvirCondGt:    return 0xF; // G
    case OvirCondLe:    return 0xE; // LE
    default:            return 0x4;
  }
}

STATIC
EFI_STATUS
EFIAPI
X64EmitInstruction (
  IN     CONST OVIR_CPU_INSTRUCTION  *Instruction,
  IN OUT UINT8                       *CodeBuffer,
  IN     UINTN                       BufferSize,
  OUT    UINTN                       *BytesEmitted
  )
{
  UINTN Offset = 0;

  if (Instruction == NULL || CodeBuffer == NULL || BytesEmitted == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // 1. NOP (0x90)
  if (Instruction->Opcode == OvirOpNop) {
    if (BufferSize < 1) return EFI_BUFFER_TOO_SMALL;
    CodeBuffer[Offset++] = 0x90;
    *BytesEmitted = Offset;
    return EFI_SUCCESS;
  }

  // 2. RET (0xC3)
  if (Instruction->Opcode == OvirOpRet) {
    if (BufferSize < 1) return EFI_BUFFER_TOO_SMALL;
    CodeBuffer[Offset++] = 0xC3;
    *BytesEmitted = Offset;
    return EFI_SUCCESS;
  }

  // 3. JMP imm (0xE9 rel32)
  if (Instruction->Opcode == OvirOpJmp) {
    if (Instruction->OperandCount >= 1 && Instruction->Operands[0].Kind == OvirOpKindImm) {
      if (BufferSize < 5) return EFI_BUFFER_TOO_SMALL;
      INT64 Rel = (INT64)Instruction->Operands[0].As.Imm.Value.UintVal - (INT64)(Instruction->GuestPc + 5);
      CodeBuffer[Offset++] = 0xE9;
      *(INT32 *)(CodeBuffer + Offset) = (INT32)Rel;
      Offset += 4;
      *BytesEmitted = Offset;
      return EFI_SUCCESS;
    }
  }

  // 4. Jcc imm (0x0F 0x8x rel32)
  if (Instruction->Opcode == OvirOpJcc) {
    if (Instruction->OperandCount >= 1 && Instruction->Operands[0].Kind == OvirOpKindImm) {
      if (BufferSize < 6) return EFI_BUFFER_TOO_SMALL;
      UINT8 Cc = OvirCondToX64 (Instruction->Condition);
      INT64 Rel = (INT64)Instruction->Operands[0].As.Imm.Value.UintVal - (INT64)(Instruction->GuestPc + 6);
      CodeBuffer[Offset++] = 0x0F;
      CodeBuffer[Offset++] = 0x80 | Cc;
      *(INT32 *)(CodeBuffer + Offset) = (INT32)Rel;
      Offset += 4;
      *BytesEmitted = Offset;
      return EFI_SUCCESS;
    }
  }

  // 5. CALL imm (0xE8 rel32)
  if (Instruction->Opcode == OvirOpCall) {
    if (Instruction->OperandCount >= 1 && Instruction->Operands[0].Kind == OvirOpKindImm) {
      if (BufferSize < 5) return EFI_BUFFER_TOO_SMALL;
      INT64 Rel = (INT64)Instruction->Operands[0].As.Imm.Value.UintVal - (INT64)(Instruction->GuestPc + 5);
      CodeBuffer[Offset++] = 0xE8;
      *(INT32 *)(CodeBuffer + Offset) = (INT32)Rel;
      Offset += 4;
      *BytesEmitted = Offset;
      return EFI_SUCCESS;
    }
  }

  // 6. MOV reg, imm / reg, reg
  if (Instruction->Opcode == OvirOpMov) {
    if (Instruction->OperandCount == 2 && Instruction->Operands[0].Kind == OvirOpKindReg) {
      UINT8 Dst = MapVRegToX64 (&Instruction->Operands[0].As.Reg);

      if (Instruction->Operands[1].Kind == OvirOpKindImm) {
        // MOV reg, imm64 (0x48 | RexB, 0xB8+reg, imm64)
        if (BufferSize < 10) return EFI_BUFFER_TOO_SMALL;
        UINT8 Rex = 0x48 | ((Dst >= 8) ? 1 : 0);
        CodeBuffer[Offset++] = Rex;
        CodeBuffer[Offset++] = 0xB8 | (Dst & 7);
        *(UINT64 *)(CodeBuffer + Offset) = Instruction->Operands[1].As.Imm.Value.UintVal;
        Offset += 8;
        *BytesEmitted = Offset;
        return EFI_SUCCESS;
      } else if (Instruction->Operands[1].Kind == OvirOpKindReg) {
        // MOV reg, reg (0x48, 0x89, 0xC0 | (src<<3) | dst)
        if (BufferSize < 3) return EFI_BUFFER_TOO_SMALL;
        UINT8 Src = MapVRegToX64 (&Instruction->Operands[1].As.Reg);
        UINT8 Rex = 0x48 | ((Src >= 8) ? 4 : 0) | ((Dst >= 8) ? 1 : 0);
        CodeBuffer[Offset++] = Rex;
        CodeBuffer[Offset++] = 0x89;
        CodeBuffer[Offset++] = 0xC0 | ((Src & 7) << 3) | (Dst & 7);
        *BytesEmitted = Offset;
        return EFI_SUCCESS;
      }
    }
  }

  // 7. ALU Operations (ADD, SUB, AND, OR, XOR, CMP)
  if (Instruction->Opcode == OvirOpAdd || Instruction->Opcode == OvirOpSub ||
      Instruction->Opcode == OvirOpAnd || Instruction->Opcode == OvirOpOr  ||
      Instruction->Opcode == OvirOpXor || Instruction->Opcode == OvirOpCmp) {

    UINT8 Dst = MapVRegToX64 (&Instruction->Operands[0].As.Reg);
    UINT8 AluOpcode = 0x01;
    UINT8 SubOp = 0;

    switch (Instruction->Opcode) {
      case OvirOpAdd: AluOpcode = 0x01; SubOp = 0; break;
      case OvirOpSub: AluOpcode = 0x29; SubOp = 5; break;
      case OvirOpAnd: AluOpcode = 0x21; SubOp = 4; break;
      case OvirOpOr:  AluOpcode = 0x09; SubOp = 1; break;
      case OvirOpXor: AluOpcode = 0x31; SubOp = 6; break;
      case OvirOpCmp: AluOpcode = 0x39; SubOp = 7; break;
      default: break;
    }

    // If 3-operand form (Dst, Src1, Src2) and Dst != Src1, emit MOV Dst, Src1 first (except for CMP)
    if (Instruction->OperandCount == 3 && Instruction->Opcode != OvirOpCmp &&
        Instruction->Operands[1].Kind == OvirOpKindReg) {
      UINT8 Src1 = MapVRegToX64 (&Instruction->Operands[1].As.Reg);
      if (Src1 != Dst) {
        if (BufferSize - Offset < 3) return EFI_BUFFER_TOO_SMALL;
        UINT8 MovRex = 0x48 | ((Src1 >= 8) ? 4 : 0) | ((Dst >= 8) ? 1 : 0);
        CodeBuffer[Offset++] = MovRex;
        CodeBuffer[Offset++] = 0x89;
        CodeBuffer[Offset++] = 0xC0 | ((Src1 & 7) << 3) | (Dst & 7);
      }
    }

    // Reg-Reg
    CONST OVIR_CPU_OPERAND *SrcOp = &Instruction->Operands[Instruction->OperandCount - 1];
    if (SrcOp->Kind == OvirOpKindReg) {
      if (BufferSize - Offset < 3) return EFI_BUFFER_TOO_SMALL;
      UINT8 Src = MapVRegToX64 (&SrcOp->As.Reg);
      UINT8 Rex = 0x48 | ((Src >= 8) ? 4 : 0) | ((Dst >= 8) ? 1 : 0);
      CodeBuffer[Offset++] = Rex;
      CodeBuffer[Offset++] = AluOpcode;
      CodeBuffer[Offset++] = 0xC0 | ((Src & 7) << 3) | (Dst & 7);
      *BytesEmitted = Offset;
      return EFI_SUCCESS;
    }

    // Reg-Imm
    if (SrcOp->Kind == OvirOpKindImm) {
      INT64 Val = SrcOp->As.Imm.Value.IntVal;
      if (Val >= -128 && Val <= 127) {
        // 0x83 /subop imm8
        if (BufferSize < 4) return EFI_BUFFER_TOO_SMALL;
        UINT8 Rex = 0x48 | ((Dst >= 8) ? 1 : 0);
        CodeBuffer[Offset++] = Rex;
        CodeBuffer[Offset++] = 0x83;
        CodeBuffer[Offset++] = 0xC0 | (SubOp << 3) | (Dst & 7);
        CodeBuffer[Offset++] = (UINT8)(INT8)Val;
        *BytesEmitted = Offset;
        return EFI_SUCCESS;
      } else {
        // 0x81 /subop imm32
        if (BufferSize < 7) return EFI_BUFFER_TOO_SMALL;
        UINT8 Rex = 0x48 | ((Dst >= 8) ? 1 : 0);
        CodeBuffer[Offset++] = Rex;
        CodeBuffer[Offset++] = 0x81;
        CodeBuffer[Offset++] = 0xC0 | (SubOp << 3) | (Dst & 7);
        *(INT32 *)(CodeBuffer + Offset) = (INT32)Val;
        Offset += 4;
        *BytesEmitted = Offset;
        return EFI_SUCCESS;
      }
    }
  }

  // 8. Multiply (IMUL dst, src)
  if (Instruction->Opcode == OvirOpMul) {
    if (Instruction->OperandCount >= 2) {
      CONST OVIR_CPU_OPERAND *SrcOp = &Instruction->Operands[Instruction->OperandCount - 1];
      if (SrcOp->Kind == OvirOpKindReg) {
        if (BufferSize < 4) return EFI_BUFFER_TOO_SMALL;
        UINT8 Dst = MapVRegToX64 (&Instruction->Operands[0].As.Reg);
        UINT8 Src = MapVRegToX64 (&SrcOp->As.Reg);
        UINT8 Rex = 0x48 | ((Dst >= 8) ? 4 : 0) | ((Src >= 8) ? 1 : 0);
        CodeBuffer[Offset++] = Rex;
        CodeBuffer[Offset++] = 0x0F;
        CodeBuffer[Offset++] = 0xAF;
        CodeBuffer[Offset++] = 0xC0 | ((Dst & 7) << 3) | (Src & 7);
        *BytesEmitted = Offset;
        return EFI_SUCCESS;
      }
    }
  }

  // 9. Load: MOV dst, [base + disp]
  if (Instruction->Opcode == OvirOpLoad) {
    if (Instruction->OperandCount == 2 && Instruction->Operands[1].Kind == OvirOpKindMem) {
      if (BufferSize < 7) return EFI_BUFFER_TOO_SMALL;
      UINT8 Dst = MapVRegToX64 (&Instruction->Operands[0].As.Reg);
      UINT8 Base = MapVRegToX64 (&Instruction->Operands[1].As.Mem.BaseReg);
      INT64 Disp = Instruction->Operands[1].As.Mem.Displacement;

      UINT8 Rex = 0x48 | ((Dst >= 8) ? 4 : 0) | ((Base >= 8) ? 1 : 0);
      CodeBuffer[Offset++] = Rex;
      CodeBuffer[Offset++] = 0x8B;

      if (Disp >= -128 && Disp <= 127) {
        CodeBuffer[Offset++] = 0x40 | ((Dst & 7) << 3) | (Base & 7);
        CodeBuffer[Offset++] = (UINT8)(INT8)Disp;
      } else {
        CodeBuffer[Offset++] = 0x80 | ((Dst & 7) << 3) | (Base & 7);
        *(INT32 *)(CodeBuffer + Offset) = (INT32)Disp;
        Offset += 4;
      }
      *BytesEmitted = Offset;
      return EFI_SUCCESS;
    }
  }

  // 10. Store: MOV [base + disp], src
  if (Instruction->Opcode == OvirOpStore) {
    if (Instruction->OperandCount == 2 && Instruction->Operands[0].Kind == OvirOpKindMem) {
      if (BufferSize < 7) return EFI_BUFFER_TOO_SMALL;
      UINT8 Base = MapVRegToX64 (&Instruction->Operands[0].As.Mem.BaseReg);
      UINT8 Src = MapVRegToX64 (&Instruction->Operands[1].As.Reg);
      INT64 Disp = Instruction->Operands[0].As.Mem.Displacement;

      UINT8 Rex = 0x48 | ((Src >= 8) ? 4 : 0) | ((Base >= 8) ? 1 : 0);
      CodeBuffer[Offset++] = Rex;
      CodeBuffer[Offset++] = 0x89;

      if (Disp >= -128 && Disp <= 127) {
        CodeBuffer[Offset++] = 0x40 | ((Src & 7) << 3) | (Base & 7);
        CodeBuffer[Offset++] = (UINT8)(INT8)Disp;
      } else {
        CodeBuffer[Offset++] = 0x80 | ((Src & 7) << 3) | (Base & 7);
        *(INT32 *)(CodeBuffer + Offset) = (INT32)Disp;
        Offset += 4;
      }
      *BytesEmitted = Offset;
      return EFI_SUCCESS;
    }
  }

  // 11. SSE Vector ADDPS (0x0F 0x58) / Scalar ADDSS (0xF3 0x0F 0x58)
  if (Instruction->Opcode == OvirOpVecAdd || Instruction->Opcode == OvirOpFadd) {
    if (Instruction->OperandCount >= 2) {
      CONST OVIR_CPU_OPERAND *SrcOp = &Instruction->Operands[Instruction->OperandCount - 1];
      UINT8 Dst = (UINT8)(Instruction->Operands[0].As.Reg.Index & 0x0F);
      UINT8 Src = (UINT8)(SrcOp->As.Reg.Index & 0x0F);

      if (Instruction->Opcode == OvirOpFadd) {
        if (BufferSize < 4) return EFI_BUFFER_TOO_SMALL;
        CodeBuffer[Offset++] = 0xF3;
      } else {
        if (BufferSize < 3) return EFI_BUFFER_TOO_SMALL;
      }

      CodeBuffer[Offset++] = 0x0F;
      CodeBuffer[Offset++] = 0x58;
      CodeBuffer[Offset++] = 0xC0 | ((Dst & 7) << 3) | (Src & 7);
      *BytesEmitted = Offset;
      return EFI_SUCCESS;
    }
  }

  return EFI_UNSUPPORTED;
}

STATIC CONST OVIR_CPU_BACKEND_INTERFACE mX64BackendInterface = {
  OvirCpuArchX64,
  L"x86-64-NativeEmitter",
  X64EmitInstruction,
  NULL
};

CONST OVIR_CPU_BACKEND_INTERFACE *
EFIAPI
OvirCpuGetBackendX64 (
  VOID
  )
{
  return &mX64BackendInterface;
}
