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

## 6. OVIR-CPU (CPU Intermediate Representation & Binary Translation Engine)

```
       ARM Architecture                          x86 / x86-64 Architecture
              │                                              │
              ▼                                              ▼
   [ ARM64 Decoder Component ]                    [ x86-64 Decoder Component ]
   (OvirCpuDecoderArm64.c)                        (OvirCpuDecoderX64.c)
              │                                              │
              └──────────────────────┬───────────────────────┘
                                     │
                                     ▼
                        [ OVIR-CPU Architecture IR ]
                (Architecture-Neutral Representation & CFG)
                                     │
                                     ▼
                         [ Safe IR Optimizer Passes ]
               (Constant Folding, Redundant Move & DCE)
                                     │
                                     ▼
                         [ Target Backend Emission ]
                      ┌──────────────┴──────────────┐
                      ▼                             ▼
             [ x86-64 Backend ]              [ ARM64 Backend ]
           (OvirCpuBackendX64.c)           (OvirCpuBackendArm64.c)
                      │                             │
                      └──────────────┬──────────────┘
                                     │
                                     ▼
                     [ Translation Cache & Invalidation ]
                     (Metadata, Hash Verification, Versioning)
                                     │
                                     ▼
                     [ OvScheduler Workload Integration ]
                   (Dynamic Core Allocation, Priorities)
```

### 6.1 Decoupled Multi-Architecture Decoders (`OvirCpuDecoderLib`)
OpenVintage CPU translation strictly isolates instruction decoding into independent, modular architecture files:
- **ARM64 Decoder (`OvirCpuDecoderArm64.c`)**: Decodes fixed-width 32-bit AArch64 instructions:
  - ALU & Bitwise: `ADD`, `SUB`, `AND`, `ORR`, `EOR`, `MOV` (register/immediate)
  - Shifts & Extensions: `LSL`, `LSR`, `ASR`, `ROR`
  - Memory: Load (`LDR`) and Store (`STR`) with register and base+offset addressing
  - Branches: Unconditional (`B`, `BR`), Conditional (`B.cond`), Calls (`BL`, `BLR`), Returns (`RET`)
  - Floating-Point: `FADD`, `FSUB`, `FMUL`, `FDIV`, `FMOV`, `FCMP`
  - SIMD / Vector: Advanced SIMD integer and float vector instructions (`VADD`)
- **x86-64 Decoder (`OvirCpuDecoderX64.c`)**: Decodes variable-length x86-64 machine code:
  - REX prefixes (`REX.W`, `REX.R`, `REX.X`, `REX.B`) and operand size overrides (`0x66`)
  - ALU instructions: `ADD`, `SUB`, `AND`, `OR`, `XOR`, `CMP`, `TEST`
  - Control Flow: Short/near conditional jumps (`0x70-0x7F`, `0x0F 0x80-0x8F`), unconditional jumps (`0xEB`, `0xE9`), calls (`0xE8`), and returns (`0xC3`)
  - Mov & Load/Store: 32-bit and 64-bit immediate movs (`0xB8-0xBF`, `0xC7`), register-to-register moves (`0x89`, `0x8B`)
  - ModR/M and SIB byte decoding for memory operand address reconstruction.

### 6.2 Architecture-Neutral Intermediate Representation (`OvirCpuLib`)
- **Typed Operands**: Architecture-neutral representation for virtual registers (`OVIR_VREG`), immediate constants (`OVIR_IMM_OPERAND`), base+index+displacement memory references (`OVIR_MEM_OPERAND`), and block labels.
- **Explicit Operations**: Instructions record opcodes, bit widths (8, 16, 32, 64, 128), condition flags, and flag side effects (`SetFlags`).
- **Basic Blocks & CFG**: Instructions are grouped into atomic basic blocks terminated by branch, return, or trap instructions, ensuring predictable control flow graph construction.

### 6.3 Safe Optimizer Passes (`OvirCpuOptimizerLib`)
- **Correctness First**: Avoids unsafe speculative optimizations that violate memory models or side effects.
- **Constant Folding**: Evaluates constant arithmetic and bitwise expressions at translation time.
- **Redundant Move Elimination**: Strips identity moves (`MOV Rx, Rx`) that do not alter state or flags.
- **Dead Code Elimination**: Prunes unreachable instructions following unconditional terminator instructions (`RET`, `JMP`).

### 6.4 Target Machine Code Backends (`OvirCpuBackendLib`)
- **x86-64 Backend (`OvirCpuBackendX64.c`)**:
  - Lowers 3-address IR operands (`Dst = Src1 OP Src2`) to 2-address x86-64 native instructions (`Dst OP= Src2`).
  - Emits native x86-64 opcodes with REX prefixes for 64-bit operations.
  - Generates function returns (`RET` - `0xC3`) and function calls.
- **ARM64 Backend (`OvirCpuBackendArm64.c`)**:
  - Emits 32-bit fixed AArch64 machine instructions.
  - Directly maps 3-address IR instructions into native ARM64 register layouts.
  - Emits `RET` (`0xD65F03C0`) and branch targets.

### 6.5 Translation Cache & Invalidation (`OvirCpuCacheLib`)
- **Metadata Protection**: Translation cache entries record source architecture, target architecture, OpenVintage version, translator version, and configuration flags.
- **Hash Verification**: 64-bit hashing of guest code prevents collisions and identifies code changes.
- **Safe Invalidation**: When versions or hardware configuration flags change, incompatible cached translations are flushed and invalidated.

