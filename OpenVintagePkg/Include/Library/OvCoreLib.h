/** @file
  OpenVintage Core Subsystem Interface Definition.
  Phase 2 Modular Core Foundation.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OV_CORE_LIB_H_
#define OV_CORE_LIB_H_

#include <Uefi.h>
#include <Library/OvConfigLib.h>
#include <Library/OvLoggerLib.h>
#include <Library/OvMemoryLib.h>
#include <Library/OvHardwareLib.h>
#include <Library/OvModuleLib.h>
#include <Library/OvResolverLib.h>
#include <Library/OvSchedulerLib.h>

//
// Core Runtime States
//
typedef enum {
  OvCoreStateUninitialized = 0,
  OvCoreStateInitializing,
  OvCoreStateReady,
  OvCoreStateRunning,
  OvCoreStateDegraded,
  OvCoreStateTerminated
} OV_CORE_STATE;

/**
  Initialize all OpenVintage Core subsystems in unified sequence:
  Logger -> Memory -> Config -> Hardware -> Modules -> Resolver -> Scheduler.

  @retval EFI_SUCCESS  OpenVintage Core initialized and operational.
**/
EFI_STATUS
EFIAPI
OvCoreInitialize (
  VOID
  );

/**
  Gracefully shut down OpenVintage subsystems.

  @retval EFI_SUCCESS  All subsystems shut down cleanly.
**/
EFI_STATUS
EFIAPI
OvCoreShutdown (
  VOID
  );

/**
  Retrieve current Core runtime state.

  @return OV_CORE_STATE  Active state.
**/
OV_CORE_STATE
EFIAPI
OvCoreGetState (
  VOID
  );

/**
  Retrieve OpenVintage version components.

  @param[out] Major   Major version number.
  @param[out] Minor   Minor version number.
  @param[out] Patch   Patch level number.
  @param[out] Build   Build sequence number.
**/
VOID
EFIAPI
OvCoreGetVersion (
  OUT UINT32  *Major,
  OUT UINT32  *Minor,
  OUT UINT32  *Patch,
  OUT UINT32  *Build
  );

/**
  Retrieve formatted version string.

  @return CONST CHAR16* Static Unicode string.
**/
CONST CHAR16 *
EFIAPI
OvCoreGetVersionString (
  VOID
  );

/**
  Record last error status code and diagnostic message.

  @param[in] Status   Status code.
  @param[in] Message  Descriptive error message.
**/
VOID
EFIAPI
OvCoreSetLastError (
  IN EFI_STATUS   Status,
  IN CONST CHAR16 *Message
  );

/**
  Retrieve last recorded error status code and message.

  @param[out] Message Optional pointer receiving error message string.

  @return EFI_STATUS  Last status code recorded.
**/
EFI_STATUS
EFIAPI
OvCoreGetLastError (
  OUT CONST CHAR16  **Message OPTIONAL
  );

/**
  Print OpenVintage ASCII startup banner.
**/
VOID
EFIAPI
OvCorePrintBanner (
  VOID
  );

#endif // OV_CORE_LIB_H_
