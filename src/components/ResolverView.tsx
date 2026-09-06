import React, { useState, useMemo } from 'react';
import { LEGACY_MAC_HARDWARE_PROFILES } from '../core/hardware';
import { SAMPLE_WORKLOADS } from '../core/phase1Data';
import { OVResolver } from '../core/resolver';
import { WorkloadRequest, HardwareProfile } from '../types';
import { ArrowRight, CheckCircle, AlertTriangle, Cpu, Disc, Zap, RotateCcw, Box } from 'lucide-react';

export const ResolverView: React.FC = () => {
  const [selectedHardwareId, setSelectedHardwareId] = useState<string>(LEGACY_MAC_HARDWARE_PROFILES[0].id);
  const [selectedWorkloadId, setSelectedWorkloadId] = useState<string>(SAMPLE_WORKLOADS[0].id);
  const [cachedKeys, setCachedKeys] = useState<Set<string>>(new Set(['ov_cache_0x7b2190ae']));
  const [customMsaa, setCustomMsaa] = useState<number>(4);
  const [customVram, setCustomVram] = useState<number>(1200);

  const selectedHardware = useMemo<HardwareProfile>(
    () => LEGACY_MAC_HARDWARE_PROFILES.find((h) => h.id === selectedHardwareId) || LEGACY_MAC_HARDWARE_PROFILES[0],
    [selectedHardwareId]
  );

  const baseWorkload = useMemo<WorkloadRequest>(
    () => SAMPLE_WORKLOADS.find((w) => w.id === selectedWorkloadId) || SAMPLE_WORKLOADS[0],
    [selectedWorkloadId]
  );

  const workload: WorkloadRequest = useMemo(
    () => ({
      ...baseWorkload,
      msaaRequested: customMsaa,
      vramRequiredMB: customVram,
    }),
    [baseWorkload, customMsaa, customVram]
  );

  const decision = useMemo(
    () => OVResolver.resolve(workload, selectedHardware, cachedKeys),
    [workload, selectedHardware, cachedKeys]
  );

  const toggleCache = () => {
    const next = new Set(cachedKeys);
    if (next.has(decision.cacheKey)) {
      next.delete(decision.cacheKey);
    } else {
      next.add(decision.cacheKey);
    }
    setCachedKeys(next);
  };

  const getPathBadge = (path: string) => {
    switch (path) {
      case 'native':
        return { bg: 'bg-emerald-950/80 text-emerald-300 border-emerald-800', label: 'NATIVE EXECUTION' };
      case 'translation':
        return { bg: 'bg-sky-950/80 text-sky-300 border-sky-800', label: 'OVIR-GPU TRANSLATION' };
      case 'simplification':
        return { bg: 'bg-amber-950/80 text-amber-300 border-amber-800', label: 'FEATURE SIMPLIFICATION' };
      case 'cache_hit':
        return { bg: 'bg-purple-950/80 text-purple-300 border-purple-800', label: 'PRE-COMPILED CACHE HIT' };
      case 'cpu_fallback':
        return { bg: 'bg-orange-950/80 text-orange-300 border-orange-800', label: 'CPU SIMD FALLBACK' };
      default:
        return { bg: 'bg-rose-950/80 text-rose-300 border-rose-800', label: 'UNSUPPORTED SILICON' };
    }
  };

  const badge = getPathBadge(decision.path);

  return (
    <div id="resolver-sandbox-view" className="space-y-6">
      {/* Intro Header */}
      <div className="bg-neutral-900/60 border border-neutral-800 rounded-xl p-6">
        <div className="flex flex-col md:flex-row md:items-center justify-between gap-4">
          <div>
            <h2 className="text-lg font-bold text-white font-mono flex items-center">
              <Zap className="w-5 h-5 mr-2 text-amber-400" />
              OVResolver — Intelligent Decision Layer
            </h2>
            <p className="text-xs text-neutral-400 mt-1">
              Evaluates incoming graphics/compute requests against hardware capability matrices. Resolves WHERE work goes: Native, OVIR-GPU translation, Feature Simplification, Pre-compiled Cache, or CPU Fallback.
            </p>
          </div>
          <button
            id="toggle-cache-btn"
            onClick={toggleCache}
            className={`px-3 py-1.5 rounded text-xs font-mono border transition-colors flex items-center ${
              cachedKeys.has(decision.cacheKey)
                ? 'bg-purple-950 text-purple-300 border-purple-700'
                : 'bg-neutral-800 text-neutral-300 border-neutral-700 hover:bg-neutral-700'
            }`}
          >
            <Disc className="w-3.5 h-3.5 mr-1.5" />
            {cachedKeys.has(decision.cacheKey) ? 'Key In Cache (Hit)' : 'Simulate Add to Cache'}
          </button>
        </div>
      </div>

      <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
        {/* Left Column: Workload Selector */}
        <div className="bg-neutral-900/40 border border-neutral-800 rounded-xl p-5 space-y-4">
          <h3 className="text-xs font-mono uppercase tracking-wider text-neutral-400 border-b border-neutral-800 pb-2 flex items-center">
            <Box className="w-4 h-4 mr-1.5 text-sky-400" />
            1. Workload Specification
          </h3>

          <div className="space-y-2">
            <label className="text-xs text-neutral-400">Preset Workload</label>
            <select
              id="workload-select"
              value={selectedWorkloadId}
              onChange={(e) => {
                setSelectedWorkloadId(e.target.value);
                const w = SAMPLE_WORKLOADS.find((x) => x.id === e.target.value);
                if (w) {
                  setCustomMsaa(w.msaaRequested);
                  setCustomVram(w.vramRequiredMB);
                }
              }}
              className="w-full bg-neutral-950 border border-neutral-800 rounded px-3 py-2 text-xs text-neutral-200 focus:border-amber-400 outline-none"
            >
              {SAMPLE_WORKLOADS.map((w) => (
                <option key={w.id} value={w.id}>
                  {w.name} ({w.api} {w.apiVersion})
                </option>
              ))}
            </select>
          </div>

          <div className="p-3 bg-neutral-950 border border-neutral-850 rounded text-xs space-y-1.5 font-mono">
            <div className="flex justify-between text-neutral-400">
              <span>Frontend API:</span>
              <span className="text-white font-semibold">{workload.api} {workload.apiVersion}</span>
            </div>
            <div className="flex justify-between text-neutral-400">
              <span>Shader Language:</span>
              <span className="text-amber-300">{workload.shaderLanguage}</span>
            </div>
            <div className="flex justify-between text-neutral-400">
              <span>Compute Required:</span>
              <span className={workload.requiresCompute ? 'text-amber-400' : 'text-neutral-500'}>
                {workload.requiresCompute ? 'Yes (CS 5.0)' : 'No'}
              </span>
            </div>
            <div className="flex justify-between text-neutral-400">
              <span>Tessellation:</span>
              <span className={workload.requiresTessellation ? 'text-amber-400' : 'text-neutral-500'}>
                {workload.requiresTessellation ? 'Yes' : 'No'}
              </span>
            </div>
          </div>

          {/* Interactive Workload Sliders */}
          <div className="space-y-3 pt-2">
            <div>
              <div className="flex justify-between text-xs text-neutral-400 mb-1">
                <span>Requested MSAA Level:</span>
                <span className="text-amber-400 font-mono">{customMsaa}x</span>
              </div>
              <input
                type="range"
                min="1"
                max="8"
                step="2"
                value={customMsaa}
                onChange={(e) => setCustomMsaa(Number(e.target.value))}
                className="w-full accent-amber-400 cursor-pointer"
              />
            </div>

            <div>
              <div className="flex justify-between text-xs text-neutral-400 mb-1">
                <span>VRAM Footprint Requirement:</span>
                <span className="text-amber-400 font-mono">{customVram} MB</span>
              </div>
              <input
                type="range"
                min="256"
                max="3072"
                step="128"
                value={customVram}
                onChange={(e) => setCustomVram(Number(e.target.value))}
                className="w-full accent-amber-400 cursor-pointer"
              />
            </div>
          </div>
        </div>

        {/* Center Column: Hardware Target */}
        <div className="bg-neutral-900/40 border border-neutral-800 rounded-xl p-5 space-y-4">
          <h3 className="text-xs font-mono uppercase tracking-wider text-neutral-400 border-b border-neutral-800 pb-2 flex items-center">
            <Cpu className="w-4 h-4 mr-1.5 text-amber-400" />
            2. Target Silicon Platform
          </h3>

          <div className="space-y-2">
            <label className="text-xs text-neutral-400">Select Legacy Mac Architecture</label>
            <select
              id="hardware-target-select"
              value={selectedHardwareId}
              onChange={(e) => setSelectedHardwareId(e.target.value)}
              className="w-full bg-neutral-950 border border-neutral-800 rounded px-3 py-2 text-xs text-neutral-200 focus:border-amber-400 outline-none"
            >
              {LEGACY_MAC_HARDWARE_PROFILES.map((h) => (
                <option key={h.id} value={h.id}>
                  {h.systemName} [{h.family}]
                </option>
              ))}
            </select>
          </div>

          <div className="p-3 bg-neutral-950 border border-neutral-850 rounded text-xs space-y-1.5 font-mono">
            <div className="flex justify-between text-neutral-400">
              <span>CPU Silicon:</span>
              <span className="text-white text-right">{selectedHardware.cpu.model}</span>
            </div>
            <div className="flex justify-between text-neutral-400">
              <span>GPU Model:</span>
              <span className="text-white text-right">{selectedHardware.gpu.model}</span>
            </div>
            <div className="flex justify-between text-neutral-400">
              <span>Physical VRAM:</span>
              <span className="text-amber-400">{selectedHardware.gpu.vramMB} MB</span>
            </div>
            <div className="flex justify-between text-neutral-400">
              <span>Max MSAA Limit:</span>
              <span className="text-neutral-300">{selectedHardware.gpu.limits.msaaMax}x</span>
            </div>
            <div className="flex justify-between text-neutral-400">
              <span>Native OpenGL:</span>
              <span className="text-emerald-400">{selectedHardware.gpu.nativeApis.openGL}</span>
            </div>
            <div className="flex justify-between text-neutral-400">
              <span>Native Metal:</span>
              <span className={selectedHardware.gpu.nativeApis.metal ? 'text-emerald-400' : 'text-neutral-500'}>
                {selectedHardware.gpu.nativeApis.metal || 'None'}
              </span>
            </div>
          </div>

          <div className="text-[11px] text-neutral-400 bg-neutral-950/60 p-2.5 rounded border border-neutral-900 space-y-1">
            <div className="font-semibold text-neutral-300">Active Quirks:</div>
            {selectedHardware.gpu.quirks.map((q, idx) => (
              <div key={idx} className="text-neutral-400 flex items-start">
                <span className="text-amber-400 mr-1.5">▪</span>
                <span>{q}</span>
              </div>
            ))}
          </div>
        </div>

        {/* Right Column: Resolver Decision */}
        <div className="bg-neutral-950 border border-neutral-800 rounded-xl p-5 flex flex-col justify-between">
          <div>
            <h3 className="text-xs font-mono uppercase tracking-wider text-neutral-400 border-b border-neutral-800 pb-2 flex items-center">
              <Zap className="w-4 h-4 mr-1.5 text-amber-400" />
              3. OVResolver Decision
            </h3>

            <div className="mt-4 space-y-4">
              <div>
                <span className="text-[11px] text-neutral-500 font-mono uppercase tracking-wider block mb-1">
                  Assigned Execution Path
                </span>
                <div className={`px-3 py-2 rounded border font-mono font-bold text-xs ${badge.bg}`}>
                  {badge.label}
                </div>
              </div>

              <div>
                <span className="text-[11px] text-neutral-500 font-mono uppercase tracking-wider block mb-1">
                  Target Backend
                </span>
                <div className="text-xs font-mono text-white bg-neutral-900 px-3 py-2 rounded border border-neutral-800">
                  {decision.targetBackend}
                </div>
              </div>

              <div>
                <span className="text-[11px] text-neutral-500 font-mono uppercase tracking-wider block mb-1">
                  Decision Heuristics &amp; Rationale
                </span>
                <p className="text-xs text-neutral-300 bg-neutral-900/60 p-3 rounded border border-neutral-850 leading-relaxed">
                  {decision.rationale}
                </p>
              </div>

              {decision.clampedFeatures.length > 0 && (
                <div>
                  <span className="text-[11px] text-amber-400 font-mono uppercase tracking-wider block mb-1 flex items-center">
                    <AlertTriangle className="w-3.5 h-3.5 mr-1" />
                    Feature Adjustments / Clamps
                  </span>
                  <ul className="text-xs space-y-1 bg-amber-950/20 border border-amber-900/30 p-2.5 rounded">
                    {decision.clampedFeatures.map((f, idx) => (
                      <li key={idx} className="text-amber-300 flex items-start text-[11px]">
                        <span className="mr-1.5 text-amber-500">›</span>
                        <span>{f}</span>
                      </li>
                    ))}
                  </ul>
                </div>
              )}

              <div className="grid grid-cols-2 gap-2 pt-1 font-mono text-[11px]">
                <div className="bg-neutral-900 p-2 rounded border border-neutral-800">
                  <div className="text-neutral-500">Translation Latency</div>
                  <div className="text-white font-bold">{decision.estimatedTranslationOverheadMs} ms</div>
                </div>
                <div className="bg-neutral-900 p-2 rounded border border-neutral-800">
                  <div className="text-neutral-500">Heuristic Confidence</div>
                  <div className="text-emerald-400 font-bold">{(decision.confidenceScore * 100).toFixed(0)}%</div>
                </div>
              </div>
            </div>
          </div>

          <div className="mt-4 pt-3 border-t border-neutral-900 flex justify-between items-center text-[10px] font-mono text-neutral-500">
            <span>Key: {decision.cacheKey}</span>
            <span className="text-amber-400">OVResolver v2.0</span>
          </div>
        </div>
      </div>
    </div>
  );
};
