/** @file
  OpenVintage Performance Measurement Infrastructure Implementation.
  Phase 3 Modular Telemetry, Timing & Resource Profiling.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/OvLoggerLib.h>
#include <Library/OvirPerfLib.h>

STATIC OVIR_PERF_METRICS mMetrics = { 0 };

EFI_STATUS
EFIAPI
OvirPerfInitialize (
  VOID
  )
{
  ZeroMem (&mMetrics, sizeof (OVIR_PERF_METRICS));
  mMetrics.MinFrameTicks = 0xFFFFFFFFFFFFFFFFULL;
  OvLogTagged (OV_LOG_LEVEL_DEBUG, L"PERF", L"Performance telemetry engine initialized (TSC-based)");
  return EFI_SUCCESS;
}

UINT64
EFIAPI
OvirPerfGetTimestamp (
  VOID
  )
{
  return AsmReadTsc ();
}

VOID
EFIAPI
OvirPerfRecordTranslation (
  IN UINT64  ElapsedTicks
  )
{
  mMetrics.TotalTranslationTicks += ElapsedTicks;
  mMetrics.TranslationCount++;
}

VOID
EFIAPI
OvirPerfRecordShaderCompile (
  IN UINT64  ElapsedTicks
  )
{
  mMetrics.TotalShaderCompileTicks += ElapsedTicks;
  mMetrics.ShaderCompileCount++;
}

VOID
EFIAPI
OvirPerfRecordFrameTiming (
  IN UINT64  FrameTicks
  )
{
  mMetrics.TotalFrameRenderTicks += FrameTicks;
  mMetrics.FrameCount++;
  mMetrics.LastFrameTicks = FrameTicks;

  if (FrameTicks < mMetrics.MinFrameTicks) {
    mMetrics.MinFrameTicks = FrameTicks;
  }
  if (FrameTicks > mMetrics.MaxFrameTicks) {
    mMetrics.MaxFrameTicks = FrameTicks;
  }
}

VOID
EFIAPI
OvirPerfRecordCacheEvent (
  IN BOOLEAN  IsShaderCache,
  IN BOOLEAN  IsHit
  )
{
  if (IsShaderCache) {
    if (IsHit) {
      mMetrics.ShaderCacheHits++;
    } else {
      mMetrics.ShaderCacheMisses++;
    }
  } else {
    if (IsHit) {
      mMetrics.PipelineCacheHits++;
    } else {
      mMetrics.PipelineCacheMisses++;
    }
  }
}

VOID
EFIAPI
OvirPerfUpdateResourceUsage (
  IN UINT64  CurrentBytes,
  IN UINT64  PeakBytes
  )
{
  mMetrics.CurrentGpuMemoryBytes = CurrentBytes;
  mMetrics.PeakGpuMemoryBytes = PeakBytes;
}

VOID
EFIAPI
OvirPerfGetMetrics (
  OUT OVIR_PERF_METRICS  *OutMetrics
  )
{
  if (OutMetrics != NULL) {
    CopyMem (OutMetrics, &mMetrics, sizeof (OVIR_PERF_METRICS));
  }
}

VOID
EFIAPI
OvirPerfReset (
  VOID
  )
{
  ZeroMem (&mMetrics, sizeof (OVIR_PERF_METRICS));
  mMetrics.MinFrameTicks = 0xFFFFFFFFFFFFFFFFULL;
}
