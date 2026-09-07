/** @file
  OpenVintage Graphics Intermediate Representation (OVIR-GPU).
  Phase 3 Modular Graphics Framework.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OVIR_GPU_LIB_H_
#define OVIR_GPU_LIB_H_

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>

#ifndef FLOAT32
typedef float FLOAT32;
#endif

#define OVIR_MAX_COLOR_ATTACHMENTS     8
#define OVIR_MAX_VERTEX_BINDINGS       16
#define OVIR_MAX_VERTEX_ATTRIBUTES     16
#define OVIR_MAX_DESCRIPTOR_SETS       4
#define OVIR_MAX_COMMANDS_DEFAULT      256
#define OVIR_INVALID_HANDLE            0xFFFFFFFF

typedef UINT32 OVIR_HANDLE;

//
// Pixel and Buffer Data Formats
//
typedef enum {
  OVIR_FORMAT_UNKNOWN = 0,
  OVIR_FORMAT_R8_UNORM,
  OVIR_FORMAT_RG8_UNORM,
  OVIR_FORMAT_RGBA8_UNORM,
  OVIR_FORMAT_BGRA8_UNORM,
  OVIR_FORMAT_RGBA16_FLOAT,
  OVIR_FORMAT_RGBA32_FLOAT,
  OVIR_FORMAT_R32_FLOAT,
  OVIR_FORMAT_RG32_FLOAT,
  OVIR_FORMAT_D16_UNORM,
  OVIR_FORMAT_D24_UNORM_S8_UINT,
  OVIR_FORMAT_D32_FLOAT,
  OVIR_FORMAT_BC1_RGBA_UNORM,
  OVIR_FORMAT_BC2_RGBA_UNORM,
  OVIR_FORMAT_BC3_RGBA_UNORM,
  OVIR_FORMAT_BC4_UNORM,
  OVIR_FORMAT_BC5_UNORM,
  OVIR_FORMAT_BC7_RGBA_UNORM
} OVIR_FORMAT;

//
// GPU Resource Types
//
typedef enum {
  OvirResourceBuffer = 1,
  OvirResourceTexture1D,
  OvirResourceTexture2D,
  OvirResourceTexture3D,
  OvirResourceTextureCube,
  OvirResourceSampler,
  OvirResourceRenderTarget
} OVIR_RESOURCE_TYPE;

//
// Resource Tracking & Transition States
//
typedef enum {
  OvirResourceStateCommon = 0,
  OvirResourceStateVertexBuffer = 1,
  OvirResourceStateIndexBuffer = 2,
  OvirResourceStateConstantBuffer = 3,
  OvirResourceStateShaderResource = 4,
  OvirResourceStateRenderTarget = 5,
  OvirResourceStateDepthWrite = 6,
  OvirResourceStateDepthRead = 7,
  OvirResourceStateUnorderedAccess = 8,
  OvirResourceStateCopySource = 9,
  OvirResourceStateCopyDest = 10,
  OvirResourceStatePresent = 11
} OVIR_RESOURCE_STATE;

//
// Buffer Usages
//
#define OVIR_BUFFER_USAGE_VERTEX         0x00000001
#define OVIR_BUFFER_USAGE_INDEX          0x00000002
#define OVIR_BUFFER_USAGE_UNIFORM        0x00000004
#define OVIR_BUFFER_USAGE_STORAGE        0x00000008
#define OVIR_BUFFER_USAGE_INDIRECT       0x00000010
#define OVIR_BUFFER_USAGE_TRANSFER_SRC   0x00000020
#define OVIR_BUFFER_USAGE_TRANSFER_DST   0x00000040

//
// Texture Usages
//
#define OVIR_TEXTURE_USAGE_SAMPLED        0x00000001
#define OVIR_TEXTURE_USAGE_STORAGE        0x00000002
#define OVIR_TEXTURE_USAGE_RENDER_TARGET  0x00000004
#define OVIR_TEXTURE_USAGE_DEPTH_STENCIL  0x00000008
#define OVIR_TEXTURE_USAGE_TRANSFER_SRC   0x00000010
#define OVIR_TEXTURE_USAGE_TRANSFER_DST   0x00000020

//
// Shader Stages
//
#define OVIR_SHADER_STAGE_VERTEX         0x00000001
#define OVIR_SHADER_STAGE_FRAGMENT       0x00000002
#define OVIR_SHADER_STAGE_COMPUTE        0x00000004
#define OVIR_SHADER_STAGE_GEOMETRY       0x00000008
#define OVIR_SHADER_STAGE_TESS_CTRL      0x00000010
#define OVIR_SHADER_STAGE_TESS_EVAL      0x00000020

//
// Primitive Topologies
//
typedef enum {
  OVIR_TOPOLOGY_POINT_LIST = 1,
  OVIR_TOPOLOGY_LINE_LIST,
  OVIR_TOPOLOGY_LINE_STRIP,
  OVIR_TOPOLOGY_TRIANGLE_LIST,
  OVIR_TOPOLOGY_TRIANGLE_STRIP
} OVIR_PRIMITIVE_TOPOLOGY;

//
// Rasterizer & Blending States
//
typedef enum {
  OVIR_CULL_NONE = 0,
  OVIR_CULL_FRONT,
  OVIR_CULL_BACK
} OVIR_CULL_MODE;

typedef enum {
  OVIR_FRONT_FACE_CCW = 0,
  OVIR_FRONT_FACE_CW
} OVIR_FRONT_FACE;

typedef enum {
  OVIR_POLYGON_MODE_FILL = 0,
  OVIR_POLYGON_MODE_LINE,
  OVIR_POLYGON_MODE_POINT
} OVIR_POLYGON_MODE;

typedef enum {
  OVIR_BLEND_FACTOR_ZERO = 0,
  OVIR_BLEND_FACTOR_ONE,
  OVIR_BLEND_FACTOR_SRC_COLOR,
  OVIR_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
  OVIR_BLEND_FACTOR_SRC_ALPHA,
  OVIR_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
  OVIR_BLEND_FACTOR_DST_ALPHA,
  OVIR_BLEND_FACTOR_ONE_MINUS_DST_ALPHA
} OVIR_BLEND_FACTOR;

typedef enum {
  OVIR_BLEND_OP_ADD = 0,
  OVIR_BLEND_OP_SUBTRACT,
  OVIR_BLEND_OP_REVERSE_SUBTRACT,
  OVIR_BLEND_OP_MIN,
  OVIR_BLEND_OP_MAX
} OVIR_BLEND_OP;

typedef enum {
  OVIR_COMPARE_OP_NEVER = 0,
  OVIR_COMPARE_OP_LESS,
  OVIR_COMPARE_OP_EQUAL,
  OVIR_COMPARE_OP_LESS_OR_EQUAL,
  OVIR_COMPARE_OP_GREATER,
  OVIR_COMPARE_OP_NOT_EQUAL,
  OVIR_COMPARE_OP_GREATER_OR_EQUAL,
  OVIR_COMPARE_OP_ALWAYS
} OVIR_COMPARE_OP;

typedef enum {
  OVIR_LOAD_OP_DONT_CARE = 0,
  OVIR_LOAD_OP_LOAD,
  OVIR_LOAD_OP_CLEAR
} OVIR_LOAD_OP;

typedef enum {
  OVIR_STORE_OP_DONT_CARE = 0,
  OVIR_STORE_OP_STORE
} OVIR_STORE_OP;

//
// Command Types
//
typedef enum {
  OVIR_CMD_UNKNOWN = 0,
  OVIR_CMD_BEGIN_RENDER_PASS,
  OVIR_CMD_END_RENDER_PASS,
  OVIR_CMD_SET_PIPELINE,
  OVIR_CMD_SET_VIEWPORT,
  OVIR_CMD_SET_SCISSOR,
  OVIR_CMD_BIND_VERTEX_BUFFER,
  OVIR_CMD_BIND_INDEX_BUFFER,
  OVIR_CMD_BIND_DESCRIPTOR_SET,
  OVIR_CMD_DRAW,
  OVIR_CMD_DRAW_INDEXED,
  OVIR_CMD_DISPATCH,
  OVIR_CMD_COPY_BUFFER,
  OVIR_CMD_COPY_TEXTURE,
  OVIR_CMD_PIPELINE_BARRIER,
  OVIR_CMD_PRESENT
} OVIR_COMMAND_TYPE;

//
// Command Payloads
//
typedef struct {
  OVIR_HANDLE   RenderTargetHandle;
  FLOAT32       ClearColor[4];
  FLOAT32       ClearDepth;
  UINT32        ClearStencil;
  OVIR_LOAD_OP  LoadOp;
  OVIR_STORE_OP StoreOp;
} OVIR_CMD_BEGIN_PASS_PAYLOAD;

typedef struct {
  OVIR_HANDLE   PipelineHandle;
} OVIR_CMD_SET_PIPELINE_PAYLOAD;

typedef struct {
  FLOAT32       X;
  FLOAT32       Y;
  FLOAT32       Width;
  FLOAT32       Height;
  FLOAT32       MinDepth;
  FLOAT32       MaxDepth;
} OVIR_CMD_SET_VIEWPORT_PAYLOAD;

typedef struct {
  INT32         X;
  INT32         Y;
  UINT32        Width;
  UINT32        Height;
} OVIR_CMD_SET_SCISSOR_PAYLOAD;

typedef struct {
  UINT32        BindingSlot;
  OVIR_HANDLE   BufferHandle;
  UINT64        Offset;
  UINT32        Stride;
} OVIR_CMD_BIND_VB_PAYLOAD;

typedef struct {
  OVIR_HANDLE   BufferHandle;
  UINT64        Offset;
  UINT32        IndexTypeSize; // 2 for UINT16, 4 for UINT32
} OVIR_CMD_BIND_IB_PAYLOAD;

typedef struct {
  UINT32        SetIndex;
  OVIR_HANDLE   DescriptorSetHandle;
} OVIR_CMD_BIND_DESCRIPTOR_SET_PAYLOAD;

typedef struct {
  UINT32        VertexCount;
  UINT32        InstanceCount;
  UINT32        FirstVertex;
  UINT32        FirstInstance;
} OVIR_CMD_DRAW_PAYLOAD;

typedef struct {
  UINT32        IndexCount;
  UINT32        InstanceCount;
  UINT32        FirstIndex;
  INT32         VertexOffset;
  UINT32        FirstInstance;
} OVIR_CMD_DRAW_INDEXED_PAYLOAD;

typedef struct {
  UINT32        GroupCountX;
  UINT32        GroupCountY;
  UINT32        GroupCountZ;
} OVIR_CMD_DISPATCH_PAYLOAD;

typedef struct {
  OVIR_HANDLE   SrcBuffer;
  OVIR_HANDLE   DstBuffer;
  UINT64        SrcOffset;
  UINT64        DstOffset;
  UINT64        ByteCount;
} OVIR_CMD_COPY_BUFFER_PAYLOAD;

typedef struct {
  OVIR_HANDLE   SrcTexture;
  OVIR_HANDLE   DstTexture;
  UINT32        Width;
  UINT32        Height;
} OVIR_CMD_COPY_TEXTURE_PAYLOAD;

typedef struct {
  OVIR_HANDLE         ResourceHandle;
  OVIR_RESOURCE_STATE OldState;
  OVIR_RESOURCE_STATE NewState;
  UINT32              SrcStageMask;
  UINT32              DstStageMask;
} OVIR_CMD_BARRIER_PAYLOAD;

typedef struct {
  OVIR_HANDLE   SwapChainOrTarget;
  UINT32        SyncInterval;
} OVIR_CMD_PRESENT_PAYLOAD;

//
// Unified Command Structure
//
typedef struct {
  OVIR_COMMAND_TYPE Type;
  UINT32            SequenceId;
  union {
    OVIR_CMD_BEGIN_PASS_PAYLOAD           BeginPass;
    OVIR_CMD_SET_PIPELINE_PAYLOAD         SetPipeline;
    OVIR_CMD_SET_VIEWPORT_PAYLOAD         SetViewport;
    OVIR_CMD_SET_SCISSOR_PAYLOAD          SetScissor;
    OVIR_CMD_BIND_VB_PAYLOAD              BindVertexBuffer;
    OVIR_CMD_BIND_IB_PAYLOAD              BindIndexBuffer;
    OVIR_CMD_BIND_DESCRIPTOR_SET_PAYLOAD  BindDescriptorSet;
    OVIR_CMD_DRAW_PAYLOAD                 Draw;
    OVIR_CMD_DRAW_INDEXED_PAYLOAD         DrawIndexed;
    OVIR_CMD_DISPATCH_PAYLOAD             Dispatch;
    OVIR_CMD_COPY_BUFFER_PAYLOAD          CopyBuffer;
    OVIR_CMD_COPY_TEXTURE_PAYLOAD         CopyTexture;
    OVIR_CMD_BARRIER_PAYLOAD              Barrier;
    OVIR_CMD_PRESENT_PAYLOAD              Present;
  } As;
} OVIR_COMMAND;

//
// Command List Stream Container
//
typedef struct {
  UINT32        Capacity;
  UINT32        Count;
  BOOLEAN       IsRecording;
  BOOLEAN       IsValidated;
  OVIR_COMMAND  *Commands;
} OVIR_COMMAND_LIST;

//
// Resource Descriptors
//
typedef struct {
  UINT64        SizeBytes;
  UINT32        UsageFlags;
  BOOLEAN       HostVisible;
  CHAR16        DebugName[32];
} OVIR_BUFFER_DESC;

typedef struct {
  OVIR_FORMAT   Format;
  UINT32        Width;
  UINT32        Height;
  UINT32        Depth;
  UINT32        MipLevels;
  UINT32        ArrayLayers;
  UINT32        SampleCount;
  UINT32        UsageFlags;
  CHAR16        DebugName[32];
} OVIR_TEXTURE_DESC;

typedef struct {
  UINT32        FilterMin;
  UINT32        FilterMag;
  UINT32        FilterMip;
  UINT32        AddressModeU;
  UINT32        AddressModeV;
  UINT32        AddressModeW;
  FLOAT32       MaxAnisotropy;
  BOOLEAN       CompareEnable;
  OVIR_COMPARE_OP CompareOp;
} OVIR_SAMPLER_DESC;

typedef struct {
  UINT32        ColorAttachmentCount;
  OVIR_HANDLE   ColorAttachments[OVIR_MAX_COLOR_ATTACHMENTS];
  OVIR_HANDLE   DepthStencilAttachment;
  UINT32        Width;
  UINT32        Height;
} OVIR_RENDER_TARGET_DESC;

//
// Pipeline State Components
//
typedef struct {
  OVIR_CULL_MODE    CullMode;
  OVIR_FRONT_FACE   FrontFace;
  OVIR_POLYGON_MODE PolygonMode;
  BOOLEAN           DepthClampEnable;
  BOOLEAN           ScissorEnable;
} OVIR_RASTERIZER_DESC;

typedef struct {
  BOOLEAN           BlendEnable;
  OVIR_BLEND_FACTOR SrcColorBlendFactor;
  OVIR_BLEND_FACTOR DstColorBlendFactor;
  OVIR_BLEND_OP     ColorBlendOp;
  OVIR_BLEND_FACTOR SrcAlphaBlendFactor;
  OVIR_BLEND_FACTOR DstAlphaBlendFactor;
  OVIR_BLEND_OP     AlphaBlendOp;
  UINT8             ColorWriteMask;
} OVIR_RENDER_TARGET_BLEND_DESC;

typedef struct {
  BOOLEAN                       AlphaToCoverageEnable;
  OVIR_RENDER_TARGET_BLEND_DESC RenderTargets[OVIR_MAX_COLOR_ATTACHMENTS];
} OVIR_BLEND_DESC;

typedef struct {
  BOOLEAN         DepthTestEnable;
  BOOLEAN         DepthWriteEnable;
  OVIR_COMPARE_OP DepthCompareOp;
  BOOLEAN         StencilTestEnable;
} OVIR_DEPTH_STENCIL_DESC;

typedef struct {
  UINT32        Location;
  UINT32        Binding;
  OVIR_FORMAT   Format;
  UINT32        Offset;
} OVIR_VERTEX_ATTRIBUTE_DESC;

typedef struct {
  UINT32        Binding;
  UINT32        Stride;
  BOOLEAN       StepPerInstance;
} OVIR_VERTEX_BINDING_DESC;

typedef struct {
  UINT32                      AttributeCount;
  OVIR_VERTEX_ATTRIBUTE_DESC  Attributes[OVIR_MAX_VERTEX_ATTRIBUTES];
  UINT32                      BindingCount;
  OVIR_VERTEX_BINDING_DESC    Bindings[OVIR_MAX_VERTEX_BINDINGS];
  OVIR_PRIMITIVE_TOPOLOGY     Topology;
} OVIR_INPUT_LAYOUT_DESC;

typedef struct {
  BOOLEAN                 IsCompute;
  OVIR_HANDLE             VertexShader;
  OVIR_HANDLE             FragmentShader;
  OVIR_HANDLE             ComputeShader;
  OVIR_INPUT_LAYOUT_DESC  InputLayout;
  OVIR_RASTERIZER_DESC    Rasterizer;
  OVIR_BLEND_DESC         Blend;
  OVIR_DEPTH_STENCIL_DESC DepthStencil;
  UINT64                  PipelineHash;
} OVIR_PIPELINE_DESC;

//
// Core Functions
//
EFI_STATUS
EFIAPI
OvirGpuInitialize (
  VOID
  );

EFI_STATUS
EFIAPI
OvirCreateCommandList (
  IN  UINT32             InitialCapacity,
  OUT OVIR_COMMAND_LIST  **CommandList
  );

EFI_STATUS
EFIAPI
OvirDestroyCommandList (
  IN OVIR_COMMAND_LIST   *CommandList
  );

EFI_STATUS
EFIAPI
OvirBeginCommandList (
  IN OVIR_COMMAND_LIST   *CommandList
  );

EFI_STATUS
EFIAPI
OvirEndCommandList (
  IN OVIR_COMMAND_LIST   *CommandList
  );

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
  );

EFI_STATUS
EFIAPI
OvirCmdEndRenderPass (
  IN OVIR_COMMAND_LIST   *CommandList
  );

EFI_STATUS
EFIAPI
OvirCmdSetPipeline (
  IN OVIR_COMMAND_LIST   *CommandList,
  IN OVIR_HANDLE         PipelineHandle
  );

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
  );

EFI_STATUS
EFIAPI
OvirCmdSetScissor (
  IN OVIR_COMMAND_LIST   *CommandList,
  IN INT32               X,
  IN INT32               Y,
  IN UINT32              Width,
  IN UINT32              Height
  );

EFI_STATUS
EFIAPI
OvirCmdBindVertexBuffer (
  IN OVIR_COMMAND_LIST   *CommandList,
  IN UINT32              Slot,
  IN OVIR_HANDLE         BufferHandle,
  IN UINT64              Offset,
  IN UINT32              Stride
  );

EFI_STATUS
EFIAPI
OvirCmdBindIndexBuffer (
  IN OVIR_COMMAND_LIST   *CommandList,
  IN OVIR_HANDLE         BufferHandle,
  IN UINT64              Offset,
  IN UINT32              IndexTypeSize
  );

EFI_STATUS
EFIAPI
OvirCmdDraw (
  IN OVIR_COMMAND_LIST   *CommandList,
  IN UINT32              VertexCount,
  IN UINT32              InstanceCount,
  IN UINT32              FirstVertex,
  IN UINT32              FirstInstance
  );

EFI_STATUS
EFIAPI
OvirCmdDrawIndexed (
  IN OVIR_COMMAND_LIST   *CommandList,
  IN UINT32              IndexCount,
  IN UINT32              InstanceCount,
  IN UINT32              FirstIndex,
  IN INT32               VertexOffset,
  IN UINT32              FirstInstance
  );

EFI_STATUS
EFIAPI
OvirCmdDispatch (
  IN OVIR_COMMAND_LIST   *CommandList,
  IN UINT32              GroupCountX,
  IN UINT32              GroupCountY,
  IN UINT32              GroupCountZ
  );

EFI_STATUS
EFIAPI
OvirCmdPipelineBarrier (
  IN OVIR_COMMAND_LIST   *CommandList,
  IN OVIR_HANDLE         ResourceHandle,
  IN OVIR_RESOURCE_STATE OldState,
  IN OVIR_RESOURCE_STATE NewState,
  IN UINT32              SrcStageMask,
  IN UINT32              DstStageMask
  );

EFI_STATUS
EFIAPI
OvirCmdPresent (
  IN OVIR_COMMAND_LIST   *CommandList,
  IN OVIR_HANDLE         SwapChainOrTarget,
  IN UINT32              SyncInterval
  );

EFI_STATUS
EFIAPI
OvirValidateCommandList (
  IN  OVIR_COMMAND_LIST  *CommandList,
  OUT CHAR16             *ErrorBuffer,
  IN  UINTN              ErrorBufferSize
  );

CONST CHAR16 *
EFIAPI
OvirCommandTypeToString (
  IN OVIR_COMMAND_TYPE   Type
  );

CONST CHAR16 *
EFIAPI
OvirFormatToString (
  IN OVIR_FORMAT         Format
  );

CONST CHAR16 *
EFIAPI
OvirResourceStateToString (
  IN OVIR_RESOURCE_STATE State
  );

#endif // OVIR_GPU_LIB_H_
