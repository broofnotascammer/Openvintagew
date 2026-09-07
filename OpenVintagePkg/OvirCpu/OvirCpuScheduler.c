/** @file
  OpenVintage CPU Translation & Workload Scheduler Integration.
  Phase 4 Dynamic Scheduling and Non-Pinning Task Coordination.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/OvirCpuSchedulerLib.h>
#include <Library/OvSchedulerLib.h>
#include <Library/PrintLib.h>
#include <Library/OvLoggerLib.h>

STATIC OVIR_CPU_SCHEDULER_STATS mWorkloadStats;
STATIC BOOLEAN                  mCpuSchedulerInitialized = FALSE;

EFI_STATUS
EFIAPI
OvirCpuSchedulerInitialize (
  VOID
  )
{
  if (mCpuSchedulerInitialized) {
    return EFI_SUCCESS;
  }

  ZeroMem (&mWorkloadStats, sizeof (OVIR_CPU_SCHEDULER_STATS));
  mCpuSchedulerInitialized = TRUE;
  OvLogTagged (OV_LOG_LEVEL_INFO, L"OCPU", L"OVIR-CPU Scheduler Integration Subsystem initialized");
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirCpuScheduleAppTask (
  IN  CONST CHAR16  *Name,
  OUT UINT32        *TaskId
  )
{
  OV_RESOURCE_DESCRIPTOR Resources;
  CHAR16                 FormattedName[32];

  if (Name == NULL || TaskId == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mCpuSchedulerInitialized) {
    OvirCpuSchedulerInitialize ();
  }

  // Dynamic OS scheduling across available cores: CoreAffinityMask = 0 (unpinned)
  ZeroMem (&Resources, sizeof (OV_RESOURCE_DESCRIPTOR));
  Resources.CoreAffinityMask = 0;
  Resources.MemoryQuotaBytes = 4 * 1024 * 1024; // 4MB
  Resources.MaxLatencyMicroseconds = 2000;
  Resources.ThermalBudgetPoints = 20;

  UnicodeSPrint (FormattedName, sizeof (FormattedName), L"APP-%s", Name);
  mWorkloadStats.ApplicationTasksSubmitted++;

  return OvSchedulerSubmitTask (FormattedName, OvPriorityHigh, &Resources, TaskId);
}

EFI_STATUS
EFIAPI
OvirCpuScheduleTranslationTask (
  IN  CONST CHAR16   *Name,
  IN  OVIR_CPU_ARCH  SourceArch,
  IN  OVIR_CPU_ARCH  TargetArch,
  IN  UINT64         GuestPc,
  IN  UINTN          CodeSize,
  OUT UINT32         *TaskId
  )
{
  OV_RESOURCE_DESCRIPTOR Resources;
  CHAR16                 FormattedName[32];

  if (Name == NULL || TaskId == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mCpuSchedulerInitialized) {
    OvirCpuSchedulerInitialize ();
  }

  // Dynamic scheduling across available cores: CoreAffinityMask = 0 (unpinned)
  ZeroMem (&Resources, sizeof (OV_RESOURCE_DESCRIPTOR));
  Resources.CoreAffinityMask = 0;
  Resources.MemoryQuotaBytes = 2 * 1024 * 1024; // 2MB
  Resources.MaxLatencyMicroseconds = 5000;
  Resources.ThermalBudgetPoints = 10;

  UnicodeSPrint (FormattedName, sizeof (FormattedName), L"XLAT-%s", Name);
  mWorkloadStats.TranslationTasksSubmitted++;

  return OvSchedulerSubmitTask (FormattedName, OvPriorityNormal, &Resources, TaskId);
}

EFI_STATUS
EFIAPI
OvirCpuScheduleJitTask (
  IN  CONST CHAR16  *Name,
  IN  UINT64        GuestPc,
  OUT UINT32        *TaskId
  )
{
  OV_RESOURCE_DESCRIPTOR Resources;
  CHAR16                 FormattedName[32];

  if (Name == NULL || TaskId == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mCpuSchedulerInitialized) {
    OvirCpuSchedulerInitialize ();
  }

  // Dynamic scheduling across available cores: CoreAffinityMask = 0 (unpinned)
  ZeroMem (&Resources, sizeof (OV_RESOURCE_DESCRIPTOR));
  Resources.CoreAffinityMask = 0;
  Resources.MemoryQuotaBytes = 1 * 1024 * 1024; // 1MB
  Resources.MaxLatencyMicroseconds = 500;        // Tight latency for interactive JIT
  Resources.ThermalBudgetPoints = 15;

  UnicodeSPrint (FormattedName, sizeof (FormattedName), L"JIT-%s", Name);
  mWorkloadStats.JitTasksSubmitted++;

  return OvSchedulerSubmitTask (FormattedName, OvPriorityHigh, &Resources, TaskId);
}

EFI_STATUS
EFIAPI
OvirCpuScheduleBackgroundTask (
  IN  CONST CHAR16  *Name,
  OUT UINT32        *TaskId
  )
{
  OV_RESOURCE_DESCRIPTOR Resources;
  CHAR16                 FormattedName[32];

  if (Name == NULL || TaskId == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mCpuSchedulerInitialized) {
    OvirCpuSchedulerInitialize ();
  }

  // Dynamic scheduling across available cores: CoreAffinityMask = 0 (unpinned)
  ZeroMem (&Resources, sizeof (OV_RESOURCE_DESCRIPTOR));
  Resources.CoreAffinityMask = 0;
  Resources.MemoryQuotaBytes = 512 * 1024; // 512KB
  Resources.MaxLatencyMicroseconds = 20000; // Tolerant background task
  Resources.ThermalBudgetPoints = 5;

  UnicodeSPrint (FormattedName, sizeof (FormattedName), L"BG-%s", Name);
  mWorkloadStats.BackgroundTasksSubmitted++;

  return OvSchedulerSubmitTask (FormattedName, OvPriorityLow, &Resources, TaskId);
}

EFI_STATUS
EFIAPI
OvirCpuSchedulerDispatchNext (
  OUT UINT32  *DispatchedTaskId
  )
{
  EFI_STATUS Status;

  if (DispatchedTaskId == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = OvSchedulerDispatchNext (DispatchedTaskId);
  if (!EFI_ERROR (Status)) {
    mWorkloadStats.TasksDispatched++;
  }
  return Status;
}

EFI_STATUS
EFIAPI
OvirCpuSchedulerCompleteTask (
  IN UINT32      TaskId,
  IN EFI_STATUS  ExitStatus
  )
{
  EFI_STATUS Status;

  Status = OvSchedulerCompleteTask (TaskId, ExitStatus);
  if (!EFI_ERROR (Status)) {
    mWorkloadStats.TasksCompleted++;
  }
  return Status;
}

VOID
EFIAPI
OvirCpuSchedulerGetStats (
  OUT OVIR_CPU_SCHEDULER_STATS  *Stats
  )
{
  if (Stats != NULL) {
    CopyMem (Stats, &mWorkloadStats, sizeof (OVIR_CPU_SCHEDULER_STATS));
  }
}
