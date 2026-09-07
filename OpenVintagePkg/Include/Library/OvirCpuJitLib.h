/** @file
  OpenVintage CPU Just-In-Time (JIT) Translation Engine.
  Phase 4 JIT Pipeline and Dynamic Block Compilation Architecture.

  Coordinates Decoding, IR Construction, Optimization, Target Emission,
  Memory Management, and Execution.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OVIR_CPU_JIT_LIB_H_
#define OVIR_CPU_JIT_LIB_H_

#include <Uefi.h>
#include <Library/OvirCpuLib.h>
#include <Library/OvirCpuCacheLib.h>

//
// Native JIT Block Execution Signature
//
typedef
UINT64
(EFIAPI *OVIR_JIT_ENTRY_FN) (
  IN UINT64 Arg0,
  IN UINT64 Arg1
  );

//
// Compiled JIT Block Descriptor
//
typedef struct {
  UINT64              GuestPc;
  UINTN               GuestSize;
  UINT8               *NativeCode;
  UINTN               NativeCodeSize;
  OVIR_CPU_ARCH       SourceArch;
  OVIR_CPU_ARCH       TargetArch;
  OVIR_JIT_ENTRY_FN   EntryPointer;
  UINT64              InvocationCount;
  UINT64              TotalCycles;
  BOOLEAN             IsExecutable;
} OVIR_JIT_BLOCK;

//
// JIT Runtime Statistics
//
typedef struct {
  UINT64  BlocksCompiled;
  UINT64  BytesTranslated;
  UINT64  NativeBytesEmitted;
  UINT64  Invocations;
  UINT64  TotalCompileTimeCycles;
} OVIR_JIT_STATS;

/**
  Initialize JIT compiler framework.

  @retval EFI_SUCCESS   JIT engine initialized.
**/
EFI_STATUS
EFIAPI
OvirCpuJitInitialize (
  VOID
  );

/**
  Compile a block of guest instructions into an executable native block.

  Runs the full translation pipeline:
    Input Binary -> Decoder -> OVIR-CPU IR -> Optimizer -> Backend -> Executable Memory.

  @param[in]  SourceArch   Source architecture (e.g., OvirCpuArchArm64).
  @param[in]  TargetArch   Target architecture (e.g., OvirCpuArchX64).
  @param[in]  GuestCode    Raw guest machine instructions.
  @param[in]  GuestSize    Byte count of guest code.
  @param[in]  GuestPc      Virtual guest instruction pointer.
  @param[out] JitBlock     Receives compiled JIT block descriptor.

  @retval EFI_SUCCESS      Compilation successful.
  @retval EFI_UNSUPPORTED  Unsupported instructions or architecture.
**/
EFI_STATUS
EFIAPI
OvirCpuJitCompileBlock (
  IN  OVIR_CPU_ARCH   SourceArch,
  IN  OVIR_CPU_ARCH   TargetArch,
  IN  CONST UINT8     *GuestCode,
  IN  UINTN           GuestSize,
  IN  UINT64          GuestPc,
  OUT OVIR_JIT_BLOCK  *JitBlock
  );

/**
  Execute a compiled JIT block.

  @param[in]  JitBlock    JIT block to execute.
  @param[in]  Arg0        First input parameter.
  @param[in]  Arg1        Second input parameter.
  @param[out] Result      Output returned from execution.

  @retval EFI_SUCCESS     Executed cleanly.
  @retval EFI_INVALID_PARAMETER Block invalid or not executable.
**/
EFI_STATUS
EFIAPI
OvirCpuJitExecute (
  IN  CONST OVIR_JIT_BLOCK *JitBlock,
  IN  UINT64               Arg0,
  IN  UINT64               Arg1,
  OUT UINT64               *Result
  );

/**
  Retrieve JIT subsystem metrics.

  @param[out] Stats   Receives JIT metrics.
**/
VOID
EFIAPI
OvirCpuJitGetStats (
  OUT OVIR_JIT_STATS  *Stats
  );

#endif // OVIR_CPU_JIT_LIB_H_
