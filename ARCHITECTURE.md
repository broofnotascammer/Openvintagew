# OPENVINTAGE ARCHITECTURAL SPECIFICATION

```
================================================================================
OPENVINTAGE SYSTEMS ARCHITECTURE
Revision: 2.0.0
Classification: Systems Engineering Specification
Status: Active Implementation
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

The system is decomposed into three primary control layers and two translation hubs:

```
                           OPENVINTAGE
                                │
                ┌───────────────┼────────────────┐
                │               │                │
             OVCore         OVResolver       OVScheduler
                │               │                │
        ┌───────┴───────┐       │        ┌───────┴────────┐
        │               │       │        │                │
      OVIR-GPU      OVIR-CPU    │     CPU Resources   Memory/Performance
        │               │       │
        │               │       │
  Graphics APIs   CPU Architectures
   (Metal/VK/     (ARM/x86/x86-64/
    GL/DirectX)       RISC-V)
```

---

## 2. Core Subsystems

### 2.1 OVCore
- **Foundation Layer**: Provides memory allocation abstractions, platform-independent primitives, timestamping, ring buffers, and diagnostic logging.
- **Portability Contract**: Contains zero OS-specific system calls in public interfaces; implementations link against UEFI protocols, POSIX syscalls, or Win32 depending on compilation target.

### 2.2 OVResolver (Intelligent Decision Engine)
OVResolver does **not** perform translation itself; it inspects requests, hardware state, and cache indexes to determine the optimal execution pipeline:

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

**Decision Rule Sets:**
1. **Pass-through / Native**: If target GPU natively satisfies API version and features (e.g. OpenGL 4.1 on Intel Iris Pro 5200).
2. **Intermediate Translation**: When frontend API exceeds hardware capabilities but can be mapped via OVIR-GPU commands (e.g. Metal 1.2 compute/tessellation translated to GLSL 4.10 / ARB extensions).
3. **Feature Simplification**: When unsupported optional features (e.g., MSAA 8x, anisotropic 16x, 4K render targets) exceed legacy VRAM or fill-rate budget, resolver negotiates clamp parameters to preserve frame stability.
4. **Cache Re-use**: Look up pre-compiled shader and pipeline state binaries by composite hash (Source Hash + Driver Hash + Hardware ID).
5. **CPU Fallback**: Route unsupported compute or geometry shaders to vectorized CPU worker threads (LLVM/SIMD).

### 2.3 OVScheduler & Resource Manager
The scheduler manages thread pools, core affinity, and thermal throttling awareness without monopolizing CPU cores:

- **Core Topology Awareness**:
  - Distinguishes physical cores vs. hyperthreads (SMT).
  - Handles legacy heterogeneous configs (e.g., Mac Pro dual Xeon NUMA nodes).
- **Dynamic Performance Profiles**:
  - `Battery`: Low clock states, reduced background worker priorities, throttled shader pre-warming.
  - `Balanced`: Default operating state, cooperative OS scheduling, dynamic worker elasticity.
  - `Performance`: Elevated thread priorities, aggressive cache pre-fetching, unlocked thread pools.
  - `Maximum Performance`: Pinned compute worker affinity, bypassed frame throttles, priority VRAM buffers.
  - `Developer`: Full instrumentation, validation layers enabled, telemetry and timing capture.

### 2.4 OVIR-GPU (Graphics Intermediate Representation)
Represents graphics workloads as an immutable directed acyclic graph (DAG) of command nodes:
- **Command Streams**: Draw, Dispatch, Copy, Clear, Barrier, Present.
- **Render Passes**: Color/Depth/Stencil attachments, Load/Store actions, Resolve targets.
- **Resource Descriptors**: Buffers (Uniform, Storage, Vertex, Index), Textures (1D, 2D, 3D, Cube, Array, formats, mip levels), Samplers.
- **Pipelines**: Fixed-function state, blend states, rasterizer state, depth-stencil state, and shader stages.
- **Synchronization**: Memory barriers, pipeline execution barriers, and timeline semaphores.

### 2.5 OVIR-CPU (CPU Instruction IR - Design Specification)
For cross-architecture binary translation:
- Frontend decoders: ARM64 (AArch64), x86/x86-64, future RISC-V.
- IR Primitives: Register SSA form, integer arithmetic, IEEE 754 floating point, memory operations with endianness abstraction, control flow graphs (CFG).
- Backend codegen: Architecture-specific JIT with block translation caching.
*(Deferred to subsequent phase per engineering roadmap)*.

---

## 3. Hardware Abstraction Layer (HAL)

OpenVintage provides a unified hardware capability database specifically modeled around legacy Intel Mac hardware:

| Platform Family | Example Models | Typical CPU | GPU Configuration | Target Strategy |
| :--- | :--- | :--- | :--- | :--- |
| **Early Intel Mac** (2006-2009) | iMac 8,1 / MBP 4,1 | Core 2 Duo (Penryn) | Nvidia GeForce 8600M / 9400M | Legacy GL 2.1 / 3.3, feature simplification |
| **Nehalem / Westmere** (2009-2010) | Mac Pro 4,1 / 5,1 | Xeon W3520 / 5600 | ATI Radeon HD 4870 / 5770 | High CPU core count, GL 3.3 / 4.1 translation |
| **Sandy Bridge** (2011) | MBP 8,1 / Mac mini 5,1 | Core i5-2415M | Intel HD Graphics 3000 / AMD HD 6750M | Strict Metal fallback, GLSL 330 translation |
| **Ivy Bridge** (2012) | MBP 9,1 / 10,1 / iMac 13,1 | Core i7-3615QM | Intel HD 4000 / Nvidia GT 650M (Kepler) | GL 4.1 Native, Metal 1.0 IR translation |
| **Haswell / Crystalwell** (2013-2014)| MBP 11,2 / iMac 14,1 | Core i7-4770HQ | Intel Iris Pro 5200 / Nvidia GT 750M | Full GL 4.1, Compute shader emulation |
| **Broadwell / Skylake** (2015) | MBP 11,4 / 12,1 | Core i7-4870HQ / i5-5257U | Intel Iris 6100 / AMD Radeon R9 M370X | Modern Metal / Vulkan via MoltenVK backend |

---

## 4. Multi-Tier Caching Architecture

Caching is fundamental to prevent repeated translation latency on constrained hardware:

1. **Shader Cache**: Keyed by `SHA256(Source_Shader_Code + Compiler_Flags + Target_GPU_Vendor_ID + Driver_Version)`.
2. **Pipeline State Cache**: Serialized graphics pipeline state objects (PSO) avoiding driver re-compilation stalls.
3. **Texture Conversion Cache**: Pre-swizzled and compressed surface formats.
4. **Resolution Cache**: Memoized OVResolver decisions for fast-path query routing.
5. **Validation & Invalidation**: Hardware UUID and platform version checks prevent corrupted or stale driver caches.

---

## 5. Ecosystem Boundary: SolitaryOS

SolitaryOS is a dedicated lightweight Linux OS project designed for bare-metal application execution.
- **Rule**: SolitaryOS architecture must remain outside OpenVintage codebase until integration phase.
- **Interface**: OpenVintage provides standard C/POSIX dynamic library interfaces (`libopenvintage.so` / `openvintage.efi`) that SolitaryOS will consume as an unprivileged or supervisor runtime layer.
