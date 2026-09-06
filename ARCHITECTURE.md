# OPENVINTAGE ARCHITECTURAL SPECIFICATION

```
================================================================================
OPENVINTAGE SYSTEMS ARCHITECTURE
Revision: 2.1.0
Classification: Systems Engineering Specification
Status: Active Implementation & Verified EDK II Firmware Pipeline
================================================================================
```

---

## 1. Architectural Overview

OpenVintage rejects the traditional $M \times N$ matrix of direct API-to-API bridges:

```
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
```

The system is decomposed into three primary control layers, a firmware platform foundation, and two translation hubs:

```
                           OPENVINTAGE ECOSYSTEM
                                     │
              ┌──────────────────────┼──────────────────────┐
              │                      │                      │
       OpenVintagePkg            OVResolver            OVScheduler
     (Firmware & UEFI)      (Intelligent Routing)  (Resource Manager)
              │                      │                      │
       ┌──────┴──────┐               │               ┌──────┴──────┐
       │             │               │               │             │
   Firmware      Bootloader       OVIR-GPU        CPU Cores     Thermal /
 (OPENVINTAGE.fd) (.EFI App)    (Graphics IR)      Affinity     RAM Budgets
              │                      │
       ┌──────┴──────┐               │
       │             │         Graphics APIs
   HAL DXE       Core Libs      (Metal / VK /
  (Protocols)   (CPUID / Mem)   GL / DirectX)
```

---

## 2. Firmware & Bootloader Subsystems (`OpenVintagePkg`)

### 2.1 EDK II Package Structure
`OpenVintagePkg` is fully integrated into the Tianocore EDK II build environment:

```
OpenVintagePkg/
├── OpenVintagePkg.dec              # Public interfaces, GUIDs, token space
├── OpenVintagePkg.dsc              # Platform description (GCC5 / X64)
├── OpenVintagePkg.fdf              # Flash layout (FD and FV definitions)
├── Include/
│   ├── Library/
│   │   ├── OpenVintageCoreLib.h    # Core platform detection API
│   │   └── OpenVintageLogLib.h     # Subsystem logging API
│   └── Protocol/
│       ├── OpenVintageHal.h        # OPEN_VINTAGE_HAL_PROTOCOL
│       └── OpenVintagePlatform.h   # OPEN_VINTAGE_PLATFORM_PROTOCOL
├── Library/
│   ├── OpenVintageCoreLib/         # CPUID detection & memory map analysis
│   └── OpenVintageLogLib/          # Low-overhead serial/screen log driver
├── Drivers/
│   └── OpenVintageHalDxe/          # DXE Service Protocol Provider
├── OpenVintageBootApp/             # Native X64 UEFI Entry Point
└── Firmware/                       # Generated flash image and volumes
    ├── OPENVINTAGE.fd              # 4.0 MB Flash Device Image
    └── OPENVINTAGE_DXEFV.Fv        # 4.0 MB DXE Firmware Volume
```

### 2.2 OpenVintage Protocols
- **`OPEN_VINTAGE_HAL_PROTOCOL`** (`gOpenVintageHalProtocolGuid`):
  Installed during DXE phase by `OpenVintageHalDxe`. Exposes hardware abstraction calls:
  - `GetCpuCapabilities`: Evaluates CPU family, model, stepping, and instruction features (SSE4.1, SSE4.2, AVX, AVX2, AES-NI).
  - `GetGpuCapabilities`: Interrogates PCI configuration space for Display Controllers (PCI class 0x03) and identifies Intel HD, Nvidia GeForce/Quadro, and AMD Radeon adapters.
- **`OPEN_VINTAGE_PLATFORM_PROTOCOL`** (`gOpenVintagePlatformProtocolGuid`):
  Exposes platform revision, vendor information, and firmware health status.

### 2.3 Firmware Volume & Flash Device Layout
The firmware definition file (`OpenVintagePkg.fdf`) defines a 4MB memory-mapped flash layout:

