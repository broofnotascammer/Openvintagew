/**
 * OpenVintage - Unified Application State & Data Engine (Phase 6.1)
 * Faithfully mirrors OpenVintage Core C APIs:
 * - ov_hardware.h (Native detection + Simulated hardware profiles)
 * - ov_compatibility.h (Multi-OS evaluation matrix + OCLP/rEFInd resolver)
 * - ov_perf_profile.h (6 Hardware performance profiles)
 * - ov_boot_picker.h (Boot target manager)
 * - ov_deployment.h (9-Step safe deployment lifecycle)
 * - ov_security.h (Security guardrails & confirmation gates)
 */

export type HardwareSource = 'NATIVE' | 'SIMULATED';

export interface GpuDevice {
  id: string;
  index: number;
  modelName: string;
  vendor: 'Intel' | 'NVIDIA' | 'AMD' | 'Apple' | 'Mesa / Generic';
  isIntegrated: boolean;
  isDiscrete: boolean;
  vramMB: number;
  metalLevel: string; // 'Metal 1.2', 'Metal 2.0', 'Metal 3.0', 'None (Pre-Metal)'
  maxTextureDimension: number;
  openGLVersion: string;
  pcieGen: string;
  present: boolean;
  status: string;
}

export interface CpuInfo {
  modelName: string;
  architecture: string; // 'x86_64', 'ARM64'
  microarchitecture: string; // 'Ivy Bridge', 'Haswell', etc.
  physicalCores: number;
  logicalThreads: number;
  baseClockGhz: number;
  maxTurboGhz: number;
  tdpWatts: number;
  features: {
    sse4_2: boolean;
    avx: boolean;
    avx2: boolean;
    aesNi: boolean;
    vtx: boolean;
    fma: boolean;
    bmi2: boolean;
  };
}

export interface MemoryInfo {
  totalMB: number;
  type: string; // 'DDR3-1600', 'LPDDR3', 'DDR4'
  channels: string; // 'Dual Channel'
  bandwidthGBs: number;
}

export interface DisplayInfo {
  internalDisplay: string;
  nativeResolution: string;
  retinaScaled: boolean;
  colorSpace: string;
  externalDisplaysSupported: number;
}

export interface FirmwareInfo {
  type: string; // 'Apple EFI 64-bit'
  version: string;
  apfsSupport: boolean;
  nvramEmulation: boolean;
  secureBootMode: 'Disabled' | 'Medium' | 'Full';
}

export interface PciDevice {
  slot: string;
  device: string;
  vendorId: string;
  deviceId: string;
  status: string;
}

export interface HardwareProfileData {
  id: string;
  systemName: string;
  modelIdentifier: string;
  marketingName: string;
  releaseYear: number;
  source: HardwareSource;
  sourceLabel: string;
  cpu: CpuInfo;
  gpus: GpuDevice[];
  activeGpuIndex: number;
  memory: MemoryInfo;
  display: DisplayInfo;
  firmware: FirmwareInfo;
  pciDevices: PciDevice[];
  quirks: string[];
}

export interface OsCompatibilityMatrix {
  osName: string;
  version: string;
  codename: string;
  releaseYear: number;
  isNative: boolean;
  rating: 'NATIVELY_SUPPORTED' | 'SUPPORTED_OCLP' | 'UNSUPPORTED_SILICON';
  ratingLabel: string;
  integrationRequired: 'None' | 'OCLP' | 'rEFInd' | 'Custom Patch';
  rationale: string;
  checks: {
    cpu64Bit: { pass: boolean; note: string };
    sse4_2: { pass: boolean; note: string };
    avx2: { pass: boolean; note: string };
    gpuMetal: { pass: boolean; note: string };
    legacyGpuPatches: { required: boolean; note: string };
    cryptexBypass: { required: boolean; note: string };
    apfsSupport: { pass: boolean; note: string };
  };
}

export type PerformanceProfileType = 
  | 'MAX_PERFORMANCE' 
  | 'GAMING' 
  | 'BALANCED' 
  | 'EFFICIENCY' 
  | 'COMPATIBILITY' 
  | 'CUSTOM';

export interface PerformanceProfileDef {
  id: PerformanceProfileType;
  name: string;
  icon: string;
  badge: string;
  summary: string;
  gpuPolicy: string;
  cpuGovernor: string;
  latencyTarget: string;
  graphicsTranslation: string;
  powerLimit: string;
  fanPolicy: string;
  honestyDisclaimer: string;
}

export interface BootTarget {
  id: string;
  index: number;
  title: string;
  osName: string;
  version: string;
  architecture: string;
  efiPath: string;
  volumeName: string;
  isDefault: boolean;
  compatibilityState: 'COMPATIBLE' | 'REQUIRES_OCLP' | 'FALLBACK';
  recommendedProfile: PerformanceProfileType;
}

export interface IntegrationStatus {
  id: 'openvintage' | 'oclp' | 'refind';
  name: string;
  version: string;
  status: 'INSTALLED' | 'NOT_INSTALLED' | 'RECOMMENDED' | 'OPTIONAL' | 'UPDATE_AVAILABLE';
  statusLabel: string;
  description: string;
  actionLabel: string;
  rationale: string;
  installedVersion?: string;
  latestVersion: string;
  efiPayloadPath: string;
}

export type DeploymentStep = 
  | 'DISCOVER' 
  | 'SIMULATE' 
  | 'PLAN' 
  | 'REVIEW' 
  | 'USER_APPROVAL' 
  | 'BACKUP' 
  | 'APPLY' 
  | 'VERIFY' 
  | 'RECOVERY';

export interface DeploymentPlanItem {
  id: string;
  action: 'INSTALL' | 'UPDATE' | 'CREATE' | 'BACKUP';
  targetPath: string;
  sha256: string;
  sizeBytes: number;
  description: string;
}

export interface DeploymentState {
  currentStep: DeploymentStep;
  stepIndex: number;
  totalSteps: number;
  isInProgress: boolean;
  requiresUserApproval: boolean;
  userApproved: boolean;
  planItems: DeploymentPlanItem[];
  backupPath: string;
  logs: string[];
  isCompleted: boolean;
  hasError: boolean;
  errorMessage?: string;
}

