/** @file
  OpenVintage Configuration Subsystem Implementation.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/OvLoggerLib.h>
#include <Library/OvConfigLib.h>

STATIC OV_CONFIG_DATA  mActiveConfig;
STATIC BOOLEAN         mConfigInitialized = FALSE;

STATIC
VOID
SetDefaultConfig (
  OUT OV_CONFIG_DATA  *Config
  )
{
  ZeroMem (Config, sizeof (OV_CONFIG_DATA));

  Config->MajorVersion = 0;
  Config->MinorVersion = 2;
  Config->PatchVersion = 0;
  Config->BuildNumber  = 2026;
  UnicodeSPrint (Config->ReleaseChannel, sizeof (Config->ReleaseChannel), L"Phase-2");

  Config->DebugMode        = TRUE;
  Config->LogLevel         = OV_LOG_LEVEL_DEBUG;
  Config->PerformanceMode  = OvPerfProfileBalanced;
  Config->HardwareProfile  = 0; // Auto-detect
  Config->FeatureFlags     = (
                               OV_FEATURE_ROBUST_LOGGING  |
                               OV_FEATURE_MEMORY_TRACKING |
                               OV_FEATURE_SAFE_CPUID      |
                               OV_FEATURE_RESOLVER_STRICT |
                               OV_FEATURE_SCHEDULER_FAIR  |
                               OV_FEATURE_SIMD_AVX_ENABLE |
                               OV_FEATURE_SIMD_AVX2_ENABLE
                               );

  // Future graphics settings
  Config->TargetRenderer         = OvGpuBackendAuto;
  Config->MaxResolutionWidth     = 1920;
  Config->MaxResolutionHeight    = 1200;
  Config->MaxMsaaSamples         = 4;
  Config->TextureClampingEnabled = FALSE;

  // Future CPU translation settings
  Config->TargetVectorLevel         = OvVectorLevelAVX2;
  Config->EnableInstructionFallback = TRUE;
  Config->JitCacheSizeMb            = 64;
}

EFI_STATUS
EFIAPI
OvConfigInitialize (
  IN CONST OV_CONFIG_DATA  *InitialConfig OPTIONAL
  )
{
  if (InitialConfig != NULL) {
    CopyMem (&mActiveConfig, InitialConfig, sizeof (OV_CONFIG_DATA));
  } else {
    SetDefaultConfig (&mActiveConfig);
  }

  mConfigInitialized = TRUE;
  OvLogTagged (
    OV_LOG_LEVEL_DEBUG,
    L"CONF",
    L"Configuration initialized (v%u.%u.%u-%s, Flags: 0x%016lx)",
    mActiveConfig.MajorVersion,
    mActiveConfig.MinorVersion,
    mActiveConfig.PatchVersion,
    mActiveConfig.ReleaseChannel,
    mActiveConfig.FeatureFlags
    );

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvConfigGet (
  OUT OV_CONFIG_DATA  *Config
  )
{
  if (Config == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mConfigInitialized) {
    OvConfigInitialize (NULL);
  }

  CopyMem (Config, &mActiveConfig, sizeof (OV_CONFIG_DATA));
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvConfigSet (
  IN CONST OV_CONFIG_DATA  *Config
  )
{
  if (Config == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (&mActiveConfig, Config, sizeof (OV_CONFIG_DATA));
  mConfigInitialized = TRUE;

  OvLogTagged (OV_LOG_LEVEL_INFO, L"CONF", L"Configuration profile updated successfully");
  return EFI_SUCCESS;
}

BOOLEAN
EFIAPI
OvConfigGetFeatureFlag (
  IN UINT64  FlagMask
  )
{
  if (!mConfigInitialized) {
    OvConfigInitialize (NULL);
  }

  return ((mActiveConfig.FeatureFlags & FlagMask) == FlagMask);
}

VOID
EFIAPI
OvConfigSetFeatureFlag (
  IN UINT64   FlagMask,
  IN BOOLEAN  Enable
  )
{
  if (!mConfigInitialized) {
    OvConfigInitialize (NULL);
  }

  if (Enable) {
    mActiveConfig.FeatureFlags |= FlagMask;
  } else {
    mActiveConfig.FeatureFlags &= ~FlagMask;
  }
}

VOID
EFIAPI
OvConfigDump (
  VOID
  )
{
  if (!mConfigInitialized) {
    OvConfigInitialize (NULL);
  }

  OvLogTagged (
    OV_LOG_LEVEL_INFO,
    L"CONF",
    L"Active Config: v%u.%u.%u (Build %u, Channel: %s)",
    mActiveConfig.MajorVersion,
    mActiveConfig.MinorVersion,
    mActiveConfig.PatchVersion,
    mActiveConfig.BuildNumber,
    mActiveConfig.ReleaseChannel
    );
  OvLogTagged (
    OV_LOG_LEVEL_INFO,
    L"CONF",
    L"Perf Profile: %u | Feature Flags: 0x%016lx | Debug: %s",
    (UINT32)mActiveConfig.PerformanceMode,
    mActiveConfig.FeatureFlags,
    mActiveConfig.DebugMode ? L"ON" : L"OFF"
    );
  OvLogTagged (
    OV_LOG_LEVEL_INFO,
    L"CONF",
    L"Graphics: Backend=%u MaxRes=%ux%u MSAA=%u Clamp=%s",
    (UINT32)mActiveConfig.TargetRenderer,
    mActiveConfig.MaxResolutionWidth,
    mActiveConfig.MaxResolutionHeight,
    mActiveConfig.MaxMsaaSamples,
    mActiveConfig.TextureClampingEnabled ? L"YES" : L"NO"
    );
  OvLogTagged (
    OV_LOG_LEVEL_INFO,
    L"CONF",
    L"CPU Translation: TargetVector=%u Fallback=%s JITCache=%u MB",
    (UINT32)mActiveConfig.TargetVectorLevel,
    mActiveConfig.EnableInstructionFallback ? L"ENABLED" : L"DISABLED",
    mActiveConfig.JitCacheSizeMb
    );
}
