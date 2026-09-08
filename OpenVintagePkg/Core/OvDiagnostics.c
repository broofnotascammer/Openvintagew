/** @file
  OpenVintage Unified Diagnostics Subsystem Implementation.
  Phase 5 Comprehensive System Auditing, Silicon Profiling, and Telemetry Reporting.

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
#include <Library/OvConfigLib.h>
#include <Library/OvUnifiedCacheLib.h>
#include <Library/OvPerfSystemLib.h>
#include <Library/OvDiagnosticsLib.h>

STATIC BOOLEAN  mInitialized = FALSE;

EFI_STATUS
EFIAPI
OvDiagnosticsInitialize (
  VOID
  )
{
  mInitialized = TRUE;
  OvLogTagged (OV_LOG_LEVEL_DEBUG, L"DIAG", L"Unified diagnostics subsystem initialized");
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvDiagnosticsGenerateReport (
  OUT OV_DIAGNOSTIC_REPORT  *Report
  )
{
  OV_CPU_TOPOLOGY         Cpu;
  OV_GPU_TOPOLOGY         Gpu;
  OV_UNIFIED_CACHE_STATS  CacheStats;
  OV_PERF_SNAPSHOT        Perf;

  if (Report == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mInitialized) {
    OvDiagnosticsInitialize ();
  }

  ZeroMem (Report, sizeof (OV_DIAGNOSTIC_REPORT));
  OvHardwareGetCpu (&Cpu);
  OvHardwareGetGpu (&Gpu);
  OvUnifiedCacheGetStats (&CacheStats);
  OvPerfSystemGetSnapshot (&Perf);

  // 1. Hardware Platform
  StrCpyS (Report->PlatformName, sizeof (Report->PlatformName) / sizeof (CHAR16), L"Vintage Mac / PC (x86_64 UEFI)");
  StrCpyS (Report->FirmwareVendor, sizeof (Report->FirmwareVendor) / sizeof (CHAR16), L"OpenVintage EDK II Core");
  Report->FirmwareRevision = 0x00050000; // v0.5.0
  Report->EfiBitness        = 64;

  // 2. CPU
  StrCpyS (Report->CpuModel, sizeof (Report->CpuModel) / sizeof (CHAR16), Cpu.BrandString);
  Report->CpuArchitecture = Cpu.Architecture;
  Report->PhysicalCores   = Cpu.PhysicalCores;
  Report->LogicalThreads  = Cpu.LogicalThreads;
  Report->BaseClockMhz    = 2400; // Nominal baseline
  Report->HasSSE41        = Cpu.HasSSE41;
  Report->HasSSE42        = Cpu.HasSSE42;
  Report->HasAVX          = Cpu.HasAVX;
  Report->HasAVX2         = Cpu.HasAVX2;
  Report->HasAESNI        = Cpu.HasAESNI;

  // 3. GPU
  StrCpyS (Report->GpuModel, sizeof (Report->GpuModel) / sizeof (CHAR16), Gpu.ModelName);
  Report->GpuVendor           = Gpu.Vendor;
  Report->PciVendorId         = Gpu.PciVendorId;
  Report->PciDeviceId         = Gpu.PciDeviceId;
  Report->VramBytes           = Gpu.VramSize;
  Report->MaxTextureDimension = Gpu.MaxTextureDimension;
  Report->SupportsCompute     = Gpu.SupportsCompute;
  Report->SupportsTessellation= Gpu.SupportsTessellation;

  // 4. Memory
  Report->TotalSystemRamBytes    = 8ULL * 1024ULL * 1024ULL * 1024ULL; // 8GB
  Report->TaggedAllocatedBytes   = OvMemoryGetAllocatedBytes ();
  Report->FreeSystemRamBytes     = Report->TotalSystemRamBytes - Report->TaggedAllocatedBytes;
  Report->ActiveAllocationsCount = (UINT32)OvMemoryGetActiveAllocationCount ();
  Report->MemoryLeaksDetected    = (Report->ActiveAllocationsCount > 500);

  // 5. Available APIs
  Report->NativeOpenGLCoreAvailable = TRUE;
  if (Cpu.Architecture == OvCpuArchHaswellBroadwell && Gpu.Vendor == OvGpuVendorIntel) {
    Report->NativeMetalAvailable  = TRUE;
    Report->NativeVulkanAvailable = TRUE;
    StrCpyS (Report->SupportedApiSummary, sizeof (Report->SupportedApiSummary) / sizeof (CHAR16),
      L"Native: Metal 2, Vulkan 1.2, OpenGL 4.1 Core");
  } else {
    Report->NativeMetalAvailable  = FALSE;
    Report->NativeVulkanAvailable = FALSE;
    StrCpyS (Report->SupportedApiSummary, sizeof (Report->SupportedApiSummary) / sizeof (CHAR16),
      L"Native: OpenGL 3.3; Translated: Metal 2, Vulkan 1.0");
  }

  // 6. Translation Support
  Report->CpuArm64ToX64Supported   = TRUE;
  Report->CpuX64ToX64RecompSupported = TRUE;
  Report->GpuSpirvToGlslSupported  = TRUE;
  Report->GpuMetalToOpenGlSupported= TRUE;

  // 7. Cache State
  Report->CacheGeneration     = CacheStats.CurrentGeneration;
  Report->TotalCacheEntries   = CacheStats.CpuCacheEntries + CacheStats.ShaderCacheEntries + CacheStats.PipelineCacheEntries;
  Report->TotalCacheSizeBytes = CacheStats.CpuCacheBytes + CacheStats.ShaderCacheBytes + CacheStats.PipelineCacheBytes;
  Report->CacheHitRatePercent = CacheStats.OverallHitRatePercent;

  // 8. Resolver Decisions & Policies
  Report->StrictResolverEnforced = OvConfigGetFeatureFlag (OV_FEATURE_RESOLVER_STRICT);
  StrCpyS (Report->DefaultExecutionPolicy, sizeof (Report->DefaultExecutionPolicy) / sizeof (CHAR16),
    L"Adaptive JIT + Clamped Fallback");

  // 9. Performance Information
  Report->TscFrequencyHz                = 2400000000ULL;
  Report->EstimatedTranslationOverheadUs = 120; // Nominal 120 us average
  Report->ActiveWorkerThreadPool         = Cpu.LogicalThreads > 2 ? 2 : 1;

  // 10. Known Limitations & Silicon Quirks
  if (Cpu.Architecture == OvCpuArchIvyBridge || Cpu.Architecture == OvCpuArchSandyBridge) {
    StrCpyS (Report->SiliconQuirks, sizeof (Report->SiliconQuirks) / sizeof (CHAR16),
      L"Intel Gen7 lacks Vulkan 1.2 & FP64; AVX2 emulated via 128-bit SSE4.2 split");
    StrCpyS (Report->ClampingNotices, sizeof (Report->ClampingNotices) / sizeof (CHAR16),
      L"Max texture size clamped to 4096px; compute shaders routed to CPU SoftPipe");
  } else {
    StrCpyS (Report->SiliconQuirks, sizeof (Report->SiliconQuirks) / sizeof (CHAR16),
      L"Haswell+ silicon: AVX2 native, ANV Vulkan driver enabled");
    StrCpyS (Report->ClampingNotices, sizeof (Report->ClampingNotices) / sizeof (CHAR16),
      L"Textures up to 16384px supported natively");
  }

  return EFI_SUCCESS;
}

VOID
EFIAPI
OvDiagnosticsPrintReport (
  IN CONST OV_DIAGNOSTIC_REPORT  *Report
  )
{
  if (Report == NULL) {
    return;
  }

  OvLogTagged (OV_LOG_LEVEL_INFO, L"DIAG", L"================================================================");
  OvLogTagged (OV_LOG_LEVEL_INFO, L"DIAG", L"          OPENVINTAGE UNIFIED SYSTEM DIAGNOSTIC REPORT         ");
  OvLogTagged (OV_LOG_LEVEL_INFO, L"DIAG", L"================================================================");
  OvLogTagged (OV_LOG_LEVEL_INFO, L"DIAG", L"Platform    : %s (FW: %08x)", Report->PlatformName, Report->FirmwareRevision);
  OvLogTagged (OV_LOG_LEVEL_INFO, L"DIAG", L"CPU Model   : %s (%u cores, %u threads)", Report->CpuModel, Report->PhysicalCores, Report->LogicalThreads);
  OvLogTagged (OV_LOG_LEVEL_INFO, L"DIAG", L"CPU Caps    : SSE4.1=%d, SSE4.2=%d, AVX=%d, AVX2=%d, AES=%d",
    Report->HasSSE41, Report->HasSSE42, Report->HasAVX, Report->HasAVX2, Report->HasAESNI);
  OvLogTagged (OV_LOG_LEVEL_INFO, L"DIAG", L"GPU Device  : %s (VRAM: %lu MB, MaxTex: %u)",
    Report->GpuModel, (UINT64)(Report->VramBytes / (1024 * 1024)), Report->MaxTextureDimension);
  OvLogTagged (OV_LOG_LEVEL_INFO, L"DIAG", L"Graphics API: %s", Report->SupportedApiSummary);
  OvLogTagged (OV_LOG_LEVEL_INFO, L"DIAG", L"Translation : ARM64->X64=%d, SPIR-V->GLSL=%d, Metal->GL=%d",
    Report->CpuArm64ToX64Supported, Report->GpuSpirvToGlslSupported, Report->GpuMetalToOpenGlSupported);
  OvLogTagged (OV_LOG_LEVEL_INFO, L"DIAG", L"Memory State: %lu MB allocated (%u tags), Free: %lu MB",
    (UINT64)(Report->TaggedAllocatedBytes / (1024 * 1024)), Report->ActiveAllocationsCount,
    (UINT64)(Report->FreeSystemRamBytes / (1024 * 1024)));
  OvLogTagged (OV_LOG_LEVEL_INFO, L"DIAG", L"Cache State : Gen %u, %u entries (%lu KB), Hit Rate: %u%%",
    Report->CacheGeneration, Report->TotalCacheEntries, (UINT64)(Report->TotalCacheSizeBytes / 1024), Report->CacheHitRatePercent);
  OvLogTagged (OV_LOG_LEVEL_INFO, L"DIAG", L"Limitations : %s", Report->SiliconQuirks);
  OvLogTagged (OV_LOG_LEVEL_INFO, L"DIAG", L"Clamping    : %s", Report->ClampingNotices);
  OvLogTagged (OV_LOG_LEVEL_INFO, L"DIAG", L"================================================================");
}
