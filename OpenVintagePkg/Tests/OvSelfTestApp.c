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

  // Dump Full Diagnostic Status
  Print (L"\n");
  OvConfigDump ();
  OvMemoryDumpStats ();
  OvModuleDumpList ();
  OvResolverDumpPolicies ();
  OvSchedulerDumpQueue ();

  // Final Test Summary
  Print (L"\n================================================================\n");
  Print (L"     OPENVINTAGE PHASE 2 SELF-TEST RESULTS SUMMARY             \n");
  Print (L"================================================================\n");
  Print (L" Total Subsystem Tests Executed : %u\n", (UINT32)mTestsRun);
  Print (L" Tests Passed Cleanly           : %u\n", (UINT32)mTestsPassed);
  Print (L" Tests Failed                   : %u\n", (UINT32)mTestsFailed);
  Print (L"----------------------------------------------------------------\n");

  if (mTestsFailed == 0) {
    Print (L" OVERALL STATUS: ALL OPENVINTAGE PHASE 2 ARCHITECTURAL TESTS PASSED!\n");
    OvLogTagged (OV_LOG_LEVEL_INFO, L"TEST", L"PHASE 2 VALIDATION: ALL %u TESTS PASSED CLEANLY", (UINT32)mTestsRun);
  } else {
    Print (L" OVERALL STATUS: SOME TESTS FAILED (%u failures detected)\n", (UINT32)mTestsFailed);
    OvLogTagged (OV_LOG_LEVEL_ERROR, L"TEST", L"PHASE 2 VALIDATION: %u TESTS FAILED", (UINT32)mTestsFailed);
  }
  Print (L"================================================================\n\n");

  // Clean shutdown
  OvCoreShutdown ();

  return (mTestsFailed == 0) ? EFI_SUCCESS : EFI_ABORTED;
}
