/** @file
  OpenVintage Resource Manager Subsystem Definition.
  Phase 5 Dynamic Resource Allocation, Platform Constraints, and Performance Profiles.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OV_RESOURCE_MANAGER_LIB_H_
#define OV_RESOURCE_MANAGER_LIB_H_

#include <Uefi.h>
#include <Library/OvSchedulerLib.h>
#include <Library/OvMemoryLib.h>
#include <Library/OvHardwareLib.h>

//
// Supported Performance Profiles
//
typedef enum {
  OvResourceProfileBalanced = 1,        // Balanced power, standard thread pool, adaptive memory
  OvResourceProfilePerformance = 2,     // High clock preference, dedicated translation workers
  OvResourceProfileMaxPerformance = 3,  // Unconstrained throughput, maximum thread pool allocation
  OvResourceProfileBatteryLowPower = 4  // Minimal background workers, conservative memory footprint
} OV_RESOURCE_PROFILE;

// Profile Bitmask Flags
#define OV_PROFILE_MASK_BALANCED          BIT0
#define OV_PROFILE_MASK_PERFORMANCE       BIT1
#define OV_PROFILE_MASK_MAX_PERFORMANCE   BIT2
#define OV_PROFILE_MASK_BATTERY_LOW_POWER BIT3

//
// Platform Supported Resource Settings
//
typedef struct {
  UINT32  SupportedProfilesMask;        // Bitmask of OV_PROFILE_MASK_*
  UINT32  MaxSupportedWorkerThreads;    // Cannot exceed physical threads
  UINT64  MaxMemoryBudgetBytes;         // Up to 75% of physical RAM
  UINT64  MaxVramBudgetBytes;           // Silicon VRAM quota
  BOOLEAN SimdAvx2Supported;            // True only if CPU provides AVX2
  BOOLEAN BatteryPowerSupported;        // True only if portable/battery platform
  BOOLEAN DynamicCoreAffinitySupported; // Multi-core scheduling capability
} OV_PLATFORM_RESOURCE_CAPS;

//
// Active Resource Manager Status
//
typedef struct {
  OV_RESOURCE_PROFILE       ActiveProfile;
  UINT32                    AllocatedWorkerThreads;
  UINT64                    CommittedMemoryBytes;
  UINT64                    AvailableMemoryBytes;
  UINT64                    CommittedVramBytes;
  UINT64                    AvailableVramBytes;
  UINT32                    ActiveTasksCount;
  BOOLEAN                   ThermalThrottlingActive;
  OV_PLATFORM_RESOURCE_CAPS Capabilities;
} OV_RESOURCE_STATUS;

/**
  Initialize OpenVintage Resource Manager.

  @retval EFI_SUCCESS  Resource manager initialized.
**/
EFI_STATUS
EFIAPI
OvResourceManagerInitialize (
  VOID
  );

/**
  Retrieve platform-supported capabilities and profile mask.

  @param[out] Caps  Pointer to structure receiving capabilities.

  @retval EFI_SUCCESS            Caps populated.
  @retval EFI_INVALID_PARAMETER  Caps is NULL.
**/
EFI_STATUS
EFIAPI
OvResourceManagerGetCaps (
  OUT OV_PLATFORM_RESOURCE_CAPS  *Caps
  );

/**
  Set active performance profile. Validates that the requested profile
  is actually supported by the current platform before applying.

  @param[in] Profile  Desired profile.

  @retval EFI_SUCCESS            Profile applied.
  @retval EFI_UNSUPPORTED        Platform does not support requested profile.
  @retval EFI_INVALID_PARAMETER  Invalid profile enum.
**/
EFI_STATUS
EFIAPI
OvResourceManagerSetProfile (
  IN OV_RESOURCE_PROFILE  Profile
  );

/**
  Retrieve active performance profile.

  @param[out] Profile  Receives active profile.

  @retval EFI_SUCCESS            Profile retrieved.
  @retval EFI_INVALID_PARAMETER  Profile is NULL.
**/
EFI_STATUS
EFIAPI
OvResourceManagerGetProfile (
  OUT OV_RESOURCE_PROFILE  *Profile
  );

/**
  Request resources for a workload task based on current profile and platform limits.

  @param[in]  MemoryBytes  Requested memory budget in bytes.
  @param[in]  Priority     Workload priority.
  @param[out] Descriptor   Populated resource allocation descriptor.

  @retval EFI_SUCCESS            Resources allocated.
  @retval EFI_OUT_OF_RESOURCES   Cannot satisfy request within current limits.
  @retval EFI_INVALID_PARAMETER  Descriptor is NULL.
**/
EFI_STATUS
EFIAPI
OvResourceManagerAllocate (
  IN  UINT64                  MemoryBytes,
  IN  OV_WORKLOAD_PRIORITY    Priority,
  OUT OV_RESOURCE_DESCRIPTOR  *Descriptor
  );

/**
  Release previously allocated resources.

  @param[in] Descriptor  Descriptor to release.
**/
VOID
EFIAPI
OvResourceManagerRelease (
  IN CONST OV_RESOURCE_DESCRIPTOR  *Descriptor
  );

/**
  Get current resource utilization status.

  @param[out] Status  Structure receiving status.

  @retval EFI_SUCCESS            Status populated.
  @retval EFI_INVALID_PARAMETER  Status is NULL.
**/
EFI_STATUS
EFIAPI
OvResourceManagerGetStatus (
  OUT OV_RESOURCE_STATUS  *Status
  );

#endif // OV_RESOURCE_MANAGER_LIB_H_
