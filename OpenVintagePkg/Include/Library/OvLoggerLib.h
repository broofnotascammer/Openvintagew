/** @file
  OpenVintage Diagnostic and Execution Logger Definition.
  Phase 2 Enhanced Subsystem Logging Interface.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OV_LOGGER_LIB_H_
#define OV_LOGGER_LIB_H_

#include <Uefi.h>

//
// Log Severity Levels
//
#define OV_LOG_LEVEL_DEBUG    0
#define OV_LOG_LEVEL_INFO     1
#define OV_LOG_LEVEL_WARN     2
#define OV_LOG_LEVEL_ERROR    3
#define OV_LOG_LEVEL_NONE     4

/**
  Initialize OpenVintage logging subsystem.

  @retval EFI_SUCCESS   Logger initialized.
**/
EFI_STATUS
EFIAPI
OvLogInit (
  VOID
  );

/**
  Set active minimum logging severity level.

  @param[in] Level      Minimum level to log.
**/
VOID
EFIAPI
OvLogSetLevel (
  IN UINT32  Level
  );

/**
  Get active minimum logging severity level.

  @return UINT32  Current log level.
**/
UINT32
EFIAPI
OvLogGetLevel (
  VOID
  );

/**
  Write formatted message with severity and optional subsystem tag.

  @param[in] Level      Severity level.
  @param[in] Subsystem  Short subsystem name (e.g. L"CORE", L"HW", L"MEM").
  @param[in] Format     Unicode format string.
  @param[in] ...        Variable arguments.
**/
VOID
EFIAPI
OvLogTagged (
  IN UINT32        Level,
  IN CONST CHAR16  *Subsystem OPTIONAL,
  IN CONST CHAR16  *Format,
  ...
  );

/**
  Write formatted message with default tag.

  @param[in] Level      Severity level.
  @param[in] Format     Unicode format string.
  @param[in] ...        Variable arguments.
**/
VOID
EFIAPI
OvLog (
  IN UINT32        Level,
  IN CONST CHAR16  *Format,
  ...
  );

#endif // OV_LOGGER_LIB_H_
