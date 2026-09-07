/** @file
  OpenVintage Pipeline Abstraction & Cache Interface Definition.
  Phase 3 Modular Pipeline Subsystem.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OVIR_PIPELINE_LIB_H_
#define OVIR_PIPELINE_LIB_H_

#include <Uefi.h>
#include <Library/OvirGpuLib.h>
#include <Library/OvirShaderLib.h>

#define OVIR_MAX_PIPELINES_IN_CACHE  32

//
// Pipeline Object
//
typedef struct {
  OVIR_HANDLE         Handle;
  UINT64              StateHash;
  BOOLEAN             IsCompute;
  OVIR_SHADER_MODULE  *VertexShader;
  OVIR_SHADER_MODULE  *FragmentShader;
  OVIR_SHADER_MODULE  *ComputeShader;
  OVIR_PIPELINE_DESC  Descriptor;
  BOOLEAN             IsValidated;
} OVIR_PIPELINE_STATE;

//
// Pipeline Cache Entry
//
typedef struct {
  UINT64               Hash;
  OVIR_PIPELINE_STATE  Pipeline;
  BOOLEAN              Occupied;
} OVIR_PIPELINE_CACHE_ENTRY;

//
// Pipeline Subsystem Context
//
typedef struct {
  BOOLEAN                    Initialized;
  UINT32                     CacheHits;
  UINT32                     CacheMisses;
  UINT32                     PipelineCount;
  OVIR_PIPELINE_CACHE_ENTRY  Cache[OVIR_MAX_PIPELINES_IN_CACHE];
} OVIR_PIPELINE_SYSTEM;

/**
  Initialize Pipeline subsystem and cache.

  @retval EFI_SUCCESS   Pipeline system ready.
**/
EFI_STATUS
EFIAPI
OvirPipelineInitialize (
  VOID
  );

/**
  Compute composite hash from pipeline descriptor state.

  @param[in] Desc   Pointer to pipeline descriptor.

  @return Computed 64-bit state hash.
**/
UINT64
EFIAPI
OvirPipelineComputeHash (
  IN CONST OVIR_PIPELINE_DESC  *Desc
  );

/**
  Create and validate a pipeline state object.

  @param[in]  Desc         Pipeline configuration descriptor.
  @param[out] OutPipeline  Pointer to receive pipeline object.

  @retval EFI_SUCCESS      Pipeline created and cached.
**/
EFI_STATUS
EFIAPI
OvirPipelineCreate (
  IN  CONST OVIR_PIPELINE_DESC  *Desc,
  OUT OVIR_PIPELINE_STATE       **OutPipeline
  );

/**
  Lookup pipeline in cache by hash.

  @param[in]  StateHash    Precomputed state hash.
  @param[out] OutPipeline  Pointer to receive pipeline if cached.

  @retval EFI_SUCCESS      Found in cache.
  @retval EFI_NOT_FOUND    Cache miss.
**/
EFI_STATUS
EFIAPI
OvirPipelineCacheLookup (
  IN  UINT64               StateHash,
  OUT OVIR_PIPELINE_STATE  **OutPipeline
  );

/**
  Validate a pipeline descriptor against state rules.

  @param[in]  Desc         Pipeline descriptor.
  @param[out] ErrorBuffer  Buffer for error report.
  @param[in]  BufferSize   Buffer size in characters.

  @retval EFI_SUCCESS      Validation passed.
**/
EFI_STATUS
EFIAPI
OvirPipelineValidate (
  IN  CONST OVIR_PIPELINE_DESC  *Desc,
  OUT CHAR16                    *ErrorBuffer,
  IN  UINTN                     BufferSize
  );

/**
  Get pipeline cache statistics.

  @param[out] Hits    Cache hits.
  @param[out] Misses  Cache misses.
**/
VOID
EFIAPI
OvirPipelineGetCacheStats (
  OUT UINT32  *Hits,
  OUT UINT32  *Misses
  );

#endif // OVIR_PIPELINE_LIB_H_
