# OPENVINTAGE ARCHITECTURAL SPECIFICATION

```
================================================================================
OPENVINTAGE SYSTEMS ARCHITECTURE
Revision: 2.1.0
Classification: Systems Engineering Specification
Status: Active Implementation & Verified EDK II Firmware Pipeline
================================================================================
```

---

## 1. Architectural Overview

OpenVintage rejects the traditional $M \times N$ matrix of direct API-to-API bridges:

```
[ Traditional Brittle Matrix: O(M x N) ]
Metal    ──► OpenGL
Metal    ──► Vulkan
Vulkan   ──► OpenGL
DirectX  ──► Metal
DirectX  ──► OpenGL

[ OpenVintage Hub & Spoke IR: O(M + N) ]
Metal    ──┐
Vulkan   ──┼──► [ OVIR-GPU ] ──► Target Hardware Backend (OpenGL / Metal / Vulkan)
DirectX  ──┤
OpenGL   ──┘
```

The system is decomposed into three primary control layers, a firmware platform foundation, and two translation hubs:

```
                           OPENVINTAGE ECOSYSTEM
                                     │
              ┌──────────────────────┼──────────────────────┐
              │                      │                      │
       OpenVintagePkg            OVResolver            OVScheduler
     (Firmware & UEFI)      (Intelligent Routing)  (Resource Manager)
              │                      │                      │
       ┌──────┴──────┐               │               ┌──────┴──────┐
       │             │               │               │             │
   Firmware      Bootloader       OVIR-GPU        CPU Cores     Thermal /
 (OPENVINTAGE.fd) (.EFI App)    (Graphics IR)      Affinity     RAM Budgets
              │                      │
       ┌──────┴──────┐               │
       │             │         Graphics APIs
   HAL DXE       Core Libs      (Metal / VK /
  (Protocols)   (CPUID / Mem)   GL / DirectX)
```

---

## 2. Firmware & Bootloader Subsystems (`OpenVintagePkg`)

### 2.1 EDK II Package Structure
`OpenVintagePkg` is fully integrated into the Tianocore EDK II build environment:

```
OpenVintagePkg/
├── OpenVintagePkg.dec              # Public interfaces, GUIDs, token space
├── OpenVintagePkg.dsc              # Platform description (GCC5 / X64)
├── OpenVintagePkg.fdf              # Flash layout (FD and FV definitions)
├── Include/
│   ├── Library/
│   │   ├── OvCoreLib.h             # Phased initialization & central state
│   │   ├── OvConfigLib.h           # Profile management & bitwise flags
│   │   ├── OvMemoryLib.h           # Tracked tagged allocation & leak check
│   │   ├── OvHardwareLib.h         # Unified CPU, RAM, GPU, Storage topology
│   │   ├── OvModuleLib.h           # Module registration & lifecycle
│   │   ├── OvResolverLib.h         # Capability matrix & workload routing
│   │   ├── OvSchedulerLib.h        # Priority task queue & resource limits
│   │   ├── OvLoggerLib.h           # Tagged multi-level logging API
│   │   ├── OpenVintageCoreLib.h    # Legacy platform detection API
│   │   └── OpenVintageLogLib.h     # Low-level logging API
│   └── Protocol/
│       ├── OpenVintageHal.h        # OPEN_VINTAGE_HAL_PROTOCOL
│       └── OpenVintagePlatform.h   # OPEN_VINTAGE_PLATFORM_PROTOCOL
├── Core/                           # Core orchestration & configuration
│   ├── OvCore.c / OvCoreLib.inf
│   ├── OvConfig.c / OvConfigLib.inf
│   └── OvModule.c / OvModuleLib.inf
├── Hardware/                       # Hardware discovery & topology
│   └── OvHardware.c / OvHardwareLib.inf
├── Memory/                         # Tracked memory management
│   └── OvMemory.c / OvMemoryLib.inf
├── Resolver/                       # Workload routing & capability matrix
│   └── OvResolver.c / OvResolverLib.inf
├── Scheduler/                      # Priority scheduler & task queue
│   └── OvScheduler.c / OvSchedulerLib.inf
├── Library/
│   ├── OvLoggerLib/                # Structured tagged logger
│   ├── OpenVintageCoreLib/         # Base CPUID & Memory map
│   └── OpenVintageLogLib/          # Base serial/screen logger
├── Drivers/
│   └── OpenVintageHalDxe/          # DXE Service Protocol Provider
├── OpenVintageBootApp/             # Native X64 UEFI Entry Point
├── Tests/                          # Architectural test application
│   └── OvSelfTestApp.c / .inf      # 8-suite self-test diagnostic
└── Firmware/                       # Generated flash image and volumes
    ├── OPENVINTAGE.fd              # 4.0 MB Flash Device Image
    └── OPENVINTAGE_DXEFV.Fv        # 4.0 MB DXE Firmware Volume
```

