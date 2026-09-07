/** @file
  OpenVintage Modular CPU Decoder Interface Definition.
  Phase 4 Modular Decoder Framework.

  Decouples guest machine instruction decoding across architecture-specific
  decoders (ARM64, x86-64, etc.) without monolithic file bundling.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OVIR_CPU_DECODER_LIB_H_
#define OVIR_CPU_DECODER_LIB_H_

#include <Uefi.h>
#include <Library/OvirCpuLib.h>

//
// Decoder Interface Callback Function Types
//
typedef
EFI_STATUS
(EFIAPI *OVIR_DECODE_INSTRUCTION_FN) (
  IN  CONST UINT8           *Code,
  IN  UINTN                 MaxLen,
  IN  UINT64                GuestPc,
  OUT OVIR_CPU_INSTRUCTION  *Instruction,
  OUT UINTN                 *BytesConsumed
  );

typedef
EFI_STATUS
(EFIAPI *OVIR_DECODE_BLOCK_FN) (
  IN  CONST UINT8           *Code,
  IN  UINTN                 MaxLen,
  IN  UINT64                GuestPc,
  IN  UINTN                 MaxInstructions,
  OUT OVIR_CPU_BASIC_BLOCK  *Block,
  OUT UINTN                 *BytesConsumed
  );

//
// Modular Decoder Interface
//
typedef struct {
  OVIR_CPU_ARCH               Architecture;
  CHAR16                      Name[32];
  OVIR_DECODE_INSTRUCTION_FN  DecodeInstruction;
  OVIR_DECODE_BLOCK_FN        DecodeBlock;
} OVIR_CPU_DECODER_INTERFACE;

/**
  Initialize decoder subsystem and register built-in architecture decoders.

  @retval EFI_SUCCESS   Decoders initialized.
**/
EFI_STATUS
EFIAPI
OvirCpuDecoderInitialize (
  VOID
  );

/**
  Register a new architecture decoder into the framework.

  @param[in] Decoder    Pointer to decoder interface table.

  @retval EFI_SUCCESS   Decoder registered successfully.
  @retval EFI_ALREADY_STARTED Architecture decoder already present.
**/
EFI_STATUS
EFIAPI
OvirCpuRegisterDecoder (
  IN CONST OVIR_CPU_DECODER_INTERFACE  *Decoder
  );

/**
  Retrieve a registered decoder by architecture type.

  @param[in] Arch       Target guest architecture.

  @return Pointer to interface or NULL if unsupported.
**/
CONST OVIR_CPU_DECODER_INTERFACE *
EFIAPI
OvirCpuGetDecoder (
  IN OVIR_CPU_ARCH  Arch
  );

/**
  Decode a single instruction using the appropriate architecture decoder.

  @param[in]  Arch           Source architecture.
  @param[in]  Code           Raw instruction bytes.
  @param[in]  MaxLen         Available buffer length.
  @param[in]  GuestPc        Current instruction address.
  @param[out] Instruction    Decoded IR instruction.
  @param[out] BytesConsumed  Number of guest bytes consumed.

  @retval EFI_SUCCESS        Instruction decoded cleanly.
  @retval EFI_UNSUPPORTED    Architecture or opcode not supported.
**/
EFI_STATUS
EFIAPI
OvirCpuDecodeInstruction (
  IN  OVIR_CPU_ARCH         Arch,
  IN  CONST UINT8           *Code,
  IN  UINTN                 MaxLen,
  IN  UINT64                GuestPc,
  OUT OVIR_CPU_INSTRUCTION  *Instruction,
  OUT UINTN                 *BytesConsumed
  );

/**
  Decode a basic block of instructions until a branch or block boundary.

  @param[in]  Arch             Source architecture.
  @param[in]  Code             Raw instruction stream.
  @param[in]  MaxLen           Available buffer length.
  @param[in]  GuestPc          Starting address.
  @param[in]  MaxInstructions  Max instructions to decode into block.
  @param[out] Block            Constructed IR basic block.
  @param[out] BytesConsumed    Total guest bytes consumed.

  @retval EFI_SUCCESS          Block decoded successfully.
**/
EFI_STATUS
EFIAPI
OvirCpuDecodeBlock (
  IN  OVIR_CPU_ARCH         Arch,
  IN  CONST UINT8           *Code,
  IN  UINTN                 MaxLen,
  IN  UINT64                GuestPc,
  IN  UINTN                 MaxInstructions,
  OUT OVIR_CPU_BASIC_BLOCK  *Block,
  OUT UINTN                 *BytesConsumed
  );

//
// Architecture-Specific Interface Accessors
//
CONST OVIR_CPU_DECODER_INTERFACE *
EFIAPI
OvirCpuGetDecoderArm64 (
  VOID
  );

CONST OVIR_CPU_DECODER_INTERFACE *
EFIAPI
OvirCpuGetDecoderX64 (
  VOID
  );

#endif // OVIR_CPU_DECODER_LIB_H_
