/** @file
  OpenVintage Modular CPU Decoder Registry and Dispatcher.
  Phase 4 Architecture-Agnostic CPU Translation Framework.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/OvirCpuDecoderLib.h>

#define MAX_REGISTERED_DECODERS 8

STATIC CONST OVIR_CPU_DECODER_INTERFACE *mRegisteredDecoders[MAX_REGISTERED_DECODERS];
STATIC UINTN                            mRegisteredDecoderCount = 0;
STATIC BOOLEAN                          mDecodersInitialized = FALSE;

EFI_STATUS
EFIAPI
OvirCpuRegisterDecoder (
  IN CONST OVIR_CPU_DECODER_INTERFACE  *Decoder
  )
{
  UINTN Idx;

  if (Decoder == NULL || Decoder->DecodeInstruction == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Check if already registered
  for (Idx = 0; Idx < mRegisteredDecoderCount; Idx++) {
    if (mRegisteredDecoders[Idx]->Architecture == Decoder->Architecture) {
      mRegisteredDecoders[Idx] = Decoder;
      return EFI_SUCCESS;
    }
  }

  if (mRegisteredDecoderCount >= MAX_REGISTERED_DECODERS) {
    return EFI_OUT_OF_RESOURCES;
  }

  mRegisteredDecoders[mRegisteredDecoderCount++] = Decoder;
  OvLogTagged (OV_LOG_LEVEL_DEBUG, L"OCPU", L"Registered CPU decoder '%s' for %s",
               Decoder->Name, OvirCpuArchToString (Decoder->Architecture));
  return EFI_SUCCESS;
}

CONST OVIR_CPU_DECODER_INTERFACE *
EFIAPI
OvirCpuGetDecoder (
  IN OVIR_CPU_ARCH  Arch
  )
{
  UINTN Idx;

  if (!mDecodersInitialized) {
    OvirCpuDecoderInitialize ();
  }

  for (Idx = 0; Idx < mRegisteredDecoderCount; Idx++) {
    if (mRegisteredDecoders[Idx]->Architecture == Arch) {
      return mRegisteredDecoders[Idx];
    }
  }

  return NULL;
}

EFI_STATUS
EFIAPI
OvirCpuDecodeInstruction (
  IN  OVIR_CPU_ARCH         Arch,
  IN  CONST UINT8           *Code,
  IN  UINTN                 MaxLen,
  IN  UINT64                GuestPc,
  OUT OVIR_CPU_INSTRUCTION  *Instruction,
  OUT UINTN                 *BytesConsumed
  )
{
  CONST OVIR_CPU_DECODER_INTERFACE *Decoder;

  if (Code == NULL || Instruction == NULL || BytesConsumed == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Decoder = OvirCpuGetDecoder (Arch);
  if (Decoder == NULL) {
    return EFI_UNSUPPORTED;
  }

  return Decoder->DecodeInstruction (Code, MaxLen, GuestPc, Instruction, BytesConsumed);
}

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
  )
{
  CONST OVIR_CPU_DECODER_INTERFACE *Decoder;

  if (Code == NULL || Block == NULL || BytesConsumed == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Decoder = OvirCpuGetDecoder (Arch);
  if (Decoder == NULL) {
    return EFI_UNSUPPORTED;
  }

  if (Decoder->DecodeBlock != NULL) {
    return Decoder->DecodeBlock (Code, MaxLen, GuestPc, MaxInstructions, Block, BytesConsumed);
  }

  // Fallback block decoder using sequential instruction decoding
  OvirCpuInitBlock (Block, 1, GuestPc);
  *BytesConsumed = 0;

  UINTN CurOffset = 0;
  UINT64 CurPc = GuestPc;

  while (CurOffset < MaxLen && Block->InstructionCount < MaxInstructions) {
    OVIR_CPU_INSTRUCTION Inst;
    UINTN InstBytes = 0;
    EFI_STATUS Status = Decoder->DecodeInstruction (
      Code + CurOffset,
      MaxLen - CurOffset,
      CurPc,
      &Inst,
      &InstBytes
      );

    if (EFI_ERROR (Status) || InstBytes == 0) {
      break;
    }

    OvirCpuAppendInstruction (Block, &Inst);
    CurOffset += InstBytes;
    CurPc += InstBytes;

    if (Block->IsTerminated) {
      break;
    }
  }

  *BytesConsumed = CurOffset;
  return (Block->InstructionCount > 0) ? EFI_SUCCESS : EFI_NOT_FOUND;
}

EFI_STATUS
EFIAPI
OvirCpuDecoderInitialize (
  VOID
  )
{
  if (mDecodersInitialized) {
    return EFI_SUCCESS;
  }

  mRegisteredDecoderCount = 0;

  // Register built-in decoders
  CONST OVIR_CPU_DECODER_INTERFACE *ArmDecoder = OvirCpuGetDecoderArm64 ();
  if (ArmDecoder != NULL) {
    OvirCpuRegisterDecoder (ArmDecoder);
  }

  CONST OVIR_CPU_DECODER_INTERFACE *X64Decoder = OvirCpuGetDecoderX64 ();
  if (X64Decoder != NULL) {
    OvirCpuRegisterDecoder (X64Decoder);
  }

  mDecodersInitialized = TRUE;
  OvLogTagged (OV_LOG_LEVEL_INFO, L"OCPU", L"OVIR-CPU Decoder Framework initialized (%u decoders registered)",
               (UINT32)mRegisteredDecoderCount);
  return EFI_SUCCESS;
}
