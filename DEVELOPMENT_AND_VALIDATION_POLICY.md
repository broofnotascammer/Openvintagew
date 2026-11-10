# OPENVINTAGE — DEVELOPMENT AND FINAL USB VALIDATION POLICY

## 1. Target Development Machine
The primary development reference and eventual physical validation target is:
* **Model**: MacBook Pro 15-inch, Mid-2012
* **Model Identifier**: `MacBookPro9,1`
* **Processor**: Intel Core i7-3615QM (4 cores / 8 threads, Ivy Bridge, 2.3 GHz base / 3.3 GHz boost)
* **Integrated Graphics**: Intel HD Graphics 4000
* **Discrete Graphics**: NVIDIA GeForce GT 650M (Kepler architecture, 512MB/1GB GDDR5)
* **Memory**: 8 GB DDR3 RAM
* **Primary Storage**: Internal HDD (`/dev/disk0`, APFS volume container on partition 2, ESP on partition 1)
* **Operating System**: macOS Catalina 10.15.8 (Build 19H2036)

---

## 2. Mandatory Testing Policy
**OpenVintage must NOT undergo experimental physical-machine testing on the internal macOS installation during development.**

1. **Physical Testing Deferred**: All real hardware testing on the physical MacBookPro9,1 machine is strictly postponed until every planned development phase has been completed, peer-reviewed, and verified in automated testing.
2. **Dedicated External USB Isolation**: The final physical validation environment must be prepared exclusively on a dedicated external USB drive.
3. **Internal Storage Inviolate**:
   * The USB environment must be completely independent of the internal macOS installation.
   * **DO NOT** modify, repartition, format, overwrite, or install experimental OpenVintage components onto the internal disk (`/dev/disk0`, `disk0s1`, APFS container `disk0s2`).
   * Never assume that the internal disk is disposable, temporary, or available for development testing.
4. **No Destructive Operations**: No destructive disk-writing operations are permitted during intermediate development phases. Physical deployment of `OPENVINTAGE.fd` or internal flashing remains strictly prohibited.

---

## 3. Development Testing Rigor Before Final Validation
Until the project formally reaches the final hardware-validation milestone, all verification must rely exclusively on non-destructive and virtualized methods:
* **Unit Tests**: Targeted assertion suites for individual modules (`tests/*.c`).
* **Integration Tests**: Deterministic end-to-end pipeline executions (`tests/*.test.js`).
* **Architecture-Specific Decoder Tests**: Exhaustive bitstream coverage for ARM64 and NEON instructions.
* **IR Validation & Optimization Tests**: Invariant checking for neutral IR, constant folding, and dead code elimination.
* **Differential Testing & Scalar Equivalence**: Numerical parity verification comparing translated JIT machine code against reference architectural simulators.
* **Virtual Machines & Emulated Environments**: QEMU UEFI firmware execution harnesses (`OVMF.fd`).
* **Static Analysis & Memory-Safety Checks**: Address Sanitizer (ASan), `valgrind`, strict $W \oplus X$ memory protection gating, and linter validation (`tsc --noEmit`).
* **Synthetic Capability Profiles**: Mock hardware interfaces testing hardware boundary conditions, resource exhaustion, and unsupported feature traps.

### Explicit 5-Tier Verification Hierarchy
Every status report, document, test log, and UI display must strictly distinguish between:
1. **Tier 1 — Subsystem Compiles**: Clean build with zero compilation errors.
2. **Tier 2 — Subsystem Passes Unit Tests**: Validated in isolation via synthetic assertions.
3. **Tier 3 — Subsystem Passes Integration Tests**: End-to-end pipeline verification across multiple integrated modules.
4. **Tier 4 — Subsystem Validated Against Actual Hardware Backend**: Verified running natively on host development CPU/GPU execution backends (e.g., host x86-64 SSE4.1 execution, host Metal API).
5. **Tier 5 — Subsystem Tested on Physical MacBook**: Booted and verified on the physical MacBookPro9,1 hardware from external USB.

> **CRITICAL RULE**: Simulated or mocked hardware results must **NEVER** be labeled as physical hardware validation.

