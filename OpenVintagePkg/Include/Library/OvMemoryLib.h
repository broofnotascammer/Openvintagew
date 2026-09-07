/** @file
  OpenVintage Memory Management & Tracking Abstraction Definition.
  Phase 2 Modular Memory Layer.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OV_MEMORY_LIB_H_
#define OV_MEMORY_LIB_H_

#include <Uefi.h>

//
// Common Memory Allocation Tags
//
#define OV_MEM_TAG_CORE     0x45524F43  // 'CORE'
#define OV_MEM_TAG_CONF     0x464E4F43  // 'CONF'
#define OV_MEM_TAG_HARD     0x44524148  // 'HARD'
#define OV_MEM_TAG_MODU     0x55444F4D  // 'MODU'
#define OV_MEM_TAG_RESO     0x4F534552  // 'RESO'
#define OV_MEM_TAG_SCHD     0x44484353  // 'SCHD'
#define OV_MEM_TAG_TEST     0x54534554  // 'TEST'
#define OV_MEM_TAG_BUFF     0x46465542  // 'BUFF'
#define OV_MEM_TAG_OVIR     0x5249564F  // 'OVIR'
#define OV_MEM_TAG_SHDR     0x52444853  // 'SHDR'
#define OV_MEM_TAG_PIPE     0x45504950  // 'PIPE'
#define OV_MEM_TAG_TEXR     0x52584554  // 'TEXR'

//
// Memory Allocation Statistics
//
typedef struct {
  UINT64  CurrentAllocatedBytes;
  UINT64  PeakAllocatedBytes;
  UINT64  TotalAllocatedBytes;
  UINT64  TotalFreedBytes;
  UINTN   CurrentAllocationCount;
  UINTN   TotalAllocationCount;
  UINTN   TotalFreeCount;
} OV_MEMORY_STATS;

/**
  Initialize memory management subsystem.

  @retval EFI_SUCCESS   Memory subsystem initialized.
**/
EFI_STATUS
EFIAPI
OvMemoryInitialize (
  VOID
  );

/**
  Allocate memory tracked by OpenVintage.

  @param[in] Size       Bytes to allocate.
  @param[in] Tag        32-bit signature tag.

  @return VOID*         Allocated buffer pointer, or NULL on failure.
**/
VOID *
EFIAPI
OvAllocate (
  IN UINTN   Size,
  IN UINT32  Tag
  );

/**
  Allocate zero-initialized memory tracked by OpenVintage.

  @param[in] Size       Bytes to allocate.
  @param[in] Tag        32-bit signature tag.

  @return VOID*         Zeroed allocated buffer pointer, or NULL on failure.
**/
VOID *
EFIAPI
OvAllocateZero (
  IN UINTN   Size,
  IN UINT32  Tag
  );

/**
  Free memory previously allocated via OvAllocate.

  @param[in] Buffer     Buffer pointer to release.
**/
VOID
EFIAPI
OvFree (
  IN VOID  *Buffer
  );

/**
  Retrieve memory usage statistics.

  @param[out] Stats     Receives memory statistics.
**/
VOID
EFIAPI
OvMemoryGetStats (
  OUT OV_MEMORY_STATS  *Stats
  );

/**
  Print memory allocation report and statistics to logger.
**/
VOID
EFIAPI
OvMemoryDumpStats (
  VOID
  );

/**
  Verify all allocations have been freed (leak checking).

  @param[out] LeakCount Number of unfreed allocations detected.

  @retval EFI_SUCCESS   No leaks detected.
  @retval EFI_NOT_READY Subsystem not initialized or leaks present.
**/
EFI_STATUS
EFIAPI
OvMemoryVerifyNoLeaks (
  OUT UINTN  *LeakCount
  );

#endif // OV_MEMORY_LIB_H_
