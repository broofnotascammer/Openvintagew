/** @file
  OpenVintage GPU Capability Manager Definition.
  Phase 3 GPU Hardware & API Capability Assessment.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OV_GPU_CAPABILITY_LIB_H_
#define OV_GPU_CAPABILITY_LIB_H_

#include <Uefi.h>
#include <Library/OvirGpuLib.h>
#include <Library/OvHardwareLib.h>

//
// Supported API Version Tiers
//
typedef struct {
  BOOLEAN Supported;
  UINT32  VersionMajor;
  UINT32  VersionMinor;
  BOOLEAN HasSpirvDirect;
  BOOLEAN HasTimelineSemaphores;
  BOOLEAN HasDescriptorIndexing;
} OV_VK_CAPABILITIES;

typedef struct {
  BOOLEAN Supported;
  UINT32  VersionMajor;
  UINT32  VersionMinor;
  BOOLEAN IsCoreProfile;
  BOOLEAN HasComputeShaders;
  BOOLEAN HasUniformBufferObjects;
  UINT32  MaxUboSizeBytes;
} OV_GL_CAPABILITIES;

typedef struct {
  BOOLEAN Supported;
  UINT32  MetalFeatureSet; // 1 = Metal 1.0, 2 = Metal 2.0, 3 = Metal 3.0
  BOOLEAN HasArgumentBuffers;
  BOOLEAN HasIndirectCommandBuffers;
  BOOLEAN HasBarycentricCoordinates;
} OV_MTL_CAPABILITIES;

typedef struct {
  BOOLEAN Supported;
  UINT32  FeatureLevel; // 0x9300 (9.3), 0xA000 (10.0), 0xB000 (11.0), 0xB100 (11.1), 0xC000 (12.0)
  BOOLEAN HasRaytracing;
  BOOLEAN HasMeshShaders;
  BOOLEAN HasConservativeRaster;
} OV_DX_CAPABILITIES;

//
// Consolidated API Capabilities
//
typedef struct {
  OV_VK_CAPABILITIES  Vulkan;
  OV_GL_CAPABILITIES  OpenGL;
  OV_MTL_CAPABILITIES Metal;
  OV_DX_CAPABILITIES  DirectX;
} OV_GRAPHICS_API_CAPABILITIES;

//
// Fine-Grained Hardware Features
//
typedef struct {
  BOOLEAN ComputeShaders;
  BOOLEAN GeometryShaders;
  BOOLEAN Tessellation;
  BOOLEAN DualSourceBlending;
  BOOLEAN MultiViewport;
  BOOLEAN DepthClamp;
  BOOLEAN ConservativeRasterization;
  BOOLEAN Float64;
  BOOLEAN Int64;
  BOOLEAN AnisotropicFiltering;
  UINT32  MaxAnisotropy;
  UINT32  MaxColorAttachments;
  UINT32  MaxTextureDimension1D;
  UINT32  MaxTextureDimension2D;
  UINT32  MaxTextureDimension3D;
  UINT32  MaxTextureDimensionCube;
  UINT32  MaxVertexAttributes;
  UINT32  MaxVertexBindings;
  UINT32  MaxComputeWorkGroupInvocations;
  UINT32  MaxComputeWorkGroupSize[3];
} OV_GPU_FEATURE_SET;

//
// Texture Format Support Bitflags
//
#define OV_FORMAT_FEAT_SAMPLED_IMAGE           0x00000001
#define OV_FORMAT_FEAT_STORAGE_IMAGE           0x00000002
#define OV_FORMAT_FEAT_COLOR_ATTACHMENT        0x00000004
#define OV_FORMAT_FEAT_DEPTH_ATTACHMENT        0x00000008
#define OV_FORMAT_FEAT_BLIT_SRC                0x00000010
#define OV_FORMAT_FEAT_BLIT_DST                0x00000020
#define OV_FORMAT_FEAT_UNIFORM_TEXEL_BUFFER    0x00000040
#define OV_FORMAT_FEAT_STORAGE_TEXEL_BUFFER    0x00000080

typedef struct {
  OVIR_FORMAT Format;
  UINT32      Features;
} OV_TEXTURE_FORMAT_SUPPORT;

//
// GPU VRAM & Memory Limits
//
typedef struct {
  UINT64 DedicatedVramBytes;
  UINT64 SharedSystemRamBytes;
  UINT64 BarApertureSizeBytes;
  UINT64 MaxAllocationSizeBytes;
  BOOLEAN UnifiedMemoryArchitecture;
} OV_GPU_MEMORY_INFO;

//
// Top-Level GPU Capability Profile
//
typedef struct {
  BOOLEAN                       IsValid;
  OV_GPU_VENDOR                 Vendor;
  UINT16                        VendorId;
  UINT16                        DeviceId;
  UINT16                        SubsystemId;
  UINT8                         RevisionId;
  CHAR16                        ModelName[64];
  CHAR16                        DriverVersionString[32];
  OV_GRAPHICS_API_CAPABILITIES  Apis;
  OV_GPU_FEATURE_SET            Features;
  OV_GPU_MEMORY_INFO            Memory;
  UINTN                         SupportedFormatCount;
  OV_TEXTURE_FORMAT_SUPPORT     FormatTable[32];
} OVIR_GPU_CAPABILITIES;

/**
  Probe active system hardware and populate capability structures.

  @param[out] Capabilities  Pointer to target capability structure.

  @retval EFI_SUCCESS       GPU capabilities evaluated.
**/
EFI_STATUS
EFIAPI
OvGpuCapabilityDetect (
  OUT OVIR_GPU_CAPABILITIES  *Capabilities
  );

/**
  Query whether a specific format satisfies desired usage features.

  @param[in] Capabilities   Pointer to populated capabilities.
  @param[in] Format         Format to query.
  @param[in] RequiredMask   Mask of OV_FORMAT_FEAT_* requirements.

  @retval TRUE              Supported.
  @retval FALSE             Unsupported or degraded.
**/
BOOLEAN
EFIAPI
OvGpuIsFormatSupported (
  IN CONST OVIR_GPU_CAPABILITIES *Capabilities,
  IN OVIR_FORMAT                 Format,
  IN UINT32                      RequiredMask
  );

/**
  Query maximum texture dimension for given format and dimensionality.

  @param[in] Capabilities   Pointer to populated capabilities.
  @param[in] Dimension      1D, 2D, 3D, Cube.

  @return Max pixel limit.
**/
UINT32
EFIAPI
OvGpuGetMaxTextureDimension (
  IN CONST OVIR_GPU_CAPABILITIES *Capabilities,
  IN OVIR_RESOURCE_TYPE          Dimension
  );

#endif // OV_GPU_CAPABILITY_LIB_H_
