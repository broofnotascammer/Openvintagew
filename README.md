# OPENVINTAGE

> **Modular compatibility and performance platform originally designed around legacy Intel Mac hardware.**

---

## 1. Executive Summary

**OpenVintage** is an experimental systems ecosystem engineered to extend the functional lifespan and performance envelope of legacy hardware, with primary initial focus on **legacy Intel Mac systems** (2006–2015 era architectures: Core 2 Duo, Nehalem, Sandy Bridge, Ivy Bridge, Haswell, Broadwell, Skylake) and designed with portability across bare-metal firmware, UEFI, Linux, macOS, and Windows.

OpenVintage now includes a full native **EDK II Platform Package (`OpenVintagePkg`)** that provides:
1. A native X64 UEFI Bootloader and Diagnostic Application (`OpenVintageBootApp.efi`).
2. An extensible Hardware Abstraction Layer (HAL) DXE Driver (`OpenVintageHalDxe.efi`).
3. Core platform hardware detection and telemetry libraries (`OpenVintageCoreLib`, `OpenVintageLogLib`).
4. Full flash device image (`OPENVINTAGE.fd`) and firmware volume (`OPENVINTAGE_DXEFV.Fv`) generation.

```
                    OPENVINTAGE ECOSYSTEM ARCHITECTURE

                                OpenVintage
                                     │
              ┌──────────────────────┼──────────────────────┐
              │                      │                      │
       OpenVintagePkg            OVResolver            OVScheduler
       (EDK II / UEFI)      (Intelligent Routing)  (Resource Manager)
              │                      │                      │
       ┌──────┴──────┐               │               ┌──────┴──────┐
       │             │               │               │             │
   Firmware      Bootloader       OVIR-GPU        CPU Cores     Thermal /
 (OPENVINTAGE.fd) (.EFI App)    (Graphics IR)      Affinity     RAM Budgets
```

---

## 2. EDK II Platform Package (`OpenVintagePkg`)

The OpenVintage platform firmware is organized as a standard EDK II package:

```
OpenVintagePkg/
├── OpenVintagePkg.dec              # Package declaration & public GUID/PCD definitions
├── OpenVintagePkg.dsc              # Platform description file (X64 GCC5 build configuration)
├── OpenVintagePkg.fdf              # Flash definition file (FV and FD layout mappings)
├── Include/
│   ├── Library/
│   │   ├── OpenVintageCoreLib.h    # Core platform, memory map, CPUID, and device APIs
│   │   └── OpenVintageLogLib.h     # Subsystem logging and diagnostics API
│   └── Protocol/
│       ├── OpenVintageHal.h        # HAL Protocol (CPU/GPU capabilities interface)
│       └── OpenVintagePlatform.h   # Platform status and information protocol
├── Library/
│   ├── OpenVintageCoreLib/         # Core hardware detection implementation
│   │   ├── OpenVintageCoreLib.inf
│   │   └── OpenVintageCoreLib.c
│   └── OpenVintageLogLib/          # Lightweight UEFI console/serial logging
│       ├── OpenVintageLogLib.inf
│       └── OpenVintageLogLib.c
├── Drivers/
│   └── OpenVintageHalDxe/          # HAL DXE Driver (installs gOpenVintageHalProtocolGuid)
│       ├── OpenVintageHalDxe.inf
│       ├── OpenVintageHalDxe.c
│       └── OpenVintageHalDxe.efi   # Compiled DXE Driver binary
├── OpenVintageBootApp/             # Native X64 UEFI Boot Application
│   ├── OpenVintageBootApp.inf
│   ├── OpenVintageBootApp.c
│   └── OpenVintageBootApp.efi      # Compiled UEFI executable binary
├── Platform/                       # Platform-specific definitions and board profiles
└── Firmware/                       # Generated firmware image and volumes
    ├── OPENVINTAGE.fd              # 4.0 MB Flash Device Image
    └── OPENVINTAGE_DXEFV.Fv        # 4.0 MB Firmware Volume
```

---

## 3. Compiled Binaries & Firmware Artifacts

