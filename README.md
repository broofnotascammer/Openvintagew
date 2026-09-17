![LoGo](logo.png)
◈ OpenVintage

Legacy hardware. Modern possibilities.

OpenVintage is an experimental, hardware-aware compatibility and boot platform designed to extend the useful life of older Apple hardware while providing a foundation for compatibility with newer operating-system generations.

It combines EFI control, hardware intelligence, compatibility routing, driver/kext integration, CPU/GPU abstraction, userspace compatibility, adaptive optimization, and transactional recovery into one architecture.

«OpenVintage does not promise compatibility simply because an architecture exists.
Hardware and OS support must be implemented, tested, and verified.»

---

✦ The Idea

                         ┌─────────────────────┐
                         │     OPENVINTAGE     │
                         └──────────┬──────────┘
                                    │
                         ┌──────────▼──────────┐
                         │ Hardware Intelligence│
                         └──────────┬──────────┘
                                    │
                         ┌──────────▼──────────┐
                         │ Compatibility Router│
                         └───────┬───────┬──────┘
                                 │       │
                    ┌────────────▼─┐   ┌─▼────────────┐
                    │    LEGACY    │   │   OPTIMUS    │
                    │  macOS 11–15 │   │  macOS 27+   │
                    └──────┬───────┘   └──────┬───────┘
                           │                  │
                           └────────┬─────────┘
                                    │
                         ┌──────────▼──────────┐
                         │ Integration Engine  │
                         └──────────┬──────────┘
                                    │
               ┌────────────────────┼────────────────────┐
               ▼                    ▼                    ▼
          Kext / Drivers        Boot Plans          OS Patches
               │                    │                    │
               └────────────────────┼────────────────────┘
                                    │
                         ┌──────────▼──────────┐
                         │ Compatibility Layer │
                         └──────┬────────┬─────┘
                                │        │
                           ┌────▼───┐ ┌──▼────┐
                           │OVIR CPU│ │OVIR GPU│
                           └────┬───┘ └──┬────┘
                                │        │
                                └────┬───┘
                                     ▼
                          Adaptive Optimization
                                     │
                                     ▼
                            macOS Experience

The architecture is intentionally layered.

EFI handles early hardware discovery and boot orchestration.
The compatibility system determines what pathway applies.
The integration engine prepares the required components.
The compatibility layers handle CPU, GPU, graphics and userspace differences.
The adaptive system measures real behaviour and learns validated configuration profiles.

---

🎯 Project Goals

OpenVintage is built around several core objectives:

- 🧠 Hardware intelligence
- 🔀 Automatic compatibility routing
- 🖥️ Legacy macOS compatibility
- 🚀 Experimental future-macOS compatibility
- 🧩 Hardware-aware kext and driver integration
- ⚙️ EFI-level boot orchestration
- 🎮 GPU and graphics compatibility
- 🧮 CPU architecture compatibility
- 🛠️ Userspace compatibility
- 📈 Adaptive performance optimization
- 🧪 Continuous validation
- 🛡️ Transactional system modification
- 🔄 Automatic recovery

The goal is not simply to make an old Mac boot.

The goal is to build a system that can understand the machine, determine its capabilities, construct an appropriate compatibility plan, validate that plan, and continuously improve the configuration without sacrificing recoverability.

---

🧠 Hardware Intelligence

OpenVintage begins with the hardware.

Instead of treating every Mac as the same machine, the system constructs an authoritative hardware model containing information such as:

- Mac model identifier
- CPU architecture
- CPU generation
- GPU inventory
- GPU architecture
- VRAM
- display topology
- memory
- storage
- PCI devices
- USB devices
- networking hardware
- audio hardware
- Bluetooth
- cameras
- sensors
- firmware capabilities
- boot environment
- operating-system version

Conceptually:

Physical Machine
       │
       ▼
Hardware Discovery
       │
       ▼
OvHardwareSnapshot
       │
       ▼
Capability Graph
       │
       ▼
Compatibility Input

This same hardware model can then be consumed by the Legacy and Optimus pathways.

---

🔀 Compatibility Router

The router determines which compatibility architecture owns the target operating system.

macOS generation| OpenVintage pathway| Status
macOS 11–15| 🟦 Legacy| Supported architecture
macOS 16–26| ⚪ Unknown / Unsupported| Explicitly unassigned
macOS 27+| 🟪 Optimus| Experimental architecture

🟦 Legacy

Legacy is specifically responsible for:

macOS 11 → macOS 15

It provides the compatibility architecture for existing legacy-macOS techniques and OCLP-compatible integration.

🟪 Optimus

Optimus is OpenVintage's experimental future-macOS compatibility pathway for:

macOS 27 and later