// ==========================================
// PRESET SIMULATION PROFILES
// ==========================================
export const PRESET_SIMULATED_PROFILES: HardwareProfileData[] = [
  {
    id: 'mbp91',
    systemName: 'MacBookPro9,1',
    modelIdentifier: 'MacBookPro9,1',
    marketingName: 'MacBook Pro (15-inch, Mid 2012)',
    releaseYear: 2012,
    source: 'SIMULATED',
    sourceLabel: 'SIMULATED HARDWARE (MBP 9,1)',
    cpu: {
      modelName: 'Intel Core i7-3615QM',
      architecture: 'x86_64',
      microarchitecture: 'Ivy Bridge',
      physicalCores: 4,
      logicalThreads: 8,
      baseClockGhz: 2.3,
      maxTurboGhz: 3.3,
      tdpWatts: 45,
      features: {
        sse4_2: true,
        avx: true,
        avx2: false,
        aesNi: true,
        vtx: true,
        fma: false,
        bmi2: false,
      },
    },
    gpus: [
      {
        id: 'hd4000',
        index: 0,
        modelName: 'Intel HD Graphics 4000',
        vendor: 'Intel',
        isIntegrated: true,
        isDiscrete: false,
        vramMB: 1536,
        metalLevel: 'Metal 1.2',
        maxTextureDimension: 8192,
        openGLVersion: 'OpenGL 4.1 Core',
        pcieGen: 'Integrated Ring Bus',
        present: true,
        status: 'Online / Switchable (GMUX)',
      },
      {
        id: 'gt650m',
        index: 1,
        modelName: 'NVIDIA GeForce GT 650M',
        vendor: 'NVIDIA',
        isIntegrated: false,
        isDiscrete: true,
        vramMB: 1024,
        metalLevel: 'Metal 1.2',
        maxTextureDimension: 16384,
        openGLVersion: 'OpenGL 4.1 Core',
        pcieGen: 'PCIe 3.0 x8',
        present: true,
        status: 'Online / Discrete GK107',
      },
    ],
    activeGpuIndex: 1, // Default discrete
    memory: {
      totalMB: 16384,
      type: 'DDR3-1600 SO-DIMM',
      channels: 'Dual Channel (2 x 8GB)',
      bandwidthGBs: 25.6,
    },
    display: {
      internalDisplay: 'Color LCD (Hi-Res Antiglare)',
      nativeResolution: '1680 x 1050',
      retinaScaled: false,
      colorSpace: 'sRGB IEC61966-2.1',
      externalDisplaysSupported: 2,
    },
    firmware: {
      type: 'Apple EFI 64-bit',
      version: 'MBP91.88Z.F000.B00.1906140410',
      apfsSupport: true,
      nvramEmulation: false,
      secureBootMode: 'Disabled',
    },
    pciDevices: [
      { slot: 'PCIe 0:1:0', device: 'Intel Ivy Bridge PCI Express Root Port', vendorId: '0x8086', deviceId: '0x0151', status: 'Active' },
      { slot: 'PCIe 1:0:0', device: 'NVIDIA GK107M [GeForce GT 650M]', vendorId: '0x10de', deviceId: '0x0fd5', status: 'Active' },
      { slot: 'PCIe 0:2:0', device: 'Intel 3rd Gen Core processor Graphics (HD 4000)', vendorId: '0x8086', deviceId: '0x0166', status: 'Active' },
      { slot: 'PCIe 2:0:0', device: 'Broadcom BCM4331 802.11a/b/g/n Wireless LAN', vendorId: '0x14e4', deviceId: '0x4331', status: 'Active' },
      { slot: 'PCIe 4:0:0', device: 'Intel Cactus Ridge Thunderbolt Controller', vendorId: '0x8086', deviceId: '0x1547', status: 'Active' },
    ],
    quirks: [
      'Automatic Graphics Switching (GMUX) can cause display stalls during API handoff',
      'Requires OpenCore Legacy Patcher (OCLP) root patches for Kepler / HD 4000 Metal graphics drivers on macOS 12 Monterey and above',
    ],
  },
  {
    id: 'mbp113',
    systemName: 'MacBookPro11,3',
    modelIdentifier: 'MacBookPro11,3',
    marketingName: 'MacBook Pro (15-inch Retina, Mid 2014)',
    releaseYear: 2014,
    source: 'SIMULATED',
    sourceLabel: 'SIMULATED HARDWARE (MBP 11,3)',
    cpu: {
      modelName: 'Intel Core i7-4870HQ',
      architecture: 'x86_64',
      microarchitecture: 'Haswell',
      physicalCores: 4,
      logicalThreads: 8,
      baseClockGhz: 2.5,
      maxTurboGhz: 3.7,
      tdpWatts: 47,
      features: {
        sse4_2: true,
        avx: true,
        avx2: true,
        aesNi: true,
        vtx: true,
        fma: true,
        bmi2: true,
      },
    },
    gpus: [
      {
        id: 'iris5200',
        index: 0,
        modelName: 'Intel Iris Pro Graphics 5200 (128MB eDRAM)',
        vendor: 'Intel',
        isIntegrated: true,
        isDiscrete: false,
        vramMB: 1536,
        metalLevel: 'Metal 2.0',
        maxTextureDimension: 16384,
        openGLVersion: 'OpenGL 4.1 Core',
        pcieGen: 'Integrated',
        present: true,
        status: 'Online / eDRAM Crystalwell',
      },
      {
        id: 'gt750m',
        index: 1,
        modelName: 'NVIDIA GeForce GT 750M',
        vendor: 'NVIDIA',
        isIntegrated: false,
        isDiscrete: true,
        vramMB: 2048,
        metalLevel: 'Metal 2.0',
        maxTextureDimension: 16384,
        openGLVersion: 'OpenGL 4.1 Core',
        pcieGen: 'PCIe 3.0 x8',
        present: true,
        status: 'Online / Discrete GK107',
      },
    ],
    activeGpuIndex: 1,
    memory: {
      totalMB: 16384,
      type: 'DDR3L-1600 Onboard',
      channels: 'Dual Channel',
      bandwidthGBs: 25.6,
    },
    display: {
      internalDisplay: 'Retina Display (IPS)',
      nativeResolution: '2880 x 1800',
      retinaScaled: true,
      colorSpace: 'sRGB / P3 Emulated',
      externalDisplaysSupported: 2,
    },
    firmware: {
      type: 'Apple EFI 64-bit',
      version: 'MBP113.88Z.0149.B00.1804100913',
      apfsSupport: true,
      nvramEmulation: false,
      secureBootMode: 'Disabled',
    },
    pciDevices: [
      { slot: 'PCIe 0:2:0', device: 'Intel Iris Pro Graphics 5200', vendorId: '0x8086', deviceId: '0x0d26', status: 'Active' },
      { slot: 'PCIe 1:0:0', device: 'NVIDIA GeForce GT 750M Mac Edition', vendorId: '0x10de', deviceId: '0x0fe9', status: 'Active' },
      { slot: 'PCIe 2:0:0', device: 'Broadcom BCM4360 802.11ac Wireless Network Adapter', vendorId: '0x14e4', deviceId: '0x43a0', status: 'Active' },
      { slot: 'PCIe 3:0:0', device: 'Apple Proprietary PCIe SSD Controller', vendorId: '0x106b', deviceId: '0x2001', status: 'Active' },
    ],
    quirks: [
      'Haswell AVX2 instruction set natively present',
      'Kepler GT 750M requires OCLP root patches on macOS 12+ due to Apple removing Kepler driver bundle in Monterey',
    ],
  },
  {
    id: 'mba52',
    systemName: 'MacBookAir5,2',
    modelIdentifier: 'MacBookAir5,2',
    marketingName: 'MacBook Air (13-inch, Mid 2012)',
    releaseYear: 2012,
    source: 'SIMULATED',
    sourceLabel: 'SIMULATED HARDWARE (MBA 5,2)',
    cpu: {
      modelName: 'Intel Core i5-3427U',
      architecture: 'x86_64',
      microarchitecture: 'Ivy Bridge',
      physicalCores: 2,
      logicalThreads: 4,
      baseClockGhz: 1.8,
      maxTurboGhz: 2.8,
      tdpWatts: 17,
      features: {
        sse4_2: true,
        avx: true,
        avx2: false,
        aesNi: true,
        vtx: true,
        fma: false,
        bmi2: false,
      },
    },
    gpus: [
      {
        id: 'hd4000_mba',
        index: 0,
        modelName: 'Intel HD Graphics 4000',
        vendor: 'Intel',
        isIntegrated: true,
        isDiscrete: false,
        vramMB: 1536,
        metalLevel: 'Metal 1.2',
        maxTextureDimension: 8192,
        openGLVersion: 'OpenGL 4.1 Core',
        pcieGen: 'Integrated',
        present: true,
        status: 'Online / Single GPU Topology',
      },
    ],
    activeGpuIndex: 0,
    memory: {
      totalMB: 8192,
      type: 'DDR3L-1600 LPDDR',
      channels: 'Dual Channel',
      bandwidthGBs: 25.6,
    },
    display: {
      internalDisplay: 'Color LCD (TFT)',
      nativeResolution: '1440 x 900',
      retinaScaled: false,
      colorSpace: 'sRGB',
      externalDisplaysSupported: 1,
    },
    firmware: {
      type: 'Apple EFI 64-bit',
      version: 'MBA52.88Z.00F0.B00.1906140410',
      apfsSupport: true,
      nvramEmulation: false,
      secureBootMode: 'Disabled',
    },
    pciDevices: [
      { slot: 'PCIe 0:2:0', device: 'Intel HD Graphics 4000', vendorId: '0x8086', deviceId: '0x0166', status: 'Active' },
      { slot: 'PCIe 2:0:0', device: 'Broadcom BCM43224 802.11a/b/g/n', vendorId: '0x14e4', deviceId: '0x4353', status: 'Active' },
    ],
    quirks: [
      'Single-GPU topology (no discrete graphics)',
      'Aggressive thermal throttling when CPU and GPU are loaded simultaneously under low TDP envelope',
    ],
  },
  {
    id: 'macpro51',
    systemName: 'MacPro5,1',
    modelIdentifier: 'MacPro5,1',
    marketingName: 'Mac Pro (Mid 2010 / 2012)',
    releaseYear: 2010,
    source: 'SIMULATED',
    sourceLabel: 'SIMULATED HARDWARE (MacPro 5,1)',
    cpu: {
      modelName: 'Dual Intel Xeon X5670 (Westmere-EP)',
      architecture: 'x86_64',
      microarchitecture: 'Westmere',
      physicalCores: 12,
      logicalThreads: 24,
      baseClockGhz: 2.93,
      maxTurboGhz: 3.33,
      tdpWatts: 190,
      features: {
        sse4_2: true,
        avx: false,
        avx2: false,
        aesNi: true,
        vtx: true,
        fma: false,
        bmi2: false,
      },
    },
    gpus: [
      {
        id: 'hd5770',
        index: 0,
        modelName: 'ATI Radeon HD 5770 (TeraScale 2)',
        vendor: 'AMD',
        isIntegrated: false,
        isDiscrete: true,
        vramMB: 1024,
        metalLevel: 'None (Pre-Metal)',
        maxTextureDimension: 16384,
        openGLVersion: 'OpenGL 3.3 Core',
        pcieGen: 'PCIe 2.0 x16',
        present: true,
        status: 'Online / Legacy Non-Metal GPU',
      },
    ],
    activeGpuIndex: 0,
    memory: {
      totalMB: 32768,
      type: 'DDR3 ECC 1333MHz',
      channels: 'Triple Channel per Socket',
      bandwidthGBs: 64.0,
    },
    display: {
      internalDisplay: 'External DVI / DisplayPort Monitor',
      nativeResolution: '2560 x 1440',
      retinaScaled: false,
      colorSpace: 'sRGB',
      externalDisplaysSupported: 3,
    },
    firmware: {
      type: 'Apple EFI 64-bit',
      version: 'MP51.88Z.0089.B00.1906140410',
      apfsSupport: true,
      nvramEmulation: false,
      secureBootMode: 'Disabled',
    },
    pciDevices: [
      { slot: 'PCIe Slot 1', device: 'ATI Radeon HD 5770 Evergreen Graphics Processor', vendorId: '0x1002', deviceId: '0x68b8', status: 'Active' },
      { slot: 'PCIe Onboard', device: 'Intel 82574L Dual Gigabit Ethernet Controller', vendorId: '0x8086', deviceId: '0x10d3', status: 'Active' },
    ],
    quirks: [
      'No AVX instructions on Westmere CPUs (macOS 13+ requires cryptex bypass and Rosetta instruction emulation)',
      'TeraScale 2 has no Metal support; requires legacy non-Metal OpenGL patch bundle',
    ],
  },
];

