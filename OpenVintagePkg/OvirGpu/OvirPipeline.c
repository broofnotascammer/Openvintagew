/** @file
  OpenVintage Pipeline System Implementation.
  Phase 3 Modular Pipeline Abstraction, State Management & Cache.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/OvLoggerLib.h>
#include <Library/OvMemoryLib.h>
#include <Library/OvirGpuLib.h>
#include <Library/OvirShaderLib.h>
#include <Library/OvirPipelineLib.h>
#include <Library/OvirPerfLib.h>

STATIC OVIR_PIPELINE_SYSTEM mPipelineSystem = { 0 };
STATIC UINT32               mNextPipelineHandle = 1;

EFI_STATUS
EFIAPI
OvirPipelineInitialize (
  VOID
  )
{
  ZeroMem (&mPipelineSystem, sizeof (OVIR_PIPELINE_SYSTEM));
  mPipelineSystem.Initialized = TRUE;
  OvLogTagged (OV_LOG_LEVEL_DEBUG, L"PIPE", L"Pipeline subsystem & state cache initialized");
  return EFI_SUCCESS;
}

UINT64
EFIAPI
OvirPipelineComputeHash (
  IN CONST OVIR_PIPELINE_DESC  *Desc
  )
{
  CONST UINT8 *Bytes;
  UINT64      Hash = 0xCBF29CE484222325ULL;
  UINTN       Index;

  if (Desc == NULL) {
    return 0;
  }

  Bytes = (CONST UINT8 *)Desc;
  // Hash all descriptor fields excluding the PipelineHash field itself
  for (Index = 0; Index < (sizeof (OVIR_PIPELINE_DESC) - sizeof (UINT64)); Index++) {
    Hash ^= (UINT64)Bytes[Index];
    Hash *= 0x100000001B3ULL;
  }

  return Hash;
}

EFI_STATUS
EFIAPI
OvirPipelineValidate (
  IN  CONST OVIR_PIPELINE_DESC  *Desc,
  OUT CHAR16                    *ErrorBuffer,
  IN  UINTN                     BufferSize
  )
{
  if (Desc == NULL) {
    if (ErrorBuffer != NULL && BufferSize > 0) {
      UnicodeSPrint (ErrorBuffer, BufferSize * sizeof (CHAR16), L"Pipeline descriptor is NULL");
    }
    return EFI_INVALID_PARAMETER;
  }

  if (Desc->IsCompute) {
    if (Desc->ComputeShader == 0 || Desc->ComputeShader == OVIR_INVALID_HANDLE) {
      if (ErrorBuffer != NULL && BufferSize > 0) {
        UnicodeSPrint (ErrorBuffer, BufferSize * sizeof (CHAR16), L"Compute pipeline missing compute shader");
      }
      return EFI_INVALID_PARAMETER;
    }
  } else {
    if (Desc->VertexShader == 0 || Desc->VertexShader == OVIR_INVALID_HANDLE) {
      if (ErrorBuffer != NULL && BufferSize > 0) {
        UnicodeSPrint (ErrorBuffer, BufferSize * sizeof (CHAR16), L"Graphics pipeline missing vertex shader");
      }
      return EFI_INVALID_PARAMETER;
    }
    if (Desc->FragmentShader == 0 || Desc->FragmentShader == OVIR_INVALID_HANDLE) {
      if (ErrorBuffer != NULL && BufferSize > 0) {
        UnicodeSPrint (ErrorBuffer, BufferSize * sizeof (CHAR16), L"Graphics pipeline missing fragment shader");
      }
      return EFI_INVALID_PARAMETER;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirPipelineCacheLookup (
  IN  UINT64               StateHash,
  OUT OVIR_PIPELINE_STATE  **OutPipeline
  )
{
  UINTN Index;

  if (OutPipeline == NULL || StateHash == 0) {
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < OVIR_MAX_PIPELINES_IN_CACHE; Index++) {
    if (mPipelineSystem.Cache[Index].Occupied && mPipelineSystem.Cache[Index].Hash == StateHash) {
      mPipelineSystem.CacheHits++;
      *OutPipeline = &mPipelineSystem.Cache[Index].Pipeline;
      return EFI_SUCCESS;
    }
  }

  mPipelineSystem.CacheMisses++;
  return EFI_NOT_FOUND;
}

EFI_STATUS
EFIAPI
OvirPipelineCreate (
  IN  CONST OVIR_PIPELINE_DESC  *Desc,
  OUT OVIR_PIPELINE_STATE       **OutPipeline
  )
{
  UINT64              Hash;
  OVIR_PIPELINE_STATE *Cached;
  OVIR_PIPELINE_STATE *Pipeline;
  CHAR16              ErrBuf[128];
  EFI_STATUS          Status;

  if (Desc == NULL || OutPipeline == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mPipelineSystem.Initialized) {
    OvirPipelineInitialize ();
  }

  Status = OvirPipelineValidate (Desc, ErrBuf, 128);
  if (EFI_ERROR (Status)) {
    OvLogTagged (OV_LOG_LEVEL_ERROR, L"PIPE", ErrBuf);
    return Status;
  }

  Hash = OvirPipelineComputeHash (Desc);

  // Check cache
  Status = OvirPipelineCacheLookup (Hash, &Cached);
  if (Status == EFI_SUCCESS) {
    OvirPerfRecordCacheEvent (FALSE, TRUE);
    *OutPipeline = Cached;
    return EFI_SUCCESS;
  }

  OvirPerfRecordCacheEvent (FALSE, FALSE);

  UINTN SlotIndex = OVIR_MAX_PIPELINES_IN_CACHE;
  for (UINTN Idx = 0; Idx < OVIR_MAX_PIPELINES_IN_CACHE; Idx++) {
    if (!mPipelineSystem.Cache[Idx].Occupied) {
      SlotIndex = Idx;
      break;
    }
  }

  if (SlotIndex >= OVIR_MAX_PIPELINES_IN_CACHE) {
    return EFI_OUT_OF_RESOURCES;
  }

  Pipeline = &mPipelineSystem.Cache[SlotIndex].Pipeline;
  ZeroMem (Pipeline, sizeof (OVIR_PIPELINE_STATE));
  Pipeline->Handle = mNextPipelineHandle++;
  Pipeline->StateHash = Hash;
  Pipeline->IsCompute = Desc->IsCompute;
  CopyMem (&Pipeline->Descriptor, Desc, sizeof (OVIR_PIPELINE_DESC));
  Pipeline->Descriptor.PipelineHash = Hash;
  Pipeline->IsValidated = TRUE;

  mPipelineSystem.Cache[SlotIndex].Occupied = TRUE;
  mPipelineSystem.Cache[SlotIndex].Hash = Hash;
  mPipelineSystem.PipelineCount++;

  *OutPipeline = Pipeline;
  return EFI_SUCCESS;
}

VOID
EFIAPI
OvirPipelineGetCacheStats (
  OUT UINT32  *Hits,
  OUT UINT32  *Misses
  )
{
  if (Hits != NULL) {
    *Hits = mPipelineSystem.CacheHits;
  }
  if (Misses != NULL) {
    *Misses = mPipelineSystem.CacheMisses;
  }
}
