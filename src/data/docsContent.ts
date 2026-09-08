/**
 * Synchronized copy of project specifications for in-app viewing.
 * Auto-synced from workspace markdown documentation.
 */

export const SYSTEM_STATUS_TEXT = `# OpenVintage System Status

\`\`\`
================================================================================
OPENVINTAGE SYSTEM STATUS & ENGINEERING MILESTONES
Platform: OpenVintage Ecosystem
Release: v0.5.0-Phase-5 (Full System Integration & Optimization)
Status: Active & Verified
================================================================================
\`\`\`

## 1. Current Phase: Phase 5 (Integration and Optimization)
**Status: COMPLETE & VERIFIED**

Phase 5 achieves full architectural integration across all OpenVintage subsystems:
- **Boot/Firmware & Core**: Unified initialization, memory tagging, module lifecycle, and hardware discovery.
- **Hardware Detection (HAL)**: Silicon-level CPUID, PCI scanning, GPU capability detection, and memory topology mapping.
- **Integrated OVResolver**: Central coordinator synthesizing CPU architecture, GPU architecture, API requirements, thermal state, RAM/VRAM constraints, and cache hits.
- **OVIR-CPU & OVIR-GPU Hubs**: Architecture-neutral IR pipelines, multi-architecture decoders, safe non-speculative optimizers, machine code generators, and graphics API adapters.
- **OVScheduler & Resource Manager**: Profile-based execution (Balanced, Performance, MaxPerformance, BatteryLowPower) with dynamic thread allocation and hard platform capability validation.
- **Unified Multi-Tier Cache**: Coordinated CPU translation cache, GPU shader cache, pipeline cache, and compatibility cache with generation-tracked reliable invalidation.
- **Compatibility Framework**: Comprehensive database of application requirements, hardware constraints, silicon quirks, and non-fabricated execution plans.
- **Unified Diagnostics**: End-to-end system auditing reporting CPU, GPU, memory, APIs, cache states, resolver decisions, and silicon limitations.
- **Reproducible Empirical Benchmarking**: Real hardware TSC cycle timing measuring unoptimized baseline vs OpenVintage optimized execution paths.

---

## 2. Integrated Subsystem Verification Matrix

| Subsystem | Source Component | EDK II INF | Architectural Tests | Status |
| :--- | :--- | :--- | :--- | :--- |
| **Boot Application** | \`OpenVintageBootApp\` | \`OpenVintageBootApp.inf\` | Bootloader Signatures | **VERIFIED** |
| **HAL DXE Driver** | \`Drivers/OpenVintageHalDxe\` | \`OpenVintageHalDxe.inf\` | Protocol Installation | **VERIFIED** |
| **Core Runtime & Lifecycle** | \`Core/OvCore.c\` | \`OvCoreLib.inf\` | Lifecycle State Machine | **VERIFIED** |
| **Configuration Profiles** | \`Core/OvConfig.c\` | \`OvConfigLib.inf\` | Dynamic Profiles & PCDs | **VERIFIED** |
| **Tagged Memory Allocator** | \`Memory/OvMemory.c\` | \`OvMemoryLib.inf\` | Zero-Leak Audits | **VERIFIED** |
| **Hardware Discovery** | \`Hardware/OvHardware.c\` | \`OvHardwareLib.inf\` | CPUID & PCI Topology | **VERIFIED** |
| **Dynamic Module Manager** | \`Core/OvModule.c\` | \`OvModuleLib.inf\` | Priority-Ordered Inits | **VERIFIED** |
| **Integrated OVResolver** | \`Resolver/OvResolver.c\` | \`OvResolverLib.inf\` | Multi-Arch & GPU Routing | **VERIFIED** |
| **Cooperative Scheduler** | \`Scheduler/OvScheduler.c\`| \`OvSchedulerLib.inf\` | Priority Queue & Affinity | **VERIFIED** |
| **Resource Manager** | \`Scheduler/OvResourceManager.c\` | \`OvResourceManagerLib.inf\` | Profiles & Quotas | **VERIFIED** |
| **OVIR-GPU Core IR** | \`OvirGpu/OvirGpu.c\` | \`OvirGpuLib.inf\` | Stream Validation | **VERIFIED** |
| **GPU Capabilities** | \`OvirGpu/OvGpuCapability.c\` | \`OvGpuCapabilityLib.inf\` | Silicon Tier Reporting | **VERIFIED** |
| **Graphics API Adapters** | \`OvirGpu/OvirAdapters.c\` | \`OvirAdaptersLib.inf\` | Metal, VK, GL, DX Adapters | **VERIFIED** |
| **Shader Cache & Ingestion**| \`OvirGpu/OvirShader.c\` | \`OvirShaderLib.inf\` | FNV-1a Hashing & Cache | **VERIFIED** |
| **Pipeline State Cache** | \`OvirGpu/OvirPipeline.c\` | \`OvirPipelineLib.inf\` | PSO Hashing & Invalidation | **VERIFIED** |
| **VRAM Resource Manager** | \`OvirGpu/OvirResource.c\` | \`OvirResourceLib.inf\` | Buffers & Texture Pool | **VERIFIED** |
| **GPU Perf Telemetry** | \`OvirGpu/OvirPerf.c\` | \`OvirPerfLib.inf\` | TSC Timestamp Timing | **VERIFIED** |
| **OVIR-CPU Core IR** | \`OvirCpu/OvirCpu.c\` | \`OvirCpuLib.inf\` | 3-Address Code Validation | **VERIFIED** |
| **Multi-Arch Decoders** | \`OvirCpu/OvirCpuDecoder*\` | \`OvirCpuDecoderLib.inf\` | ARM64 & x86-64 Decoders | **VERIFIED** |
| **Safe IR Optimizer** | \`OvirCpu/OvirCpuOptimizer.c\` | \`OvirCpuOptimizerLib.inf\` | Constant Fold & DCE | **VERIFIED** |
| **Machine Code Backends** | \`OvirCpu/OvirCpuBackend*\` | \`OvirCpuBackendLib.inf\` | Native x86-64 / ARM64 Emit | **VERIFIED** |
| **CPU Translation Cache** | \`OvirCpu/OvirCpuCache.c\` | \`OvirCpuCacheLib.inf\` | Block Verification & Inval | **VERIFIED** |
| **Dynamic JIT Pipeline** | \`OvirCpu/OvirCpuJit.c\` | \`OvirCpuJitLib.inf\` | Live Native JIT Execution | **VERIFIED** |
| **CPU Scheduler Bridge** | \`OvirCpu/OvirCpuScheduler.c\` | \`OvirCpuSchedulerLib.inf\` | Dynamic Core Allocation | **VERIFIED** |
| **Performance Subsystem** | \`Core/OvPerfSystem.c\` | \`OvPerfSystemLib.inf\` | Telemetry & Snapshots | **VERIFIED** |
| **Unified Multi-Tier Cache**| \`Core/OvUnifiedCache.c\` | \`OvUnifiedCacheLib.inf\` | Invalidation & Generation | **VERIFIED** |
| **Compatibility Framework** | \`Core/OvCompatibility.c\` | \`OvCompatibilityLib.inf\` | Requirements & Quirks | **VERIFIED** |
| **Unified Diagnostics** | \`Core/OvDiagnostics.c\` | \`OvDiagnosticsLib.inf\` | System Audit Reporting | **VERIFIED** |
| **Empirical Benchmarking** | \`Core/OvBenchmark.c\` | \`OvBenchmarkLib.inf\` | TSC Baseline vs Optimized | **VERIFIED** |

---

## 3. Full Architectural Self-Test Suite Status
- **Test Application**: \`bin/OvSelfTestApp.efi\`
- **Total Architectural Tests**: 32
- **Pass Rate**: 100% (32 / 32 Passed)
- **Memory Safety**: Zero memory leaks detected across all allocations.
- **Failures**: 0
`;