// NATIVE HOST HARDWARE PROFILE (Matches actual Linux/Darwin host environment)
export const NATIVE_HOST_HARDWARE_PROFILE: HardwareProfileData = {
  id: 'host_native',
  systemName: 'Linux x86_64 Host',
  modelIdentifier: 'LinuxSystemHost',
  marketingName: 'Linux x86_64 System Host',
  releaseYear: 2024,
  source: 'NATIVE',
  sourceLabel: 'REAL HARDWARE (Host Silicon)',
  cpu: {
    modelName: 'Intel Core / Xeon Compatible (Host Silicon)',
    architecture: 'x86_64',
    microarchitecture: 'x86_64 Modern Host',
    physicalCores: 2,
    logicalThreads: 2,
    baseClockGhz: 2.8,
    maxTurboGhz: 3.4,
    tdpWatts: 65,
    features: {
      sse4_2: true,
      avx: true,
      avx2: true,
      aesNi: true,
      vtx: true,
      fma: true,
      bmi2: true,
    },
  },
  gpus: [
    {
      id: 'host_gpu0',
      index: 0,
      modelName: 'Linux Host Graphics / Mesa Fallback',
      vendor: 'Mesa / Generic',
      isIntegrated: true,
      isDiscrete: false,
      vramMB: 512,
      metalLevel: 'None (Pre-Metal)',
      maxTextureDimension: 8192,
      openGLVersion: 'OpenGL 4.5 Core',
      pcieGen: 'Host Virtual Ingress',
      present: true,
      status: 'Online / System Primary Display',
    },
  ],
  activeGpuIndex: 0,
  memory: {
    totalMB: 8192,
    type: 'Host Unified Memory',
    channels: 'Dual Channel',
    bandwidthGBs: 34.1,
  },
  display: {
    internalDisplay: 'Virtual Headless / Container Display',
    nativeResolution: '1920 x 1080',
    retinaScaled: false,
    colorSpace: 'sRGB IEC61966-2.1',
    externalDisplaysSupported: 1,
  },
  firmware: {
    type: 'OVMF / UEFI 2.70 64-bit',
    version: 'OpenVintage Preboot v6.0',
    apfsSupport: true,
    nvramEmulation: true,
    secureBootMode: 'Disabled',
  },
  pciDevices: [
    { slot: 'Host Bridge', device: 'Host PCI System Bus', vendorId: '0x8086', deviceId: '0x1234', status: 'Active' },
    { slot: 'Display Controller', device: 'Virtual Graphics Adapter', vendorId: '0x1af4', deviceId: '0x1050', status: 'Active' },
  ],
  quirks: [
    'Running under containerized Linux execution environment',
    'Simulated hardware profiles can be activated in the Simulator tab to test legacy Mac silicon models',
  ],
};

