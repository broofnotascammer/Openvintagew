/**
 * OpenVintage - OVResolver Decision Engine
 * Deterministic path resolution and capability heuristic matrix.
 */

import { HardwareProfile, WorkloadRequest, ResolverDecision, ExecutionPath } from '../types';

export class OVResolver {
  /**
   * Generates a unique composite cache signature based on workload and target silicon.
   */
  public static generateCacheKey(workload: WorkloadRequest, hardware: HardwareProfile): string {
    const raw = `${workload.api}_${workload.apiVersion}_${workload.shaderLanguage}_${hardware.gpu.model}_${hardware.gpu.architecture}`;
    // Fast deterministic hash calculation
    let hash = 0;
    for (let i = 0; i < raw.length; i++) {
      const char = raw.charCodeAt(i);
      hash = (hash << 5) - hash + char;
      hash |= 0;
    }
    return `ov_cache_0x${Math.abs(hash).toString(16).padStart(8, '0')}`;
  }

  /**
   * Resolves the optimal execution route for a given workload on specified target hardware.
   */
  public static resolve(
    workload: WorkloadRequest,
    hardware: HardwareProfile,
    cachedKeys: Set<string>
  ): ResolverDecision {
    const cacheKey = this.generateCacheKey(workload, hardware);
    const clampedFeatures: string[] = [];

    // Check Cache First: If an identical compiled binary already exists in cache
    if (cachedKeys.has(cacheKey)) {
      return {
        path: 'cache_hit',
        targetBackend: `Cached Binary (${hardware.gpu.nativeApis.openGL || 'GOP'})`,
        rationale: `Found pre-validated compiled binary for key ${cacheKey}. Bypassing intermediate translation overhead.`,
        clampedFeatures: [],
        estimatedTranslationOverheadMs: 0.12,
        cacheKey,
        confidenceScore: 0.99,
        cpuFallbackUsed: false,
      };
    }

    const { gpu, cpu } = hardware;

    // Rule 1: Check native API match
    const hasNativeMetal = workload.api === 'Metal' && gpu.nativeApis.metal !== null;
    const hasNativeOpenGL = workload.api === 'OpenGL' && gpu.nativeApis.openGL !== null;
    const hasNativeVulkan = workload.api === 'Vulkan' && gpu.nativeApis.vulkan !== null;

    if (hasNativeMetal || hasNativeOpenGL || hasNativeVulkan) {
      // Check if feature requirements exceed silicon limits
      let needsSimplification = false;
      if (workload.msaaRequested > gpu.limits.msaaMax) {
        clampedFeatures.push(`MSAA clamped from ${workload.msaaRequested}x to ${gpu.limits.msaaMax}x`);
        needsSimplification = true;
      }
      if (workload.vramRequiredMB > gpu.vramMB * 0.85) {
        clampedFeatures.push(`VRAM constraint: Downsampling textures to fit ${gpu.vramMB}MB budget`);
        needsSimplification = true;
      }

      if (needsSimplification) {
        return {
          path: 'simplification',
          targetBackend: hasNativeMetal ? gpu.nativeApis.metal! : gpu.nativeApis.openGL,
          rationale: `Hardware natively supports ${workload.api}, but exceeds silicon memory/fill-rate envelope. Applied safety clamps.`,
          clampedFeatures,
          estimatedTranslationOverheadMs: 0.45,
          cacheKey,
          confidenceScore: 0.92,
          cpuFallbackUsed: false,
        };
      }

      return {
        path: 'native',
        targetBackend: hasNativeMetal ? gpu.nativeApis.metal! : gpu.nativeApis.openGL,
        rationale: `Hardware natively provides full driver compliance for ${workload.api} ${workload.apiVersion}. Direct dispatch enabled.`,
        clampedFeatures: [],
        estimatedTranslationOverheadMs: 0.05,
        cacheKey,
        confidenceScore: 0.98,
        cpuFallbackUsed: false,
      };
    }

    // Rule 2: Compute Shader evaluation
    let cpuFallback = false;
    if (workload.requiresCompute && !gpu.limits.computeSupported) {
      if (cpu.instructionSets.includes('AVX') || cpu.instructionSets.includes('SSE4.2')) {
        cpuFallback = true;
        clampedFeatures.push('GPU lacks compute shaders (OpenGL 3.3 / DX10 limit); compute kernels routed to CPU SIMD thread pool');
      } else {
        return {
          path: 'unsupported',
          targetBackend: 'None',
          rationale: `Workload strictly requires compute shaders, but GPU (${gpu.model}) lacks compute silicon and CPU lacks vector instructions.`,
          clampedFeatures: ['Compute execution rejected'],
          estimatedTranslationOverheadMs: 0,
          cacheKey,
          confidenceScore: 0.1,
          cpuFallbackUsed: false,
        };
      }
    }

    // Rule 3: Intermediate IR Translation (e.g. Metal / Vulkan / DirectX -> OVIR-GPU -> OpenGL backend)
    const availableBackend = gpu.nativeApis.openGL || 'UEFI_GOP_FRAMEBUFFER';

    if (workload.msaaRequested > gpu.limits.msaaMax) {
      clampedFeatures.push(`MSAA clamped from ${workload.msaaRequested}x to ${gpu.limits.msaaMax}x`);
    }

    if (workload.vramRequiredMB > gpu.vramMB) {
      clampedFeatures.push(`VRAM pressure: Active page paging enabled (${workload.vramRequiredMB}MB requested vs ${gpu.vramMB}MB available)`);
    }

    const estimatedOverhead = cpuFallback ? 4.8 : 1.9;

    return {
      path: cpuFallback ? 'cpu_fallback' : 'translation',
      targetBackend: `OVIR-GPU → ${availableBackend}${cpuFallback ? ' + CPU SIMD Vectorizer' : ''}`,
      rationale: cpuFallback
        ? `No native ${workload.api} support on ${gpu.model}. Rendering graphics commands through OVIR-GPU to ${availableBackend}, while dispatching compute to CPU worker threads.`
        : `Target silicon does not support ${workload.api} natively. Decomposed workload into OVIR-GPU command nodes targeting ${availableBackend}.`,
      clampedFeatures,
      estimatedTranslationOverheadMs: estimatedOverhead,
      cacheKey,
      confidenceScore: cpuFallback ? 0.78 : 0.88,
      cpuFallbackUsed: cpuFallback,
    };
  }
}
