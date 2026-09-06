# OPENVINTAGE

> **Modular compatibility and performance platform originally designed around legacy Intel Mac hardware.**

---

## 1. Executive Summary

**OpenVintage** is an experimental systems ecosystem engineered to extend the functional lifespan and performance envelope of legacy hardware, with primary initial focus on **Intel Mac systems** (2006–2015 era architectures: Core 2 Duo, Nehalem, Sandy Bridge, Ivy Bridge, Haswell, Broadwell, Skylake) and designed with portability to Linux, Windows, macOS, and future bare-metal runtime targets.

Rather than relying on brittle, combinatorial point-to-point translation layers ($M \times N$ bridges such as Metal→OpenGL, Vulkan→OpenGL, DirectX→Metal), OpenVintage establishes **intermediate representations (IR)** and an **intelligent decision engine**:

```
                    OPENVINTAGE ECOSYSTEM

                           OpenVintage
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
 Graphics APIs    CPU Architectures
        │               │
 Metal           ARM
 Vulkan          x86 / x86-64
 OpenGL          Future: RISC-V
 DirectX
```

---

## 2. Core Pillars

| Subsystem | Primary Role |
| :--- | :--- |
| **OVCore** | Foundational runtime, memory management primitives, logging, and portable platform abstraction layer. |
| **OVResolver** | Intelligent execution path selector; determines whether workloads run native, translated via IR, simplified, served from cache, or redirected to CPU fallback. |
| **OVScheduler** | Workload & resource manager; dynamically adjusts to CPU core topology, thermal headroom, memory pressure, and performance profiles (Battery, Balanced, Performance, Max Performance, Developer). |
| **OVIR-GPU** | Unified graphics intermediate representation decoupling frontend APIs (Metal, Vulkan, OpenGL, DirectX) from legacy GPU execution backends. |
| **OVIR-CPU** | Instruction intermediate representation for multi-architecture emulation and JIT compilation (ARM $\leftrightarrow$ x86/x86-64, future RISC-V). *(Design phase)* |
| **Hardware HAL** | Unified hardware detection database abstracting CPU, GPU, RAM, PCI devices, and storage topologies. |
| **Cache Subsystem** | Multi-tier validated cache covering compiled shaders, pipeline states, texture conversions, and translation blocks. |

---

## 3. Current Phase Status

### Phase 1: COMPLETE
- **Target**: X64 UEFI Application using EDK II.
- **Achievements**:
  - OpenVintage EFI application compiled and verified.
  - X64 build configuration with GCC toolchain support.
  - Subsystem logger initialized.
  - Device enumeration, Block I/O detection, and Device Path reporting operational.
  - Filesystem/media detection confirmed.
  - Clean application exit verified.
  - **Testing**: Executed and verified inside QEMU + OVMF virtualized firmware environment.

### Phase 2: IN PROGRESS
- **Target**: Core Modular Architecture, Unified Hardware Abstraction Layer, and OVResolver / OVScheduler Foundations.
- **Objectives**:
  - Formalize modular TypeScript/C interfaces across all subsystems.
  - Implement unified Hardware Capability Database for legacy Intel Macs (Intel HD Graphics 3000/4000/5000/Iris, AMD Radeon HD/GCN, Nvidia Kepler/Tesla).
  - Implement deterministic OVResolver path selection matrix.
  - Implement OVScheduler dynamic profile policies and memory tracking.
  - Define OVIR-GPU specification schema and pipeline states.

---

## 4. Engineering Discipline

1. **Modularity**: Subsystems must be independently testable with zero circular dependencies.
2. **No Monoliths**: Logic is partitioned into specialized modules (`core`, `resolver`, `scheduler`, `hardware`, `ovir-gpu`, `cache`).
3. **Verified Testing**: No component is marked passing without concrete build or test evidence.
4. **Honest Limitations**: Untested or stubbed subsystems are clearly documented.
5. **No Synthetic "AI" Claims**: OVResolver uses rigorous capability heuristics, deterministic graph traversal, and verified hardware matrices.
6. **No Phantom Capabilities**: Hardware features are matched against real physical silicon constraints.

---

## 5. Ecosystem Roadmap & SolitaryOS

OpenVintage maintains a strict boundary with **SolitaryOS**:
- **OpenVintage** is the platform-agnostic compatibility, IR, and scheduling layer.
- **SolitaryOS** is a planned future lightweight Linux-based operating system designed for streamlined application/game execution without traditional desktop overhead. OpenVintage components will integrate cleanly into SolitaryOS when Phase 4 is reached.

---

## 6. Repository Navigation

- `ARCHITECTURE.md` — Detailed systems architecture, dataflow diagrams, and interface specifications.
- `PROJECT_STATUS.md` — Active phase tracking, completed modules, tests, and pending milestones.
- `CHANGELOG.md` — Chronological log of changes across phases.
