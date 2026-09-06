/** @file
  OpenVintage Logging Library Implementation.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/OpenVintageLogLib.h>

EFI_STATUS
EFIAPI
OpenVintageLogInit (
  VOID
  )
{
  return EFI_SUCCESS;
}

VOID
EFIAPI
OpenVintageLog (
  IN UINTN   Level,
  IN CHAR16  *Format,
  ...
  )
{
  VA_LIST  Marker;
  CHAR16   Buffer[512];
  CHAR16   Prefix[32];

  switch (Level) {
    case OV_LOG_VERBOSE:
      UnicodeSPrint (Prefix, sizeof (Prefix), L"[OV-DBG] ");
      break;
    case OV_LOG_WARN:
      UnicodeSPrint (Prefix, sizeof (Prefix), L"[OV-WRN] ");
      break;
    case OV_LOG_ERROR:
      UnicodeSPrint (Prefix, sizeof (Prefix), L"[OV-ERR] ");
      break;
    case OV_LOG_INFO:
    default:
      UnicodeSPrint (Prefix, sizeof (Prefix), L"[OV-LOG] ");
      break;
  }

  VA_START (Marker, Format);
  UnicodeVSPrint (Buffer, sizeof (Buffer), Format, Marker);
  VA_END (Marker);

  Print (L"%s%s\n", Prefix, Buffer);
}
