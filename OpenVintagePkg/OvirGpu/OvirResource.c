/** @file
  OpenVintage Resource System Implementation.
  Phase 3 Modular Resource Abstractions (Buffers, Textures, Samplers, GPU Memory).

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/OvLoggerLib.h>
#include <Library/OvMemoryLib.h>
#include <Library/OvirGpuLib.h>
#include <Library/OvirResourceLib.h>
#include <Library/OvirPerfLib.h>

STATIC OVIR_RESOURCE_SYSTEM mResourceSystem = { 0 };
STATIC UINT32               mNextResourceHandle = 1;

EFI_STATUS
EFIAPI
OvirResourceInitialize (
  VOID
  )
{
  ZeroMem (&mResourceSystem, sizeof (OVIR_RESOURCE_SYSTEM));
  mResourceSystem.Initialized = TRUE;
  mResourceSystem.MemoryPool.TotalPoolSizeBytes = 256ULL * 1024ULL * 1024ULL; // 256 MB tracking pool
  OvLogTagged (OV_LOG_LEVEL_DEBUG, L"RESM", L"Resource manager & GPU memory pool initialized");
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirBufferCreate (
  IN  CONST OVIR_BUFFER_DESC  *Desc,
  OUT OVIR_BUFFER             **OutBuffer
  )
{
  UINTN       Index;
  OVIR_BUFFER *Buf = NULL;

  if (Desc == NULL || OutBuffer == NULL || Desc->SizeBytes == 0) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mResourceSystem.Initialized) {
    OvirResourceInitialize ();
  }

  for (Index = 0; Index < OVIR_MAX_BUFFERS; Index++) {
    if (!mResourceSystem.Buffers[Index].Active) {
      Buf = &mResourceSystem.Buffers[Index];
      break;
    }
  }

  if (Buf == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (Buf, sizeof (OVIR_BUFFER));
  Buf->Handle = mNextResourceHandle++;
  CopyMem (&Buf->Desc, Desc, sizeof (OVIR_BUFFER_DESC));
  Buf->CurrentState = OvirResourceStateCommon;
  Buf->AllocatedSizeBytes = Desc->SizeBytes;
  Buf->Active = TRUE;

  // Allocate host-visible backing memory
  Buf->MappedAddress = OvAllocate ((UINTN)Desc->SizeBytes, OV_MEM_TAG_BUFF);
  if (Buf->MappedAddress == NULL) {
    Buf->Active = FALSE;
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (Buf->MappedAddress, (UINTN)Desc->SizeBytes);

  mResourceSystem.MemoryPool.AllocatedBytes += Desc->SizeBytes;
  if (mResourceSystem.MemoryPool.AllocatedBytes > mResourceSystem.MemoryPool.PeakAllocatedBytes) {
    mResourceSystem.MemoryPool.PeakAllocatedBytes = mResourceSystem.MemoryPool.AllocatedBytes;
  }
  mResourceSystem.MemoryPool.ActiveAllocationCount++;

  OvirPerfUpdateResourceUsage (
    mResourceSystem.MemoryPool.AllocatedBytes,
    mResourceSystem.MemoryPool.PeakAllocatedBytes
    );

  *OutBuffer = Buf;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirBufferDestroy (
  IN OVIR_BUFFER  *Buffer
  )
{
  if (Buffer == NULL || !Buffer->Active) {
    return EFI_INVALID_PARAMETER;
  }

  if (Buffer->MappedAddress != NULL) {
    OvFree (Buffer->MappedAddress);
    Buffer->MappedAddress = NULL;
  }

  if (mResourceSystem.MemoryPool.AllocatedBytes >= Buffer->AllocatedSizeBytes) {
    mResourceSystem.MemoryPool.AllocatedBytes -= Buffer->AllocatedSizeBytes;
  }
  if (mResourceSystem.MemoryPool.ActiveAllocationCount > 0) {
    mResourceSystem.MemoryPool.ActiveAllocationCount--;
  }

  OvirPerfUpdateResourceUsage (
    mResourceSystem.MemoryPool.AllocatedBytes,
    mResourceSystem.MemoryPool.PeakAllocatedBytes
    );

  Buffer->Active = FALSE;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirBufferMap (
  IN  OVIR_BUFFER  *Buffer,
  OUT VOID         **OutAddress
  )
{
  if (Buffer == NULL || !Buffer->Active || OutAddress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Buffer->IsMapped = TRUE;
  *OutAddress = Buffer->MappedAddress;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirBufferUnmap (
  IN OVIR_BUFFER  *Buffer
  )
{
  if (Buffer == NULL || !Buffer->Active) {
    return EFI_INVALID_PARAMETER;
  }

  Buffer->IsMapped = FALSE;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirBufferUpload (
  IN OVIR_BUFFER  *Buffer,
  IN UINT64       Offset,
  IN CONST VOID   *Data,
  IN UINT64       SizeBytes
  )
{
  if (Buffer == NULL || !Buffer->Active || Data == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Offset + SizeBytes > Buffer->AllocatedSizeBytes) {
    return EFI_BAD_BUFFER_SIZE;
  }

  CopyMem ((UINT8 *)Buffer->MappedAddress + Offset, Data, (UINTN)SizeBytes);
  return EFI_SUCCESS;
}

STATIC
UINT64
OvirCalculateTextureSizeBytes (
  IN CONST OVIR_TEXTURE_DESC  *Desc
  )
{
  UINT64 BytesPerPixel = 4;

  switch (Desc->Format) {
    case OVIR_FORMAT_R8_UNORM:
      BytesPerPixel = 1;
      break;
    case OVIR_FORMAT_RG8_UNORM:
    case OVIR_FORMAT_D16_UNORM:
      BytesPerPixel = 2;
      break;
    case OVIR_FORMAT_RGBA8_UNORM:
    case OVIR_FORMAT_BGRA8_UNORM:
    case OVIR_FORMAT_D24_UNORM_S8_UINT:
    case OVIR_FORMAT_D32_FLOAT:
    case OVIR_FORMAT_R32_FLOAT:
      BytesPerPixel = 4;
      break;
    case OVIR_FORMAT_RGBA16_FLOAT:
    case OVIR_FORMAT_RG32_FLOAT:
      BytesPerPixel = 8;
      break;
    case OVIR_FORMAT_RGBA32_FLOAT:
      BytesPerPixel = 16;
      break;
    default:
      BytesPerPixel = 4;
      break;
  }

  return (UINT64)Desc->Width * (UINT64)Desc->Height * (UINT64)((Desc->Depth > 0) ? Desc->Depth : 1) * BytesPerPixel;
}

EFI_STATUS
EFIAPI
OvirTextureCreate (
  IN  CONST OVIR_TEXTURE_DESC  *Desc,
  OUT OVIR_TEXTURE             **OutTexture
  )
{
  UINTN        Index;
  OVIR_TEXTURE *Tex = NULL;
  UINT64       Size;

  if (Desc == NULL || OutTexture == NULL || Desc->Width == 0 || Desc->Height == 0) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mResourceSystem.Initialized) {
    OvirResourceInitialize ();
  }

  for (Index = 0; Index < OVIR_MAX_TEXTURES; Index++) {
    if (!mResourceSystem.Textures[Index].Active) {
      Tex = &mResourceSystem.Textures[Index];
      break;
    }
  }

  if (Tex == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Size = OvirCalculateTextureSizeBytes (Desc);

  ZeroMem (Tex, sizeof (OVIR_TEXTURE));
  Tex->Handle = mNextResourceHandle++;
  CopyMem (&Tex->Desc, Desc, sizeof (OVIR_TEXTURE_DESC));
  Tex->CurrentState = OvirResourceStateCommon;
  Tex->MemorySizeBytes = Size;
  Tex->Active = TRUE;

  Tex->HostStagingBuffer = OvAllocate ((UINTN)Size, OV_MEM_TAG_TEXR);
  if (Tex->HostStagingBuffer == NULL) {
    Tex->Active = FALSE;
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (Tex->HostStagingBuffer, (UINTN)Size);

  mResourceSystem.MemoryPool.AllocatedBytes += Size;
  if (mResourceSystem.MemoryPool.AllocatedBytes > mResourceSystem.MemoryPool.PeakAllocatedBytes) {
    mResourceSystem.MemoryPool.PeakAllocatedBytes = mResourceSystem.MemoryPool.AllocatedBytes;
  }
  mResourceSystem.MemoryPool.ActiveAllocationCount++;

  OvirPerfUpdateResourceUsage (
    mResourceSystem.MemoryPool.AllocatedBytes,
    mResourceSystem.MemoryPool.PeakAllocatedBytes
    );

  *OutTexture = Tex;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirTextureDestroy (
  IN OVIR_TEXTURE  *Texture
  )
{
  if (Texture == NULL || !Texture->Active) {
    return EFI_INVALID_PARAMETER;
  }

  if (Texture->HostStagingBuffer != NULL) {
    OvFree (Texture->HostStagingBuffer);
    Texture->HostStagingBuffer = NULL;
  }

  if (mResourceSystem.MemoryPool.AllocatedBytes >= Texture->MemorySizeBytes) {
    mResourceSystem.MemoryPool.AllocatedBytes -= Texture->MemorySizeBytes;
  }
  if (mResourceSystem.MemoryPool.ActiveAllocationCount > 0) {
    mResourceSystem.MemoryPool.ActiveAllocationCount--;
  }

  OvirPerfUpdateResourceUsage (
    mResourceSystem.MemoryPool.AllocatedBytes,
    mResourceSystem.MemoryPool.PeakAllocatedBytes
    );

  Texture->Active = FALSE;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirSamplerCreate (
  IN  CONST OVIR_SAMPLER_DESC  *Desc,
  OUT OVIR_SAMPLER             **OutSampler
  )
{
  UINTN        Index;
  OVIR_SAMPLER *Smp = NULL;

  if (Desc == NULL || OutSampler == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mResourceSystem.Initialized) {
    OvirResourceInitialize ();
  }

  for (Index = 0; Index < OVIR_MAX_SAMPLERS; Index++) {
    if (!mResourceSystem.Samplers[Index].Active) {
      Smp = &mResourceSystem.Samplers[Index];
      break;
    }
  }

  if (Smp == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (Smp, sizeof (OVIR_SAMPLER));
  Smp->Handle = mNextResourceHandle++;
  CopyMem (&Smp->Desc, Desc, sizeof (OVIR_SAMPLER_DESC));
  Smp->Active = TRUE;

  *OutSampler = Smp;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirSamplerDestroy (
  IN OVIR_SAMPLER  *Sampler
  )
{
  if (Sampler == NULL || !Sampler->Active) {
    return EFI_INVALID_PARAMETER;
  }

  Sampler->Active = FALSE;
  return EFI_SUCCESS;
}

VOID
EFIAPI
OvirResourceGetMemoryStats (
  OUT OVIR_GPU_MEMORY_POOL  *Pool
  )
{
  if (Pool != NULL) {
    CopyMem (Pool, &mResourceSystem.MemoryPool, sizeof (OVIR_GPU_MEMORY_POOL));
  }
}
