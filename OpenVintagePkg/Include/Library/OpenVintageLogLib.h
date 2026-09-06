/** @file
  OpenVintage Diagnostic Logger Library Definition.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OPEN_VINTAGE_LOG_LIB_H_
#define OPEN_VINTAGE_LOG_LIB_H_

#include <Uefi.h>

#define OV_LOG_INFO     0x00000001
#define OV_LOG_VERBOSE  0x00000002
#define OV_LOG_WARN     0x00000004
#define OV_LOG_ERROR    0x00000008

EFI_STATUS
EFIAPI
OpenVintageLogInit (
  VOID
  );

VOID
EFIAPI
OpenVintageLog (
  IN UINTN   Level,
  IN CHAR16  *Format,
  ...
  );

#endif
