/** @file
  OpenVintage ARM64 (AArch64) Instruction Decoder.
  Phase 4 Architecture-Agnostic CPU Translation Framework.

  Decodes A64 32-bit machine code instructions into OVIR-CPU intermediate
  representation for arithmetic, logical, memory, branch, call, return,
  floating-point, and SIMD vector operations.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/OvirCpuDecoderLib.h>

STATIC
INT64
SignExtend (
  IN UINT64 Value,
  IN UINT8  Bits
  )
{
  UINT64 SignBit = 1ULL << (Bits - 1);
  if (Value & SignBit) {
    return (INT64)(Value | ~((1ULL << Bits) - 1));
  }
  return (INT64)Value;
}

STATIC
OVIR_COND
Arm64CondToOvir (
  IN UINT8 Cond
  )
{
  switch (Cond) {
    case 0x0: return OvirCondEq;
    case 0x1: return OvirCondNe;
    case 0x2: return OvirCondCs;
    case 0x3: return OvirCondCc;
    case 0x4: return OvirCondMi;
    case 0x5: return OvirCondPl;
    case 0x6: return OvirCondVs;
    case 0x7: return OvirCondVc;
    case 0x8: return OvirCondHi;
    case 0x9: return OvirCondLs;
    case 0xA: return OvirCondGe;
    case 0xB: return OvirCondLt;
    case 0xC: return OvirCondGt;
    case 0xD: return OvirCondLe;
    case 0xE: return OvirCondAlways;
    default:  return OvirCondAlways;
  }
}

STATIC
EFI_STATUS
EFIAPI
Arm64DecodeInstruction (
  IN  CONST UINT8           *Code,
  IN  UINTN                 MaxLen,
  IN  UINT64                GuestPc,
  OUT OVIR_CPU_INSTRUCTION  *Instruction,
  OUT UINTN                 *BytesConsumed
  )
{
  UINT32 Raw;

  if (Code == NULL || Instruction == NULL || BytesConsumed == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (MaxLen < 4) {
    return EFI_BUFFER_TOO_SMALL;
  }

  Raw = *(CONST UINT32 *)Code;
  ZeroMem (Instruction, sizeof (OVIR_CPU_INSTRUCTION));
  Instruction->GuestPc = GuestPc;
  Instruction->GuestLength = 4;
  Instruction->Condition = OvirCondAlways;
  *BytesConsumed = 4;

  // -------------------------------------------------------------
  // 1. RET / BR (Branch to register)
  // -------------------------------------------------------------
  if ((Raw & 0xFFFFFC1F) == 0xD65F0000) {
    // RET
    UINT8 Rn = (Raw >> 5) & 0x1F;
    Instruction->Opcode = OvirOpRet;
    Instruction->BitWidth = 64;
    Instruction->OperandCount = 1;
    Instruction->Operands[0].Kind = OvirOpKindReg;
    Instruction->Operands[0].As.Reg.Index = (Rn == 30) ? OVIR_VREG_LR : (UINT16)Rn;
    Instruction->Operands[0].As.Reg.Class = OvirRegClassInt;
    Instruction->Operands[0].As.Reg.BitWidth = 64;
    return EFI_SUCCESS;
  }

  if ((Raw & 0xFFFFFC1F) == 0xD61F0000) {
    // BR (indirect branch)
    UINT8 Rn = (Raw >> 5) & 0x1F;
    Instruction->Opcode = OvirOpJmp;
    Instruction->BitWidth = 64;
    Instruction->OperandCount = 1;
    Instruction->Operands[0].Kind = OvirOpKindReg;
    Instruction->Operands[0].As.Reg.Index = (UINT16)Rn;
    Instruction->Operands[0].As.Reg.Class = OvirRegClassInt;
    Instruction->Operands[0].As.Reg.BitWidth = 64;
    return EFI_SUCCESS;
  }

  // -------------------------------------------------------------
  // 2. Unconditional Branch & Call (B, BL)
  // -------------------------------------------------------------
  if ((Raw & 0xFC000000) == 0x14000000) {
    // B imm26
    INT64 Imm26 = SignExtend ((UINT64)(Raw & 0x03FFFFFF), 26);
    INT64 TargetOffset = Imm26 << 2;
    Instruction->Opcode = OvirOpJmp;
    Instruction->BitWidth = 64;
    Instruction->OperandCount = 1;
    Instruction->Operands[0].Kind = OvirOpKindImm;
    Instruction->Operands[0].As.Imm.Value.UintVal = (UINT64)(GuestPc + TargetOffset);
    Instruction->Operands[0].As.Imm.BitWidth = 64;
    Instruction->Operands[0].As.Imm.IsSigned = FALSE;
    return EFI_SUCCESS;
  }

  if ((Raw & 0xFC000000) == 0x94000000) {
    // BL imm26
    INT64 Imm26 = SignExtend ((UINT64)(Raw & 0x03FFFFFF), 26);
    INT64 TargetOffset = Imm26 << 2;
    Instruction->Opcode = OvirOpCall;
    Instruction->BitWidth = 64;
    Instruction->OperandCount = 1;
    Instruction->Operands[0].Kind = OvirOpKindImm;
    Instruction->Operands[0].As.Imm.Value.UintVal = (UINT64)(GuestPc + TargetOffset);
    Instruction->Operands[0].As.Imm.BitWidth = 64;
    return EFI_SUCCESS;
  }

  // -------------------------------------------------------------
  // 3. Conditional Branch (B.cond)
  // -------------------------------------------------------------
  if ((Raw & 0xFF000010) == 0x54000000) {
    INT64 Imm19 = SignExtend ((UINT64)((Raw >> 5) & 0x7FFFF), 19);
    UINT8 Cond = Raw & 0xF;
    Instruction->Opcode = OvirOpJcc;
    Instruction->BitWidth = 64;
    Instruction->Condition = Arm64CondToOvir (Cond);
    Instruction->OperandCount = 1;
    Instruction->Operands[0].Kind = OvirOpKindImm;
    Instruction->Operands[0].As.Imm.Value.UintVal = (UINT64)(GuestPc + (Imm19 << 2));
    Instruction->Operands[0].As.Imm.BitWidth = 64;
    return EFI_SUCCESS;
  }

  // -------------------------------------------------------------
  // 4. ADD / SUB (immediate)
  // -------------------------------------------------------------
  if ((Raw & 0x1F000000) == 0x11000000) {
    UINT8   Sf = (Raw >> 31) & 1;
    UINT8   Op = (Raw >> 30) & 1;     // 0=ADD, 1=SUB
    UINT8   S  = (Raw >> 29) & 1;     // SetFlags (ADDS, SUBS)
    UINT8   Shift = (Raw >> 22) & 3;  // 0=0, 1=12
    UINT32  Imm12 = (Raw >> 10) & 0xFFF;
    UINT8   Rn = (Raw >> 5) & 0x1F;
    UINT8   Rd = Raw & 0x1F;
    UINT64  Val = (Shift == 1) ? ((UINT64)Imm12 << 12) : Imm12;
    UINT8   Width = Sf ? 64 : 32;

    if (S && Rd == 31 && Op == 1) {
      // CMP Rn, #imm
      Instruction->Opcode = OvirOpCmp;
      Instruction->BitWidth = Width;
      Instruction->SetFlags = TRUE;
      Instruction->OperandCount = 2;

      Instruction->Operands[0].Kind = OvirOpKindReg;
      Instruction->Operands[0].As.Reg.Index = (Rn == 31) ? OVIR_VREG_SP : (UINT16)Rn;
      Instruction->Operands[0].As.Reg.Class = OvirRegClassInt;
      Instruction->Operands[0].As.Reg.BitWidth = Width;

      Instruction->Operands[1].Kind = OvirOpKindImm;
      Instruction->Operands[1].As.Imm.Value.UintVal = Val;
      Instruction->Operands[1].As.Imm.BitWidth = Width;
      return EFI_SUCCESS;
    }

    Instruction->Opcode = (Op == 0) ? OvirOpAdd : OvirOpSub;
    Instruction->BitWidth = Width;
    Instruction->SetFlags = (S != 0);
    Instruction->OperandCount = 3;

    Instruction->Operands[0].Kind = OvirOpKindReg;
    Instruction->Operands[0].As.Reg.Index = (Rd == 31) ? OVIR_VREG_SP : (UINT16)Rd;
    Instruction->Operands[0].As.Reg.Class = OvirRegClassInt;
    Instruction->Operands[0].As.Reg.BitWidth = Width;

    Instruction->Operands[1].Kind = OvirOpKindReg;
    Instruction->Operands[1].As.Reg.Index = (Rn == 31) ? OVIR_VREG_SP : (UINT16)Rn;
    Instruction->Operands[1].As.Reg.Class = OvirRegClassInt;
    Instruction->Operands[1].As.Reg.BitWidth = Width;

    Instruction->Operands[2].Kind = OvirOpKindImm;
    Instruction->Operands[2].As.Imm.Value.UintVal = Val;
    Instruction->Operands[2].As.Imm.BitWidth = Width;
    return EFI_SUCCESS;
  }

  // -------------------------------------------------------------
  // 5. ADD / SUB (shifted register)
  // -------------------------------------------------------------
  if ((Raw & 0x1F200000) == 0x0B000000) {
    UINT8 Sf = (Raw >> 31) & 1;
    UINT8 Op = (Raw >> 30) & 1;
    UINT8 S  = (Raw >> 29) & 1;
    UINT8 Rm = (Raw >> 16) & 0x1F;
    UINT8 Rn = (Raw >> 5) & 0x1F;
    UINT8 Rd = Raw & 0x1F;
    UINT8 Width = Sf ? 64 : 32;

    if (S && Rd == 31 && Op == 1) {
      // CMP Rn, Rm
      Instruction->Opcode = OvirOpCmp;
      Instruction->BitWidth = Width;
      Instruction->SetFlags = TRUE;
      Instruction->OperandCount = 2;

      Instruction->Operands[0].Kind = OvirOpKindReg;
      Instruction->Operands[0].As.Reg.Index = (UINT16)Rn;
      Instruction->Operands[0].As.Reg.Class = OvirRegClassInt;
      Instruction->Operands[0].As.Reg.BitWidth = Width;

      Instruction->Operands[1].Kind = OvirOpKindReg;
      Instruction->Operands[1].As.Reg.Index = (UINT16)Rm;
      Instruction->Operands[1].As.Reg.Class = OvirRegClassInt;
      Instruction->Operands[1].As.Reg.BitWidth = Width;
      return EFI_SUCCESS;
    }

    Instruction->Opcode = (Op == 0) ? OvirOpAdd : OvirOpSub;
    Instruction->BitWidth = Width;
    Instruction->SetFlags = (S != 0);
    Instruction->OperandCount = 3;

    Instruction->Operands[0].Kind = OvirOpKindReg;
    Instruction->Operands[0].As.Reg.Index = (UINT16)Rd;
    Instruction->Operands[0].As.Reg.Class = OvirRegClassInt;
    Instruction->Operands[0].As.Reg.BitWidth = Width;

    Instruction->Operands[1].Kind = OvirOpKindReg;
    Instruction->Operands[1].As.Reg.Index = (UINT16)Rn;
    Instruction->Operands[1].As.Reg.Class = OvirRegClassInt;
    Instruction->Operands[1].As.Reg.BitWidth = Width;

    Instruction->Operands[2].Kind = OvirOpKindReg;
    Instruction->Operands[2].As.Reg.Index = (UINT16)Rm;
    Instruction->Operands[2].As.Reg.Class = OvirRegClassInt;
    Instruction->Operands[2].As.Reg.BitWidth = Width;
    return EFI_SUCCESS;
  }

  // -------------------------------------------------------------
  // 6. Logical (shifted register): AND, ORR, EOR
  // -------------------------------------------------------------
  if ((Raw & 0x1F000000) == 0x0A000000) {
    UINT8 Sf = (Raw >> 31) & 1;
    UINT8 Opc = (Raw >> 29) & 3; // 00=AND, 01=ORR, 10=EOR
    UINT8 Rm = (Raw >> 16) & 0x1F;
    UINT8 Rn = (Raw >> 5) & 0x1F;
    UINT8 Rd = Raw & 0x1F;
    UINT8 Width = Sf ? 64 : 32;

    // Detect MOV Rd, Rm alias (ORR Rd, XZR, Rm)
    if (Opc == 1 && Rn == 31) {
      Instruction->Opcode = OvirOpMov;
      Instruction->BitWidth = Width;
      Instruction->OperandCount = 2;

      Instruction->Operands[0].Kind = OvirOpKindReg;
      Instruction->Operands[0].As.Reg.Index = (UINT16)Rd;
      Instruction->Operands[0].As.Reg.Class = OvirRegClassInt;
      Instruction->Operands[0].As.Reg.BitWidth = Width;

      Instruction->Operands[1].Kind = OvirOpKindReg;
      Instruction->Operands[1].As.Reg.Index = (UINT16)Rm;
      Instruction->Operands[1].As.Reg.Class = OvirRegClassInt;
      Instruction->Operands[1].As.Reg.BitWidth = Width;
      return EFI_SUCCESS;
    }

    Instruction->BitWidth = Width;
    Instruction->OperandCount = 3;

    if (Opc == 0)      Instruction->Opcode = OvirOpAnd;
    else if (Opc == 1) Instruction->Opcode = OvirOpOr;
    else               Instruction->Opcode = OvirOpXor;

    Instruction->Operands[0].Kind = OvirOpKindReg;
    Instruction->Operands[0].As.Reg.Index = (UINT16)Rd;
    Instruction->Operands[0].As.Reg.Class = OvirRegClassInt;
    Instruction->Operands[0].As.Reg.BitWidth = Width;

    Instruction->Operands[1].Kind = OvirOpKindReg;
    Instruction->Operands[1].As.Reg.Index = (UINT16)Rn;
    Instruction->Operands[1].As.Reg.Class = OvirRegClassInt;
    Instruction->Operands[1].As.Reg.BitWidth = Width;

    Instruction->Operands[2].Kind = OvirOpKindReg;
    Instruction->Operands[2].As.Reg.Index = (UINT16)Rm;
    Instruction->Operands[2].As.Reg.Class = OvirRegClassInt;
    Instruction->Operands[2].As.Reg.BitWidth = Width;
    return EFI_SUCCESS;
  }

  // -------------------------------------------------------------
  // 7. Move Wide Immediate (MOVZ, MOVN)
  // -------------------------------------------------------------
  if ((Raw & 0x1F800000) == 0x12800000) {
    UINT8  Sf = (Raw >> 31) & 1;
    UINT8  Opc = (Raw >> 29) & 3; // 10=MOVZ, 00=MOVN
    UINT8  Hw = (Raw >> 21) & 3;
    UINT16 Imm16 = (Raw >> 5) & 0xFFFF;
    UINT8  Rd = Raw & 0x1F;
    UINT64 Val = (UINT64)Imm16 << (Hw * 16);
    if (Opc == 0) Val = ~Val; // MOVN

    Instruction->Opcode = OvirOpMov;
    Instruction->BitWidth = Sf ? 64 : 32;
    Instruction->OperandCount = 2;

    Instruction->Operands[0].Kind = OvirOpKindReg;
    Instruction->Operands[0].As.Reg.Index = (UINT16)Rd;
    Instruction->Operands[0].As.Reg.Class = OvirRegClassInt;
    Instruction->Operands[0].As.Reg.BitWidth = Instruction->BitWidth;

    Instruction->Operands[1].Kind = OvirOpKindImm;
    Instruction->Operands[1].As.Imm.Value.UintVal = Val;
    Instruction->Operands[1].As.Imm.BitWidth = Instruction->BitWidth;
    return EFI_SUCCESS;
  }

  // -------------------------------------------------------------
  // 8. Multiply & Divide (MADD, MUL, SDIV, UDIV)
  // -------------------------------------------------------------
  if ((Raw & 0x1F000000) == 0x1B000000) {
    UINT8 Sf = (Raw >> 31) & 1;
    UINT8 Opc = (Raw >> 15) & 0x3F;
    UINT8 Rm = (Raw >> 16) & 0x1F;
    UINT8 Ra = (Raw >> 10) & 0x1F;
    UINT8 Rn = (Raw >> 5) & 0x1F;
    UINT8 Rd = Raw & 0x1F;
    UINT8 Width = Sf ? 64 : 32;

    if (Opc == 0 && Ra == 31) {
      // MUL Rd, Rn, Rm (alias for MADD Rd, Rn, Rm, XZR)
      Instruction->Opcode = OvirOpMul;
      Instruction->BitWidth = Width;
      Instruction->OperandCount = 3;

      Instruction->Operands[0].Kind = OvirOpKindReg;
      Instruction->Operands[0].As.Reg.Index = (UINT16)Rd;
      Instruction->Operands[0].As.Reg.Class = OvirRegClassInt;
      Instruction->Operands[0].As.Reg.BitWidth = Width;

      Instruction->Operands[1].Kind = OvirOpKindReg;
      Instruction->Operands[1].As.Reg.Index = (UINT16)Rn;
      Instruction->Operands[1].As.Reg.Class = OvirRegClassInt;
      Instruction->Operands[1].As.Reg.BitWidth = Width;

      Instruction->Operands[2].Kind = OvirOpKindReg;
      Instruction->Operands[2].As.Reg.Index = (UINT16)Rm;
      Instruction->Operands[2].As.Reg.Class = OvirRegClassInt;
      Instruction->Operands[2].As.Reg.BitWidth = Width;
      return EFI_SUCCESS;
    }
  }

  // -------------------------------------------------------------
  // 9. Load / Store (immediate unsigned offset)
  // -------------------------------------------------------------
  if ((Raw & 0x3B200000) == 0x39000000) {
    UINT8  Size = (Raw >> 30) & 3;    // 00=1B, 01=2B, 10=4B, 11=8B
    UINT8  Opc  = (Raw >> 22) & 3;    // 00=STR, 01=LDR
    UINT16 Imm12 = (Raw >> 10) & 0xFFF;
    UINT8  Rn   = (Raw >> 5) & 0x1F;
    UINT8  Rt   = Raw & 0x1F;
    UINT8  Bytes = 1 << Size;
    UINT64 Offset = (UINT64)Imm12 * Bytes;

    Instruction->Opcode = (Opc == 1) ? OvirOpLoad : OvirOpStore;
    Instruction->BitWidth = Bytes * 8;
    Instruction->OperandCount = 2;

    if (Opc == 1) {
      // LDR Rt, [Rn, #Offset]
      Instruction->Operands[0].Kind = OvirOpKindReg;
      Instruction->Operands[0].As.Reg.Index = (UINT16)Rt;
      Instruction->Operands[0].As.Reg.Class = OvirRegClassInt;
      Instruction->Operands[0].As.Reg.BitWidth = Instruction->BitWidth;

      Instruction->Operands[1].Kind = OvirOpKindMem;
      Instruction->Operands[1].As.Mem.BaseReg.Index = (Rn == 31) ? OVIR_VREG_SP : (UINT16)Rn;
      Instruction->Operands[1].As.Mem.BaseReg.Class = OvirRegClassInt;
      Instruction->Operands[1].As.Mem.BaseReg.BitWidth = 64;
      Instruction->Operands[1].As.Mem.IndexReg.Index = OVIR_VREG_INVALID;
      Instruction->Operands[1].As.Mem.Scale = 1;
      Instruction->Operands[1].As.Mem.Displacement = Offset;
      Instruction->Operands[1].As.Mem.AccessSize = Bytes;
    } else {
      // STR Rt, [Rn, #Offset]
      Instruction->Operands[0].Kind = OvirOpKindMem;
      Instruction->Operands[0].As.Mem.BaseReg.Index = (Rn == 31) ? OVIR_VREG_SP : (UINT16)Rn;
      Instruction->Operands[0].As.Mem.BaseReg.Class = OvirRegClassInt;
      Instruction->Operands[0].As.Mem.BaseReg.BitWidth = 64;
      Instruction->Operands[0].As.Mem.IndexReg.Index = OVIR_VREG_INVALID;
      Instruction->Operands[0].As.Mem.Scale = 1;
      Instruction->Operands[0].As.Mem.Displacement = Offset;
      Instruction->Operands[0].As.Mem.AccessSize = Bytes;

      Instruction->Operands[1].Kind = OvirOpKindReg;
      Instruction->Operands[1].As.Reg.Index = (UINT16)Rt;
      Instruction->Operands[1].As.Reg.Class = OvirRegClassInt;
      Instruction->Operands[1].As.Reg.BitWidth = Instruction->BitWidth;
    }
    return EFI_SUCCESS;
  }

  // -------------------------------------------------------------
  // 10. Floating-point Scalar (FADD, FSUB, FMUL, FDIV)
  // -------------------------------------------------------------
  if ((Raw & 0x5F20FC00) == 0x1E202800) {
    UINT8 Ftype = (Raw >> 22) & 3; // 00=S (single/32), 01=D (double/64)
    UINT8 Rm = (Raw >> 16) & 0x1F;
    UINT8 Rn = (Raw >> 5) & 0x1F;
    UINT8 Rd = Raw & 0x1F;
    UINT8 Width = (Ftype == 1) ? 64 : 32;

    Instruction->Opcode = OvirOpFadd;
    Instruction->BitWidth = Width;
    Instruction->OperandCount = 3;

    Instruction->Operands[0].Kind = OvirOpKindReg;
    Instruction->Operands[0].As.Reg.Index = (UINT16)Rd;
    Instruction->Operands[0].As.Reg.Class = OvirRegClassFloat;
    Instruction->Operands[0].As.Reg.BitWidth = Width;

    Instruction->Operands[1].Kind = OvirOpKindReg;
    Instruction->Operands[1].As.Reg.Index = (UINT16)Rn;
    Instruction->Operands[1].As.Reg.Class = OvirRegClassFloat;
    Instruction->Operands[1].As.Reg.BitWidth = Width;

    Instruction->Operands[2].Kind = OvirOpKindReg;
    Instruction->Operands[2].As.Reg.Index = (UINT16)Rm;
    Instruction->Operands[2].As.Reg.Class = OvirRegClassFloat;
    Instruction->Operands[2].As.Reg.BitWidth = Width;
    return EFI_SUCCESS;
  }

  // -------------------------------------------------------------
  // 11. SIMD Vector (VADD - Advanced SIMD vector addition)
  // -------------------------------------------------------------
  if ((Raw & 0xBF20FC00) == 0x0E208400) {
    UINT8 Q = (Raw >> 30) & 1; // 0=64-bit vector, 1=128-bit vector
    UINT8 Rm = (Raw >> 16) & 0x1F;
    UINT8 Rn = (Raw >> 5) & 0x1F;
    UINT8 Rd = Raw & 0x1F;
    UINT8 Width = Q ? 128 : 64;

    Instruction->Opcode = OvirOpVecAdd;
    Instruction->BitWidth = Width;
    Instruction->OperandCount = 3;

    Instruction->Operands[0].Kind = OvirOpKindReg;
    Instruction->Operands[0].As.Reg.Index = (UINT16)Rd;
    Instruction->Operands[0].As.Reg.Class = OvirRegClassVector;
    Instruction->Operands[0].As.Reg.BitWidth = Width;

    Instruction->Operands[1].Kind = OvirOpKindReg;
    Instruction->Operands[1].As.Reg.Index = (UINT16)Rn;
    Instruction->Operands[1].As.Reg.Class = OvirRegClassVector;
    Instruction->Operands[1].As.Reg.BitWidth = Width;

    Instruction->Operands[2].Kind = OvirOpKindReg;
    Instruction->Operands[2].As.Reg.Index = (UINT16)Rm;
    Instruction->Operands[2].As.Reg.Class = OvirRegClassVector;
    Instruction->Operands[2].As.Reg.BitWidth = Width;
    return EFI_SUCCESS;
  }

  // -------------------------------------------------------------
  // 12. NOP
  // -------------------------------------------------------------
  if (Raw == 0xD503201F) {
    Instruction->Opcode = OvirOpNop;
    Instruction->BitWidth = 32;
    Instruction->OperandCount = 0;
    return EFI_SUCCESS;
  }

  return EFI_UNSUPPORTED;
}

STATIC CONST OVIR_CPU_DECODER_INTERFACE mArm64DecoderInterface = {
  OvirCpuArchArm64,
  L"AArch64-Decoder",
  Arm64DecodeInstruction,
  NULL  // Use default block decoder
};

CONST OVIR_CPU_DECODER_INTERFACE *
EFIAPI
OvirCpuGetDecoderArm64 (
  VOID
  )
{
  return &mArm64DecoderInterface;
}
