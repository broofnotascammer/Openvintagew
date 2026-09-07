/** @file
  OpenVintage Graphics Workload Resolver Bridge Implementation.
  Phase 3 OVIR-GPU to Hardware Backend Resolution Engine.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/OvLoggerLib.h>
#include <Library/OvResolverLib.h>
#include <Library/OvGpuCapabilityLib.h>
#include <Library/OvirGpuLib.h>
#include <Library/OvirPipelineLib.h>
#include <Library/OvirShaderLib.h>
#include <Library/OvirResolverLib.h>

EFI_STATUS
EFIAPI
OvirResolvePipeline (
  IN  CONST OVIR_GPU_CAPABILITIES *Capabilities,
  IN  CONST OVIR_PIPELINE_STATE   *Pipeline,
  OUT OV_RESOLVER_RESULT          *Result
  )
{
  if (Capabilities == NULL || Pipeline == NULL || Result == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (Result, sizeof (OV_RESOLVER_RESULT));

  // If pipeline is compute and GPU has no compute support
  if (Pipeline->IsCompute) {
    if (Capabilities->Features.ComputeShaders) {
      Result->Decision = OvResolutionNative;
      Result->PerformanceCostFactor = 100; // 1.0x baseline
      StrCpyS (Result->RoutingPath, 64, L"GPU 3D Engine");
      StrCpyS (Result->ReasonMessage, 64, L"GPU native compute dispatch supported");
      return EFI_SUCCESS;
    } else {
      // Fallback to CPU SIMD compute emulation
      Result->Decision = OvResolutionFallback;
      Result->PerformanceCostFactor = 650; // 6.5x software emulation penalty
      StrCpyS (Result->RoutingPath, 64, L"CPU SIMD Emulation");
      StrCpyS (Result->ReasonMessage, 64, L"Compute unsupported on GPU; resolving to CPU SIMD");
      return EFI_SUCCESS;
    }
  }

  // Graphics Pipeline Checks
  if (Pipeline->Descriptor.Rasterizer.PolygonMode == OVIR_POLYGON_MODE_LINE &&
      !Capabilities->Features.GeometryShaders &&
      Capabilities->Vendor == OvGpuVendorUnknown) {
    // Legacy software rasterizer simplifies non-fill polygon modes
    Result->Decision = OvResolutionSimplified;
    Result->PerformanceCostFactor = 120;
    Result->SimplifyShaders = TRUE;
    StrCpyS (Result->RoutingPath, 64, L"Simplified Primitive Emulation");
    StrCpyS (Result->ReasonMessage, 64, L"Wireframe rasterization clamped to simplified primitive");
    return EFI_SUCCESS;
  }

  // Check anisotropic filtering limits
  if (Pipeline->Descriptor.Blend.AlphaToCoverageEnable && !Capabilities->Features.DualSourceBlending) {
    Result->Decision = OvResolutionSimplified;
    Result->PerformanceCostFactor = 115;
    Result->SimplifyShaders = TRUE;
    StrCpyS (Result->RoutingPath, 64, L"Simplified Alpha Blending");
    StrCpyS (Result->ReasonMessage, 64, L"Alpha-to-coverage simplified due to absent dual-source blend");
    return EFI_SUCCESS;
  }

  // If vendor supports modern Vulkan or OpenGL core profile natively
  if (Capabilities->Apis.Vulkan.Supported || Capabilities->Apis.OpenGL.Supported) {
    Result->Decision = OvResolutionNative;
    Result->PerformanceCostFactor = 100;
    StrCpyS (Result->RoutingPath, 64, L"Native GPU Pipeline");
    StrCpyS (Result->ReasonMessage, 64, L"Hardware satisfies pipeline configuration natively");
  } else {
    Result->Decision = OvResolutionTranslated;
    Result->PerformanceCostFactor = 135;
    StrCpyS (Result->RoutingPath, 64, L"OVIR Translated Pipeline");
    StrCpyS (Result->ReasonMessage, 64, L"Pipeline mapped through OVIR translated path");
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirResolveCommandList (
  IN  CONST OVIR_GPU_CAPABILITIES *Capabilities,
  IN  CONST OVIR_COMMAND_LIST     *CmdList,
  OUT OV_RESOLVER_RESULT          *Result
  )
{
  UINT32   Index;
  BOOLEAN  HasCompute = FALSE;

  if (Capabilities == NULL || CmdList == NULL || Result == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (Result, sizeof (OV_RESOLVER_RESULT));

  for (Index = 0; Index < CmdList->Count; Index++) {
    if (CmdList->Commands[Index].Type == OVIR_CMD_DISPATCH) {
      HasCompute = TRUE;
    }
  }

  if (HasCompute && !Capabilities->Features.ComputeShaders) {
    Result->Decision = OvResolutionFallback;
    Result->PerformanceCostFactor = 700;
    StrCpyS (Result->RoutingPath, 64, L"CPU SIMD Emulation");
    StrCpyS (Result->ReasonMessage, 64, L"Command list compute routed to CPU fallback");
    return EFI_SUCCESS;
  }

  if (Capabilities->Vendor == OvGpuVendorIntel || Capabilities->Vendor == OvGpuVendorNvidia) {
    Result->Decision = OvResolutionNative;
    Result->PerformanceCostFactor = 100;
    StrCpyS (Result->RoutingPath, 64, L"Direct GPU Command Stream");
    StrCpyS (Result->ReasonMessage, 64, L"Command list directly maps to hardware command stream");
  } else {
    Result->Decision = OvResolutionTranslated;
    Result->PerformanceCostFactor = 140;
    StrCpyS (Result->RoutingPath, 64, L"Host Translated Command Stream");
    StrCpyS (Result->ReasonMessage, 64, L"Command list translated into host display adapter");
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirResolveFormat (
  IN  CONST OVIR_GPU_CAPABILITIES *Capabilities,
  IN  OVIR_FORMAT                 Format,
  IN  UINT32                      RequiredUsage,
  OUT OV_RESOLVER_RESULT          *Result
  )
{
  if (Capabilities == NULL || Result == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (Result, sizeof (OV_RESOLVER_RESULT));

  if (OvGpuIsFormatSupported (Capabilities, Format, RequiredUsage)) {
    Result->Decision = OvResolutionNative;
    Result->PerformanceCostFactor = 100;
    StrCpyS (Result->RoutingPath, 64, L"Direct Hardware Surface");
    StrCpyS (Result->ReasonMessage, 64, L"Format natively accelerated for requested usages");
    return EFI_SUCCESS;
  }

  // Format not directly supported: check if we can simplify or translate
  if (Format == OVIR_FORMAT_BC7_RGBA_UNORM && !OvGpuIsFormatSupported (Capabilities, Format, RequiredUsage)) {
    // Simplify BC7 compression down to uncompressed RGBA8
    Result->Decision = OvResolutionSimplified;
    Result->PerformanceCostFactor = 180;
    Result->SimplifyShaders = TRUE;
    StrCpyS (Result->RoutingPath, 64, L"Decompressed RGBA8 Surface");
    StrCpyS (Result->ReasonMessage, 64, L"BC7 texture simplified to decompressed RGBA8");
    return EFI_SUCCESS;
  }

  if (Format == OVIR_FORMAT_RGBA32_FLOAT && !OvGpuIsFormatSupported (Capabilities, Format, RequiredUsage)) {
    // Simplify 32-bit float to 16-bit half float
    Result->Decision = OvResolutionSimplified;
    Result->PerformanceCostFactor = 125;
    Result->ClampResolution = FALSE;
    StrCpyS (Result->RoutingPath, 64, L"Precision Clamped RGBA16F");
    StrCpyS (Result->ReasonMessage, 64, L"RGBA32F precision clamped to RGBA16F");
    return EFI_SUCCESS;
  }

  Result->Decision = OvResolutionUnsupported;
  Result->PerformanceCostFactor = 1000;
  StrCpyS (Result->RoutingPath, 64, L"None");
  StrCpyS (Result->ReasonMessage, 64, L"Requested format completely unsupported");
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirResolveShader (
  IN  CONST OVIR_GPU_CAPABILITIES *Capabilities,
  IN  CONST OVIR_SHADER_MODULE    *Shader,
  OUT OV_RESOLVER_RESULT          *Result
  )
{
  if (Capabilities == NULL || Shader == NULL || Result == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (Result, sizeof (OV_RESOLVER_RESULT));

  if ((Shader->Stage & OVIR_SHADER_STAGE_COMPUTE) && !Capabilities->Features.ComputeShaders) {
    Result->Decision = OvResolutionFallback;
    Result->PerformanceCostFactor = 500;
    StrCpyS (Result->RoutingPath, 64, L"CPU SIMD Compute");
    StrCpyS (Result->ReasonMessage, 64, L"Compute shader routed to CPU emulation");
    return EFI_SUCCESS;
  }

  if (Shader->Language == OvirShaderLangSpirV && Capabilities->Apis.Vulkan.HasSpirvDirect) {
    Result->Decision = OvResolutionNative;
    Result->PerformanceCostFactor = 100;
    StrCpyS (Result->RoutingPath, 64, L"Hardware SPIR-V Ingestion");
    StrCpyS (Result->ReasonMessage, 64, L"SPIR-V ingested natively by GPU driver");
    return EFI_SUCCESS;
  }

  Result->Decision = OvResolutionTranslated;
  Result->PerformanceCostFactor = 130;
  StrCpyS (Result->RoutingPath, 64, L"Cross-compiled Target Bytecode");
  StrCpyS (Result->ReasonMessage, 64, L"Shader translated to backend target bytecode");
  return EFI_SUCCESS;
}
