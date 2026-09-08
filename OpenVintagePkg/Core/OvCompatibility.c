/** @file
  OpenVintage Compatibility Subsystem Implementation.
  Phase 5 Application Requirements, Hardware Constraints, and Execution Profiles.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/OvLoggerLib.h>
#include <Library/OvHardwareLib.h>
#include <Library/OvCompatibilityLib.h>

STATIC OV_COMPAT_RECORD  mDatabase[OV_MAX_COMPAT_RECORDS];
STATIC UINTN             mRecordCount = 0;
STATIC BOOLEAN           mInitialized = FALSE;

EFI_STATUS
EFIAPI
OvCompatibilityInitialize (
  VOID
  )
{
  OV_COMPAT_RECORD  Rec;

  ZeroMem (mDatabase, sizeof (mDatabase));
  mRecordCount = 0;

  //
  // Seed baseline application profiles with realistic requirements
  //

  // Profile 1: Modern CAD 3D Workstation (Requires Compute & OpenGL 4.1+)
  ZeroMem (&Rec, sizeof (OV_COMPAT_RECORD));
  StrCpyS (Rec.AppName, sizeof (Rec.AppName) / sizeof (CHAR16), L"VintageCAD3D");
  StrCpyS (Rec.TargetOs, sizeof (Rec.TargetOs) / sizeof (CHAR16), L"macOS/Win64");
  Rec.Bitness                = 64;
  Rec.RequiredCpuArch        = OvCpuGuestX64;
  Rec.MinCpuCores            = 2;
  Rec.RequiresSSE42           = TRUE;
  Rec.RequiresAVX            = TRUE;
  Rec.RequiredApi            = OvirApiOpenGL;
  Rec.ApiVersionMajor        = 4;
  Rec.ApiVersionMinor        = 1;
  Rec.RequiresComputeShaders = TRUE;
  Rec.MaxTextureSizeRequired = 8192;
  Rec.MinVramBytes           = 512 * 1024 * 1024; // 512MB
  Rec.MinSystemRamBytes      = 4ULL * 1024ULL * 1024ULL * 1024ULL; // 4GB
  StrCpyS (Rec.KnownLimitations, sizeof (Rec.KnownLimitations) / sizeof (CHAR16),
           L"Compute shaders unsupported on Intel Gen7; requires CPU SoftPipe fallback");
  Rec.RecommendedMode        = OvCompatModeTranslatedGpu;
  OvCompatibilityRegisterApp (&Rec);

  // Profile 2: Metal 2 Retro Arcade Engine (Target ARM64 / Metal 2)
  ZeroMem (&Rec, sizeof (OV_COMPAT_RECORD));
  StrCpyS (Rec.AppName, sizeof (Rec.AppName) / sizeof (CHAR16), L"MetalArcadeSim");
  StrCpyS (Rec.TargetOs, sizeof (Rec.TargetOs) / sizeof (CHAR16), L"macOS/ARM64");
  Rec.Bitness                = 64;
  Rec.RequiredCpuArch        = OvCpuGuestArm64;
  Rec.MinCpuCores            = 2;
  Rec.RequiredApi            = OvirApiMetal;
  Rec.ApiVersionMajor        = 2;
  Rec.ApiVersionMinor        = 0;
  Rec.MaxTextureSizeRequired = 4096;
  Rec.MinVramBytes           = 256 * 1024 * 1024; // 256MB
  Rec.MinSystemRamBytes      = 2ULL * 1024ULL * 1024ULL * 1024ULL; // 2GB
  StrCpyS (Rec.KnownLimitations, sizeof (Rec.KnownLimitations) / sizeof (CHAR16),
           L"ARM64 JIT translation active; Metal translated to OpenGL 3.3 Core on Ivy Bridge");
  Rec.RecommendedMode        = OvCompatModeTranslatedBoth;
  OvCompatibilityRegisterApp (&Rec);

  // Profile 3: Classic 32-bit Direct3D 9 Game
  ZeroMem (&Rec, sizeof (OV_COMPAT_RECORD));
  StrCpyS (Rec.AppName, sizeof (Rec.AppName) / sizeof (CHAR16), L"ClassicRacer9");
  StrCpyS (Rec.TargetOs, sizeof (Rec.TargetOs) / sizeof (CHAR16), L"Win32");
  Rec.Bitness                = 32;
  Rec.RequiredCpuArch        = OvCpuGuestX86;
  Rec.MinCpuCores            = 1;
  Rec.RequiredApi            = OvirApiDirectX;
  Rec.ApiVersionMajor        = 9;
  Rec.ApiVersionMinor        = 0;
  Rec.MaxTextureSizeRequired = 2048;
  Rec.MinVramBytes           = 128 * 1024 * 1024; // 128MB
  Rec.MinSystemRamBytes      = 1ULL * 1024ULL * 1024ULL * 1024ULL; // 1GB
  StrCpyS (Rec.KnownLimitations, sizeof (Rec.KnownLimitations) / sizeof (CHAR16),
           L"Fixed-function state translated to OpenGL programmable shaders");
  Rec.RecommendedMode        = OvCompatModeTranslatedGpu;
  OvCompatibilityRegisterApp (&Rec);

  // Profile 4: Native x86_64 Vulkan 1.2 Benchmark
  ZeroMem (&Rec, sizeof (OV_COMPAT_RECORD));
  StrCpyS (Rec.AppName, sizeof (Rec.AppName) / sizeof (CHAR16), L"VkRayCompute");
  StrCpyS (Rec.TargetOs, sizeof (Rec.TargetOs) / sizeof (CHAR16), L"Linux/X64");
  Rec.Bitness                = 64;
  Rec.RequiredCpuArch        = OvCpuGuestX64;
  Rec.MinCpuCores            = 4;
  Rec.RequiresAVX            = TRUE;
  Rec.RequiredApi            = OvirApiVulkan;
  Rec.ApiVersionMajor        = 1;
  Rec.ApiVersionMinor        = 2;
  Rec.RequiresComputeShaders = TRUE;
  Rec.MaxTextureSizeRequired = 16384;
  Rec.MinVramBytes           = 1024ULL * 1024ULL * 1024ULL; // 1GB
  Rec.MinSystemRamBytes      = 8ULL * 1024ULL * 1024ULL * 1024ULL; // 8GB
  StrCpyS (Rec.KnownLimitations, sizeof (Rec.KnownLimitations) / sizeof (CHAR16),
           L"Texture sizes > 4096 clamped on Intel HD 4000; Vulkan ANV native on Haswell");
  Rec.RecommendedMode        = OvCompatModeNative;
  OvCompatibilityRegisterApp (&Rec);

  mInitialized = TRUE;
  OvLogTagged (OV_LOG_LEVEL_DEBUG, L"COMP", L"Compatibility database initialized with %u profiles", mRecordCount);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvCompatibilityRegisterApp (
  IN CONST OV_COMPAT_RECORD  *Record
  )
{
  if (Record == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (mRecordCount >= OV_MAX_COMPAT_RECORDS) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (&mDatabase[mRecordCount], Record, sizeof (OV_COMPAT_RECORD));
  mRecordCount++;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvCompatibilityLookupApp (
  IN  CONST CHAR16      *AppName,
  OUT OV_COMPAT_RECORD  *Record
  )
{
  UINTN  Index;

  if (AppName == NULL || Record == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mInitialized) {
    OvCompatibilityInitialize ();
  }

  for (Index = 0; Index < mRecordCount; Index++) {
    if (StrCmp (mDatabase[Index].AppName, AppName) == 0) {
      CopyMem (Record, &mDatabase[Index], sizeof (OV_COMPAT_RECORD));
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
EFIAPI
OvCompatibilityEvaluateAgainstHardware (
  IN  CONST OV_COMPAT_RECORD    *Record,
  OUT OV_COMPAT_EVALUATION      *Evaluation
  )
{
  OV_CPU_TOPOLOGY  Cpu;
  OV_GPU_TOPOLOGY  Gpu;

  if (Record == NULL || Evaluation == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (Evaluation, sizeof (OV_COMPAT_EVALUATION));
  OvHardwareGetCpu (&Cpu);
  OvHardwareGetGpu (&Gpu);

  Evaluation->CanExecute = TRUE;
  Evaluation->ExpectedOverheadFactor = 100;

  // 1. CPU Evaluation
  if (Record->RequiredCpuArch == OvCpuGuestArm64) {
    Evaluation->CpuTranslationRequired = TRUE;
    Evaluation->ExpectedOverheadFactor += 35;
  } else if (Record->RequiresAVX2 && !Cpu.HasAVX2) {
    Evaluation->CpuTranslationRequired = TRUE;
    Evaluation->ExpectedOverheadFactor += 25;
  }

  // 2. GPU & API Evaluation
  if (Record->RequiredApi == OvirApiMetal) {
    if (Cpu.Architecture == OvCpuArchHaswellBroadwell && Gpu.Vendor == OvGpuVendorIntel) {
      Evaluation->GpuTranslationRequired = FALSE;
    } else {
      Evaluation->GpuTranslationRequired = TRUE;
      Evaluation->ExpectedOverheadFactor += 40;
    }
  } else if (Record->RequiredApi == OvirApiDirectX) {
    Evaluation->GpuTranslationRequired = TRUE;
    Evaluation->ExpectedOverheadFactor += 45;
  }

  // 3. Compute Evaluation
  if (Record->RequiresComputeShaders && !Gpu.SupportsCompute) {
    Evaluation->SoftwareComputeFallback = TRUE;
    Evaluation->ExpectedOverheadFactor += 80;
  }

  // 4. Texture Clamping Evaluation
  if (Record->MaxTextureSizeRequired > Gpu.MaxTextureDimension && Gpu.MaxTextureDimension > 0) {
    Evaluation->TextureClampApplied = TRUE;
    UnicodeSPrint (Evaluation->ClampedFeatures, sizeof (Evaluation->ClampedFeatures),
      L"Textures clamped from %u to %u px", Record->MaxTextureSizeRequired, Gpu.MaxTextureDimension);
  }

  // 5. Execution Mode Selection
  if (Evaluation->SoftwareComputeFallback) {
    Evaluation->SelectedMode = OvCompatModeCpuSoftwareFallback;
    UnicodeSPrint (Evaluation->ExecutionPlan, sizeof (Evaluation->ExecutionPlan),
      L"Compute dispatched to CPU Software SoftPipe");
  } else if (Evaluation->TextureClampApplied) {
    Evaluation->SelectedMode = OvCompatModeSimplifiedTextures;
    UnicodeSPrint (Evaluation->ExecutionPlan, sizeof (Evaluation->ExecutionPlan),
      L"Simplified execution with texture clamping");
  } else if (Evaluation->CpuTranslationRequired && Evaluation->GpuTranslationRequired) {
    Evaluation->SelectedMode = OvCompatModeTranslatedBoth;
    UnicodeSPrint (Evaluation->ExecutionPlan, sizeof (Evaluation->ExecutionPlan),
      L"Dual translation: OVIR-CPU JIT and OVIR-GPU SPIR-V");
  } else if (Evaluation->CpuTranslationRequired) {
    Evaluation->SelectedMode = OvCompatModeTranslatedCpu;
    UnicodeSPrint (Evaluation->ExecutionPlan, sizeof (Evaluation->ExecutionPlan),
      L"OVIR-CPU dynamic JIT translation to native x86-64");
  } else if (Evaluation->GpuTranslationRequired) {
    Evaluation->SelectedMode = OvCompatModeTranslatedGpu;
    UnicodeSPrint (Evaluation->ExecutionPlan, sizeof (Evaluation->ExecutionPlan),
      L"OVIR-GPU translation to host OpenGL Core");
  } else {
    Evaluation->SelectedMode = OvCompatModeNative;
    UnicodeSPrint (Evaluation->ExecutionPlan, sizeof (Evaluation->ExecutionPlan),
      L"Full native silicon execution path");
  }

  OvLogTagged (OV_LOG_LEVEL_INFO, L"COMP", L"Evaluated '%s': Mode %d, Plan: %s",
    Record->AppName, Evaluation->SelectedMode, Evaluation->ExecutionPlan);

  return EFI_SUCCESS;
}

UINTN
EFIAPI
OvCompatibilityGetKnownAppCount (
  VOID
  )
{
  if (!mInitialized) {
    OvCompatibilityInitialize ();
  }

  return mRecordCount;
}
