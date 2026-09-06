/** @file
  OpenVintage Resource & Workload Scheduler Implementation.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/OvLoggerLib.h>
#include <Library/OvConfigLib.h>
#include <Library/OvSchedulerLib.h>

STATIC OV_SCHEDULED_TASK     mTasks[OV_MAX_SCHEDULED_TASKS];
STATIC OV_SCHEDULER_METRICS  mMetrics;
STATIC UINT32                mNextTaskId = 1;
STATIC UINT64                mTimeCounter = 1;
STATIC BOOLEAN               mSchedulerInitialized = FALSE;

EFI_STATUS
EFIAPI
OvSchedulerInitialize (
  VOID
  )
{
  ZeroMem (mTasks, sizeof (mTasks));
  ZeroMem (&mMetrics, sizeof (OV_SCHEDULER_METRICS));

  mMetrics.ActiveProfile = OvPerfProfileBalanced;
  mNextTaskId = 1;
  mTimeCounter = 1;
  mSchedulerInitialized = TRUE;

  OvLogTagged (OV_LOG_LEVEL_DEBUG, L"SCHD", L"Scheduler engine initialized (Profile: Balanced)");
  return EFI_SUCCESS;
}

VOID
EFIAPI
OvSchedulerSetProfile (
  IN OV_PERF_PROFILE  Profile
  )
{
  mMetrics.ActiveProfile = Profile;
  OvLogTagged (OV_LOG_LEVEL_INFO, L"SCHD", L"Active performance profile switched to %u", (UINT32)Profile);
}

OV_PERF_PROFILE
EFIAPI
OvSchedulerGetProfile (
  VOID
  )
{
  return mMetrics.ActiveProfile;
}

EFI_STATUS
EFIAPI
OvSchedulerSubmitTask (
  IN  CONST CHAR16                *Name,
  IN  OV_WORKLOAD_PRIORITY        Priority,
  IN  CONST OV_RESOURCE_DESCRIPTOR *Resources,
  OUT UINT32                      *TaskId
  )
{
  UINTN  Index;
  UINTN  FreeSlot;

  if ((Name == NULL) || (TaskId == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mSchedulerInitialized) {
    OvSchedulerInitialize ();
  }

  FreeSlot = OV_MAX_SCHEDULED_TASKS;
  for (Index = 0; Index < OV_MAX_SCHEDULED_TASKS; Index++) {
    if (mTasks[Index].State == OvTaskStateFree) {
      FreeSlot = Index;
      break;
    }
  }

  if (FreeSlot == OV_MAX_SCHEDULED_TASKS) {
    OvLogTagged (OV_LOG_LEVEL_ERROR, L"SCHD", L"Task submission failed: queue capacity exhausted (%u)", OV_MAX_SCHEDULED_TASKS);
    return EFI_OUT_OF_RESOURCES;
  }

  mTasks[FreeSlot].TaskId           = mNextTaskId++;
  mTasks[FreeSlot].Priority         = Priority;
  mTasks[FreeSlot].State            = OvTaskStateQueued;
  mTasks[FreeSlot].EnqueueTimestamp = mTimeCounter++;
  mTasks[FreeSlot].ExitCode         = EFI_NOT_READY;
  StrnCpyS (mTasks[FreeSlot].Name, sizeof (mTasks[FreeSlot].Name) / sizeof (CHAR16), Name, 31);

  if (Resources != NULL) {
    CopyMem (&mTasks[FreeSlot].Resources, Resources, sizeof (OV_RESOURCE_DESCRIPTOR));
    mMetrics.CommittedMemoryBytes += Resources->MemoryQuotaBytes;
  } else {
    ZeroMem (&mTasks[FreeSlot].Resources, sizeof (OV_RESOURCE_DESCRIPTOR));
  }

  mMetrics.QueuedTasksCount++;
  *TaskId = mTasks[FreeSlot].TaskId;

  OvLogTagged (
    OV_LOG_LEVEL_DEBUG,
    L"SCHD",
    L"Queued task [%u] '%s' (Priority %u, MemoryQuota: %lu bytes)",
    *TaskId,
    Name,
    (UINT32)Priority,
    mTasks[FreeSlot].Resources.MemoryQuotaBytes
    );

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvSchedulerDispatchNext (
  OUT UINT32  *DispatchedTaskId
  )
{
  UINTN                 Index;
  INTN                  BestIndex;
  OV_WORKLOAD_PRIORITY  HighestPriority;
  UINT64                EarliestEnqueue;

  if (DispatchedTaskId == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mSchedulerInitialized || (mMetrics.QueuedTasksCount == 0)) {
    return EFI_NOT_READY;
  }

  BestIndex       = -1;
  HighestPriority = OvPriorityIdle;
  EarliestEnqueue = (UINT64)-1;

  for (Index = 0; Index < OV_MAX_SCHEDULED_TASKS; Index++) {
    if (mTasks[Index].State == OvTaskStateQueued) {
      if ((BestIndex == -1) ||
          (mTasks[Index].Priority > HighestPriority) ||
          ((mTasks[Index].Priority == HighestPriority) && (mTasks[Index].EnqueueTimestamp < EarliestEnqueue))) {
        BestIndex       = (INTN)Index;
        HighestPriority = mTasks[Index].Priority;
        EarliestEnqueue = mTasks[Index].EnqueueTimestamp;
      }
    }
  }

  if (BestIndex < 0) {
    return EFI_NOT_READY;
  }

  mTasks[BestIndex].State = OvTaskStateDispatched;
  mMetrics.QueuedTasksCount--;
  mMetrics.TotalDispatchedTasks++;
  *DispatchedTaskId = mTasks[BestIndex].TaskId;

  OvLogTagged (
    OV_LOG_LEVEL_DEBUG,
    L"SCHD",
    L"Dispatched task [%u] '%s' (Priority %u)",
    mTasks[BestIndex].TaskId,
    mTasks[BestIndex].Name,
    (UINT32)mTasks[BestIndex].Priority
    );

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvSchedulerCompleteTask (
  IN UINT32      TaskId,
  IN EFI_STATUS  ExitStatus
  )
{
  UINTN  Index;

  for (Index = 0; Index < OV_MAX_SCHEDULED_TASKS; Index++) {
    if ((mTasks[Index].TaskId == TaskId) &&
        ((mTasks[Index].State == OvTaskStateDispatched) || (mTasks[Index].State == OvTaskStateRunning))) {
      mTasks[Index].ExitCode = ExitStatus;
      if (EFI_ERROR (ExitStatus)) {
        mTasks[Index].State = OvTaskStateFailed;
        mMetrics.FailedTasksCount++;
      } else {
        mTasks[Index].State = OvTaskStateCompleted;
        mMetrics.CompletedTasksCount++;
      }

      if (mMetrics.CommittedMemoryBytes >= mTasks[Index].Resources.MemoryQuotaBytes) {
        mMetrics.CommittedMemoryBytes -= mTasks[Index].Resources.MemoryQuotaBytes;
      } else {
        mMetrics.CommittedMemoryBytes = 0;
      }

      OvLogTagged (
        OV_LOG_LEVEL_DEBUG,
        L"SCHD",
        L"Task [%u] '%s' completed with %r",
        TaskId,
        mTasks[Index].Name,
        ExitStatus
        );

      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

VOID
EFIAPI
OvSchedulerGetMetrics (
  OUT OV_SCHEDULER_METRICS  *Metrics
  )
{
  if (Metrics != NULL) {
    CopyMem (Metrics, &mMetrics, sizeof (OV_SCHEDULER_METRICS));
  }
}

VOID
EFIAPI
OvSchedulerDumpQueue (
  VOID
  )
{
  UINTN         Index;
  CONST CHAR16  *StateStr;

  OvLogTagged (
    OV_LOG_LEVEL_INFO,
    L"SCHD",
    L"Scheduler Metrics: Queued: %u | Completed: %u | Dispatched Total: %lu | Committed Memory: %lu KB",
    (UINT32)mMetrics.QueuedTasksCount,
    (UINT32)mMetrics.CompletedTasksCount,
    mMetrics.TotalDispatchedTasks,
    mMetrics.CommittedMemoryBytes / 1024
    );

  for (Index = 0; Index < OV_MAX_SCHEDULED_TASKS; Index++) {
    if (mTasks[Index].State != OvTaskStateFree) {
      switch (mTasks[Index].State) {
        case OvTaskStateQueued:     StateStr = L"QUEUED"; break;
        case OvTaskStateDispatched: StateStr = L"DISPATCHED"; break;
        case OvTaskStateRunning:    StateStr = L"RUNNING"; break;
        case OvTaskStateCompleted:  StateStr = L"COMPLETED"; break;
        case OvTaskStateFailed:     StateStr = L"FAILED"; break;
        case OvTaskStateCancelled:  StateStr = L"CANCELLED"; break;
        default:                    StateStr = L"UNKNOWN"; break;
      }

      OvLogTagged (
        OV_LOG_LEVEL_INFO,
        L"SCHD",
        L"  [%02u] %-20s | Pri: %u | State: %-10s | Result: %r",
        mTasks[Index].TaskId,
        mTasks[Index].Name,
        (UINT32)mTasks[Index].Priority,
        StateStr,
        mTasks[Index].ExitCode
        );
    }
  }
}
