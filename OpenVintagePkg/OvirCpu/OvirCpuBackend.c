/** @file
  OpenVintage Modular CPU Translation Backend Registry and Dispatcher.
  Phase 4 Architecture-Agnostic CPU Translation Framework.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/OvirCpuBackendLib.h>

#define MAX_REGISTERED_BACKENDS 8

STATIC CONST OVIR_CPU_BACKEND_INTERFACE *mRegisteredBackends[MAX_REGISTERED_BACKENDS];
STATIC UINTN                            mRegisteredBackendCount = 0;
STATIC BOOLEAN                          mBackendsInitialized = FALSE;

EFI_STATUS
EFIAPI
OvirCpuRegisterBackend (
  IN CONST OVIR_CPU_BACKEND_INTERFACE  *Backend
  )
{
  UINTN Idx;

  if (Backend == NULL || Backend->EmitInstruction == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  for (Idx = 0; Idx < mRegisteredBackendCount; Idx++) {
    if (mRegisteredBackends[Idx]->Architecture == Backend->Architecture) {
      mRegisteredBackends[Idx] = Backend;
      return EFI_SUCCESS;
    }
  }

  if (mRegisteredBackendCount >= MAX_REGISTERED_BACKENDS) {
    return EFI_OUT_OF_RESOURCES;
  }

  mRegisteredBackends[mRegisteredBackendCount++] = Backend;
  OvLogTagged (OV_LOG_LEVEL_DEBUG, L"OCPU", L"Registered CPU backend '%s' for %s",
               Backend->Name, OvirCpuArchToString (Backend->Architecture));
  return EFI_SUCCESS;
}

CONST OVIR_CPU_BACKEND_INTERFACE *
EFIAPI
OvirCpuGetBackend (
  IN OVIR_CPU_ARCH  Arch
  )
{
  UINTN Idx;

  if (!mBackendsInitialized) {
    OvirCpuBackendInitialize ();
  }

  for (Idx = 0; Idx < mRegisteredBackendCount; Idx++) {
    if (mRegisteredBackends[Idx]->Architecture == Arch) {
      return mRegisteredBackends[Idx];
    }
  }

  return NULL;
}

EFI_STATUS
EFIAPI
OvirCpuEmitInstruction (
  IN     OVIR_CPU_ARCH               Arch,
  IN     CONST OVIR_CPU_INSTRUCTION  *Instruction,
  IN OUT UINT8                       *CodeBuffer,
  IN     UINTN                       BufferSize,
  OUT    UINTN                       *BytesEmitted
  )
{
  CONST OVIR_CPU_BACKEND_INTERFACE *Backend;

  if (Instruction == NULL || CodeBuffer == NULL || BytesEmitted == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Backend = OvirCpuGetBackend (Arch);
  if (Backend == NULL) {
    return EFI_UNSUPPORTED;
  }

  return Backend->EmitInstruction (Instruction, CodeBuffer, BufferSize, BytesEmitted);
}

EFI_STATUS
EFIAPI
OvirCpuEmitBlock (
  IN     OVIR_CPU_ARCH               Arch,
  IN     CONST OVIR_CPU_BASIC_BLOCK  *Block,
  IN OUT UINT8                       *CodeBuffer,
  IN     UINTN                       BufferSize,
  OUT    UINTN                       *BytesEmitted
  )
{
  CONST OVIR_CPU_BACKEND_INTERFACE *Backend;

  if (Block == NULL || CodeBuffer == NULL || BytesEmitted == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Backend = OvirCpuGetBackend (Arch);
  if (Backend == NULL) {
    return EFI_UNSUPPORTED;
  }

  if (Backend->EmitBlock != NULL) {
    return Backend->EmitBlock (Block, CodeBuffer, BufferSize, BytesEmitted);
  }

  // Fallback block emitter: loop through instructions sequentially
  UINT32 Idx;
  UINTN  TotalEmitted = 0;

  for (Idx = 0; Idx < Block->InstructionCount; Idx++) {
    UINTN InstEmitted = 0;
    EFI_STATUS Status = Backend->EmitInstruction (
      &Block->Instructions[Idx],
      CodeBuffer + TotalEmitted,
      BufferSize - TotalEmitted,
      &InstEmitted
      );

    if (EFI_ERROR (Status)) {
      return Status;
    }

    TotalEmitted += InstEmitted;
  }

  *BytesEmitted = TotalEmitted;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirCpuBackendInitialize (
  VOID
  )
{
  if (mBackendsInitialized) {
    return EFI_SUCCESS;
  }

  mRegisteredBackendCount = 0;

  CONST OVIR_CPU_BACKEND_INTERFACE *X64Backend = OvirCpuGetBackendX64 ();
  if (X64Backend != NULL) {
    OvirCpuRegisterBackend (X64Backend);
  }

  CONST OVIR_CPU_BACKEND_INTERFACE *ArmBackend = OvirCpuGetBackendArm64 ();
  if (ArmBackend != NULL) {
    OvirCpuRegisterBackend (ArmBackend);
  }

  mBackendsInitialized = TRUE;
  OvLogTagged (OV_LOG_LEVEL_INFO, L"OCPU", L"OVIR-CPU Backend Framework initialized (%u backends registered)",
               (UINT32)mRegisteredBackendCount);
  return EFI_SUCCESS;
}
