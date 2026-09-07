/** @file
  OpenVintage Shader Management, Compilation & Cache Interface Definition.
  Phase 3 Modular Shader Subsystem.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OVIR_SHADER_LIB_H_
#define OVIR_SHADER_LIB_H_

#include <Uefi.h>
#include <Library/OvirGpuLib.h>

#define OVIR_MAX_SHADER_BINDINGS   32
#define OVIR_MAX_PUSH_CONSTANTS    8
#define OVIR_MAX_SHADER_ENTRIES    64

//
// Source & Intermediate Shader Languages
//
typedef enum {
  OvirShaderLangSpirV = 1,
  OvirShaderLangMsl,
  OvirShaderLangGlsl,
  OvirShaderLangHlsl,
  OvirShaderLangOvirBytecode
} OVIR_SHADER_LANGUAGE;

//
// Shader Binding Reflection
//
typedef enum {
  OvirBindingUniformBuffer = 1,
  OvirBindingStorageBuffer,
  OvirBindingSampledTexture,
  OvirBindingStorageTexture,
  OvirBindingSampler
} OVIR_BINDING_TYPE;

typedef struct {
  UINT32             Set;
  UINT32             Binding;
  OVIR_BINDING_TYPE  Type;
  UINT32             ArraySize;
  UINT32             StageMask;
  CHAR16             Name[32];
} OVIR_SHADER_BINDING;

//
// Push Constant Range
//
typedef struct {
  UINT32 StageMask;
  UINT32 Offset;
  UINT32 Size;
} OVIR_PUSH_CONSTANT_RANGE;

//
// Shader Module Object
//
typedef struct {
  OVIR_HANDLE              Handle;
  UINT32                   Stage; // OVIR_SHADER_STAGE_*
  OVIR_SHADER_LANGUAGE     Language;
  CHAR16                   EntryPoint[32];
  UINT64                   BytecodeHash; // Composite 64-bit hash
  UINT32                   BytecodeSize;
  UINT8                    *Bytecode;
  UINT32                   BindingCount;
  OVIR_SHADER_BINDING      Bindings[OVIR_MAX_SHADER_BINDINGS];
  UINT32                   PushConstantCount;
  OVIR_PUSH_CONSTANT_RANGE PushConstants[OVIR_MAX_PUSH_CONSTANTS];
  BOOLEAN                  IsValidated;
} OVIR_SHADER_MODULE;

//
// Shader Cache Entry
//
typedef struct {
  UINT64              Hash;
  OVIR_SHADER_MODULE  Module;
  BOOLEAN             Occupied;
} OVIR_SHADER_CACHE_ENTRY;

//
// Shader Manager Context
//
typedef struct {
  BOOLEAN                  Initialized;
  UINT32                   CacheHits;
  UINT32                   CacheMisses;
  UINT32                   ActiveModuleCount;
  OVIR_SHADER_CACHE_ENTRY  Cache[OVIR_MAX_SHADER_ENTRIES];
} OVIR_SHADER_MANAGER;

/**
  Initialize Shader Subsystem and cache.

  @retval EFI_SUCCESS   Shader manager ready.
**/
EFI_STATUS
EFIAPI
OvirShaderInitialize (
  VOID
  );

/**
  Calculate 64-bit composite hash for shader bytecode.

  @param[in] Data   Bytecode pointer.
  @param[in] Length Byte count.

  @return Hash value.
**/
UINT64
EFIAPI
OvirShaderComputeHash (
  IN CONST VOID  *Data,
  IN UINTN       Length
  );

/**
  Compile or ingest shader into an OVIR Shader Module.

  @param[in]  Stage        Shader stage mask (Vertex, Fragment, Compute, etc.)
  @param[in]  Language     Input language.
  @param[in]  EntryPoint   Name of entry point (e.g., L"main")
  @param[in]  Bytecode     Bytecode or source text buffer.
  @param[in]  BytecodeSize Byte count.
  @param[out] OutModule    Pointer to receive created module.

  @retval EFI_SUCCESS      Compilation / ingestion succeeded.
**/
EFI_STATUS
EFIAPI
OvirShaderCompile (
  IN  UINT32                Stage,
  IN  OVIR_SHADER_LANGUAGE  Language,
  IN  CONST CHAR16          *EntryPoint,
  IN  CONST VOID            *Bytecode,
  IN  UINT32                BytecodeSize,
  OUT OVIR_SHADER_MODULE    **OutModule
  );

/**
  Validate shader module interface and resource bindings.

  @param[in]  Module       Shader module pointer.
  @param[out] ErrorBuffer  Buffer for error description.
  @param[in]  BufferSize   Size of error buffer in characters.

  @retval EFI_SUCCESS      Validation clean.
**/
EFI_STATUS
EFIAPI
OvirShaderValidate (
  IN  OVIR_SHADER_MODULE  *Module,
  OUT CHAR16              *ErrorBuffer,
  IN  UINTN               BufferSize
  );

/**
  Query shader cache by hash.

  @param[in]  Hash        Computed 64-bit hash.
  @param[out] OutModule   Module if found.

  @retval EFI_SUCCESS     Cache hit.
  @retval EFI_NOT_FOUND   Cache miss.
**/
EFI_STATUS
EFIAPI
OvirShaderCacheLookup (
  IN  UINT64              Hash,
  OUT OVIR_SHADER_MODULE  **OutModule
  );

/**
  Store shader module into cache.

  @param[in] Module       Module to cache.

  @retval EFI_SUCCESS     Cached.
**/
EFI_STATUS
EFIAPI
OvirShaderCacheStore (
  IN OVIR_SHADER_MODULE  *Module
  );

/**
  Retrieve shader cache performance metrics.

  @param[out] Hits        Number of cache hits.
  @param[out] Misses      Number of cache misses.
**/
VOID
EFIAPI
OvirShaderGetCacheStats (
  OUT UINT32  *Hits,
  OUT UINT32  *Misses
  );

#endif // OVIR_SHADER_LIB_H_
