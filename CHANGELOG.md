# OpenVintage Changelog

All notable changes to the OpenVintage project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to Semantic Versioning.

---

## [0.4.0] - 2026-09-07
### Added
- **OVIR-CPU (CPU Architecture Translation Framework & Intermediate Representation)**:
  - Designed and implemented a modular CPU translation framework connecting guest architectures to target execution hardware via an architecture-neutral Intermediate Representation (`OVIR-CPU`).
  - `OvirCpuLib`: Canonical, strongly typed IR representation for virtual registers (`OVIR_VREG`), immediate values (`OVIR_IMM_OPERAND`), base/index/displacement memory references (`OVIR_MEM_OPERAND`), arithmetic, logical, floating-point, SIMD, branch, call, and return operations, complete with basic block and program structure validation.
  - `OvirCpuDecoderLib`: Isolated, architecture-specific decoders:
    - `OvirCpuDecoderArm64.c`: Decodes 32-bit fixed AArch64 machine instructions (ALU, shifts, load/store, conditional branches, FP, and SIMD vector operations).
    - `OvirCpuDecoderX64.c`: Decodes variable-length x86-64 instructions with REX prefixes, operand-size overrides, SIB/ModR/M bytes, and jump/call/return control flow.
  - `OvirCpuOptimizerLib`: Safe, non-speculative optimization passes prioritizing correctness:
    - Constant folding for compile-time arithmetic simplification.
    - Redundant identity move elimination (`MOV Rx, Rx`).
    - Dead code elimination for unreachable instructions following terminators (`RET`, `JMP`).
  - `OvirCpuBackendLib`: Native target machine code emission:
    - `OvirCpuBackendX64.c`: Emits native x86-64 machine instructions, lowering 3-address IR to 2-address instructions.
    - `OvirCpuBackendArm64.c`: Emits native 32-bit AArch64 machine instructions.
  - `OvirCpuCacheLib`: High-performance translation caching with strict metadata validation (Source/Target Arch, OpenVintage version, translator version, configuration flags), code hash checking, and automatic invalidation of stale entries.
  - `OvirCpuJitLib`: Dynamic JIT compilation pipeline (Decode -> Optimize -> Emit -> Cache) and live native execution engine.
  - `OvirCpuSchedulerLib`: Cooperative integration with `OvScheduler` allocating CPU translation tasks dynamically without permanent core pinning, while prioritizing interactive application responsiveness.
- **Architectural Self-Test Suite Expansion (`OvSelfTestApp.efi`)**:
  - Expanded test suite from 16 to 24 automated tests covering all Phase 4 CPU subsystems.
  - Added Test 17: Multi-Architecture Instruction Decoding (ARM64 & x86-64).
  - Added Test 18: IR Generation, Basic Blocks & Program Structure Validation.
  - Added Test 19: Safe Optimizer (Constant Folding, Identity & Dead Code Elimination).
  - Added Test 20: Target Machine Code Emission (x86-64 and ARM64 Backends).
  - Added Test 21: Translation Cache (Hash Verification, Hit/Miss Tracking).
  - Added Test 22: Cache Invalidation (Version, Translator & Configuration Safeguards).
  - Added Test 23: Dynamic JIT Pipeline & Native Execution Correctness (100+42=142).
  - Added Test 24: Workload Scheduling Integration (Dynamic Core Allocation & Priorities).
- **QEMU Verification**:
  - 100% of all 24 architectural tests passing cleanly under QEMU emulation with zero memory leaks.

## [0.3.0] - 2026-09-07
### Added
- **OVIR-GPU (Graphics Intermediate Representation)**:
  - Implemented the hub-and-spoke Graphics Intermediate Representation (`OVIR-GPU`) decoupling graphics APIs from hardware execution backends.
  - `OvirGpuLib`: Command recording engine, stream validation rules, and render pass lifecycle.
  - `OvGpuCapabilityLib`: Silicon vendor detection (Intel, Nvidia, AMD), API tier reporting, and texture format support matrix.
  - `OvirAdaptersLib`: Unified adapters for Metal 2/3, Vulkan, OpenGL Core, and DirectX 11/12 translating API states into canonical OVIR-GPU streams.
  - `OvirShaderLib`: SPIR-V bytecode ingestion, FNV-1a 64-bit hashing, reflection binding parser, and zero-redundancy bytecode cache.
  - `OvirPipelineLib`: Full pipeline state object descriptor hashing, state management, and high-performance PSO cache.
  - `OvirResourceLib`: VRAM tracking pool, buffer allocation, multi-dimensional texture descriptions, and sampler definitions.
  - `OvirPerfLib`: Hardware TSC timestamp profiling, compile duration metrics, and cache hit/miss ratio calculation.
  - `OvirResolverBridgeLib`: Workload routing bridge resolving OVIR-GPU streams to Native, Translated, Simplified, or Fallback paths.
