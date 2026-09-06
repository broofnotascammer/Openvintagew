/** @file
  OpenVintage Hardware Discovery and Topology Abstraction.
  Phase 2 Hardware Detection Layer.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OV_HARDWARE_LIB_H_
#define OV_HARDWARE_LIB_H_

#include <Uefi.h>
#include <Protocol/OpenVintageHal.h>

//
// Maximum discoverable items
//
#define OV_MAX_PCI_DEVICES      32
#define OV_MAX_STORAGE_DEVICES  16

//
// Detailed CPU Topology and Feature Set
//
typedef struct {
  OV_CPU_ARCHITECTURE Architecture;
  CHAR8               VendorId[13];
  CHAR8               BrandString[49];
  UINT32              Family;
  UINT32              Model;
  UINT32              Stepping;
  UINT32              LogicalCores;
  UINT32              PhysicalCores;
  BOOLEAN             HasSSE;
  BOOLEAN             HasSSE2;
  BOOLEAN             HasSSE3;
  BOOLEAN             HasSSSE3;
  BOOLEAN             HasSSE41;
  BOOLEAN             HasSSE42;
  BOOLEAN             HasAVX;
  BOOLEAN             HasAVX2;
  BOOLEAN             HasFMA;
  BOOLEAN             HasAESNI;
  BOOLEAN             HasRDRAND;
  BOOLEAN             HasBMI1;
  BOOLEAN             HasBMI2;
} OV_CPU_TOPOLOGY;

//
// Physical Memory Topology
//
typedef struct {
  UINT64  TotalPhysicalBytes;
  UINT64  AvailableConventionalBytes;
  UINT64  ReservedBytes;
  UINT64  AcpiReclaimBytes;
  UINTN   MemoryMapDescriptorCount;
} OV_MEMORY_TOPOLOGY;

//
// Graphics Display Controller Info
//
typedef struct {
  OV_GPU_VENDOR Vendor;
  UINT16        VendorId;
  UINT16        DeviceId;
  UINT16        SubsystemId;
  UINT8         RevisionId;
  CHAR16        AdapterName[64];
  BOOLEAN       GopPresent;
  UINT32        HorizontalResolution;
  UINT32        VerticalResolution;
  UINT32        PixelsPerScanLine;
  UINT32        PixelFormat;
  UINT64        FrameBufferBase;
  UINT64        FrameBufferSize;
} OV_GPU_TOPOLOGY;

//
// Storage Device Info
//
typedef struct {
  UINT32  Index;
  BOOLEAN MediaPresent;
  BOOLEAN Removable;
  BOOLEAN ReadOnly;
  UINT32  BlockSize;
  UINT64  TotalBlockCount;
  UINT64  CapacityBytes;
} OV_STORAGE_DEVICE;

typedef struct {
  UINTN             DeviceCount;
  UINT64            TotalCapacityBytes;
  OV_STORAGE_DEVICE Devices[OV_MAX_STORAGE_DEVICES];
} OV_STORAGE_TOPOLOGY;

//
// Discovered PCI Device
//
typedef struct {
  UINT8   Bus;
  UINT8   Device;
  UINT8   Function;
  UINT16  VendorId;
  UINT16  DeviceId;
  UINT8   ClassCode;
  UINT8   SubClassCode;
} OV_PCI_INFO;

typedef struct {
  UINTN       Count;
  OV_PCI_INFO Devices[OV_MAX_PCI_DEVICES];
} OV_PCI_TOPOLOGY;

//
// Platform Firmware Info
//
typedef struct {
  CHAR16  *FirmwareVendor;
  UINT32  FirmwareRevision;
  UINT32  UefiSpecification;
  UINTN   TotalActiveHandles;
  EFI_HANDLE ImageHandle;
  EFI_SYSTEM_TABLE *SystemTable;
} OV_PLATFORM_TOPOLOGY;

/**
  Initialize hardware discovery layer.

  @retval EFI_SUCCESS  Hardware discovery layer initialized.
**/
EFI_STATUS
EFIAPI
OvHardwareInitialize (
  VOID
  );

/**
  Retrieve CPU topology and instruction capabilities.

  @param[out] CpuInfo  Receives CPU topology.

  @retval EFI_SUCCESS  CPU info populated.
**/
EFI_STATUS
EFIAPI
OvHardwareGetCpu (
  OUT OV_CPU_TOPOLOGY  *CpuInfo
  );

/**
  Retrieve Physical Memory layout and conventional RAM availability.

  @param[out] MemInfo  Receives memory topology.

  @retval EFI_SUCCESS  Memory topology populated.
**/
EFI_STATUS
EFIAPI
OvHardwareGetMemory (
  OUT OV_MEMORY_TOPOLOGY  *MemInfo
  );

/**
  Retrieve primary GPU display controller and GOP state.

  @param[out] GpuInfo  Receives GPU topology.

  @retval EFI_SUCCESS  GPU topology populated.
**/
EFI_STATUS
EFIAPI
OvHardwareGetGpu (
  OUT OV_GPU_TOPOLOGY  *GpuInfo
  );

/**
  Retrieve storage device enumeration.

  @param[out] StorageInfo  Receives storage topology.

  @retval EFI_SUCCESS      Storage topology populated.
**/
EFI_STATUS
EFIAPI
OvHardwareGetStorage (
  OUT OV_STORAGE_TOPOLOGY  *StorageInfo
  );

/**
  Retrieve PCI bus device topology.

  @param[out] PciInfo  Receives PCI bus topology.

  @retval EFI_SUCCESS  PCI topology populated.
**/
EFI_STATUS
EFIAPI
OvHardwareGetPci (
  OUT OV_PCI_TOPOLOGY  *PciInfo
  );

/**
  Retrieve UEFI platform and firmware information.

  @param[out] PlatformInfo  Receives platform topology.

  @retval EFI_SUCCESS       Platform topology populated.
**/
EFI_STATUS
EFIAPI
OvHardwareGetPlatform (
  OUT OV_PLATFORM_TOPOLOGY  *PlatformInfo
  );

/**
  Dump summary of discovered hardware to logger.
**/
VOID
EFIAPI
OvHardwareDumpTopology (
  VOID
  );

#endif // OV_HARDWARE_LIB_H_