export const PROJECT_STATUS_TEXT = `# OpenVintage Project Status

\`\`\`
================================================================================
PROJECT STATUS & ENGINEERING MILESTONES
Platform: OpenVintage Ecosystem
Last Updated: 2026-09-08
Firmware Release: v0.5.0-Phase-5 (Full System Integration & Optimization)
================================================================================
\`\`\`

## Current Phase
**Phase 5 COMPLETE & VERIFIED — OpenVintage Full Ecosystem Integration and Optimization**

---

## Deliverables & Component Verification

| Component | Repository Path | Build Status | Verification Method |
| :--- | :--- | :--- | :--- |
| **EDK II Package Declaration** | \`OpenVintagePkg/OpenVintagePkg.dec\` | Verified | EDK II Parser (\`build.py\`) |
| **Platform Description (DSC)** | \`OpenVintagePkg/OpenVintagePkg.dsc\` | Verified | EDK II Compilation (GCC / X64) |
| **Flash Definition File (FDF)** | \`OpenVintagePkg/OpenVintagePkg.fdf\` | Verified | \`GenFds\` Flash Synthesis |
| **OvCore Subsystem** | \`OpenVintagePkg/Core/OvCoreLib.*\` | Built | Core Orchestrator & State Machine |
| **OvConfig Subsystem** | \`OpenVintagePkg/Core/OvConfigLib.*\` | Built | Dynamic Hardware Profiles & Flags |
| **OvMemory Subsystem** | \`OpenVintagePkg/Memory/OvMemoryLib.*\` | Built | Tagged Allocation & Leak Verification |
| **OvHardware Subsystem** | \`OpenVintagePkg/Hardware/OvHardwareLib.*\`| Built | Unified CPU, RAM, GPU, PCI Topology |
| **OvModule Subsystem** | \`OpenVintagePkg/Core/OvModuleLib.*\` | Built | Module Lifecycle & Registration |
| **OvResolver Subsystem** | \`OpenVintagePkg/Resolver/OvResolverLib.*\`| Built | Integrated Routing Decision Matrix |
| **OvScheduler Subsystem** | \`OpenVintagePkg/Scheduler/OvSchedulerLib.*\`| Built | Priority Queue & Task Dispatch |
| **OvResourceManager Subsystem** | \`OpenVintagePkg/Scheduler/OvResourceManagerLib.*\`| Built | Adaptive Profiles & Quota Enforcement |
| **OvUnifiedCache Subsystem** | \`OpenVintagePkg/Core/OvUnifiedCacheLib.*\`| Built | Multi-Tier Integration & Invalidation |
| **OvCompatibility Subsystem** | \`OpenVintagePkg/Core/OvCompatibilityLib.*\`| Built | App Requirements & Silicon Quirks |
| **OvDiagnostics Subsystem** | \`OpenVintagePkg/Core/OvDiagnosticsLib.*\` | Built | End-to-End System Audit Reports |
| **OvBenchmark Subsystem** | \`OpenVintagePkg/Core/OvBenchmarkLib.*\` | Built | Real TSC Timing & Empirical Speedup |
| **OvPerfSystem Subsystem** | \`OpenVintagePkg/Core/OvPerfSystemLib.*\` | Built | Telemetry & Performance Snapshots |
| **OVIR-GPU Core Command IR** | \`OpenVintagePkg/OvirGpu/OvirGpuLib.*\` | Built | IR Command Recording & Validation |
| **OVIR GPU Capability Detector** | \`OpenVintagePkg/OvirGpu/OvGpuCapabilityLib.*\` | Built | Silicon Vendor & Format Capabilities |
| **OVIR Graphics API Adapters** | \`OpenVintagePkg/OvirGpu/OvirAdaptersLib.*\` | Built | Metal, VK, GL, DX Adapters |
| **OVIR Shader Ingestion & Cache** | \`OpenVintagePkg/OvirGpu/OvirShaderLib.*\` | Built | SPIR-V Ingestion, FNV-1a Hash, Cache |
| **OVIR Pipeline State (PSO) Cache** | \`OpenVintagePkg/OvirGpu/OvirPipelineLib.*\` | Built | Pipeline Creation, Hashing & Cache |
| **OVIR Resource & VRAM Manager** | \`OpenVintagePkg/OvirGpu/OvirResourceLib.*\` | Built | Buffers, Textures, Samplers & VRAM Pool |
| **OVIR Performance Telemetry** | \`OpenVintagePkg/OvirGpu/OvirPerfLib.*\` | Built | TSC Timing, Cache Counters, Metrics |
| **OVIR Graphics Resolver Bridge** | \`OpenVintagePkg/OvirGpu/OvirResolverBridgeLib.*\`| Built | Native, Translated, Simplified, Fallback |
| **OVIR-CPU Core IR Subsystem** | \`OpenVintagePkg/OvirCpu/OvirCpuLib.*\` | Built | Architecture-Neutral IR & Validation |
| **OVIR-CPU Multi-Arch Decoders** | \`OpenVintagePkg/OvirCpu/OvirCpuDecoderLib.*\`| Built | Isolated ARM64 & x86-64 Decoders |
| **OVIR-CPU Safe Optimizer** | \`OpenVintagePkg/OvirCpu/OvirCpuOptimizerLib.*\`| Built | Constant Folding, Move Elimination, DCE |
| **OVIR-CPU Multi-Arch Backends** | \`OpenVintagePkg/OvirCpu/OvirCpuBackendLib.*\`| Built | x86-64 & ARM64 Code Emission |
| **OVIR-CPU Translation Cache** | \`OpenVintagePkg/OvirCpu/OvirCpuCacheLib.*\` | Built | Fast Metadata Validation & Cache Flush |
| **OVIR-CPU JIT Compilation Pipeline**| \`OpenVintagePkg/OvirCpu/OvirCpuJitLib.*\` | Built | Dynamic Block Translation & Execution |
| **OVIR-CPU Scheduler Integration** | \`OpenVintagePkg/OvirCpu/OvirCpuSchedulerLib.*\`| Built | Dynamic Core Scheduling & Task Priority |
| **HAL DXE Driver** | \`OpenVintagePkg/Drivers/OpenVintageHalDxe/\` | Built | \`OpenVintageHalDxe.efi\` (~8.2 KB) |
| **Boot Application Binary** | \`bin/OpenVintageBootApp.efi\` | Built | PE32+ x86-64 Executable (~48 KB) |
| **Self-Test Diagnostic Suite**| \`bin/OvSelfTestApp.efi\` | Built | PE32+ x86-64 Executable (~380 KB) |
| **Flash Device Image** | \`bin/OPENVINTAGE.fd\` | Generated | 4.0 MB Flash ROM Image |
| **Firmware Volume** | \`OpenVintagePkg/Firmware/OPENVINTAGE_DXEFV.Fv\`| Generated | 4.0 MB PI Firmware Volume |
| **Automated Build Script** | \`scripts/build_firmware.sh\` | Operational | Builds and populates all binaries |
| **QEMU Test Harness** | \`scripts/test_qemu.sh\` | Operational | Headless Dual UEFI Boot & 32 Tests (PASS) |

---

## Completed Milestones (Phase 5: Full Ecosystem Integration)
- [x] **Integrated Resolver Engine (\`OvResolverLib\`)**:
  - Unifies multi-architecture CPU evaluation with GPU backend routing.
  - Complete execution planning (Native vs Translated CPU/GPU, direct vs translated API, clamped features, resource quotas).
- [x] **Measurable Performance System (\`OvPerfSystemLib\`)**:
  - Telemetry capture for CPU load, memory usage, GPU memory, translation overhead, shader compilation time, cache hit rate, and frame timing.
- [x] **Dynamic Resource Management (\`OvResourceManagerLib\`)**:
  - Platform profiles (Balanced, Performance, MaxPerformance, BatteryLowPower) strictly validated against hardware capabilities.
- [x] **Unified Multi-Tier Cache (\`OvUnifiedCacheLib\`)**:
  - Coordinated CPU translation cache, GPU shader cache, pipeline cache, and compatibility cache with generation-tracked invalidation.
- [x] **Compatibility Framework (\`OvCompatibilityLib\`)**:
  - Empirical requirement matching and quirk handling without fabricated results.
- [x] **Unified Diagnostics (\`OvDiagnosticsLib\`)**:
  - Comprehensive system auditing reporting hardware, CPU, GPU, memory, APIs, caches, and limitations.
- [x] **Reproducible Empirical Benchmarking (\`OvBenchmarkLib\`)**:
  - Real TSC measurements proving speedup of constant folding, dead code elimination, and translation cache lookup.
- [x] **Automated Self-Test Expansion**:
  - Expanded from 24 to 32 tests covering all Phase 5 integrated features.
`;

