/** @file
  OpenVintage Unified Cache Subsystem Definition.
  Phase 5 Multi-Tier Cache Integration and Reliable Invalidation.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OV_UNIFIED_CACHE_LIB_H_
#define OV_UNIFIED_CACHE_LIB_H_

#include <Uefi.h>

//
// Cache Tiers
//
typedef enum {
  OvCacheTierCpuTranslation = 1,  // Binary translation blocks (OvirCpuCache)
  OvCacheTierShader = 2,          // SPIR-V -> Bytecode shaders (OvirShader)
  OvCacheTierPipeline = 3,        // Pipeline State Objects (OvirPipeline)
  OvCacheTierCompatibility = 4,   // App capability & quirk resolution cache
  OvCacheTierAll = 0xFF           // All cache subsystems
} OV_CACHE_TIER;

//
// Invalidation Triggers / Reasons
//
typedef enum {
  OvInvalidateReasonManual = 1,       // User or application requested flush
  OvInvalidateReasonVersionChange,    // Firmware or translator version increment
  OvInvalidateReasonHardwareChange,   // Hardware device or CPUID configuration changed
  OvInvalidateReasonMemoryPressure,   // VRAM or RAM low, purging LRU entries
  OvInvalidateReasonIntegrityFailure  // Checksum or hash mismatch detected
} OV_INVALIDATE_REASON;

//
// Cache Subsystem Statistics
//
typedef struct {
  UINT32  CpuCacheEntries;
  UINT64  CpuCacheBytes;
  UINT64  CpuCacheHits;
  UINT64  CpuCacheMisses;

  UINT32  ShaderCacheEntries;
  UINT64  ShaderCacheBytes;
  UINT64  ShaderCacheHits;
  UINT64  ShaderCacheMisses;

  UINT32  PipelineCacheEntries;
  UINT64  PipelineCacheBytes;
  UINT64  PipelineCacheHits;
  UINT64  PipelineCacheMisses;

  UINT32  CompatCacheEntries;
  UINT64  CompatCacheHits;
  UINT64  CompatCacheMisses;

  UINT64  TotalQueries;
  UINT64  TotalHits;
  UINT64  TotalMisses;
  UINT32  OverallHitRatePercent;
  UINT32  CurrentGeneration;          // Monotonically increasing cache generation
} OV_UNIFIED_CACHE_STATS;

/**
  Initialize unified cache manager.

  @retval EFI_SUCCESS  Unified cache initialized.
**/
EFI_STATUS
EFIAPI
OvUnifiedCacheInitialize (
  VOID
  );

/**
  Retrieve overall cache statistics and hit/miss telemetry.

  @param[out] Stats  Pointer to structure receiving stats.

  @retval EFI_SUCCESS            Stats populated.
  @retval EFI_INVALID_PARAMETER  Stats is NULL.
**/
EFI_STATUS
EFIAPI
OvUnifiedCacheGetStats (
  OUT OV_UNIFIED_CACHE_STATS  *Stats
  );

/**
  Reliably invalidate cache entries based on tier and reason.

  @param[in] Tier    Cache tier to invalidate (or OvCacheTierAll).
  @param[in] Reason  Reason for invalidation.

  @retval EFI_SUCCESS  Requested tier invalidated.
**/
EFI_STATUS
EFIAPI
OvUnifiedCacheInvalidate (
  IN OV_CACHE_TIER         Tier,
  IN OV_INVALIDATE_REASON  Reason
  );

/**
  Verify integrity across all cache tiers (signatures, hashes, bounds).

  @retval EFI_SUCCESS  All cache tiers intact.
  @retval EFI_CRC_ERROR Checksum or corruption detected.
**/
EFI_STATUS
EFIAPI
OvUnifiedCacheVerifyIntegrity (
  VOID
  );

/**
  Get the active cache generation sequence number.

  @return UINT32  Active generation number.
**/
UINT32
EFIAPI
OvUnifiedCacheGetGeneration (
  VOID
  );

#endif // OV_UNIFIED_CACHE_LIB_H_
