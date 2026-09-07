/** @file
  OpenVintage Graphics Workload Resolver Definition.
  Phase 3 OVIR-GPU to Hardware Backend Resolution Engine.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OVIR_RESOLVER_LIB_H_
#define OVIR_RESOLVER_LIB_H_

#include <Uefi.h>
#include <Library/OvirGpuLib.h>
#include <Library/OvirPipelineLib.h>
#include <Library/OvirShaderLib.h>
#include <Library/OvGpuCapabilityLib.h>
#include <Library/OvResolverLib.h>

/**
  Evaluate an OVIR Pipeline state against active GPU capabilities.

  Determines whether the pipeline will execute Native, Translated, Simplified,
  Fallback, or Unsupported.

  @param[in]  Capabilities  Hardware capabilities.
  @param[in]  Pipeline      Pipeline state object.
  @param[out] Result        Resolver decision and routing info.

  @retval EFI_SUCCESS       Evaluated cleanly.
**/
EFI_STATUS
EFIAPI
OvirResolvePipeline (
  IN  CONST OVIR_GPU_CAPABILITIES *Capabilities,
  IN  CONST OVIR_PIPELINE_STATE   *Pipeline,
  OUT OV_RESOLVER_RESULT          *Result
  );

/**
  Evaluate a command list of graphics operations.

  @param[in]  Capabilities  Hardware capabilities.
  @param[in]  CmdList       Command list stream.
  @param[out] Result        Resolver decision.

  @retval EFI_SUCCESS       Evaluated cleanly.
**/
EFI_STATUS
EFIAPI
OvirResolveCommandList (
  IN  CONST OVIR_GPU_CAPABILITIES *Capabilities,
  IN  CONST OVIR_COMMAND_LIST     *CmdList,
  OUT OV_RESOLVER_RESULT          *Result
  );

/**
  Evaluate an individual texture or buffer format request.

  @param[in]  Capabilities  Hardware capabilities.
  @param[in]  Format        Requested format.
  @param[in]  RequiredUsage Required usage bitmask.
  @param[out] Result        Decision and recommendation.

  @retval EFI_SUCCESS       Evaluated.
**/
EFI_STATUS
EFIAPI
OvirResolveFormat (
  IN  CONST OVIR_GPU_CAPABILITIES *Capabilities,
  IN  OVIR_FORMAT                 Format,
  IN  UINT32                      RequiredUsage,
  OUT OV_RESOLVER_RESULT          *Result
  );

/**
  Evaluate a shader stage module for hardware compatibility.

  @param[in]  Capabilities  Hardware capabilities.
  @param[in]  Shader        Shader module.
  @param[out] Result        Resolver evaluation.

  @retval EFI_SUCCESS       Evaluated.
**/
EFI_STATUS
EFIAPI
OvirResolveShader (
  IN  CONST OVIR_GPU_CAPABILITIES *Capabilities,
  IN  CONST OVIR_SHADER_MODULE    *Shader,
  OUT OV_RESOLVER_RESULT          *Result
  );

#endif // OVIR_RESOLVER_LIB_H_