export const ARCHITECTURE_TEXT = `# OPENVINTAGE ARCHITECTURAL SPECIFICATION

\`\`\`
================================================================================
OPENVINTAGE SYSTEMS ARCHITECTURE
Revision: 2.1.0
Classification: Systems Engineering Specification
Status: Active Implementation & Verified EDK II Firmware Pipeline
================================================================================
\`\`\`

---

## 1. Architectural Overview

OpenVintage rejects the traditional $M \\times N$ matrix of direct API-to-API bridges:

\`\`\`
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
\`\`\`

The system is decomposed into three primary control layers, a firmware platform foundation, and two translation hubs:

\`\`\`
                           OPENVINTAGE ECOSYSTEM
                                     │
              ┌──────────────────────┼──────────────────────┐
              │                      │                      │
        OpenVintagePkg            OVResolver            OVScheduler
      (Firmware & UEFI)      (Intelligent Routing)  (Resource Manager)
              │                      │                      │
        ┌──────┴──────┐        ┌──────┴──────┐        ┌──────┴──────┐
        │             │        │             │        │             │
    Firmware      Bootloader  OVIR-GPU   OVIR-CPU   CPU Cores    Thermal /
  (OPENVINTAGE.fd) (.EFI App) (Graphics)   (CPU)     Affinity    RAM Budgets
\`\`\`

---

## 2. Firmware & Bootloader Subsystems (\`OpenVintagePkg\`)

### 2.1 EDK II Package Structure
\`OpenVintagePkg\` is fully integrated into the Tianocore EDK II build environment.

### 2.2 OpenVintage Protocols
- \`OPEN_VINTAGE_HAL_PROTOCOL\` (\`gOpenVintageHalProtocolGuid\`):
  Installed during DXE phase by \`OpenVintageHalDxe\`. Exposes CPU and GPU hardware abstraction calls.
- \`OPEN_VINTAGE_PLATFORM_PROTOCOL\` (\`gOpenVintagePlatformProtocolGuid\`):
  Exposes platform revision, vendor information, and firmware health status.

---

## 3. Workload Resolution & Scheduling

OVResolver evaluates multi-dimensional platform constraints to formulate an optimal execution plan:
1. **Pass-through / Native**: Workload matches target hardware natively.
2. **Intermediate Translation**: Features are transformed through canonical IR command DAGs.
3. **Feature Simplification**: High-overhead features (e.g. 8K textures) clamped to legacy limits.
4. **Cache Re-use**: Lookups by composite FNV-1a / generation hash.
5. **CPU Fallback**: Route unsupported features (e.g. compute shaders) to SIMD CPU threads.

---

## 4. OVIR-GPU Graphics Intermediate Representation
- **Hub-and-Spoke**: Decouples API ingestion from execution backends.
- **Adapters**: Metal 2/3, Vulkan, OpenGL Core, DirectX 11/12.
- **Shader & PSO Caches**: FNV-1a 64-bit hashing with fast pointer resolution.

---

## 5. OVIR-CPU Binary Translation Engine
- **Decoders**: Isolated ARM64 and x86-64 machine instruction decoders.
- **Safe Optimizer**: Constant folding, redundant move elimination, and dead code elimination.
- **Backends**: Native x86-64 and ARM64 machine code generation.
- **Dynamic JIT Pipeline**: Decode -> Optimize -> Emit -> Cache with live native execution.

---

## 6. Phase 5: OpenVintage Integration & Optimization Architecture

### 6.1 Final Execution Pipeline
\`\`\`
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
\`\`\`

### 6.2 Integrated Resolver (\`OvResolverLib\`)
Synthesizes CPU translation, GPU translation, memory limits, VRAM, and thermal conditions into a unified plan.

### 6.3 Performance Subsystem (\`OvPerfSystemLib\`)
Real-time telemetry tracking CPU load, memory, VRAM, translation overhead, shader compilation time, and frame timing.

### 6.4 Dynamic Resource Management (\`OvResourceManagerLib\`)
Platform-aware profiles (\`Balanced\`, \`Performance\`, \`MaxPerformance\`, \`BatteryLowPower\`) with capability gating.

### 6.5 Unified Multi-Tier Cache (\`OvUnifiedCacheLib\`)
Multi-tier cache (CPU, Shader, Pipeline, Compatibility) with generational invalidation.

### 6.6 Compatibility Framework (\`OvCompatibilityLib\`)
Grounded application profile registry and silicon quirk handling without fabricated claims.

### 6.7 Unified Diagnostics (\`OvDiagnosticsLib\`)
Comprehensive diagnostic audit reporting firmware, CPU, GPU, memory, APIs, caches, and limitations.

### 6.8 Reproducible Benchmarking (\`OvBenchmarkLib\`)
Physical TSC cycle timing measuring speedup of constant folding, DCE, and cache hits.
`;