### 2.2 OpenVintage Protocols
- **`OPEN_VINTAGE_HAL_PROTOCOL`** (`gOpenVintageHalProtocolGuid`):
  Installed during DXE phase by `OpenVintageHalDxe`. Exposes hardware abstraction calls:
  - `GetCpuCapabilities`: Evaluates CPU family, model, stepping, and instruction features (SSE4.1, SSE4.2, AVX, AVX2, AES-NI).
  - `GetGpuCapabilities`: Interrogates PCI configuration space for Display Controllers (PCI class 0x03) and identifies Intel HD, Nvidia GeForce/Quadro, and AMD Radeon adapters.
- **`OPEN_VINTAGE_PLATFORM_PROTOCOL`** (`gOpenVintagePlatformProtocolGuid`):
  Exposes platform revision, vendor information, and firmware health status.

### 2.3 Firmware Volume & Flash Device Layout
The firmware definition file (`OpenVintagePkg.fdf`) defines a 4MB memory-mapped flash layout:

```
+-------------------------------------------------------------------+
| Flash Device: OPENVINTAGE.fd (Base: 0xFFC00000, Size: 0x400000)  |
+-------------------------------------------------------------------+
| 0x00000000 - 0x00400000 : OPENVINTAGE_DXEFV (4096 KB)             |
|   ├── OpenVintageHalDxe.ffs (DXE Driver + Depex + Version)        |
|   ├── OpenVintageBootApp.ffs (UEFI Application + UI Section)      |
|   ├── OvSelfTestApp.ffs (Diagnostic Self-Test Application)        |
|   └── Free Flash Space (3936 KB available for additional modules) |
+-------------------------------------------------------------------+
```

### 2.4 Modular Foundation Subsystems (Phase 2)
The OpenVintage architecture decomposes system responsibilities into eight specialized, decoupled subsystems:

1. **OvCoreLib (`OpenVintagePkg/Core/OvCoreLib.*`)**:
   Central coordinator orchestrating a strict 7-phase boot initialization pipeline:
   `Logger` ──► `Memory` ──► `Config` ──► `Hardware` ──► `Modules` ──► `Resolver` ──► `Scheduler`.
   Maintains the global platform state machine (`Uninitialized` ──► `Initializing` ──► `Ready` / `Degraded` / `Shutdown`).

2. **OvConfigLib (`OpenVintagePkg/Core/OvConfigLib.*`)**:
   Dynamic hardware profile management and bitwise feature flags:
   `CompatibilityMode`, `GpuTranslation`, `CpuEmulation`, `MemoryTrack`, `PowerOptimize`, `DebugVerbosity`.
   Allows runtime overriding of hardware targets and silicon generation profiles.

3. **OvMemoryLib (`OpenVintagePkg/Memory/OvMemoryLib.*`)**:
   Memory safety layer with tagged allocations (`CORE`, `CONF`, `HARD`, `MODU`, `RESO`, `SCHD`, `TEST`, `BUFF`), 64-byte alignment, internal canary validation, cumulative leak detection (`OvMemoryVerifyNoLeaks`), and real-time telemetry (peak bytes, active counts).

4. **OvHardwareLib (`OpenVintagePkg/Hardware/OvHardwareLib.*`)**:
   Unified topology discovery engine interrogating:
   - **CPU**: CPUID feature extraction (Stepping, Microcode, SSE4.1/4.2, AVX, AVX2, AES, cores, brand string).
   - **RAM**: UEFI memory map descriptors, total physical memory calculation, and conventional free pages.
   - **GPU**: Graphics Output Protocol (GOP) resolution probe combined with PCI Class 0x03 configuration space enumeration (Intel HD, Nvidia GeForce, AMD Radeon).
   - **Storage**: Block I/O device discovery and mass storage controller classification (SATA/AHCI/NVMe).

5. **OvModuleLib (`OpenVintagePkg/Core/OvModuleLib.*`)**:
   Dynamic module registry supporting registration by priority, lifecycle states (`Registered`, `Initialized`, `Active`, `Degraded`, `Halted`), and dependency-ordered initialization and shutdown.

