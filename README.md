# OPENVINTAGE

> **Hardware-aware boot management, compatibility, and performance platform for legacy and modern systems.**

---

## 1. Executive Summary

**OpenVintage** is an experimental systems ecosystem engineered to extend the functional lifespan and configuration flexibility of legacy hardware, with primary initial focus on **legacy Intel Mac systems** (2006–2015 era architectures) and designed with portability across bare-metal firmware, UEFI, Linux, macOS, and Windows.

OpenVintage includes a native **EDK II Platform Package (`OpenVintagePkg`)** that provides:
1. A native X64 UEFI Bootloader and Diagnostic Application (`OpenVintageBootApp.efi`).
2. An extensible Hardware Abstraction Layer (HAL) DXE Driver (`OpenVintageHalDxe.efi`).
3. Core platform hardware detection and telemetry libraries.
4. Firmware image and firmware-volume generation for controlled virtualization/testing workflows.

```
                    OPENVINTAGE ECOSYSTEM

                         OpenVintage
                              │
          ┌───────────────────┼────────────────────┐
          │                   │                    │
   Native macOS App      Core / HAL / Resolver   UEFI / Boot
   SwiftUI + AppKit              │                 OpenVintagePkg
          │                ┌─────┴─────┐                │
          │             OVIR-CPU   OVIR-GPU             │
          │                │         │                 │
          └────────────── Application / Boot Plan ─────┘
```

---

## 2. EDK II Platform Package (`OpenVintagePkg`)

The OpenVintage platform firmware is organized as a standard EDK II package:

```
OpenVintagePkg/
├── OpenVintagePkg.dec
├── OpenVintagePkg.dsc
├── OpenVintagePkg.fdf
├── Include/
├── Core/
├── Memory/
├── Hardware/
├── Resolver/
├── Scheduler/
├── OvirGpu/
├── OvirCpu/
├── Drivers/OpenVintageHalDxe/
├── OpenVintageBootApp/
├── Tests/
└── Firmware/
```

---

## 3. Compiled Binaries & Firmware Artifacts

Production firmware artifacts are built from source and inspected using standard PE32+ and binary tooling. Exact artifact sizes are build-dependent and should be taken from the current CI run rather than treated as fixed constants.

Typical artifacts include:

| Artifact | Type | Description |
| :--- | :--- | :--- |
| `bin/OpenVintageBootApp.efi` | PE32+ x86-64 EFI App | Native X64 UEFI boot application |
| `bin/OvSelfTestApp.efi` | PE32+ x86-64 EFI App | Firmware/self-test diagnostics |
| `bin/OPENVINTAGE.fd` | Binary Flash Image | QEMU/testing firmware image |
| `OpenVintagePkg/Drivers/OpenVintageHalDxe/OpenVintageHalDxe.efi` | PE32+ x86-64 DXE Driver | Hardware abstraction layer |

---

## 4. Build Prerequisites & Compilation

### Firmware prerequisites
- Linux (Ubuntu/Debian or compatible)
- GCC/Clang toolchain and NASM
- Python 3.10+
- EDK II workspace
- QEMU + OVMF for virtualized boot testing

### Automated firmware build

```bash
./scripts/build_firmware.sh
```

### Manual EDK II build

```bash
cd /edk2
source edksetup.sh BaseTools
build -p OpenVintagePkg/OpenVintagePkg.dsc -a X64 -t GCC5 -b DEBUG
```

---

## 5. QEMU Virtualized Test Procedure

OpenVintage includes a UEFI/QEMU test harness for controlled firmware validation. Firmware images produced for this workflow are not intended to be flashed onto physical Mac SPI storage without separate hardware-specific verification.

```bash
./scripts/test_qemu.sh [CPU_MODEL] [TIMEOUT_SECONDS]
```

Example:

```bash
./scripts/test_qemu.sh Haswell 15
```

---

## 6. Unified Architecture

The runtime architecture is intentionally layered:

