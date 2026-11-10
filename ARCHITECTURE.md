# OpenVintage — Unified System Architecture

## Overview
OpenVintage is a hardware-aware compatibility, execution, and performance platform sitting between software workloads and host hardware. It bridges legacy and modern architectures, providing deterministic execution planning, JIT translation (OVIR-CPU), graphics translation (OVIR-GPU), unified caching, and resource scheduling.

---

## 1. System Topology & Implementation Roles

| Subsystem | Source Location | Implementation Role |
| :--- | :--- | :--- |
| **Firmware / Pre-boot** | `OpenVintagePkg/OpenVintageBootApp/` | Native EDK II UEFI application dispatched by Apple BootROM; performs pre-boot hardware init, multi-disk UEFI volume scanning, interactive boot picker, and safe `LoadImage`/`StartImage` handoff. |
| **Native Core Platform** | `core/` | Canonical native execution engine: `core/resolver` (OVResolver), `core/scheduler` (OVScheduler & Resource Manager), `core/cache` (Unified Cache), `core/pipeline` (Integrated Execution API), `core/telemetry`, `core/diagnostics`. |
| **Intermediate Representation** | `ovir/` | `ovir/cpu` (ARM64 decode &rarr; OVIR intermediate instructions &rarr; optimizer &rarr; x86-64 emitter &rarr; verifier &rarr; simulation), `ovir/gpu` (Resource creation, shader ingestion, pipeline builder, draw execution, sync). |
| **Pre-boot Simulator** | `OpenVintagePrebootSimulator/` & Web UI | Pre-boot and execution pipeline development and visualization simulator; exposes interactive workload analysis, UEFI picker preview, and telemetry. |
| **Platform Adapters** | `platforms/` & `macOS/` | Host OS abstraction adapters (macOS AppKit bundle, Linux host wrappers). |
| **Developer UI** | `src/` & `server.ts` | AI Studio full-stack developer interface, real-time workload evaluator, and test runner. |
| **Test Harness** | `tests/` & `bin/` | Native C test suites (`test_boot_handoff`, `test_unified_pipeline`) and Node.js deterministic test runners (`boot_handoff.test.js`, `unified_pipeline.test.js`). |

---

## 2. Phase 10 Unified Execution Pipeline

```text
               Application / Workload Description
                             │
                             ▼
              Workload Requirement Analysis
                             │
                             ▼
                        OVResolver
                             │
      ┌──────────────────────┴──────────────────────┐
      │                                             │
Native Path                                Translation Required
(Direct Host Dispatch)                              │
                                                    ▼
                                           OVIR-CPU / OVIR-GPU
                                                    │
                                                    ▼
                                                Optimizer
                                                    │
                                                    ▼
                                             Target Backend
      │                                             │
      └──────────────────────┬──────────────────────┘
                             │
                             ▼
                    Cache Lookup / Store
               (Strict Metadata Validation)
                             │
                             ▼
             OVScheduler / Resource Manager
            (Priority, Cores, RAM, VRAM Admission)
                             │
                             ▼
                    Execution / Dispatch
                             │
                             ▼
                  Telemetry & Diagnostics
              (Real &mu;s Timers, Inspection Report)
                             │
                             ▼
                    Compatibility Result
```

---

## 3. Component Details

### 3.1 OVResolver (`core/resolver/`)
Constructs an explicit, deterministic `ov_execution_plan_t` for any incoming workload:
* Evaluates host resources (RAM, VRAM, CPU cores).
* Verifies CPU and GPU hardware capability feature masks.
* Determines whether native execution is viable.
* Evaluates if CPU translation (OVIR-CPU) or GPU translation (OVIR-GPU) is required.
* Categorizes decision: `NATIVE`, `CPU_TRANSLATION`, `GPU_TRANSLATION`, `CPU_AND_GPU_TRANSLATION`, or `UNSUPPORTED`.
* Produces granular reason codes (e.g., `OV_REASON_CPU_ARCH_MISMATCH_TRANSLATABLE`, `OV_REASON_INSUFFICIENT_RAM`, `OV_REASON_UNSUPPORTED_GPU_FEATURE`).