- **Architectural Self-Test Suite Expansion (`OvSelfTestApp.efi`)**:
  - Expanded test suite from 8 to 16 automated tests covering all Phase 2 and Phase 3 subsystems.
  - Added Test 9: GPU Capability Probing & Format Validation.
  - Added Test 10: OVIR-GPU Command Recording & Stream Validation.
  - Added Test 11: Graphics API Adapters (Metal, Vulkan, OpenGL, DirectX).
  - Added Test 12: Shader Ingestion, FNV-1a Hashing & Bytecode Cache.
  - Added Test 13: Pipeline State Object Hashing & PSO Cache.
  - Added Test 14: Resource System (Buffers, Textures, Samplers & VRAM Tracking).
  - Added Test 15: Graphics Resolver Routing Decisions.
  - Added Test 16: Performance Telemetry (TSC Clock Timing & Cache Metrics).
- **QEMU Dual-Run Verification**:
  - Updated `scripts/test_qemu.sh` to run both `OpenVintageBootApp.efi` and `OvSelfTestApp.efi` sequentially in headless QEMU.
  - 100% test pass verified with zero memory leaks.

## [0.2.0] - 2026-09-06
### Added
- **Native EDK II Platform Package (`OpenVintagePkg`)**:
  - `OpenVintagePkg.dec`: Package declaration with custom protocol GUIDs and token space PCDs.
  - `OpenVintagePkg.dsc`: Platform description configuring library mappings, PCDs, and component compilation targets for X64 under GCC5.
  - `OpenVintagePkg.fdf`: Flash definition file establishing a 4MB SPI Flash device image (`OPENVINTAGE.fd`) and DXE Firmware Volume (`OPENVINTAGE_DXEFV.Fv`).
- **OpenVintage UEFI Boot Application (`OpenVintageBootApp.efi`)**:
  - Entry point `UefiMain` written in standard C conforming to UEFI 2.70 specification.
  - OpenVintage diagnostic startup banner and runtime initialization.
  - CPUID instruction decoder detecting silicon architectural profiles (Core 2, Nehalem/Westmere, Sandy Bridge, Ivy Bridge, Haswell/Broadwell) and SIMD features (SSE4.1, SSE4.2, AVX, AVX2, AES-NI).
  - UEFI memory map inspection calculating total physical RAM and available conventional pages.
  - Handle buffer enumeration discovering GOP framebuffers (resolution and base MMIO address) and scanning PCI bus devices.
  - Clean exit sequence returning `EFI_SUCCESS` to UEFI Boot Services.
- **Hardware Abstraction Layer DXE Driver (`OpenVintageHalDxe.efi`)**:
  - Installs `OPEN_VINTAGE_HAL_PROTOCOL` (`gOpenVintageHalProtocolGuid`) for use by DXE drivers and bootloaders.
  - Implements GPU discovery parsing PCI configuration space for Display Controllers (Intel HD, Nvidia GeForce, AMD Radeon).
- **Core Libraries**:
  - `OpenVintageCoreLib`: Modular routines for platform inspection, CPUID queries, and memory descriptor walks.
  - `OpenVintageLogLib`: Lightweight UEFI serial and console logging subsystem.
- **Automation Scripts**:
  - `scripts/build_firmware.sh`: End-to-end compilation script using EDK II BaseTools and GCC5.
  - `scripts/test_qemu.sh`: Automated QEMU test harness formatting a FAT32 ESP virtual disk and running headless boot verification.
- **Verified Binary Deliverables**:
  - `bin/OpenVintageBootApp.efi` (PE32+ executable, ~13 KB)
  - `bin/OPENVINTAGE.fd` (Flash Device Image, 4.0 MB)
  - `OpenVintagePkg/Firmware/OPENVINTAGE_DXEFV.Fv` (Firmware Volume, 4.0 MB)
  - `OpenVintagePkg/Drivers/OpenVintageHalDxe/OpenVintageHalDxe.efi` (DXE Driver, ~8.2 KB)

### Changed
- Transitioned project from conceptual UEFI design to an implemented, compiled, and virtualized firmware package verified in QEMU.
- Updated documentation (`README.md`, `ARCHITECTURE.md`, `PROJECT_STATUS.md`, `CHANGELOG.md`) reflecting verified build instructions, binary artifacts, and QEMU test traces.

---

## [0.1.0] - Initial Architecture Specification
### Added
- Architectural definitions for OVCore, OVResolver, OVScheduler, OVIR-GPU, and OVIR-CPU.
- Hardware capability database schema for legacy Intel Macs (2006–2015).
