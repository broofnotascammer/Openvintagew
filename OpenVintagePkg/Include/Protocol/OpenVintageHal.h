/** @file
  OpenVintage Hardware Abstraction Layer Protocol Definition.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OPEN_VINTAGE_HAL_H_
#define OPEN_VINTAGE_HAL_H_

#define OPEN_VINTAGE_HAL_PROTOCOL_GUID \
  { 0x1F744E82, 0x7462, 0x4F49, { 0x82, 0x90, 0x3D, 0x49, 0xA7, 0x01, 0x83, 0x21 } }

typedef struct _OPEN_VINTAGE_HAL_PROTOCOL OPEN_VINTAGE_HAL_PROTOCOL;

typedef enum {
  OvCpuArchUnknown = 0,
  OvCpuArchCore2,
  OvCpuArchNehalemWestmere,
  OvCpuArchSandyBridge,
  OvCpuArchIvyBridge,
  OvCpuArchHaswellBroadwell
} OV_CPU_ARCHITECTURE;

typedef enum {
  OvGpuVendorUnknown = 0,
  OvGpuVendorIntel,
  OvGpuVendorNvidia,
  OvGpuVendorAmd
} OV_GPU_VENDOR;

typedef struct {
  OV_CPU_ARCHITECTURE CpuArchitecture;
  CHAR8               CpuBrandString[49];
  UINT32              Family;
  UINT32              Model;
  UINT32              Stepping;
  BOOLEAN             HasSSE41;
  BOOLEAN             HasSSE42;
  BOOLEAN             HasAVX;
  BOOLEAN             HasAVX2;
  BOOLEAN             HasAESNI;
} OV_CPU_CAPABILITIES;

typedef struct {
  OV_GPU_VENDOR       Vendor;
  UINT16              VendorId;
  UINT16              DeviceId;
  UINT16              SubsystemId;
  CHAR16              ModelName[64];
  UINT64              VramBytes;
  BOOLEAN             MetalSupported;
  BOOLEAN             OpenGLCoreSupported;
} OV_GPU_CAPABILITIES;

typedef
EFI_STATUS
(EFIAPI *OPEN_VINTAGE_HAL_GET_CPU_CAPABILITIES)(
  IN  OPEN_VINTAGE_HAL_PROTOCOL *This,
  OUT OV_CPU_CAPABILITIES       *CpuCaps
  );

typedef
EFI_STATUS
(EFIAPI *OPEN_VINTAGE_HAL_GET_GPU_CAPABILITIES)(
  IN  OPEN_VINTAGE_HAL_PROTOCOL *This,
  OUT OV_GPU_CAPABILITIES       *GpuCaps
  );

struct _OPEN_VINTAGE_HAL_PROTOCOL {
  UINT64                                Revision;
  OPEN_VINTAGE_HAL_GET_CPU_CAPABILITIES GetCpuCapabilities;
  OPEN_VINTAGE_HAL_GET_GPU_CAPABILITIES GetGpuCapabilities;
};

extern EFI_GUID gOpenVintageHalProtocolGuid;

#endif
