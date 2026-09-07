/** @file
  OpenVintage CPU Just-In-Time (JIT) Translation Engine.
  Phase 4 JIT Pipeline and Dynamic Block Compilation Architecture.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/OvirCpuJitLib.h>
#include <Library/OvirCpuDecoderLib.h>
#include <Library/OvirCpuOptimizerLib.h>
#include <Library/OvirCpuBackendLib.h>
#include <Library/OvirPerfLib.h>
#include <Library/OvMemoryLib.h>
#include <Library/OvLoggerLib.h>

STATIC OVIR_JIT_STATS mJitStats;
STATIC BOOLEAN        mJitInitialized = FALSE;

EFI_STATUS
EFIAPI
OvirCpuJitInitialize (
  VOID
  )
{
  if (mJitInitialized) {
    return EFI_SUCCESS;
  }

  ZeroMem (&mJitStats, sizeof (OVIR_JIT_STATS));

  OvirCpuDecoderInitialize ();
  OvirCpuBackendInitialize ();
  OvirCpuCacheInitialize ();

  mJitInitialized = TRUE;
  OvLogTagged (OV_LOG_LEVEL_INFO, L"OJIT", L"OVIR-CPU JIT Compilation Engine initialized");
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirCpuJitCompileBlock (
  IN  OVIR_CPU_ARCH   SourceArch,
  IN  OVIR_CPU_ARCH   TargetArch,
  IN  CONST UINT8     *GuestCode,
  IN  UINTN           GuestSize,
  IN  UINT64          GuestPc,
  OUT OVIR_JIT_BLOCK  *JitBlock
  )
{
  EFI_STATUS                Status;
  UINT64                    StartTime;
  UINT64                    EndTime;
  UINT64                    CodeHash;
  OVIR_TRANSLATION_METADATA Meta;
  CONST UINT8               *CachedCode = NULL;
  UINTN                     CachedSize = 0;
  UINT8                     EmitBuffer[512];
  UINTN                     NativeBytes = 0;
  UINTN                     GuestBytesConsumed = 0;
  OVIR_CPU_BASIC_BLOCK      Block;
  OVIR_OPTIMIZER_STATS      OptStats;

  if (GuestCode == NULL || GuestSize == 0 || JitBlock == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mJitInitialized) {
    OvirCpuJitInitialize ();
  }

  StartTime = OvirPerfGetTimestamp ();
  ZeroMem (JitBlock, sizeof (OVIR_JIT_BLOCK));

  CodeHash = OvirCpuCacheComputeCodeHash (GuestCode, GuestSize);

  // 1. Prepare metadata for cache lookup
  Meta.SourceArch = SourceArch;
  Meta.TargetArch = TargetArch;
  Meta.ModuleId = 0x1000;
  Meta.GuestPc = GuestPc;
  Meta.GuestCodeHash = CodeHash;
  Meta.GuestCodeSize = (UINT32)GuestSize;
  Meta.OpenVintageVersion = OPENVINTAGE_CURRENT_VERSION_PACKED;
  Meta.TranslatorVersion = OPENVINTAGE_TRANSLATOR_VERSION;
  Meta.ConfigFlags = 0x01; // Standard optimization level 1

  // 2. Check translation cache
  Status = OvirCpuCacheLookup (&Meta, &CachedCode, &CachedSize);
  if (!EFI_ERROR (Status) && CachedCode != NULL) {
    JitBlock->GuestPc = GuestPc;
    JitBlock->GuestSize = GuestSize;
    JitBlock->NativeCode = (UINT8 *)CachedCode;
    JitBlock->NativeCodeSize = CachedSize;
    JitBlock->SourceArch = SourceArch;
    JitBlock->TargetArch = TargetArch;
    JitBlock->EntryPointer = (OVIR_JIT_ENTRY_FN)(VOID *)CachedCode;
    JitBlock->IsExecutable = TRUE;
    return EFI_SUCCESS;
  }

  // 3. Decode guest machine instructions into OVIR-CPU IR
  Status = OvirCpuDecodeBlock (
    SourceArch,
    GuestCode,
    GuestSize,
    GuestPc,
    OVIR_MAX_BLOCK_INSTRUCTIONS,
    &Block,
    &GuestBytesConsumed
    );

  if (EFI_ERROR (Status)) {
    OvLogTagged (OV_LOG_LEVEL_ERROR, L"OJIT", L"Failed decoding guest block at 0x%lx: %r", GuestPc, Status);
    return Status;
  }

  // 4. Optimize IR block (constant folding, dead code elimination, etc.)
  OvirCpuOptimizeBlock (&Block, &OptStats);

  // 5. Emit target native machine code
  Status = OvirCpuEmitBlock (
    TargetArch,
    &Block,
    EmitBuffer,
    sizeof (EmitBuffer),
    &NativeBytes
    );

  if (EFI_ERROR (Status)) {
    OvLogTagged (OV_LOG_LEVEL_ERROR, L"OJIT", L"Failed emitting native code for block at 0x%lx: %r", GuestPc, Status);
    return Status;
  }

  // 6. Store into translation cache
  Status = OvirCpuCacheStore (&Meta, EmitBuffer, NativeBytes);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Retrieve cached pointer
  Status = OvirCpuCacheLookup (&Meta, &CachedCode, &CachedSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  EndTime = OvirPerfGetTimestamp ();

  // Fill JIT block
  JitBlock->GuestPc = GuestPc;
  JitBlock->GuestSize = GuestBytesConsumed;
  JitBlock->NativeCode = (UINT8 *)CachedCode;
  JitBlock->NativeCodeSize = CachedSize;
  JitBlock->SourceArch = SourceArch;
  JitBlock->TargetArch = TargetArch;
  JitBlock->EntryPointer = (OVIR_JIT_ENTRY_FN)(VOID *)CachedCode;
  JitBlock->IsExecutable = TRUE;

  // Update statistics
  mJitStats.BlocksCompiled++;
  mJitStats.BytesTranslated += GuestBytesConsumed;
  mJitStats.NativeBytesEmitted += CachedSize;
  if (EndTime > StartTime) {
    mJitStats.TotalCompileTimeCycles += (EndTime - StartTime);
  }

  OvLogTagged (OV_LOG_LEVEL_DEBUG, L"OJIT",
               L"JIT Compiled block 0x%lx: %u guest bytes -> %u native bytes (%u folded, %u dead removed)",
               GuestPc, (UINT32)GuestBytesConsumed, (UINT32)CachedSize,
               OptStats.ConstantFoldsCount, OptStats.DeadInstructionsEliminated);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirCpuJitExecute (
  IN  CONST OVIR_JIT_BLOCK *JitBlock,
  IN  UINT64               Arg0,
  IN  UINT64               Arg1,
  OUT UINT64               *Result
  )
{
  if (JitBlock == NULL || Result == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!JitBlock->IsExecutable || JitBlock->EntryPointer == NULL) {
    return EFI_NOT_READY;
  }

  // Execute native compiled code
  *Result = JitBlock->EntryPointer (Arg0, Arg1);

  ((OVIR_JIT_BLOCK *)JitBlock)->InvocationCount++;
  mJitStats.Invocations++;

  return EFI_SUCCESS;
}

VOID
EFIAPI
OvirCpuJitGetStats (
  OUT OVIR_JIT_STATS  *Stats
  )
{
  if (Stats != NULL) {
    CopyMem (Stats, &mJitStats, sizeof (OVIR_JIT_STATS));
  }
}
