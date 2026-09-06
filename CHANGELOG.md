# OpenVintage Changelog

All notable changes to the OpenVintage project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to Semantic Versioning.

---

## [0.2.0] - 2026-09-06
### Added
- Comprehensive architectural documentation (`ARCHITECTURE.md`) outlining the multi-tier OpenVintage ecosystem: OVCore, OVResolver, OVScheduler, OVIR-GPU, and OVIR-CPU.
- Project status tracker (`PROJECT_STATUS.md`) documenting Phase 1 achievements, active Phase 2 milestones, and known hardware limitations.
- Master project overview (`README.md`) defining the engineering philosophy, hardware targets (Intel Macs 2006-2015), and SolitaryOS roadmap boundaries.
- Interactive Systems Control & Verification Dashboard in TypeScript/React for testing and visualizing hardware profiles, OVResolver path decisions, and OVScheduler performance dynamics.
- Modular TypeScript core domain modules (`/src/core/hardware`, `/src/core/resolver`, `/src/core/scheduler`, `/src/core/ovir`) modeling the verified hardware specifications and routing algorithms.

### Changed
- Transitioned project lifecycle status from Phase 1 (UEFI Applet Proof of Concept) to Phase 2 (Modular Architecture and Subsystem Foundations).

---

## [0.1.0] - Phase 1 Complete
### Added
- Initial OpenVintage X64 EFI application using EDK II framework.
- GCC toolchain and cross-compilation pipeline configuration for x86_64 targets.
- Lightweight EFI Logger protocol for serial and screen diagnostics.
- PCI device enumeration and configuration inspection routines.
- Block I/O protocol discovery and mass storage partition detection.
- Device path parser and textual report generation.
- Media and file system validation routines.
- Graceful application exit sequence returning `EFI_SUCCESS`.
- QEMU and OVMF UEFI virtualized integration test harness.