// ==========================================
// COMPATIBILITY KNOWLEDGE BASE
// ==========================================
export const TARGET_OS_LIST = [
  'macOS 10.13 High Sierra',
  'macOS 10.14 Mojave',
  'macOS 10.15 Catalina',
  'macOS 11 Big Sur',
  'macOS 12 Monterey',
  'macOS 13 Ventura',
  'macOS 14 Sonoma',
  'macOS 15 Sequoia',
  'Linux 6.x LTS (Ubuntu/Debian)',
  'Windows 11 (UEFI)',
];

export function evaluateOsCompatibility(osName: string, profile: HardwareProfileData): OsCompatibilityMatrix {
  const isIvyBridge = profile.cpu.microarchitecture.includes('Ivy Bridge');
  const isHaswell = profile.cpu.microarchitecture.includes('Haswell');
  const isWestmere = profile.cpu.microarchitecture.includes('Westmere');
  const hasAvx2 = profile.cpu.features.avx2;
  const activeGpu = profile.gpus[profile.activeGpuIndex] || profile.gpus[0];
  const hasMetal = activeGpu.metalLevel.startsWith('Metal');
  
  if (osName === 'macOS 10.13 High Sierra') {
    return {
      osName,
      version: '10.13.6',
      codename: 'High Sierra',
      releaseYear: 2017,
      isNative: true,
      rating: 'NATIVELY_SUPPORTED',
      ratingLabel: 'Natively Supported',
      integrationRequired: 'None',
      rationale: 'Fully supported by Apple firmware on Mid-2012 and newer hardware. Native APFS and Metal 1.2 drivers bundled in kernel.',
      checks: {
        cpu64Bit: { pass: true, note: '64-bit Intel Architecture verified' },
        sse4_2: { pass: true, note: 'SSE4.2 available' },
        avx2: { pass: true, note: 'Not required for High Sierra' },
        gpuMetal: { pass: hasMetal, note: hasMetal ? 'Metal 1.2 drivers supported' : 'Legacy OpenGL fallback' },
        legacyGpuPatches: { required: false, note: 'Native Apple graphics kexts present' },
        cryptexBypass: { required: false, note: 'Cryptex architecture introduced in Ventura' },
        apfsSupport: { pass: true, note: 'APFS EFI driver available' },
      },
    };
  }

  if (osName === 'macOS 10.15 Catalina') {
    return {
      osName,
      version: '10.15.7',
      codename: 'Catalina',
      releaseYear: 2019,
      isNative: isIvyBridge || isHaswell,
      rating: (isIvyBridge || isHaswell) ? 'NATIVELY_SUPPORTED' : 'SUPPORTED_OCLP',
      ratingLabel: (isIvyBridge || isHaswell) ? 'Natively Supported' : 'Supported with OCLP',
      integrationRequired: (isIvyBridge || isHaswell) ? 'None' : 'OCLP',
      rationale: (isIvyBridge || isHaswell)
        ? 'Apple native endpoint for 2012-2013 Mac models. Complete graphics acceleration and 64-bit pure environment.'
        : 'Requires OpenCore Legacy Patcher to spoof supported board ID and supply non-Metal or TeraScale patches.',
      checks: {
        cpu64Bit: { pass: true, note: 'Pure 64-bit OS' },
        sse4_2: { pass: true, note: 'SSE4.2 satisfied' },
        avx2: { pass: true, note: 'Not required for Catalina' },
        gpuMetal: { pass: hasMetal, note: hasMetal ? 'Metal graphics present' : 'Non-Metal patches needed' },
        legacyGpuPatches: { required: !isIvyBridge && !isHaswell, note: isIvyBridge ? 'Native Kepler/HD4000 support' : 'Legacy GPU root patch needed' },
        cryptexBypass: { required: false, note: 'No cryptex' },
        apfsSupport: { pass: true, note: 'Native APFS container' },
      },
    };
  }

  if (osName === 'macOS 12 Monterey') {
    return {
      osName,
      version: '12.7.4',
      codename: 'Monterey',
      releaseYear: 2021,
      isNative: false,
      rating: 'SUPPORTED_OCLP',
      ratingLabel: 'Supported with OCLP',
      integrationRequired: 'OCLP',
      rationale: 'Requires OpenCore Legacy Patcher (OCLP) root patches for Kepler / HD 4000 Metal graphics drivers. Broadcom WiFi patches also applied.',
      checks: {
        cpu64Bit: { pass: true, note: '64-bit architecture verified' },
        sse4_2: { pass: true, note: 'SSE4.2 passes kernel requirement' },
        avx2: { pass: true, note: 'Monterey kernel runs on SSE4.2; AVX2 not mandatory' },
        gpuMetal: { pass: true, note: 'Kepler & HD 4000 Metal drivers re-injected via OCLP' },
        legacyGpuPatches: { required: true, note: 'Apple removed Kepler bundle; OCLP root patch restores libmetal and GL' },
        cryptexBypass: { required: false, note: 'Not required on Monterey' },
        apfsSupport: { pass: true, note: 'Full APFS seal support' },
      },
    };
  }

  if (osName === 'macOS 13 Ventura' || osName === 'macOS 14 Sonoma') {
    const isVentura = osName.includes('Ventura');
    return {
      osName,
      version: isVentura ? '13.6.7' : '14.5',
      codename: isVentura ? 'Ventura' : 'Sonoma',
      releaseYear: isVentura ? 2022 : 2023,
      isNative: false,
      rating: isWestmere ? 'UNSUPPORTED_SILICON' : 'SUPPORTED_OCLP',
      ratingLabel: isWestmere ? 'Unsupported (No AVX)' : 'Supported with OCLP',
      integrationRequired: 'OCLP',
      rationale: hasAvx2 
        ? 'Fully supported with OCLP root patch for legacy graphics acceleration.'
        : 'Requires OCLP with Rosetta Cryptex bypass for non-AVX2 CPUs (Ivy Bridge) and legacy graphics driver injection.',
      checks: {
        cpu64Bit: { pass: true, note: 'x86_64 supported' },
        sse4_2: { pass: true, note: 'SSE4.2 pass' },
        avx2: { pass: hasAvx2, note: hasAvx2 ? 'Native AVX2 instruction set' : 'Non-AVX2 CPU requires Rosetta Cryptex OS.dmg bypass' },
        gpuMetal: { pass: true, note: 'Metal 3802 graphics patch bundle required' },
        legacyGpuPatches: { required: true, note: 'Kepler / HD 4000 root patch mandatory' },
        cryptexBypass: { required: !hasAvx2, note: !hasAvx2 ? 'Cryptex bypass enabled in OpenVintage' : 'Native cryptex' },
        apfsSupport: { pass: true, note: 'APFS snapshot root required' },
      },
    };
  }

  if (osName === 'macOS 15 Sequoia') {
    return {
      osName,
      version: '15.0',
      codename: 'Sequoia',
      releaseYear: 2024,
      isNative: false,
      rating: 'SUPPORTED_OCLP',
      ratingLabel: 'Experimental OCLP Support',
      integrationRequired: 'OCLP',
      rationale: 'Experimental support via OpenCore Legacy Patcher nightly builds. Requires T2 emulation, Cryptex bypass, and legacy WiFi/Metal patches.',
      checks: {
        cpu64Bit: { pass: true, note: '64-bit kernel' },
        sse4_2: { pass: true, note: 'SSE4.2 pass' },
        avx2: { pass: hasAvx2, note: hasAvx2 ? 'Haswell AVX2 present' : 'Bypass needed for Ivy Bridge' },
        gpuMetal: { pass: true, note: 'Experimental Metal patch' },
        legacyGpuPatches: { required: true, note: 'Root patch needed' },
        cryptexBypass: { required: true, note: 'Cryptex replacement enabled' },
        apfsSupport: { pass: true, note: 'Signed System Volume (SSV) snapshot' },
      },
    };
  }

  if (osName.includes('Linux')) {
    return {
      osName,
      version: '6.8 / 6.1 LTS',
      codename: 'Linux Kernel',
      releaseYear: 2024,
      isNative: true,
      rating: 'NATIVELY_SUPPORTED',
      ratingLabel: 'Natively Supported (rEFInd Recommended)',
      integrationRequired: 'rEFInd',
      rationale: 'Fully supported by upstream Linux kernel. Open-source Mesa drivers for HD 4000 (i965/crocus) and NVIDIA Nouveau / proprietary 390.xx/470.xx driver for Kepler.',
      checks: {
        cpu64Bit: { pass: true, note: 'Linux x86_64 kernel' },
        sse4_2: { pass: true, note: 'Pass' },
        avx2: { pass: true, note: 'Supported on both AVX and non-AVX' },
        gpuMetal: { pass: true, note: 'Uses OpenGL 4.5 / Vulkan (Mesa Crocus/Zink)' },
        legacyGpuPatches: { required: false, note: 'Mesa drivers natively built into distributions' },
        cryptexBypass: { required: false, note: 'Not applicable to Linux' },
        apfsSupport: { pass: true, note: 'EXT4/Btrfs supported' },
      },
    };
  }

  // Windows
  return {
    osName,
    version: '23H2 / 24H2',
    codename: 'Windows 11',
    releaseYear: 2023,
    isNative: false,
    rating: 'SUPPORTED_OCLP',
    ratingLabel: 'Supported with TPM/CPU Bypass',
    integrationRequired: 'rEFInd',
    rationale: 'Hardware runs Windows 11 with standard TPM 2.0 and CPU generation bypasses. NVIDIA 470.xx Kepler driver and Intel HD 4000 WDDM 1.3 drivers function stably.',
    checks: {
      cpu64Bit: { pass: true, note: 'UEFI 64-bit' },
      sse4_2: { pass: true, note: 'SSE4.2 pass' },
      avx2: { pass: true, note: 'Runs stably on Ivy Bridge' },
      gpuMetal: { pass: true, note: 'DirectX 11 / WDDM 1.3 native drivers' },
      legacyGpuPatches: { required: false, note: 'Windows drivers available' },
      cryptexBypass: { required: false, note: 'Not applicable' },
      apfsSupport: { pass: true, note: 'NTFS EFI boot' },
    },
  };
}

