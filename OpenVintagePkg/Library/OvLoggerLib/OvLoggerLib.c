/** @file
  OpenVintage Enhanced Logger Library Implementation.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/IoLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/OvLoggerLib.h>

STATIC UINT32  mCurrentLogLevel = OV_LOG_LEVEL_DEBUG;
STATIC UINT32  mLogSequenceNumber = 0;

STATIC
VOID
WriteSerialByte (
  IN UINT8  Data
  )
{
  // Port 0x3FD is Line Status Register; Bit 5 is Transmitter Holding Register Empty
  // For basic QEMU serial output, direct write to 0x3F8 is reliable
  IoWrite8 (0x3F8, Data);
}

STATIC
VOID
WriteSerialAsciiString (
  IN CONST CHAR8  *String
  )
{
  while (*String != '\0') {
    if (*String == '\n') {
      WriteSerialByte ('\r');
    }
    WriteSerialByte ((UINT8)*String);
    String++;
  }
}

EFI_STATUS
EFIAPI
OvLogInit (
  VOID
  )
{
  mCurrentLogLevel = OV_LOG_LEVEL_DEBUG;
  mLogSequenceNumber = 0;
  return EFI_SUCCESS;
}

VOID
EFIAPI
OvLogSetLevel (
  IN UINT32  Level
  )
{
  mCurrentLogLevel = Level;
}

UINT32
EFIAPI
OvLogGetLevel (
  VOID
  )
{
  return mCurrentLogLevel;
}

VOID
EFIAPI
OvLogTagged (
  IN UINT32        Level,
  IN CONST CHAR16  *Subsystem OPTIONAL,
  IN CONST CHAR16  *Format,
  ...
  )
{
  VA_LIST     Marker;
  CHAR16      FormattedMessage[512];
  CHAR16      Header[64];
  CHAR8       AsciiBuffer[600];
  CONST CHAR16 *LevelStr;
  EFI_TIME    Time;
  EFI_STATUS  Status;
  BOOLEAN     HaveTime;
  CONST CHAR16 *Tag;

  if (Level < mCurrentLogLevel) {
    return;
  }

  mLogSequenceNumber++;

  switch (Level) {
    case OV_LOG_LEVEL_DEBUG:
      LevelStr = L"DBG";
      break;
    case OV_LOG_LEVEL_WARN:
      LevelStr = L"WRN";
      break;
    case OV_LOG_LEVEL_ERROR:
      LevelStr = L"ERR";
      break;
    case OV_LOG_LEVEL_INFO:
    default:
      LevelStr = L"INF";
      break;
  }

  Tag = (Subsystem != NULL) ? Subsystem : L"SYS";

  HaveTime = FALSE;
  if ((gRT != NULL) && (gRT->GetTime != NULL)) {
    Status = gRT->GetTime (&Time, NULL);
    if (!EFI_ERROR (Status)) {
      HaveTime = TRUE;
    }
  }

  if (HaveTime) {
    UnicodeSPrint (
      Header,
      sizeof (Header),
      L"[OV:%02d:%02d:%02d:%s:%s] ",
      Time.Hour,
      Time.Minute,
      Time.Second,
      LevelStr,
      Tag
      );
  } else {
    UnicodeSPrint (
      Header,
      sizeof (Header),
      L"[OV:%04d:%s:%s] ",
      mLogSequenceNumber,
      LevelStr,
      Tag
      );
  }

  VA_START (Marker, Format);
  UnicodeVSPrint (FormattedMessage, sizeof (FormattedMessage), Format, Marker);
  VA_END (Marker);

  // Print to UEFI Console
  Print (L"%s%s\n", Header, FormattedMessage);

  // Also mirror directly to Serial COM1 (0x3F8) for virtualization capture
  UnicodeStrToAsciiStrS (Header, AsciiBuffer, sizeof (AsciiBuffer));
  WriteSerialAsciiString (AsciiBuffer);

  UnicodeStrToAsciiStrS (FormattedMessage, AsciiBuffer, sizeof (AsciiBuffer));
  WriteSerialAsciiString (AsciiBuffer);
  WriteSerialAsciiString ("\n");
}

VOID
EFIAPI
OvLog (
  IN UINT32        Level,
  IN CONST CHAR16  *Format,
  ...
  )
{
  VA_LIST  Marker;
  CHAR16   Buffer[512];

  if (Level < mCurrentLogLevel) {
    return;
  }

  VA_START (Marker, Format);
  UnicodeVSPrint (Buffer, sizeof (Buffer), Format, Marker);
  VA_END (Marker);

  OvLogTagged (Level, NULL, L"%s", Buffer);
}
