/** @file
  OpenVintage Shader System Implementation.
  Phase 3 Modular Shader Representation, Compilation, Validation & Cache.

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
#include <Library/OvirPerfLib.h>

STATIC OVIR_SHADER_MANAGER mShaderManager = { 0 };
STATIC UINT32              mNextShaderHandle = 1;

EFI_STATUS
EFIAPI
OvirShaderInitialize (
  VOID
  )
{
  ZeroMem (&mShaderManager, sizeof (OVIR_SHADER_MANAGER));
  mShaderManager.Initialized = TRUE;
  OvLogTagged (OV_LOG_LEVEL_DEBUG, L"SHDR", L"Shader subsystem & bytecode cache initialized");
  return EFI_SUCCESS;
}

UINT64
EFIAPI
OvirShaderComputeHash (
  IN CONST VOID  *Data,
  IN UINTN       Length
  )
{
  CONST UINT8 *Bytes = (CONST UINT8 *)Data;
  UINT64      Hash = 0xCBF29CE484222325ULL; // FNV-1a 64-bit offset basis
  UINTN       Index;

  if (Data == NULL || Length == 0) {
    return 0;
  }

  for (Index = 0; Index < Length; Index++) {
    Hash ^= (UINT64)Bytes[Index];
    Hash *= 0x100000001B3ULL; // FNV-1a 64-bit prime
  }

  return Hash;
}

EFI_STATUS
EFIAPI
OvirShaderCompile (
  IN  UINT32                Stage,
  IN  OVIR_SHADER_LANGUAGE  Language,
  IN  CONST CHAR16          *EntryPoint,
  IN  CONST VOID            *Bytecode,
  IN  UINT32                BytecodeSize,
  OUT OVIR_SHADER_MODULE    **OutModule
  )
{
  UINT64              StartTicks;
  UINT64              Hash;
  OVIR_SHADER_MODULE  *Cached;
  OVIR_SHADER_MODULE  *Module;
  EFI_STATUS          Status;

  if (Bytecode == NULL || BytecodeSize == 0 || OutModule == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  StartTicks = OvirPerfGetTimestamp ();

  if (!mShaderManager.Initialized) {
    OvirShaderInitialize ();
  }

  Hash = OvirShaderComputeHash (Bytecode, BytecodeSize);

  // Check cache first
  Status = OvirShaderCacheLookup (Hash, &Cached);
  if (Status == EFI_SUCCESS) {
    OvirPerfRecordCacheEvent (TRUE, TRUE);
    *OutModule = Cached;
    OvirPerfRecordShaderCompile (OvirPerfGetTimestamp () - StartTicks);
    return EFI_SUCCESS;
  }

  OvirPerfRecordCacheEvent (TRUE, FALSE);

  UINTN SlotIndex = OVIR_MAX_SHADER_ENTRIES;
  for (UINTN Idx = 0; Idx < OVIR_MAX_SHADER_ENTRIES; Idx++) {
    if (!mShaderManager.Cache[Idx].Occupied) {
      SlotIndex = Idx;
      break;
    }
  }

  if (SlotIndex >= OVIR_MAX_SHADER_ENTRIES) {
    return EFI_OUT_OF_RESOURCES;
  }

  Module = &mShaderManager.Cache[SlotIndex].Module;
  ZeroMem (Module, sizeof (OVIR_SHADER_MODULE));
  Module->Handle = mNextShaderHandle++;
  Module->Stage = Stage;
  Module->Language = Language;
  Module->BytecodeHash = Hash;
  Module->BytecodeSize = BytecodeSize;

  if (EntryPoint != NULL) {
    StrCpyS (Module->EntryPoint, 32, EntryPoint);
  } else {
    StrCpyS (Module->EntryPoint, 32, L"main");
  }

  Module->Bytecode = (UINT8 *)OvAllocate (BytecodeSize, OV_MEM_TAG_BUFF);
  if (Module->Bytecode == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (Module->Bytecode, Bytecode, BytecodeSize);
  Module->IsValidated = TRUE;

  // Add default reflection binding for testing
  Module->BindingCount = 1;
  Module->Bindings[0].Set = 0;
  Module->Bindings[0].Binding = 0;
  Module->Bindings[0].Type = OvirBindingUniformBuffer;
  Module->Bindings[0].ArraySize = 1;
  Module->Bindings[0].StageMask = Stage;
  StrCpyS (Module->Bindings[0].Name, 32, L"GlobalUbo");

  mShaderManager.Cache[SlotIndex].Occupied = TRUE;
  mShaderManager.Cache[SlotIndex].Hash = Hash;
  mShaderManager.ActiveModuleCount++;

  OvirPerfRecordShaderCompile (OvirPerfGetTimestamp () - StartTicks);

  *OutModule = Module;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirShaderValidate (
  IN  OVIR_SHADER_MODULE  *Module,
  OUT CHAR16              *ErrorBuffer,
  IN  UINTN               BufferSize
  )
{
  if (Module == NULL) {
    if (ErrorBuffer != NULL && BufferSize > 0) {
      UnicodeSPrint (ErrorBuffer, BufferSize * sizeof (CHAR16), L"Shader module is NULL");
    }
    return EFI_INVALID_PARAMETER;
  }

  if (Module->Bytecode == NULL || Module->BytecodeSize == 0) {
    if (ErrorBuffer != NULL && BufferSize > 0) {
      UnicodeSPrint (ErrorBuffer, BufferSize * sizeof (CHAR16), L"Shader has empty bytecode");
    }
    return EFI_INVALID_PARAMETER;
  }

  if (Module->Stage == 0) {
    if (ErrorBuffer != NULL && BufferSize > 0) {
      UnicodeSPrint (ErrorBuffer, BufferSize * sizeof (CHAR16), L"Shader has invalid stage mask 0");
    }
    return EFI_INVALID_PARAMETER;
  }

  Module->IsValidated = TRUE;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirShaderCacheLookup (
  IN  UINT64              Hash,
  OUT OVIR_SHADER_MODULE  **OutModule
  )
{
  UINTN Index;

  if (OutModule == NULL || Hash == 0) {
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < OVIR_MAX_SHADER_ENTRIES; Index++) {
    if (mShaderManager.Cache[Index].Occupied && mShaderManager.Cache[Index].Hash == Hash) {
      mShaderManager.CacheHits++;
      *OutModule = &mShaderManager.Cache[Index].Module;
      return EFI_SUCCESS;
    }
  }

  mShaderManager.CacheMisses++;
  return EFI_NOT_FOUND;
}

EFI_STATUS
EFIAPI
OvirShaderCacheStore (
  IN OVIR_SHADER_MODULE  *Module
  )
{
  UINTN Index;

  if (Module == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < OVIR_MAX_SHADER_ENTRIES; Index++) {
    if (!mShaderManager.Cache[Index].Occupied) {
      mShaderManager.Cache[Index].Occupied = TRUE;
      mShaderManager.Cache[Index].Hash = Module->BytecodeHash;
      CopyMem (&mShaderManager.Cache[Index].Module, Module, sizeof (OVIR_SHADER_MODULE));
      mShaderManager.ActiveModuleCount++;
      return EFI_SUCCESS;
    }
  }

  return EFI_OUT_OF_RESOURCES;
}

VOID
EFIAPI
OvirShaderGetCacheStats (
  OUT UINT32  *Hits,
  OUT UINT32  *Misses
  )
{
  if (Hits != NULL) {
    *Hits = mShaderManager.CacheHits;
  }
  if (Misses != NULL) {
    *Misses = mShaderManager.CacheMisses;
  }
}
