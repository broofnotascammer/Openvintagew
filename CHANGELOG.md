# OpenVintage Changelog

All notable changes to the OpenVintage project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to Semantic Versioning.

---

## [0.2.0] - 2026-09-06
### Added
- **Native EDK II Platform Package (`OpenVintagePkg`)**:
  - `OpenVintagePkg.dec`: Package declaration with custom protocol GUIDs and token space PCDs.
  - `OpenVintagePkg.dsc`: Platform description configuring library mappings, PCDs, and component compilation targets for X64 under GCC5.
  - `OpenVintagePkg.fdf`: Flash definition file establishing a 4MB SPI Flash device image (`OPENVINTAGE.fd`) and DXE Firmware Volume (`OPENVINTAGE_DXEFV.Fv`).
- **OpenVintage UEFI Boot Application (`OpenVintageBootApp.efi`)**:
  - Entry point `UefiMain` written in standard C conforming to UEFI 2.70 specification.
  - OpenVintage diagnostic startup banner and runtime initialization.
  - CPUID instruction decoder detecting silicon architectural profiles (Core 2, Nehalem/Westmere, Sandy Bridge, Ivy Bridge, Haswell/Broadwell) and SIMD features (SSE4.1, SSE4.2, AVX, AVX2, AES-NI).
  - UEFI memory map inspection calculating total physical RAM and available conventional pages.
  - Handle buffer enumeration discovering GOP framebuffers (resolution and base MMIO address) and scanning PCI bus devices.
  - Clean exit sequence returning `EFI_SUCCESS` to UEFI Boot Services.
- **Hardware Abstraction Layer DXE Driver (`OpenVintageHalDxe.efi`)**:
  - Installs `OPEN_VINTAGE_HAL_PROTOCOL` (`gOpenVintageHalProtocolGuid`) for use by DXE drivers and bootloaders.
  - Implements GPU discovery parsing PCI configuration space for Display Controllers (Intel HD, Nvidia GeForce, AMD Radeon).
- **Core Libraries**:
  - `OpenVintageCoreLib`: Modular routines for platform inspection, CPUID queries, and memory descriptor walks.
  - `OpenVintageLogLib`: Lightweight UEFI serial and console logging subsystem.
- **Automation Scripts**:
  - `scripts/build_firmware.sh`: End-to-end compilation script using EDK II BaseTools and GCC5.
  - `scripts/test_qemu.sh`: Automated QEMU test harness formatting a FAT32 ESP virtual disk and running headless boot verification.
- **Verified Binary Deliverables**:
  - `bin/OpenVintageBootApp.efi` (PE32+ executable, ~13 KB)
  - `bin/OPENVINTAGE.fd` (Flash Device Image, 4.0 MB)
  - `OpenVintagePkg/Firmware/OPENVINTAGE_DXEFV.Fv` (Firmware Volume, 4.0 MB)
  - `OpenVintagePkg/Drivers/OpenVintageHalDxe/OpenVintageHalDxe.efi` (DXE Driver, ~8.2 KB)

### Changed
- Transitioned project from conceptual UEFI design to an implemented, compiled, and virtualized firmware package verified in QEMU.
- Updated documentation (`README.md`, `ARCHITECTURE.md`, `PROJECT_STATUS.md`, `CHANGELOG.md`) reflecting verified build instructions, binary artifacts, and QEMU test traces.

---

## [0.1.0] - Initial Architecture Specification
### Added
- Architectural definitions for OVCore, OVResolver, OVScheduler, OVIR-GPU, and OVIR-CPU.
- Hardware capability database schema for legacy Intel Macs (2006–2015).