6. **OvResolverLib (`OpenVintagePkg/Resolver/OvResolverLib.*`)**:
   Intelligent hardware-aware workload resolution matrix. Dissects compute and graphical requirements (Metal, Vulkan, AVX2, SIMD) against detected silicon capabilities to select optimal execution pathways: `Native`, `Translated` (via OVIR), `Emulated`, `Fallback` (CPU software rasterizer), or `Unsupported`, accompanied by performance cost factor estimates.

7. **OvSchedulerLib (`OpenVintagePkg/Scheduler/OvSchedulerLib.*`)**:
   Priority-based task queueing engine (5 priority levels: `Idle` to `Realtime`). Enforces task resource quotas (memory, time limits), provides round-robin dispatch, and records task lifecycle performance metrics.

8. **OvLoggerLib (`OpenVintagePkg/Library/OvLoggerLib/`)**:
   High-performance structured logger formatting logs with timestamps, severity levels (`[DBG]`, `[INF]`, `[WRN]`, `[ERR]`), and subsystem tags (`[MEM]`, `[CONF]`, `[HW]`, `[MOD]`, `[RESO]`, `[SCHD]`, `[TEST]`) across UEFI console and serial output.

---

## 3. Native Boot Application (`OpenVintageBootApp.efi`)

The boot application provides early hardware telemetry and platform verification:
1. **Banner & Logging**: Initializes serial port and UEFI console output.
2. **Firmware Identification**: Queries `gST->FirmwareVendor`, revision, and UEFI specification version.
3. **CPU Architecture Analysis**:
   - Executes standard and extended CPUID functions (0x00000001, 0x00000007, 0x80000002-0x80000004).
   - Maps silicon family to known hardware profiles (Core 2, Nehalem/Westmere, Sandy Bridge, Ivy Bridge, Haswell/Broadwell).
   - Verifies SIMD vector extensions: SSE4.1, SSE4.2, AES-NI, AVX, AVX2.
4. **Memory Topology Walk**:
   - Invokes `gBS->GetMemoryMap` to enumerate memory descriptor regions.
   - Computes total physical RAM and available conventional memory.
5. **Device & Protocol Discovery**:
   - Enumerates active handles via `gBS->LocateHandleBuffer`.
   - Discovers Graphics Output Protocol (GOP) framebuffers, resolution, and base MMIO address.
   - Walks PCI bus devices and identifies bridges, mass storage controllers, and display adapters.
6. **Graceful Exit**: Returns `EFI_SUCCESS` to UEFI Boot Services.

---

## 4. Workload Resolution & Scheduling

```
                      Incoming Workload Request
                                 │
                                 ▼
                         [ OVResolver ]
                                 │
            ┌────────────────────┼────────────────────┐
            ▼                    ▼                    ▼
     Inspect Hardware     Query Capability     Check Multi-Tier
     Topology (CPU/GPU)       Database          Cache Index
            │                    │                    │
            └────────────────────┼────────────────────┘
                                 │
                                 ▼
                     Execution Path Decision
         ┌──────────────┬──────────────┬──────────────┬──────────────┐
         ▼              ▼              ▼              ▼              ▼
     [Native]     [Translation]  [Simplification] [Cache Hit]   [CPU Fallback /
   (Hardware has  (Transform via  (Clamp features   (Reuse pre-    Unsupported]
   native support)  OVIR to HAL)   for stability)  compiled blob)
```

### Decision Rules:
1. **Pass-through / Native**: Workload matches target GPU hardware capabilities.
2. **Intermediate Translation**: Features exceeding hardware are transformed through OVIR-GPU command DAGs.
3. **Feature Simplification**: Non-essential features (e.g., MSAA 8x, 4K render targets) are clamped to fit legacy VRAM constraints.
4. **Cache Re-use**: Lookups by composite hash `SHA256(Source + Driver + Hardware_UUID)`.
5. **CPU Fallback**: Route unsupported compute or geometry shaders to SIMD CPU worker threads.

---

## 5. OVIR-GPU Graphics Intermediate Representation

```
 Application Graphics API (Metal / Vulkan / OpenGL / DirectX)
                             │
                             ▼
                 [ Graphics API Adapters ]
            (MetalAdapter, VulkanAdapter, etc.)
                             │
                             ▼
                   ┌───────────────────┐
                   │     OVIR-GPU      │
                   │ Intermediate Rep  │
                   └─────────┬─────────┘
                             │
            ┌────────────────┼────────────────┐
            ▼                ▼                ▼
     Command Stream     Shader Engine    Pipeline / PSO
      & RenderPass       & Bytecode       State Tracking
     Validation Rules       Cache            & Cache
            │                │                │
            └────────────────┼────────────────┘
                             │
                             ▼
                   [ Graphics Resolver ]
           (Evaluates Silicon Capability Matrix)
                             │
         ┌───────────┬───────┴───────┬───────────┐
         ▼           ▼               ▼           ▼
      Native    Translated      Simplified    Fallback
     Silicon     Execution      Execution    SIMD / CPU
```