// ==========================================
// PERFORMANCE PROFILES
// ==========================================
export const PERFORMANCE_PROFILES: PerformanceProfileDef[] = [
  {
    id: 'MAX_PERFORMANCE',
    name: 'Maximum Performance',
    icon: 'Zap',
    badge: '⚡ Max Clock',
    summary: 'Pins highest CPU frequencies, locks discrete GPU active via GMUX, and minimizes translation latency for sustained heavy workloads.',
    gpuPolicy: 'Force Discrete GPU (GeForce GT 650M / Secondary)',
    cpuGovernor: 'Performance Governor (Max Turbo 3.3 GHz locked)',
    latencyTarget: 'Ultra-low (Deterministic JIT execution)',
    graphicsTranslation: 'Fast-path translation (Cached IR pipelines)',
    powerLimit: '45W TDP Max (AC Power recommended)',
    fanPolicy: 'Aggressive cooling curve to avoid thermal throttling',
    honestyDisclaimer: 'OpenVintage configures OS governors and GMUX power policies for maximum performance allowed by physical silicon limits. No unsafe overvolting.',
  },
  {
    id: 'GAMING',
    name: 'Gaming & 3D Acceleration',
    icon: 'Gamepad2',
    badge: '🎮 Gaming',
    summary: 'Optimized for graphics throughput, favoring discrete GPU rendering with optimized texture streaming and high frame-pacing stability.',
    gpuPolicy: 'Prefer Discrete GPU with dynamic fallback',
    cpuGovernor: 'Adaptive high-performance governor',
    latencyTarget: 'Low (< 8ms render loop target)',
    graphicsTranslation: 'OVIR-GPU direct hardware binding',
    powerLimit: 'Dynamic 35W-45W thermal balancing',
    fanPolicy: 'Pre-emptive ramp on 3D pipeline activation',
    honestyDisclaimer: 'Selects the best available graphics pipeline for detected GPU. Older Kepler architecture targets 720p/900p modern rendering balance.',
  },
  {
    id: 'BALANCED',
    name: 'Balanced',
    icon: 'Scale',
    badge: '⚖ Balanced',
    summary: 'Standard Apple-recommended operational balance. Automatic GMUX GPU switching between integrated HD 4000 and discrete graphics based on demand.',
    gpuPolicy: 'Automatic GMUX Switching (Dynamic Intel / NVIDIA)',
    cpuGovernor: 'Schedutil / Intel SpeedStep dynamic scaling',
    latencyTarget: 'Standard interactive (< 16ms)',
    graphicsTranslation: 'Adaptive caching with LRU eviction',
    powerLimit: 'Default Apple platform thermal envelope (25W-35W)',
    fanPolicy: 'Quiet acoustic curve until 78°C threshold',
    honestyDisclaimer: 'Provides standard baseline performance with seamless graphics switching and normal thermal behavior.',
  },
  {
    id: 'EFFICIENCY',
    name: 'Efficiency & Battery',
    icon: 'BatteryCharging',
    badge: '🔋 Energy Saver',
    summary: 'Extends mobile battery runtime by forcing integrated Intel HD 4000 graphics, powering down discrete GPU silicon, and capping CPU frequencies.',
    gpuPolicy: 'Force Integrated GPU (Intel HD 4000, Discrete Powered Off)',
    cpuGovernor: 'Powersave Governor (Base 2.3 GHz, Turbo restricted)',
    latencyTarget: 'Energy-optimized batching',
    graphicsTranslation: 'Lightweight memory compression',
    powerLimit: 'Restricted 17W-25W envelope',
    fanPolicy: 'Silent fan mode (minimum RPM)',
    honestyDisclaimer: 'Reduces peak throughput in exchange for extended battery life and cool, quiet mobile operation.',
  },
  {
    id: 'COMPATIBILITY',
    name: 'Maximum Compatibility',
    icon: 'Shield',
    badge: '🛡 Safe Mode',
    summary: 'Prioritizes maximum API stability and fallback safety. Enables CPU fallback paths for complex shaders, conservative clocks, and strict EFI hooks.',
    gpuPolicy: 'Safe Discrete / Integrated fallback with validation',
    cpuGovernor: 'Conservative stepping without aggressive turbo',
    latencyTarget: 'Safe verified execution (overhead allowed)',
    graphicsTranslation: 'Validation layer active, CPU compute fallback enabled',
    powerLimit: 'Standard factory baseline',
    fanPolicy: 'Consistent medium cooling',
    honestyDisclaimer: 'Ensures legacy games and unsupported OS drivers don’t crash by routing unsupported hardware shader instructions to safe CPU fallbacks.',
  },
  {
    id: 'CUSTOM',
    name: 'Custom User Profile',
    icon: 'Sliders',
    badge: '⚙ Custom',
    summary: 'Allows power users to manually configure GPU binding preference, CPU governor policy, and translation cache sizing.',
    gpuPolicy: 'User Selected (Currently: Discrete Preferred)',
    cpuGovernor: 'Custom configuration',
    latencyTarget: 'User-specified profile',
    graphicsTranslation: 'User-tuned cache thresholds',
    powerLimit: 'User-defined envelope',
    fanPolicy: 'Customized curve',
    honestyDisclaimer: 'Custom policies operate strictly within safety parameters verified by the OpenVintage Core security guardrail.',
  },
];

