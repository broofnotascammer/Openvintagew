import React, { useState } from 'react';
import { 
  Layers, 
  Play, 
  CheckCircle2, 
  AlertTriangle, 
  Cpu, 
  Monitor, 
  Zap, 
  RotateCcw,
  Sparkles,
  Sliders,
  ShieldAlert,
  Info
} from 'lucide-react';
import { 
  HardwareProfileData, 
  PRESET_SIMULATED_PROFILES, 
  TARGET_OS_LIST,
  evaluateOsCompatibility,
  PERFORMANCE_PROFILES,
  PerformanceProfileType
} from '../core/openvintageState';
import { GlassPanel } from './GlassPanel';
import { StatusBadge } from './StatusBadge';

interface SimulatorViewProps {
  currentProfile: HardwareProfileData;
  onApplyProfile: (profile: HardwareProfileData) => void;
}

export const SimulatorView: React.FC<SimulatorViewProps> = ({
  currentProfile,
  onApplyProfile,
}) => {
  const [selectedPresetId, setSelectedPresetId] = useState<string>('mbp91');
  const [activePreset, setActivePreset] = useState<HardwareProfileData>(
    PRESET_SIMULATED_PROFILES.find((p) => p.id === 'mbp91') || PRESET_SIMULATED_PROFILES[0]
  );
  
  // Customization controls
  const [customRamMB, setCustomRamMB] = useState<number>(activePreset.memory.totalMB);
  const [selectedActiveGpuIndex, setSelectedActiveGpuIndex] = useState<number>(activePreset.activeGpuIndex);
  const [targetOs, setTargetOs] = useState<string>('macOS 12 Monterey');
  const [perfProfile, setPerfProfile] = useState<PerformanceProfileType>('BALANCED');

  // Simulation execution state
  const [isSimulating, setIsSimulating] = useState<boolean>(false);
  const [simulationResult, setSimulationResult] = useState<{
    ran: boolean;
    timestamp: string;
    compatResult: ReturnType<typeof evaluateOsCompatibility>;
    ovResolverRoutes: string[];
    bootArgs: string[];
    warnings: string[];
  } | null>(null);

  const handlePresetSelect = (id: string) => {
    const found = PRESET_SIMULATED_PROFILES.find((p) => p.id === id);
    if (found) {
      setSelectedPresetId(id);
      setActivePreset(found);
      setCustomRamMB(found.memory.totalMB);
      setSelectedActiveGpuIndex(found.activeGpuIndex);
      setSimulationResult(null); // Reset until ran
    }
  };

  const handleRunSimulation = () => {
    setIsSimulating(true);
    setTimeout(() => {
      // Construct effective profile for simulation
      const effectiveProfile: HardwareProfileData = {
        ...activePreset,
        activeGpuIndex: selectedActiveGpuIndex,
        memory: {
          ...activePreset.memory,
          totalMB: customRamMB,
        },
      };

      const compat = evaluateOsCompatibility(targetOs, effectiveProfile);
      const activeGpu = effectiveProfile.gpus[selectedActiveGpuIndex] || effectiveProfile.gpus[0];
      const hasMetal = activeGpu.metalLevel.startsWith('Metal');
      const hasAvx2 = effectiveProfile.cpu.features.avx2;

      const routes: string[] = [];
      const bootArgs: string[] = ['keepsyms=1', 'debug=0x100', '-v'];
      const warnings: string[] = [];

      // Determine OVResolver translation pathways
      if (hasMetal) {
        routes.push(`OVIR-GPU: Native Metal 1.2 Pipeline -> Hardware GK107/HD4000 Direct Dispatch`);
        routes.push(`OVResolver: Fast-path cached shader compilation (SPIR-V -> AIR)`);
      } else {
        routes.push(`OVIR-GPU: Legacy GL 3.3 Translation Hub -> OpenGL Core Engine`);
        routes.push(`OVResolver: Fallback shader emulator for Metal shaders`);
        warnings.push('Active GPU lacks hardware Metal support. Desktop compositing relies on legacy OpenGL patches.');
      }

      if (!hasAvx2 && (targetOs.includes('Ventura') || targetOs.includes('Sonoma') || targetOs.includes('Sequoia'))) {
        routes.push(`OVIR-CPU: Rosetta Cryptex Dynamic JIT emulation for AVX2 instructions`);
        bootArgs.push('ipc_control_port_options=0', 'amfi_get_out_of_my_way=1');
        warnings.push('Non-AVX2 CPU detected. macOS Cryptex OS.dmg bypass injected into preboot chain.');
      }

      if (effectiveProfile.gpus.length > 1) {
        routes.push(`GMUX Controller: Switching lock enabled for ${activeGpu.modelName}`);
        bootArgs.push(`nv_disable=${selectedActiveGpuIndex === 0 ? '1' : '0'}`);
      }

      setSimulationResult({
        ran: true,
        timestamp: new Date().toLocaleTimeString(),
        compatResult: compat,
        ovResolverRoutes: routes,
        bootArgs,
        warnings,
      });
      setIsSimulating(false);
    }, 450);
  };

  const handleActivateInSession = () => {
    const customizedProfile: HardwareProfileData = {
      ...activePreset,
      activeGpuIndex: selectedActiveGpuIndex,
      memory: {
        ...activePreset.memory,
        totalMB: customRamMB,
      },
    };
    onApplyProfile(customizedProfile);
  };

  return (
    <div id="simulator-page-view" className="space-y-6">
      {/* Prominent Simulated Hardware Source Banner */}
      <div className="p-4 rounded-xl bg-amber-500/10 border border-amber-500/30 flex items-start justify-between gap-4">
        <div className="flex items-start gap-3">
          <div className="p-2 rounded-lg bg-amber-500/20 text-amber-600 dark:text-amber-400 shrink-0 mt-0.5">
            <Layers className="w-5 h-5" />
          </div>
          <div>
            <div className="flex items-center gap-2">
              <StatusBadge label="SOURCE: SIMULATED SILICON" tone="amber" pulse={true} />
              <span className="text-xs font-semibold text-amber-700 dark:text-amber-300">
                Sandbox Environment
              </span>
            </div>
            <p className="text-xs text-neutral-600 dark:text-neutral-300 mt-1 leading-relaxed">
              OpenVintage Simulator allows auditing how the OpenVintage boot picker, OVResolver, and 
              OCLP patch pipeline respond to various legacy Mac silicon architectures without altering physical hardware.
            </p>
          </div>
        </div>

        <button
          id="btn-apply-simulated-profile"
          onClick={handleActivateInSession}
          className="shrink-0 px-3 py-1.5 rounded-lg bg-amber-600 hover:bg-amber-700 text-white text-xs font-medium transition-colors shadow-xs"
        >
          Set as Active Profile
        </button>
      </div>

      {/* Preset Hardware Profile Selector */}
      <GlassPanel
        id="simulator-preset-selector-panel"
        title="1. Select Hardware Architecture Preset"
        action={
          <span className="text-xs text-neutral-400 font-mono">
            {PRESET_SIMULATED_PROFILES.length} Presets Available
          </span>
        }
      >
        <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-3">
          {PRESET_SIMULATED_PROFILES.map((preset) => {
            const isSelected = selectedPresetId === preset.id;
            return (
              <div
                key={preset.id}
                id={`preset-card-${preset.id}`}
                onClick={() => handlePresetSelect(preset.id)}
                className={`p-3.5 rounded-xl border cursor-pointer transition-all ${
                  isSelected
                    ? 'border-amber-500/50 bg-amber-500/10 shadow-xs ring-1 ring-amber-500/30'
                    : 'border-black/5 dark:border-white/5 bg-black/2 dark:bg-white/2 hover:border-black/15 dark:hover:border-white/15'
                }`}
              >
                <div className="flex items-center justify-between mb-1">
                  <span className="text-xs font-mono font-semibold text-neutral-900 dark:text-neutral-100">
                    {preset.modelIdentifier}
                  </span>
                  <span className="text-[10px] text-neutral-400">{preset.releaseYear}</span>
                </div>
                <div className="text-xs font-medium text-neutral-700 dark:text-neutral-300 truncate">
                  {preset.marketingName}
                </div>
                <div className="text-[11px] text-neutral-500 dark:text-neutral-400 font-mono mt-2 flex items-center justify-between">
                  <span>{preset.cpu.microarchitecture}</span>
                  <span>{preset.gpus.length} GPU{preset.gpus.length > 1 ? 's' : ''}</span>
                </div>
              </div>
            );
          })}
        </div>
      </GlassPanel>

      {/* Hardware Customization Matrix */}
      <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
        {/* Silicon Specs */}
        <GlassPanel id="simulator-specs-panel" title="Hardware Profile Parameters" className="lg:col-span-1">
          <div className="space-y-3 text-xs font-mono">
            <div className="p-3 rounded-lg bg-black/5 dark:bg-white/5 space-y-1.5">
              <span className="text-[11px] text-neutral-400 uppercase tracking-wider block">Processor</span>
              <div className="text-neutral-900 dark:text-neutral-100 font-semibold">{activePreset.cpu.modelName}</div>
              <div className="text-neutral-500 dark:text-neutral-400">
                {activePreset.cpu.physicalCores} Cores / {activePreset.cpu.logicalThreads} Threads @ {activePreset.cpu.baseClockGhz} GHz
              </div>
            </div>

            {/* RAM Capacity Picker */}
            <div className="p-3 rounded-lg bg-black/5 dark:bg-white/5 space-y-2">
              <div className="flex items-center justify-between">
                <span className="text-[11px] text-neutral-400 uppercase tracking-wider">Memory Allocation</span>
                <span className="text-blue-600 dark:text-blue-400 font-bold">{(customRamMB / 1024).toFixed(0)} GB</span>
              </div>
              <div className="flex gap-1.5 flex-wrap">
                {[4096, 8192, 16384, 32768].map((ram) => (
                  <button
                    key={ram}
                    onClick={() => setCustomRamMB(ram)}
                    className={`px-2 py-1 rounded text-xs font-mono transition-colors ${
                      customRamMB === ram
                        ? 'bg-blue-600 text-white font-bold'
                        : 'bg-black/5 dark:bg-white/10 text-neutral-700 dark:text-neutral-300 hover:bg-black/10'
                    }`}
                  >
                    {ram / 1024}GB
                  </button>
                ))}
              </div>
            </div>

            {/* Active GPU Switcher */}
            <div className="p-3 rounded-lg bg-black/5 dark:bg-white/5 space-y-2">
              <span className="text-[11px] text-neutral-400 uppercase tracking-wider block">Active GPU Binding</span>
              <div className="space-y-1.5">
                {activePreset.gpus.map((gpu) => {
                  const isSelectedGpu = gpu.index === selectedActiveGpuIndex;
                  return (
                    <button
                      key={gpu.id}
                      onClick={() => setSelectedActiveGpuIndex(gpu.index)}
                      className={`w-full text-left p-2 rounded border text-xs transition-all flex items-center justify-between ${
                        isSelectedGpu
                          ? 'border-blue-500/50 bg-blue-500/10 text-blue-700 dark:text-blue-300 font-medium'
                          : 'border-black/5 dark:border-white/5 bg-transparent text-neutral-600 dark:text-neutral-400'
                      }`}
                    >
                      <span className="truncate max-w-[170px]">{gpu.modelName}</span>
                      <span className="text-[10px] font-mono">{gpu.isDiscrete ? 'dGPU' : 'iGPU'}</span>
                    </button>
                  );
                })}
              </div>
            </div>
          </div>
        </GlassPanel>

        {/* Simulation Execution & Target Configuration */}
        <GlassPanel id="simulator-config-panel" title="2. Simulation Execution &amp; Target OS" className="lg:col-span-2">
          <div className="space-y-4">
            <div className="grid grid-cols-1 sm:grid-cols-2 gap-4">
              {/* Target OS Selector */}
              <div>
                <label className="text-xs font-medium text-neutral-700 dark:text-neutral-300 block mb-1.5">
                  Target Operating System
                </label>
                <select
                  id="simulator-select-target-os"
                  value={targetOs}
                  onChange={(e) => setTargetOs(e.target.value)}
                  className="w-full text-xs font-mono p-2 rounded-lg border border-black/10 dark:border-white/10 bg-white/50 dark:bg-neutral-900/60 text-neutral-900 dark:text-neutral-100 focus:ring-2 focus:ring-blue-500"
                >
                  {TARGET_OS_LIST.map((os) => (
                    <option key={os} value={os}>
                      {os}
                    </option>
                  ))}
                </select>
              </div>

              {/* Performance Profile Selector */}
              <div>
                <label className="text-xs font-medium text-neutral-700 dark:text-neutral-300 block mb-1.5">
                  Simulated Performance Profile
                </label>
                <select
                  id="simulator-select-perf-profile"
                  value={perfProfile}
                  onChange={(e) => setPerfProfile(e.target.value as PerformanceProfileType)}
                  className="w-full text-xs font-mono p-2 rounded-lg border border-black/10 dark:border-white/10 bg-white/50 dark:bg-neutral-900/60 text-neutral-900 dark:text-neutral-100 focus:ring-2 focus:ring-blue-500"
                >
                  {PERFORMANCE_PROFILES.map((p) => (
                    <option key={p.id} value={p.id}>
                      {p.name}
                    </option>
                  ))}
                </select>
              </div>
            </div>

            {/* Run Simulation Trigger */}
            <div className="pt-2">
              <button
                id="btn-run-simulation"
                onClick={handleRunSimulation}
                disabled={isSimulating}
                className="w-full py-2.5 px-4 rounded-xl bg-blue-600 hover:bg-blue-700 active:bg-blue-800 text-white text-xs font-semibold shadow-xs flex items-center justify-center gap-2 transition-all disabled:opacity-50"
              >
                <Play className={`w-4 h-4 ${isSimulating ? 'animate-spin' : ''}`} />
                <span>{isSimulating ? 'Simulating OpenVintage Pipeline...' : 'Run Simulation Pipeline'}</span>
              </button>
            </div>

            {/* Simulation Results Sheet */}
            {simulationResult && (
              <div id="simulation-results-box" className="mt-4 p-4 rounded-xl bg-black/5 dark:bg-white/5 border border-black/10 dark:border-white/10 space-y-3">
                <div className="flex items-center justify-between border-b border-black/5 dark:border-white/5 pb-2">
                  <div className="flex items-center gap-2">
                    <CheckCircle2 className="w-4 h-4 text-emerald-500" />
                    <span className="text-xs font-bold text-neutral-900 dark:text-neutral-100">
                      Simulation Complete
                    </span>
                  </div>
                  <span className="text-[11px] font-mono text-neutral-400">
                    Timestamp: {simulationResult.timestamp}
                  </span>
                </div>

                {/* Compatibility Outcome */}
                <div className="flex items-center justify-between p-2.5 rounded-lg bg-black/5 dark:bg-white/5">
                  <div className="text-xs">
                    <span className="text-neutral-400 block text-[10px] uppercase">Compatibility Rating</span>
                    <span className="font-semibold text-neutral-900 dark:text-neutral-100">
                      {simulationResult.compatResult.ratingLabel}
                    </span>
                  </div>
                  <StatusBadge
                    label={simulationResult.compatResult.integrationRequired === 'None' ? 'Native' : 'OCLP Required'}
                    tone={simulationResult.compatResult.integrationRequired === 'None' ? 'emerald' : 'amber'}
                  />
                </div>

                {/* OVResolver Translation Decisions */}
                <div className="space-y-1 text-xs font-mono">
                  <span className="text-[10px] text-neutral-400 uppercase tracking-wider block">
                    OVResolver Dynamic Routing Decisions:
                  </span>
                  {simulationResult.ovResolverRoutes.map((route, i) => (
                    <div key={i} className="p-2 rounded bg-black/5 dark:bg-white/5 text-neutral-700 dark:text-neutral-300 text-[11px]">
                      • {route}
                    </div>
                  ))}
                </div>

                {/* Injected Preboot EFI Arguments */}
                <div className="space-y-1 text-xs font-mono">
                  <span className="text-[10px] text-neutral-400 uppercase tracking-wider block">
                    Generated EFI Boot Arguments:
                  </span>
                  <div className="p-2 rounded bg-black/5 dark:bg-white/5 text-blue-600 dark:text-blue-400 text-[11px] truncate">
                    boot-args="{simulationResult.bootArgs.join(' ')}"
                  </div>
                </div>

                {/* Warnings if any */}
                {simulationResult.warnings.length > 0 && (
                  <div className="p-2.5 rounded-lg bg-amber-500/10 border border-amber-500/20 text-xs text-amber-700 dark:text-amber-300 space-y-1">
                    <div className="font-bold flex items-center gap-1.5">
                      <AlertTriangle className="w-3.5 h-3.5" />
                      <span>Silicon Warnings</span>
                    </div>
                    {simulationResult.warnings.map((w, i) => (
                      <div key={i} className="text-[11px]">• {w}</div>
                    ))}
                  </div>
                )}
              </div>
            )}
          </div>
        </GlassPanel>
      </div>
    </div>
  );
};