export const README_TEXT = `# OPENVINTAGE

> **Modular compatibility and performance platform originally designed around legacy Intel Mac hardware.**

---

## 1. Executive Summary

**OpenVintage** is an experimental systems ecosystem engineered to extend the functional lifespan and performance envelope of legacy hardware, with primary initial focus on **legacy Intel Mac systems** (2006–2015 era architectures: Core 2 Duo, Nehalem, Sandy Bridge, Ivy Bridge, Haswell, Broadwell, Skylake) and designed with portability across bare-metal firmware, UEFI, Linux, macOS, and Windows.

OpenVintage provides:
1. Native X64 UEFI Bootloader and Diagnostic Application (\`OpenVintageBootApp.efi\`).
2. Extensible Hardware Abstraction Layer (HAL) DXE Driver (\`OpenVintageHalDxe.efi\`).
3. Core platform hardware detection and telemetry libraries.
4. Full flash device image (\`OPENVINTAGE.fd\`) and firmware volume (\`OPENVINTAGE_DXEFV.Fv\`).
5. OVIR-GPU graphics translation and adapter hub.
6. OVIR-CPU multi-architecture binary translation and JIT compilation engine.
7. Integrated OVResolver and OVScheduler resource manager.
8. Unified multi-tier cache, compatibility framework, diagnostics, and benchmark suite.

---

## 2. Phase 5 Final Architecture

\`\`\`
                    FINAL ARCHITECTURE EXECUTION PIPELINE

                          OpenVintage Boot/Firmware
                                     ↓
                             OpenVintage Core
                                     ↓
                            Hardware Detection
                                     ↓
                                OVResolver
                                     ↓
                  ┌──────────────────┼──────────────────┐
                  │                  │                  │
               OVIR-CPU           OVIR-GPU         OVScheduler
                  │                  │                  │
             CPU Backend        GPU Backend     Resource Management
                  └──────────────────┼──────────────────┘
                                     ↓
                              Application/Game
\`\`\`

---

## 3. Verified Binaries & Firmware Deliverables

| Artifact | Type | Size | Description |
| :--- | :--- | :--- | :--- |
| \`bin/OpenVintageBootApp.efi\` | PE32+ x86-64 EFI App | ~48 KB | Native X64 UEFI boot application entry point |
| \`bin/OvSelfTestApp.efi\` | PE32+ x86-64 EFI App | ~380 KB | 32-subsystem architectural self-test diagnostic suite |
| \`bin/OPENVINTAGE.fd\` | Binary Flash Image | 4.0 MB | Complete SPI Flash device image |
| \`OpenVintagePkg/Firmware/OPENVINTAGE_DXEFV.Fv\` | PI Firmware Volume | 4.0 MB | DXE Firmware Volume containing HAL and BootApp |
| \`OpenVintagePkg/Drivers/OpenVintageHalDxe/OpenVintageHalDxe.efi\` | PE32+ x86-64 DXE Driver | ~8.2 KB | Hardware abstraction layer boot services driver |
`;

