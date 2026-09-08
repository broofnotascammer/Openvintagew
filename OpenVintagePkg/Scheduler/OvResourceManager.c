/** @file
  OpenVintage Resource Manager Implementation.
  Phase 5 Dynamic Resource Allocation, Platform Constraints, and Performance Profiles.

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
#include <Library/OvSchedulerLib.h>
#include <Library/OvResourceManagerLib.h>

STATIC OV_RESOURCE_STATUS  mStatus;
STATIC BOOLEAN             mInitialized = FALSE;

EFI_STATUS
EFIAPI
OvResourceManagerInitialize (
  VOID
  )
{
  OV_CPU_TOPOLOGY  Cpu;
  OV_GPU_TOPOLOGY  Gpu;

  ZeroMem (&mStatus, sizeof (OV_RESOURCE_STATUS));
  OvHardwareGetCpu (&Cpu);
  OvHardwareGetGpu (&Gpu);

  // Initialize platform capabilities grounded in detected silicon
  mStatus.Capabilities.MaxSupportedWorkerThreads = Cpu.LogicalThreads > 0 ? Cpu.LogicalThreads : 2;
  mStatus.Capabilities.MaxMemoryBudgetBytes      = 6ULL * 1024ULL * 1024ULL * 1024ULL; // 6GB out of 8GB
  mStatus.Capabilities.MaxVramBudgetBytes        = Gpu.VramSize > 0 ? Gpu.VramSize : (512 * 1024 * 1024);
  mStatus.Capabilities.SimdAvx2Supported         = Cpu.HasAVX2;
  mStatus.Capabilities.DynamicCoreAffinitySupported = (Cpu.LogicalThreads > 1);

  // Base profiles supported on all systems
  mStatus.Capabilities.SupportedProfilesMask = OV_PROFILE_MASK_BALANCED | OV_PROFILE_MASK_PERFORMANCE;

  // MaxPerformance profile requires multi-core architecture
  if (mStatus.Capabilities.MaxSupportedWorkerThreads >= 4) {
    mStatus.Capabilities.SupportedProfilesMask |= OV_PROFILE_MASK_MAX_PERFORMANCE;
  }

  // BatteryLowPower supported
  mStatus.Capabilities.BatteryPowerSupported = TRUE;
  mStatus.Capabilities.SupportedProfilesMask |= OV_PROFILE_MASK_BATTERY_LOW_POWER;

  // Set default profile to Balanced
  mStatus.ActiveProfile           = OvResourceProfileBalanced;
  mStatus.AllocatedWorkerThreads  = mStatus.Capabilities.MaxSupportedWorkerThreads > 2 ? 2 : 1;
  mStatus.AvailableMemoryBytes    = mStatus.Capabilities.MaxMemoryBudgetBytes;
  mStatus.AvailableVramBytes      = mStatus.Capabilities.MaxVramBudgetBytes;
  mStatus.ThermalThrottlingActive = FALSE;

  mInitialized = TRUE;
  OvLogTagged (OV_LOG_LEVEL_DEBUG, L"RESM", L"Resource manager initialized with profile: Balanced");
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvResourceManagerGetCaps (
  OUT OV_PLATFORM_RESOURCE_CAPS  *Caps
  )
{
  if (Caps == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mInitialized) {
    OvResourceManagerInitialize ();
  }

  CopyMem (Caps, &mStatus.Capabilities, sizeof (OV_PLATFORM_RESOURCE_CAPS));
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvResourceManagerSetProfile (
  IN OV_RESOURCE_PROFILE  Profile
  )
{
  UINT32  RequiredMask;

  if (!mInitialized) {
    OvResourceManagerInitialize ();
  }

  switch (Profile) {
    case OvResourceProfileBalanced:
      RequiredMask = OV_PROFILE_MASK_BALANCED;
      break;
    case OvResourceProfilePerformance:
      RequiredMask = OV_PROFILE_MASK_PERFORMANCE;
      break;
    case OvResourceProfileMaxPerformance:
      RequiredMask = OV_PROFILE_MASK_MAX_PERFORMANCE;
      break;
    case OvResourceProfileBatteryLowPower:
      RequiredMask = OV_PROFILE_MASK_BATTERY_LOW_POWER;
      break;
    default:
      return EFI_INVALID_PARAMETER;
  }

  // Strictly enforce platform capability: only activate supported profiles
  if ((mStatus.Capabilities.SupportedProfilesMask & RequiredMask) == 0) {
    OvLogTagged (OV_LOG_LEVEL_WARN, L"RESM", L"Requested profile %d not supported by current silicon", Profile);
    return EFI_UNSUPPORTED;
  }

  mStatus.ActiveProfile = Profile;

  // Re-tune thread pool based on selected profile
  switch (Profile) {
    case OvResourceProfileBalanced:
      mStatus.AllocatedWorkerThreads = mStatus.Capabilities.MaxSupportedWorkerThreads > 2 ? 2 : 1;
      OvSchedulerSetProfile (OvPerfProfileBalanced);
      break;
    case OvResourceProfilePerformance:
      mStatus.AllocatedWorkerThreads = mStatus.Capabilities.MaxSupportedWorkerThreads > 1 ?
                                       mStatus.Capabilities.MaxSupportedWorkerThreads - 1 : 1;
      OvSchedulerSetProfile (OvPerfProfilePerformance);
      break;
    case OvResourceProfileMaxPerformance:
      mStatus.AllocatedWorkerThreads = mStatus.Capabilities.MaxSupportedWorkerThreads;
      OvSchedulerSetProfile (OvPerfProfileUltraLowLatency);
      break;
    case OvResourceProfileBatteryLowPower:
      mStatus.AllocatedWorkerThreads = 1;
      OvSchedulerSetProfile (OvPerfProfilePowerSaver);
      break;
  }

  OvLogTagged (OV_LOG_LEVEL_INFO, L"RESM", L"Resource profile set to %d (Workers: %u)",
    Profile, mStatus.AllocatedWorkerThreads);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvResourceManagerGetProfile (
  OUT OV_RESOURCE_PROFILE  *Profile
  )
{
  if (Profile == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mInitialized) {
    OvResourceManagerInitialize ();
  }

  *Profile = mStatus.ActiveProfile;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvResourceManagerAllocate (
  IN  UINT64                  MemoryBytes,
  IN  OV_WORKLOAD_PRIORITY    Priority,
  OUT OV_RESOURCE_DESCRIPTOR  *Descriptor
  )
{
  if (Descriptor == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mInitialized) {
    OvResourceManagerInitialize ();
  }

  if (MemoryBytes > mStatus.AvailableMemoryBytes) {
    OvLogTagged (OV_LOG_LEVEL_WARN, L"RESM", L"Resource allocation denied: requested %lu > available %lu",
      MemoryBytes, mStatus.AvailableMemoryBytes);
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (Descriptor, sizeof (OV_RESOURCE_DESCRIPTOR));
  Descriptor->MemoryQuotaBytes = MemoryBytes;

  // Configure affinity based on priority and active profile
  if (Priority == OvPriorityRealtime || mStatus.ActiveProfile == OvResourceProfileMaxPerformance) {
    Descriptor->CoreAffinityMask       = (1ULL << mStatus.Capabilities.MaxSupportedWorkerThreads) - 1;
    Descriptor->MaxLatencyMicroseconds = 500;
  } else {
    Descriptor->CoreAffinityMask       = 0x3; // Default first 2 cores
    Descriptor->MaxLatencyMicroseconds = 2000;
  }

  Descriptor->ThermalBudgetPoints = 85; // Standard thermal margin

  mStatus.CommittedMemoryBytes += MemoryBytes;
  mStatus.AvailableMemoryBytes -= MemoryBytes;
  mStatus.ActiveTasksCount++;

  return EFI_SUCCESS;
}

VOID
EFIAPI
OvResourceManagerRelease (
  IN CONST OV_RESOURCE_DESCRIPTOR  *Descriptor
  )
{
  if (Descriptor == NULL) {
    return;
  }

  if (mStatus.CommittedMemoryBytes >= Descriptor->MemoryQuotaBytes) {
    mStatus.CommittedMemoryBytes -= Descriptor->MemoryQuotaBytes;
    mStatus.AvailableMemoryBytes += Descriptor->MemoryQuotaBytes;
  }

  if (mStatus.ActiveTasksCount > 0) {
    mStatus.ActiveTasksCount--;
  }
}

EFI_STATUS
EFIAPI
OvResourceManagerGetStatus (
  OUT OV_RESOURCE_STATUS  *Status
  )
{
  if (Status == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mInitialized) {
    OvResourceManagerInitialize ();
  }

  CopyMem (Status, &mStatus, sizeof (OV_RESOURCE_STATUS));
  return EFI_SUCCESS;
}
