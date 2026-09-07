/** @file
  OpenVintage Workload Resolver Foundation Implementation.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/OvLoggerLib.h>
#include <Library/OvHardwareLib.h>
#include <Library/OvConfigLib.h>
#include <Library/OvResolverLib.h>

STATIC BOOLEAN  mResolverInitialized = FALSE;

EFI_STATUS
EFIAPI
OvResolverInitialize (
  VOID
  )
{
  mResolverInitialized = TRUE;
  OvLogTagged (OV_LOG_LEVEL_DEBUG, L"RESO", L"Resolver framework initialized");
  return EFI_SUCCESS;
}

CONST CHAR16 *
EFIAPI
OvResolverDecisionToString (
  IN OV_RESOLUTION_DECISION  Decision
  )
{
  switch (Decision) {
    case OvResolutionNative:
      return L"NATIVE";
    case OvResolutionTranslated:
      return L"TRANSLATED";
    case OvResolutionSimplified:
      return L"SIMPLIFIED";
    case OvResolutionFallback:
      return L"FALLBACK";
    case OvResolutionUnsupported:
      return L"UNSUPPORTED";
    default:
      return L"UNKNOWN";
  }
}

EFI_STATUS
EFIAPI
OvResolverEvaluate (
  IN  CONST OV_RESOLVER_REQUEST  *Request,
  OUT OV_RESOLVER_RESULT         *Result
  )
{
  OV_CPU_TOPOLOGY  Cpu;
  OV_GPU_TOPOLOGY  Gpu;
  BOOLEAN          StrictResolver;

  if ((Request == NULL) || (Result == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mResolverInitialized) {
    OvResolverInitialize ();
  }

  ZeroMem (Result, sizeof (OV_RESOLVER_RESULT));
  OvHardwareGetCpu (&Cpu);
  OvHardwareGetGpu (&Gpu);
  StrictResolver = OvConfigGetFeatureFlag (OV_FEATURE_RESOLVER_STRICT);

  switch (Request->Class) {
    case OvWorkloadClassGpuRendering:
    case OvWorkloadClassGpuCompute:
      if (Request->RequiresMetal) {
        // Evaluate Metal requirement:
        // Haswell/Broadwell with Intel HD 4600/5000/Iris or Nvidia Kepler/Maxwell or AMD GCN has Metal support
        if ((Cpu.Architecture == OvCpuArchHaswellBroadwell) &&
            ((Gpu.Vendor == OvGpuVendorIntel) || (Gpu.Vendor == OvGpuVendorNvidia) || (Gpu.Vendor == OvGpuVendorAmd))) {
          Result->Decision              = OvResolutionNative;
          Result->PerformanceCostFactor = 100;
          UnicodeSPrint (Result->ReasonMessage, sizeof (Result->ReasonMessage), L"Silicon hardware natively supports Metal");
          UnicodeSPrint (Result->RoutingPath, sizeof (Result->RoutingPath), L"Metal2 -> Direct Hardware Ring Buffer");
        } else if ((Cpu.Architecture == OvCpuArchSandyBridge) || (Cpu.Architecture == OvCpuArchIvyBridge)) {
          Result->Decision              = OvResolutionTranslated;
          Result->PerformanceCostFactor = 135;
          UnicodeSPrint (Result->ReasonMessage, sizeof (Result->ReasonMessage), L"Legacy GPU lacks native Metal; routing to OpenGL Core");
          UnicodeSPrint (Result->RoutingPath, sizeof (Result->RoutingPath), L"Metal2 -> OVIR-GPU -> OpenGL 3.3/4.1 Core");
        } else {
          if (StrictResolver) {
            Result->Decision              = OvResolutionFallback;
            Result->PerformanceCostFactor = 480;
            UnicodeSPrint (Result->ReasonMessage, sizeof (Result->ReasonMessage), L"GPU incapable; routing to SIMD CPU Software Rasterizer");
            UnicodeSPrint (Result->RoutingPath, sizeof (Result->RoutingPath), L"Metal2 -> OVIR-GPU -> CPU Soft-Rasterizer");
          } else {
            Result->Decision              = OvResolutionFallback;
            Result->PerformanceCostFactor = 450;
            UnicodeSPrint (Result->ReasonMessage, sizeof (Result->ReasonMessage), L"CPU Software Rasterizer Fallback");
            UnicodeSPrint (Result->RoutingPath, sizeof (Result->RoutingPath), L"Metal2 -> CPU SoftPipe");
          }
        }
      } else if (Request->RequiresVulkan) {
        if (Gpu.Vendor == OvGpuVendorIntel && Cpu.Architecture == OvCpuArchHaswellBroadwell) {
          Result->Decision              = OvResolutionNative;
          Result->PerformanceCostFactor = 105;
          UnicodeSPrint (Result->ReasonMessage, sizeof (Result->ReasonMessage), L"ANV / Vulkan 1.2 supported on Haswell+");
          UnicodeSPrint (Result->RoutingPath, sizeof (Result->RoutingPath), L"Vulkan 1.2 -> Direct Hardware");
        } else {
          Result->Decision              = OvResolutionTranslated;
          Result->PerformanceCostFactor = 145;
          UnicodeSPrint (Result->ReasonMessage, sizeof (Result->ReasonMessage), L"Vulkan transformed to OpenGL ES/GL backend");
          UnicodeSPrint (Result->RoutingPath, sizeof (Result->RoutingPath), L"Vulkan -> OVIR-GPU -> OpenGL 3.3 Core");
        }
      } else {
        // Standard legacy rendering
        Result->Decision              = OvResolutionNative;
        Result->PerformanceCostFactor = 100;
        UnicodeSPrint (Result->ReasonMessage, sizeof (Result->ReasonMessage), L"Standard GOP / OpenGL compatible");
        UnicodeSPrint (Result->RoutingPath, sizeof (Result->RoutingPath), L"Legacy Fixed/Core -> Direct");
      }

      // Memory constraint check: if requested VRAM > 256MB on low-memory GPU, clamp
      if (Request->EstimatedVramBytes > (256 * 1024 * 1024)) {
        Result->ClampResolution = TRUE;
        Result->SimplifyShaders = TRUE;
      }
      break;

    case OvWorkloadClassCpuSimd:
      if (Request->RequiresAVX2) {
        if (Cpu.HasAVX2) {
          Result->Decision              = OvResolutionNative;
          Result->PerformanceCostFactor = 100;
          UnicodeSPrint (Result->ReasonMessage, sizeof (Result->ReasonMessage), L"Host CPU provides native 256-bit AVX2/FMA");
          UnicodeSPrint (Result->RoutingPath, sizeof (Result->RoutingPath), L"AVX2 -> Direct Silicon Execution");
        } else if (Cpu.HasAVX) {
          Result->Decision              = OvResolutionTranslated;
          Result->PerformanceCostFactor = 160;
          UnicodeSPrint (Result->ReasonMessage, sizeof (Result->ReasonMessage), L"CPU supports AVX (128/256-bit FP only); integer AVX2 translated");
          UnicodeSPrint (Result->RoutingPath, sizeof (Result->RoutingPath), L"AVX2 -> OVIR-CPU -> AVX/SSE4.2 Split Dispatch");
        } else if (Cpu.HasSSE41) {
          Result->Decision              = OvResolutionTranslated;
          Result->PerformanceCostFactor = 230;
          UnicodeSPrint (Result->ReasonMessage, sizeof (Result->ReasonMessage), L"CPU limited to SSE4.1; translating 256-bit vectors to 128-bit");
          UnicodeSPrint (Result->RoutingPath, sizeof (Result->RoutingPath), L"AVX2 -> OVIR-CPU -> SSE4.1 Dual-Pump Emulation");
        } else {
          Result->Decision              = OvResolutionFallback;
          Result->PerformanceCostFactor = 550;
          UnicodeSPrint (Result->ReasonMessage, sizeof (Result->ReasonMessage), L"CPU lacks vector extensions; scalar software fallback");
          UnicodeSPrint (Result->RoutingPath, sizeof (Result->RoutingPath), L"AVX2 -> Scalar Soft-FPU Loop");
        }
      } else {
        Result->Decision              = OvResolutionNative;
        Result->PerformanceCostFactor = 100;
        UnicodeSPrint (Result->ReasonMessage, sizeof (Result->ReasonMessage), L"Standard x86_64 instruction baseline satisfied");
        UnicodeSPrint (Result->RoutingPath, sizeof (Result->RoutingPath), L"Native x86_64 Pipeline");
      }
      break;

    case OvWorkloadClassMemoryOps:
    default:
      Result->Decision              = OvResolutionNative;
      Result->PerformanceCostFactor = 100;
      UnicodeSPrint (Result->ReasonMessage, sizeof (Result->ReasonMessage), L"Direct Memory DMA / MMIO");
      UnicodeSPrint (Result->RoutingPath, sizeof (Result->RoutingPath), L"Direct Memory Bus Access");
      break;
  }

  OvLogTagged (
    OV_LOG_LEVEL_DEBUG,
    L"RESO",
    L"Workload '%s' resolved to %s (Cost Factor: %u%%) via %s",
    Request->Name,
    OvResolverDecisionToString (Result->Decision),
    Result->PerformanceCostFactor,
    Result->RoutingPath
    );

  return EFI_SUCCESS;
}

VOID
EFIAPI
OvResolverDumpPolicies (
  VOID
  )
{
  OvLogTagged (OV_LOG_LEVEL_INFO, L"RESO", L"--- WORKLOAD RESOLUTION POLICY MATRIX ---");
  OvLogTagged (OV_LOG_LEVEL_INFO, L"RESO", L"  [1] NATIVE      : Zero-overhead direct execution on compatible silicon");
  OvLogTagged (OV_LOG_LEVEL_INFO, L"RESO", L"  [2] TRANSLATED  : Intermediate DAG transformation via OVIR-GPU / OVIR-CPU");
  OvLogTagged (OV_LOG_LEVEL_INFO, L"RESO", L"  [3] FALLBACK    : Multi-threaded SIMD CPU emulation / SoftPipe");
  OvLogTagged (OV_LOG_LEVEL_INFO, L"RESO", L"  [4] UNSUPPORTED : Hardware rejection with clean safety abort");
}
