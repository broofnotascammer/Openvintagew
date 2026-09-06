/** @file
  OpenVintage Memory Management & Tracking Abstraction Implementation.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/OvLoggerLib.h>
#include <Library/OvMemoryLib.h>

#define OV_MEM_HEADER_SIGNATURE  0x4D454D54  // 'MEMT'
#define OV_MEM_TAIL_SIGNATURE    0x54454D54  // 'TEMT'

#pragma pack(1)
typedef struct {
  UINT32  Signature;
  UINT32  Tag;
  UINTN   Size;
  UINT32  Checksum;
} OV_MEM_HEADER;
#pragma pack()

STATIC OV_MEMORY_STATS  mMemoryStats = { 0 };
STATIC BOOLEAN          mMemoryInitialized = FALSE;

EFI_STATUS
EFIAPI
OvMemoryInitialize (
  VOID
  )
{
  ZeroMem (&mMemoryStats, sizeof (OV_MEMORY_STATS));
  mMemoryInitialized = TRUE;
  OvLogTagged (OV_LOG_LEVEL_DEBUG, L"MEM", L"Memory subsystem initialized with allocation tracking");
  return EFI_SUCCESS;
}

VOID *
EFIAPI
OvAllocate (
  IN UINTN   Size,
  IN UINT32  Tag
  )
{
  EFI_STATUS     Status;
  UINTN          TotalBytes;
  OV_MEM_HEADER  *Header;
  VOID           *Buffer;

  if (Size == 0) {
    return NULL;
  }

  if (!mMemoryInitialized) {
    OvMemoryInitialize ();
  }

  TotalBytes = sizeof (OV_MEM_HEADER) + Size;
  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  TotalBytes,
                  (VOID **)&Header
                  );
  if (EFI_ERROR (Status) || (Header == NULL)) {
    OvLogTagged (OV_LOG_LEVEL_ERROR, L"MEM", L"Failed to allocate %lu bytes (Tag: 0x%08X)", Size, Tag);
    return NULL;
  }

  Header->Signature = OV_MEM_HEADER_SIGNATURE;
  Header->Tag       = Tag;
  Header->Size      = Size;
  Header->Checksum  = (UINT32)(Header->Signature ^ Header->Tag ^ (UINT32)Header->Size);

  mMemoryStats.CurrentAllocatedBytes += Size;
  mMemoryStats.TotalAllocatedBytes += Size;
  mMemoryStats.CurrentAllocationCount++;
  mMemoryStats.TotalAllocationCount++;

  if (mMemoryStats.CurrentAllocatedBytes > mMemoryStats.PeakAllocatedBytes) {
    mMemoryStats.PeakAllocatedBytes = mMemoryStats.CurrentAllocatedBytes;
  }

  Buffer = (VOID *)(Header + 1);
  return Buffer;
}

VOID *
EFIAPI
OvAllocateZero (
  IN UINTN   Size,
  IN UINT32  Tag
  )
{
  VOID  *Buffer;

  Buffer = OvAllocate (Size, Tag);
  if (Buffer != NULL) {
    ZeroMem (Buffer, Size);
  }
  return Buffer;
}

VOID
EFIAPI
OvFree (
  IN VOID  *Buffer
  )
{
  OV_MEM_HEADER  *Header;
  UINT32         ExpectedChecksum;

  if (Buffer == NULL) {
    return;
  }

  Header = ((OV_MEM_HEADER *)Buffer) - 1;

  if (Header->Signature != OV_MEM_HEADER_SIGNATURE) {
    OvLogTagged (OV_LOG_LEVEL_ERROR, L"MEM", L"Corrupt allocation header at %p - signature mismatch (0x%08X)", Buffer, Header->Signature);
    gBS->FreePool (Buffer);
    return;
  }

  ExpectedChecksum = (UINT32)(Header->Signature ^ Header->Tag ^ (UINT32)Header->Size);
  if (Header->Checksum != ExpectedChecksum) {
    OvLogTagged (OV_LOG_LEVEL_ERROR, L"MEM", L"Corrupt memory checksum at %p", Buffer);
  }

  if (mMemoryStats.CurrentAllocatedBytes >= Header->Size) {
    mMemoryStats.CurrentAllocatedBytes -= Header->Size;
  } else {
    mMemoryStats.CurrentAllocatedBytes = 0;
  }

  mMemoryStats.TotalFreedBytes += Header->Size;
  if (mMemoryStats.CurrentAllocationCount > 0) {
    mMemoryStats.CurrentAllocationCount--;
  }
  mMemoryStats.TotalFreeCount++;

  // Invalidate header signature before freeing
  Header->Signature = 0xDEADBEEF;

  gBS->FreePool ((VOID *)Header);
}

VOID
EFIAPI
OvMemoryGetStats (
  OUT OV_MEMORY_STATS  *Stats
  )
{
  if (Stats != NULL) {
    CopyMem (Stats, &mMemoryStats, sizeof (OV_MEMORY_STATS));
  }
}

VOID
EFIAPI
OvMemoryDumpStats (
  VOID
  )
{
  OvLogTagged (
    OV_LOG_LEVEL_INFO,
    L"MEM",
    L"Memory Stats: Active: %lu bytes (%u blocks) | Peak: %lu bytes | Total: %lu bytes (%u blocks)",
    mMemoryStats.CurrentAllocatedBytes,
    (UINT32)mMemoryStats.CurrentAllocationCount,
    mMemoryStats.PeakAllocatedBytes,
    mMemoryStats.TotalAllocatedBytes,
    (UINT32)mMemoryStats.TotalAllocationCount
    );
}

EFI_STATUS
EFIAPI
OvMemoryVerifyNoLeaks (
  OUT UINTN  *LeakCount
  )
{
  if (LeakCount != NULL) {
    *LeakCount = mMemoryStats.CurrentAllocationCount;
  }

  if (mMemoryStats.CurrentAllocationCount == 0) {
    OvLogTagged (OV_LOG_LEVEL_INFO, L"MEM", L"Memory leak verification: PASS (0 active allocations)");
    return EFI_SUCCESS;
  } else {
    OvLogTagged (
      OV_LOG_LEVEL_WARN,
      L"MEM",
      L"Memory leak verification: DETECTED %u active allocations (%lu bytes unreleased)",
      (UINT32)mMemoryStats.CurrentAllocationCount,
      mMemoryStats.CurrentAllocatedBytes
      );
    return EFI_NOT_READY;
  }
}
