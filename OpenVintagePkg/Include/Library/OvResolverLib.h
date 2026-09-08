/** @file
  OpenVintage Workload Resolver Foundation Definition.
  Phase 2 Architectural Decision Framework.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OV_RESOLVER_LIB_H_
#define OV_RESOLVER_LIB_H_

#include <Uefi.h>
#include <Library/OvHardwareLib.h>

//
// Resolution Decisions
//
typedef enum {
  OvResolutionNative = 1,       // Target silicon natively satisfies workload requirements
  OvResolutionTranslated = 2,   // Workload transformed through OVIR translation layer
  OvResolutionSimplified = 3,   // Workload clamped/simplified for compatibility/VRAM
  OvResolutionFallback = 4,     // Workload executed on CPU software emulation path
  OvResolutionUnsupported = 5   // Workload cannot be executed by available hardware
} OV_RESOLUTION_DECISION;

//
// Workload Subsystem Class
//
typedef enum {
  OvWorkloadClassGpuRendering = 1,
  OvWorkloadClassGpuCompute,
  OvWorkloadClassCpuSimd,
  OvWorkloadClassMemoryOps
} OV_WORKLOAD_CLASS;

//
// Requested Workload Specification
//
typedef struct {
  CHAR16            Name[32];
  OV_WORKLOAD_CLASS Class;
  UINT32            RequiredApiMajor;
  UINT32            RequiredApiMinor;
  BOOLEAN           RequiresMetal;
  BOOLEAN           RequiresVulkan;
  BOOLEAN           RequiresDirectX;
  BOOLEAN           RequiresAVX2;
  BOOLEAN           RequiresTessellation;
  BOOLEAN           RequiresFloat64;
  UINT64            EstimatedVramBytes;
} OV_RESOLVER_REQUEST;

//
// Phase 5 Integrated Multi-Subsystem Workload Request
//
typedef struct {
  CHAR16            ApplicationName[32];
  UINT32            GuestCpuArch;       // 1 = ARM64, 2 = x86_64, 3 = x86_32
  UINT32            RequestedApi;       // 1 = Metal, 2 = Vulkan, 3 = OpenGL, 4 = DirectX
  UINT32            ApiVersionMajor;
  UINT32            ApiVersionMinor;
  BOOLEAN           RequiresCompute;
  BOOLEAN           RequiresTessellation;
  UINT32            MaxTextureDimension;
  UINT64            RequiredVramBytes;
  UINT64            RequiredRamBytes;
  UINT32            RequiredCpuCores;
  UINT64            GuestCodeHash;      // For CPU translation cache lookup
  UINT64            ShaderBytecodeHash; // For GPU shader cache lookup
} OV_INTEGRATED_WORKLOAD_REQUEST;

//
// Phase 5 Integrated Resolution Evaluation Result
//
typedef struct {
  OV_RESOLUTION_DECISION  Decision;
  UINT32                  PerformanceCostFactor;  // 100 = 1.0x baseline
  BOOLEAN                 CpuTranslationRequired;
  BOOLEAN                 GpuTranslationRequired;
  BOOLEAN                 CpuCacheHit;
  BOOLEAN                 ShaderCacheHit;
  BOOLEAN                 TextureClampApplied;
  BOOLEAN                 ShaderSimplificationApplied;
  BOOLEAN                 SoftwareFallbackUsed;
  UINT64                  AllocatedVramQuota;
  UINT64                  AllocatedRamQuota;
  CHAR16                  CpuPath[64];
  CHAR16                  GpuPath[64];
  CHAR16                  RoutingPath[96];
  CHAR16                  Rationale[128];
} OV_INTEGRATED_RESOLUTION_RESULT;

//
// Resolution Result Evaluation
//
typedef struct {
  OV_RESOLUTION_DECISION  Decision;
  UINT32                  PerformanceCostFactor;  // 100 = 1.0x (Baseline Native), 150 = 1.5x, 500 = 5.0x
  CHAR16                  ReasonMessage[64];
  CHAR16                  RoutingPath[64];
  BOOLEAN                 ClampResolution;
  BOOLEAN                 SimplifyShaders;
} OV_RESOLVER_RESULT;

/**
  Initialize OpenVintage Resolver framework.

  @retval EFI_SUCCESS  Resolver initialized.
**/
EFI_STATUS
EFIAPI
OvResolverInitialize (
  VOID
  );

/**
  Evaluate a workload against detected hardware capabilities.

  @param[in]  Request  Workload requirements specification.
  @param[out] Result   Evaluation decision and routing plan.

  @retval EFI_SUCCESS  Workload resolved.
**/
EFI_STATUS
EFIAPI
OvResolverEvaluate (
  IN  CONST OV_RESOLVER_REQUEST  *Request,
  OUT OV_RESOLVER_RESULT         *Result
  );

/**
  Phase 5 Integrated Resolution Evaluation.
  Coordinates CPU architecture, GPU architecture, Graphics API,
  Hardware capabilities, Compatibility information, Cached translations,
  and Resource availability to select the most appropriate execution path.

  @param[in]  Request  Integrated workload requirements.
  @param[out] Result   Comprehensive resolution result and resource quotas.

  @retval EFI_SUCCESS            Workload resolved.
  @retval EFI_INVALID_PARAMETER  Request or Result is NULL.
**/
EFI_STATUS
EFIAPI
OvResolverEvaluateIntegrated (
  IN  CONST OV_INTEGRATED_WORKLOAD_REQUEST  *Request,
  OUT OV_INTEGRATED_RESOLUTION_RESULT       *Result
  );

/**
  Convert resolution decision enum to string representation.

  @param[in] Decision   Resolution decision enum value.

  @return CONST CHAR16* Human-readable string.
**/
CONST CHAR16 *
EFIAPI
OvResolverDecisionToString (
  IN OV_RESOLUTION_DECISION  Decision
  );

/**
  Dump active resolver policies to logger.
**/
VOID
EFIAPI
OvResolverDumpPolicies (
  VOID
  );

#endif // OV_RESOLVER_LIB_H_
