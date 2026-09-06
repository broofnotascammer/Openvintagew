/**
 * OpenVintage - Core Type Definitions
 * Systems Engineering Specification Revision 2.0
 */

export type PlatformFamily = 
  | 'Early Intel Mac (2006-2009)'
  | 'Nehalem / Westmere (2009-2010)'
  | 'Sandy Bridge (2011)'
  | 'Ivy Bridge (2012)'
  | 'Haswell (2013-2014)'
  | 'Broadwell / Skylake (2015)'
  | 'Generic x86-64 Target';

export interface CpuSpec {
  model: string;
  architecture: 'x86' | 'x86_64' | 'ARM64' | 'RISC-V';
  microarchitecture: string;
  physicalCores: number;
  logicalThreads: number;
  baseClockGhz: number;
  maxTurboGhz: number;
  instructionSets: string[];
  tdpWatts: number;
}

export interface GpuSpec {
  vendor: 'Intel' | 'Nvidia' | 'AMD' | 'Software';
  model: string;
  architecture: string;
  vramMB: number;
  pcieGen: string;
  nativeApis: {
    openGL: string;
    openCL: string;
    metal: string | null;
    vulkan: string | null;
    directX: string;
  };
  limits: {
    maxTextureSize: number;
    maxColorAttachments: number;
    computeSupported: boolean;
    tessellationSupported: boolean;
    geometryShaderSupported: boolean;
    anisotropicMax: number;
    msaaMax: number;
  };
  quirks: string[];
}

export interface HardwareProfile {
  id: string;
  systemName: string;
  modelIdentifier: string;
  releaseYear: number;
  family: PlatformFamily;
  cpu: CpuSpec;
  gpu: GpuSpec;
  ramMB: number;
  storageType: 'SATA HDD' | 'SATA SSD' | 'Apple Proprietary PCIe SSD' | 'NVMe';
  efiBitness: '32-bit' | '64-bit';
}

export type GraphicsApi = 'Metal' | 'Vulkan' | 'OpenGL' | 'DirectX';

export interface WorkloadRequest {
  id: string;
  name: string;
  api: GraphicsApi;
  apiVersion: string;
  requiresCompute: boolean;
  requiresTessellation: boolean;
  drawCallsPerFrame: number;
  vramRequiredMB: number;
  msaaRequested: number;
  targetFramerate: number;
  shaderLanguage: 'MSL' | 'SPIR-V' | 'GLSL' | 'HLSL';
}

export type ExecutionPath = 
  | 'native'
  | 'translation'
  | 'simplification'
  | 'cache_hit'
  | 'cpu_fallback'
  | 'unsupported';

export interface ResolverDecision {
  path: ExecutionPath;
  targetBackend: string;
  rationale: string;
  clampedFeatures: string[];
  estimatedTranslationOverheadMs: number;
  cacheKey: string;
  confidenceScore: number;
  cpuFallbackUsed: boolean;
}

export type PerformanceProfile = 
  | 'battery'
  | 'balanced'
  | 'performance'
  | 'max_performance'
  | 'developer';

export interface CoreAffinity {
  coreId: number;
  isHyperthread: boolean;
  assignedRole: 'primary' | 'worker' | 'io' | 'idle';
  loadPercent: number;
}

export interface SchedulerState {
  currentProfile: PerformanceProfile;
  totalPhysicalCores: number;
  totalThreads: number;
  cores: CoreAffinity[];
  workerPoolSize: number;
  cooperativeOsScheduling: boolean;
  thermalThrottled: boolean;
  thermalTempC: number;
  memoryUsageMB: number;
  memoryCapacityMB: number;
}

export interface CacheEntry {
  id: string;
  type: 'shader' | 'pipeline' | 'texture' | 'resolution';
  sourceHash: string;
  compiledHash: string;
  sizeBytes: number;
  hitCount: number;
  lastAccessed: string;
  targetHardwareId: string;
}

export interface Phase1Artifacts {
  efiVersion: string;
  buildTarget: string;
  toolchain: string;
  qemuStatus: 'PASS' | 'FAIL';
  ovmfStatus: 'PASS' | 'FAIL';
  devicesEnumerated: number;
  blockIoDevices: string[];
  devicePathsDetected: string[];
  exitCode: string;
  timestamp: string;
}
