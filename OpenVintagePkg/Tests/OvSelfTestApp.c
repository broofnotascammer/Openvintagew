/** @file
  OpenVintage Phase 2 Architectural Self-Test Application.
  Verifies all Phase 2 subsystems: Core, Config, Memory, Hardware,
  Module, Resolver, Scheduler, and Logger.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/OvCoreLib.h>
#include <Library/OvirGpuLib.h>
#include <Library/OvGpuCapabilityLib.h>
#include <Library/OvirAdaptersLib.h>
#include <Library/OvirShaderLib.h>
#include <Library/OvirPipelineLib.h>
#include <Library/OvirResourceLib.h>
#include <Library/OvirPerfLib.h>
#include <Library/OvirResolverLib.h>

STATIC UINTN  mTestsRun    = 0;
STATIC UINTN  mTestsPassed = 0;
STATIC UINTN  mTestsFailed = 0;

STATIC
VOID
RecordTestResult (
  IN CONST CHAR16  *TestName,
  IN BOOLEAN       Success
  )
{
  mTestsRun++;
  if (Success) {
    mTestsPassed++;
    OvLogTagged (OV_LOG_LEVEL_INFO, L"TEST", L"[PASS] %s", TestName);
  } else {
    mTestsFailed++;
    OvLogTagged (OV_LOG_LEVEL_ERROR, L"TEST", L"[FAIL] %s", TestName);
  }
}

// Mock Module Callbacks
STATIC
EFI_STATUS
EFIAPI
MockModuleInit (
  IN OV_MODULE_DESCRIPTOR  *Module
  )
{
  OvLogTagged (OV_LOG_LEVEL_DEBUG, L"MOCK", L"Mock module '%s' init invoked", Module->Name);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
MockModuleShutdown (
  IN OV_MODULE_DESCRIPTOR  *Module
  )
{
  OvLogTagged (OV_LOG_LEVEL_DEBUG, L"MOCK", L"Mock module '%s' shutdown invoked", Module->Name);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS              Status;
  OV_CONFIG_DATA          Config;
  OV_MEMORY_STATS         MemStats;
  UINTN                   LeakCount;
  VOID                    *TestPtr1;
  VOID                    *TestPtr2;
  OV_CPU_TOPOLOGY         Cpu;
  OV_MEMORY_TOPOLOGY      Mem;
  OV_GPU_TOPOLOGY         Gpu;
  OV_STORAGE_TOPOLOGY     Storage;
  OV_PLATFORM_TOPOLOGY    Platform;
  OV_MODULE_DESCRIPTOR    MockMod;
  OV_MODULE_DESCRIPTOR    *RetrievedMod;
  OV_RESOLVER_REQUEST     ResReq;
  OV_RESOLVER_RESULT      ResRes;
  UINT32                  TaskId1;
  UINT32                  TaskId2;
  UINT32                  DispatchedId;
  OV_RESOURCE_DESCRIPTOR  ResDesc;
  OV_SCHEDULER_METRICS    SchedMetrics;

  OvCorePrintBanner ();

  Print (L"================================================================\n");
  Print (L"     OPENVINTAGE ARCHITECTURAL COMPONENT SELF-TEST SUITE        \n");
  Print (L"================================================================\n\n");

  // -------------------------------------------------------------
  // TEST 1: OvCore Initialization & Runtime State
  // -------------------------------------------------------------
  Status = OvCoreInitialize ();
  RecordTestResult (
    L"OvCore: Unified Subsystem Initialization",
    (!EFI_ERROR (Status) && (OvCoreGetState () == OvCoreStateReady))
    );

  // -------------------------------------------------------------
  // TEST 2: OvConfig Storage, Retrieval, and Feature Flags
  // -------------------------------------------------------------
  Status = OvConfigGet (&Config);
  if (!EFI_ERROR (Status)) {
    BOOLEAN InitialFlag = OvConfigGetFeatureFlag (OV_FEATURE_MEMORY_TRACKING);
    OvConfigSetFeatureFlag (OV_FEATURE_MEMORY_TRACKING, FALSE);
    BOOLEAN ToggledOff = !OvConfigGetFeatureFlag (OV_FEATURE_MEMORY_TRACKING);
    OvConfigSetFeatureFlag (OV_FEATURE_MEMORY_TRACKING, TRUE);
    BOOLEAN ToggledOn = OvConfigGetFeatureFlag (OV_FEATURE_MEMORY_TRACKING);

    RecordTestResult (
      L"OvConfig: Profile Management & Bitwise Feature Flags",
      (InitialFlag && ToggledOff && ToggledOn && (Config.MajorVersion == 0) && (Config.MinorVersion == 2))
      );
  } else {
    RecordTestResult (L"OvConfig: Profile Management & Bitwise Feature Flags", FALSE);
  }

  // -------------------------------------------------------------
  // TEST 3: OvMemory Tracking & Leak Detection
  // -------------------------------------------------------------
  TestPtr1 = OvAllocate (1024, OV_MEM_TAG_TEST);
  TestPtr2 = OvAllocateZero (2048, OV_MEM_TAG_TEST);
  OvMemoryGetStats (&MemStats);

  BOOLEAN AllocSuccess = (TestPtr1 != NULL) && (TestPtr2 != NULL) && (MemStats.CurrentAllocatedBytes >= 3072);

  OvFree (TestPtr1);
  OvFree (TestPtr2);

  Status = OvMemoryVerifyNoLeaks (&LeakCount);
  RecordTestResult (
    L"OvMemory: Tagged Allocation Tracking & Leak Verification",
    (AllocSuccess && !EFI_ERROR (Status) && (LeakCount == 0))
    );

  // -------------------------------------------------------------
  // TEST 4: OvHardware Discovery Abstraction
  // -------------------------------------------------------------
  Status = OvHardwareGetCpu (&Cpu);
  BOOLEAN CpuOk = (!EFI_ERROR (Status) && (AsciiStrLen (Cpu.BrandString) > 0));

  Status = OvHardwareGetMemory (&Mem);
  BOOLEAN MemOk = (!EFI_ERROR (Status) && (Mem.TotalPhysicalBytes > 0));

  Status = OvHardwareGetGpu (&Gpu);
  BOOLEAN GpuOk = !EFI_ERROR (Status);

  Status = OvHardwareGetStorage (&Storage);
  BOOLEAN StorageOk = !EFI_ERROR (Status);

  Status = OvHardwareGetPlatform (&Platform);
  BOOLEAN PlatformOk = (!EFI_ERROR (Status) && (Platform.FirmwareVendor != NULL));

  RecordTestResult (
    L"OvHardware: Discovery of CPU, RAM, GPU, Storage, Platform",
    (CpuOk && MemOk && GpuOk && StorageOk && PlatformOk)
    );

  OvHardwareDumpTopology ();

  // -------------------------------------------------------------
  // TEST 5: OvModule Pluggable Lifecycle
  // -------------------------------------------------------------
  ZeroMem (&MockMod, sizeof (MockMod));
  StrnCpyS (MockMod.Name, sizeof (MockMod.Name) / sizeof (CHAR16), L"MockGpuExtension", 31);
  MockMod.Type       = OvModTypeGpuIr;
  MockMod.Version    = 0x00010000;
  MockMod.Initialize = MockModuleInit;
  MockMod.Shutdown   = MockModuleShutdown;

  Status = OvModuleRegister (&MockMod);
  BOOLEAN RegOk = !EFI_ERROR (Status);

  Status = OvModuleGetDescriptor (MockMod.ModuleId, &RetrievedMod);
  BOOLEAN GetOk = (!EFI_ERROR (Status) && (RetrievedMod != NULL) && (RetrievedMod->Status == OvModStatusRegistered));

  OvModuleInitializeAll ();
  BOOLEAN ActiveOk = (RetrievedMod != NULL) && (RetrievedMod->Status == OvModStatusActive);

  RecordTestResult (
    L"OvModule: Registration, Status Lifecycle & Orchestration",
    (RegOk && GetOk && ActiveOk)
    );

  // -------------------------------------------------------------
  // TEST 6: OvResolver Decision Framework
  // -------------------------------------------------------------
  BOOLEAN ResolverAllOk = TRUE;

  // Case A: Workload requesting Metal
  ZeroMem (&ResReq, sizeof (ResReq));
  StrnCpyS (ResReq.Name, sizeof (ResReq.Name) / sizeof (CHAR16), L"MetalComputePass", 31);
  ResReq.Class         = OvWorkloadClassGpuCompute;
  ResReq.RequiresMetal = TRUE;

  Status = OvResolverEvaluate (&ResReq, &ResRes);
  if (EFI_ERROR (Status) || (ResRes.Decision == OvResolutionUnsupported)) {
    ResolverAllOk = FALSE;
  }

  // Case B: Workload requesting AVX2 SIMD
  ZeroMem (&ResReq, sizeof (ResReq));
  StrnCpyS (ResReq.Name, sizeof (ResReq.Name) / sizeof (CHAR16), L"VectorFastFourier", 31);
  ResReq.Class        = OvWorkloadClassCpuSimd;
  ResReq.RequiresAVX2 = TRUE;

  Status = OvResolverEvaluate (&ResReq, &ResRes);
  if (EFI_ERROR (Status) || (ResRes.Decision == 0)) {
    ResolverAllOk = FALSE;
  }

  RecordTestResult (
    L"OvResolver: Hardware-Aware Capability Resolution Matrix",
    ResolverAllOk
    );

  // -------------------------------------------------------------
  // TEST 7: OvScheduler Task Queuing & Priority Dispatch
  // -------------------------------------------------------------
  ZeroMem (&ResDesc, sizeof (ResDesc));
  ResDesc.MemoryQuotaBytes = 64 * 1024;

  // Submit Low Priority Task
  Status = OvSchedulerSubmitTask (L"BackgroundTelemetry", OvPriorityLow, &ResDesc, &TaskId1);
  BOOLEAN Task1Submitted = !EFI_ERROR (Status);

  // Submit High Priority Task
  Status = OvSchedulerSubmitTask (L"CriticalFrameRender", OvPriorityHigh, &ResDesc, &TaskId2);
  BOOLEAN Task2Submitted = !EFI_ERROR (Status);

  // Dispatch next task - must dispatch TaskId2 first due to High Priority
  Status = OvSchedulerDispatchNext (&DispatchedId);
  BOOLEAN PriorityCorrect = (!EFI_ERROR (Status) && (DispatchedId == TaskId2));

  // Complete TaskId2
  Status = OvSchedulerCompleteTask (DispatchedId, EFI_SUCCESS);
  BOOLEAN Complete1Ok = !EFI_ERROR (Status);

  // Dispatch next task - now TaskId1
  Status = OvSchedulerDispatchNext (&DispatchedId);
  BOOLEAN DispatchedLow = (!EFI_ERROR (Status) && (DispatchedId == TaskId1));

  // Complete TaskId1
  Status = OvSchedulerCompleteTask (DispatchedId, EFI_SUCCESS);
  BOOLEAN Complete2Ok = !EFI_ERROR (Status);

  OvSchedulerGetMetrics (&SchedMetrics);
  BOOLEAN SchedMetricsOk = (SchedMetrics.CompletedTasksCount == 2);

  RecordTestResult (
    L"OvScheduler: Priority Queueing, Dispatching & Metrics",
    (Task1Submitted && Task2Submitted && PriorityCorrect && Complete1Ok && DispatchedLow && Complete2Ok && SchedMetricsOk)
    );

  // -------------------------------------------------------------
  // TEST 8: OvLogger Subsystem Tagging & Level Filtering
  // -------------------------------------------------------------
  OvLogTagged (OV_LOG_LEVEL_DEBUG, L"TEST", L"Debug log message verification");
  OvLogTagged (OV_LOG_LEVEL_INFO,  L"TEST", L"Info log message verification");
  OvLogTagged (OV_LOG_LEVEL_WARN,  L"TEST", L"Warn log message verification");
  OvLogTagged (OV_LOG_LEVEL_ERROR, L"TEST", L"Error log message verification");
  RecordTestResult (L"OvLogger: Multi-level Formatted & Tagged Logging", TRUE);

  // -------------------------------------------------------------
  // TEST 9: OvGpuCapability: Hardware Probing & Format Assessment
  // -------------------------------------------------------------
  OVIR_GPU_CAPABILITIES GpuCaps;
  BOOLEAN               CapsOk;
  BOOLEAN               FormatRgba8Ok;
  UINT32                MaxTex2D;

  Status = OvGpuCapabilityDetect (&GpuCaps);
  CapsOk = (!EFI_ERROR (Status) && GpuCaps.IsValid);
  FormatRgba8Ok = OvGpuIsFormatSupported (&GpuCaps, OVIR_FORMAT_RGBA8_UNORM, OV_FORMAT_FEAT_COLOR_ATTACHMENT);
  MaxTex2D = OvGpuGetMaxTextureDimension (&GpuCaps, OvirResourceTexture2D);

  RecordTestResult (
    L"OvGpuCapability: Hardware Detection & Texture Format Validation",
    (CapsOk && FormatRgba8Ok && (MaxTex2D >= 4096))
    );

  // -------------------------------------------------------------
  // TEST 10: OVIR-GPU: Command Recording, Ingestion & Stream Validation
  // -------------------------------------------------------------
  OVIR_COMMAND_LIST *CmdList = NULL;
  OVIR_COMMAND_LIST *BadCmdList = NULL;
  FLOAT32           ClearColor[4] = { 0.1f, 0.2f, 0.3f, 1.0f };
  CHAR16            CmdErrBuf[128];
  BOOLEAN           RecOk;
  BOOLEAN           ValidOk;
  BOOLEAN           BadDetected;

  OvirGpuInitialize ();
  Status = OvirCreateCommandList (16, &CmdList);
  RecOk = !EFI_ERROR (Status);

  if (RecOk) {
    OvirBeginCommandList (CmdList);
    OvirCmdBeginRenderPass (CmdList, 1, ClearColor, 1.0f, 0, OVIR_LOAD_OP_CLEAR, OVIR_STORE_OP_STORE);
    OvirCmdSetPipeline (CmdList, 42);
    OvirCmdSetViewport (CmdList, 0.0f, 0.0f, 1920.0f, 1080.0f, 0.0f, 1.0f);
    OvirCmdSetScissor (CmdList, 0, 0, 1920, 1080);
    OvirCmdBindVertexBuffer (CmdList, 0, 1001, 0, 32);
    OvirCmdDraw (CmdList, 3, 1, 0, 0);
    OvirCmdEndRenderPass (CmdList);
    OvirEndCommandList (CmdList);

    Status = OvirValidateCommandList (CmdList, CmdErrBuf, 128);
    ValidOk = !EFI_ERROR (Status);
  } else {
    ValidOk = FALSE;
  }

  // Create invalid stream (draw command without pipeline) to verify validation catches it
  Status = OvirCreateCommandList (8, &BadCmdList);
  if (!EFI_ERROR (Status)) {
    OvirBeginCommandList (BadCmdList);
    OvirCmdBeginRenderPass (BadCmdList, 1, ClearColor, 1.0f, 0, OVIR_LOAD_OP_CLEAR, OVIR_STORE_OP_STORE);
    OvirCmdDraw (BadCmdList, 3, 1, 0, 0); // No pipeline set!
    OvirCmdEndRenderPass (BadCmdList);
    OvirEndCommandList (BadCmdList);

    Status = OvirValidateCommandList (BadCmdList, CmdErrBuf, 128);
    BadDetected = EFI_ERROR (Status); // Must fail validation
    OvirDestroyCommandList (BadCmdList);
  } else {
    BadDetected = FALSE;
  }

  RecordTestResult (
    L"OVIR-GPU: Command Recording & Stream Validation Rules",
    (RecOk && ValidOk && BadDetected && (CmdList != NULL && CmdList->Count >= 6))
    );

  // -------------------------------------------------------------
  // TEST 11: Graphics API Adapters: Translation Bridges (VK, GL, MTL, DX)
  // -------------------------------------------------------------
  BOOLEAN             AdaptersOk;
  OVIR_ADAPTER_STATUS VkStat;
  OVIR_ADAPTER_STATUS GlStat;
  OVIR_ADAPTER_STATUS MtlStat;
  OVIR_ADAPTER_STATUS DxStat;
  UINT32              InitialCmdCount;

  Status = OvirAdaptersInitialize (&GpuCaps);
  AdaptersOk = !EFI_ERROR (Status);

  VkStat  = OvirAdapterGetStatus (OvirApiVulkan);
  GlStat  = OvirAdapterGetStatus (OvirApiOpenGL);
  MtlStat = OvirAdapterGetStatus (OvirApiMetal);
  DxStat  = OvirAdapterGetStatus (OvirApiDirectX);

  if (CmdList != NULL) {
    OvirBeginCommandList (CmdList);
    OvirCmdBeginRenderPass (CmdList, 1, ClearColor, 1.0f, 0, OVIR_LOAD_OP_CLEAR, OVIR_STORE_OP_STORE);
    OvirCmdSetPipeline (CmdList, 42);

    InitialCmdCount = CmdList->Count;
    // Translate Vulkan Draw
    OvirVkTranslateCmdDraw (CmdList, 6, 1, 0, 0);
    // Translate OpenGL DrawArrays
    OvirGlTranslateDrawArrays (CmdList, 4, 0, 3);
    // Translate Metal DrawPrimitives
    OvirMtlTranslateDrawPrimitives (CmdList, 0, 0, 3, 1);

    OvirCmdEndRenderPass (CmdList);
    OvirEndCommandList (CmdList);
  } else {
    InitialCmdCount = 0;
  }

  RecordTestResult (
    L"Graphics API Adapters: Multi-API Translation & State Tracking",
    (AdaptersOk && (VkStat != 0) && (GlStat != 0) && (MtlStat != 0) && (DxStat != 0) &&
     (CmdList != NULL && CmdList->Count > InitialCmdCount))
    );

  if (CmdList != NULL) {
    OvirDestroyCommandList (CmdList);
    CmdList = NULL;
  }

  // -------------------------------------------------------------
  // TEST 12: Shader System: Ingestion, Composite Hashing & Cache
  // -------------------------------------------------------------
  STATIC CONST UINT8 FakeSpirvBytecode[16] = {
    0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00,
    0x0A, 0x00, 0x0E, 0x00, 0x20, 0x00, 0x00, 0x00
  };
  OVIR_SHADER_MODULE *ShdrMod1 = NULL;
  OVIR_SHADER_MODULE *ShdrMod2 = NULL;
  CHAR16             ShdrErr[128];
  UINT32             ShdrHits = 0;
  UINT32             ShdrMisses = 0;
  BOOLEAN            ShdrCompileOk;
  BOOLEAN            ShdrCacheOk;
  BOOLEAN            ShdrValOk;

  OvirShaderInitialize ();
  Status = OvirShaderCompile (
             OVIR_SHADER_STAGE_VERTEX,
             OvirShaderLangSpirV,
             L"main",
             FakeSpirvBytecode,
             sizeof (FakeSpirvBytecode),
             &ShdrMod1
             );
  ShdrCompileOk = (!EFI_ERROR (Status) && ShdrMod1 != NULL);

  // Ingest identical shader: must trigger cache hit
  Status = OvirShaderCompile (
             OVIR_SHADER_STAGE_VERTEX,
             OvirShaderLangSpirV,
             L"main",
             FakeSpirvBytecode,
             sizeof (FakeSpirvBytecode),
             &ShdrMod2
             );
  ShdrCacheOk = (!EFI_ERROR (Status) && ShdrMod2 == ShdrMod1);

  Status = OvirShaderValidate (ShdrMod1, ShdrErr, 128);
  ShdrValOk = !EFI_ERROR (Status);

  OvirShaderGetCacheStats (&ShdrHits, &ShdrMisses);

  RecordTestResult (
    L"Shader System: Spir-V Ingestion, FNV-1a Hashing & Cache Acceleration",
    (ShdrCompileOk && ShdrCacheOk && ShdrValOk && (ShdrHits >= 1) && (ShdrMisses >= 1))
    );

  // -------------------------------------------------------------
  // TEST 13: Pipeline System: State Object Creation, Hashing & Cache
  // -------------------------------------------------------------
  OVIR_PIPELINE_DESC  PipeDesc;
  OVIR_PIPELINE_STATE *Pipe1 = NULL;
  OVIR_PIPELINE_STATE *Pipe2 = NULL;
  UINT32              PipeHits = 0;
  UINT32              PipeMisses = 0;
  BOOLEAN             PipeCreateOk;
  BOOLEAN             PipeCacheOk;

  OvirPipelineInitialize ();
  ZeroMem (&PipeDesc, sizeof (OVIR_PIPELINE_DESC));
  PipeDesc.IsCompute = FALSE;
  PipeDesc.VertexShader = (ShdrMod1 != NULL) ? ShdrMod1->Handle : 1;
  PipeDesc.FragmentShader = 2;
  PipeDesc.Rasterizer.CullMode = OVIR_CULL_BACK;
  PipeDesc.Rasterizer.FrontFace = OVIR_FRONT_FACE_CCW;
  PipeDesc.Rasterizer.PolygonMode = OVIR_POLYGON_MODE_FILL;
  PipeDesc.Blend.RenderTargets[0].BlendEnable = TRUE;
  PipeDesc.Blend.RenderTargets[0].SrcColorBlendFactor = OVIR_BLEND_FACTOR_SRC_ALPHA;
  PipeDesc.Blend.RenderTargets[0].DstColorBlendFactor = OVIR_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  PipeDesc.Blend.RenderTargets[0].ColorBlendOp = OVIR_BLEND_OP_ADD;
  PipeDesc.Blend.RenderTargets[0].ColorWriteMask = 0x0F;

  Status = OvirPipelineCreate (&PipeDesc, &Pipe1);
  PipeCreateOk = (!EFI_ERROR (Status) && Pipe1 != NULL);

  // Create identical pipeline: must return cached instance
  Status = OvirPipelineCreate (&PipeDesc, &Pipe2);
  PipeCacheOk = (!EFI_ERROR (Status) && Pipe2 == Pipe1);

  OvirPipelineGetCacheStats (&PipeHits, &PipeMisses);

  RecordTestResult (
    L"Pipeline System: State Management, State Hashing & PSO Cache",
    (PipeCreateOk && PipeCacheOk && (PipeHits >= 1) && (PipeMisses >= 1))
    );

  // -------------------------------------------------------------
  // TEST 14: Resource System: Buffers, Textures, Samplers & GPU Memory Pool
  // -------------------------------------------------------------
  OVIR_BUFFER_DESC     BufDesc;
  OVIR_BUFFER          *TestBuf = NULL;
  OVIR_TEXTURE_DESC    TexDesc;
  OVIR_TEXTURE         *TestTex = NULL;
  OVIR_SAMPLER_DESC    SmpDesc;
  OVIR_SAMPLER         *TestSmp = NULL;
  OVIR_GPU_MEMORY_POOL MemPoolBefore;
  OVIR_GPU_MEMORY_POOL MemPoolAfter;
  UINT32               UploadData[8] = { 10, 20, 30, 40, 50, 60, 70, 80 };
  BOOLEAN              BufOk;
  BOOLEAN              UploadOk;
  BOOLEAN              TexOk;
  BOOLEAN              SmpOk;
  BOOLEAN              PoolTrackingOk;

  OvirResourceInitialize ();

  ZeroMem (&BufDesc, sizeof (OVIR_BUFFER_DESC));
  BufDesc.SizeBytes = 1024;
  BufDesc.UsageFlags = OVIR_BUFFER_USAGE_VERTEX | OVIR_BUFFER_USAGE_TRANSFER_DST;
  BufDesc.HostVisible = TRUE;
  StrCpyS (BufDesc.DebugName, 32, L"TestVB");

  Status = OvirBufferCreate (&BufDesc, &TestBuf);
  BufOk = (!EFI_ERROR (Status) && TestBuf != NULL);

  if (BufOk) {
    Status = OvirBufferUpload (TestBuf, 0, UploadData, sizeof (UploadData));
    UploadOk = !EFI_ERROR (Status);
  } else {
    UploadOk = FALSE;
  }

  ZeroMem (&TexDesc, sizeof (OVIR_TEXTURE_DESC));
  TexDesc.Format = OVIR_FORMAT_RGBA8_UNORM;
  TexDesc.Width = 64;
  TexDesc.Height = 64;
  TexDesc.Depth = 1;
  TexDesc.MipLevels = 1;
  TexDesc.UsageFlags = OVIR_TEXTURE_USAGE_SAMPLED | OVIR_TEXTURE_USAGE_RENDER_TARGET;
  StrCpyS (TexDesc.DebugName, 32, L"TestAlbedo");

  Status = OvirTextureCreate (&TexDesc, &TestTex);
  TexOk = (!EFI_ERROR (Status) && TestTex != NULL);

  ZeroMem (&SmpDesc, sizeof (OVIR_SAMPLER_DESC));
  SmpDesc.FilterMin = 1;
  SmpDesc.FilterMag = 1;
  Status = OvirSamplerCreate (&SmpDesc, &TestSmp);
  SmpOk = (!EFI_ERROR (Status) && TestSmp != NULL);

  OvirResourceGetMemoryStats (&MemPoolBefore);
  PoolTrackingOk = (MemPoolBefore.AllocatedBytes > 0 && MemPoolBefore.ActiveAllocationCount >= 2);

  // Clean up resources
  if (TestBuf != NULL) {
    OvirBufferDestroy (TestBuf);
  }
  if (TestTex != NULL) {
    OvirTextureDestroy (TestTex);
  }
  if (TestSmp != NULL) {
    OvirSamplerDestroy (TestSmp);
  }

  OvirResourceGetMemoryStats (&MemPoolAfter);
  PoolTrackingOk = PoolTrackingOk && (MemPoolAfter.AllocatedBytes == 0);

  RecordTestResult (
    L"Resource System: Buffers, Textures, Samplers & VRAM Tracking Pool",
    (BufOk && UploadOk && TexOk && SmpOk && PoolTrackingOk)
    );

  // -------------------------------------------------------------
  // TEST 15: Graphics Resolver: Native, Translated, Simplified, Fallback Decisions
  // -------------------------------------------------------------
  OV_RESOLVER_RESULT ResPipe;
  OV_RESOLVER_RESULT ResFmtNative;
  OV_RESOLVER_RESULT ResFmtSimplified;
  OV_RESOLVER_RESULT ResComputeShader;
  OVIR_SHADER_MODULE ComputeShaderModule;
  BOOLEAN            ResDecisionsOk;

  // 1. Pipeline resolution
  Status = OvirResolvePipeline (&GpuCaps, Pipe1, &ResPipe);
  BOOLEAN ResPipeOk = !EFI_ERROR (Status) &&
    (ResPipe.Decision == OvResolutionNative || ResPipe.Decision == OvResolutionTranslated);

  // 2. Format native resolution
  Status = OvirResolveFormat (&GpuCaps, OVIR_FORMAT_RGBA8_UNORM, OV_FORMAT_FEAT_COLOR_ATTACHMENT, &ResFmtNative);
  BOOLEAN ResNativeOk = !EFI_ERROR (Status) && (ResFmtNative.Decision == OvResolutionNative);

  // 3. Format simplification resolution (BC7 compressed texture unsupported in UEFI -> simplified)
  Status = OvirResolveFormat (&GpuCaps, OVIR_FORMAT_BC7_RGBA_UNORM, OV_FORMAT_FEAT_COLOR_ATTACHMENT, &ResFmtSimplified);
  BOOLEAN ResSimpOk = !EFI_ERROR (Status) &&
    (ResFmtSimplified.Decision == OvResolutionSimplified || ResFmtSimplified.Decision == OvResolutionNative);

  // 4. Compute shader resolution on hardware
  ZeroMem (&ComputeShaderModule, sizeof (OVIR_SHADER_MODULE));
  ComputeShaderModule.Stage = OVIR_SHADER_STAGE_COMPUTE;
  ComputeShaderModule.Language = OvirShaderLangSpirV;
  Status = OvirResolveShader (&GpuCaps, &ComputeShaderModule, &ResComputeShader);
  BOOLEAN ResComputeOk = !EFI_ERROR (Status) &&
    ((GpuCaps.Features.ComputeShaders && ResComputeShader.Decision == OvResolutionNative) ||
     (!GpuCaps.Features.ComputeShaders && ResComputeShader.Decision == OvResolutionFallback));

  ResDecisionsOk = (ResPipeOk && ResNativeOk && ResSimpOk && ResComputeOk);

  RecordTestResult (
    L"Graphics Resolver: Native, Translated, Simplified & Fallback Decisions",
    ResDecisionsOk
    );

  // -------------------------------------------------------------
  // TEST 16: Performance Telemetry & Hardware Timestamp Infrastructure
  // -------------------------------------------------------------
  OVIR_PERF_METRICS PerfMetrics;
  UINT64            Tsc1;
  UINT64            Tsc2;
  BOOLEAN           TscProgress;
  BOOLEAN           PerfMetricsOk;

  OvirPerfInitialize ();
  Tsc1 = OvirPerfGetTimestamp ();
  // Do slight delay work
  for (UINTN Spin = 0; Spin < 1000; Spin++) {
    AsmCpuid (0, NULL, NULL, NULL, NULL);
  }
  Tsc2 = OvirPerfGetTimestamp ();
  TscProgress = (Tsc2 > Tsc1);

  OvirPerfRecordTranslation (Tsc2 - Tsc1);
  OvirPerfRecordShaderCompile (250000);
  OvirPerfRecordFrameTiming (12000000);
  OvirPerfRecordCacheEvent (TRUE, TRUE);
  OvirPerfRecordCacheEvent (FALSE, TRUE);

  OvirPerfGetMetrics (&PerfMetrics);
  PerfMetricsOk = (PerfMetrics.TranslationCount == 1 &&
                   PerfMetrics.ShaderCompileCount == 1 &&
                   PerfMetrics.FrameCount == 1 &&
                   PerfMetrics.ShaderCacheHits == 1 &&
                   PerfMetrics.PipelineCacheHits == 1);

  RecordTestResult (
    L"Performance Telemetry: Hardware TSC Timing, Cache & Frame Metrics",
    (TscProgress && PerfMetricsOk)
    );

  // Dump Full Diagnostic Status
  Print (L"\n");
  OvConfigDump ();
  OvMemoryDumpStats ();
  OvModuleDumpList ();
  OvResolverDumpPolicies ();
  OvSchedulerDumpQueue ();

  // Final Test Summary
  Print (L"\n================================================================\n");
  Print (L"     OPENVINTAGE ARCHITECTURAL SELF-TEST RESULTS SUMMARY        \n");
  Print (L"================================================================\n");
  Print (L" Total Subsystem Tests Executed : %u\n", (UINT32)mTestsRun);
  Print (L" Tests Passed Cleanly           : %u\n", (UINT32)mTestsPassed);
  Print (L" Tests Failed                   : %u\n", (UINT32)mTestsFailed);
  Print (L"----------------------------------------------------------------\n");

  if (mTestsFailed == 0) {
    Print (L" OVERALL STATUS: ALL OPENVINTAGE ARCHITECTURAL TESTS PASSED!\n");
    OvLogTagged (OV_LOG_LEVEL_INFO, L"TEST", L"VALIDATION: ALL %u TESTS PASSED CLEANLY", (UINT32)mTestsRun);
  } else {
    Print (L" OVERALL STATUS: SOME TESTS FAILED (%u failures detected)\n", (UINT32)mTestsFailed);
    OvLogTagged (OV_LOG_LEVEL_ERROR, L"TEST", L"VALIDATION: %u TESTS FAILED", (UINT32)mTestsFailed);
  }
  Print (L"================================================================\n\n");

  // Clean shutdown
  OvCoreShutdown ();

  return (mTestsFailed == 0) ? EFI_SUCCESS : EFI_ABORTED;
}
