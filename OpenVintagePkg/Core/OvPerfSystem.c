/** @file
  OpenVintage Performance Subsystem Implementation.
  Phase 5 Measurable Performance and Telemetry Management.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/OvLoggerLib.h>
#include <Library/OvHardwareLib.h>
#include <Library/OvMemoryLib.h>
#include <Library/OvPerfSystemLib.h>

STATIC OV_PERF_SNAPSHOT  mSnapshot;
STATIC BOOLEAN           mInitialized = FALSE;

EFI_STATUS
EFIAPI
OvPerfSystemInitialize (
  VOID
  )
{
  OV_CPU_TOPOLOGY  Cpu;
  OV_GPU_TOPOLOGY  Gpu;

  ZeroMem (&mSnapshot, sizeof (OV_PERF_SNAPSHOT));
  OvHardwareGetCpu (&Cpu);
  OvHardwareGetGpu (&Gpu);

  mSnapshot.CpuActiveCores         = Cpu.PhysicalCores > 0 ? Cpu.PhysicalCores : 2;
  mSnapshot.CpuTotalThreads        = Cpu.LogicalThreads > 0 ? Cpu.LogicalThreads : 4;
  mSnapshot.CpuLoadPercent         = 12; // Baseline idle firmware load
  mSnapshot.CpuTscCyclesPerSecond  = 2400000000ULL; // 2.40 GHz baseline estimate

  mSnapshot.MemoryTotalBytes       = 8ULL * 1024ULL * 1024ULL * 1024ULL; // 8GB nominal baseline
  mSnapshot.MemoryFreeBytes        = mSnapshot.MemoryTotalBytes;

  mSnapshot.GpuVramTotalBytes      = Gpu.VramSize > 0 ? Gpu.VramSize : (512 * 1024 * 1024);
  mSnapshot.TargetFramerateFps     = 60;
  mSnapshot.FrameTimeMinUs         = 16666;
  mSnapshot.FrameTimeMaxUs         = 16666;
  mSnapshot.FrameTimeAverageUs     = 16666;
  mSnapshot.FrameTimeCurrentUs     = 16666;

  mInitialized = TRUE;
  OvLogTagged (OV_LOG_LEVEL_DEBUG, L"PERF", L"Performance monitoring subsystem initialized");
  return EFI_SUCCESS;
}

VOID
EFIAPI
OvPerfSystemRecordCpuMetric (
  IN UINT32  LoadPercent,
  IN UINT32  ActiveCores
  )
{
  if (!mInitialized) {
    OvPerfSystemInitialize ();
  }

  mSnapshot.CpuLoadPercent = LoadPercent > 100 ? 100 : LoadPercent;
  if (ActiveCores > 0) {
    mSnapshot.CpuActiveCores = ActiveCores;
  }
}

VOID
EFIAPI
OvPerfSystemRecordMemoryMetric (
  IN UINT64  AllocatedBytes,
  IN UINT64  PeakBytes
  )
{
  if (!mInitialized) {
    OvPerfSystemInitialize ();
  }

  mSnapshot.MemoryAllocatedBytes = AllocatedBytes;
  if (PeakBytes > mSnapshot.MemoryPeakBytes) {
    mSnapshot.MemoryPeakBytes = PeakBytes;
  }
  if (mSnapshot.MemoryAllocatedBytes > mSnapshot.MemoryPeakBytes) {
    mSnapshot.MemoryPeakBytes = mSnapshot.MemoryAllocatedBytes;
  }

  if (mSnapshot.MemoryTotalBytes > mSnapshot.MemoryAllocatedBytes) {
    mSnapshot.MemoryFreeBytes = mSnapshot.MemoryTotalBytes - mSnapshot.MemoryAllocatedBytes;
  } else {
    mSnapshot.MemoryFreeBytes = 0;
  }
}

VOID
EFIAPI
OvPerfSystemRecordGpuMetric (
  IN UINT64  VramUsedBytes,
  IN UINT32  DrawCallsThisFrame,
  IN UINT32  PushConstantBytes
  )
{
  if (!mInitialized) {
    OvPerfSystemInitialize ();
  }

  mSnapshot.GpuVramAllocatedBytes = VramUsedBytes;
  mSnapshot.GpuDrawCallsLastFrame = DrawCallsThisFrame;
  mSnapshot.GpuPushConstantBytes  = PushConstantBytes;
}

VOID
EFIAPI
OvPerfSystemRecordTranslationMetric (
  IN UINT64  Cycles,
  IN UINT32  DurationUs
  )
{
  if (!mInitialized) {
    OvPerfSystemInitialize ();
  }

  mSnapshot.TranslationTotalCalls++;
  mSnapshot.TranslationTotalCycles += Cycles;
  if (DurationUs > mSnapshot.TranslationMaxTimeUs) {
    mSnapshot.TranslationMaxTimeUs = DurationUs;
  }
  if (mSnapshot.TranslationTotalCalls > 0) {
    mSnapshot.TranslationAverageTimeUs = (UINT32)(mSnapshot.TranslationTotalCycles / (mSnapshot.TranslationTotalCalls * 2400));
    if (mSnapshot.TranslationAverageTimeUs == 0) {
      mSnapshot.TranslationAverageTimeUs = DurationUs;
    }
  }
}

VOID
EFIAPI
OvPerfSystemRecordShaderMetric (
  IN UINT64  Cycles,
  IN UINT32  DurationUs
  )
{
  if (!mInitialized) {
    OvPerfSystemInitialize ();
  }

  mSnapshot.ShadersCompiledCount++;
  mSnapshot.ShaderCompilationTotalCycles += Cycles;
  if (mSnapshot.ShadersCompiledCount > 0) {
    mSnapshot.ShaderAverageCompileUs = (UINT32)(mSnapshot.ShaderCompilationTotalCycles / (mSnapshot.ShadersCompiledCount * 2400));
    if (mSnapshot.ShaderAverageCompileUs == 0) {
      mSnapshot.ShaderAverageCompileUs = DurationUs;
    }
  }
}

VOID
EFIAPI
OvPerfSystemRecordCacheMetric (
  IN UINT64  Hits,
  IN UINT64  Misses
  )
{
  if (!mInitialized) {
    OvPerfSystemInitialize ();
  }

  mSnapshot.CacheHits         += Hits;
  mSnapshot.CacheMisses       += Misses;
  mSnapshot.CacheTotalQueries  = mSnapshot.CacheHits + mSnapshot.CacheMisses;

  if (mSnapshot.CacheTotalQueries > 0) {
    mSnapshot.CacheHitRatePercent = (UINT32)((mSnapshot.CacheHits * 100) / mSnapshot.CacheTotalQueries);
  }
}

VOID
EFIAPI
OvPerfSystemRecordFrameTiming (
  IN UINT32  FrameDurationUs
  )
{
  if (!mInitialized) {
    OvPerfSystemInitialize ();
  }

  mSnapshot.FrameCountRecorded++;
  mSnapshot.FrameTimeCurrentUs = FrameDurationUs;

  if (mSnapshot.FrameCountRecorded == 1) {
    mSnapshot.FrameTimeMinUs     = FrameDurationUs;
    mSnapshot.FrameTimeMaxUs     = FrameDurationUs;
    mSnapshot.FrameTimeAverageUs = FrameDurationUs;
  } else {
    if (FrameDurationUs < mSnapshot.FrameTimeMinUs) {
      mSnapshot.FrameTimeMinUs = FrameDurationUs;
    }
    if (FrameDurationUs > mSnapshot.FrameTimeMaxUs) {
      mSnapshot.FrameTimeMaxUs = FrameDurationUs;
    }
    mSnapshot.FrameTimeAverageUs = (mSnapshot.FrameTimeAverageUs * 3 + FrameDurationUs) / 4;
  }
}

EFI_STATUS
EFIAPI
OvPerfSystemGetSnapshot (
  OUT OV_PERF_SNAPSHOT  *Snapshot
  )
{
  if (Snapshot == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mInitialized) {
    OvPerfSystemInitialize ();
  }

  CopyMem (Snapshot, &mSnapshot, sizeof (OV_PERF_SNAPSHOT));
  return EFI_SUCCESS;
}

VOID
EFIAPI
OvPerfSystemResetMetrics (
  VOID
  )
{
  OvPerfSystemInitialize ();
}
