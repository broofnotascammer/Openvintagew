# ◈ OpenVintage

> **Hardware-aware boot management, compatibility, and execution platform for legacy and modern systems.**

---

## Architecture Overview

OpenVintage bridges software workloads and hardware through a deterministic, modular pipeline:

```text
Application / Workload
        ↓
Workload Requirement Analysis
        ↓
OVResolver
        ↓
 ┌───────────────┬────────────────┐
 │ Native Path   │ Translation    │
 │               │ Required       │
 │               ↓
 │          OVIR-CPU / OVIR-GPU
 │               ↓
 │          Optimizer
 │               ↓
 │          Backend
 └───────────────┴────────────────┘
                ↓
          Cache Lookup/Store
                ↓
       OVScheduler / Resource
          Manager
                ↓
          Execution/Dispatch
                ↓
        Telemetry / Diagnostics
                ↓
          Compatibility Result
```

---

## Repository Structure

* **`OpenVintagePkg/`**: EDK II UEFI firmware pre-boot application (`OpenVintageBootApp.c`) with dynamic target enumeration, safe default fallback, and explicit handoff.
* **`core/`**:
  * `core/types/`: Core primitives, architectures, API, and decision definitions.
  * `core/resolver/`: `OVResolver` decision engine producing deterministic execution plans.
  * `core/cache/`: Unified Cache with multi-field metadata validation and invalidation.
  * `core/scheduler/`: `OVScheduler` and Resource Manager for admission control.
  * `core/telemetry/`: High-resolution microsecond latency measurement.
  * `core/diagnostics/`: Human-readable diagnostic report generator.
  * `core/pipeline/`: `ov_pipeline_execute()` unified entry point.
  * `core/boot/`: Pre-boot picker logic.
  * `core/installer/`: ESP safety auditor and external USB creator.
* **`ovir/`**:
  * `ovir/cpu/`: OVIR-CPU decoder, intermediate representation, optimizer, and x86-64 machine code emitter.
  * `ovir/gpu/`: OVIR-GPU graphics workload representation, shader ingestion, and backend mapping.
* **`tests/`**: Native C and Node.js test suites.
* **`src/`**: React developer interface for interactive pipeline evaluation and telemetry.

---

## Development & Hardware Testing Policy

* **Target Hardware**: MacBook Pro 15-inch, Mid-2012 (`MacBookPro9,1`, Core i7-3615QM, HD 4000 / GT 650M, macOS Catalina 10.15.8).
* **Strict Non-Destructive Rule**: Physical machine testing on the internal macOS installation is **strictly prohibited** during development.
* **Internal Storage Protection**: The internal disk (`disk0`, `disk0s1`, `disk0s2`) must never be modified, repartitioned, formatted, or written to.
* **Development Verification**: All intermediate development phases rely strictly on unit tests, integration tests, virtualized environments (QEMU), static analysis, and synthetic capability models.
* **Physical Validation Milestone**: Physical hardware testing is postponed until the **Final Phase — External USB Hardware Validation**, which runs exclusively from an isolated, removable external USB drive.
* **Detailed Policy**: See [DEVELOPMENT_AND_VALIDATION_POLICY.md](./DEVELOPMENT_AND_VALIDATION_POLICY.md) for full compliance guidelines and verification tiers.

---

## Build & Test Instructions

### Native C Build & Execution Tests:
```bash
make clean
make all
make test
```

### Node / TypeScript Test Suite:
```bash
npm test
```

### Development Web Server:
```bash
npm run dev
# Server accessible at http://localhost:3000
```
