/**
 * Synchronized copy of project specifications for in-app viewing.
 */

export const PROJECT_STATUS_TEXT = `# OpenVintage Project Status

================================================================================
PROJECT STATUS & ENGINEERING MILESTONES
Platform: OpenVintage Ecosystem
Last Updated: 2026-09-06
================================================================================

## Current Phase
Phase 1 Complete — Transitioning to Phase 2: Core Architecture & Platform Subsystems

---

## Completed Modules
- [x] OpenVintage EFI Application: Native X64 UEFI binary compiled with EDK II framework.
- [x] X64 Build Configuration: Targeted compilation settings for x86_64 legacy Intel architecture.
- [x] GCC Toolchain Support: Validated GCC/Clang cross-compilation pipeline for standalone execution.
- [x] Logger Subsystem: Low-overhead diagnostic logger functional across UEFI serial/console output.
- [x] Device Enumeration: Scans and parses system handles and PCI device configurations.
- [x] Block I/O Detection: Interrogates mass storage controllers, ATA/SATA/AHCI drives, and media descriptors.
- [x] Device Path Reporting: Resolves and logs ACPI, PCI, and hard drive device path nodes.
- [x] Filesystem / Media Detection: Validates FAT/ESP volume mounting and file system protocol instances.
- [x] Clean Application Exit: Implements graceful termination returning status EFI_SUCCESS to Boot Services.
- [x] QEMU Virtualized Testing: Validated runtime boot sequence in simulated target hardware.
- [x] OVMF UEFI Firmware Verification: Confirmed standard firmware protocol compatibility.

---

## Modules in Progress
- [ ] Ecosystem Baseline Documentation: Formalizing README.md, ARCHITECTURE.md, PROJECT_STATUS.md, CHANGELOG.md.
- [ ] OVCore Portability Layer: Hardware-independent interfaces for memory, strings, diagnostic telemetry.
- [ ] Unified Hardware Database (HAL): Silicon capability database for Intel HD, AMD Radeon, Nvidia GeForce.
- [ ] OVResolver Decision Matrix: Deterministic heuristics for evaluating workloads (Native, Translation, Simplification, Cache, Fallback).
- [ ] OVScheduler Dynamic Resource Coordinator: Core topology detection, thermal profile rules, thread mapping.

---

## Pending Modules
- [ ] OVIR-GPU Intermediate Representation Engine: Directed acyclic graph (DAG) command builder, render pass descriptors, and pipeline state objects.
- [ ] Multi-Tier Cache Engine: SHA256-keyed persistent cache for shaders, pipeline state objects, and texture swizzles.
- [ ] OVIR-CPU Instruction Translation Layer: Instruction decoder and SSA representation (scheduled for designated future phase).
- [ ] SolitaryOS Runtime Integration: OS-level hooks and launcher integration (scheduled for future phase).

---

## Last Successful Build
- UEFI Subsystem: X64 EDK II Release Target (OpenVintage.efi) — PASS
- Web Interface & Core Verification Applet: Vite / TypeScript 5.8 / React 19 Build — PASS

---

## Last Successful Test
- Test Suite: QEMU 8.x + OVMF X64 Firmware Emulation
  - Boot: Success
  - Device Path Discovery: Success (NVRAM, PCI Root, SATA AHCI Block IO)
  - Memory Map: Success
  - Logger Dump: Clean output stream
  - Return Code: EFI_SUCCESS (0x00000000)

---

## Known Limitations
1. Bare-Metal GPU Direct Submission: Currently constrained to UEFI GOP (Graphics Output Protocol) framebuffer access in Phase 1; hardware 3D acceleration requires OS kernel drivers or custom PCIe MMIO ring buffer initialization.
2. OVIR-CPU Scope: Architectural design defined; full dynamic binary translator is intentionally deferred to its scheduled phase to avoid over-engineering.
3. Legacy Silicon Diversity: Non-standard Apple EFI quirks (e.g. 32-bit EFI with 64-bit Core 2 Duo CPUs on 2006 Mac Pros, dual-GPU mux switching on MacBook Pros) require explicit hardware database quirks tables.`;

export const ARCHITECTURE_TEXT = `# OPENVINTAGE ARCHITECTURAL SPECIFICATION

================================================================================
OPENVINTAGE SYSTEMS ARCHITECTURE
Revision: 2.0.0
Classification: Systems Engineering Specification
Status: Active Implementation
================================================================================

1. Architectural Overview
OpenVintage rejects the traditional M x N matrix of direct API-to-API bridges:

Metal    ──┐
Vulkan   ──┼──► [ OVIR-GPU ] ──► Target Hardware Backend (OpenGL / Metal / Vulkan)
DirectX  ──┤
OpenGL   ──┘

Decomposed into three primary control layers and two translation hubs:

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

2. Core Subsystems
- OVCore: Foundational runtime, memory management primitives, logging, and portable platform abstraction.
- OVResolver: Dynamic path selection matrix evaluating Native, Translation, Simplification, Cache, or Fallback.
- OVScheduler: Dynamic CPU core topology and thermal coordinator supporting 5 performance profiles.
- OVIR-GPU: Universal Graphics Hub & Spoke intermediate representation.
- OVIR-CPU: Future instruction IR (Design phase; deferred to designated milestone).

3. Hardware Abstraction Layer (HAL)
Unified capability database covering Intel HD Graphics (3000/4000/5200), AMD Radeon HD/GCN, and Nvidia Kepler/Tesla.

4. Multi-Tier Caching Architecture
SHA256-keyed persistent cache for shaders, pipeline state objects (PSO), texture swizzles, and resolution decisions.

5. SolitaryOS Ecosystem Boundary
SolitaryOS is a dedicated lightweight Linux OS project designed for bare-metal application execution. Maintained as a separate roadmap phase.`;

export const README_TEXT = `# OPENVINTAGE

Modular compatibility and performance platform originally designed around legacy Intel Mac hardware.

Purpose:
- Extend useful life of older hardware (2006-2015 Intel Macs)
- Eliminate brittle point-to-point translation layers via Intermediate Representations
- Provide dynamic execution path selection via OVResolver
- Orchestrate resources via OVScheduler
- Maintain portable architecture for future Linux, Windows, macOS, and bare-metal targets

Current Phase:
- Phase 1 Complete: Working X64 UEFI application in EDK II verified in QEMU + OVMF.
- Phase 2 Active: Core modular architecture and unified abstraction layer.`;

export const CHANGELOG_TEXT = `# OpenVintage Changelog

## [0.2.0] - 2026-09-06
- Formalized comprehensive architectural specification (ARCHITECTURE.md).
- Updated project status tracker and phase verification records (PROJECT_STATUS.md).
- Established master project overview and engineering rules (README.md).
- Created interactive systems engineering console with OVResolver sandbox and OVScheduler controller.
- Transitioned project status from Phase 1 to Phase 2.

## [0.1.0] - Phase 1 Complete
- Working X64 UEFI application compiled with EDK II and GCC toolchain.
- Device enumeration, block I/O detection, and device path reporting verified in QEMU + OVMF.
- Clean exit with EFI_SUCCESS.`;