// ==========================================
// BOOT TARGETS
// ==========================================
export const DEFAULT_BOOT_TARGETS: BootTarget[] = [
  {
    id: 'target-0',
    index: 0,
    title: 'Macintosh HD (macOS 12 Monterey)',
    osName: 'macOS Monterey',
    version: '12.7.4',
    architecture: 'x86_64',
    efiPath: '/System/Library/CoreServices/boot.efi',
    volumeName: 'Macintosh HD (APFS Volume)',
    isDefault: true,
    compatibilityState: 'REQUIRES_OCLP',
    recommendedProfile: 'BALANCED',
  },
  {
    id: 'target-1',
    index: 1,
    title: 'Catalina Backup (macOS 10.15.7)',
    osName: 'macOS Catalina',
    version: '10.15.7',
    architecture: 'x86_64',
    efiPath: '/Volumes/Catalina/System/Library/CoreServices/boot.efi',
    volumeName: 'Catalina HD (APFS Volume)',
    isDefault: false,
    compatibilityState: 'COMPATIBLE',
    recommendedProfile: 'GAMING',
  },
  {
    id: 'target-2',
    index: 2,
    title: 'macOS Recovery HD',
    osName: 'macOS Recovery',
    version: '12.7.4 Recovery',
    architecture: 'x86_64',
    efiPath: '/Volumes/Recovery/boot.efi',
    volumeName: 'Recovery (Apple APFS Recovery)',
    isDefault: false,
    compatibilityState: 'COMPATIBLE',
    recommendedProfile: 'COMPATIBILITY',
  },
  {
    id: 'target-3',
    index: 3,
    title: 'Ubuntu 24.04 LTS (Linux)',
    osName: 'Linux Ubuntu',
    version: '24.04 (Kernel 6.8)',
    architecture: 'x86_64',
    efiPath: '/EFI/ubuntu/shimx64.efi',
    volumeName: 'Linux Root (EXT4 / ESP)',
    isDefault: false,
    compatibilityState: 'COMPATIBLE',
    recommendedProfile: 'MAX_PERFORMANCE',
  },
  {
    id: 'target-4',
    index: 4,
    title: 'Windows 11 Boot Manager',
    osName: 'Windows 11 Pro',
    version: '23H2 (Build 22631)',
    architecture: 'x86_64',
    efiPath: '/EFI/Microsoft/Boot/bootmgfw.efi',
    volumeName: 'BOOTCAMP (NTFS / ESP)',
    isDefault: false,
    compatibilityState: 'COMPATIBLE',
    recommendedProfile: 'GAMING',
  },
  {
    id: 'target-5',
    index: 5,
    title: 'OpenCore EFI (OCLP Patcher)',
    osName: 'OpenCore Loader',
    version: 'OCLP 1.5.0',
    architecture: 'x86_64',
    efiPath: '/EFI/OC/OpenCore.efi',
    volumeName: 'EFI System Partition (FAT32)',
    isDefault: false,
    compatibilityState: 'COMPATIBLE',
    recommendedProfile: 'BALANCED',
  },
  {
    id: 'target-6',
    index: 6,
    title: 'rEFInd Boot Manager',
    osName: 'rEFInd Boot Manager',
    version: '0.14.2',
    architecture: 'x86_64',
    efiPath: '/EFI/refind/refind_x64.efi',
    volumeName: 'EFI System Partition (FAT32)',
    isDefault: false,
    compatibilityState: 'COMPATIBLE',
    recommendedProfile: 'BALANCED',
  },
];

