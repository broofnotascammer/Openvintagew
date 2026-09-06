/** @file
  OpenVintage Resource & Workload Scheduler Definition.
  Phase 2 Task Coordination and Resource Allocation Framework.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OV_SCHEDULER_LIB_H_
#define OV_SCHEDULER_LIB_H_

#include <Uefi.h>
#include <Library/OvConfigLib.h>

#define OV_MAX_SCHEDULED_TASKS  64

//
// Workload Priorities
//
typedef enum {
  OvPriorityIdle = 0,
  OvPriorityLow,
  OvPriorityNormal,
  OvPriorityHigh,
  OvPriorityRealtime
} OV_WORKLOAD_PRIORITY;

//
// Workload Lifecycle State
//
typedef enum {
  OvTaskStateFree = 0,
  OvTaskStateQueued,
  OvTaskStateDispatched,
  OvTaskStateRunning,
  OvTaskStateCompleted,
  OvTaskStateFailed,
  OvTaskStateCancelled
} OV_TASK_STATE;

//
// Resource Allocation Descriptor
//
typedef struct {
  UINT64  CoreAffinityMask;
  UINT64  MemoryQuotaBytes;
  UINT32  MaxLatencyMicroseconds;
  UINT32  ThermalBudgetPoints;
} OV_RESOURCE_DESCRIPTOR;

//
// Scheduled Task Entry
//
typedef struct {
  UINT32                  TaskId;
  CHAR16                  Name[32];
  OV_WORKLOAD_PRIORITY    Priority;
  OV_TASK_STATE           State;
  OV_RESOURCE_DESCRIPTOR  Resources;
  UINT64                  EnqueueTimestamp;
  UINT64                  ExecutionDuration;
  EFI_STATUS              ExitCode;
} OV_SCHEDULED_TASK;

//
// Scheduler Performance Metrics
//
typedef struct {
  UINTN   QueuedTasksCount;
  UINTN   CompletedTasksCount;
  UINTN   FailedTasksCount;
  UINT64  TotalDispatchedTasks;
  UINT64  CommittedMemoryBytes;
  OV_PERF_PROFILE ActiveProfile;
} OV_SCHEDULER_METRICS;

/**
  Initialize scheduler subsystem.

  @retval EFI_SUCCESS  Scheduler initialized.
**/
EFI_STATUS
EFIAPI
OvSchedulerInitialize (
  VOID
  );

/**
  Set active performance and power profile.

  @param[in] Profile  Desired profile.
**/
VOID
EFIAPI
OvSchedulerSetProfile (
  IN OV_PERF_PROFILE  Profile
  );

/**
  Retrieve active performance profile.

  @return OV_PERF_PROFILE Current profile.
**/
OV_PERF_PROFILE
EFIAPI
OvSchedulerGetProfile (
  VOID
  );

/**
  Submit a new task to the scheduler queue.

  @param[in]  Name       Task descriptive name.
  @param[in]  Priority   Workload priority.
  @param[in]  Resources  Resource budget parameters.
  @param[out] TaskId     Assigned unique task identifier.

  @retval EFI_SUCCESS    Task queued successfully.
  @retval EFI_OUT_OF_RESOURCES Queue is full.
**/
EFI_STATUS
EFIAPI
OvSchedulerSubmitTask (
  IN  CONST CHAR16                *Name,
  IN  OV_WORKLOAD_PRIORITY        Priority,
  IN  CONST OV_RESOURCE_DESCRIPTOR *Resources,
  OUT UINT32                      *TaskId
  );

/**
  Dispatch the highest priority pending task from queue.

  @param[out] DispatchedTaskId  Receives ID of dispatched task.

  @retval EFI_SUCCESS           Task dispatched.
  @retval EFI_NOT_READY         No tasks queued.
**/
EFI_STATUS
EFIAPI
OvSchedulerDispatchNext (
  OUT UINT32  *DispatchedTaskId
  );

/**
  Mark a dispatched task as completed with status.

  @param[in] TaskId      Task ID.
  @param[in] ExitStatus  Resulting status code.

  @retval EFI_SUCCESS    Task marked completed.
  @retval EFI_NOT_FOUND  Task not found or not running.
**/
EFI_STATUS
EFIAPI
OvSchedulerCompleteTask (
  IN UINT32      TaskId,
  IN EFI_STATUS  ExitStatus
  );

/**
  Query scheduler metrics and queue health.

  @param[out] Metrics  Receives current scheduler metrics.
**/
VOID
EFIAPI
OvSchedulerGetMetrics (
  OUT OV_SCHEDULER_METRICS  *Metrics
  );

/**
  Dump queue state to logger.
**/
VOID
EFIAPI
OvSchedulerDumpQueue (
  VOID
  );

#endif // OV_SCHEDULER_LIB_H_
