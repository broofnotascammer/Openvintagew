/** @file
  OpenVintage Hardware Abstraction Layer DXE Driver Implementation.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OpenVintageLogLib.h>
#include <Library/OpenVintageCoreLib.h>
#include <Protocol/OpenVintageHal.h>
#include <Protocol/PciIo.h>
#include <IndustryStandard/Pci.h>

STATIC OPEN_VINTAGE_HAL_PROTOCOL  mOpenVintageHalProtocol;

EFI_STATUS
EFIAPI
OpenVintageHalGetCpuCapabilities (
  IN  OPEN_VINTAGE_HAL_PROTOCOL *This,
  OUT OV_CPU_CAPABILITIES       *CpuCaps
  )
{
  return OpenVintageDetectCpuArchitecture (CpuCaps);
}

EFI_STATUS
EFIAPI
OpenVintageHalGetGpuCapabilities (
  IN  OPEN_VINTAGE_HAL_PROTOCOL *This,
  OUT OV_GPU_CAPABILITIES       *GpuCaps
  )
{
  EFI_STATUS           Status;
  UINTN                HandleCount;
  EFI_HANDLE           *HandleBuffer;
  UINTN                Idx;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  PCI_TYPE00           PciConfig;

  if (GpuCaps == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (GpuCaps, sizeof (OV_GPU_CAPABILITIES));

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Idx = 0; Idx < HandleCount; Idx++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Idx],
                    &gEfiPciIoProtocolGuid,
                    (VOID **)&PciIo
                    );

    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = PciIo->Pci.Read (
                          PciIo,
                          EfiPciIoWidthUint32,
                          0,
                          sizeof (PciConfig) / sizeof (UINT32),
                          &PciConfig
                          );

    if (EFI_ERROR (Status)) {
      continue;
    }

    // Check PCI Class Code for Display Controller (Base Class 0x03)
    if (PciConfig.Hdr.ClassCode[2] == PCI_CLASS_DISPLAY) {
      GpuCaps->VendorId = PciConfig.Hdr.VendorId;
      GpuCaps->DeviceId = PciConfig.Hdr.DeviceId;

      if (GpuCaps->VendorId == 0x8086) {
        GpuCaps->Vendor = OvGpuVendorIntel;
        UnicodeSPrint (GpuCaps->ModelName, sizeof (GpuCaps->ModelName), L"Intel HD Graphics (PCI: 0x%04x)", GpuCaps->DeviceId);
        GpuCaps->MetalSupported = FALSE;
        GpuCaps->OpenGLCoreSupported = TRUE;
      } else if (GpuCaps->VendorId == 0x10DE) {
        GpuCaps->Vendor = OvGpuVendorNvidia;
        UnicodeSPrint (GpuCaps->ModelName, sizeof (GpuCaps->ModelName), L"Nvidia GeForce / Quadro (PCI: 0x%04x)", GpuCaps->DeviceId);
        GpuCaps->MetalSupported = TRUE;
        GpuCaps->OpenGLCoreSupported = TRUE;
      } else if (GpuCaps->VendorId == 0x1002) {
        GpuCaps->Vendor = OvGpuVendorAmd;
        UnicodeSPrint (GpuCaps->ModelName, sizeof (GpuCaps->ModelName), L"AMD Radeon Graphics (PCI: 0x%04x)", GpuCaps->DeviceId);
        GpuCaps->MetalSupported = FALSE;
        GpuCaps->OpenGLCoreSupported = TRUE;
      } else {
        GpuCaps->Vendor = OvGpuVendorUnknown;
        UnicodeSPrint (GpuCaps->ModelName, sizeof (GpuCaps->ModelName), L"Display Adapter (0x%04x:0x%04x)", GpuCaps->VendorId, GpuCaps->DeviceId);
      }
      break;
    }
  }

  FreePool (HandleBuffer);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OpenVintageHalDxeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  NewHandle;

  OpenVintageLog (OV_LOG_INFO, L"OpenVintage HAL DXE Driver starting...");

  mOpenVintageHalProtocol.Revision           = 0x00010000;
  mOpenVintageHalProtocol.GetCpuCapabilities = OpenVintageHalGetCpuCapabilities;
  mOpenVintageHalProtocol.GetGpuCapabilities = OpenVintageHalGetGpuCapabilities;

  NewHandle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &NewHandle,
                  &gOpenVintageHalProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mOpenVintageHalProtocol
                  );

  if (EFI_ERROR (Status)) {
    OpenVintageLog (OV_LOG_ERROR, L"Failed to install OpenVintage HAL Protocol: %r", Status);
    return Status;
  }

  OpenVintageLog (OV_LOG_INFO, L"OpenVintage HAL Protocol installed successfully.");
  return EFI_SUCCESS;
}
