/** @file
  OpenVintage Core Library Definition.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OPEN_VINTAGE_CORE_LIB_H_
#define OPEN_VINTAGE_CORE_LIB_H_

#include <Uefi.h>
#include <Protocol/OpenVintageHal.h>

EFI_STATUS
EFIAPI
OpenVintageInitializeRuntime (
  VOID
  );

VOID
EFIAPI
OpenVintagePrintBanner (
  VOID
  );

EFI_STATUS
EFIAPI
OpenVintageGetPlatformInfo (
  OUT CHAR16  **FirmwareVendor,
  OUT UINT32  *FirmwareRevision,
  OUT UINT32  *UefiRevision
  );

EFI_STATUS
EFIAPI
OpenVintageDetectCpuArchitecture (
  OUT OV_CPU_CAPABILITIES  *CpuCaps
  );

EFI_STATUS
EFIAPI
OpenVintageReportMemoryMap (
  OUT UINT64  *TotalPhysicalBytes,
  OUT UINT64  *AvailableConventionalBytes
  );

EFI_STATUS
EFIAPI
OpenVintageEnumerateDevices (
  OUT UINTN   *TotalHandleCount,
  OUT UINTN   *PciDeviceCount,
  OUT UINTN   *BlockIoDeviceCount
  );

#endif
