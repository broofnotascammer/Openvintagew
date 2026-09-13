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
       ┌──────┴──────┐        ┌──────┴──────┐        ┌──────┴──────┐
       │             │        │             │        │             │
   Firmware      Bootloader  OVIR-GPU   OVIR-CPU  CPU Cores    Thermal /
 (OPENVINTAGE.fd) (.EFI App) (Graphics)   (CPU)    Affinity    RAM Budgets
```

---

## 2. EDK II Platform Package (`OpenVintagePkg`)

The OpenVintage platform firmware is organized as a standard EDK II package:

```
OpenVintagePkg/
├── OpenVintagePkg.dec              # Package declaration & public GUID/PCD definitions
├── OpenVintagePkg.dsc              # Platform description file (X64 GCC build configuration)
├── OpenVintagePkg.fdf              # Flash definition file (FV and FD layout mappings)
├── Include/
│   ├── OpenVintage.h               # Master umbrella header
│   └── Library/
│       ├── OvCoreLib.h             # Core initialization & lifecycle management
│       ├── OvConfigLib.h           # Configuration profiles & feature flags
│       ├── OvMemoryLib.h           # Tagged memory allocation & leak auditing
│       ├── OvHardwareLib.h         # Multi-subsystem hardware discovery
│       ├── OvModuleLib.h           # Dynamic module registration & lifecycle
│       ├── OvResolverLib.h         # Capability matrix & intelligent dispatch
│       ├── OvSchedulerLib.h        # Cooperative priority queue scheduler
│       ├── OvLoggerLib.h           # Multi-level structured logger
│       ├── OvGpuCapabilityLib.h    # GPU capability & texture format support
│       ├── OvirGpuLib.h            # OVIR-GPU unified graphics IR
│       └── OvirCpuLib.h            # OVIR-CPU architecture translation IR
├── Core/                           # Core runtime, configuration, and modules
├── Memory/                         # Tagged memory pool management
├── Hardware/                       # CPU, GPU, memory, platform discovery
├── Resolver/                       # Hardware-aware capability resolution
├── Scheduler/                      # Priority task scheduling engine
├── OvirGpu/                        # OVIR-GPU graphics translation & adapters
├── OvirCpu/                        # OVIR-CPU binary translation & decoders
├── Drivers/
│   └── OpenVintageHalDxe/          # HAL DXE Driver
├── OpenVintageBootApp/             # Native X64 UEFI Boot Application
├── Tests/                          # Comprehensive self-test application (24 tests)
└── Firmware/                       # Generated firmware image and volumes
    ├── OPENVINTAGE.fd              # 4.0 MB Flash Device Image
    └── OPENVINTAGE_DXEFV.Fv        # 4.0 MB Firmware Volume
```

---

## 3. Compiled Binaries & Firmware Artifacts

All production binaries are built from source and verified using standard PE32+ and binary inspection tools:

| Artifact | Type | Size | Description |
| :--- | :--- | :--- | :--- |
| `bin/OpenVintageBootApp.efi` | PE32+ x86-64 EFI App | ~48 KB | Native X64 UEFI boot application entry point |
| `bin/OvSelfTestApp.efi` | PE32+ x86-64 EFI App | ~320 KB | 24-subsystem architectural self-test diagnostic suite |
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

## 6. Phase 5: Final Architecture & Optimization

Phase 5 establishes end-to-end integration of all OpenVintage systems into a unified execution pipeline:

```
                    FINAL ARCHITECTURE EXECUTION PIPELINE

                          OpenVintage Boot/Firmware
                                     ↓
                             OpenVintage Core
                                     ↓
                            Hardware Detection
                                     ↓
                                OVResolver
                                     ↓
                  ┌──────────────────┼──────────────────┐
                  │                  │                  │
               OVIR-CPU           OVIR-GPU         OVScheduler
                  │                  │                  │
             CPU Backend        GPU Backend     Resource Management
                  └──────────────────┼──────────────────┘
                                     ↓
                              Application/Game
