/** @file
  OpenVintage CPU Intermediate Representation Safe Optimizer.
  Phase 4 Safe Transformation Pipeline.

  Performs safe, non-speculative optimization passes: constant folding,
  redundant move elimination, identity operation simplification, and dead
  code removal on basic blocks and IR programs.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OVIR_CPU_OPTIMIZER_LIB_H_
#define OVIR_CPU_OPTIMIZER_LIB_H_

#include <Uefi.h>
#include <Library/OvirCpuLib.h>

//
// Optimization Pass Statistics
//
typedef struct {
  UINT32  ConstantFoldsCount;
  UINT32  RedundantMovesEliminated;
  UINT32  IdentityOpsEliminated;
  UINT32  DeadInstructionsEliminated;
  UINT32  TotalInstructionsBefore;
  UINT32  TotalInstructionsAfter;
} OVIR_OPTIMIZER_STATS;

/**
  Optimize a single basic block using safe non-speculative transformations.

  @param[in,out] Block   Basic block to transform in-place.
  @param[out]    Stats   Receives counts of optimizations applied (optional).

  @retval EFI_SUCCESS    Block optimized cleanly.
**/
EFI_STATUS
EFIAPI
OvirCpuOptimizeBlock (
  IN OUT OVIR_CPU_BASIC_BLOCK  *Block,
  OUT    OVIR_OPTIMIZER_STATS  *Stats OPTIONAL
  );

/**
  Optimize an entire IR program across all basic blocks.

  @param[in,out] Program  Program to optimize.
  @param[out]    Stats    Aggregate statistics (optional).

  @retval EFI_SUCCESS     Program optimized cleanly.
**/
EFI_STATUS
EFIAPI
OvirCpuOptimizeProgram (
  IN OUT OVIR_CPU_PROGRAM       *Program,
  OUT    OVIR_OPTIMIZER_STATS   *Stats OPTIONAL
  );

#endif // OVIR_CPU_OPTIMIZER_LIB_H_