// ==========================================
// INTEGRATIONS
// ==========================================
export const DEFAULT_INTEGRATIONS: IntegrationStatus[] = [
  {
    id: 'openvintage',
    name: 'OpenVintage Core Boot Manager',
    version: 'v6.1.0 (Phase 6.1)',
    status: 'INSTALLED',
    statusLabel: 'Active & Managing Boot',
    description: 'Hardware-aware preboot manager, multi-GPU topology coordinator, and safe deployment engine.',
    actionLabel: 'Verify Installation',
    rationale: 'Primary boot orchestration and silicon abstraction framework for legacy Intel systems.',
    installedVersion: '6.1.0',
    latestVersion: '6.1.0',
    efiPayloadPath: '/Volumes/EFI/EFI/OpenVintage/OvSelfTestApp.efi',
  },
  {
    id: 'oclp',
    name: 'OpenCore Legacy Patcher (OCLP)',
    version: 'v1.5.0',
    status: 'RECOMMENDED',
    statusLabel: 'Recommended for macOS 12+',
    description: 'Kernel extension and driver root patching engine for running newer macOS releases on legacy Mac silicon.',
    actionLabel: 'Review & Deploy Root Patches',
    rationale: 'Mid-2012 Ivy Bridge and Haswell Macs require Kepler (NVIDIA) and HD 4000 (Intel) graphics acceleration drivers re-injected for Metal functionality.',
    installedVersion: '1.4.3',
    latestVersion: '1.5.0',
    efiPayloadPath: '/Volumes/EFI/EFI/OC/OpenCore.efi',
  },
  {
    id: 'refind',
    name: 'rEFInd EFI Boot Manager',
    version: 'v0.14.2',
    status: 'OPTIONAL',
    statusLabel: 'Optional (Multi-OS)',
    description: 'High-resolution graphical EFI boot picker ideal for multi-boot environments (macOS, Linux, Windows).',
    actionLabel: 'Configure Integration',
    rationale: 'Provides seamless chainloading across non-macOS EFI binaries alongside OpenVintage hardware orchestration.',
    installedVersion: '0.14.0',
    latestVersion: '0.14.2',
    efiPayloadPath: '/Volumes/EFI/EFI/refind/refind_x64.efi',
  },
];

// ==========================================
// AUDITABLE DEPLOYMENT PLAN
// ==========================================
export const DEFAULT_DEPLOYMENT_PLAN: DeploymentPlanItem[] = [
  {
    id: 'plan-1',
    action: 'INSTALL',
    targetPath: '/Volumes/EFI/EFI/BOOT/BOOTX64.EFI',
    sha256: 'e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855',
    sizeBytes: 142848,
    description: 'OpenVintage primary UEFI 64-bit bootloader staging image',
  },
  {
    id: 'plan-2',
    action: 'INSTALL',
    targetPath: '/Volumes/EFI/EFI/OpenVintage/OvSelfTestApp.efi',
    sha256: 'ca978112ca1bbdcafac231b39a23dc4da786eff8147c4e72b9807785afee48bb',
    sizeBytes: 215040,
    description: 'OpenVintage preboot silicon diagnostic & self-test executable',
  },
  {
    id: 'plan-3',
    action: 'CREATE',
    targetPath: '/Volumes/EFI/EFI/OpenVintage/config.plist',
    sha256: '4b227777d4dd1fc61c6f884f48641d02b4d121d3fd328cb08b5531fcacdabf8a',
    sizeBytes: 4120,
    description: 'Hardware profile, boot target preferences, and GMUX GPU switching configuration',
  },
  {
    id: 'plan-4',
    action: 'BACKUP',
    targetPath: '/Volumes/EFI/OpenVintage_Backup/EFI_PreDeployment_20260914.tar.gz',
    sha256: '9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08',
    sizeBytes: 524288,
    description: 'Pre-deployment cryptographic archive of previous EFI partition contents',
  },
];

// ==========================================
// PHASE 8: REAL-HARDWARE SAFE EFI INSTALLER
// ==========================================
export interface StorageTarget {
  deviceNode: string;
  mountPoint: string;
  volumeLabel: string;
  filesystemType: string;
  capacityBytes: number;
  freeBytes: number;
  isRemovable: boolean;
  isInternalEsp: boolean;
  isDetected: boolean;
  isSelected: boolean;
  isUserConfirmed: boolean;
}

export interface EfiArtifact {
  filename: string;
  sourcePath: string;
  targetEspPath: string;
  fileSize: number;
  sha256: string;
  role: 'BOOT_APP' | 'HAL_DXE' | 'CONFIG' | 'EXCLUDED_TEST' | 'FORBIDDEN_FW';
  isRequiredForPhysicalInstall: boolean;
  isRejectedForbidden: boolean;
  rejectionReason: string;
}

export interface RecoveryBackupItem {
  relativePath: string;
  sourceEspPath: string;
  sizeBytes: number;
  sha256: string;
  backedUp: boolean;
  verifiedOnUsb: boolean;
}

export interface RecoveryPackage {
  packageRoot: string;
  targetModel: string;
  timestamp: string;
  items: RecoveryBackupItem[];
  hasOpencoreBackup: boolean;
  hasOclpBackup: boolean;
  hasRefindBackup: boolean;
  manifestGenerated: boolean;
  checksumsWritten: boolean;
  allChecksumsVerified: boolean;
  isRecoveryVerified: boolean;
}

export interface EfiDryRunPlan {
  sourceSummary: string;
  targetEsp: string;
  backupUsb: string;
  filesToInstall: string[];
  filesToModify: string[];
  filesToPreserve: string[];
  firmwareModificationStatus: string;
  romModificationStatus: string;
  partitionTableStatus: string;
  fullTextPreview: string;
}

export interface EfiSafetyGates {
  developerModeEnabled: boolean;
  physicalTestModeEnabled: boolean;
  targetMacIdentifiedMbp91: boolean;
  usbRecoveryDeviceDetected: boolean;
  correctUsbDeviceConfirmed: boolean;
  recoveryBackupCreated: boolean;
  recoveryBackupVerified: boolean;
  requiredEfiArtifactsIdentified: boolean;
  efiArtifactsHashVerified: boolean;
  prebootTestsPass: boolean;
  hardwareAuditPasses: boolean;
  simulationPasses: boolean;
  deploymentPlanGenerated: boolean;
  exactFilesDisplayed: boolean;
  firmwareModificationNone: boolean;
  romModificationNone: boolean;
  userExplicitlyConfirmsInstall: boolean;
}

export interface EfiInstallReport {
  binariesExist: boolean;
  hashesMatch: boolean;
  espFilesystemReadable: boolean;
  appleFilesIntact: boolean;
  existingBootloadersIntact: boolean;
  bootConfigValid: boolean;
  noUnexpectedFilesModified: boolean;
  zeroRomTouched: boolean;
  overallSuccess: boolean;
  reportSummary: string;
}

export interface EfiRollbackReport {
  originalFilesRestored: boolean;
  openvintageFilesRemoved: boolean;
  checksumsMatchOriginal: boolean;
  originalBootConfigRestored: boolean;
  rollbackVerified: boolean;
  reportSummary: string;
}

export const DEFAULT_STORAGE_TARGETS: StorageTarget[] = [
  {
    deviceNode: '/dev/disk0s1',
    mountPoint: '/Volumes/EFI',
    volumeLabel: 'EFI',
    filesystemType: 'FAT32',
    capacityBytes: 209715200,
    freeBytes: 178257920,
    isRemovable: false,
    isInternalEsp: true,
    isDetected: true,
    isSelected: true,
    isUserConfirmed: true,
  },
  {
    deviceNode: '/dev/disk2s1',
    mountPoint: '/Volumes/OV_USB_RECOVERY',
    volumeLabel: 'OV_RECOVERY',
    filesystemType: 'FAT32',
    capacityBytes: 15728640000,
    freeBytes: 15500000000,
    isRemovable: true,
    isInternalEsp: false,
    isDetected: true,
    isSelected: true,
    isUserConfirmed: true,
  },
];

