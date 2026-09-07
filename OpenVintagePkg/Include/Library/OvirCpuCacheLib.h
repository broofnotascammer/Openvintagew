/** @file
  OpenVintage CPU Translation Cache & Invalidation Definition.
  Phase 4 Translation Cache Architecture.

  Provides fast translation caching with strict metadata validation
  (Source/Target Arch, Module Identity, Versioning, Configuration Flags).

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OVIR_CPU_CACHE_LIB_H_
#define OVIR_CPU_CACHE_LIB_H_

#include <Uefi.h>
#include <Library/OvirCpuLib.h>

#define OVIR_CPU_CACHE_MAX_ENTRIES          64
#define OPENVINTAGE_TRANSLATOR_VERSION      1
#define OPENVINTAGE_CURRENT_VERSION_PACKED  0x00040000  // v0.4.0

//
// Translation Cache Metadata
//
typedef struct {
  OVIR_CPU_ARCH   SourceArch;
  OVIR_CPU_ARCH   TargetArch;
  UINT64          ModuleId;
  UINT64          GuestPc;
  UINT64          GuestCodeHash;
  UINT32          GuestCodeSize;
  UINT32          OpenVintageVersion;
  UINT32          TranslatorVersion;
  UINT32          ConfigFlags;
} OVIR_TRANSLATION_METADATA;

//
// Cached Code Entry
//
typedef struct {
  OVIR_TRANSLATION_METADATA Metadata;
  UINT8                     *NativeCode;
  UINTN                     NativeCodeSize;
  UINT64                    AccessCount;
  UINT64                    LastAccessTimestamp;
  BOOLEAN                   IsValid;
} OVIR_CPU_CACHE_ENTRY;

//
// Cache Performance Statistics
//
typedef struct {
  UINT64  LookupsCount;
  UINT64  HitsCount;
  UINT64  MissesCount;
  UINT64  InvalidationsCount;
  UINTN   ActiveEntries;
  UINT64  TotalAllocatedBytes;
} OVIR_CPU_CACHE_STATS;

/**
  Initialize CPU translation cache.

  @retval EFI_SUCCESS   Cache initialized.
**/
EFI_STATUS
EFIAPI
OvirCpuCacheInitialize (
  VOID
  );

/**
  Compute 64-bit FNV-1a hash of binary instruction buffer.

  @param[in] Code   Binary buffer.
  @param[in] Size   Number of bytes.

  @return Computed hash value.
**/
UINT64
EFIAPI
OvirCpuCacheComputeCodeHash (
  IN CONST UINT8  *Code,
  IN UINTN        Size
  );

/**
  Lookup a translation in cache matching metadata criteria.

  @param[in]  Query       Metadata search keys.
  @param[out] NativeCode  Pointer to cached machine code buffer.
  @param[out] NativeSize  Size of cached native code.

  @retval EFI_SUCCESS     Matching, compatible cache entry found.
  @retval EFI_NOT_FOUND   No valid matching entry.
**/
EFI_STATUS
EFIAPI
OvirCpuCacheLookup (
  IN  CONST OVIR_TRANSLATION_METADATA *Query,
  OUT CONST UINT8                     **NativeCode,
  OUT UINTN                           *NativeSize
  );

/**
  Store a newly compiled native block in cache.

  @param[in] Metadata    Associated translation metadata.
  @param[in] NativeCode  Pointer to compiled native machine code.
  @param[in] NativeSize  Size in bytes.

  @retval EFI_SUCCESS    Entry cached cleanly.
  @retval EFI_OUT_OF_RESOURCES Cache capacity exceeded.
**/
EFI_STATUS
EFIAPI
OvirCpuCacheStore (
  IN CONST OVIR_TRANSLATION_METADATA *Metadata,
  IN CONST UINT8                     *NativeCode,
  IN UINTN                           NativeSize
  );

/**
  Invalidate all cache entries that do not match expected system versions or config flags.

  @param[in] ExpectedOvVersion          Expected OpenVintage core version.
  @param[in] ExpectedTranslatorVersion  Expected translator version.
  @param[in] ExpectedConfigFlags        Required configuration bitflags.

  @return Number of entries invalidated.
**/
UINTN
EFIAPI
OvirCpuCacheInvalidateIncompatible (
  IN UINT32  ExpectedOvVersion,
  IN UINT32  ExpectedTranslatorVersion,
  IN UINT32  ExpectedConfigFlags
  );

/**
  Flush all entries in translation cache.
**/
VOID
EFIAPI
OvirCpuCacheFlush (
  VOID
  );

/**
  Query translation cache statistics.

  @param[out] Stats   Receives statistics snapshot.
**/
VOID
EFIAPI
OvirCpuCacheGetStats (
  OUT OVIR_CPU_CACHE_STATS  *Stats
  );

#endif // OVIR_CPU_CACHE_LIB_H_
