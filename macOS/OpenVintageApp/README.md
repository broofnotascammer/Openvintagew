# OpenVintage native macOS application

This directory is the real macOS application surface for OpenVintage. It is intentionally separate from the Vite/React browser preview.

## Architecture

```text
OpenVintage.app (SwiftUI + AppKit)
        │
        ├── Darwin hardware adapter (sysctl + IOKit)
        │
        └── Application boundary
                │
                └── OpenVintage Core / HAL / Resolver / Security
                        │
                        └── Boot configuration + deployment
```

The app is read-only during hardware discovery. It must never let a UI action directly write EFI/NVRAM, replace boot files, patch a system volume, or install an integration without going through the existing safe deployment lifecycle and explicit user approval.

## Visual direction

The native UI uses AppKit `NSVisualEffectView` materials instead of simulated opaque CSS panels. This gives the application real macOS translucency, blur, depth, and system appearance behaviour while retaining readable content.

## Compatibility

The initial native shell targets macOS 10.15+ for Intel and builds an arm64 slice for macOS 11+. The CI build produces a universal application bundle.

The existing `src/` Vite application remains useful as a browser-based design/development preview. It is not presented as the privileged/native OpenVintage application.