«Important: “Optimus” is the name of an OpenVintage compatibility pathway. It does not refer to NVIDIA Optimus GPU switching.»

Legacy and Optimus share common hardware intelligence and infrastructure, but their OS-specific compatibility logic remains deliberately separated.

---

🟦 LEGACY

macOS 11–15

The Legacy pathway is intended to integrate established compatibility mechanisms into the OpenVintage architecture.

                  LEGACY
                    │
          ┌─────────┴─────────┐
          ▼                   ▼
    OS Version Profile   Hardware Profile
          │                   │
          └─────────┬─────────┘
                    ▼
              Compatibility
                    │
                    ▼
               Boot Plan
                    │
                    ▼
             Validation
                    │
                    ▼
                macOS

Potential responsibilities include:

- macOS version profiles
- hardware-specific compatibility
- OCLP-compatible interfaces
- kext integration
- boot arguments
- graphics patches
- system configuration
- compatibility validation
- recovery configuration

OpenVintage should not claim OCLP functionality is implemented until the relevant functionality actually exists in the repository.

---

🟪 OPTIMUS

macOS 27+

Optimus is the experimental OpenVintage architecture for future macOS generations.

Its purpose is to provide a framework for situations where existing legacy compatibility mechanisms may no longer be sufficient.

Potential layers include:

Future macOS
     │
     ▼
Userspace Compatibility
     │
     ▼
Framework / API Compatibility
     │
     ▼
Graphics Compatibility
     │
     ▼
OVIR-GPU
     │
     ▼
Hardware Capability Resolver
     │
     ▼
Physical Hardware

The same concept applies to CPU compatibility through OVIR-CPU.

Optimus is experimental. Architecture and code availability must not be confused with verified support for Tahoe or future macOS releases.

---

🧩 Kext & Driver Integration

OpenVintage treats drivers and kexts as hardware-aware components rather than blindly installing everything available.

The integration system should evaluate:

Hardware
   │
   ▼
Capability Match
   │
   ▼
Kext Manifest
   │
   ├── Dependencies
   ├── OS Requirements
   ├── Hardware Requirements
   ├── Load Order
   ├── Validation Rules
   └── Recovery Metadata
   │
   ▼
Boot Integration

The architecture is intended to cover areas such as:

- 🎨 Graphics
- 🖥️ Display
- 🔊 Audio
- 🌐 Ethernet
- 📡 Wi-Fi
- 🔵 Bluetooth
- 💾 Storage
- 🔌 USB
- ⌨️ Input
- 📷 Camera
- 🌡️ Sensors

Components should only be selected when they match the detected hardware and target environment.

---

⚙️ Boot Integration Engine

OpenVintage uses a plan-first approach to system modification.

Discover
   ↓
Simulate
   ↓
Plan
   ↓
Review
   ↓
User Approval
   ↓
Backup
   ↓
Apply
   ↓
Boot
   ↓
Verify
   ↓
Commit

A boot plan can describe:

OvBootPlan
├── Target OS
├── Compatibility Pathway
├── Hardware Profile
├── Kext Set
├── Patch Set
├── Boot Arguments
├── Graphics Configuration
├── Security Configuration
└── Recovery Configuration

This prevents the EFI layer from becoming one enormous monolithic collection of drivers and OS logic.

---

🎮 Graphics Compatibility

Graphics compatibility is treated as a complete pipeline rather than a single GPU check.

macOS Graphics API
        │
        ▼
Graphics Compatibility Layer
        │
        ▼
OVIR-GPU
        │
        ▼
GPU Capability Resolver
        │
        ├── Feature Translation
        ├── Resource Translation
        ├── Shader Compatibility
        ├── Pipeline Handling
        └── Capability Fallback
        │
        ▼
Hardware GPU

OpenVintage distinguishes between:

- GPU detection
- GPU capability
- driver compatibility
- graphics API compatibility
- display routing
- display capabilities
- shader compatibility
- OS framework compatibility

This distinction is especially important on systems containing multiple GPUs.

---

🧮 CPU Compatibility

OVIR-CPU provides an architecture-neutral execution layer.

Source Architecture
        │
        ▼
Architecture Decoder
        │
        ▼
OVIR-CPU IR
        │
        ▼
Optimizer
        │
        ▼
Target Backend
        │
        ▼
Native Execution

Potential components include:

- architecture-neutral IR
- ARM64 decoding
- x86-64 decoding
- optimization passes
- x86-64 backend
- ARM64 backend
- translation cache
- JIT pipeline
- scheduler integration

The objective is to provide a structured compatibility layer rather than treating binary translation as a single opaque component.

---

🖥️ Userspace Compatibility

Not every compatibility problem exists inside EFI or the kernel.