### 3.2 OVIR-CPU (`ovir/cpu/`)
* **Decoder**: Decodes raw instruction streams (e.g. ARM64 `MOV`, `ADD`, `SUB`, `MUL`, `RET`, logic, compares, memory load/store, branches, syscall, and barriers).
* **NEON Vector Decoder**: Real 32-bit bitstream decoders for `ADD.4S`, `SUB.4S`, `MUL.4S`, `ADD.2D`, `SUB.2D`, `AND.16B`, `ORR.16B`, `EOR.16B`, `DUP`, and `MOVI`.
* **Register Allocator**: Tracks guest registers $X0$–$X30$, zero register $XZR$ (strictly constant zero), $SP$, $W$ vs $X$ width differentiation, and callee/caller save conventions.
* **Condition Code Evaluator**: Full evaluation of all 14 standard ARM64 condition codes with flag-preservation invariants during optimization.
* **IR & Optimizer**: Neutral intermediate representation (`ovir_insn_t`) with constant folding and dead code elimination.
* **Backend**: Modular x86-64 machine code emitter supporting scalar integer and SSE2/SSE4.1 128-bit SIMD vector instructions with verified scalar equivalence.
* **Native JIT Execution**: Strict $W \oplus X$ memory allocation with `__builtin___clear_cache()` synchronization and zero memory leakage.
* **DBT Trace Cache & Linking**: Direct block linking (`ov_block_link_t`), hot-loop detection (`OV_HOT_TRACE_THRESHOLD = 5`), and cascade invalidation.

### 3.3 Guest Virtual MMU & TLB (`core/mmu/`)
* **Two-Level Page Table**: 512-entry Page Global Directory (PGD) and 512-entry Page Table Entry (PTE) hierarchy of 4KB pages, providing 1GB virtual addressability per PGD.
* **64-Entry Associative TLB**: Fully associative with ASID tagging, generation tracking, and LRU eviction.
* **Structured Fault Traps**: Traps and reports `OV_MMU_FAULT_TRANSLATION`, `OV_MMU_FAULT_PERMISSION`, and `OV_MMU_FAULT_PRIVILEGE`.

### 3.4 OVIR-GPU (`ovir/gpu/`)
* Represents graphics pipeline workloads: Resource creation (VBOs, textures) &rarr; Shader ingestion &rarr; Pipeline description &rarr; Draw operations &rarr; Synchronization barriers.
* 5-Stage route evaluation hierarchy: Native &rarr; Translation &rarr; Reconstruction &rarr; CPU Fallback &rarr; Unsupported.
* Validates requested GPU features against host GPU capabilities (Metal Native, MoltenVK, OpenGL Core).
* Rejects unsupported hardware operations cleanly without crashing or guessing.

### 3.5 Unified Cache (`core/cache/`)
* Caches both compiled CPU translation blocks and translated GPU shaders/pipelines.
* Enforces strict validation metadata:
  * `source_arch`, `target_arch`, `code_identity_hash`, `translator_version_id`, `required_features`, `optimization_level`.
* Automatically invalidates entries on metadata mismatch, preventing stale execution.

### 3.6 OVScheduler & Resource Manager (`core/scheduler/`)
* Manages host CPU core, RAM, and VRAM pools.
* Selects scheduling profiles: `Energy Saver`, `Balanced`, `Performance`, `Max Throughput`.
* Implements admission control: grants tickets upon successful resource allocation, or rejects overcommits with explicit error codes.

### 3.7 Telemetry & Diagnostics (`core/telemetry/`, `core/diagnostics/`)
* Uses high-resolution monotonic clocks (`clock_gettime(CLOCK_MONOTONIC)`) to measure exact microsecond latencies for resolver decisions, cache lookups, translation, optimization, emission, and scheduling.
* Produces inspectable diagnostic summaries answering: *"What did OpenVintage do with this workload, and why?"*

---

## 4. Development and Hardware Validation Policy

OpenVintage enforces strict development and testing rules documented canonically in [`DEVELOPMENT_AND_VALIDATION_POLICY.md`](./DEVELOPMENT_AND_VALIDATION_POLICY.md):

* **Target Hardware**: MacBook Pro 15-inch, Mid-2012 (`MacBookPro9,1`, Core i7-3615QM, HD 4000 / GT 650M Kepler, macOS Catalina 10.15.8 Build 19H2036).
* **Mandatory Testing Policy**: Physical machine testing on the internal macOS installation is **strictly prohibited** during development. The internal disk (`disk0`, `disk0s1`, `disk0s2`) must never be modified or targeted.
* **5-Tier Verification Model**: All intermediate development results are classified into strict tiers:
  1. *Tier 1 (Compiles)* &rarr; 2. *Tier 2 (Unit Tested)* &rarr; 3. *Tier 3 (Integration Tested)* &rarr; 4. *Tier 4 (Host Backend Verified)* &rarr; 5. *Tier 5 (Physical MacBook Tested)*.
  Subsystems passing automated tests must never be labeled as physically hardware validated.
* **Final USB Validation Milestone**: Physical testing is executed only after all planned phases are complete, deployed exclusively to an isolated external USB media.