### 6.6 Workload Integration with OvScheduler (`OvirCpuSchedulerLib`)
- **Dynamic Task Coordination**: CPU translation tasks (`OvirCpuScheduleJitTask`, `OvirCpuScheduleTranslationTask`) are dispatched alongside application workloads.
- **Priority Tiering**: Interactive application tasks receive high priority (Priority 3) while background optimization passes run at idle/low priority (Priority 1).
- **Zero Permanent Pinning**: Adheres strictly to operating system scheduling mechanisms without hardcoding permanent core affinity masks (`CoreAffinityMask = 0`).

---

## 7. Phase 5: OpenVintage Integration & Optimization Architecture

### 7.1 Final Execution Pipeline
OpenVintage integrates all firmware, translation, scheduling, and caching components into a deterministic evaluation pipeline:

```
OpenVintage Boot/Firmware
        ↓
OpenVintage Core
        ↓
Hardware Detection (HAL)
        ↓
OVResolver (Integrated Engine)
        ↓
┌───────────────┬────────────────┐
│               │                │
OVIR-CPU       OVIR-GPU       OVScheduler
│               │                │
CPU Backend    GPU Backend    Resource Management
└───────────────┴────────────────┘
        ↓
Application / Game Workload
```

### 7.2 Integrated Resolver (`OvResolverLib`)
The integrated resolver evaluates multi-dimensional platform constraints:
- **Input Parameters**: Guest CPU architecture, target OS bitness, requested graphics API & version, compute shader requirement, max texture dimensions, RAM & VRAM requirements, and code hash.
- **Coordination Logic**: Synthesizes CPU translation necessity (e.g. ARM64 to x86-64), GPU translation necessity (e.g. Metal 2 to OpenGL 3.3 Core), feature clamping (e.g. 8192px textures clamped to 4096px on Intel HD 4000), compute fallback (software compute via CPU when hardware lacks compute shaders), and resource quotas.
- **Output Plan**: Generates concrete execution routes (`Native`, `Translated`, `Simplified`, `Fallback`) and assigns performance cost factors.

### 7.3 Performance Subsystem (`OvPerfSystemLib`)
- **Metric Collection**: Telemetry capture for CPU load %, active cores, tagged memory bytes, active allocation counts, VRAM usage, draw calls, vertex counts, translation overhead (microseconds), shader compilation duration, cache hit rates, and frame time (microseconds).
- **Snapshot Extraction**: Provides consistent performance state snapshots for real-time monitoring and scheduler feedback.

### 7.4 Dynamic Resource Management (`OvResourceManagerLib`)
- **Platform Capability Profiles**:
  - `Balanced`: Standard profile balancing responsiveness and power.
  - `Performance`: Unlocks higher core pools and memory quotas for intensive workloads.
  - `MaxPerformance`: Maximum thread pools and VRAM allocations; strictly gated by platform capability checks (requires multi-core hardware).
  - `BatteryLowPower`: Restricts background threads and tightens memory allocations for battery conservation.
- **Constraint Enforcement**: Prevents invalid configurations; rejects requests exceeding physical memory or core limits.

### 7.5 Unified Multi-Tier Cache (`OvUnifiedCacheLib`)
- **Cache Tiers**:
  1. `CpuTranslation`: JIT compiled machine code blocks.
  2. `Shader`: SPIR-V and GLSL compiled shader binaries.
  3. `Pipeline`: Pipeline State Objects (PSO) descriptors.
  4. `Compatibility`: Application resolution plans and silicon quirk evaluations.
- **Reliable Invalidation**:
  - Generation-tracked invalidation (`CurrentGeneration`).
  - Selective or global invalidation triggered by hardware configuration change, version upgrade, or memory pressure.
  - Integrity verification ensuring cache consistency before execution.

### 7.6 Compatibility Framework (`OvCompatibilityLib`)
- **Grounded Verification**: Records concrete application requirements and maps them to hardware limits without synthetic or exaggerated claims.
- **Quirk & Limitation Database**: Accounts for real silicon limitations (e.g., Intel Ivy Bridge Gen7 lacking Vulkan 1.2, requiring 4096px texture clamping and compute shader software fallback).

### 7.7 Unified Diagnostics (`OvDiagnosticsLib`)
- Comprehensive diagnostic reports auditing firmware revision, CPU model and instruction sets, GPU capabilities and VRAM, tagged memory allocations and leaks, supported graphics APIs, translation layer availability, cache generations, and active resolver policies.

### 7.8 Reproducible Empirical Benchmarking (`OvBenchmarkLib`)
- Uses physical hardware Time Stamp Counter (`AsmReadTsc`) to measure real cycle counts.
- Evaluates baseline vs OpenVintage optimized paths across:
  1. Arithmetic constant folding.
  2. Dead code & redundant move elimination.
  3. Translation cache lookup vs cold compilation.
  4. End-to-end JIT pipeline compilation.

---

## 8. QEMU Virtualized Test Architecture

Testing is executed in an automated, headless virtual machine environment:
- **Host Test Harness**: `scripts/test_qemu.sh`
- **Firmware Base**: Tianocore OVMF X64 (`/usr/share/ovmf/OVMF.fd`)
- **Virtual Disk**: 64MB FAT32 ESP disk containing `EFI/BOOT/BOOTX64.EFI` and `startup.nsh`
- **CPU Profiles Tested**: Intel Haswell, QEMU Virtual CPU
- **Telemetry Channel**: ISA debugcon / Serial port redirection to file
- **Verification Suites**:
  1. `OpenVintageBootApp.efi`: Bootloader initialization, hardware discovery, and module state verification.
  2. `OvSelfTestApp.efi`: 32 comprehensive architectural tests covering core subsystems, OVIR-GPU components, OVIR-CPU subsystems, and Phase 5 integrated features.
- **Pass Rule**: Both apps return `EFI_SUCCESS` and output `ALL OPENVINTAGE ARCHITECTURAL TESTS PASSED!`.

