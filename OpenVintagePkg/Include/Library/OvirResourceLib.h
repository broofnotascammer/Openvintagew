/** @file
  OpenVintage Resource System Interface Definition.
  Phase 3 Modular Resource Abstractions (Buffers, Textures, Samplers, GPU Memory).

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OVIR_RESOURCE_LIB_H_
#define OVIR_RESOURCE_LIB_H_

#include <Uefi.h>
#include <Library/OvirGpuLib.h>

#define OVIR_MAX_BUFFERS       64
#define OVIR_MAX_TEXTURES      64
#define OVIR_MAX_SAMPLERS      16

//
// Buffer Object
//
typedef struct {
  OVIR_HANDLE         Handle;
  OVIR_BUFFER_DESC    Desc;
  OVIR_RESOURCE_STATE CurrentState;
  VOID                *MappedAddress;
  BOOLEAN             IsMapped;
  UINT64              AllocatedSizeBytes;
  BOOLEAN             Active;
} OVIR_BUFFER;

//
// Texture Object
//
typedef struct {
  OVIR_HANDLE         Handle;
  OVIR_TEXTURE_DESC   Desc;
  OVIR_RESOURCE_STATE CurrentState;
  UINT64              MemorySizeBytes;
  VOID                *HostStagingBuffer;
  BOOLEAN             Active;
} OVIR_TEXTURE;

//
// Sampler Object
//
typedef struct {
  OVIR_HANDLE        Handle;
  OVIR_SAMPLER_DESC  Desc;
  BOOLEAN            Active;
} OVIR_SAMPLER;

//
// GPU Memory Pool / Sub-allocator
//
typedef struct {
  UINT64  TotalPoolSizeBytes;
  UINT64  AllocatedBytes;
  UINT64  PeakAllocatedBytes;
  UINT32  ActiveAllocationCount;
} OVIR_GPU_MEMORY_POOL;

//
// Consolidated Resource Manager Context
//
typedef struct {
  BOOLEAN               Initialized;
  OVIR_GPU_MEMORY_POOL  MemoryPool;
  OVIR_BUFFER           Buffers[OVIR_MAX_BUFFERS];
  OVIR_TEXTURE          Textures[OVIR_MAX_TEXTURES];
  OVIR_SAMPLER          Samplers[OVIR_MAX_SAMPLERS];
} OVIR_RESOURCE_SYSTEM;

/**
  Initialize Resource subsystem.

  @retval EFI_SUCCESS   Resource system initialized.
**/
EFI_STATUS
EFIAPI
OvirResourceInitialize (
  VOID
  );

/**
  Create a GPU buffer resource.

  @param[in]  Desc       Buffer specification.
  @param[out] OutBuffer  Pointer to created buffer object.

  @retval EFI_SUCCESS    Buffer created.
**/
EFI_STATUS
EFIAPI
OvirBufferCreate (
  IN  CONST OVIR_BUFFER_DESC  *Desc,
  OUT OVIR_BUFFER             **OutBuffer
  );

/**
  Destroy a GPU buffer.

  @param[in] Buffer      Buffer to destroy.

  @retval EFI_SUCCESS    Destroyed cleanly.
**/
EFI_STATUS
EFIAPI
OvirBufferDestroy (
  IN OVIR_BUFFER  *Buffer
  );

/**
  Map buffer memory for CPU access.

  @param[in]  Buffer     Buffer pointer.
  @param[out] OutAddress Mapped pointer.

  @retval EFI_SUCCESS    Mapped successfully.
**/
EFI_STATUS
EFIAPI
OvirBufferMap (
  IN  OVIR_BUFFER  *Buffer,
  OUT VOID         **OutAddress
  );

/**
  Unmap buffer memory.

  @param[in] Buffer      Buffer pointer.

  @retval EFI_SUCCESS    Unmapped.
**/
EFI_STATUS
EFIAPI
OvirBufferUnmap (
  IN OVIR_BUFFER  *Buffer
  );

/**
  Upload data into buffer.

  @param[in] Buffer      Target buffer.
  @param[in] Offset      Byte offset.
  @param[in] Data        Source data pointer.
  @param[in] SizeBytes   Byte count.

  @retval EFI_SUCCESS    Uploaded.
**/
EFI_STATUS
EFIAPI
OvirBufferUpload (
  IN OVIR_BUFFER  *Buffer,
  IN UINT64       Offset,
  IN CONST VOID   *Data,
  IN UINT64       SizeBytes
  );

/**
  Create a texture resource.

  @param[in]  Desc        Texture specification.
  @param[out] OutTexture  Pointer to created texture object.

  @retval EFI_SUCCESS     Texture created.
**/
EFI_STATUS
EFIAPI
OvirTextureCreate (
  IN  CONST OVIR_TEXTURE_DESC  *Desc,
  OUT OVIR_TEXTURE             **OutTexture
  );

/**
  Destroy a texture.

  @param[in] Texture      Texture to destroy.

  @retval EFI_SUCCESS     Destroyed.
**/
EFI_STATUS
EFIAPI
OvirTextureDestroy (
  IN OVIR_TEXTURE  *Texture
  );

/**
  Create a sampler state object.

  @param[in]  Desc        Sampler specification.
  @param[out] OutSampler  Pointer to created sampler object.

  @retval EFI_SUCCESS     Sampler created.
**/
EFI_STATUS
EFIAPI
OvirSamplerCreate (
  IN  CONST OVIR_SAMPLER_DESC  *Desc,
  OUT OVIR_SAMPLER             **OutSampler
  );

/**
  Destroy a sampler.

  @param[in] Sampler      Sampler to destroy.

  @retval EFI_SUCCESS     Destroyed.
**/
EFI_STATUS
EFIAPI
OvirSamplerDestroy (
  IN OVIR_SAMPLER  *Sampler
  );

/**
  Get memory pool statistics.

  @param[out] Pool   Pointer to receive pool stats.
**/
VOID
EFIAPI
OvirResourceGetMemoryStats (
  OUT OVIR_GPU_MEMORY_POOL  *Pool
  );

#endif // OVIR_RESOURCE_LIB_H_
