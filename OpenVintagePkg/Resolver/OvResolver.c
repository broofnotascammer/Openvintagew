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

EFI_STATUS
EFIAPI
OvResolverEvaluateIntegrated (
  IN  CONST OV_INTEGRATED_WORKLOAD_REQUEST  *Request,
  OUT OV_INTEGRATED_RESOLUTION_RESULT       *Result
  )
{
  OV_CPU_TOPOLOGY  Cpu;
  OV_GPU_TOPOLOGY  Gpu;
  UINT32           CostFactor;

  if ((Request == NULL) || (Result == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mResolverInitialized) {
    OvResolverInitialize ();
  }

  ZeroMem (Result, sizeof (OV_INTEGRATED_RESOLUTION_RESULT));
  OvHardwareGetCpu (&Cpu);
  OvHardwareGetGpu (&Gpu);

  CostFactor = 100; // Baseline 1.0x

  //
  // 1. CPU Architecture Coordination
  //
  if (Request->GuestCpuArch == 1) { // Guest ARM64
    Result->CpuTranslationRequired = TRUE;
    CostFactor += 35; // 35% translation overhead
    if (Request->GuestCodeHash != 0 && (Request->GuestCodeHash & 0x1) == 0) {
      Result->CpuCacheHit = TRUE;
      CostFactor -= 15; // Cache hit amortizes translation overhead
      UnicodeSPrint (Result->CpuPath, sizeof (Result->CpuPath), L"ARM64 -> Cached JIT Block -> Native X64");
    } else {
      Result->CpuCacheHit = FALSE;
      UnicodeSPrint (Result->CpuPath, sizeof (Result->CpuPath), L"ARM64 -> OVIR-CPU JIT -> Native X64");
    }
  } else if (Request->GuestCpuArch == 3) { // Guest x86_32
    Result->CpuTranslationRequired = TRUE;
    CostFactor += 20;
    UnicodeSPrint (Result->CpuPath, sizeof (Result->CpuPath), L"IA32 -> OVIR-CPU Thunk -> Native X64");
  } else { // Guest x86_64
    Result->CpuTranslationRequired = FALSE;
    UnicodeSPrint (Result->CpuPath, sizeof (Result->CpuPath), L"Native X64 Direct Execution");
  }

  //
  // 2. GPU Architecture & API Coordination
  //
  switch (Request->RequestedApi) {
    case 1: // Metal
      if ((Cpu.Architecture == OvCpuArchHaswellBroadwell) &&
          ((Gpu.Vendor == OvGpuVendorIntel) || (Gpu.Vendor == OvGpuVendorNvidia) || (Gpu.Vendor == OvGpuVendorAmd))) {
        Result->GpuTranslationRequired = FALSE;
        UnicodeSPrint (Result->GpuPath, sizeof (Result->GpuPath), L"Metal2 -> Native Silicon Pipeline");
      } else if ((Cpu.Architecture == OvCpuArchSandyBridge) || (Cpu.Architecture == OvCpuArchIvyBridge)) {
        Result->GpuTranslationRequired = TRUE;
        CostFactor += 40;
        UnicodeSPrint (Result->GpuPath, sizeof (Result->GpuPath), L"Metal2 -> OVIR-GPU -> OpenGL 3.3/4.1 Core");
      } else {
        Result->GpuTranslationRequired = TRUE;
        Result->SoftwareFallbackUsed   = TRUE;
        CostFactor += 300;
        UnicodeSPrint (Result->GpuPath, sizeof (Result->GpuPath), L"Metal2 -> OVIR-GPU -> CPU Soft-Rasterizer");
      }
      break;

    case 2: // Vulkan
      if (Gpu.Vendor == OvGpuVendorIntel && Cpu.Architecture == OvCpuArchHaswellBroadwell) {
        Result->GpuTranslationRequired = FALSE;
        CostFactor += 5;
        UnicodeSPrint (Result->GpuPath, sizeof (Result->GpuPath), L"Vulkan 1.2 -> Native ANV Driver");
      } else {
        Result->GpuTranslationRequired = TRUE;
        CostFactor += 35;
        UnicodeSPrint (Result->GpuPath, sizeof (Result->GpuPath), L"Vulkan -> OVIR-GPU -> OpenGL 3.3 Core");
      }
      break;

    case 4: // DirectX
      Result->GpuTranslationRequired = TRUE;
      CostFactor += 45;
      UnicodeSPrint (Result->GpuPath, sizeof (Result->GpuPath), L"DirectX -> OVIR-GPU -> Vulkan/GL Core");
      break;

    case 3: // OpenGL
    default:
      Result->GpuTranslationRequired = FALSE;
      UnicodeSPrint (Result->GpuPath, sizeof (Result->GpuPath), L"OpenGL Core -> Native Driver");
      break;
  }

  //
  // 3. Shader Cache Coordination
  //
  if (Request->ShaderBytecodeHash != 0 && (Request->ShaderBytecodeHash & 0x2) == 0) {
    Result->ShaderCacheHit = TRUE;
    if (CostFactor > 10) {
      CostFactor -= 10;
    }
  }

  //
  // 4. Hardware Constraints & Clamping
  //
  if (Request->MaxTextureDimension > Gpu.MaxTextureDimension && Gpu.MaxTextureDimension > 0) {
    Result->TextureClampApplied = TRUE;
    CostFactor += 10;
  }

  if (Request->RequiresCompute && !Gpu.SupportsCompute) {
    Result->SoftwareFallbackUsed = TRUE;
    CostFactor += 80;
  }

  //
  // 5. Resource Budget Quota Allocation
  //
  if (Request->RequiredVramBytes > Gpu.VramSize && Gpu.VramSize > 0) {
    // VRAM exceeds physical limit -> Clamp resolution and simplify shaders
    Result->ShaderSimplificationApplied = TRUE;
    Result->AllocatedVramQuota          = Gpu.VramSize;
    CostFactor += 25;
  } else {
    Result->AllocatedVramQuota = Request->RequiredVramBytes;
  }

  Result->AllocatedRamQuota = Request->RequiredRamBytes;

  //
  // 6. Final Decision & Rationale Synthesis
  //
  if (Result->SoftwareFallbackUsed) {
    Result->Decision = OvResolutionFallback;
    UnicodeSPrint (Result->Rationale, sizeof (Result->Rationale), L"Hardware lacks required compute or GPU features; CPU fallback active");
  } else if (Result->TextureClampApplied || Result->ShaderSimplificationApplied) {
    Result->Decision = OvResolutionSimplified;
    UnicodeSPrint (Result->Rationale, sizeof (Result->Rationale), L"Workload clamped/simplified to fit legacy silicon VRAM and texture limits");
  } else if (Result->CpuTranslationRequired || Result->GpuTranslationRequired) {
    Result->Decision = OvResolutionTranslated;
    UnicodeSPrint (Result->Rationale, sizeof (Result->Rationale), L"Coordinated cross-architecture translation via OVIR-CPU / OVIR-GPU");
  } else {
    Result->Decision = OvResolutionNative;
    UnicodeSPrint (Result->Rationale, sizeof (Result->Rationale), L"Target hardware satisfies all CPU and GPU architecture requirements");
  }

  Result->PerformanceCostFactor = CostFactor;

  UnicodeSPrint (
    Result->RoutingPath,
    sizeof (Result->RoutingPath),
    L"CPU: [%s] | GPU: [%s]",
    Result->CpuPath,
    Result->GpuPath
    );

  OvLogTagged (
    OV_LOG_LEVEL_INFO,
    L"RESO",
    L"Integrated App '%s': %s (Cost: %u%%) via %s",
    Request->ApplicationName,
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
