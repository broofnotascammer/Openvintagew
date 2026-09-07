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
