/** @file
  OpenVintage Graphics API Adapters Definition.
  Phase 3 Frontend API Translation Bridges (Vulkan, OpenGL, Metal, DirectX).

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OVIR_ADAPTERS_LIB_H_
#define OVIR_ADAPTERS_LIB_H_

#include <Uefi.h>
#include <Library/OvirGpuLib.h>
#include <Library/OvGpuCapabilityLib.h>

//
// Source Graphics API Identification
//
typedef enum {
  OvirApiVulkan = 1,
  OvirApiOpenGL = 2,
  OvirApiMetal = 3,
  OvirApiDirectX = 4
} OVIR_API_SOURCE;

//
// Adapter Implementation & Runtime Status
//
typedef enum {
  OvirAdapterStatusActiveTranslation = 1,  // Fully functional translation layer to OVIR
  OvirAdapterStatusHostPassthrough = 2,    // Backend has native host support
  OvirAdapterStatusStubUnsupported = 3     // Clearly marked interface/stub for unsupported platform targets
} OVIR_ADAPTER_STATUS;

//
// Vulkan Translation Types & Emulation State
//
typedef struct {
  OVIR_ADAPTER_STATUS Status;
  UINT32              VkApiVersion;
  BOOLEAN             ValidationLayersEnabled;
  UINTN               DrawCallsTranslated;
  UINTN               DispatchesTranslated;
} OVIR_VK_ADAPTER;

//
// OpenGL Translation Types & State
//
typedef struct {
  OVIR_ADAPTER_STATUS Status;
  UINT32              GlVersion;
  BOOLEAN             IsCoreProfile;
  UINT32              CurrentProgram;
  UINT32              CurrentVao;
  UINTN               DrawArraysCount;
  UINTN               DrawElementsCount;
} OVIR_GL_ADAPTER;

//
// Metal Translation Types & State
//
typedef struct {
  OVIR_ADAPTER_STATUS Status;
  UINT32              MetalVersion;
  BOOLEAN             ArgumentBuffersSupported;
  UINTN               PrimitivesRendered;
  UINTN               ComputeEncodersActive;
} OVIR_MTL_ADAPTER;

//
// DirectX Translation Types & State
//
typedef struct {
  OVIR_ADAPTER_STATUS Status;
  UINT32              FeatureLevel;
  UINT32              D3dVersion; // 11 or 12
  UINTN               DrawIndexedCount;
  UINTN               CommandListsRecorded;
} OVIR_DX_ADAPTER;

//
// Consolidated API Adapter Context
//
typedef struct {
  BOOLEAN           Initialized;
  OVIR_VK_ADAPTER   Vulkan;
  OVIR_GL_ADAPTER   OpenGL;
  OVIR_MTL_ADAPTER  Metal;
  OVIR_DX_ADAPTER   DirectX;
} OVIR_ADAPTER_SYSTEM;

/**
  Initialize all graphics API adapters against detected GPU capabilities.

  @param[in] Capabilities   Pointer to active GPU capability profile.

  @retval EFI_SUCCESS       Adapters configured.
**/
EFI_STATUS
EFIAPI
OvirAdaptersInitialize (
  IN CONST OVIR_GPU_CAPABILITIES  *Capabilities
  );

/**
  Get status of an API adapter on current platform.

  @param[in] Api   API source type.

  @return Status enum.
**/
OVIR_ADAPTER_STATUS
EFIAPI
OvirAdapterGetStatus (
  IN OVIR_API_SOURCE  Api
  );

CONST CHAR16 *
EFIAPI
OvirAdapterStatusToString (
  IN OVIR_ADAPTER_STATUS Status
  );

CONST CHAR16 *
EFIAPI
OvirApiSourceToString (
  IN OVIR_API_SOURCE  Api
  );

//
// Vulkan Adapter Translation Interface
//
EFI_STATUS
EFIAPI
OvirVkTranslateCmdDraw (
  IN OUT OVIR_COMMAND_LIST  *CmdList,
  IN     UINT32             VertexCount,
  IN     UINT32             InstanceCount,
  IN     UINT32             FirstVertex,
  IN     UINT32             FirstInstance
  );

EFI_STATUS
EFIAPI
OvirVkTranslateCmdDispatch (
  IN OUT OVIR_COMMAND_LIST  *CmdList,
  IN     UINT32             GroupCountX,
  IN     UINT32             GroupCountY,
  IN     UINT32             GroupCountZ
  );

//
// OpenGL Adapter Translation Interface
//
EFI_STATUS
EFIAPI
OvirGlTranslateDrawArrays (
  IN OUT OVIR_COMMAND_LIST  *CmdList,
  IN     UINT32             Mode,
  IN     INT32              First,
  IN     UINT32             Count
  );

EFI_STATUS
EFIAPI
OvirGlTranslateDrawElements (
  IN OUT OVIR_COMMAND_LIST  *CmdList,
  IN     UINT32             Mode,
  IN     UINT32             Count,
  IN     UINT32             Type,
  IN     UINT64             IndicesOffset
  );

//
// Metal Adapter Translation Interface
//
EFI_STATUS
EFIAPI
OvirMtlTranslateDrawPrimitives (
  IN OUT OVIR_COMMAND_LIST  *CmdList,
  IN     UINT32             PrimitiveType,
  IN     UINTN              VertexStart,
  IN     UINTN              VertexCount,
  IN     UINTN              InstanceCount
  );

//
// DirectX Adapter Translation Interface
//
EFI_STATUS
EFIAPI
OvirDxTranslateDrawIndexed (
  IN OUT OVIR_COMMAND_LIST  *CmdList,
  IN     UINT32             IndexCount,
  IN     UINT32             StartIndexLocation,
  IN     INT32              BaseVertexLocation
  );

#endif // OVIR_ADAPTERS_LIB_H_