OpenVintage therefore provides a separate userspace compatibility layer for problems involving:

- frameworks
- APIs
- application requirements
- OS-version differences
- graphics frameworks
- compatibility shims
- fallback behaviour
- application diagnostics

The architecture becomes:

EFI
 ↓
Kernel / Drivers
 ↓
Compatibility Layers
 ↓
Userspace
 ↓
Applications

Each layer has its own failure and recovery model.

---

📈 Adaptive Performance

OpenVintage is designed to measure actual system behaviour instead of blindly applying optimization flags.

Possible telemetry includes:

- CPU utilization
- GPU utilization
- GPU memory
- system memory pressure
- frame timing
- shader compilation time
- translation overhead
- cache hit rate
- I/O performance
- thermal state
- application compatibility
- boot performance

The optimization objective is multi-dimensional:

Performance
    +
Visual Quality
    +
Responsiveness
    +
Power Efficiency
    +
Stability
    +
Compatibility

The system should select configurations based on measured behaviour and the user's chosen priorities.

---

🤖 Self-Learning Optimization

OpenVintage can learn from validated results.

The intended loop is:

Observe
   ↓
Analyze
   ↓
Generate Candidate
   ↓
Validate
   ↓
Measure
   ↓
Accept / Reject
   ↓
Store Profile
   ↓
Future Optimization

The learning system should primarily learn configuration and optimization profiles, not arbitrarily rewrite its own executable code.

Examples of learnable information could include:

Hardware + OS + Workload
            │
            ▼
      Optimization Profile
            │
      ┌─────┼─────┐
      ▼     ▼     ▼
   Graphics Cache Scheduler

A stable baseline must always remain available.

---

🛡️ Safety Model

OpenVintage follows a conservative modification model.

Never assume:

"It booted once"
        ≠
"It is safe"

Instead:

Candidate Configuration
        │
        ▼
Validation
        │
        ▼
Controlled Application
        │
        ▼
Boot Verification
        │
        ▼
Performance Verification
        │
        ▼
Compatibility Verification

Failed configurations should be isolated and reversible.

---

🔄 Transactional System Modification

System changes should behave like transactions.

Known Good
    │
    ▼
Backup
    │
    ▼
Prepare
    │
    ▼
Validate
    │
    ▼
Apply
    │
    ▼
Boot
    │
 ┌──┴──┐
 ▼     ▼
PASS  FAIL
 │     │
 ▼     ▼
Commit Rollback

The recovery system should distinguish between:

- boot failure
- driver failure
- graphics failure
- userspace failure
- compatibility failure
- performance regression

---

🧪 Simulation & Validation

OpenVintage is designed to validate changes before touching physical hardware.

Testing environments include:

- QEMU
- OVMF
- synthetic hardware
- firmware images
- preboot simulation
- automated test harnesses
- native macOS hardware

Tests should be categorized clearly:

Status| Meaning
🟢 "IMPLEMENTED"| Functionality exists
🔵 "TESTED"| Verified in a defined environment
🟡 "PARTIAL"| Some functionality exists
🟣 "EXPERIMENTAL"| Research / incomplete validation
⚪ "UNSUPPORTED"| Not currently supported

This prevents architectural capability from being mistaken for real-world compatibility.

---

🏗️ Architecture

The existing OpenVintage architecture contains major subsystems including:

OpenVintagePkg
│
├── Core
├── HAL
├── Hardware
├── Resolver
├── Scheduler
├── Resource Manager
├── Compatibility
├── Diagnostics
├── Benchmark
├── Performance System
│
├── OVIR-CPU
│
└── OVIR-GPU

OVIR-GPU provides the graphics abstraction and capability infrastructure.

OVIR-CPU provides the architecture-neutral CPU compatibility infrastructure.

The resolver connects hardware capabilities with execution and compatibility plans.

---

🍎 Native macOS Integration

OpenVintage includes a native macOS application architecture using SwiftUI/AppKit.

The native backend can inspect system information such as:

- machine identity
- CPU
- memory
- PCI devices
- GPU vendor/device information
- VRAM
- hardware topology

Physical GPU inventory and active display routing are treated as separate concepts.

The native application is intended to provide the user-facing control plane while EFI and low-level components handle early boot responsibilities.

---

🧭 Development Roadmap

Phase 1 — EFI Foundation

Bootloader and foundational EFI infrastructure.

Phase 2 — Hardware Abstraction

Hardware detection and HAL architecture.

Phase 3 — OVIR-GPU

GPU abstraction, capability modeling and graphics IR.

Phase 4 — Architecture & Resolver

Compatibility planning and architecture-neutral resolution.

Phase 5 — System Integration

Core, HAL, resolver, CPU/GPU IR, scheduler and supporting systems.

