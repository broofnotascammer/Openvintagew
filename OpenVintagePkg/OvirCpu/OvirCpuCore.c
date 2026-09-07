/** @file
  OpenVintage CPU Intermediate Representation (OVIR-CPU) Core.
  Phase 4 Architecture-Agnostic CPU Translation Framework.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/OvirCpuLib.h>

VOID
EFIAPI
OvirCpuInitBlock (
  OUT OVIR_CPU_BASIC_BLOCK  *Block,
  IN  UINT32                BlockId,
  IN  UINT64                StartGuestPc
  )
{
  if (Block == NULL) {
    return;
  }

  ZeroMem (Block, sizeof (OVIR_CPU_BASIC_BLOCK));
  Block->BlockId = BlockId;
  Block->StartGuestPc = StartGuestPc;
  Block->EndGuestPc = StartGuestPc;
  Block->TerminatorType = OvirTerminatorFallthrough;
}

EFI_STATUS
EFIAPI
OvirCpuAppendInstruction (
  IN OUT OVIR_CPU_BASIC_BLOCK        *Block,
  IN     CONST OVIR_CPU_INSTRUCTION  *Instruction
  )
{
  if (Block == NULL || Instruction == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Block->InstructionCount >= OVIR_MAX_BLOCK_INSTRUCTIONS) {
    return EFI_BUFFER_TOO_SMALL;
  }

  CopyMem (
    &Block->Instructions[Block->InstructionCount],
    Instruction,
    sizeof (OVIR_CPU_INSTRUCTION)
    );

  Block->InstructionCount++;
  Block->EndGuestPc = Instruction->GuestPc + Instruction->GuestLength;

  // Track termination
  if (Instruction->Opcode == OvirOpJmp) {
    Block->TerminatorType = OvirTerminatorBranch;
    Block->IsTerminated = TRUE;
  } else if (Instruction->Opcode == OvirOpJcc) {
    Block->TerminatorType = OvirTerminatorCondBranch;
    Block->IsTerminated = TRUE;
  } else if (Instruction->Opcode == OvirOpRet) {
    Block->TerminatorType = OvirTerminatorReturn;
    Block->IsTerminated = TRUE;
  } else if (Instruction->Opcode == OvirOpTrap) {
    Block->TerminatorType = OvirTerminatorTrap;
    Block->IsTerminated = TRUE;
  }

  return EFI_SUCCESS;
}

VOID
EFIAPI
OvirCpuInitProgram (
  OUT OVIR_CPU_PROGRAM  *Program,
  IN  UINT32            ProgramId,
  IN  OVIR_CPU_ARCH     SourceArch,
  IN  OVIR_CPU_ARCH     TargetArch
  )
{
  if (Program == NULL) {
    return;
  }

  ZeroMem (Program, sizeof (OVIR_CPU_PROGRAM));
  Program->ProgramId = ProgramId;
  Program->SourceArch = SourceArch;
  Program->TargetArch = TargetArch;
}

EFI_STATUS
EFIAPI
OvirCpuAppendBlock (
  IN OUT OVIR_CPU_PROGRAM            *Program,
  IN     CONST OVIR_CPU_BASIC_BLOCK  *Block
  )
{
  if (Program == NULL || Block == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Program->BlockCount >= OVIR_MAX_PROGRAM_BLOCKS) {
    return EFI_BUFFER_TOO_SMALL;
  }

  CopyMem (
    &Program->Blocks[Program->BlockCount],
    Block,
    sizeof (OVIR_CPU_BASIC_BLOCK)
    );

  Program->BlockCount++;
  Program->TotalInstructionCount += Block->InstructionCount;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirCpuValidateInstruction (
  IN CONST OVIR_CPU_INSTRUCTION  *Instruction
  )
{
  UINTN Idx;

  if (Instruction == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Instruction->OperandCount > OVIR_MAX_INSTRUCTION_OPERANDS) {
    return EFI_INVALID_PARAMETER;
  }

  // Validate operands
  for (Idx = 0; Idx < Instruction->OperandCount; Idx++) {
    CONST OVIR_CPU_OPERAND *Op = &Instruction->Operands[Idx];
    if (Op->Kind == OvirOpKindMem) {
      if (Op->As.Mem.BaseReg.Index == OVIR_VREG_INVALID) {
        return EFI_INVALID_PARAMETER;
      }
      if (Op->As.Mem.Scale != 1 && Op->As.Mem.Scale != 2 &&
          Op->As.Mem.Scale != 4 && Op->As.Mem.Scale != 8) {
        return EFI_INVALID_PARAMETER;
      }
    }
  }

  // Opcode specific verification
  switch (Instruction->Opcode) {
    case OvirOpNop:
    case OvirOpRet:
    case OvirOpTrap:
    case OvirOpFence:
      break;

    case OvirOpMov:
    case OvirOpNeg:
    case OvirOpAbs:
    case OvirOpNot:
    case OvirOpLoad:
    case OvirOpLea:
    case OvirOpFmov:
    case OvirOpFneg:
      if (Instruction->OperandCount < 2) {
        return EFI_INVALID_PARAMETER;
      }
      if (Instruction->Operands[0].Kind != OvirOpKindReg) {
        return EFI_INVALID_PARAMETER;
      }
      break;

    case OvirOpStore:
      if (Instruction->OperandCount < 2) {
        return EFI_INVALID_PARAMETER;
      }
      if (Instruction->Operands[0].Kind != OvirOpKindMem) {
        return EFI_INVALID_PARAMETER;
      }
      break;

    case OvirOpAdd:
    case OvirOpSub:
    case OvirOpMul:
    case OvirOpDiv:
    case OvirOpAdc:
    case OvirOpSbb:
    case OvirOpAnd:
    case OvirOpOr:
    case OvirOpXor:
    case OvirOpShl:
    case OvirOpShr:
    case OvirOpSar:
    case OvirOpRor:
    case OvirOpFadd:
    case OvirOpFsub:
    case OvirOpFmul:
    case OvirOpFdiv:
    case OvirOpVecAdd:
    case OvirOpVecSub:
    case OvirOpVecMul:
    case OvirOpVecDot:
      if (Instruction->OperandCount < 2) {
        return EFI_INVALID_PARAMETER;
      }
      if (Instruction->Operands[0].Kind != OvirOpKindReg) {
        return EFI_INVALID_PARAMETER;
      }
      break;

    case OvirOpCmp:
    case OvirOpTest:
    case OvirOpFcmp:
      if (Instruction->OperandCount < 2) {
        return EFI_INVALID_PARAMETER;
      }
      break;

    case OvirOpJmp:
    case OvirOpJcc:
    case OvirOpCall:
      if (Instruction->OperandCount < 1) {
        return EFI_INVALID_PARAMETER;
      }
      break;

    default:
      break;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirCpuValidateBlock (
  IN CONST OVIR_CPU_BASIC_BLOCK  *Block
  )
{
  UINT32     Idx;
  EFI_STATUS Status;

  if (Block == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Block->InstructionCount > OVIR_MAX_BLOCK_INSTRUCTIONS) {
    return EFI_INVALID_PARAMETER;
  }

  for (Idx = 0; Idx < Block->InstructionCount; Idx++) {
    Status = OvirCpuValidateInstruction (&Block->Instructions[Idx]);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if (Block->SuccessorCount > OVIR_MAX_BLOCK_SUCCESSORS) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirCpuValidateProgram (
  IN OUT OVIR_CPU_PROGRAM  *Program
  )
{
  UINT32     Idx;
  EFI_STATUS Status;

  if (Program == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Program->SourceArch == OvirCpuArchUnknown ||
      Program->TargetArch == OvirCpuArchUnknown) {
    return EFI_INVALID_PARAMETER;
  }

  if (Program->BlockCount == 0 || Program->BlockCount > OVIR_MAX_PROGRAM_BLOCKS) {
    return EFI_INVALID_PARAMETER;
  }

  for (Idx = 0; Idx < Program->BlockCount; Idx++) {
    Status = OvirCpuValidateBlock (&Program->Blocks[Idx]);
    if (EFI_ERROR (Status)) {
      Program->IsValidated = FALSE;
      return Status;
    }
  }

  Program->IsValidated = TRUE;
  return EFI_SUCCESS;
}

CONST CHAR16 *
EFIAPI
OvirCpuOpcodeToString (
  IN OVIR_OPCODE  Opcode
  )
{
  switch (Opcode) {
    case OvirOpNop:     return L"NOP";
    case OvirOpMov:     return L"MOV";
    case OvirOpLoad:    return L"LOAD";
    case OvirOpStore:   return L"STORE";
    case OvirOpLea:     return L"LEA";
    case OvirOpAdd:     return L"ADD";
    case OvirOpSub:     return L"SUB";
    case OvirOpMul:     return L"MUL";
    case OvirOpDiv:     return L"DIV";
    case OvirOpNeg:     return L"NEG";
    case OvirOpAbs:     return L"ABS";
    case OvirOpAdc:     return L"ADC";
    case OvirOpSbb:     return L"SBB";
    case OvirOpAnd:     return L"AND";
    case OvirOpOr:      return L"OR";
    case OvirOpXor:     return L"XOR";
    case OvirOpNot:     return L"NOT";
    case OvirOpShl:     return L"SHL";
    case OvirOpShr:     return L"SHR";
    case OvirOpSar:     return L"SAR";
    case OvirOpRor:     return L"ROR";
    case OvirOpCmp:     return L"CMP";
    case OvirOpTest:    return L"TEST";
    case OvirOpJmp:     return L"JMP";
    case OvirOpJcc:     return L"JCC";
    case OvirOpCall:    return L"CALL";
    case OvirOpRet:     return L"RET";
    case OvirOpFadd:    return L"FADD";
    case OvirOpFsub:    return L"FSUB";
    case OvirOpFmul:    return L"FMUL";
    case OvirOpFdiv:    return L"FDIV";
    case OvirOpFneg:    return L"FNEG";
    case OvirOpFcmp:    return L"FCMP";
    case OvirOpFmov:    return L"FMOV";
    case OvirOpVecLoad: return L"VLOAD";
    case OvirOpVecStore:return L"VSTORE";
    case OvirOpVecAdd:  return L"VADD";
    case OvirOpVecSub:  return L"VSUB";
    case OvirOpVecMul:  return L"VMUL";
    case OvirOpVecDot:  return L"VDOT";
    case OvirOpSyscall: return L"SYSCALL";
    case OvirOpTrap:    return L"TRAP";
    case OvirOpFence:   return L"FENCE";
    default:            return L"UNKNOWN";
  }
}

CONST CHAR16 *
EFIAPI
OvirCpuArchToString (
  IN OVIR_CPU_ARCH  Arch
  )
{
  switch (Arch) {
    case OvirCpuArchArm64:   return L"ARM64 (AArch64)";
    case OvirCpuArchX64:     return L"x86-64 (AMD64)";
    case OvirCpuArchRiscV64: return L"RISC-V 64";
    default:                 return L"Unknown";
  }
}
