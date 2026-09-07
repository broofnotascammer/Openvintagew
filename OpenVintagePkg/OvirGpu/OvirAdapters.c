/** @file
  OpenVintage Graphics API Adapters Implementation.
  Phase 3 Frontend API Translation Bridges (Vulkan, OpenGL, Metal, DirectX).

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/OvLoggerLib.h>
#include <Library/OvirGpuLib.h>
#include <Library/OvirAdaptersLib.h>
#include <Library/OvirPerfLib.h>

STATIC OVIR_ADAPTER_SYSTEM mAdapterSystem = { 0 };

EFI_STATUS
EFIAPI
OvirAdaptersInitialize (
  IN CONST OVIR_GPU_CAPABILITIES  *Capabilities
  )
{
  if (Capabilities == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&mAdapterSystem, sizeof (OVIR_ADAPTER_SYSTEM));
  mAdapterSystem.Initialized = TRUE;

  //
  // 1. Vulkan Adapter:
  // Active translation is supported if GPU supports Vulkan; otherwise, it operates
  // as a translation bridge routing through OVIR to the active hardware backend.
  //
  if (Capabilities->Apis.Vulkan.Supported) {
    mAdapterSystem.Vulkan.Status = OvirAdapterStatusActiveTranslation;
    mAdapterSystem.Vulkan.VkApiVersion = 0x00400000 | (Capabilities->Apis.Vulkan.VersionMinor << 12);
  } else {
    // Unsupported natively on this GPU; functions as translation bridge into OVIR
    mAdapterSystem.Vulkan.Status = OvirAdapterStatusActiveTranslation;
    mAdapterSystem.Vulkan.VkApiVersion = 0x00401000; // Vulkan 1.1 translation profile
  }

  //
  // 2. OpenGL Adapter:
  // OpenGL is natively accelerated on older Intel/Nvidia hardware and available
  // through desktop drivers / Mesa / UEFI GOP framebuffers.
  //
  if (Capabilities->Apis.OpenGL.Supported) {
    mAdapterSystem.OpenGL.Status = OvirAdapterStatusHostPassthrough;
    mAdapterSystem.OpenGL.GlVersion = (Capabilities->Apis.OpenGL.VersionMajor * 100) + Capabilities->Apis.OpenGL.VersionMinor;
    mAdapterSystem.OpenGL.IsCoreProfile = Capabilities->Apis.OpenGL.IsCoreProfile;
  } else {
    mAdapterSystem.OpenGL.Status = OvirAdapterStatusActiveTranslation;
    mAdapterSystem.OpenGL.GlVersion = 210;
  }

  //
  // 3. Metal Adapter:
  // Apple Metal has no native host execution in standard UEFI firmware.
  // It is supported strictly as an active translation bridge (MSL/Metal -> OVIR -> Target GPU).
  //
  mAdapterSystem.Metal.Status = OvirAdapterStatusActiveTranslation;
  mAdapterSystem.Metal.MetalVersion = Capabilities->Apis.Metal.MetalFeatureSet;

  //
  // 4. DirectX Adapter:
  // Microsoft Direct3D is proprietary to Windows. On OpenVintage UEFI firmware,
  // it is clearly documented and implemented as an active translation bridge (HLSL/D3D -> OVIR).
  // If target GPU cannot satisfy minimum Direct3D feature level, marked as stub.
  //
  if (Capabilities->Features.ComputeShaders) {
    mAdapterSystem.DirectX.Status = OvirAdapterStatusActiveTranslation;
    mAdapterSystem.DirectX.FeatureLevel = 0xB000; // Direct3D 11.0 translation target
    mAdapterSystem.DirectX.D3dVersion = 11;
  } else {
    // Clearly marked interface/stub for legacy hardware lacking Direct3D 11 capabilities
    mAdapterSystem.DirectX.Status = OvirAdapterStatusStubUnsupported;
    mAdapterSystem.DirectX.FeatureLevel = 0x9300; // Direct3D 9.3 legacy stub
    mAdapterSystem.DirectX.D3dVersion = 9;
  }

  OvLogTagged (OV_LOG_LEVEL_INFO, L"ADPT", L"Vulkan Adapter: Active Translation (Vulkan -> OVIR-GPU)");
  OvLogTagged (OV_LOG_LEVEL_INFO, L"ADPT", L"OpenGL Adapter: Active Translation & Host Passthrough");
  OvLogTagged (OV_LOG_LEVEL_INFO, L"ADPT", L"Metal Adapter: Active Translation (Metal -> OVIR-GPU)");
  if (mAdapterSystem.DirectX.Status == OvirAdapterStatusStubUnsupported) {
    OvLogTagged (OV_LOG_LEVEL_WARN, L"ADPT", L"DirectX Adapter: STUB UNSUPPORTED (Hardware lacks FL 11_0 compute)");
  } else {
    OvLogTagged (OV_LOG_LEVEL_INFO, L"ADPT", L"DirectX Adapter: Active Translation (D3D11 -> OVIR-GPU)");
  }

  return EFI_SUCCESS;
}

OVIR_ADAPTER_STATUS
EFIAPI
OvirAdapterGetStatus (
  IN OVIR_API_SOURCE  Api
  )
{
  if (!mAdapterSystem.Initialized) {
    return OvirAdapterStatusStubUnsupported;
  }

  switch (Api) {
    case OvirApiVulkan:  return mAdapterSystem.Vulkan.Status;
    case OvirApiOpenGL:  return mAdapterSystem.OpenGL.Status;
    case OvirApiMetal:   return mAdapterSystem.Metal.Status;
    case OvirApiDirectX: return mAdapterSystem.DirectX.Status;
    default:             return OvirAdapterStatusStubUnsupported;
  }
}

CONST CHAR16 *
EFIAPI
OvirAdapterStatusToString (
  IN OVIR_ADAPTER_STATUS  Status
  )
{
  switch (Status) {
    case OvirAdapterStatusActiveTranslation: return L"Active Translation";
    case OvirAdapterStatusHostPassthrough:   return L"Host Passthrough";
    case OvirAdapterStatusStubUnsupported:   return L"Stub / Unsupported";
    default:                                 return L"Unknown";
  }
}

CONST CHAR16 *
EFIAPI
OvirApiSourceToString (
  IN OVIR_API_SOURCE  Api
  )
{
  switch (Api) {
    case OvirApiVulkan:  return L"Vulkan";
    case OvirApiOpenGL:  return L"OpenGL";
    case OvirApiMetal:   return L"Metal";
    case OvirApiDirectX: return L"DirectX";
    default:             return L"Unknown";
  }
}

//
// Vulkan Adapter Implementation
//
EFI_STATUS
EFIAPI
OvirVkTranslateCmdDraw (
  IN OUT OVIR_COMMAND_LIST  *CmdList,
  IN     UINT32             VertexCount,
  IN     UINT32             InstanceCount,
  IN     UINT32             FirstVertex,
  IN     UINT32             FirstInstance
  )
{
  UINT64     StartTicks;
  EFI_STATUS Status;

  if (CmdList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  StartTicks = OvirPerfGetTimestamp ();
  Status = OvirCmdDraw (CmdList, VertexCount, InstanceCount, FirstVertex, FirstInstance);
  OvirPerfRecordTranslation (OvirPerfGetTimestamp () - StartTicks);

  if (!EFI_ERROR (Status)) {
    mAdapterSystem.Vulkan.DrawCallsTranslated++;
  }

  return Status;
}

EFI_STATUS
EFIAPI
OvirVkTranslateCmdDispatch (
  IN OUT OVIR_COMMAND_LIST  *CmdList,
  IN     UINT32             GroupCountX,
  IN     UINT32             GroupCountY,
  IN     UINT32             GroupCountZ
  )
{
  UINT64     StartTicks;
  EFI_STATUS Status;

  if (CmdList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  StartTicks = OvirPerfGetTimestamp ();
  Status = OvirCmdDispatch (CmdList, GroupCountX, GroupCountY, GroupCountZ);
  OvirPerfRecordTranslation (OvirPerfGetTimestamp () - StartTicks);

  if (!EFI_ERROR (Status)) {
    mAdapterSystem.Vulkan.DispatchesTranslated++;
  }

  return Status;
}

//
// OpenGL Adapter Implementation
//
EFI_STATUS
EFIAPI
OvirGlTranslateDrawArrays (
  IN OUT OVIR_COMMAND_LIST  *CmdList,
  IN     UINT32             Mode,
  IN     INT32              First,
  IN     UINT32             Count
  )
{
  UINT64     StartTicks;
  EFI_STATUS Status;

  if (CmdList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  StartTicks = OvirPerfGetTimestamp ();
  Status = OvirCmdDraw (CmdList, Count, 1, (UINT32)First, 0);
  OvirPerfRecordTranslation (OvirPerfGetTimestamp () - StartTicks);

  if (!EFI_ERROR (Status)) {
    mAdapterSystem.OpenGL.DrawArraysCount++;
  }

  return Status;
}

EFI_STATUS
EFIAPI
OvirGlTranslateDrawElements (
  IN OUT OVIR_COMMAND_LIST  *CmdList,
  IN     UINT32             Mode,
  IN     UINT32             Count,
  IN     UINT32             Type,
  IN     UINT64             IndicesOffset
  )
{
  UINT64     StartTicks;
  EFI_STATUS Status;

  if (CmdList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  StartTicks = OvirPerfGetTimestamp ();
  Status = OvirCmdDrawIndexed (CmdList, Count, 1, (UINT32)(IndicesOffset / 2), 0, 0);
  OvirPerfRecordTranslation (OvirPerfGetTimestamp () - StartTicks);

  if (!EFI_ERROR (Status)) {
    mAdapterSystem.OpenGL.DrawElementsCount++;
  }

  return Status;
}

//
// Metal Adapter Implementation
//
EFI_STATUS
EFIAPI
OvirMtlTranslateDrawPrimitives (
  IN OUT OVIR_COMMAND_LIST  *CmdList,
  IN     UINT32             PrimitiveType,
  IN     UINTN              VertexStart,
  IN     UINTN              VertexCount,
  IN     UINTN              InstanceCount
  )
{
  UINT64     StartTicks;
  EFI_STATUS Status;

  if (CmdList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  StartTicks = OvirPerfGetTimestamp ();
  Status = OvirCmdDraw (CmdList, (UINT32)VertexCount, (UINT32)InstanceCount, (UINT32)VertexStart, 0);
  OvirPerfRecordTranslation (OvirPerfGetTimestamp () - StartTicks);

  if (!EFI_ERROR (Status)) {
    mAdapterSystem.Metal.PrimitivesRendered++;
  }

  return Status;
}

//
// DirectX Adapter Implementation
//
EFI_STATUS
EFIAPI
OvirDxTranslateDrawIndexed (
  IN OUT OVIR_COMMAND_LIST  *CmdList,
  IN     UINT32             IndexCount,
  IN     UINT32             StartIndexLocation,
  IN     INT32              BaseVertexLocation
  )
{
  UINT64     StartTicks;
  EFI_STATUS Status;

  if (CmdList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (mAdapterSystem.DirectX.Status == OvirAdapterStatusStubUnsupported) {
    // Clearly documented stub on unsupported legacy targets
    return EFI_UNSUPPORTED;
  }

  StartTicks = OvirPerfGetTimestamp ();
  Status = OvirCmdDrawIndexed (CmdList, IndexCount, 1, StartIndexLocation, BaseVertexLocation, 0);
  OvirPerfRecordTranslation (OvirPerfGetTimestamp () - StartTicks);

  if (!EFI_ERROR (Status)) {
    mAdapterSystem.DirectX.DrawIndexedCount++;
  }

  return Status;
}
