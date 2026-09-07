/** @file
  OpenVintage CPU Scheduler Integration Interface.
  Phase 4 Task Categorization and Cooperative Workload Scheduling.

  Connects CPU translation, JIT compilation, application execution, and
  background cache tasks to the OvScheduler subsystem.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OVIR_CPU_SCHEDULER_LIB_H_
#define OVIR_CPU_SCHEDULER_LIB_H_

#include <Uefi.h>
#include <Library/OvSchedulerLib.h>
#include <Library/OvirCpuLib.h>

//
// CPU Translation Workload Classes
//
typedef enum {
  OvirCpuWorkloadApplication = 0, // Running application / guest instructions
  OvirCpuWorkloadTranslation,     // Translation thread converting basic blocks
  OvirCpuWorkloadJit,             // Critical on-demand JIT compilation
  OvirCpuWorkloadBackground       // Background cache pruning and profile analysis
} OVIR_CPU_WORKLOAD_CLASS;

//
// Workload Coordination Statistics
//
typedef struct {
  UINT64  ApplicationTasksSubmitted;
  UINT64  TranslationTasksSubmitted;
  UINT64  JitTasksSubmitted;
  UINT64  BackgroundTasksSubmitted;
  UINT64  TasksDispatched;
  UINT64  TasksCompleted;
} OVIR_CPU_SCHEDULER_STATS;

/**
  Initialize CPU workload scheduler bridge.

  @retval EFI_SUCCESS   Scheduler bridge initialized.
**/
EFI_STATUS
EFIAPI
OvirCpuSchedulerInitialize (
  VOID
  );

/**
  Submit an application workload task.

  @param[in]  Name     Descriptive task name.
  @param[out] TaskId   Receives scheduler task ID.

  @retval EFI_SUCCESS  Task queued.
**/
EFI_STATUS
EFIAPI
OvirCpuScheduleAppTask (
  IN  CONST CHAR16  *Name,
  OUT UINT32        *TaskId
  );

/**
  Submit a block translation task.

  @param[in]  Name        Descriptive name.
  @param[in]  SourceArch  Source architecture.
  @param[in]  TargetArch  Target architecture.
  @param[in]  GuestPc     Guest virtual address.
  @param[in]  CodeSize    Byte size of guest code.
  @param[out] TaskId      Receives scheduler task ID.

  @retval EFI_SUCCESS     Task queued.
**/
EFI_STATUS
EFIAPI
OvirCpuScheduleTranslationTask (
  IN  CONST CHAR16   *Name,
  IN  OVIR_CPU_ARCH  SourceArch,
  IN  OVIR_CPU_ARCH  TargetArch,
  IN  UINT64         GuestPc,
  IN  UINTN          CodeSize,
  OUT UINT32         *TaskId
  );

/**
  Submit a high-priority interactive JIT compilation task.

  @param[in]  Name     Descriptive name.
  @param[in]  GuestPc  Guest PC triggering JIT request.
  @param[out] TaskId   Receives scheduler task ID.

  @retval EFI_SUCCESS  Task queued.
**/
EFI_STATUS
EFIAPI
OvirCpuScheduleJitTask (
  IN  CONST CHAR16  *Name,
  IN  UINT64        GuestPc,
  OUT UINT32        *TaskId
  );

/**
  Submit a background maintenance or cache pruning task.

  @param[in]  Name     Descriptive name.
  @param[out] TaskId   Receives scheduler task ID.

  @retval EFI_SUCCESS  Task queued.
**/
EFI_STATUS
EFIAPI
OvirCpuScheduleBackgroundTask (
  IN  CONST CHAR16  *Name,
  OUT UINT32        *TaskId
  );

/**
  Dispatch the highest priority pending CPU workload from scheduler queue.

  @param[out] DispatchedTaskId  Receives ID of task.

  @retval EFI_SUCCESS           Task dispatched.
  @retval EFI_NOT_READY         Queue empty.
**/
EFI_STATUS
EFIAPI
OvirCpuSchedulerDispatchNext (
  OUT UINT32  *DispatchedTaskId
  );

/**
  Mark a CPU workload task as completed.

  @param[in] TaskId      Task ID to finish.
  @param[in] ExitStatus  Execution result status code.

  @retval EFI_SUCCESS    Task completed.
**/
EFI_STATUS
EFIAPI
OvirCpuSchedulerCompleteTask (
  IN UINT32      TaskId,
  IN EFI_STATUS  ExitStatus
  );

/**
  Query CPU workload scheduler statistics.

  @param[out] Stats   Receives statistics snapshot.
**/
VOID
EFIAPI
OvirCpuSchedulerGetStats (
  OUT OVIR_CPU_SCHEDULER_STATS  *Stats
  );

#endif // OVIR_CPU_SCHEDULER_LIB_H_