---

## 4. Final USB Validation Milestone Specification
The definitive physical testing milestone is scheduled as:

### **Final Phase — External USB Hardware Validation**

This milestone executes only after all preceding implementation phases (Phases 9.1 through Phase 13 and any subsequent development phases) are fully complete and audited.

### A. Build Preparation & Image Verification
* Comprehensive review of all preceding development phases.
* Execution and passing of 100% of the automated test matrix (C suites and Node suites).
* Zero open critical defects or memory leaks.
* Verification of build reproducibility.
* Generation of a standalone test image (`.img`/`.iso`) verified against virtual disk loops before touching physical flash media.

### B. USB Environment & Safety Protocol
* Isolated deployment to a dedicated, removable external USB drive (`/dev/disk2` or higher).
* Absolute isolation of the internal macOS Catalina installation.
* Automated pre-flight target verification:
  * Explicit identification of external bus (`kDADiskDescriptionDeviceProtocolKey == "USB"`).
  * Explicit confirmation prompt displaying target device node, vendor string, model name, and size.
  * Rejection of any internal disk identifier (`disk0`, `nvme`, `Internal`).
* Comprehensive diagnostic logging to an isolated partition on the USB stick.
* Non-destructive rollback procedure: simply unplug the USB drive to return immediately to standard macOS Catalina boot flow.

### C. Scope of Physical Hardware Validation
Only genuinely implemented subsystems are validated on physical hardware:
1. UEFI startup, volume scanning, and boot handoff (`OpenVintageBootApp.efi`).
2. CPU capability detection and hardware feature discovery.
3. OVIR-CPU instruction decoding and translation.
4. Dynamic binary translation (DBT) trace cache and hot-loop execution.
5. Register allocation, $XZR$ zero register invariants, and condition flags.
6. ARM64 NEON vector translation (SSE4.1 mapping on Ivy Bridge host).
7. Guest virtual memory, two-level page tables, and TLB hit/miss/fault handling.
8. OVIR-GPU capability detection (Intel HD 4000 + NVIDIA GT 650M Kepler).
9. Shader translation and backend execution (where implemented).
10. GPU reconstruction and CPU software fallback paths.
11. Runtime memory protections ($W \oplus X$ enforcement).
12. Multi-level cache invalidation (hash, range, generation).
13. Hardware telemetry, microsecond timing, and diagnostic logging.

> **RULE**: No feature, API, GPU route, or instruction set may be claimed as supported unless verified according to this protocol.

### D. Standardized Test Recording Schema
For every physical test executed during the final validation milestone, the test record must log:
* `test_id`: Unique hierarchical identifier (e.g. `PHYS-BOOT-001`, `PHYS-MMU-004`).
* `hardware_config`: Machine model, CPU stepping, GPU active, RAM, EFI ROM revision.
* `subsystem_tested`: Specific subsystem module under test.
* `test_procedure`: Step-by-step physical reproduction procedure.
* `expected_result`: Concrete architectural success criteria.
* `actual_result`: Observed hardware behavior.
* `verdict`: `PASS` | `FAIL` | `PARTIAL` | `NOT TESTED`.
* `performance_observations`: Monotonic microsecond latency, frame times, or cycle count.
* `limitations`: Hardware-specific quirks or thermal throttling observations.
* `log_payload`: Captured serial output or file telemetry log.

---

## 5. USB Tooling Safety Requirements
Any future tool or script designed to prepare or write external USB validation media must comply with these conservative safety rules:
1. **Explicit Identification**: Require the user or caller to explicitly specify the destination target device.
2. **Removable / External Guard**: Query OS storage subsystem to ensure the target device is marked external/removable.
3. **Target Confirmation**: Interactively display device model, vendor, capacity, and partition table prior to any write.
4. **Ambiguity Rejection**: Refuse any target that matches ambiguous patterns, internal disk identifiers (`disk0`), or root mount points.
5. **Virtual Testing First**: The final USB writer itself must be verified against virtual loopback disk images before physical media deployment.
6. **No Current Destructive Operations**: No disk-writing tools or scripts are to be executed during intermediate development phases.
