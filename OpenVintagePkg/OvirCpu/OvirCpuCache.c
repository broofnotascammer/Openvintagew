/** @file
  OpenVintage CPU Translation Cache & Invalidation Framework.
  Phase 4 Translation Cache Architecture.

  Provides fast translation caching with strict metadata validation
  (Source/Target Arch, Module Identity, Versioning, Configuration Flags).

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/OvirCpuCacheLib.h>
#include <Library/OvMemoryLib.h>
#include <Library/OvLoggerLib.h>

STATIC OVIR_CPU_CACHE_ENTRY mCacheEntries[OVIR_CPU_CACHE_MAX_ENTRIES];
STATIC OVIR_CPU_CACHE_STATS mCacheStats;
STATIC BOOLEAN              mCacheInitialized = FALSE;

UINT64
EFIAPI
OvirCpuCacheComputeCodeHash (
  IN CONST UINT8  *Code,
  IN UINTN        Size
  )
{
  UINT64 Hash = 0xCBF29CE484222325ULL; // FNV-1a 64-bit offset basis
  UINTN  Idx;

  if (Code == NULL || Size == 0) {
    return 0;
  }

  for (Idx = 0; Idx < Size; Idx++) {
    Hash ^= (UINT64)Code[Idx];
    Hash *= 0x100000001B3ULL; // FNV-1a 64-bit prime
  }

  return Hash;
}

EFI_STATUS
EFIAPI
OvirCpuCacheInitialize (
  VOID
  )
{
  UINTN Idx;

  if (mCacheInitialized) {
    OvirCpuCacheFlush ();
  }

  ZeroMem (mCacheEntries, sizeof (mCacheEntries));
  ZeroMem (&mCacheStats, sizeof (mCacheStats));

  for (Idx = 0; Idx < OVIR_CPU_CACHE_MAX_ENTRIES; Idx++) {
    mCacheEntries[Idx].IsValid = FALSE;
  }

  mCacheInitialized = TRUE;
  OvLogTagged (OV_LOG_LEVEL_INFO, L"OCPU", L"OVIR-CPU Translation Cache initialized (%u max slots)",
               OVIR_CPU_CACHE_MAX_ENTRIES);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirCpuCacheLookup (
  IN  CONST OVIR_TRANSLATION_METADATA *Query,
  OUT CONST UINT8                     **NativeCode,
  OUT UINTN                           *NativeSize
  )
{
  UINTN Idx;

  if (Query == NULL || NativeCode == NULL || NativeSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mCacheInitialized) {
    OvirCpuCacheInitialize ();
  }

  mCacheStats.LookupsCount++;

  for (Idx = 0; Idx < OVIR_CPU_CACHE_MAX_ENTRIES; Idx++) {
    OVIR_CPU_CACHE_ENTRY *Entry = &mCacheEntries[Idx];

    if (!Entry->IsValid) {
      continue;
    }

    // Match all metadata criteria
    if (Entry->Metadata.SourceArch == Query->SourceArch &&
        Entry->Metadata.TargetArch == Query->TargetArch &&
        Entry->Metadata.ModuleId == Query->ModuleId &&
        Entry->Metadata.GuestPc == Query->GuestPc &&
        Entry->Metadata.GuestCodeHash == Query->GuestCodeHash &&
        Entry->Metadata.OpenVintageVersion == Query->OpenVintageVersion &&
        Entry->Metadata.TranslatorVersion == Query->TranslatorVersion &&
        Entry->Metadata.ConfigFlags == Query->ConfigFlags) {

      Entry->AccessCount++;
      mCacheStats.HitsCount++;
      *NativeCode = Entry->NativeCode;
      *NativeSize = Entry->NativeCodeSize;
      return EFI_SUCCESS;
    }
  }

  mCacheStats.MissesCount++;
  return EFI_NOT_FOUND;
}

EFI_STATUS
EFIAPI
OvirCpuCacheStore (
  IN CONST OVIR_TRANSLATION_METADATA *Metadata,
  IN CONST UINT8                     *NativeCode,
  IN UINTN                           NativeSize
  )
{
  UINTN Idx;
  UINTN TargetSlot = OVIR_CPU_CACHE_MAX_ENTRIES;
  UINT64 LowestAccess = 0xFFFFFFFFFFFFFFFFULL;
  UINTN  LruSlot = 0;

  if (Metadata == NULL || NativeCode == NULL || NativeSize == 0) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mCacheInitialized) {
    OvirCpuCacheInitialize ();
  }

  // 1. Check if matching entry already exists (update it)
  for (Idx = 0; Idx < OVIR_CPU_CACHE_MAX_ENTRIES; Idx++) {
    if (mCacheEntries[Idx].IsValid &&
        mCacheEntries[Idx].Metadata.SourceArch == Metadata->SourceArch &&
        mCacheEntries[Idx].Metadata.TargetArch == Metadata->TargetArch &&
        mCacheEntries[Idx].Metadata.GuestPc == Metadata->GuestPc &&
        mCacheEntries[Idx].Metadata.ModuleId == Metadata->ModuleId) {
      TargetSlot = Idx;
      break;
    }
  }

  // 2. Otherwise find an unused slot, or track LRU
  if (TargetSlot == OVIR_CPU_CACHE_MAX_ENTRIES) {
    for (Idx = 0; Idx < OVIR_CPU_CACHE_MAX_ENTRIES; Idx++) {
      if (!mCacheEntries[Idx].IsValid) {
        TargetSlot = Idx;
        break;
      }
      if (mCacheEntries[Idx].AccessCount < LowestAccess) {
        LowestAccess = mCacheEntries[Idx].AccessCount;
        LruSlot = Idx;
      }
    }
  }

  // 3. If full, evict LRU
  if (TargetSlot == OVIR_CPU_CACHE_MAX_ENTRIES) {
    TargetSlot = LruSlot;
    if (mCacheEntries[TargetSlot].NativeCode != NULL) {
      OvFree (mCacheEntries[TargetSlot].NativeCode);
      mCacheEntries[TargetSlot].NativeCode = NULL;
    }
    mCacheEntries[TargetSlot].IsValid = FALSE;
    mCacheStats.ActiveEntries--;
  }

  OVIR_CPU_CACHE_ENTRY *Entry = &mCacheEntries[TargetSlot];

  // Free existing buffer if slot had one
  if (Entry->NativeCode != NULL) {
    OvFree (Entry->NativeCode);
    Entry->NativeCode = NULL;
  }

  // Allocate tracked memory for compiled code
  Entry->NativeCode = (UINT8 *)OvAllocate (NativeSize, OV_MEM_TAG_OCPU);
  if (Entry->NativeCode == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (Entry->NativeCode, NativeCode, NativeSize);
  Entry->NativeCodeSize = NativeSize;
  CopyMem (&Entry->Metadata, Metadata, sizeof (OVIR_TRANSLATION_METADATA));
  Entry->AccessCount = 1;
  Entry->IsValid = TRUE;

  mCacheStats.ActiveEntries++;
  mCacheStats.TotalAllocatedBytes += NativeSize;

  return EFI_SUCCESS;
}

UINTN
EFIAPI
OvirCpuCacheInvalidateIncompatible (
  IN UINT32  ExpectedOvVersion,
  IN UINT32  ExpectedTranslatorVersion,
  IN UINT32  ExpectedConfigFlags
  )
{
  UINTN Idx;
  UINTN InvalidationCount = 0;

  for (Idx = 0; Idx < OVIR_CPU_CACHE_MAX_ENTRIES; Idx++) {
    OVIR_CPU_CACHE_ENTRY *Entry = &mCacheEntries[Idx];

    if (!Entry->IsValid) {
      continue;
    }

    if (Entry->Metadata.OpenVintageVersion != ExpectedOvVersion ||
        Entry->Metadata.TranslatorVersion != ExpectedTranslatorVersion ||
        Entry->Metadata.ConfigFlags != ExpectedConfigFlags) {

      if (Entry->NativeCode != NULL) {
        OvFree (Entry->NativeCode);
        Entry->NativeCode = NULL;
      }
      Entry->IsValid = FALSE;
      mCacheStats.ActiveEntries--;
      mCacheStats.InvalidationsCount++;
      InvalidationCount++;
    }
  }

  if (InvalidationCount > 0) {
    OvLogTagged (OV_LOG_LEVEL_INFO, L"OCPU", L"Invalidated %u incompatible translation cache entries",
                 (UINT32)InvalidationCount);
  }

  return InvalidationCount;
}

VOID
EFIAPI
OvirCpuCacheFlush (
  VOID
  )
{
  UINTN Idx;

  for (Idx = 0; Idx < OVIR_CPU_CACHE_MAX_ENTRIES; Idx++) {
    if (mCacheEntries[Idx].NativeCode != NULL) {
      OvFree (mCacheEntries[Idx].NativeCode);
      mCacheEntries[Idx].NativeCode = NULL;
    }
    mCacheEntries[Idx].IsValid = FALSE;
  }

  mCacheStats.ActiveEntries = 0;
  mCacheStats.TotalAllocatedBytes = 0;
}

VOID
EFIAPI
OvirCpuCacheGetStats (
  OUT OVIR_CPU_CACHE_STATS  *Stats
  )
{
  if (Stats != NULL) {
    CopyMem (Stats, &mCacheStats, sizeof (OVIR_CPU_CACHE_STATS));
  }
}
