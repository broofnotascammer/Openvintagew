/** @file
  OpenVintage Hardware Discovery and Topology Abstraction Implementation.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/PciLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/OvLoggerLib.h>
#include <Library/OvHardwareLib.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/BlockIo.h>
#include <IndustryStandard/Pci.h>

STATIC BOOLEAN  mHardwareInitialized = FALSE;

EFI_STATUS
EFIAPI
OvHardwareInitialize (
  VOID
  )
{
  mHardwareInitialized = TRUE;
  OvLogTagged (OV_LOG_LEVEL_DEBUG, L"HW", L"Hardware abstraction layer initialized");
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvHardwareGetCpu (
  OUT OV_CPU_TOPOLOGY  *CpuInfo
  )
{
  UINT32  RegEax;
  UINT32  RegEbx;
  UINT32  RegEcx;
  UINT32  RegEdx;
  UINT32  MaxExtId;
  UINT32  *BrandPtr;

  if (CpuInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (CpuInfo, sizeof (OV_CPU_TOPOLOGY));

  // 1. CPUID Leaf 0: Vendor String
  AsmCpuid (0x00000000, &RegEax, &RegEbx, &RegEcx, &RegEdx);
  *(UINT32 *)(&CpuInfo->VendorId[0]) = RegEbx;
  *(UINT32 *)(&CpuInfo->VendorId[4]) = RegEdx;
  *(UINT32 *)(&CpuInfo->VendorId[8]) = RegEcx;
  CpuInfo->VendorId[12] = '\0';

  // 2. CPUID Leaf 1: Family, Model, Stepping, Features
  AsmCpuid (0x00000001, &RegEax, &RegEbx, &RegEcx, &RegEdx);

  CpuInfo->Stepping = RegEax & 0xF;
  CpuInfo->Family   = (RegEax >> 8) & 0xF;
  CpuInfo->Model    = (RegEax >> 4) & 0xF;

  if (CpuInfo->Family == 15) {
    CpuInfo->Family += (RegEax >> 20) & 0xFF;
  }
  if ((CpuInfo->Family == 6) || (CpuInfo->Family == 15)) {
    CpuInfo->Model += ((RegEax >> 16) & 0xF) << 4;
  }

  CpuInfo->LogicalCores = (RegEbx >> 16) & 0xFF;
  if (CpuInfo->LogicalCores == 0) {
    CpuInfo->LogicalCores = 1;
  }
  CpuInfo->PhysicalCores = CpuInfo->LogicalCores; // Approximation without APIC enumeration

  // Instruction flags
  CpuInfo->HasSSE    = (RegEdx & BIT25) != 0;
  CpuInfo->HasSSE2   = (RegEdx & BIT26) != 0;
  CpuInfo->HasSSE3   = (RegEcx & BIT0) != 0;
  CpuInfo->HasSSSE3  = (RegEcx & BIT9) != 0;
  CpuInfo->HasSSE41  = (RegEcx & BIT19) != 0;
  CpuInfo->HasSSE42  = (RegEcx & BIT20) != 0;
  CpuInfo->HasAESNI  = (RegEcx & BIT25) != 0;
  CpuInfo->HasAVX    = (RegEcx & BIT28) != 0;
  CpuInfo->HasRDRAND = (RegEcx & BIT30) != 0;
  CpuInfo->HasFMA    = (RegEcx & BIT12) != 0;

  // 3. CPUID Leaf 7 Subleaf 0: Extended Features
  AsmCpuidEx (0x00000007, 0x00000000, &RegEax, &RegEbx, &RegEcx, &RegEdx);
  CpuInfo->HasAVX2 = (RegEbx & BIT5) != 0;
  CpuInfo->HasBMI1 = (RegEbx & BIT3) != 0;
  CpuInfo->HasBMI2 = (RegEbx & BIT8) != 0;

  // 4. Extended CPUID: Brand String
  AsmCpuid (0x80000000, &MaxExtId, &RegEbx, &RegEcx, &RegEdx);
  if (MaxExtId >= 0x80000004) {
    BrandPtr = (UINT32 *)CpuInfo->BrandString;
    AsmCpuid (0x80000002, &BrandPtr[0], &BrandPtr[1], &BrandPtr[2], &BrandPtr[3]);
    AsmCpuid (0x80000003, &BrandPtr[4], &BrandPtr[5], &BrandPtr[6], &BrandPtr[7]);
    AsmCpuid (0x80000004, &BrandPtr[8], &BrandPtr[9], &BrandPtr[10], &BrandPtr[11]);
    CpuInfo->BrandString[48] = '\0';
  } else {
    AsciiStrCpyS (CpuInfo->BrandString, sizeof (CpuInfo->BrandString), "Generic x86_64 CPU");
  }

  // 5. Silicon Architecture Classification
  if (CpuInfo->Family == 0x06) {
    switch (CpuInfo->Model) {
      case 0x0F:
      case 0x16:
      case 0x17:
      case 0x1D:
        CpuInfo->Architecture = OvCpuArchCore2;
        break;
      case 0x1A:
      case 0x1E:
      case 0x1F:
      case 0x25:
      case 0x2C:
      case 0x2E:
      case 0x2F:
        CpuInfo->Architecture = OvCpuArchNehalemWestmere;
        break;
      case 0x2A:
      case 0x2D:
        CpuInfo->Architecture = OvCpuArchSandyBridge;
        break;
      case 0x3A:
      case 0x3E:
        CpuInfo->Architecture = OvCpuArchIvyBridge;
        break;
      case 0x3C:
      case 0x3F:
      case 0x45:
      case 0x46:
      case 0x3D:
      case 0x47:
      case 0x4F:
      case 0x56:
        CpuInfo->Architecture = OvCpuArchHaswellBroadwell;
        break;
      default:
        CpuInfo->Architecture = OvCpuArchUnknown;
        break;
    }
  } else {
    CpuInfo->Architecture = OvCpuArchUnknown;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvHardwareGetMemory (
  OUT OV_MEMORY_TOPOLOGY  *MemInfo
  )
{
  EFI_STATUS             Status;
  UINTN                  MapKey;
  UINTN                  DescriptorSize;
  UINT32                 DescriptorVersion;
  UINTN                  MemoryMapSize;
  EFI_MEMORY_DESCRIPTOR  *MemoryMap;
  EFI_MEMORY_DESCRIPTOR  *Desc;
  UINTN                  Index;
  UINT64                 RegionBytes;

  if (MemInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (MemInfo, sizeof (OV_MEMORY_TOPOLOGY));

  MemoryMapSize = 0;
  Status = gBS->GetMemoryMap (
                  &MemoryMapSize,
                  NULL,
                  &MapKey,
                  &DescriptorSize,
                  &DescriptorVersion
                  );
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return Status;
  }

  // Add buffer safety padding
  MemoryMapSize += (16 * DescriptorSize);
  MemoryMap = AllocatePool (MemoryMapSize);
  if (MemoryMap == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->GetMemoryMap (
                  &MemoryMapSize,
                  MemoryMap,
                  &MapKey,
                  &DescriptorSize,
                  &DescriptorVersion
                  );
  if (EFI_ERROR (Status)) {
    FreePool (MemoryMap);
    return Status;
  }

  MemInfo->MemoryMapDescriptorCount = MemoryMapSize / DescriptorSize;

  for (Index = 0; Index < MemInfo->MemoryMapDescriptorCount; Index++) {
    Desc = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + (Index * DescriptorSize));
    RegionBytes = MultU64x32 (Desc->NumberOfPages, EFI_PAGE_SIZE);

    switch (Desc->Type) {
      case EfiConventionalMemory:
        MemInfo->AvailableConventionalBytes += RegionBytes;
        MemInfo->TotalPhysicalBytes += RegionBytes;
        break;
      case EfiBootServicesCode:
      case EfiBootServicesData:
      case EfiRuntimeServicesCode:
      case EfiRuntimeServicesData:
      case EfiReservedMemoryType:
      case EfiMemoryMappedIO:
      case EfiMemoryMappedIOPortSpace:
        MemInfo->ReservedBytes += RegionBytes;
        MemInfo->TotalPhysicalBytes += RegionBytes;
        break;
      case EfiACPIReclaimMemory:
      case EfiACPIMemoryNVS:
        MemInfo->AcpiReclaimBytes += RegionBytes;
        MemInfo->TotalPhysicalBytes += RegionBytes;
        break;
      default:
        break;
    }
  }

  FreePool (MemoryMap);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvHardwareGetGpu (
  OUT OV_GPU_TOPOLOGY  *GpuInfo
  )
{
  EFI_STATUS                    Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *Gop;
  UINT8                         Bus;
  UINT8                         Device;
  UINT8                         Function;
  UINT32                        PciId;
  UINT16                        VendorId;
  UINT16                        DeviceId;
  UINT8                         ClassCode;

  if (GpuInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (GpuInfo, sizeof (OV_GPU_TOPOLOGY));

  // 1. Query Graphics Output Protocol (GOP)
  Status = gBS->LocateProtocol (
                  &gEfiGraphicsOutputProtocolGuid,
                  NULL,
                  (VOID **)&Gop
                  );
  if (!EFI_ERROR (Status) && (Gop != NULL) && (Gop->Mode != NULL) && (Gop->Mode->Info != NULL)) {
    GpuInfo->GopPresent           = TRUE;
    GpuInfo->HorizontalResolution = Gop->Mode->Info->HorizontalResolution;
    GpuInfo->VerticalResolution   = Gop->Mode->Info->VerticalResolution;
    GpuInfo->PixelsPerScanLine    = Gop->Mode->Info->PixelsPerScanLine;
    GpuInfo->PixelFormat          = (UINT32)Gop->Mode->Info->PixelFormat;
    GpuInfo->FrameBufferBase      = Gop->Mode->FrameBufferBase;
    GpuInfo->FrameBufferSize      = Gop->Mode->FrameBufferSize;
  }

  // 2. Discover PCI Display Controller
  for (Bus = 0; Bus <= 1; Bus++) {
    for (Device = 0; Device < 32; Device++) {
      for (Function = 0; Function < 8; Function++) {
        PciId = PciRead32 (PCI_LIB_ADDRESS (Bus, Device, Function, 0x00));
        VendorId = (UINT16)(PciId & 0xFFFF);
        DeviceId = (UINT16)((PciId >> 16) & 0xFFFF);

        if ((VendorId == 0xFFFF) || (VendorId == 0x0000)) {
          if (Function == 0) {
            break;
          }
          continue;
        }

        ClassCode = PciRead8 (PCI_LIB_ADDRESS (Bus, Device, Function, 0x0B));

        // Class 0x03 is Display Controller
        if (ClassCode == 0x03) {
          GpuInfo->VendorId   = VendorId;
          GpuInfo->DeviceId   = DeviceId;
          GpuInfo->RevisionId = PciRead8 (PCI_LIB_ADDRESS (Bus, Device, Function, 0x08));

          switch (VendorId) {
            case 0x8086:
              GpuInfo->Vendor = OvGpuVendorIntel;
              UnicodeSPrint (GpuInfo->AdapterName, sizeof (GpuInfo->AdapterName), L"Intel HD / Iris Graphics (PCI %02X:%02X.%X)", Bus, Device, Function);
              break;
            case 0x10DE:
              GpuInfo->Vendor = OvGpuVendorNvidia;
              UnicodeSPrint (GpuInfo->AdapterName, sizeof (GpuInfo->AdapterName), L"Nvidia GeForce / Quadro (PCI %02X:%02X.%X)", Bus, Device, Function);
              break;
            case 0x1002:
              GpuInfo->Vendor = OvGpuVendorAmd;
              UnicodeSPrint (GpuInfo->AdapterName, sizeof (GpuInfo->AdapterName), L"AMD Radeon Graphics (PCI %02X:%02X.%X)", Bus, Device, Function);
              break;
            case 0x1234:
              GpuInfo->Vendor = OvGpuVendorUnknown;
              UnicodeSPrint (GpuInfo->AdapterName, sizeof (GpuInfo->AdapterName), L"QEMU/Bochs Standard VGA (PCI %02X:%02X.%X)", Bus, Device, Function);
              break;
            default:
              GpuInfo->Vendor = OvGpuVendorUnknown;
              UnicodeSPrint (GpuInfo->AdapterName, sizeof (GpuInfo->AdapterName), L"PCI Display Controller (Vendor: 0x%04X, Dev: 0x%04X)", VendorId, DeviceId);
              break;
          }
          return EFI_SUCCESS;
        }
      }
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvHardwareGetStorage (
  OUT OV_STORAGE_TOPOLOGY  *StorageInfo
  )
{
  EFI_STATUS             Status;
  UINTN                  HandleCount;
  EFI_HANDLE             *HandleBuffer;
  UINTN                  Index;
  EFI_BLOCK_IO_PROTOCOL  *BlockIo;

  if (StorageInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (StorageInfo, sizeof (OV_STORAGE_TOPOLOGY));

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiBlockIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; (Index < HandleCount) && (StorageInfo->DeviceCount < OV_MAX_STORAGE_DEVICES); Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiBlockIoProtocolGuid,
                    (VOID **)&BlockIo
                    );
    if (!EFI_ERROR (Status) && (BlockIo != NULL) && (BlockIo->Media != NULL)) {
      OV_STORAGE_DEVICE *Dev = &StorageInfo->Devices[StorageInfo->DeviceCount];
      Dev->Index        = (UINT32)Index;
      Dev->MediaPresent = BlockIo->Media->MediaPresent;
      Dev->Removable    = BlockIo->Media->RemovableMedia;
      Dev->ReadOnly     = BlockIo->Media->ReadOnly;
      Dev->BlockSize    = BlockIo->Media->BlockSize;
      Dev->TotalBlockCount = BlockIo->Media->LastBlock + 1;
      Dev->CapacityBytes   = MultU64x32 (Dev->TotalBlockCount, Dev->BlockSize);

      StorageInfo->TotalCapacityBytes += Dev->CapacityBytes;
      StorageInfo->DeviceCount++;
    }
  }

  FreePool (HandleBuffer);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvHardwareGetPci (
  OUT OV_PCI_TOPOLOGY  *PciInfo
  )
{
  UINT8   Bus;
  UINT8   Device;
  UINT8   Function;
  UINT32  PciId;
  UINT16  VendorId;
  UINT16  DeviceId;

  if (PciInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (PciInfo, sizeof (OV_PCI_TOPOLOGY));

  for (Bus = 0; Bus <= 1; Bus++) {
    for (Device = 0; Device < 32; Device++) {
      for (Function = 0; Function < 8; Function++) {
        PciId = PciRead32 (PCI_LIB_ADDRESS (Bus, Device, Function, 0x00));
        VendorId = (UINT16)(PciId & 0xFFFF);
        DeviceId = (UINT16)((PciId >> 16) & 0xFFFF);

        if ((VendorId == 0xFFFF) || (VendorId == 0x0000)) {
          if (Function == 0) {
            break;
          }
          continue;
        }

        if (PciInfo->Count < OV_MAX_PCI_DEVICES) {
          OV_PCI_INFO *PciDev = &PciInfo->Devices[PciInfo->Count];
          PciDev->Bus          = Bus;
          PciDev->Device       = Device;
          PciDev->Function     = Function;
          PciDev->VendorId     = VendorId;
          PciDev->DeviceId     = DeviceId;
          PciDev->ClassCode    = PciRead8 (PCI_LIB_ADDRESS (Bus, Device, Function, 0x0B));
          PciDev->SubClassCode = PciRead8 (PCI_LIB_ADDRESS (Bus, Device, Function, 0x0A));
          PciInfo->Count++;
        }
      }
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvHardwareGetPlatform (
  OUT OV_PLATFORM_TOPOLOGY  *PlatformInfo
  )
{
  EFI_STATUS  Status;
  UINTN       HandleCount;
  EFI_HANDLE  *HandleBuffer;

  if (PlatformInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (PlatformInfo, sizeof (OV_PLATFORM_TOPOLOGY));

  if (gST != NULL) {
    PlatformInfo->FirmwareVendor    = gST->FirmwareVendor;
    PlatformInfo->FirmwareRevision  = gST->FirmwareRevision;
    PlatformInfo->UefiSpecification = gST->Hdr.Revision;
    PlatformInfo->SystemTable       = gST;
  }

  Status = gBS->LocateHandleBuffer (
                  AllHandles,
                  NULL,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (!EFI_ERROR (Status)) {
    PlatformInfo->TotalActiveHandles = HandleCount;
    FreePool (HandleBuffer);
  }

  return EFI_SUCCESS;
}

VOID
EFIAPI
OvHardwareDumpTopology (
  VOID
  )
{
  OV_CPU_TOPOLOGY       Cpu;
  OV_MEMORY_TOPOLOGY    Mem;
  OV_GPU_TOPOLOGY       Gpu;
  OV_STORAGE_TOPOLOGY   Storage;
  OV_PLATFORM_TOPOLOGY  Platform;

  OvHardwareGetCpu (&Cpu);
  OvHardwareGetMemory (&Mem);
  OvHardwareGetGpu (&Gpu);
  OvHardwareGetStorage (&Storage);
  OvHardwareGetPlatform (&Platform);

  OvLogTagged (OV_LOG_LEVEL_INFO, L"HW", L"--- HARDWARE TOPOLOGY REPORT ---");
  OvLogTagged (OV_LOG_LEVEL_INFO, L"HW", L"CPU Silicon  : %a (Family 0x%02X, Model 0x%02X, Stepping 0x%02X)", Cpu.BrandString, Cpu.Family, Cpu.Model, Cpu.Stepping);
  OvLogTagged (OV_LOG_LEVEL_INFO, L"HW", L"CPU Vector   : SSE4.1[%s] SSE4.2[%s] AVX[%s] AVX2[%s] AES-NI[%s]",
    Cpu.HasSSE41 ? L"YES" : L"NO", Cpu.HasSSE42 ? L"YES" : L"NO",
    Cpu.HasAVX ? L"YES" : L"NO", Cpu.HasAVX2 ? L"YES" : L"NO",
    Cpu.HasAESNI ? L"YES" : L"NO"
    );
  OvLogTagged (OV_LOG_LEVEL_INFO, L"HW", L"System RAM   : Total %lu MB | Free %lu MB | Reserved %lu MB",
    Mem.TotalPhysicalBytes / (1024 * 1024),
    Mem.AvailableConventionalBytes / (1024 * 1024),
    Mem.ReservedBytes / (1024 * 1024)
    );
  OvLogTagged (OV_LOG_LEVEL_INFO, L"HW", L"Display GPU  : %s (GOP: %ux%u)", Gpu.AdapterName, Gpu.HorizontalResolution, Gpu.VerticalResolution);
  OvLogTagged (OV_LOG_LEVEL_INFO, L"HW", L"Storage Disks: %u device(s) discovered, total %lu MB capacity",
    (UINT32)Storage.DeviceCount, Storage.TotalCapacityBytes / (1024 * 1024));
  OvLogTagged (OV_LOG_LEVEL_INFO, L"HW", L"Firmware/UEFI: %s (Rev: 0x%08X, Spec: %u.%02u, Active Handles: %u)",
    Platform.FirmwareVendor, Platform.FirmwareRevision,
    (Platform.UefiSpecification >> 16) & 0xFFFF,
    Platform.UefiSpecification & 0xFFFF,
    (UINT32)Platform.TotalActiveHandles
    );
}