```
+-------------------------------------------------------------------+
| Flash Device: OPENVINTAGE.fd (Base: 0xFFC00000, Size: 0x400000)  |
+-------------------------------------------------------------------+
| 0x00000000 - 0x00400000 : OPENVINTAGE_DXEFV (4096 KB)             |
|   ├── OpenVintageHalDxe.ffs (DXE Driver + Depex + Version)        |
|   ├── OpenVintageBootApp.ffs (UEFI Application + UI Section)      |
|   └── Free Flash Space (3978 KB available for additional modules) |
+-------------------------------------------------------------------+
```

---

## 3. Native Boot Application (`OpenVintageBootApp.efi`)

The boot application provides early hardware telemetry and platform verification:
1. **Banner & Logging**: Initializes serial port and UEFI console output.
2. **Firmware Identification**: Queries `gST->FirmwareVendor`, revision, and UEFI specification version.
3. **CPU Architecture Analysis**:
   - Executes standard and extended CPUID functions (0x00000001, 0x00000007, 0x80000002-0x80000004).
   - Maps silicon family to known hardware profiles (Core 2, Nehalem/Westmere, Sandy Bridge, Ivy Bridge, Haswell/Broadwell).
   - Verifies SIMD vector extensions: SSE4.1, SSE4.2, AES-NI, AVX, AVX2.
4. **Memory Topology Walk**:
   - Invokes `gBS->GetMemoryMap` to enumerate memory descriptor regions.
   - Computes total physical RAM and available conventional memory.
5. **Device & Protocol Discovery**:
   - Enumerates active handles via `gBS->LocateHandleBuffer`.
   - Discovers Graphics Output Protocol (GOP) framebuffers, resolution, and base MMIO address.
   - Walks PCI bus devices and identifies bridges, mass storage controllers, and display adapters.
6. **Graceful Exit**: Returns `EFI_SUCCESS` to UEFI Boot Services.

---

## 4. Workload Resolution & Scheduling

```
                      Incoming Workload Request
                                 │
                                 ▼
                         [ OVResolver ]
                                 │
            ┌────────────────────┼────────────────────┐
            ▼                    ▼                    ▼
     Inspect Hardware     Query Capability     Check Multi-Tier
     Topology (CPU/GPU)       Database          Cache Index
            │                    │                    │
            └────────────────────┼────────────────────┘
                                 │
                                 ▼
                     Execution Path Decision
         ┌──────────────┬──────────────┬──────────────┬──────────────┐
         ▼              ▼              ▼              ▼              ▼
     [Native]     [Translation]  [Simplification] [Cache Hit]   [CPU Fallback /
   (Hardware has  (Transform via  (Clamp features   (Reuse pre-    Unsupported]
   native support)  OVIR to HAL)   for stability)  compiled blob)
```

### Decision Rules:
1. **Pass-through / Native**: Workload matches target GPU hardware capabilities.
2. **Intermediate Translation**: Features exceeding hardware are transformed through OVIR-GPU command DAGs.
3. **Feature Simplification**: Non-essential features (e.g., MSAA 8x, 4K render targets) are clamped to fit legacy VRAM constraints.
4. **Cache Re-use**: Lookups by composite hash `SHA256(Source + Driver + Hardware_UUID)`.
5. **CPU Fallback**: Route unsupported compute or geometry shaders to SIMD CPU worker threads.

---

## 5. QEMU Virtualized Test Architecture

Testing is executed in an automated, headless virtual machine environment:
- **Host Test Harness**: `scripts/test_qemu.sh`
- **Firmware Base**: Tianocore OVMF X64 (`/usr/share/ovmf/OVMF.fd`)
- **Virtual Disk**: 64MB FAT32 ESP disk containing `EFI/BOOT/BOOTX64.EFI` and `startup.nsh`
- **CPU Profiles Tested**: Intel Haswell, QEMU Virtual CPU
- **Telemetry Channel**: ISA debugcon / Serial port redirection to file
- **Verification Rule**: String validation against `OpenVintage Boot App Phase 1/2 Check: PASS` and `EFI_SUCCESS`.
