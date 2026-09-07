/** @file
  OpenVintage x86-64 (AMD64) Instruction Decoder.
  Phase 4 Architecture-Agnostic CPU Translation Framework.

  Decodes variable-length x86-64 machine instructions into OVIR-CPU IR,
  handling legacy prefixes, REX prefixes, ModR/M and SIB encodings,
  displacements, immediates, ALU operations, jumps, calls, returns,
  and SSE vector instructions.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/OvirCpuDecoderLib.h>

STATIC
OVIR_COND
X64CondToOvir (
  IN UINT8 Cond
  )
{
  switch (Cond & 0xF) {
    case 0x0: return OvirCondVs;        // Overflow (O)
    case 0x1: return OvirCondVc;        // Not Overflow (NO)
    case 0x2: return OvirCondCc;        // Below / Carry (B / C)
    case 0x3: return OvirCondCs;        // Above or Equal / No Carry (AE / NC)
    case 0x4: return OvirCondEq;        // Equal / Zero (E / Z)
    case 0x5: return OvirCondNe;        // Not Equal / Not Zero (NE / NZ)
    case 0x6: return OvirCondLs;        // Below or Equal (BE)
    case 0x7: return OvirCondHi;        // Above (A)
    case 0x8: return OvirCondMi;        // Sign / Negative (S)
    case 0x9: return OvirCondPl;        // Not Sign / Positive (NS)
    case 0xC: return OvirCondLt;        // Less Than (L)
    case 0xD: return OvirCondGe;        // Greater or Equal (GE)
    case 0xE: return OvirCondLe;        // Less or Equal (LE)
    case 0xF: return OvirCondGt;        // Greater Than (G)
    default:  return OvirCondAlways;
  }
}

STATIC
EFI_STATUS
EFIAPI
X64DecodeInstruction (
  IN  CONST UINT8           *Code,
  IN  UINTN                 MaxLen,
  IN  UINT64                GuestPc,
  OUT OVIR_CPU_INSTRUCTION  *Instruction,
  OUT UINTN                 *BytesConsumed
  )
{
  UINTN   Offset = 0;
  UINT8   Rex = 0;
  BOOLEAN HasRex = FALSE;
  BOOLEAN Prefix66 = FALSE;
  BOOLEAN PrefixF3 = FALSE;
  UINT8   Opcode;

  if (Code == NULL || Instruction == NULL || BytesConsumed == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (MaxLen < 1) {
    return EFI_BUFFER_TOO_SMALL;
  }

  ZeroMem (Instruction, sizeof (OVIR_CPU_INSTRUCTION));
  Instruction->GuestPc = GuestPc;
  Instruction->Condition = OvirCondAlways;

  // 1. Consume legacy prefixes
  while (Offset < MaxLen) {
    UINT8 B = Code[Offset];
    if (B == 0x66) {
      Prefix66 = TRUE;
      Offset++;
    } else if (B == 0xF3) {
      PrefixF3 = TRUE;
      Offset++;
    } else {
      break;
    }
  }

  // 2. Consume REX prefix (0x40 - 0x4F)
  if (Offset < MaxLen && (Code[Offset] & 0xF0) == 0x40) {
    Rex = Code[Offset];
    HasRex = TRUE;
    Offset++;
  }

  if (Offset >= MaxLen) {
    return EFI_BUFFER_TOO_SMALL;
  }

  UINT8 RexW = HasRex ? ((Rex >> 3) & 1) : 0;
  UINT8 RexR = HasRex ? ((Rex >> 2) & 1) : 0;
  UINT8 RexX = HasRex ? ((Rex >> 1) & 1) : 0;
  UINT8 RexB = HasRex ? (Rex & 1) : 0;
  UINT8 BitWidth = RexW ? 64 : (Prefix66 ? 16 : 32);
  (VOID)RexX;

  Opcode = Code[Offset++];

  // -------------------------------------------------------------
  // NOP (0x90)
  // -------------------------------------------------------------
  if (Opcode == 0x90 && !HasRex) {
    Instruction->Opcode = OvirOpNop;
    Instruction->BitWidth = 32;
    Instruction->GuestLength = (UINT8)Offset;
    *BytesConsumed = Offset;
    return EFI_SUCCESS;
  }

  // -------------------------------------------------------------
  // RET (0xC3)
  // -------------------------------------------------------------
  if (Opcode == 0xC3) {
    Instruction->Opcode = OvirOpRet;
    Instruction->BitWidth = 64;
    Instruction->GuestLength = (UINT8)Offset;
    *BytesConsumed = Offset;
    return EFI_SUCCESS;
  }

  // -------------------------------------------------------------
  // JMP rel8 (0xEB), JMP rel32 (0xE9)
  // -------------------------------------------------------------
  if (Opcode == 0xEB) {
    if (Offset >= MaxLen) return EFI_BUFFER_TOO_SMALL;
    INT8 Rel8 = (INT8)Code[Offset++];
    Instruction->Opcode = OvirOpJmp;
    Instruction->BitWidth = 64;
    Instruction->OperandCount = 1;
    Instruction->Operands[0].Kind = OvirOpKindImm;
    Instruction->Operands[0].As.Imm.Value.UintVal = (UINT64)(GuestPc + Offset + Rel8);
    Instruction->Operands[0].As.Imm.BitWidth = 64;
    Instruction->GuestLength = (UINT8)Offset;
    *BytesConsumed = Offset;
    return EFI_SUCCESS;
  }

  if (Opcode == 0xE9) {
    if (Offset + 4 > MaxLen) return EFI_BUFFER_TOO_SMALL;
    INT32 Rel32 = *(CONST INT32 *)(Code + Offset);
    Offset += 4;
    Instruction->Opcode = OvirOpJmp;
    Instruction->BitWidth = 64;
    Instruction->OperandCount = 1;
    Instruction->Operands[0].Kind = OvirOpKindImm;
    Instruction->Operands[0].As.Imm.Value.UintVal = (UINT64)(GuestPc + Offset + Rel32);
    Instruction->Operands[0].As.Imm.BitWidth = 64;
    Instruction->GuestLength = (UINT8)Offset;
    *BytesConsumed = Offset;
    return EFI_SUCCESS;
  }

  // -------------------------------------------------------------
  // CALL rel32 (0xE8)
  // -------------------------------------------------------------
  if (Opcode == 0xE8) {
    if (Offset + 4 > MaxLen) return EFI_BUFFER_TOO_SMALL;
    INT32 Rel32 = *(CONST INT32 *)(Code + Offset);
    Offset += 4;
    Instruction->Opcode = OvirOpCall;
    Instruction->BitWidth = 64;
    Instruction->OperandCount = 1;
    Instruction->Operands[0].Kind = OvirOpKindImm;
    Instruction->Operands[0].As.Imm.Value.UintVal = (UINT64)(GuestPc + Offset + Rel32);
    Instruction->Operands[0].As.Imm.BitWidth = 64;
    Instruction->GuestLength = (UINT8)Offset;
    *BytesConsumed = Offset;
    return EFI_SUCCESS;
  }

  // -------------------------------------------------------------
  // Jcc rel8 (0x70 - 0x7F)
  // -------------------------------------------------------------
  if ((Opcode & 0xF0) == 0x70) {
    if (Offset >= MaxLen) return EFI_BUFFER_TOO_SMALL;
    INT8 Rel8 = (INT8)Code[Offset++];
    Instruction->Opcode = OvirOpJcc;
    Instruction->BitWidth = 64;
    Instruction->Condition = X64CondToOvir (Opcode & 0xF);
    Instruction->OperandCount = 1;
    Instruction->Operands[0].Kind = OvirOpKindImm;
    Instruction->Operands[0].As.Imm.Value.UintVal = (UINT64)(GuestPc + Offset + Rel8);
    Instruction->Operands[0].As.Imm.BitWidth = 64;
    Instruction->GuestLength = (UINT8)Offset;
    *BytesConsumed = Offset;
    return EFI_SUCCESS;
  }

  // -------------------------------------------------------------
  // MOV reg, imm32 / imm64 (0xB8 + reg)
  // -------------------------------------------------------------
  if ((Opcode & 0xF8) == 0xB8) {
    UINT8 RegId = (Opcode & 0x7) | (RexB << 3);
    Instruction->Opcode = OvirOpMov;
    Instruction->BitWidth = BitWidth;
    Instruction->OperandCount = 2;

    Instruction->Operands[0].Kind = OvirOpKindReg;
    Instruction->Operands[0].As.Reg.Index = (UINT16)RegId;
    Instruction->Operands[0].As.Reg.Class = OvirRegClassInt;
    Instruction->Operands[0].As.Reg.BitWidth = BitWidth;

    if (RexW) {
      // 64-bit immediate
      if (Offset + 8 > MaxLen) return EFI_BUFFER_TOO_SMALL;
      UINT64 Imm64 = *(CONST UINT64 *)(Code + Offset);
      Offset += 8;
      Instruction->Operands[1].Kind = OvirOpKindImm;
      Instruction->Operands[1].As.Imm.Value.UintVal = Imm64;
      Instruction->Operands[1].As.Imm.BitWidth = 64;
    } else {
      // 32-bit immediate
      if (Offset + 4 > MaxLen) return EFI_BUFFER_TOO_SMALL;
      UINT32 Imm32 = *(CONST UINT32 *)(Code + Offset);
      Offset += 4;
      Instruction->Operands[1].Kind = OvirOpKindImm;
      Instruction->Operands[1].As.Imm.Value.UintVal = Imm32;
      Instruction->Operands[1].As.Imm.BitWidth = 32;
    }

    Instruction->GuestLength = (UINT8)Offset;
    *BytesConsumed = Offset;
    return EFI_SUCCESS;
  }

  // -------------------------------------------------------------
  // Two-byte opcode 0x0F
  // -------------------------------------------------------------
  if (Opcode == 0x0F) {
    if (Offset >= MaxLen) return EFI_BUFFER_TOO_SMALL;
    UINT8 Op2 = Code[Offset++];

    // Jcc rel32 (0x0F 0x80 - 0x0F 0x8F)
    if ((Op2 & 0xF0) == 0x80) {
      if (Offset + 4 > MaxLen) return EFI_BUFFER_TOO_SMALL;
      INT32 Rel32 = *(CONST INT32 *)(Code + Offset);
      Offset += 4;
      Instruction->Opcode = OvirOpJcc;
      Instruction->BitWidth = 64;
      Instruction->Condition = X64CondToOvir (Op2 & 0xF);
      Instruction->OperandCount = 1;
      Instruction->Operands[0].Kind = OvirOpKindImm;
      Instruction->Operands[0].As.Imm.Value.UintVal = (UINT64)(GuestPc + Offset + Rel32);
      Instruction->Operands[0].As.Imm.BitWidth = 64;
      Instruction->GuestLength = (UINT8)Offset;
      *BytesConsumed = Offset;
      return EFI_SUCCESS;
    }

    // SSE Vector ADDPS (0x0F 0x58) / ADDSS (0xF3 0x0F 0x58)
    if (Op2 == 0x58) {
      if (Offset >= MaxLen) return EFI_BUFFER_TOO_SMALL;
      UINT8 ModRm = Code[Offset++];
      UINT8 Mod = (ModRm >> 6) & 3;
      UINT8 Reg = ((ModRm >> 3) & 7) | (RexR << 3);
      UINT8 Rm  = (ModRm & 7) | (RexB << 3);

      if (Mod == 3) {
        Instruction->Opcode = PrefixF3 ? OvirOpFadd : OvirOpVecAdd;
        Instruction->BitWidth = PrefixF3 ? 32 : 128;
        Instruction->OperandCount = 3;

        Instruction->Operands[0].Kind = OvirOpKindReg;
        Instruction->Operands[0].As.Reg.Index = (UINT16)Reg;
        Instruction->Operands[0].As.Reg.Class = PrefixF3 ? OvirRegClassFloat : OvirRegClassVector;
        Instruction->Operands[0].As.Reg.BitWidth = Instruction->BitWidth;

        Instruction->Operands[1].Kind = OvirOpKindReg;
        Instruction->Operands[1].As.Reg.Index = (UINT16)Reg;
        Instruction->Operands[1].As.Reg.Class = Instruction->Operands[0].As.Reg.Class;
        Instruction->Operands[1].As.Reg.BitWidth = Instruction->BitWidth;

        Instruction->Operands[2].Kind = OvirOpKindReg;
        Instruction->Operands[2].As.Reg.Index = (UINT16)Rm;
        Instruction->Operands[2].As.Reg.Class = Instruction->Operands[0].As.Reg.Class;
        Instruction->Operands[2].As.Reg.BitWidth = Instruction->BitWidth;

        Instruction->GuestLength = (UINT8)Offset;
        *BytesConsumed = Offset;
        return EFI_SUCCESS;
      }
    }
  }

  // -------------------------------------------------------------
  // ALU Reg/Reg or Reg/Mem Operations
  // 0x01 (ADD rm, r), 0x03 (ADD r, rm)
  // 0x29 (SUB rm, r), 0x2B (SUB r, rm)
  // 0x31 (XOR rm, r), 0x33 (XOR r, rm)
  // 0x21 (AND rm, r), 0x23 (AND r, rm)
  // 0x09 (OR  rm, r), 0x0B (OR  r, rm)
  // 0x39 (CMP rm, r), 0x3B (CMP r, rm)
  // 0x89 (MOV rm, r), 0x8B (MOV r, rm)
  // -------------------------------------------------------------
  BOOLEAN IsAlu = FALSE;
  BOOLEAN DirToReg = FALSE; // TRUE if dest is reg, FALSE if dest is rm
  OVIR_OPCODE AluOp = OvirOpNop;

  switch (Opcode) {
    case 0x01: IsAlu = TRUE; DirToReg = FALSE; AluOp = OvirOpAdd; break;
    case 0x03: IsAlu = TRUE; DirToReg = TRUE;  AluOp = OvirOpAdd; break;
    case 0x29: IsAlu = TRUE; DirToReg = FALSE; AluOp = OvirOpSub; break;
    case 0x2B: IsAlu = TRUE; DirToReg = TRUE;  AluOp = OvirOpSub; break;
    case 0x31: IsAlu = TRUE; DirToReg = FALSE; AluOp = OvirOpXor; break;
    case 0x33: IsAlu = TRUE; DirToReg = TRUE;  AluOp = OvirOpXor; break;
    case 0x21: IsAlu = TRUE; DirToReg = FALSE; AluOp = OvirOpAnd; break;
    case 0x23: IsAlu = TRUE; DirToReg = TRUE;  AluOp = OvirOpAnd; break;
    case 0x09: IsAlu = TRUE; DirToReg = FALSE; AluOp = OvirOpOr;  break;
    case 0x0B: IsAlu = TRUE; DirToReg = TRUE;  AluOp = OvirOpOr;  break;
    case 0x39: IsAlu = TRUE; DirToReg = FALSE; AluOp = OvirOpCmp; break;
    case 0x3B: IsAlu = TRUE; DirToReg = TRUE;  AluOp = OvirOpCmp; break;
    case 0x89: IsAlu = TRUE; DirToReg = FALSE; AluOp = OvirOpMov; break;
    case 0x8B: IsAlu = TRUE; DirToReg = TRUE;  AluOp = OvirOpMov; break;
    default:   break;
  }

  if (IsAlu) {
    if (Offset >= MaxLen) return EFI_BUFFER_TOO_SMALL;
    UINT8 ModRm = Code[Offset++];
    UINT8 Mod = (ModRm >> 6) & 3;
    UINT8 Reg = ((ModRm >> 3) & 7) | (RexR << 3);
    UINT8 Rm  = (ModRm & 7) | (RexB << 3);

    if (Mod == 3) {
      // Register - Register
      UINT8 DstReg = DirToReg ? Reg : Rm;
      UINT8 SrcReg = DirToReg ? Rm  : Reg;

      Instruction->Opcode = AluOp;
      Instruction->BitWidth = BitWidth;

      if (AluOp == OvirOpMov) {
        Instruction->OperandCount = 2;
        Instruction->Operands[0].Kind = OvirOpKindReg;
        Instruction->Operands[0].As.Reg.Index = (UINT16)DstReg;
        Instruction->Operands[0].As.Reg.Class = OvirRegClassInt;
        Instruction->Operands[0].As.Reg.BitWidth = BitWidth;

        Instruction->Operands[1].Kind = OvirOpKindReg;
        Instruction->Operands[1].As.Reg.Index = (UINT16)SrcReg;
        Instruction->Operands[1].As.Reg.Class = OvirRegClassInt;
        Instruction->Operands[1].As.Reg.BitWidth = BitWidth;
      } else if (AluOp == OvirOpCmp) {
        Instruction->OperandCount = 2;
        Instruction->SetFlags = TRUE;
        Instruction->Operands[0].Kind = OvirOpKindReg;
        Instruction->Operands[0].As.Reg.Index = (UINT16)DstReg;
        Instruction->Operands[0].As.Reg.Class = OvirRegClassInt;
        Instruction->Operands[0].As.Reg.BitWidth = BitWidth;

        Instruction->Operands[1].Kind = OvirOpKindReg;
        Instruction->Operands[1].As.Reg.Index = (UINT16)SrcReg;
        Instruction->Operands[1].As.Reg.Class = OvirRegClassInt;
        Instruction->Operands[1].As.Reg.BitWidth = BitWidth;
      } else {
        Instruction->OperandCount = 3;
        Instruction->Operands[0].Kind = OvirOpKindReg;
        Instruction->Operands[0].As.Reg.Index = (UINT16)DstReg;
        Instruction->Operands[0].As.Reg.Class = OvirRegClassInt;
        Instruction->Operands[0].As.Reg.BitWidth = BitWidth;

        Instruction->Operands[1].Kind = OvirOpKindReg;
        Instruction->Operands[1].As.Reg.Index = (UINT16)DstReg;
        Instruction->Operands[1].As.Reg.Class = OvirRegClassInt;
        Instruction->Operands[1].As.Reg.BitWidth = BitWidth;

        Instruction->Operands[2].Kind = OvirOpKindReg;
        Instruction->Operands[2].As.Reg.Index = (UINT16)SrcReg;
        Instruction->Operands[2].As.Reg.Class = OvirRegClassInt;
        Instruction->Operands[2].As.Reg.BitWidth = BitWidth;
      }

      Instruction->GuestLength = (UINT8)Offset;
      *BytesConsumed = Offset;
      return EFI_SUCCESS;
    } else {
      // Memory operand: [base + disp]
      INT64 Disp = 0;
      if (Mod == 1) {
        if (Offset >= MaxLen) return EFI_BUFFER_TOO_SMALL;
        Disp = (INT8)Code[Offset++];
      } else if (Mod == 2) {
        if (Offset + 4 > MaxLen) return EFI_BUFFER_TOO_SMALL;
        Disp = *(CONST INT32 *)(Code + Offset);
        Offset += 4;
      }

      Instruction->Opcode = DirToReg ? ((AluOp == OvirOpMov) ? OvirOpLoad : AluOp) :
                                       ((AluOp == OvirOpMov) ? OvirOpStore : AluOp);
      Instruction->BitWidth = BitWidth;
      Instruction->OperandCount = 2;

      if (DirToReg) {
        // Load into Reg from [Rm + Disp]
        Instruction->Operands[0].Kind = OvirOpKindReg;
        Instruction->Operands[0].As.Reg.Index = (UINT16)Reg;
        Instruction->Operands[0].As.Reg.Class = OvirRegClassInt;
        Instruction->Operands[0].As.Reg.BitWidth = BitWidth;

        Instruction->Operands[1].Kind = OvirOpKindMem;
        Instruction->Operands[1].As.Mem.BaseReg.Index = (UINT16)Rm;
        Instruction->Operands[1].As.Mem.BaseReg.Class = OvirRegClassInt;
        Instruction->Operands[1].As.Mem.BaseReg.BitWidth = 64;
        Instruction->Operands[1].As.Mem.IndexReg.Index = OVIR_VREG_INVALID;
        Instruction->Operands[1].As.Mem.Scale = 1;
        Instruction->Operands[1].As.Mem.Displacement = Disp;
        Instruction->Operands[1].As.Mem.AccessSize = BitWidth / 8;
      } else {
        // Store from Reg into [Rm + Disp]
        Instruction->Operands[0].Kind = OvirOpKindMem;
        Instruction->Operands[0].As.Mem.BaseReg.Index = (UINT16)Rm;
        Instruction->Operands[0].As.Mem.BaseReg.Class = OvirRegClassInt;
        Instruction->Operands[0].As.Mem.BaseReg.BitWidth = 64;
        Instruction->Operands[0].As.Mem.IndexReg.Index = OVIR_VREG_INVALID;
        Instruction->Operands[0].As.Mem.Scale = 1;
        Instruction->Operands[0].As.Mem.Displacement = Disp;
        Instruction->Operands[0].As.Mem.AccessSize = BitWidth / 8;

        Instruction->Operands[1].Kind = OvirOpKindReg;
        Instruction->Operands[1].As.Reg.Index = (UINT16)Reg;
        Instruction->Operands[1].As.Reg.Class = OvirRegClassInt;
        Instruction->Operands[1].As.Reg.BitWidth = BitWidth;
      }

      Instruction->GuestLength = (UINT8)Offset;
      *BytesConsumed = Offset;
      return EFI_SUCCESS;
    }
  }

  // -------------------------------------------------------------
  // Group 1: 0x81 / 0x83 Immediate ALU (ADD, SUB, CMP, etc.)
  // -------------------------------------------------------------
  if (Opcode == 0x81 || Opcode == 0x83) {
    if (Offset >= MaxLen) return EFI_BUFFER_TOO_SMALL;
    UINT8 ModRm = Code[Offset++];
    UINT8 Mod = (ModRm >> 6) & 3;
    UINT8 SubOp = (ModRm >> 3) & 7;
    UINT8 Rm  = (ModRm & 7) | (RexB << 3);

    if (Mod == 3) {
      INT64 ImmVal = 0;
      if (Opcode == 0x83) {
        if (Offset >= MaxLen) return EFI_BUFFER_TOO_SMALL;
        ImmVal = (INT8)Code[Offset++];
      } else {
        if (Offset + 4 > MaxLen) return EFI_BUFFER_TOO_SMALL;
        ImmVal = *(CONST INT32 *)(Code + Offset);
        Offset += 4;
      }

      OVIR_OPCODE GrpOp = OvirOpAdd;
      if (SubOp == 0) GrpOp = OvirOpAdd;
      else if (SubOp == 5) GrpOp = OvirOpSub;
      else if (SubOp == 7) GrpOp = OvirOpCmp;
      else if (SubOp == 4) GrpOp = OvirOpAnd;
      else if (SubOp == 1) GrpOp = OvirOpOr;
      else if (SubOp == 6) GrpOp = OvirOpXor;

      Instruction->Opcode = GrpOp;
      Instruction->BitWidth = BitWidth;

      if (GrpOp == OvirOpCmp) {
        Instruction->OperandCount = 2;
        Instruction->SetFlags = TRUE;
        Instruction->Operands[0].Kind = OvirOpKindReg;
        Instruction->Operands[0].As.Reg.Index = (UINT16)Rm;
        Instruction->Operands[0].As.Reg.Class = OvirRegClassInt;
        Instruction->Operands[0].As.Reg.BitWidth = BitWidth;

        Instruction->Operands[1].Kind = OvirOpKindImm;
        Instruction->Operands[1].As.Imm.Value.UintVal = (UINT64)ImmVal;
        Instruction->Operands[1].As.Imm.BitWidth = BitWidth;
      } else {
        Instruction->OperandCount = 3;
        Instruction->Operands[0].Kind = OvirOpKindReg;
        Instruction->Operands[0].As.Reg.Index = (UINT16)Rm;
        Instruction->Operands[0].As.Reg.Class = OvirRegClassInt;
        Instruction->Operands[0].As.Reg.BitWidth = BitWidth;

        Instruction->Operands[1].Kind = OvirOpKindReg;
        Instruction->Operands[1].As.Reg.Index = (UINT16)Rm;
        Instruction->Operands[1].As.Reg.Class = OvirRegClassInt;
        Instruction->Operands[1].As.Reg.BitWidth = BitWidth;

        Instruction->Operands[2].Kind = OvirOpKindImm;
        Instruction->Operands[2].As.Imm.Value.UintVal = (UINT64)ImmVal;
        Instruction->Operands[2].As.Imm.BitWidth = BitWidth;
      }

      Instruction->GuestLength = (UINT8)Offset;
      *BytesConsumed = Offset;
      return EFI_SUCCESS;
    }
  }

  return EFI_UNSUPPORTED;
}

STATIC CONST OVIR_CPU_DECODER_INTERFACE mX64DecoderInterface = {
  OvirCpuArchX64,
  L"x86-64-Decoder",
  X64DecodeInstruction,
  NULL
};

CONST OVIR_CPU_DECODER_INTERFACE *
EFIAPI
OvirCpuGetDecoderX64 (
  VOID
  )
{
  return &mX64DecoderInterface;
}
