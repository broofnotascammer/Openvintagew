# OpenVintage Project Status

## Current Phase: Phase 15.1 — Real Native Hardware Discovery & Platform Detection
**Status**: Completed and Verified

---

## 1. Master Development Roadmap & Verification Tiers

The project progresses through sequential developmental phases, culminating in a dedicated physical validation milestone. Real hardware testing on the MacBook is strictly deferred to the final milestone.

| Phase / Milestone | Status | Testing Level | Description |
| :--- | :--- | :--- | :--- |
| **Phase 9.1** — Boot Picker & Safe EFI Handoff | **COMPLETED** | Unit + Emulated | Dynamic volume enumeration, interactive picker, 5s timeout, safe default target fallback, `gBS->LoadImage()`/`StartImage()` handoff. |
| **Phase 10** — Unified Execution Pipeline | **COMPLETED** | Integration | `OVResolver`, `OVIR-CPU`, `OVIR-GPU`, Unified Cache, `OVScheduler`, and monotonic microsecond telemetry. |
| **Phase 11** — Executable Memory & Fallback Runtime | **COMPLETED** | Integration | Strict $W \oplus X$ virtual memory, translation block lifecycle, 5-stage GPU route hierarchy, CPU software fallback rasterizer. |
| **Phase 12** — Real OVIR-CPU Workloads | **COMPLETED** | Integration + Host JIT | Full ARM64 decoder subset, neutral IR, constant folding, dead-code elimination, host x86-64 native W^X JIT, privilege rings, syscall table. |
| **Phase 13** — DBT, NEON & Guest Virtual MMU | **COMPLETED** | Integration + Host JIT | JIT register manager ($X0$–$X30$, $XZR$, $SP$, $W$/$X$ width), 14 condition codes, DBT trace cache & direct block linking (threshold 5), ARM64 NEON decoding & SSE4.1 host translation with scalar equivalence, two-level page table & 64-entry associative TLB with structured faults. |
| **Phase 14** — Real OVIR-GPU Shader Translation | **COMPLETED** | Integration + Host JIT | Real SPIR-V bitstream parser & header validator, typed architecture-neutral GPU IR, constant folding & dead code elimination passes, GLSL 330 Core emitter, operation reconstruction (matrix-vector decomposition), driver compilation verification. |
| **Phase 15** — Cross-Platform Native Backends & GPU Integration | **COMPLETED** | Integration + Host JIT | Unified platform interface (`core/platform/`), macOS Catalina (MacBookPro9,1 dual-GPU), Linux (Mesa/Vulkan), and Windows (WDDM/D3D11) adapters, runtime CPU/GPU capability discovery, 9-stage shader execution lifecycle, OVResolver hardware integration. |
| **Phase 15.1** — Real Native Hardware Discovery & Platform Detection | **COMPLETED** | Integration + Host Discovery | Authentic OS kernel discovery via `uname()`, real RAM via `/proc/meminfo` & `sysconf`, CPUID + XGETBV OS vector enablement verification (`XCR0[2:1] == 0x6`), sysfs `/sys/class/drm/` multi-GPU enumeration with PCI vendor mapping, intelligent discrete GPU VRAM priority ranking, and explicit mock profile isolation (`MacBookPro9,1`). |
| **Subsequent Development Phases** (Phase 16+) | **PLANNED** | Unit + Integration + Emulation | OS-scale system emulation, multi-threaded CPU translation, extended shader backends, and pipeline optimizations. |
| **Final Phase — External USB Hardware Validation** | **PENDING ALL DEV PHASES** | Physical Hardware (Tier 5) | Comprehensive physical testing on MacBookPro9,1 strictly using dedicated external USB drive. Internal macOS installation remains 100% untouched. |

---

## 2. Testing & Hardware Validation Policy Summary

* **Target Hardware**: MacBook Pro (15-inch, Mid-2012, `MacBookPro9,1`, Core i7-3615QM, Intel HD 4000 + NVIDIA GT 650M Kepler, 8 GB RAM, macOS Catalina 10.15.8 Build 19H2036).
* **Strict Non-Destructive Invariant**: All physical testing on the MacBook is postponed until all development phases are complete. The internal storage (`disk0`, `disk0s1`, `disk0s2`) is strictly read-only and must never be modified, repartitioned, or written to.
* **External USB Isolation**: Physical testing executes exclusively via an independent external USB drive (`/dev/disk2` or higher) prepared with non-destructive image validation.
* **Verification Tier Rigor**: Subsystems passing automated unit and integration tests are strictly classified as emulated/host-verified; they must never be represented as physical hardware validated until executed in the Final Phase milestone.
* **Canonical Policy**: See [DEVELOPMENT_AND_VALIDATION_POLICY.md](./DEVELOPMENT_AND_VALIDATION_POLICY.md) for full compliance specifications.

---

## 3. Automated Test Suite Metrics (Current Implementation)

* **Phase 9.1 Boot Handoff**: 31 / 31 C assertions passing
* **Phase 10 Unified Pipeline**: 56 / 56 C assertions passing
* **Phase 11 Executable Memory & Runtime**: 70 / 70 C assertions passing
* **Phase 12 Real OVIR-CPU Workloads**: 62 / 62 C assertions passing
* **Phase 13 DBT, NEON & Guest MMU**: 136 / 136 C assertions passing
* **Phase 14 Real OVIR-GPU Shader Translation**: 36 / 36 C assertions passing
* **Phase 15 Cross-Platform Backends & GPU Integration**: 42 / 42 C assertions passing
* **Phase 15.1 Real Hardware Discovery**: 22 / 22 C assertions passing
* **Total C Assertions**: **455 / 455 passing (100% SUCCESS)**
* **Node Test Suites**: 9 suites, 58 / 58 individual tests passing
* **TypeScript / Dev Server**: Zero compile or lint errors (`tsc --noEmit`)
