/** @file
  OpenVintage Performance Subsystem Definition.
  Phase 5 Measurable Performance and Telemetry Management.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OV_PERF_SYSTEM_LIB_H_
#define OV_PERF_SYSTEM_LIB_H_

#include <Uefi.h>

//
// Measurable Performance Metrics Snapshot
//
typedef struct {
  // CPU Utilization
  UINT32  CpuActiveCores;
  UINT32  CpuTotalThreads;
  UINT32  CpuLoadPercent;             // 0-100%
  UINT64  CpuTscCyclesPerSecond;

  // Memory Utilization
  UINT64  MemoryAllocatedBytes;
  UINT64  MemoryPeakBytes;
  UINT64  MemoryFreeBytes;
  UINT64  MemoryTotalBytes;

  // GPU Utilization
  UINT64  GpuVramAllocatedBytes;
  UINT64  GpuVramTotalBytes;
  UINT32  GpuDrawCallsLastFrame;
  UINT32  GpuPushConstantBytes;

  // Translation Overhead (Microseconds & Cycles)
  UINT64  TranslationTotalCalls;
  UINT64  TranslationTotalCycles;
  UINT32  TranslationAverageTimeUs;
  UINT32  TranslationMaxTimeUs;

  // Shader Compilation
  UINT32  ShadersCompiledCount;
  UINT64  ShaderCompilationTotalCycles;
  UINT32  ShaderAverageCompileUs;

  // Cache Hit / Miss Ratios
  UINT64  CacheTotalQueries;
  UINT64  CacheHits;
  UINT64  CacheMisses;
  UINT32  CacheHitRatePercent;        // 0-100%

  // Frame Timing
  UINT32  FrameCountRecorded;
  UINT32  FrameTimeCurrentUs;
  UINT32  FrameTimeMinUs;
  UINT32  FrameTimeMaxUs;
  UINT32  FrameTimeAverageUs;
  UINT32  TargetFramerateFps;

  // Resource Scheduler
  UINT32  SchedulerActiveTasks;
  UINT32  SchedulerCompletedTasks;
  UINT32  SchedulerWorkerPoolUtilizationPercent;
} OV_PERF_SNAPSHOT;

/**
  Initialize Performance Management Subsystem.

  @retval EFI_SUCCESS  Performance subsystem initialized.
**/
EFI_STATUS
EFIAPI
OvPerfSystemInitialize (
  VOID
  );

/**
  Record CPU load metrics.

  @param[in] LoadPercent  Estimated CPU utilization percent (0-100).
  @param[in] ActiveCores  Number of active processor cores executing workloads.
**/
VOID
EFIAPI
OvPerfSystemRecordCpuMetric (
  IN UINT32  LoadPercent,
  IN UINT32  ActiveCores
  );

/**
  Record memory allocation tracking metrics.

  @param[in] AllocatedBytes  Currently active allocated bytes.
  @param[in] PeakBytes       Peak memory watermark.
**/
VOID
EFIAPI
OvPerfSystemRecordMemoryMetric (
  IN UINT64  AllocatedBytes,
  IN UINT64  PeakBytes
  );

/**
  Record GPU and rendering metrics.

  @param[in] VramUsedBytes      VRAM currently allocated.
  @param[in] DrawCallsThisFrame Draw call count in the current frame.
  @param[in] PushConstantBytes  Total push constant bytes transferred.
**/
VOID
EFIAPI
OvPerfSystemRecordGpuMetric (
  IN UINT64  VramUsedBytes,
  IN UINT32  DrawCallsThisFrame,
  IN UINT32  PushConstantBytes
  );

/**
  Record CPU translation overhead.

  @param[in] Cycles      TSC cycles spent in translation.
  @param[in] DurationUs  Elapsed microseconds.
**/
VOID
EFIAPI
OvPerfSystemRecordTranslationMetric (
  IN UINT64  Cycles,
  IN UINT32  DurationUs
  );

/**
  Record shader compilation metrics.

  @param[in] Cycles      TSC cycles spent compiling/ingesting shaders.
  @param[in] DurationUs  Elapsed microseconds.
**/
VOID
EFIAPI
OvPerfSystemRecordShaderMetric (
  IN UINT64  Cycles,
  IN UINT32  DurationUs
  );

/**
  Record cache queries and hits/misses.

  @param[in] Hits    Number of cache hits.
  @param[in] Misses  Number of cache misses.
**/
VOID
EFIAPI
OvPerfSystemRecordCacheMetric (
  IN UINT64  Hits,
  IN UINT64  Misses
  );

/**
  Record frame timing duration.

  @param[in] FrameDurationUs  Time taken to render frame in microseconds.
**/
VOID
EFIAPI
OvPerfSystemRecordFrameTiming (
  IN UINT32  FrameDurationUs
  );

/**
  Retrieve current comprehensive performance metrics snapshot.

  @param[out] Snapshot  Structure populated with telemetry data.

  @retval EFI_SUCCESS            Snapshot retrieved.
  @retval EFI_INVALID_PARAMETER  Snapshot is NULL.
**/
EFI_STATUS
EFIAPI
OvPerfSystemGetSnapshot (
  OUT OV_PERF_SNAPSHOT  *Snapshot
  );

/**
  Reset telemetry metrics to zero.
**/
VOID
EFIAPI
OvPerfSystemResetMetrics (
  VOID
  );

#endif // OV_PERF_SYSTEM_LIB_H_
