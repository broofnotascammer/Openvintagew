# OpenVintage Pre-Boot Simulator

A native GTK4 application for Zorin OS that simulates Phase 2 pre-boot initialization sequence.

## Features

- Real hardware detection (CPU, GPU, RAM, PCI devices)
- Phase 2 subsystem execution (OvCore, OvHardware, OvResolver, OvScheduler, OvMemory, OvLogger, OvConfig, OvModule)
- Visual boot sequence simulation
- Hardware capability analysis
- Export diagnostics and reports

## Requirements

- Zorin OS (Ubuntu 22.04+)
- GTK 4.0+
- GLib 2.70+
- libpci-dev
- gcc/make

## Installation

```bash
cd OpenVintagePrebootSimulator
make
sudo make install
```

## Usage

```bash
# Run from applications menu or command line
openvintage-preboot-simulator
```

## Architecture

- `src/` — Source files
- `include/` — Header files
- `ui/` — GTK resource files
- `Makefile` — Build configuration

## License

See LICENSE file
