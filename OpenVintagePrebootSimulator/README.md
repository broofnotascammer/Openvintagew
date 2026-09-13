# OpenVintage Pre-Boot Simulator

A cross-platform (Linux & macOS) pre-boot architecture simulator and empirical benchmarking environment.

## Features

- **Strict Native vs. Simulated Hardware Modes**:
  - **Native Mode (`--native`)**: Discovers host hardware (macOS sysctl/IOKit, Linux /proc/sysfs/libpci) without profile substitution.
  - **Simulated Mode (`--simulate <model>`)**: Loads standard legacy Mac hardware profiles (e.g., `MacBookPro9,1`, `MacPro5,1`, `iMac13,2`, `Macmini6,2`) for capability simulation and patchability evaluation.
- Subsystem execution across all phases: `ov_core`, `ov_hardware`, `ov_resolver`, `ov_cpu_engine` (OVIR-CPU), `ov_gpu_engine` (OVIR-GPU), `ov_resource_manager`, `ov_unified_cache`, `ov_compatibility`, `ov_benchmark`, and `ov_diagnostics`.
- Empirical hardware TSC cycle timing benchmarks measuring real optimization speedups.
- Export diagnostics and audit reports to Text, JSON, and HTML formats.

## Requirements

- **Linux**: Ubuntu / Debian / Zorin OS, GCC/Clang, `make`, `libpci-dev`, GTK4 (optional for GUI)
- **macOS**: macOS 10.13+ (High Sierra, Mojave, Catalina, Big Sur, Monterey, Ventura, Sonoma, Sequoia), Xcode Command Line Tools, Clang, `make`

## Building

```bash
cd OpenVintagePrebootSimulator
make -j4
```

This compiles:
- `bin/openvintage-preboot-simulator`: The simulator runtime
- `bin/test_runner`: Comprehensive automated test suite (161 tests)

## Usage

```bash
# Run in CLI mode with default native host detection
./bin/openvintage-preboot-simulator -c

# Run in simulated mode for a specific Mac hardware profile
./bin/openvintage-preboot-simulator -c --simulate MacBookPro9,1

# Force native host hardware detection
./bin/openvintage-preboot-simulator -c --native

# Perform read-only host hardware discovery
./bin/openvintage-preboot-simulator -d

# List all available Mac simulation profiles
./bin/openvintage-preboot-simulator -l

# Export diagnostic reports
./bin/openvintage-preboot-simulator -c -r report.txt --json report.json --html report.html

# Run the automated test suite
./bin/test_runner
```

## Hardware Architecture

- **`OV_HW_MODE_NATIVE`**: Queries physical host hardware directly. On macOS, uses `sysctlbyname` for CPU and memory, and `IOKit` (`IOAccelerator`, `IOPCIDevice`) for active GPU and VRAM. On Linux, queries `/proc/cpuinfo`, `/proc/meminfo`, and PCI configuration space.
- **`OV_HW_MODE_SIMULATED`**: Activated explicitly when passing `--simulate <profile>`. Populates the simulated environment with verified Mac hardware specifications.
- **Anti-Fabrication Principle**: Native mode never falls back to or assumes `MacBookPro9,1` or any other simulated profile.

## License

See LICENSE file

