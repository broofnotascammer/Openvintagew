/** @file
  OpenVintage Core Subsystem Implementation.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/OvCoreLib.h>

#define OV_CORE_MAJOR   0
#define OV_CORE_MINOR   2
#define OV_CORE_PATCH   0
#define OV_CORE_BUILD   20260906

STATIC OV_CORE_STATE  mCoreState = OvCoreStateUninitialized;
STATIC EFI_STATUS     mLastErrorStatus = EFI_SUCCESS;
STATIC CHAR16         mLastErrorMessage[128] = { 0 };

STATIC
EFI_STATUS
EFIAPI
CoreDummyModuleInit (
  IN OV_MODULE_DESCRIPTOR  *Module
  )
{
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
CoreDummyModuleShutdown (
  IN OV_MODULE_DESCRIPTOR  *Module
  )
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvCoreInitialize (
  VOID
  )
{
  EFI_STATUS            Status;
  OV_MODULE_DESCRIPTOR  Mod;

  mCoreState = OvCoreStateInitializing;

  // 1. Initialize Logger
  Status = OvLogInit ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // 2. Initialize Memory Tracking
  Status = OvMemoryInitialize ();
  if (EFI_ERROR (Status)) {
    OvLogTagged (OV_LOG_LEVEL_ERROR, L"CORE", L"Failed to initialize memory layer: %r", Status);
    mCoreState = OvCoreStateDegraded;
    return Status;
  }

  // 3. Initialize Configuration
  Status = OvConfigInitialize (NULL);
  if (EFI_ERROR (Status)) {
    OvLogTagged (OV_LOG_LEVEL_ERROR, L"CORE", L"Failed to initialize configuration layer: %r", Status);
    mCoreState = OvCoreStateDegraded;
    return Status;
  }

  // 4. Initialize Hardware Discovery
  Status = OvHardwareInitialize ();
  if (EFI_ERROR (Status)) {
    OvLogTagged (OV_LOG_LEVEL_ERROR, L"CORE", L"Failed to initialize hardware layer: %r", Status);
    mCoreState = OvCoreStateDegraded;
    return Status;
  }

  // 5. Initialize Module Engine
  Status = OvModuleInitializeEngine ();
  if (EFI_ERROR (Status)) {
    OvLogTagged (OV_LOG_LEVEL_ERROR, L"CORE", L"Failed to initialize module engine: %r", Status);
    mCoreState = OvCoreStateDegraded;
    return Status;
  }

  // Register Built-in Core Subsystem Modules
  ZeroMem (&Mod, sizeof (Mod));
  StrnCpyS (Mod.Name, sizeof (Mod.Name) / sizeof (CHAR16), L"OvCore", 31);
  Mod.Type       = OvModTypeCore;
  Mod.Version    = 0x00020000;
  Mod.Initialize = CoreDummyModuleInit;
  Mod.Shutdown   = CoreDummyModuleShutdown;
  OvModuleRegister (&Mod);

  ZeroMem (&Mod, sizeof (Mod));
  StrnCpyS (Mod.Name, sizeof (Mod.Name) / sizeof (CHAR16), L"OvMemory", 31);
  Mod.Type       = OvModTypeMemory;
  Mod.Version    = 0x00020000;
  Mod.Initialize = CoreDummyModuleInit;
  Mod.Shutdown   = CoreDummyModuleShutdown;
  OvModuleRegister (&Mod);

  ZeroMem (&Mod, sizeof (Mod));
  StrnCpyS (Mod.Name, sizeof (Mod.Name) / sizeof (CHAR16), L"OvHardware", 31);
  Mod.Type       = OvModTypeHardware;
  Mod.Version    = 0x00020000;
  Mod.Initialize = CoreDummyModuleInit;
  Mod.Shutdown   = CoreDummyModuleShutdown;
  OvModuleRegister (&Mod);

  ZeroMem (&Mod, sizeof (Mod));
  StrnCpyS (Mod.Name, sizeof (Mod.Name) / sizeof (CHAR16), L"OvResolver", 31);
  Mod.Type       = OvModTypeResolver;
  Mod.Version    = 0x00020000;
  Mod.Initialize = CoreDummyModuleInit;
  Mod.Shutdown   = CoreDummyModuleShutdown;
  OvModuleRegister (&Mod);

  ZeroMem (&Mod, sizeof (Mod));
  StrnCpyS (Mod.Name, sizeof (Mod.Name) / sizeof (CHAR16), L"OvScheduler", 31);
  Mod.Type       = OvModTypeScheduler;
  Mod.Version    = 0x00020000;
  Mod.Initialize = CoreDummyModuleInit;
  Mod.Shutdown   = CoreDummyModuleShutdown;
  OvModuleRegister (&Mod);

  // Initialize all registered modules
  OvModuleInitializeAll ();

  // 6. Initialize Resolver Framework
  Status = OvResolverInitialize ();
  if (EFI_ERROR (Status)) {
    OvLogTagged (OV_LOG_LEVEL_ERROR, L"CORE", L"Failed to initialize resolver: %r", Status);
    mCoreState = OvCoreStateDegraded;
    return Status;
  }

  // 7. Initialize Scheduler Framework
  Status = OvSchedulerInitialize ();
  if (EFI_ERROR (Status)) {
    OvLogTagged (OV_LOG_LEVEL_ERROR, L"CORE", L"Failed to initialize scheduler: %r", Status);
    mCoreState = OvCoreStateDegraded;
    return Status;
  }

  mCoreState = OvCoreStateReady;
  OvLogTagged (
    OV_LOG_LEVEL_INFO,
    L"CORE",
    L"OpenVintage Core Subsystems operational [v%u.%u.%u-P2, State: READY]",
    OV_CORE_MAJOR,
    OV_CORE_MINOR,
    OV_CORE_PATCH
    );

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvCoreShutdown (
  VOID
  )
{
  OvLogTagged (OV_LOG_LEVEL_INFO, L"CORE", L"Initiating OpenVintage Core shutdown...");

  // Shut down modules in reverse
  OvModuleShutdownAll ();

  mCoreState = OvCoreStateTerminated;
  OvLogTagged (OV_LOG_LEVEL_INFO, L"CORE", L"OpenVintage Core shutdown complete.");
  return EFI_SUCCESS;
}

OV_CORE_STATE
EFIAPI
OvCoreGetState (
  VOID
  )
{
  return mCoreState;
}

VOID
EFIAPI
OvCoreGetVersion (
  OUT UINT32  *Major,
  OUT UINT32  *Minor,
  OUT UINT32  *Patch,
  OUT UINT32  *Build
  )
{
  if (Major != NULL) *Major = OV_CORE_MAJOR;
  if (Minor != NULL) *Minor = OV_CORE_MINOR;
  if (Patch != NULL) *Patch = OV_CORE_PATCH;
  if (Build != NULL) *Build = OV_CORE_BUILD;
}

CONST CHAR16 *
EFIAPI
OvCoreGetVersionString (
  VOID
  )
{
  return L"OpenVintage Platform & Firmware v0.2.0 (Phase 2 Foundation)";
}

VOID
EFIAPI
OvCoreSetLastError (
  IN EFI_STATUS   Status,
  IN CONST CHAR16 *Message
  )
{
  mLastErrorStatus = Status;
  if (Message != NULL) {
    StrnCpyS (mLastErrorMessage, sizeof (mLastErrorMessage) / sizeof (CHAR16), Message, 127);
  } else {
    mLastErrorMessage[0] = L'\0';
  }
}

EFI_STATUS
EFIAPI
OvCoreGetLastError (
  OUT CONST CHAR16  **Message OPTIONAL
  )
{
  if (Message != NULL) {
    *Message = mLastErrorMessage;
  }
  return mLastErrorStatus;
}

VOID
EFIAPI
OvCorePrintBanner (
  VOID
  )
{
  Print (L"\n");
  Print (L"================================================================\n");
  Print (L"  ____  ____  _____ _   ___     _____ _   _ _____  _    ____ _____ \n");
  Print (L" / __ \\\\|  _ \\\\| ____| \\\\ | \\\\ \\\\   / /_ _| \\\\ | |_   _|/ \\\\  / ___| ____|\n");
  Print (L"| |  | | |_) |  _| |  \\\\| |\\\\ \\\\ / / | ||  \\\\| | | | / _ \\\\| |  _|  _|  \n");
  Print (L"| |__| |  __/| |___| |\\\\  | \\\\ V /  | || |\\\\  | | |/ ___ \\\\ |_| | |___ \n");
  Print (L" \\\\____/|_|   |_____|_| \\\\_|  \\\\_/  |___|_| \\\\_| |_/_/   \\\\_\\\\____|_____|\n");
  Print (L"================================================================\n");
  Print (L" OpenVintage Modular Platform & Firmware Architecture (Phase 2)\n");
  Print (L" Target: Legacy Intel Mac / x86_64 Silicon (2006 - 2015)\n");
  Print (L"================================================================\n\n");
}