### 5.1 The Hub-and-Spoke IR Model
Direct translators between every pair of graphics APIs result in an unmaintainable $O(M \times N)$ combination matrix. OpenVintage enforces a clean Hub-and-Spoke model:
- **API Adapters (Spokes)**: Ingest commands, shaders, and state from application-level APIs (Metal, Vulkan, OpenGL, DirectX) into OVIR-GPU structs.
- **OVIR-GPU Hub**: Canonical, state-validated, hardware-agnostic IR representing commands, buffers, textures, samplers, pipelines, render passes, and synchronization barriers.
- **Resolver Bridge**: Evaluates the IR command stream against detected GPU capabilities (`OVIR_GPU_CAPABILITIES`) to select execution strategies without coupling APIs to target hardware.

### 5.2 Command Stream & Render Pass Architecture
- **Command Recording**: Commands (`Draw`, `DrawIndexed`, `Dispatch`, `BindPipeline`, `SetViewport`, `SetScissor`, `SetPushConstants`, `PipelineBarrier`) are recorded into structured IR lists.
- **Validation**: Strict validation rules ensure:
  - Render pass balance: `EndRenderPass` without matching `BeginRenderPass` is rejected.
  - Resource binding: Pipelines and vertex/index buffers must be bound before draw calls.
  - Handle validity: Zero or invalid handles (`OVIR_INVALID_HANDLE`) fail gracefully.

### 5.3 Modular Shader Subsystem (`OvirShaderLib`)
- **SPIR-V Ingestion**: Direct ingestion of SPIR-V bytecode modules with reflection extraction (descriptor sets, bindings, stage masks).
- **Composite Hash Cache**: FNV-1a 64-bit hashing guarantees duplicate shaders are recognized instantly, returning cached shader instances with zero redundant memory allocations.
- **Language Support**: Abstract representation for SPIR-V, MSL, GLSL, HLSL, and custom OVIR bytecode.

### 5.4 Pipeline State Object Subsystem (`OvirPipelineLib`)
- **Deterministic State Hashing**: Hashes blend states, rasterizer configurations, depth-stencil setups, and shader handles.
- **Fast PSO Cache**: Pointer-identity resolution for matching pipeline descriptors, eliminating redundant GPU state compilation.

### 5.5 Resource System & VRAM Pool (`OvirResourceLib`)
- **Tracked Allocations**: Buffers and textures (1D, 2D, 3D, Cube) allocated with precise usage flags (`OVIR_BUFFER_USAGE_*`, `OVIR_TEXTURE_USAGE_*`).
- **Dynamic VRAM Budgeting**: Centralized tracking pool maintaining total, committed, and peak video memory usage, preventing silent out-of-memory driver crashes.

### 5.6 Telemetry & Timing (`OvirPerfLib`)
- **Hardware TSC Cycle Counters**: Read direct CPU timestamp counters (`AsmReadTsc`) to measure compilation latency and dispatch times with zero operating system overhead.
- **Cache Efficiency Metrics**: Continuous monitoring of hit/miss ratios for shaders and pipelines.

---

## 6. QEMU Virtualized Test Architecture

Testing is executed in an automated, headless virtual machine environment:
- **Host Test Harness**: `scripts/test_qemu.sh`
- **Firmware Base**: Tianocore OVMF X64 (`/usr/share/ovmf/OVMF.fd`)
- **Virtual Disk**: 64MB FAT32 ESP disk containing `EFI/BOOT/BOOTX64.EFI` and `startup.nsh`
- **CPU Profiles Tested**: Intel Haswell, QEMU Virtual CPU
- **Telemetry Channel**: ISA debugcon / Serial port redirection to file
- **Verification Suites**:
  1. `OpenVintageBootApp.efi`: Bootloader initialization, hardware discovery, and module state verification.
  2. `OvSelfTestApp.efi`: 16 comprehensive architectural tests covering core subsystems and OVIR-GPU components.
- **Pass Rule**: Both apps return `EFI_SUCCESS` and output `ALL OPENVINTAGE ARCHITECTURAL TESTS PASSED!`.
