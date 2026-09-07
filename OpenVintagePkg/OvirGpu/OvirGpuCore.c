/** @file
  OpenVintage Graphics Intermediate Representation (OVIR-GPU) Core Implementation.
  Phase 3 Modular Graphics Framework.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/OvLoggerLib.h>
#include <Library/OvMemoryLib.h>
#include <Library/OvHardwareLib.h>
#include <Library/OvirGpuLib.h>
#include <Library/OvirPerfLib.h>
#include <Protocol/GraphicsOutput.h>

STATIC BOOLEAN mOvirGpuInitialized = FALSE;
STATIC UINT32  mNextSequenceId = 1;

EFI_STATUS
EFIAPI
OvirGpuInitialize (
  VOID
  )
{
  if (mOvirGpuInitialized) {
    return EFI_SUCCESS;
  }

  mOvirGpuInitialized = TRUE;
  OvLogTagged (OV_LOG_LEVEL_DEBUG, L"OVIR", L"OVIR-GPU intermediate representation core initialized");
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirCreateCommandList (
  IN  UINT32             InitialCapacity,
  OUT OVIR_COMMAND_LIST  **CommandList
  )
{
  OVIR_COMMAND_LIST  *CmdList;
  UINT32             Capacity;

  if (CommandList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Capacity = (InitialCapacity > 0) ? InitialCapacity : OVIR_MAX_COMMANDS_DEFAULT;

  CmdList = (OVIR_COMMAND_LIST *)OvAllocate (sizeof (OVIR_COMMAND_LIST), OV_MEM_TAG_OVIR);
  if (CmdList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (CmdList, sizeof (OVIR_COMMAND_LIST));

  CmdList->Commands = (OVIR_COMMAND *)OvAllocate (Capacity * sizeof (OVIR_COMMAND), OV_MEM_TAG_BUFF);
  if (CmdList->Commands == NULL) {
    OvFree (CmdList);
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (CmdList->Commands, Capacity * sizeof (OVIR_COMMAND));
  CmdList->Capacity = Capacity;
  CmdList->Count = 0;
  CmdList->IsRecording = FALSE;
  CmdList->IsValidated = FALSE;

  *CommandList = CmdList;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirDestroyCommandList (
  IN OVIR_COMMAND_LIST   *CommandList
  )
{
  if (CommandList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (CommandList->Commands != NULL) {
    OvFree (CommandList->Commands);
    CommandList->Commands = NULL;
  }

  OvFree (CommandList);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirBeginCommandList (
  IN OVIR_COMMAND_LIST   *CommandList
  )
{
  if (CommandList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CommandList->Count = 0;
  CommandList->IsRecording = TRUE;
  CommandList->IsValidated = FALSE;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirEndCommandList (
  IN OVIR_COMMAND_LIST   *CommandList
  )
{
  if (CommandList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!CommandList->IsRecording) {
    return EFI_NOT_READY;
  }

  CommandList->IsRecording = FALSE;
  return EFI_SUCCESS;
}

STATIC
OVIR_COMMAND *
OvirAllocateCommandSlot (
  IN OVIR_COMMAND_LIST  *CmdList,
  IN OVIR_COMMAND_TYPE  Type
  )
{
  OVIR_COMMAND  *Cmd;

  if (CmdList == NULL || !CmdList->IsRecording) {
    return NULL;
  }

  if (CmdList->Count >= CmdList->Capacity) {
    return NULL; // Capacity reached
  }

  Cmd = &CmdList->Commands[CmdList->Count++];
  ZeroMem (Cmd, sizeof (OVIR_COMMAND));
  Cmd->Type = Type;
  Cmd->SequenceId = mNextSequenceId++;

  return Cmd;
}

EFI_STATUS
EFIAPI
OvirCmdBeginRenderPass (
  IN OVIR_COMMAND_LIST   *CommandList,
  IN OVIR_HANDLE         RenderTargetHandle,
  IN CONST FLOAT32       ClearColor[4],
  IN FLOAT32             ClearDepth,
  IN UINT32              ClearStencil,
  IN OVIR_LOAD_OP        LoadOp,
  IN OVIR_STORE_OP       StoreOp
  )
{
  OVIR_COMMAND  *Cmd;

  Cmd = OvirAllocateCommandSlot (CommandList, OVIR_CMD_BEGIN_RENDER_PASS);
  if (Cmd == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Cmd->As.BeginPass.RenderTargetHandle = RenderTargetHandle;
  if (ClearColor != NULL) {
    CopyMem (Cmd->As.BeginPass.ClearColor, ClearColor, sizeof (FLOAT32) * 4);
  } else {
    ZeroMem (Cmd->As.BeginPass.ClearColor, sizeof (FLOAT32) * 4);
  }
  Cmd->As.BeginPass.ClearDepth = ClearDepth;
  Cmd->As.BeginPass.ClearStencil = ClearStencil;
  Cmd->As.BeginPass.LoadOp = LoadOp;
  Cmd->As.BeginPass.StoreOp = StoreOp;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirCmdEndRenderPass (
  IN OVIR_COMMAND_LIST   *CommandList
  )
{
  OVIR_COMMAND  *Cmd;

  Cmd = OvirAllocateCommandSlot (CommandList, OVIR_CMD_END_RENDER_PASS);
  if (Cmd == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirCmdSetPipeline (
  IN OVIR_COMMAND_LIST   *CommandList,
  IN OVIR_HANDLE         PipelineHandle
  )
{
  OVIR_COMMAND  *Cmd;

  Cmd = OvirAllocateCommandSlot (CommandList, OVIR_CMD_SET_PIPELINE);
  if (Cmd == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Cmd->As.SetPipeline.PipelineHandle = PipelineHandle;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirCmdSetViewport (
  IN OVIR_COMMAND_LIST   *CommandList,
  IN FLOAT32             X,
  IN FLOAT32             Y,
  IN FLOAT32             Width,
  IN FLOAT32             Height,
  IN FLOAT32             MinDepth,
  IN FLOAT32             MaxDepth
  )
{
  OVIR_COMMAND  *Cmd;

  Cmd = OvirAllocateCommandSlot (CommandList, OVIR_CMD_SET_VIEWPORT);
  if (Cmd == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Cmd->As.SetViewport.X = X;
  Cmd->As.SetViewport.Y = Y;
  Cmd->As.SetViewport.Width = Width;
  Cmd->As.SetViewport.Height = Height;
  Cmd->As.SetViewport.MinDepth = MinDepth;
  Cmd->As.SetViewport.MaxDepth = MaxDepth;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirCmdSetScissor (
  IN OVIR_COMMAND_LIST   *CommandList,
  IN INT32               X,
  IN INT32               Y,
  IN UINT32              Width,
  IN UINT32              Height
  )
{
  OVIR_COMMAND  *Cmd;

  Cmd = OvirAllocateCommandSlot (CommandList, OVIR_CMD_SET_SCISSOR);
  if (Cmd == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Cmd->As.SetScissor.X = X;
  Cmd->As.SetScissor.Y = Y;
  Cmd->As.SetScissor.Width = Width;
  Cmd->As.SetScissor.Height = Height;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirCmdBindVertexBuffer (
  IN OVIR_COMMAND_LIST   *CommandList,
  IN UINT32              Slot,
  IN OVIR_HANDLE         BufferHandle,
  IN UINT64              Offset,
  IN UINT32              Stride
  )
{
  OVIR_COMMAND  *Cmd;

  Cmd = OvirAllocateCommandSlot (CommandList, OVIR_CMD_BIND_VERTEX_BUFFER);
  if (Cmd == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Cmd->As.BindVertexBuffer.BindingSlot = Slot;
  Cmd->As.BindVertexBuffer.BufferHandle = BufferHandle;
  Cmd->As.BindVertexBuffer.Offset = Offset;
  Cmd->As.BindVertexBuffer.Stride = Stride;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirCmdBindIndexBuffer (
  IN OVIR_COMMAND_LIST   *CommandList,
  IN OVIR_HANDLE         BufferHandle,
  IN UINT64              Offset,
  IN UINT32              IndexTypeSize
  )
{
  OVIR_COMMAND  *Cmd;

  Cmd = OvirAllocateCommandSlot (CommandList, OVIR_CMD_BIND_INDEX_BUFFER);
  if (Cmd == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Cmd->As.BindIndexBuffer.BufferHandle = BufferHandle;
  Cmd->As.BindIndexBuffer.Offset = Offset;
  Cmd->As.BindIndexBuffer.IndexTypeSize = IndexTypeSize;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirCmdDraw (
  IN OVIR_COMMAND_LIST   *CommandList,
  IN UINT32              VertexCount,
  IN UINT32              InstanceCount,
  IN UINT32              FirstVertex,
  IN UINT32              FirstInstance
  )
{
  OVIR_COMMAND  *Cmd;

  Cmd = OvirAllocateCommandSlot (CommandList, OVIR_CMD_DRAW);
  if (Cmd == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Cmd->As.Draw.VertexCount = VertexCount;
  Cmd->As.Draw.InstanceCount = InstanceCount;
  Cmd->As.Draw.FirstVertex = FirstVertex;
  Cmd->As.Draw.FirstInstance = FirstInstance;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirCmdDrawIndexed (
  IN OVIR_COMMAND_LIST   *CommandList,
  IN UINT32              IndexCount,
  IN UINT32              InstanceCount,
  IN UINT32              FirstIndex,
  IN INT32               VertexOffset,
  IN UINT32              FirstInstance
  )
{
  OVIR_COMMAND  *Cmd;

  Cmd = OvirAllocateCommandSlot (CommandList, OVIR_CMD_DRAW_INDEXED);
  if (Cmd == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Cmd->As.DrawIndexed.IndexCount = IndexCount;
  Cmd->As.DrawIndexed.InstanceCount = InstanceCount;
  Cmd->As.DrawIndexed.FirstIndex = FirstIndex;
  Cmd->As.DrawIndexed.VertexOffset = VertexOffset;
  Cmd->As.DrawIndexed.FirstInstance = FirstInstance;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirCmdDispatch (
  IN OVIR_COMMAND_LIST   *CommandList,
  IN UINT32              GroupCountX,
  IN UINT32              GroupCountY,
  IN UINT32              GroupCountZ
  )
{
  OVIR_COMMAND  *Cmd;

  Cmd = OvirAllocateCommandSlot (CommandList, OVIR_CMD_DISPATCH);
  if (Cmd == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Cmd->As.Dispatch.GroupCountX = GroupCountX;
  Cmd->As.Dispatch.GroupCountY = GroupCountY;
  Cmd->As.Dispatch.GroupCountZ = GroupCountZ;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirCmdPipelineBarrier (
  IN OVIR_COMMAND_LIST   *CommandList,
  IN OVIR_HANDLE         ResourceHandle,
  IN OVIR_RESOURCE_STATE OldState,
  IN OVIR_RESOURCE_STATE NewState,
  IN UINT32              SrcStageMask,
  IN UINT32              DstStageMask
  )
{
  OVIR_COMMAND  *Cmd;

  Cmd = OvirAllocateCommandSlot (CommandList, OVIR_CMD_PIPELINE_BARRIER);
  if (Cmd == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Cmd->As.Barrier.ResourceHandle = ResourceHandle;
  Cmd->As.Barrier.OldState = OldState;
  Cmd->As.Barrier.NewState = NewState;
  Cmd->As.Barrier.SrcStageMask = SrcStageMask;
  Cmd->As.Barrier.DstStageMask = DstStageMask;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirCmdPresent (
  IN OVIR_COMMAND_LIST   *CommandList,
  IN OVIR_HANDLE         SwapChainOrTarget,
  IN UINT32              SyncInterval
  )
{
  OVIR_COMMAND  *Cmd;

  Cmd = OvirAllocateCommandSlot (CommandList, OVIR_CMD_PRESENT);
  if (Cmd == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Cmd->As.Present.SwapChainOrTarget = SwapChainOrTarget;
  Cmd->As.Present.SyncInterval = SyncInterval;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OvirValidateCommandList (
  IN  OVIR_COMMAND_LIST  *CommandList,
  OUT CHAR16             *ErrorBuffer,
  IN  UINTN              ErrorBufferSize
  )
{
  BOOLEAN  InsidePass;
  BOOLEAN  HasPipeline;
  UINT32   Index;

  if (CommandList == NULL) {
    if (ErrorBuffer != NULL && ErrorBufferSize > 0) {
      UnicodeSPrint (ErrorBuffer, ErrorBufferSize * sizeof (CHAR16), L"Command list pointer is NULL");
    }
    return EFI_INVALID_PARAMETER;
  }

  if (CommandList->IsRecording) {
    if (ErrorBuffer != NULL && ErrorBufferSize > 0) {
      UnicodeSPrint (ErrorBuffer, ErrorBufferSize * sizeof (CHAR16), L"Command list is still actively recording");
    }
    return EFI_NOT_READY;
  }

  InsidePass = FALSE;
  HasPipeline = FALSE;

  for (Index = 0; Index < CommandList->Count; Index++) {
    OVIR_COMMAND *Cmd = &CommandList->Commands[Index];

    switch (Cmd->Type) {
      case OVIR_CMD_BEGIN_RENDER_PASS:
        if (InsidePass) {
          if (ErrorBuffer != NULL && ErrorBufferSize > 0) {
            UnicodeSPrint (ErrorBuffer, ErrorBufferSize * sizeof (CHAR16), L"Nested render pass detected at index %u", Index);
          }
          return EFI_ABORTED;
        }
        InsidePass = TRUE;
        HasPipeline = FALSE;
        break;

      case OVIR_CMD_END_RENDER_PASS:
        if (!InsidePass) {
          if (ErrorBuffer != NULL && ErrorBufferSize > 0) {
            UnicodeSPrint (ErrorBuffer, ErrorBufferSize * sizeof (CHAR16), L"EndRenderPass without BeginRenderPass at index %u", Index);
          }
          return EFI_ABORTED;
        }
        InsidePass = FALSE;
        break;

      case OVIR_CMD_SET_PIPELINE:
        HasPipeline = TRUE;
        break;

      case OVIR_CMD_DRAW:
      case OVIR_CMD_DRAW_INDEXED:
        if (!InsidePass) {
          if (ErrorBuffer != NULL && ErrorBufferSize > 0) {
            UnicodeSPrint (ErrorBuffer, ErrorBufferSize * sizeof (CHAR16), L"Draw command outside render pass at index %u", Index);
          }
          return EFI_ABORTED;
        }
        if (!HasPipeline) {
          if (ErrorBuffer != NULL && ErrorBufferSize > 0) {
            UnicodeSPrint (ErrorBuffer, ErrorBufferSize * sizeof (CHAR16), L"Draw command without bound pipeline at index %u", Index);
          }
          return EFI_ABORTED;
        }
        break;

      default:
        break;
    }
  }

  if (InsidePass) {
    if (ErrorBuffer != NULL && ErrorBufferSize > 0) {
      UnicodeSPrint (ErrorBuffer, ErrorBufferSize * sizeof (CHAR16), L"Command list ended with unclosed render pass");
    }
    return EFI_ABORTED;
  }

  CommandList->IsValidated = TRUE;
  return EFI_SUCCESS;
}

CONST CHAR16 *
EFIAPI
OvirCommandTypeToString (
  IN OVIR_COMMAND_TYPE  Type
  )
{
  switch (Type) {
    case OVIR_CMD_BEGIN_RENDER_PASS:   return L"BEGIN_RENDER_PASS";
    case OVIR_CMD_END_RENDER_PASS:     return L"END_RENDER_PASS";
    case OVIR_CMD_SET_PIPELINE:        return L"SET_PIPELINE";
    case OVIR_CMD_SET_VIEWPORT:        return L"SET_VIEWPORT";
    case OVIR_CMD_SET_SCISSOR:         return L"SET_SCISSOR";
    case OVIR_CMD_BIND_VERTEX_BUFFER:  return L"BIND_VERTEX_BUFFER";
    case OVIR_CMD_BIND_INDEX_BUFFER:   return L"BIND_INDEX_BUFFER";
    case OVIR_CMD_BIND_DESCRIPTOR_SET: return L"BIND_DESCRIPTOR_SET";
    case OVIR_CMD_DRAW:                return L"DRAW";
    case OVIR_CMD_DRAW_INDEXED:        return L"DRAW_INDEXED";
    case OVIR_CMD_DISPATCH:            return L"DISPATCH";
    case OVIR_CMD_COPY_BUFFER:         return L"COPY_BUFFER";
    case OVIR_CMD_COPY_TEXTURE:        return L"COPY_TEXTURE";
    case OVIR_CMD_PIPELINE_BARRIER:    return L"PIPELINE_BARRIER";
    case OVIR_CMD_PRESENT:             return L"PRESENT";
    default:                           return L"UNKNOWN";
  }
}

CONST CHAR16 *
EFIAPI
OvirFormatToString (
  IN OVIR_FORMAT  Format
  )
{
  switch (Format) {
    case OVIR_FORMAT_R8_UNORM:          return L"R8_UNORM";
    case OVIR_FORMAT_RG8_UNORM:         return L"RG8_UNORM";
    case OVIR_FORMAT_RGBA8_UNORM:       return L"RGBA8_UNORM";
    case OVIR_FORMAT_BGRA8_UNORM:       return L"BGRA8_UNORM";
    case OVIR_FORMAT_RGBA16_FLOAT:      return L"RGBA16_FLOAT";
    case OVIR_FORMAT_RGBA32_FLOAT:      return L"RGBA32_FLOAT";
    case OVIR_FORMAT_R32_FLOAT:         return L"R32_FLOAT";
    case OVIR_FORMAT_RG32_FLOAT:        return L"RG32_FLOAT";
    case OVIR_FORMAT_D16_UNORM:         return L"D16_UNORM";
    case OVIR_FORMAT_D24_UNORM_S8_UINT: return L"D24_UNORM_S8_UINT";
    case OVIR_FORMAT_D32_FLOAT:         return L"D32_FLOAT";
    case OVIR_FORMAT_BC1_RGBA_UNORM:    return L"BC1_RGBA_UNORM";
    case OVIR_FORMAT_BC2_RGBA_UNORM:    return L"BC2_RGBA_UNORM";
    case OVIR_FORMAT_BC3_RGBA_UNORM:    return L"BC3_RGBA_UNORM";
    case OVIR_FORMAT_BC4_UNORM:         return L"BC4_UNORM";
    case OVIR_FORMAT_BC5_UNORM:         return L"BC5_UNORM";
    case OVIR_FORMAT_BC7_RGBA_UNORM:    return L"BC7_RGBA_UNORM";
    default:                            return L"UNKNOWN";
  }
}

CONST CHAR16 *
EFIAPI
OvirResourceStateToString (
  IN OVIR_RESOURCE_STATE  State
  )
{
  switch (State) {
    case OvirResourceStateCommon:         return L"Common";
    case OvirResourceStateVertexBuffer:   return L"VertexBuffer";
    case OvirResourceStateIndexBuffer:    return L"IndexBuffer";
    case OvirResourceStateConstantBuffer: return L"ConstantBuffer";
    case OvirResourceStateShaderResource: return L"ShaderResource";
    case OvirResourceStateRenderTarget:   return L"RenderTarget";
    case OvirResourceStateDepthWrite:     return L"DepthWrite";
    case OvirResourceStateDepthRead:      return L"DepthRead";
    case OvirResourceStateUnorderedAccess:return L"UnorderedAccess";
    case OvirResourceStateCopySource:     return L"CopySource";
    case OvirResourceStateCopyDest:       return L"CopyDest";
    case OvirResourceStatePresent:        return L"Present";
    default:                              return L"Unknown";
  }
}
