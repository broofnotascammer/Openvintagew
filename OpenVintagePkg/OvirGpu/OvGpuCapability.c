/** @file
  OpenVintage GPU Capability Manager Implementation.
  Phase 3 GPU Hardware & API Capability Assessment.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/OvLoggerLib.h>
#include <Library/OvHardwareLib.h>
#include <Library/OvirGpuLib.h>
#include <Library/OvGpuCapabilityLib.h>

STATIC
VOID
OvPopulateFormatTable (
  IN OUT OVIR_GPU_CAPABILITIES *Caps
  )
{
  UINTN Count = 0;

  // R8_UNORM
  Caps->FormatTable[Count].Format = OVIR_FORMAT_R8_UNORM;
  Caps->FormatTable[Count].Features = OV_FORMAT_FEAT_SAMPLED_IMAGE | OV_FORMAT_FEAT_BLIT_SRC | OV_FORMAT_FEAT_BLIT_DST;
  Count++;

  // RG8_UNORM
  Caps->FormatTable[Count].Format = OVIR_FORMAT_RG8_UNORM;
  Caps->FormatTable[Count].Features = OV_FORMAT_FEAT_SAMPLED_IMAGE | OV_FORMAT_FEAT_BLIT_SRC | OV_FORMAT_FEAT_BLIT_DST;
  Count++;

  // RGBA8_UNORM
  Caps->FormatTable[Count].Format = OVIR_FORMAT_RGBA8_UNORM;
  Caps->FormatTable[Count].Features = OV_FORMAT_FEAT_SAMPLED_IMAGE | OV_FORMAT_FEAT_STORAGE_IMAGE |
                                      OV_FORMAT_FEAT_COLOR_ATTACHMENT | OV_FORMAT_FEAT_BLIT_SRC | OV_FORMAT_FEAT_BLIT_DST;
  Count++;

  // BGRA8_UNORM
  Caps->FormatTable[Count].Format = OVIR_FORMAT_BGRA8_UNORM;
  Caps->FormatTable[Count].Features = OV_FORMAT_FEAT_SAMPLED_IMAGE | OV_FORMAT_FEAT_COLOR_ATTACHMENT |
                                      OV_FORMAT_FEAT_BLIT_SRC | OV_FORMAT_FEAT_BLIT_DST;
  Count++;

  // RGBA16_FLOAT
  Caps->FormatTable[Count].Format = OVIR_FORMAT_RGBA16_FLOAT;
  Caps->FormatTable[Count].Features = OV_FORMAT_FEAT_SAMPLED_IMAGE | OV_FORMAT_FEAT_STORAGE_IMAGE |
                                      OV_FORMAT_FEAT_COLOR_ATTACHMENT | OV_FORMAT_FEAT_BLIT_SRC;
  Count++;

  // RGBA32_FLOAT
  Caps->FormatTable[Count].Format = OVIR_FORMAT_RGBA32_FLOAT;
  Caps->FormatTable[Count].Features = OV_FORMAT_FEAT_SAMPLED_IMAGE | OV_FORMAT_FEAT_STORAGE_IMAGE;
  Count++;

  // D24_UNORM_S8_UINT
  Caps->FormatTable[Count].Format = OVIR_FORMAT_D24_UNORM_S8_UINT;
  Caps->FormatTable[Count].Features = OV_FORMAT_FEAT_SAMPLED_IMAGE | OV_FORMAT_FEAT_DEPTH_ATTACHMENT;
  Count++;

  // D32_FLOAT
  Caps->FormatTable[Count].Format = OVIR_FORMAT_D32_FLOAT;
  Caps->FormatTable[Count].Features = OV_FORMAT_FEAT_SAMPLED_IMAGE | OV_FORMAT_FEAT_DEPTH_ATTACHMENT;
  Count++;

  // BC1_RGBA_UNORM
  Caps->FormatTable[Count].Format = OVIR_FORMAT_BC1_RGBA_UNORM;
  Caps->FormatTable[Count].Features = OV_FORMAT_FEAT_SAMPLED_IMAGE;
  Count++;

  // BC3_RGBA_UNORM
  Caps->FormatTable[Count].Format = OVIR_FORMAT_BC3_RGBA_UNORM;
  Caps->FormatTable[Count].Features = OV_FORMAT_FEAT_SAMPLED_IMAGE;
  Count++;

  // BC7_RGBA_UNORM
  Caps->FormatTable[Count].Format = OVIR_FORMAT_BC7_RGBA_UNORM;
  Caps->FormatTable[Count].Features = OV_FORMAT_FEAT_SAMPLED_IMAGE;
  Count++;

  Caps->SupportedFormatCount = Count;
}

EFI_STATUS
EFIAPI
OvGpuCapabilityDetect (
  OUT OVIR_GPU_CAPABILITIES  *Capabilities
  )
{
  OV_GPU_TOPOLOGY  Gpu;
  OV_CPU_TOPOLOGY  Cpu;
  CHAR16           LogBuf[128];

  if (Capabilities == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (Capabilities, sizeof (OVIR_GPU_CAPABILITIES));
  OvHardwareGetGpu (&Gpu);
  OvHardwareGetCpu (&Cpu);

  Capabilities->IsValid = TRUE;
  Capabilities->Vendor = Gpu.Vendor;
  Capabilities->VendorId = Gpu.VendorId;
  Capabilities->DeviceId = Gpu.DeviceId;
  Capabilities->SubsystemId = Gpu.SubsystemId;
  Capabilities->RevisionId = Gpu.RevisionId;

  if (StrLen (Gpu.AdapterName) > 0) {
    StrCpyS (Capabilities->ModelName, 64, Gpu.AdapterName);
  } else {
    StrCpyS (Capabilities->ModelName, 64, L"Generic Display Adapter");
  }

  UnicodeSPrint (Capabilities->DriverVersionString, 32 * sizeof (CHAR16), L"OVIR-v0.3.0");

  //
  // Configure API Capabilities based on detected hardware profile
  //
  if (Gpu.Vendor == OvGpuVendorIntel) {
    // Intel Gen 7/7.5/8 (Haswell / Broadwell / Ivy Bridge)
    Capabilities->Apis.Vulkan.Supported = TRUE;
    Capabilities->Apis.Vulkan.VersionMajor = 1;
    Capabilities->Apis.Vulkan.VersionMinor = 0;
    Capabilities->Apis.Vulkan.HasSpirvDirect = TRUE;
    Capabilities->Apis.Vulkan.HasTimelineSemaphores = FALSE;
    Capabilities->Apis.Vulkan.HasDescriptorIndexing = FALSE;

    Capabilities->Apis.OpenGL.Supported = TRUE;
    Capabilities->Apis.OpenGL.VersionMajor = 4;
    Capabilities->Apis.OpenGL.VersionMinor = 2;
    Capabilities->Apis.OpenGL.IsCoreProfile = TRUE;
    Capabilities->Apis.OpenGL.HasComputeShaders = TRUE;
    Capabilities->Apis.OpenGL.HasUniformBufferObjects = TRUE;
    Capabilities->Apis.OpenGL.MaxUboSizeBytes = 65536;

    // Metal is translated via OVIR on Intel Mac hardware
    Capabilities->Apis.Metal.Supported = FALSE; // Direct native host metal is not in UEFI
    Capabilities->Apis.Metal.MetalFeatureSet = 1; // Translates Metal 1.0/2.0
    Capabilities->Apis.Metal.HasArgumentBuffers = FALSE;

    // DirectX is translated via OVIR
    Capabilities->Apis.DirectX.Supported = FALSE; // Direct DX host runtime is Windows-only
    Capabilities->Apis.DirectX.FeatureLevel = 0xB000; // FL 11_0 translation target

    // Feature set
    Capabilities->Features.ComputeShaders = TRUE;
    Capabilities->Features.GeometryShaders = TRUE;
    Capabilities->Features.Tessellation = TRUE;
    Capabilities->Features.DualSourceBlending = TRUE;
    Capabilities->Features.MultiViewport = FALSE;
    Capabilities->Features.DepthClamp = TRUE;
    Capabilities->Features.Float64 = FALSE;
    Capabilities->Features.Int64 = FALSE;
    Capabilities->Features.AnisotropicFiltering = TRUE;
    Capabilities->Features.MaxAnisotropy = 16;
    Capabilities->Features.MaxColorAttachments = 8;
    Capabilities->Features.MaxTextureDimension1D = 8192;
    Capabilities->Features.MaxTextureDimension2D = 8192;
    Capabilities->Features.MaxTextureDimension3D = 2048;
    Capabilities->Features.MaxTextureDimensionCube = 8192;
    Capabilities->Features.MaxVertexAttributes = 16;
    Capabilities->Features.MaxVertexBindings = 16;
    Capabilities->Features.MaxComputeWorkGroupInvocations = 512;
    Capabilities->Features.MaxComputeWorkGroupSize[0] = 512;
    Capabilities->Features.MaxComputeWorkGroupSize[1] = 512;
    Capabilities->Features.MaxComputeWorkGroupSize[2] = 64;

    // Memory
    Capabilities->Memory.DedicatedVramBytes = 0; // Integrated GPU
    Capabilities->Memory.SharedSystemRamBytes = 1536ULL * 1024ULL * 1024ULL; // 1.5 GB shared aperture
    Capabilities->Memory.BarApertureSizeBytes = 256ULL * 1024ULL * 1024ULL;
    Capabilities->Memory.MaxAllocationSizeBytes = 512ULL * 1024ULL * 1024ULL;
    Capabilities->Memory.UnifiedMemoryArchitecture = TRUE;

  } else if (Gpu.Vendor == OvGpuVendorNvidia) {
    // Nvidia GeForce / Quadro (Kepler / Maxwell)
    Capabilities->Apis.Vulkan.Supported = TRUE;
    Capabilities->Apis.Vulkan.VersionMajor = 1;
    Capabilities->Apis.Vulkan.VersionMinor = 2;
    Capabilities->Apis.Vulkan.HasSpirvDirect = TRUE;
    Capabilities->Apis.Vulkan.HasTimelineSemaphores = TRUE;
    Capabilities->Apis.Vulkan.HasDescriptorIndexing = TRUE;

    Capabilities->Apis.OpenGL.Supported = TRUE;
    Capabilities->Apis.OpenGL.VersionMajor = 4;
    Capabilities->Apis.OpenGL.VersionMinor = 5;
    Capabilities->Apis.OpenGL.IsCoreProfile = TRUE;
    Capabilities->Apis.OpenGL.HasComputeShaders = TRUE;
    Capabilities->Apis.OpenGL.HasUniformBufferObjects = TRUE;
    Capabilities->Apis.OpenGL.MaxUboSizeBytes = 65536;

    Capabilities->Apis.Metal.Supported = FALSE;
    Capabilities->Apis.Metal.MetalFeatureSet = 2;

    Capabilities->Apis.DirectX.Supported = FALSE;
    Capabilities->Apis.DirectX.FeatureLevel = 0xB100; // FL 11_1

    Capabilities->Features.ComputeShaders = TRUE;
    Capabilities->Features.GeometryShaders = TRUE;
    Capabilities->Features.Tessellation = TRUE;
    Capabilities->Features.DualSourceBlending = TRUE;
    Capabilities->Features.MultiViewport = TRUE;
    Capabilities->Features.DepthClamp = TRUE;
    Capabilities->Features.Float64 = TRUE;
    Capabilities->Features.AnisotropicFiltering = TRUE;
    Capabilities->Features.MaxAnisotropy = 16;
    Capabilities->Features.MaxColorAttachments = 8;
    Capabilities->Features.MaxTextureDimension1D = 16384;
    Capabilities->Features.MaxTextureDimension2D = 16384;
    Capabilities->Features.MaxTextureDimension3D = 2048;
    Capabilities->Features.MaxTextureDimensionCube = 16384;
    Capabilities->Features.MaxVertexAttributes = 16;
    Capabilities->Features.MaxVertexBindings = 16;
    Capabilities->Features.MaxComputeWorkGroupInvocations = 1024;
    Capabilities->Features.MaxComputeWorkGroupSize[0] = 1024;
    Capabilities->Features.MaxComputeWorkGroupSize[1] = 1024;
    Capabilities->Features.MaxComputeWorkGroupSize[2] = 64;

    Capabilities->Memory.DedicatedVramBytes = 2048ULL * 1024ULL * 1024ULL; // 2 GB VRAM
    Capabilities->Memory.SharedSystemRamBytes = 2048ULL * 1024ULL * 1024ULL;
    Capabilities->Memory.BarApertureSizeBytes = 256ULL * 1024ULL * 1024ULL;
    Capabilities->Memory.MaxAllocationSizeBytes = 1024ULL * 1024ULL * 1024ULL;
    Capabilities->Memory.UnifiedMemoryArchitecture = FALSE;

  } else {
    // Generic / Software / QEMU Bochs / Standard VGA fallback
    Capabilities->Apis.Vulkan.Supported = FALSE;
    Capabilities->Apis.OpenGL.Supported = TRUE;
    Capabilities->Apis.OpenGL.VersionMajor = 2;
    Capabilities->Apis.OpenGL.VersionMinor = 1;
    Capabilities->Apis.OpenGL.IsCoreProfile = FALSE;
    Capabilities->Apis.OpenGL.HasComputeShaders = FALSE;
    Capabilities->Apis.OpenGL.HasUniformBufferObjects = FALSE;
    Capabilities->Apis.OpenGL.MaxUboSizeBytes = 16384;

    Capabilities->Apis.Metal.Supported = FALSE;
    Capabilities->Apis.DirectX.Supported = FALSE;

    Capabilities->Features.ComputeShaders = FALSE;
    Capabilities->Features.GeometryShaders = FALSE;
    Capabilities->Features.Tessellation = FALSE;
    Capabilities->Features.DualSourceBlending = FALSE;
    Capabilities->Features.MultiViewport = FALSE;
    Capabilities->Features.DepthClamp = FALSE;
    Capabilities->Features.Float64 = FALSE;
    Capabilities->Features.AnisotropicFiltering = FALSE;
    Capabilities->Features.MaxAnisotropy = 1;
    Capabilities->Features.MaxColorAttachments = 1;
    Capabilities->Features.MaxTextureDimension1D = 4096;
    Capabilities->Features.MaxTextureDimension2D = 4096;
    Capabilities->Features.MaxTextureDimension3D = 512;
    Capabilities->Features.MaxTextureDimensionCube = 4096;
    Capabilities->Features.MaxVertexAttributes = 8;
    Capabilities->Features.MaxVertexBindings = 8;
    Capabilities->Features.MaxComputeWorkGroupInvocations = 0;

    Capabilities->Memory.DedicatedVramBytes = (Gpu.FrameBufferSize > 0) ? Gpu.FrameBufferSize : (16ULL * 1024ULL * 1024ULL);
    Capabilities->Memory.SharedSystemRamBytes = 512ULL * 1024ULL * 1024ULL;
    Capabilities->Memory.BarApertureSizeBytes = Capabilities->Memory.DedicatedVramBytes;
    Capabilities->Memory.MaxAllocationSizeBytes = Capabilities->Memory.DedicatedVramBytes;
    Capabilities->Memory.UnifiedMemoryArchitecture = FALSE;
  }

  OvPopulateFormatTable (Capabilities);

  UnicodeSPrint (
    LogBuf,
    sizeof (LogBuf),
    L"GPU Model: %s (Vendor: 0x%04x, Device: 0x%04x)",
    Capabilities->ModelName,
    Capabilities->VendorId,
    Capabilities->DeviceId
    );
  OvLogTagged (OV_LOG_LEVEL_INFO, L"CAPS", LogBuf);

  return EFI_SUCCESS;
}

BOOLEAN
EFIAPI
OvGpuIsFormatSupported (
  IN CONST OVIR_GPU_CAPABILITIES *Capabilities,
  IN OVIR_FORMAT                 Format,
  IN UINT32                      RequiredMask
  )
{
  UINTN Index;

  if (Capabilities == NULL || !Capabilities->IsValid) {
    return FALSE;
  }

  for (Index = 0; Index < Capabilities->SupportedFormatCount; Index++) {
    if (Capabilities->FormatTable[Index].Format == Format) {
      return ((Capabilities->FormatTable[Index].Features & RequiredMask) == RequiredMask);
    }
  }

  return FALSE;
}

UINT32
EFIAPI
OvGpuGetMaxTextureDimension (
  IN CONST OVIR_GPU_CAPABILITIES *Capabilities,
  IN OVIR_RESOURCE_TYPE          Dimension
  )
{
  if (Capabilities == NULL || !Capabilities->IsValid) {
    return 2048;
  }

  switch (Dimension) {
    case OvirResourceTexture1D:
      return Capabilities->Features.MaxTextureDimension1D;
    case OvirResourceTexture2D:
      return Capabilities->Features.MaxTextureDimension2D;
    case OvirResourceTexture3D:
      return Capabilities->Features.MaxTextureDimension3D;
    case OvirResourceTextureCube:
      return Capabilities->Features.MaxTextureDimensionCube;
    default:
      return 2048;
  }
}