```

### 6.1 Integrated Subsystems

- **Integrated OVResolver (`OvResolverLib`)**: Unifies multi-architecture CPU evaluation with GPU backend routing, memory bounds, thermal throttling, and cache hits to create complete execution plans.
- **Performance Subsystem (`OvPerfSystemLib`)**: Measurable performance telemetry capturing CPU load, memory usage, GPU VRAM, translation overhead, shader compilation time, cache hit rate, and frame timing.
- **Dynamic Resource Management (`OvResourceManagerLib`)**: Profile-based resource enforcement (`Balanced`, `Performance`, `MaxPerformance`, `BatteryLowPower`) with hardware capability checks.
- **Unified Multi-Tier Cache (`OvUnifiedCacheLib`)**: Integrates CPU translation, GPU shader, pipeline, and compatibility caches with reliable generational invalidation.
- **Compatibility Framework (`OvCompatibilityLib`)**: Empirically records application requirements, hardware limits, and silicon quirks (e.g. Intel Gen7 texture clamping) without fabricated claims.
- **Unified Diagnostics (`OvDiagnosticsLib`)**: System auditing of hardware, CPU, GPU, memory, APIs, caches, and limitations.
- **Reproducible Benchmarking (`OvBenchmarkLib`)**: Real hardware TSC cycle timing measuring arithmetic constant folding, dead code elimination, and translation cache retrieval.

---

## 7. Native vs. Simulated Hardware Architecture

OpenVintage enforces a strict architectural boundary between **Native Hardware Execution** and **Simulated Hardware Evaluation**:

```
                              Hardware Mode Selector
                                        │
                ┌───────────────────────┴───────────────────────┐
                ▼                                               ▼
         [ NATIVE MODE ]                                [ SIMULATED MODE ]
   Default on macOS & Linux                        Activated via --simulate <mac>
                │                                               │
   Physical OS Hardware Query                      Static Architectural Database
   - macOS: sysctlbyname, IOKit                    - 30+ verified Mac models
   - Linux: /proc, sysfs, libpci                   - Multi-GPU topologies (GMUX)
                │                                               │
   Active Profile: OV_HW_PROFILE_HOST              Active Profile: e.g. MBP91
   is_simulated = false                            is_simulated = true
   source = OV_HW_SOURCE_NATIVE                    source = OV_HW_SOURCE_SIMULATED
```

### Key Architectural Rules
1. **Zero Silent Fallback**: The native hardware backend interrogates actual physical hardware via OS platform interfaces. It **never** silently substitutes simulated profile data (e.g. `MacBookPro9,1`) when running in native mode.
2. **True macOS Detection**:
   - Machine Model: `sysctlbyname("hw.model")`
   - CPU Details: `machdep.cpu.brand_string`, `hw.physicalcpu`, `hw.logicalcpu`, `hw.cpufrequency`
   - Physical Memory: `hw.memsize`
   - GPU Discovery: `IOKit` (`IOAccelerator`, `IOPCIDevice`) probing vendor ID, device ID, and VRAM without hardcoding.
3. **Transparent Reporting**: Diagnostics (Text, JSON, HTML) and runtime logs explicitly output:
   - `hardware_mode`: `NATIVE` | `SIMULATED`
   - `hardware_source`: `NATIVE` | `SIMULATED`
   - `host_detected_model`: Real host machine identifier
   - `simulated_target_model`: Simulated target model when in simulated mode, or `N/A (Native Mode)`

---

## 8. Engineering Discipline

1. **Deterministic Verification**: No component is marked complete without actual binary artifacts and passing execution tests.
2. **Modular Decoupling**: Subsystems adhere to clear PI/UEFI and POSIX boundaries with zero circular dependencies.
3. **No Synthetic Claims**: Real CPUID instruction queries, real memory descriptor walks, and genuine PCI config space interrogation.
4. **Safety Notice**: Generated firmware images are targeted for QEMU and virtualization testing; flashing onto physical Mac SPI flash ROMs requires hardware programming rigs and safety verification.