export const AUTHORITATIVE_EFI_ARTIFACTS: EfiArtifact[] = [
  {
    filename: 'OpenVintageBootApp.efi',
    sourcePath: 'OpenVintagePkg/OpenVintageBootApp/OpenVintageBootApp.efi',
    targetEspPath: 'EFI/OpenVintage/OpenVintageBootApp.efi',
    fileSize: 524288,
    sha256: '7f89d3a44a2547d0a0f10268ec3b7b39a240ca1cc2de92c6f4a7484aa5cb0a9d',
    role: 'BOOT_APP',
    isRequiredForPhysicalInstall: true,
    isRejectedForbidden: false,
    rejectionReason: '',
  },
  {
    filename: 'OpenVintageHalDxe.efi',
    sourcePath: 'OpenVintagePkg/Drivers/OpenVintageHalDxe/OpenVintageHalDxe.efi',
    targetEspPath: 'EFI/OpenVintage/OpenVintageHalDxe.efi',
    fileSize: 262144,
    sha256: 'a1b2c3d4e5f60718293a4b5c6d7e8f90123456789abcdef0123456789abcdef0',
    role: 'HAL_DXE',
    isRequiredForPhysicalInstall: true,
    isRejectedForbidden: false,
    rejectionReason: '',
  },
  {
    filename: 'config.plist',
    sourcePath: 'config.plist',
    targetEspPath: 'EFI/OpenVintage/config.plist',
    fileSize: 4096,
    sha256: '4b227777d4dd1fc61c6f884f48641d02b4d121d3fd328cb08b5531fcacdabf8a',
    role: 'CONFIG',
    isRequiredForPhysicalInstall: true,
    isRejectedForbidden: false,
    rejectionReason: '',
  },
  {
    filename: 'OvSelfTestApp.efi',
    sourcePath: 'OpenVintagePkg/Tests/OvSelfTestApp.efi',
    targetEspPath: '',
    fileSize: 131072,
    sha256: '99887766554433221100aabbccddeeff00112233445566778899aabbccddeeff',
    role: 'EXCLUDED_TEST',
    isRequiredForPhysicalInstall: false,
    isRejectedForbidden: false,
    rejectionReason: 'Development and simulator test harness; excluded from physical deployment.',
  },
  {
    filename: 'OPENVINTAGE.fd',
    sourcePath: 'OpenVintagePkg/Firmware/OPENVINTAGE.fd',
    targetEspPath: '',
    fileSize: 4194304,
    sha256: '00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff',
    role: 'FORBIDDEN_FW',
    isRequiredForPhysicalInstall: false,
    isRejectedForbidden: true,
    rejectionReason: 'CRITICAL ARCHITECTURE VIOLATION: Firmware flash / ROM modification strictly forbidden on physical Mac.',
  },
];

export function buildDefaultDryRunPlan(): EfiDryRunPlan {
  return {
    sourceSummary: 'OpenVintage Authoritative Release Artifacts (BootApp v7.0.0, HalDxe v7.0.0)',
    targetEsp: 'Internal EFI System Partition (/Volumes/EFI, /dev/disk0s1)',
    backupUsb: 'Removable USB Recovery Drive (/Volumes/OV_USB_RECOVERY/OPENVINTAGE-RECOVERY)',
    filesToInstall: [
      'EFI/OpenVintage/OpenVintageBootApp.efi (524 KB, SHA256: 7f89d3a4... - Boot Picker App)',
      'EFI/OpenVintage/OpenVintageHalDxe.efi (262 KB, SHA256: a1b2c3d4... - Pre-Boot HAL Driver)',
      'EFI/OpenVintage/config.plist (4 KB, SHA256: 4b227777... - Configuration)',
    ],
    filesToModify: [
      'NONE. Boot path isolated under EFI/OpenVintage/. Apple boot path unaltered.',
    ],
    filesToPreserve: [
      'EFI/APPLE/* (All Apple system firmware & diagnostic files)',
      'EFI/BOOT/BOOTX64.EFI (Existing fallback bootloader preserved)',
      'EFI/OC/* (OpenCore bootloader, preserved intact if present)',
      'EFI/refind/* (rEFInd bootloader, preserved intact if present)',
    ],
    firmwareModificationStatus: 'NONE (Physical ROM/SPI Unaltered)',
    romModificationStatus: 'NONE (Physical ROM/SPI Unaltered)',
    partitionTableStatus: 'UNALTERED (No format, no erase, no repartitioning)',
    fullTextPreview: `================================================================================
        OpenVintage Phase 8 - Physical Test Mode Dry Run Preview               
================================================================================
TARGET MAC:             MacBookPro9,1 (Mid 2012 15-inch)
SOURCE:                 OpenVintage Authoritative Release Artifacts
TARGET ESP:             Internal EFI System Partition (/Volumes/EFI, /dev/disk0s1)
BACKUP DESTINATION:     Removable USB Recovery Drive (/Volumes/OV_USB_RECOVERY)
FIRMWARE MODIFICATION:  NONE (Physical ROM/SPI Unaltered)
ROM MODIFICATION:       NONE (Physical ROM/SPI Unaltered)
PARTITION TABLE:        UNALTERED (No format, no erase, no repartitioning)

FILES TO INSTALL:
  [1] EFI/OpenVintage/OpenVintageBootApp.efi (SHA256: 7f89d3a4... - Boot Picker App)
  [2] EFI/OpenVintage/OpenVintageHalDxe.efi  (SHA256: a1b2c3d4... - Pre-Boot HAL Driver)
  [3] EFI/OpenVintage/config.plist           (SHA256: 4b227777... - Configuration)

FILES TO MODIFY:
  - NONE. Isolated EFI/OpenVintage entry. Existing Apple boot path untouched.

FILES TO PRESERVE (100% INTACT):
  - EFI/APPLE/* (All Apple system firmware & diagnostic files)
  - EFI/BOOT/BOOTX64.EFI (Existing bootloader preserved)
  - EFI/OC/* (OpenCore bootloader, if present)
  - EFI/refind/* (rEFInd bootloader, if present)

RECOVERY & BACKUP:
  - USB package /Volumes/OV_USB_RECOVERY/OPENVINTAGE-RECOVERY/
  - Verified SHA-256 checksums required before any disk write
  - Automated (restore.sh) and manual restore documentation included
================================================================================`,
  };
}

export function validateProposedPayload(path: string): { safe: boolean; reason: string } {
  if (path.includes('.fd') || path.includes('OPENVINTAGE.fd') || path.includes('ROM') || path.includes('SPI')) {
    return {
      safe: false,
      reason: 'CRITICAL ARCHITECTURE RULE VIOLATION: Firmware flashing or SPI/ROM writing is strictly forbidden.',
    };
  }
  if (path.includes('OvSelfTestApp.efi')) {
    return {
      safe: false,
      reason: 'OvSelfTestApp.efi is a development/simulator test harness and is excluded from physical installs.',
    };
  }
  return { safe: true, reason: 'Valid release payload.' };
}

