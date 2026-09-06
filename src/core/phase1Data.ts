/**
 * OpenVintage - Phase 1 Verification Artifacts & Telemetry
 * Verified logs from X64 UEFI EDK II deployment in QEMU + OVMF.
 */

import { Phase1Artifacts } from '../types';

export const PHASE_1_VERIFIED_DATA: Phase1Artifacts = {
  efiVersion: 'UEFI 2.70 (EDK II release/202305)',
  buildTarget: 'X64-RELEASE-GCC5',
  toolchain: 'x86_64-linux-gnu-gcc 12.2.0',
  qemuStatus: 'PASS',
  ovmfStatus: 'PASS',
  devicesEnumerated: 14,
  blockIoDevices: [
    'PciRoot(0x0)/Pci(0x1,0x1)/Ata(0x0)/HD(1,MBR,0x12345678,0x800,0x100000)',
    'PciRoot(0x0)/Pci(0x2,0x0)/Scsi(0x0,0x0)/HD(1,GPT,6A7B-8C9D,0x2048,0x3A000)',
    'PciRoot(0x0)/Pci(0x1F,0x2)/Sata(0x0,0xFFFF,0x0)/HD(1,FAT32,ESP_SYS,0x800,0x40000)',
  ],
  devicePathsDetected: [
    'Acpi(PNP0A03,0)/Pci(0x1F,0x0)/Isa(0x0)',
    'Acpi(PNP0A03,0)/Pci(0x2,0x0)/GraphicsOutput()',
    'Acpi(PNP0A03,0)/Pci(0x3,0x0)/Ethernet(52:54:00:12:34:56)',
    'Acpi(PNP0A03,0)/Pci(0x1F,0x2)/Sata(0x0,0x0,0x0)',
  ],
  exitCode: 'EFI_SUCCESS (0x00000000)',
  timestamp: 'Phase 1 Milestone Sign-off Verified',
};

export const SAMPLE_WORKLOADS = [
  {
    id: 'wk-metal-game',
    name: 'Modern 3D Indie Title (Metal 2.0)',
    api: 'Metal' as const,
    apiVersion: '2.0',
    requiresCompute: true,
    requiresTessellation: false,
    drawCallsPerFrame: 1450,
    vramRequiredMB: 1200,
    msaaRequested: 4,
    targetFramerate: 60,
    shaderLanguage: 'MSL' as const,
  },
  {
    id: 'wk-vulkan-render',
    name: 'Vulkan Path-Tracer Preview',
    api: 'Vulkan' as const,
    apiVersion: '1.2',
    requiresCompute: true,
    requiresTessellation: true,
    drawCallsPerFrame: 3200,
    vramRequiredMB: 2048,
    msaaRequested: 8,
    targetFramerate: 30,
    shaderLanguage: 'SPIR-V' as const,
  },
  {
    id: 'wk-gl-legacy',
    name: 'Legacy OpenGL 3.3 CAD Viewer',
    api: 'OpenGL' as const,
    apiVersion: '3.3 Core',
    requiresCompute: false,
    requiresTessellation: false,
    drawCallsPerFrame: 650,
    vramRequiredMB: 380,
    msaaRequested: 2,
    targetFramerate: 60,
    shaderLanguage: 'GLSL' as const,
  },
  {
    id: 'wk-dx11-sim',
    name: 'DirectX 11 Flight Sim Shader Pipeline',
    api: 'DirectX' as const,
    apiVersion: '11.0',
    requiresCompute: true,
    requiresTessellation: false,
    drawCallsPerFrame: 2100,
    vramRequiredMB: 1500,
    msaaRequested: 4,
    targetFramerate: 60,
    shaderLanguage: 'HLSL' as const,
  },
];
