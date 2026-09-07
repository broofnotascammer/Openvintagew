/** @file
  OpenVintage Modular CPU Translation Backend Interface.
  Phase 4 Architecture-Agnostic Code Emission Layer.

  Decouples native machine code generation (x86-64, ARM64, etc.) from
  the OVIR-CPU intermediate representation.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OVIR_CPU_BACKEND_LIB_H_
#define OVIR_CPU_BACKEND_LIB_H_

#include <Uefi.h>
#include <Library/OvirCpuLib.h>

typedef
EFI_STATUS
(EFIAPI *OVIR_BACKEND_EMIT_INSTRUCTION_FN) (
  IN     CONST OVIR_CPU_INSTRUCTION  *Instruction,
  IN OUT UINT8                       *CodeBuffer,
  IN     UINTN                       BufferSize,
  OUT    UINTN                       *BytesEmitted
  );

typedef
EFI_STATUS
(EFIAPI *OVIR_BACKEND_EMIT_BLOCK_FN) (
  IN     CONST OVIR_CPU_BASIC_BLOCK  *Block,
  IN OUT UINT8                       *CodeBuffer,
  IN     UINTN                       BufferSize,
  OUT    UINTN                       *BytesEmitted
  );

//
// Backend Interface Structure
//
typedef struct {
  OVIR_CPU_ARCH                     Architecture;
  CHAR16                            Name[32];
  OVIR_BACKEND_EMIT_INSTRUCTION_FN  EmitInstruction;
  OVIR_BACKEND_EMIT_BLOCK_FN        EmitBlock;
} OVIR_CPU_BACKEND_INTERFACE;

/**
  Initialize backend subsystem and register built-in targets (x86-64 and ARM64).

  @retval EFI_SUCCESS   Backends initialized.
**/
EFI_STATUS
EFIAPI
OvirCpuBackendInitialize (
  VOID
  );

/**
  Register an architecture backend into the registry.

  @param[in] Backend    Backend interface descriptor.

  @retval EFI_SUCCESS   Backend registered.
**/
EFI_STATUS
EFIAPI
OvirCpuRegisterBackend (
  IN CONST OVIR_CPU_BACKEND_INTERFACE  *Backend
  );

/**
  Retrieve a backend descriptor by architecture.

  @param[in] Arch       Target target architecture.

  @return Pointer to interface or NULL if unavailable.
**/
CONST OVIR_CPU_BACKEND_INTERFACE *
EFIAPI
OvirCpuGetBackend (
  IN OVIR_CPU_ARCH  Arch
  );

/**
  Emit machine code for a single IR instruction using selected backend.

  @param[in]     Arch          Target native architecture.
  @param[in]     Instruction   IR instruction to translate.
  @param[in,out] CodeBuffer    Target machine code output buffer.
  @param[in]     BufferSize    Buffer capacity.
  @param[out]    BytesEmitted  Bytes written to buffer.

  @retval EFI_SUCCESS          Instruction emitted.
  @retval EFI_BUFFER_TOO_SMALL Insufficient buffer size.
**/
EFI_STATUS
EFIAPI
OvirCpuEmitInstruction (
  IN     OVIR_CPU_ARCH               Arch,
  IN     CONST OVIR_CPU_INSTRUCTION  *Instruction,
  IN OUT UINT8                       *CodeBuffer,
  IN     UINTN                       BufferSize,
  OUT    UINTN                       *BytesEmitted
  );

/**
  Emit machine code for an entire basic block using selected backend.

  @param[in]     Arch          Target native architecture.
  @param[in]     Block         IR basic block to emit.
  @param[in,out] CodeBuffer    Target machine code output buffer.
  @param[in]     BufferSize    Buffer capacity.
  @param[out]    BytesEmitted  Total bytes emitted.

  @retval EFI_SUCCESS          Block emitted cleanly.
**/
EFI_STATUS
EFIAPI
OvirCpuEmitBlock (
  IN     OVIR_CPU_ARCH               Arch,
  IN     CONST OVIR_CPU_BASIC_BLOCK  *Block,
  IN OUT UINT8                       *CodeBuffer,
  IN     UINTN                       BufferSize,
  OUT    UINTN                       *BytesEmitted
  );

//
// Architecture Backend Accessors
//
CONST OVIR_CPU_BACKEND_INTERFACE *
EFIAPI
OvirCpuGetBackendX64 (
  VOID
  );

CONST OVIR_CPU_BACKEND_INTERFACE *
EFIAPI
OvirCpuGetBackendArm64 (
  VOID
  );

#endif // OVIR_CPU_BACKEND_LIB_H_
