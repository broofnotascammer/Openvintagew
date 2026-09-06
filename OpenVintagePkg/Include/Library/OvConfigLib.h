/** @file
  OpenVintage Configuration Subsystem Definition.
  Phase 2 Persistent & Runtime Configuration Interface.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OV_CONFIG_LIB_H_
#define OV_CONFIG_LIB_H_

#include <Uefi.h>

//
// Performance Profiles
//
typedef enum {
  OvPerfProfilePowerSaver = 0,
  OvPerfProfileBalanced,
  OvPerfProfilePerformance,
  OvPerfProfileUltraLowLatency
} OV_PERF_PROFILE;

//
// Feature Flags
//
#define OV_FEATURE_ROBUST_LOGGING     BIT0
#define OV_FEATURE_MEMORY_TRACKING    BIT1
#define OV_FEATURE_SAFE_CPUID         BIT2
#define OV_FEATURE_RESOLVER_STRICT    BIT3
#define OV_FEATURE_SCHEDULER_FAIR     BIT4
#define OV_FEATURE_SIMD_AVX_ENABLE    BIT5
#define OV_FEATURE_SIMD_AVX2_ENABLE   BIT6
#define OV_FEATURE_FAST_BOOT          BIT7

//
// Future Graphics Renderer Targets
//
typedef enum {
  OvGpuBackendAuto = 0,
  OvGpuBackendMetalEmu,
  OvGpuBackendVulkanEmu,
  OvGpuBackendOpenGLCore,
  OvGpuBackendSoftware
} OV_GPU_BACKEND;

//
// Target Vector SIMD Capabilities
//
typedef enum {
  OvVectorLevelDefault = 0,
  OvVectorLevelSSE41,
  OvVectorLevelSSE42,
  OvVectorLevelAVX,
  OvVectorLevelAVX2
} OV_VECTOR_LEVEL;

//
// OpenVintage Configuration Structure
//
typedef struct {
  // Version Info
  UINT16          MajorVersion;
  UINT16          MinorVersion;
  UINT16          PatchVersion;
  UINT16          BuildNumber;
  CHAR16          ReleaseChannel[16];

  // Runtime Controls
  BOOLEAN         DebugMode;
  UINT32          LogLevel;
  OV_PERF_PROFILE PerformanceMode;
  UINT32          HardwareProfile;
  UINT64          FeatureFlags;

  // Graphics Settings (Future)
  OV_GPU_BACKEND  TargetRenderer;
  UINT32          MaxResolutionWidth;
  UINT32          MaxResolutionHeight;
  UINT32          MaxMsaaSamples;
  BOOLEAN         TextureClampingEnabled;

  // CPU Translation Settings (Future)
  OV_VECTOR_LEVEL TargetVectorLevel;
  BOOLEAN         EnableInstructionFallback;
  UINT32          JitCacheSizeMb;
} OV_CONFIG_DATA;

/**
  Initialize configuration subsystem with defaults or specified data.

  @param[in] InitialConfig  Optional initial configuration overrides.

  @retval EFI_SUCCESS       Configuration initialized.
**/
EFI_STATUS
EFIAPI
OvConfigInitialize (
  IN CONST OV_CONFIG_DATA  *InitialConfig OPTIONAL
  );

/**
  Retrieve current active configuration.

  @param[out] Config   Buffer receiving current configuration.

  @retval EFI_SUCCESS  Configuration retrieved.
**/
EFI_STATUS
EFIAPI
OvConfigGet (
  OUT OV_CONFIG_DATA  *Config
  );

/**
  Update active configuration.

  @param[in] Config    New configuration values.

  @retval EFI_SUCCESS  Configuration updated.
**/
EFI_STATUS
EFIAPI
OvConfigSet (
  IN CONST OV_CONFIG_DATA  *Config
  );

/**
  Query whether a specific feature flag is set.

  @param[in] FlagMask  Bitmask of feature to test.

  @return BOOLEAN      TRUE if all bits in mask are set.
**/
BOOLEAN
EFIAPI
OvConfigGetFeatureFlag (
  IN UINT64  FlagMask
  );

/**
  Enable or disable a feature flag.

  @param[in] FlagMask  Bitmask of feature to modify.
  @param[in] Enable    TRUE to set, FALSE to clear.
**/
VOID
EFIAPI
OvConfigSetFeatureFlag (
  IN UINT64   FlagMask,
  IN BOOLEAN  Enable
  );

/**
  Print configuration state to logger.
**/
VOID
EFIAPI
OvConfigDump (
  VOID
  );

#endif // OV_CONFIG_LIB_H_
