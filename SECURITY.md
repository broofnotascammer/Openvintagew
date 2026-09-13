# Security Policy

## Scope

OpenVintage includes pre-boot, UEFI, hardware-detection, firmware-image, and platform-integration components. Security reports involving boot behavior, memory safety, firmware images, privilege boundaries, or third-party integrations should be treated as high priority.

## Safety model

- Native hardware detection is read-only unless a future feature explicitly documents a separate write operation.
- Generated firmware images are intended for controlled virtualization/QEMU testing unless a release explicitly documents otherwise.
- No component should silently modify EFI variables, NVRAM, firmware, partitions, or SPI flash.
- Third-party integrations must be isolated and version-pinned.
- Simulation data must never be presented as native hardware data.

## Reporting

Do not publish sensitive vulnerability details before a fix is available. Use the repository's private security-reporting mechanism when available.

## Release requirements

A release should not be marked production-ready until the relevant build, sanitizer, regression, firmware-image, and third-party dependency checks pass.