Phase 6 — Hardware Intelligence

Authoritative hardware snapshots and capability graphs.

Phase 7 — Compatibility Router

Legacy / Unknown / Optimus routing.

Phase 8 — Kext & Driver Integration

Hardware-aware component manifests and validation.

Phase 9 — Boot Integration Engine

Declarative boot plans and transactional application.

Phase 10 — Legacy

macOS 11–15 compatibility architecture.

Phase 11 — Optimus Foundation

macOS 27+ experimental compatibility architecture.

Phase 12 — Graphics Compatibility

macOS graphics integration with OVIR-GPU.

Phase 13 — Userspace Compatibility

Framework, API and application compatibility.

Phase 14 — Adaptive Performance

Measured performance optimization.

Phase 15 — Visual Optimization

Graphics quality, display and frame-pacing optimization.

Phase 16 — Self-Learning Optimizer

Validated adaptive configuration learning.

Phase 17 — Recovery & Transactions

Backup, verification, rollback and known-good profiles.

Phase 18 — Universal Validation

Cross-hardware, cross-OS, regression and recovery testing.

---

📁 Repository Structure

The architecture is organized around clear ownership boundaries.

Openvintagew/
│
├── OpenVintagePkg/
│   ├── Core/
│   ├── HAL/
│   ├── Hardware/
│   ├── Resolver/
│   ├── Scheduler/
│   ├── ResourceManager/
│   ├── Compatibility/
│   ├── Diagnostics/
│   ├── Benchmark/
│   ├── OvirCpu/
│   └── OvirGpu/
│
├── OpenVintagePrebootSimulator/
│
├── OpenVintage.app/
│
├── Tests/
│
├── ARCHITECTURE.md
├── PROJECT_STATUS.md
├── SYSTEM_STATUS.md
├── SECURITY.md
├── CHANGELOG.md
└── Makefile

Additional Legacy and Optimus implementation directories can be introduced as those phases are implemented.

---

🔐 Security Philosophy

OpenVintage operates at extremely low system levels.

That means security and recovery are not optional features.

Important principles include:

- least privilege where possible
- explicit user approval
- signed or validated components where applicable
- deterministic boot plans
- configuration backups
- known-good recovery states
- isolated experimental features
- no silent destructive modification
- clear distinction between simulation and physical application

«Experimental compatibility should never mean experimental safety.»

---

📊 Current State

OpenVintage already contains the foundations for:

- EFI boot infrastructure
- hardware abstraction
- hardware discovery
- resolver architecture
- OVIR-CPU
- OVIR-GPU
- scheduler infrastructure
- compatibility modeling
- diagnostics
- benchmarking
- performance measurement
- native macOS integration
- preboot simulation
- QEMU/OVMF validation infrastructure

The next architectural direction is to connect these foundations into the Hardware Intelligence → Compatibility Router → Integration → Compatibility → Optimization → Recovery pipeline.

---

⚠️ Important Status Notice

OpenVintage is an experimental research and engineering project.

The existence of an implementation, abstraction layer, adapter, resolver, or compatibility interface does not automatically mean that a particular Mac or macOS release is supported.

In particular:

- Legacy targets macOS 11–15.
- macOS 16–26 remain explicitly unassigned unless separately implemented.
- Optimus targets macOS 27+ as an experimental architecture.
- Future macOS compatibility is not guaranteed.
- Firmware artifacts are controlled test artifacts unless hardware-specific validation has been completed.
- Physical firmware modification requires separate hardware-specific investigation and validation.

---

✦ Design Principles

Hardware-aware

Every major decision should consider the actual machine.

Modular

EFI, kernel, compatibility, userspace and optimization layers should remain independently understandable.

Measurable

Optimization decisions should be based on observed results.

Reversible

System changes should always have a recovery strategy.

Explicit

Unsupported configurations should be reported as unsupported rather than silently guessed.

Testable

Simulation should precede physical deployment whenever possible.

Adaptive

The system can learn from validated results without sacrificing a known-good baseline.

Honest

No compatibility claim should be made without evidence.

---

🌌 Project Philosophy

OpenVintage is built around a simple idea:

«Old hardware should not automatically mean obsolete hardware.»

A machine's age does not completely describe its capabilities.

With hardware-aware compatibility, abstraction, translation, intelligent integration and measured optimization, OpenVintage explores how legacy hardware can remain useful in modern computing environments.

             DISCOVER
                 │
                 ▼
              UNDERSTAND
                 │
                 ▼
               ADAPT
                 │
                 ▼
              VALIDATE
                 │
                 ▼
              OPTIMIZE
                 │
                 ▼
              RECOVER
                 │
                 └──────────────► LEARN

OpenVintage — Legacy hardware. Modern possibilities.