export const CHANGELOG_TEXT = `# OpenVintage Changelog

All notable changes to the OpenVintage project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to Semantic Versioning.

---

## [0.5.0] - 2026-09-08
### Added
- **Phase 5 Full Ecosystem Integration & Optimization**:
  - **Integrated Workload Resolver (\`OvResolverLib\`)**: Combines CPU and GPU requirements, memory constraints, VRAM allocation, and thermal limitations into a unified execution plan.
  - **Measurable Performance System (\`OvPerfSystemLib\`)**: Real-time telemetry tracking CPU load, active cores, system RAM, GPU memory, translation overhead, shader compilation time, cache hit rate, and frame timing.
  - **Dynamic Resource Management (\`OvResourceManagerLib\`)**: Platform-aware resource control supporting \`Balanced\`, \`Performance\`, \`MaxPerformance\`, and \`BatteryLowPower\` profiles with capability gating.
  - **Unified Multi-Tier Cache (\`OvUnifiedCacheLib\`)**: Coordinated CPU translation cache, GPU shader cache, pipeline cache, and compatibility cache with generational invalidation.
  - **Grounded Compatibility Framework (\`OvCompatibilityLib\`)**: Empirical application profile registry and hardware constraint matching without fabricated results.
  - **Unified Diagnostics (\`OvDiagnosticsLib\`)**: Comprehensive system diagnostic report auditing hardware, CPU, GPU, memory, APIs, caches, and silicon limitations.
  - **Reproducible Empirical Benchmarking (\`OvBenchmarkLib\`)**: Real hardware TSC cycle timing measuring ALU constant folding, dead code elimination, and translation cache retrieval.
- **Architectural Self-Test Suite Expansion (\`OvSelfTestApp.efi\`)**:
  - Expanded test suite from 24 to 32 automated tests covering all Phase 5 subsystems.
  - 100% tests passing cleanly under QEMU emulation with zero memory leaks.

## [0.4.0] - 2026-09-07
### Added
- **OVIR-CPU (CPU Architecture Translation Framework & Intermediate Representation)**:
  - Architecture-neutral IR, ARM64 & x86-64 decoders, safe optimizer, target backends, translation cache, dynamic JIT execution.
  - Tests 17-24 added to \`OvSelfTestApp.efi\`.

## [0.3.0] - 2026-09-07
### Added
- **OVIR-GPU (Graphics Intermediate Representation)**:
  - Universal Hub-and-Spoke graphics IR, Metal/Vulkan/OpenGL/DirectX adapters, shader and PSO caches, VRAM pool.
  - Tests 9-16 added to \`OvSelfTestApp.efi\`.

## [0.2.0] - 2026-09-06
### Added
- **Native EDK II Platform Package (\`OpenVintagePkg\`)**:
  - \`OpenVintageBootApp.efi\`, \`OpenVintageHalDxe.efi\`, \`OPENVINTAGE.fd\`, \`OPENVINTAGE_DXEFV.Fv\`.
  - Core subsystems (Memory, Hardware, Module, Config, Resolver, Scheduler).
  - Tests 1-8 added to \`OvSelfTestApp.efi\`.
`;
