/** @file
  OpenVintage CPU Intermediate Representation Safe Optimizer.
  Phase 4 Safe Non-Speculative Transformation Layer.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/OvirCpuOptimizerLib.h>

EFI_STATUS
EFIAPI
OvirCpuOptimizeBlock (
  IN OUT OVIR_CPU_BASIC_BLOCK  *Block,
  OUT    OVIR_OPTIMIZER_STATS  *Stats OPTIONAL
  )
{
  UINT32              Idx;
  OVIR_OPTIMIZER_STATS LocalStats;

  if (Block == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&LocalStats, sizeof (OVIR_OPTIMIZER_STATS));
  LocalStats.TotalInstructionsBefore = Block->InstructionCount;

  // Pass 1: Constant folding, identity simplification & redundant move elimination
  for (Idx = 0; Idx < Block->InstructionCount; Idx++) {
    OVIR_CPU_INSTRUCTION *Inst = &Block->Instructions[Idx];

    // Redundant MOV: MOV Rx, Rx
    if (Inst->Opcode == OvirOpMov && Inst->OperandCount == 2) {
      if (Inst->Operands[0].Kind == OvirOpKindReg &&
          Inst->Operands[1].Kind == OvirOpKindReg &&
          Inst->Operands[0].As.Reg.Index == Inst->Operands[1].As.Reg.Index &&
          Inst->Operands[0].As.Reg.Class == Inst->Operands[1].As.Reg.Class) {
        Inst->Opcode = OvirOpNop;
        Inst->OperandCount = 0;
        LocalStats.RedundantMovesEliminated++;
        continue;
      }
    }

    // Identity Operations:
    // ADD Rx, Rx, 0  /  SUB Rx, Rx, 0  /  MUL Rx, Rx, 1  /  OR Rx, Rx, 0
    if (Inst->OperandCount == 3 &&
        Inst->Operands[0].Kind == OvirOpKindReg &&
        Inst->Operands[1].Kind == OvirOpKindReg &&
        Inst->Operands[0].As.Reg.Index == Inst->Operands[1].As.Reg.Index &&
        Inst->Operands[2].Kind == OvirOpKindImm &&
        !Inst->SetFlags) {

      UINT64 ImmVal = Inst->Operands[2].As.Imm.Value.UintVal;

      if ((Inst->Opcode == OvirOpAdd || Inst->Opcode == OvirOpSub || Inst->Opcode == OvirOpOr) && ImmVal == 0) {
        Inst->Opcode = OvirOpNop;
        Inst->OperandCount = 0;
        LocalStats.IdentityOpsEliminated++;
        continue;
      }

      if (Inst->Opcode == OvirOpMul && ImmVal == 1) {
        Inst->Opcode = OvirOpNop;
        Inst->OperandCount = 0;
        LocalStats.IdentityOpsEliminated++;
        continue;
      }

      if (Inst->Opcode == OvirOpAnd) {
        UINT64 Mask = (Inst->BitWidth == 64) ? 0xFFFFFFFFFFFFFFFFULL :
                      ((1ULL << Inst->BitWidth) - 1);
        if (ImmVal == Mask) {
          Inst->Opcode = OvirOpNop;
          Inst->OperandCount = 0;
          LocalStats.IdentityOpsEliminated++;
          continue;
        }
      }
    }

    // Constant Folding: Op Dst, ImmA, ImmB
    if (Inst->OperandCount == 3 &&
        Inst->Operands[0].Kind == OvirOpKindReg &&
        Inst->Operands[1].Kind == OvirOpKindImm &&
        Inst->Operands[2].Kind == OvirOpKindImm &&
        !Inst->SetFlags) {

      UINT64 A = Inst->Operands[1].As.Imm.Value.UintVal;
      UINT64 B = Inst->Operands[2].As.Imm.Value.UintVal;
      UINT64 Res = 0;
      BOOLEAN Folded = FALSE;

      switch (Inst->Opcode) {
        case OvirOpAdd: Res = A + B; Folded = TRUE; break;
        case OvirOpSub: Res = A - B; Folded = TRUE; break;
        case OvirOpMul: Res = A * B; Folded = TRUE; break;
        case OvirOpAnd: Res = A & B; Folded = TRUE; break;
        case OvirOpOr:  Res = A | B; Folded = TRUE; break;
        case OvirOpXor: Res = A ^ B; Folded = TRUE; break;
        default: break;
      }

      if (Folded) {
        // Transform into MOV Dst, ImmResult
        Inst->Opcode = OvirOpMov;
        Inst->OperandCount = 2;
        Inst->Operands[1].Kind = OvirOpKindImm;
        Inst->Operands[1].As.Imm.Value.UintVal = Res;
        Inst->Operands[1].As.Imm.BitWidth = Inst->BitWidth;
        Inst->Operands[1].As.Imm.IsSigned = FALSE;
        LocalStats.ConstantFoldsCount++;
      }
    }
  }

  // Pass 2: Dead Code Elimination (instructions following unconditional JMP / RET in same block)
  for (Idx = 0; Idx < Block->InstructionCount; Idx++) {
    if (Block->Instructions[Idx].Opcode == OvirOpJmp ||
        Block->Instructions[Idx].Opcode == OvirOpRet ||
        Block->Instructions[Idx].Opcode == OvirOpTrap) {
      if (Idx + 1 < Block->InstructionCount) {
        LocalStats.DeadInstructionsEliminated += (Block->InstructionCount - (Idx + 1));
        Block->InstructionCount = Idx + 1;
        Block->IsTerminated = TRUE;
      }
      break;
    }
  }

  // Pass 3: NOP compaction
  UINT32 WriteIdx = 0;
  for (Idx = 0; Idx < Block->InstructionCount; Idx++) {
    if (Block->Instructions[Idx].Opcode != OvirOpNop) {
      if (WriteIdx != Idx) {
        CopyMem (&Block->Instructions[WriteIdx], &Block->Instructions[Idx], sizeof (OVIR_CPU_INSTRUCTION));
      }
      WriteIdx++;
    }
  }
  Block->InstructionCount = WriteIdx;
  LocalStats.TotalInstructionsAfter = Block->InstructionCount;

  if (Stats != NULL) {
    CopyMem (Stats, &LocalStats, sizeof (OVIR_OPTIMIZER_STATS));
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirCpuOptimizeProgram (
  IN OUT OVIR_CPU_PROGRAM       *Program,
  OUT    OVIR_OPTIMIZER_STATS   *Stats OPTIONAL
  )
{
  UINT32              Idx;
  OVIR_OPTIMIZER_STATS AggStats;

  if (Program == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&AggStats, sizeof (OVIR_OPTIMIZER_STATS));
  Program->TotalInstructionCount = 0;

  for (Idx = 0; Idx < Program->BlockCount; Idx++) {
    OVIR_OPTIMIZER_STATS BlockStats;
    OvirCpuOptimizeBlock (&Program->Blocks[Idx], &BlockStats);

    AggStats.ConstantFoldsCount += BlockStats.ConstantFoldsCount;
    AggStats.RedundantMovesEliminated += BlockStats.RedundantMovesEliminated;
    AggStats.IdentityOpsEliminated += BlockStats.IdentityOpsEliminated;
    AggStats.DeadInstructionsEliminated += BlockStats.DeadInstructionsEliminated;
    AggStats.TotalInstructionsBefore += BlockStats.TotalInstructionsBefore;
    AggStats.TotalInstructionsAfter += BlockStats.TotalInstructionsAfter;

    Program->TotalInstructionCount += Program->Blocks[Idx].InstructionCount;
  }

  if (Stats != NULL) {
    CopyMem (Stats, &AggStats, sizeof (OVIR_OPTIMIZER_STATS));
  }

  return EFI_SUCCESS;
}
