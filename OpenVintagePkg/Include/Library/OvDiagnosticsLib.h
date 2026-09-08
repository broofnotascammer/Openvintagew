/** @file
  OpenVintage Unified Diagnostics Subsystem Definition.
  Phase 5 Comprehensive System Auditing, Silicon Profiling, and Telemetry Reporting.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OV_DIAGNOSTICS_LIB_H_
#define OV_DIAGNOSTICS_LIB_H_

#include <Uefi.h>
#include <Library/OvHardwareLib.h>
#include <Library/OvUnifiedCacheLib.h>
#include <Library/OvPerfSystemLib.h>

//
// Unified Diagnostic Report Structure
//
typedef struct {
  // 1. Hardware Platform
  CHAR16                  PlatformName[48];
  CHAR16                  FirmwareVendor[32];
  UINT32                  FirmwareRevision;
  UINT32                  EfiBitness;

  // 2. CPU
  CHAR16                  CpuModel[48];
  OV_CPU_ARCHITECTURE     CpuArchitecture;
  UINT32                  PhysicalCores;
  UINT32                  LogicalThreads;
  UINT32                  BaseClockMhz;
  BOOLEAN                 HasSSE41;
  BOOLEAN                 HasSSE42;
  BOOLEAN                 HasAVX;
  BOOLEAN                 HasAVX2;
  BOOLEAN                 HasAESNI;

  // 3. GPU
  CHAR16                  GpuModel[48];
  OV_GPU_VENDOR           GpuVendor;
  UINT32                  PciVendorId;
  UINT32                  PciDeviceId;
  UINT64                  VramBytes;
  UINT32                  MaxTextureDimension;
  BOOLEAN                 SupportsCompute;
  BOOLEAN                 SupportsTessellation;

  // 4. Memory
  UINT64                  TotalSystemRamBytes;
  UINT64                  FreeSystemRamBytes;
  UINT64                  TaggedAllocatedBytes;
  UINT32                  ActiveAllocationsCount;
  BOOLEAN                 MemoryLeaksDetected;

  // 5. Available APIs
  BOOLEAN                 NativeMetalAvailable;
  BOOLEAN                 NativeVulkanAvailable;
  BOOLEAN                 NativeOpenGLCoreAvailable;
  CHAR16                  SupportedApiSummary[64];

  // 6. Translation Support
  BOOLEAN                 CpuArm64ToX64Supported;
  BOOLEAN                 CpuX64ToX64RecompSupported;
  BOOLEAN                 GpuSpirvToGlslSupported;
  BOOLEAN                 GpuMetalToOpenGlSupported;

  // 7. Cache State
  UINT32                  CacheGeneration;
  UINT32                  TotalCacheEntries;
  UINT64                  TotalCacheSizeBytes;
  UINT32                  CacheHitRatePercent;

  // 8. Resolver Decisions & Mode
  BOOLEAN                 StrictResolverEnforced;
  CHAR16                  DefaultExecutionPolicy[48];

  // 9. Performance Information
  UINT64                  TscFrequencyHz;
  UINT32                  EstimatedTranslationOverheadUs;
  UINT32                  ActiveWorkerThreadPool;

  // 10. Known Limitations & Quirks
  CHAR16                  SiliconQuirks[96];
  CHAR16                  ClampingNotices[96];
} OV_DIAGNOSTIC_REPORT;

/**
  Initialize unified diagnostics subsystem.

  @retval EFI_SUCCESS  Diagnostics subsystem initialized.
**/
EFI_STATUS
EFIAPI
OvDiagnosticsInitialize (
  VOID
  );

/**
  Generate comprehensive system diagnostic report.

  @param[out] Report  Pointer to structure receiving diagnostic report.

  @retval EFI_SUCCESS            Report generated successfully.
  @retval EFI_INVALID_PARAMETER  Report is NULL.
**/
EFI_STATUS
EFIAPI
OvDiagnosticsGenerateReport (
  OUT OV_DIAGNOSTIC_REPORT  *Report
  );

/**
  Print formatted diagnostic report to logging subsystem.

  @param[in] Report  Pointer to diagnostic report to display.
**/
VOID
EFIAPI
OvDiagnosticsPrintReport (
  IN CONST OV_DIAGNOSTIC_REPORT  *Report
  );

#endif // OV_DIAGNOSTICS_LIB_H_
