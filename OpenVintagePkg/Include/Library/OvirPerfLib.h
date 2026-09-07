/** @file
  OpenVintage Performance Measurement Infrastructure Definition.
  Phase 3 Modular Telemetry, Timing & Resource Profiling.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OVIR_PERF_LIB_H_
#define OVIR_PERF_LIB_H_

#include <Uefi.h>

//
// Performance Metrics Telemetry
//
typedef struct {
  UINT64 TotalTranslationTicks;
  UINT64 TotalShaderCompileTicks;
  UINT64 TotalFrameRenderTicks;
  UINT32 TranslationCount;
  UINT32 ShaderCompileCount;
  UINT32 FrameCount;
  UINT32 ShaderCacheHits;
  UINT32 ShaderCacheMisses;
  UINT32 PipelineCacheHits;
  UINT32 PipelineCacheMisses;
  UINT64 CurrentGpuMemoryBytes;
  UINT64 PeakGpuMemoryBytes;
  UINT64 LastFrameTicks;
  UINT64 MinFrameTicks;
  UINT64 MaxFrameTicks;
} OVIR_PERF_METRICS;

/**
  Initialize Performance subsystem.

  @retval EFI_SUCCESS   Ready.
**/
EFI_STATUS
EFIAPI
OvirPerfInitialize (
  VOID
  );

/**
  Capture high-resolution timestamp (using hardware TSC on x86_64).

  @return Timestamp in CPU cycles/ticks.
**/
UINT64
EFIAPI
OvirPerfGetTimestamp (
  VOID
  );

/**
  Record an API-to-OVIR command translation measurement.

  @param[in] ElapsedTicks   CPU cycles spent translating.
**/
VOID
EFIAPI
OvirPerfRecordTranslation (
  IN UINT64  ElapsedTicks
  );

/**
  Record a shader compilation/ingestion measurement.

  @param[in] ElapsedTicks   CPU cycles spent compiling.
**/
VOID
EFIAPI
OvirPerfRecordShaderCompile (
  IN UINT64  ElapsedTicks
  );

/**
  Record frame timing for active render backend.

  @param[in] FrameTicks     CPU cycles for completed frame.
**/
VOID
EFIAPI
OvirPerfRecordFrameTiming (
  IN UINT64  FrameTicks
  );

/**
  Record cache hit or miss event.

  @param[in] IsShaderCache  TRUE for shader, FALSE for pipeline.
  @param[in] IsHit          TRUE for hit, FALSE for miss.
**/
VOID
EFIAPI
OvirPerfRecordCacheEvent (
  IN BOOLEAN  IsShaderCache,
  IN BOOLEAN  IsHit
  );

/**
  Update resource usage tracking.

  @param[in] CurrentBytes   Active GPU memory bytes.
  @param[in] PeakBytes      Peak GPU memory bytes.
**/
VOID
EFIAPI
OvirPerfUpdateResourceUsage (
  IN UINT64  CurrentBytes,
  IN UINT64  PeakBytes
  );

/**
  Query accumulated performance metrics.

  @param[out] OutMetrics    Destination structure.
**/
VOID
EFIAPI
OvirPerfGetMetrics (
  OUT OVIR_PERF_METRICS  *OutMetrics
  );

/**
  Reset all performance counters.
**/
VOID
EFIAPI
OvirPerfReset (
  VOID
  );

#endif // OVIR_PERF_LIB_H_
