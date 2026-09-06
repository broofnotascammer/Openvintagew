/** @file
  OpenVintage Subsystem Module Abstraction Implementation.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/OvLoggerLib.h>
#include <Library/OvModuleLib.h>

STATIC OV_MODULE_DESCRIPTOR  mModules[OV_MAX_MODULES];
STATIC UINTN                 mModuleCount = 0;
STATIC BOOLEAN               mEngineInitialized = FALSE;

EFI_STATUS
EFIAPI
OvModuleInitializeEngine (
  VOID
  )
{
  ZeroMem (mModules, sizeof (mModules));
  mModuleCount = 0;
  mEngineInitialized = TRUE;
  OvLogTagged (OV_LOG_LEVEL_DEBUG, L"MOD", L"Module orchestration engine initialized");
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvModuleRegister (
  IN OUT OV_MODULE_DESCRIPTOR  *Descriptor
  )
{
  UINTN  Index;

  if (Descriptor == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mEngineInitialized) {
    OvModuleInitializeEngine ();
  }

  // Check for duplicate name
  for (Index = 0; Index < mModuleCount; Index++) {
    if (StrCmp (mModules[Index].Name, Descriptor->Name) == 0) {
      OvLogTagged (OV_LOG_LEVEL_WARN, L"MOD", L"Module '%s' already registered", Descriptor->Name);
      return EFI_ALREADY_STARTED;
    }
  }

  if (mModuleCount >= OV_MAX_MODULES) {
    OvLogTagged (OV_LOG_LEVEL_ERROR, L"MOD", L"Exceeded max modules limit (%u)", OV_MAX_MODULES);
    return EFI_OUT_OF_RESOURCES;
  }

  Descriptor->ModuleId = (UINT32)(mModuleCount + 1);
  Descriptor->Status   = OvModStatusRegistered;

  CopyMem (&mModules[mModuleCount], Descriptor, sizeof (OV_MODULE_DESCRIPTOR));
  mModuleCount++;

  OvLogTagged (
    OV_LOG_LEVEL_DEBUG,
    L"MOD",
    L"Registered module [%u]: '%s' (Type %u, v%u)",
    Descriptor->ModuleId,
    Descriptor->Name,
    (UINT32)Descriptor->Type,
    Descriptor->Version
    );

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvModuleInitializeAll (
  VOID
  )
{
  UINTN       Index;
  EFI_STATUS  Status;

  if (!mEngineInitialized) {
    OvModuleInitializeEngine ();
  }

  OvLogTagged (OV_LOG_LEVEL_INFO, L"MOD", L"Initializing %u registered module(s)...", (UINT32)mModuleCount);

  for (Index = 0; Index < mModuleCount; Index++) {
    if (mModules[Index].Initialize != NULL) {
      mModules[Index].Status = OvModStatusInitializing;
      Status = mModules[Index].Initialize (&mModules[Index]);
      if (EFI_ERROR (Status)) {
        mModules[Index].Status = OvModStatusError;
        OvLogTagged (
          OV_LOG_LEVEL_ERROR,
          L"MOD",
          L"Failed to initialize module '%s' (Status: %r)",
          mModules[Index].Name,
          Status
          );
      } else {
        mModules[Index].Status = OvModStatusActive;
        OvLogTagged (OV_LOG_LEVEL_INFO, L"MOD", L"Module '%s' initialized cleanly [ACTIVE]", mModules[Index].Name);
      }
    } else {
      mModules[Index].Status = OvModStatusActive;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvModuleShutdownAll (
  VOID
  )
{
  INTN        Index;
  EFI_STATUS  Status;

  if (!mEngineInitialized || (mModuleCount == 0)) {
    return EFI_SUCCESS;
  }

  OvLogTagged (OV_LOG_LEVEL_INFO, L"MOD", L"Shutting down %u module(s) in reverse order...", (UINT32)mModuleCount);

  for (Index = (INTN)mModuleCount - 1; Index >= 0; Index--) {
    if (mModules[Index].Shutdown != NULL) {
      Status = mModules[Index].Shutdown (&mModules[Index]);
      if (EFI_ERROR (Status)) {
        OvLogTagged (OV_LOG_LEVEL_WARN, L"MOD", L"Module '%s' shutdown reported: %r", mModules[Index].Name, Status);
      }
    }
    mModules[Index].Status = OvModStatusShutdown;
  }

  return EFI_SUCCESS;
}

UINTN
EFIAPI
OvModuleGetCount (
  VOID
  )
{
  return mModuleCount;
}

EFI_STATUS
EFIAPI
OvModuleGetDescriptor (
  IN  UINT32                ModuleId,
  OUT OV_MODULE_DESCRIPTOR  **Descriptor
  )
{
  UINTN  Index;

  if (Descriptor == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < mModuleCount; Index++) {
    if (mModules[Index].ModuleId == ModuleId) {
      *Descriptor = &mModules[Index];
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

VOID
EFIAPI
OvModuleDumpList (
  VOID
  )
{
  UINTN         Index;
  CONST CHAR16  *StatusStr;

  OvLogTagged (OV_LOG_LEVEL_INFO, L"MOD", L"--- OpenVintage Registered Subsystem Modules (%u) ---", (UINT32)mModuleCount);

  for (Index = 0; Index < mModuleCount; Index++) {
    switch (mModules[Index].Status) {
      case OvModStatusRegistered:   StatusStr = L"REGISTERED"; break;
      case OvModStatusInitializing: StatusStr = L"INITIALIZING"; break;
      case OvModStatusActive:       StatusStr = L"ACTIVE"; break;
      case OvModStatusDegraded:     StatusStr = L"DEGRADED"; break;
      case OvModStatusError:        StatusStr = L"ERROR"; break;
      case OvModStatusShutdown:     StatusStr = L"SHUTDOWN"; break;
      default:                      StatusStr = L"UNKNOWN"; break;
    }

    OvLogTagged (
      OV_LOG_LEVEL_INFO,
      L"MOD",
      L"  [%u] %-16s | Type: %u | v%u | Status: %s",
      mModules[Index].ModuleId,
      mModules[Index].Name,
      (UINT32)mModules[Index].Type,
      mModules[Index].Version,
      StatusStr
      );
  }
}
