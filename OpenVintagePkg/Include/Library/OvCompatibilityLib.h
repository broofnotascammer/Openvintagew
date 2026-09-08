/** @file
  OpenVintage Compatibility Subsystem Definition.
  Phase 5 Application Requirements, Hardware Constraints, and Execution Profiles.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OV_COMPATIBILITY_LIB_H_
#define OV_COMPATIBILITY_LIB_H_

#include <Uefi.h>
#include <Library/OvirGpuLib.h>
#include <Library/OvirCpuLib.h>
#include <Library/OvResolverLib.h>

#define OV_MAX_COMPAT_RECORDS 32

//
// Recommended Execution Modes
//
typedef enum {
  OvCompatModeNative = 1,                 // Pure native execution on target silicon
  OvCompatModeTranslatedGpu = 2,          // Translated GPU stream (e.g. Metal -> OpenGL Core)
  OvCompatModeTranslatedCpu = 3,          // Translated CPU instructions (e.g. ARM64 -> x86-64)
  OvCompatModeTranslatedBoth = 4,         // Both CPU and GPU translation active
  OvCompatModeSimplifiedTextures = 5,     // Texture resolution clamped (e.g. 4096px limit)
  OvCompatModeCpuSoftwareFallback = 6,    // CPU Software compute or softpipe rasterizer
  OvCompatModeUnsupportedHardware = 7     // Target hardware cannot satisfy requirements
} OV_COMPAT_EXECUTION_MODE;

//
// Application Compatibility Specification Record
//
typedef struct {
  CHAR16                    AppName[32];
  CHAR16                    TargetOs[24];
  UINT32                    Bitness;                    // 32 or 64
  
  // CPU Requirements
  OV_CPU_GUEST_ARCH         RequiredCpuArch;
  UINT32                    MinCpuCores;
  BOOLEAN                   RequiresSSE42;
  BOOLEAN                   RequiresAVX;
  BOOLEAN                   RequiresAVX2;

  // GPU & API Requirements
  OVIR_API_TYPE             RequiredApi;
  UINT32                    ApiVersionMajor;
  UINT32                    ApiVersionMinor;
  BOOLEAN                   RequiresComputeShaders;
  BOOLEAN                   RequiresTessellation;
  UINT32                    MaxTextureSizeRequired;     // e.g. 4096, 8192, 16384
  UINT64                    MinVramBytes;
  UINT64                    MinSystemRamBytes;

  // Known Limitations & Quirks
  CHAR16                    KnownLimitations[96];

  // Recommended Mode
  OV_COMPAT_EXECUTION_MODE  RecommendedMode;
} OV_COMPAT_RECORD;

//
// Compatibility Evaluation Against Actual Silicon
//
typedef struct {
  BOOLEAN                   CanExecute;
  OV_COMPAT_EXECUTION_MODE  SelectedMode;
  CHAR16                    ExecutionPlan[96];
  CHAR16                    ClampedFeatures[64];
  UINT32                    ExpectedOverheadFactor;     // 100 = 1.0x baseline
  BOOLEAN                   CpuTranslationRequired;
  BOOLEAN                   GpuTranslationRequired;
  BOOLEAN                   TextureClampApplied;
  BOOLEAN                   SoftwareComputeFallback;
} OV_COMPAT_EVALUATION;

/**
  Initialize compatibility subsystem and register baseline application profiles.

  @retval EFI_SUCCESS  Compatibility subsystem initialized.
**/
EFI_STATUS
EFIAPI
OvCompatibilityInitialize (
  VOID
  );

/**
  Register an application compatibility profile.

  @param[in] Record  Pointer to application record.

  @retval EFI_SUCCESS            Record registered.
  @retval EFI_OUT_OF_RESOURCES   Database full.
  @retval EFI_INVALID_PARAMETER  Record is NULL.
**/
EFI_STATUS
EFIAPI
OvCompatibilityRegisterApp (
  IN CONST OV_COMPAT_RECORD  *Record
  );

/**
  Lookup application compatibility profile by name.

  @param[in]  AppName  Unicode application name.
  @param[out] Record   Pointer receiving record.

  @retval EFI_SUCCESS    Found.
  @retval EFI_NOT_FOUND  Not found.
**/
EFI_STATUS
EFIAPI
OvCompatibilityLookupApp (
  IN  CONST CHAR16      *AppName,
  OUT OV_COMPAT_RECORD  *Record
  );

/**
  Evaluate application profile against currently detected silicon hardware.

  @param[in]  Record      Application compatibility record.
  @param[out] Evaluation  Evaluation result and execution plan.

  @retval EFI_SUCCESS            Evaluation complete.
  @retval EFI_INVALID_PARAMETER  Invalid parameter.
**/
EFI_STATUS
EFIAPI
OvCompatibilityEvaluateAgainstHardware (
  IN  CONST OV_COMPAT_RECORD    *Record,
  OUT OV_COMPAT_EVALUATION      *Evaluation
  );

/**
  Get total count of registered known application profiles.

  @return UINTN  Number of registered profiles.
**/
UINTN
EFIAPI
OvCompatibilityGetKnownAppCount (
  VOID
  );

#endif // OV_COMPATIBILITY_LIB_H_
