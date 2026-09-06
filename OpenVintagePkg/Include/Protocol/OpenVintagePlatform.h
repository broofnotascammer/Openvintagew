/** @file
  OpenVintage Platform Protocol Definition.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OPEN_VINTAGE_PLATFORM_H_
#define OPEN_VINTAGE_PLATFORM_H_

#define OPEN_VINTAGE_PLATFORM_PROTOCOL_GUID \
  { 0x8A923891, 0x5981, 0x47E1, { 0x93, 0x8B, 0x22, 0x9A, 0x73, 0xA1, 0x6E, 0x50 } }

typedef struct _OPEN_VINTAGE_PLATFORM_PROTOCOL OPEN_VINTAGE_PLATFORM_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *OPEN_VINTAGE_GET_PLATFORM_VERSION)(
  IN  OPEN_VINTAGE_PLATFORM_PROTOCOL  *This,
  OUT UINT32                          *MajorVersion,
  OUT UINT32                          *MinorVersion,
  OUT CHAR16                          **PlatformName
  );

struct _OPEN_VINTAGE_PLATFORM_PROTOCOL {
  UINT64                            Revision;
  OPEN_VINTAGE_GET_PLATFORM_VERSION GetPlatformVersion;
};

extern EFI_GUID gOpenVintagePlatformProtocolGuid;

#endif
