# OpenVintage Pre-Boot Firmware Subsystem

This directory contains pre-boot firmware components and boot applications built for OpenVintage:

- `OPENVINTAGE.fd`: Unified 4MB EDK II platform flash image for QEMU / OVMF and pre-boot initialization.
- `OpenVintageBootApp.efi`: Standalone 64-bit EFI boot application implementing the OpenVintage hardware discovery and boot-picker protocol.
- `OvSelfTestApp.efi`: Standalone EFI self-test application running pre-boot memory, HAL, and device enumeration tests.

## EDK II Source Integration
Source definitions for these firmware targets reside in `OpenVintagePkg/` and are built using the TianoCore EDK II toolchain (BSD-2-Clause-Patent license). See `THIRD_PARTY.md` for licensing compliance.