All production binaries are built from source and verified using standard PE32+ and binary inspection tools:

| Artifact | Type | Size | Description |
| :--- | :--- | :--- | :--- |
| `bin/OpenVintageBootApp.efi` | PE32+ x86-64 EFI App | ~13 KB | Native X64 UEFI application entry point |
| `bin/OPENVINTAGE.fd` | Binary Flash Image | 4.0 MB | Complete SPI Flash device image |
| `OpenVintagePkg/Firmware/OPENVINTAGE_DXEFV.Fv` | PI Firmware Volume | 4.0 MB | DXE Firmware Volume containing HAL and BootApp |
| `OpenVintagePkg/Drivers/OpenVintageHalDxe/OpenVintageHalDxe.efi` | PE32+ x86-64 DXE Driver | ~8.2 KB | Hardware abstraction layer boot services driver |

---

## 4. Build Prerequisites & Compilation

### Prerequisites
- Linux (Ubuntu/Debian or compatible)
- GCC 11+ / Clang with `gcc-ar`, `objcopy`
- NASM assembler (2.15+)
- Python 3.10+
- `uuid-dev`, `build-essential`, `dosfstools`, `mtools`
- QEMU 7.0+ (`qemu-system-x86_64`) with OVMF firmware (`/usr/share/ovmf/OVMF.fd`)
- EDK II workspace (`tianocore/edk2` at `/edk2` or local path)

### Automated Build Pipeline
To compile `OpenVintagePkg`, build all libraries, drivers, the boot app, and generate the firmware volume:

```bash
./scripts/build_firmware.sh
```

### Manual Build via EDK II
```bash
cd /edk2
source edksetup.sh BaseTools
build -p OpenVintagePkg/OpenVintagePkg.dsc -a X64 -t GCC5 -b DEBUG
```

---

## 5. QEMU Virtualized Test Procedure

OpenVintage includes an automated test harness that configures a virtual UEFI System Partition (ESP), formats a FAT32 filesystem with `EFI/BOOT/BOOTX64.EFI`, boots under QEMU with OVMF firmware, and captures serial telemetry.

### Running the Test
```bash
./scripts/test_qemu.sh [CPU_MODEL] [TIMEOUT_SECONDS]
```

Example for testing with an emulated Intel Haswell CPU:
```bash
./scripts/test_qemu.sh Haswell 15
```

### Verified Boot Sequence Trace
```
Power-on (QEMU 7.2 x86_64)
  ↓
OVMF UEFI Firmware Initialization (EDK II v1.0, UEFI 2.70)
  ↓
BdsDxe: loading Boot0002 "UEFI QEMU HARDDISK"
  ↓
OpenVintageBootApp.efi (Entry Point: UefiMain)
  ↓
[1] Banner Display & Runtime / Logging Init (ACTIVE)
[2] Firmware & Platform ID (Vendor: EDK II, Rev: 0x00010000)
[3] CPU Detection: Intel Core Processor (Haswell), Family 0x06, Model 0x3C
    Instruction Sets: SSE4.1 [YES]  SSE4.2 [YES]  AES-NI [YES]  AVX [YES]  AVX2 [YES]
[4] Physical Memory Topology: 2047 MB Total System RAM (2007 MB Free)
[5] UEFI Protocol & Device Enumeration:
    Active Handles: 183 | PCI Devices: 5 | Block I/O Devices: 2
    GOP Framebuffer: 1280x800 @ 0x80000000 (Size: 4000 KB)
[6] Exit Status: EFI_SUCCESS (0x00000000)
```

---

## 6. Engineering Discipline

1. **Deterministic Verification**: No component is marked complete without actual binary artifacts and passing execution tests.
2. **Modular Decoupling**: Subsystems adhere to clear PI/UEFI and POSIX boundaries with zero circular dependencies.
3. **No Synthetic Claims**: Real CPUID instruction queries, real memory descriptor walks, and genuine PCI config space interrogation.
4. **Safety Notice**: Generated firmware images are targeted for QEMU and virtualization testing; flashing onto physical Mac SPI flash ROMs requires hardware programming rigs and safety verification.