```
OpenVintage Boot / Firmware
          ↓
      Application
          ↓
     Core / HAL
          ↓
 Hardware Detection
          ↓
       OVResolver
          ↓
   ┌──────┼──────┐
 OVIR-CPU OVIR-GPU OVScheduler
   └──────┼──────┘
          ↓
 Compatibility / Performance / Boot Plan
          ↓
 Safe Deployment (only after explicit approval)
```

The resolver evaluates CPU architecture, GPU/API capabilities, memory limits, OS requirements, compatibility constraints, and available integrations. Performance profiles describe policy; they do not claim to create hardware capabilities that do not exist.

---

## 7. Native vs. Simulated Hardware

OpenVintage maintains a strict boundary between real hardware and simulated profiles:

```
                 Hardware Source
                       │
          ┌────────────┴────────────┐
          ▼                         ▼
       NATIVE                    SIMULATED
          │                         │
 Darwin / Linux APIs          Static Mac profiles
 sysctl / IOKit / sysfs       Multi-GPU topology
          │                         │
      Real host                Target model
```

Native mode must never silently substitute a simulated Mac. Simulated mode must be visibly labelled as simulated throughout the UI and diagnostics.

On macOS, the native backend uses Darwin facilities such as `sysctlbyname` and IOKit to discover machine identity, CPU/memory information, PCI devices, and graphics topology. Physical GPU inventory and active display routing are separate concepts.

---

## 8. Safe Deployment

Any operation that could alter boot configuration, EFI files, system configuration, or an integration follows the safety boundary:

```text
Discover → Simulate → Plan → Review → User Approval
→ Backup → Apply → Verify → Recovery
```

The UI is not permitted to bypass the security/deployment layer or perform privileged writes directly.

---

## 9. Native macOS Application (Phase 6.1)

OpenVintage now has a **real native macOS application surface** in:

```text
macOS/OpenVintageApp/
```

This is not the Vite website. It is a SwiftUI + AppKit macOS application bundle that talks to the Darwin layer through native APIs.

### Native app architecture

```text
OpenVintage.app
   │
   ├── SwiftUI application UI
   │      └── AppKit NSVisualEffectView materials
   │
   ├── Darwin hardware adapter
   │      ├── sysctl
   │      └── IOKit / PCI discovery
   │
   └── Application/Core boundary
          └── Core / HAL / Resolver / Security
```

The native UI deliberately uses real AppKit visual-effect materials instead of opaque CSS panels. This provides the translucent, blurred, layered appearance expected from a macOS application without making the interface look like plastic.

The initial application shell targets macOS 10.15+ on Intel and includes an arm64 build for macOS 11+. CI produces a universal `OpenVintage.app` artifact.

### Browser preview

The existing `src/` Vite/React interface is retained as a **browser-based design and development preview**. It is not the privileged OpenVintage application and cannot directly inspect Darwin hardware from a normal browser tab.

This distinction is intentional:

```text
Browser preview                     Native product
───────────────                     ──────────────
Vite + React                        SwiftUI + AppKit
Design / UX testing                 Real macOS application
Simulated state                      Darwin hardware APIs
No privileged access                 Core/HAL security boundary
```

---

## 10. CI Verification

The GitHub Actions pipeline now validates the two UI surfaces independently:

1. **Frontend job:** TypeScript typecheck, frontend tests, and Vite production build.
2. **macOS core job:** Intel Catalina-compatible native simulator/test runner and MBP9,1 compatibility simulation.
3. **macOS app job:** universal SwiftUI/AppKit application build, bundle validation, code-signature verification, and artifact packaging.

The feature branch `feature/preboot-simulator` is not part of the main push CI path; `main` is the integration branch.

---

## 11. Engineering Discipline

1. No component is marked complete without an executable verification path.
2. Native and simulated hardware states remain distinguishable.
3. No synthetic hardware capability is presented as native capability.
4. Privileged operations stay behind explicit security/deployment gates.
5. Firmware images are treated as controlled test artifacts unless independently validated for a specific physical platform.
