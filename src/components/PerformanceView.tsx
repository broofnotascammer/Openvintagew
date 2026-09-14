import React from 'react';
import { 
  Zap, 
  Gamepad2, 
  Scale, 
  BatteryCharging, 
  Shield, 
  Sliders, 
  Check, 
  Info,
  Flame,
  ArrowRight
} from 'lucide-react';
import { 
  PERFORMANCE_PROFILES, 
  PerformanceProfileDef, 
  PerformanceProfileType,
  HardwareProfileData 
} from '../core/openvintageState';
import { GlassPanel } from './GlassPanel';
import { StatusBadge } from './StatusBadge';

interface PerformanceViewProps {
  currentProfile: PerformanceProfileDef;
  hardwareProfile: HardwareProfileData;
  onSelectProfile: (profileId: PerformanceProfileType) => void;
}

export const PerformanceView: React.FC<PerformanceViewProps> = ({
  currentProfile,
  hardwareProfile,
  onSelectProfile,
}) => {
  const activeGpu = hardwareProfile.gpus[hardwareProfile.activeGpuIndex] || hardwareProfile.gpus[0];

  const getProfileIcon = (id: PerformanceProfileType) => {
    switch (id) {
      case 'MAX_PERFORMANCE':
        return <Zap className="w-5 h-5 text-amber-500" />;
      case 'GAMING':
        return <Gamepad2 className="w-5 h-5 text-violet-500" />;
      case 'BALANCED':
        return <Scale className="w-5 h-5 text-blue-500" />;
      case 'EFFICIENCY':
        return <BatteryCharging className="w-5 h-5 text-emerald-500" />;
      case 'COMPATIBILITY':
        return <Shield className="w-5 h-5 text-sky-500" />;
      case 'CUSTOM':
        return <Sliders className="w-5 h-5 text-neutral-400" />;
    }
  };

  return (
    <div id="performance-page-view" className="space-y-6">
      {/* Intro Banner with Honest Engineering Disclaimer */}
      <div className="p-5 rounded-xl glass-card-light dark:glass-card-dark border border-black/10 dark:border-white/10 space-y-3">
        <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3">
          <div>
            <h2 className="text-xl font-bold tracking-tight text-neutral-900 dark:text-neutral-100 flex items-center gap-2">
              <Zap className="w-5 h-5 text-blue-500" />
              <span>Hardware Performance Profiles</span>
            </h2>
            <p className="text-xs text-neutral-500 dark:text-neutral-400 mt-0.5">
              Coordinated power governor, GPU switching policies, and translation caching tailored for {hardwareProfile.marketingName}.
            </p>
          </div>

          <div className="flex items-center gap-2">
            <span className="text-xs text-neutral-400">Active Profile:</span>
            <StatusBadge label={currentProfile.name} tone="emerald" />
          </div>
        </div>

        {/* Engineering Honesty Notice */}
        <div className="p-3 rounded-lg bg-blue-500/5 dark:bg-blue-500/10 border border-blue-500/20 text-xs text-neutral-600 dark:text-neutral-300 flex items-start gap-2.5">
          <Info className="w-4 h-4 text-blue-500 shrink-0 mt-0.5" />
          <span>
            <strong>Engineering Integrity Notice:</strong> OpenVintage selects the optimal hardware configuration, GMUX graphics bindings, and IR translation pipeline for your detected silicon. It does not perform unsafe voltage overclocking or make impossible performance promises.
          </span>
        </div>
      </div>

      {/* Profile Selector Cards Grid */}
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
        {PERFORMANCE_PROFILES.map((prof) => {
          const isSelected = currentProfile.id === prof.id;

          return (
            <div
              key={prof.id}
              id={`perf-card-${prof.id}`}
              onClick={() => onSelectProfile(prof.id)}
              className={`rounded-xl p-5 border cursor-pointer transition-all flex flex-col justify-between ${
                isSelected
                  ? 'border-blue-500/50 bg-blue-500/5 dark:bg-blue-500/10 ring-1 ring-blue-500/30 shadow-xs'
                  : 'glass-card-light dark:glass-card-dark border-black/5 dark:border-white/5 hover:border-black/20 dark:hover:border-white/20'
              }`}
            >
              <div className="space-y-3">
                <div className="flex items-start justify-between gap-3">
                  <div className="flex items-center gap-3">
                    <div className="p-2.5 rounded-lg bg-black/5 dark:bg-white/5">
                      {getProfileIcon(prof.id)}
                    </div>
                    <div>
                      <h3 className="text-sm font-bold text-neutral-900 dark:text-neutral-100">
                        {prof.name}
                      </h3>
                      <span className="text-[11px] font-mono text-neutral-400">{prof.badge}</span>
                    </div>
                  </div>

                  {isSelected && (
                    <div className="w-5 h-5 rounded-full bg-blue-600 text-white flex items-center justify-center shrink-0">
                      <Check className="w-3 h-3" />
                    </div>
                  )}
                </div>

                <p className="text-xs text-neutral-600 dark:text-neutral-400 leading-relaxed">
                  {prof.summary}
                </p>

                {/* Micro-specs */}
                <div className="space-y-1.5 pt-2 border-t border-black/5 dark:border-white/5 text-[11px] font-mono">
                  <div className="flex justify-between">
                    <span className="text-neutral-400">GPU Policy:</span>
                    <span className="text-neutral-800 dark:text-neutral-200 truncate max-w-[170px]">{prof.gpuPolicy}</span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-neutral-400">Latency Target:</span>
                    <span className="text-neutral-800 dark:text-neutral-200">{prof.latencyTarget}</span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-neutral-400">Thermal Envelope:</span>
                    <span className="text-neutral-800 dark:text-neutral-200">{prof.powerLimit}</span>
                  </div>
                </div>
              </div>

              <div className="mt-4 pt-3 border-t border-black/5 dark:border-white/5 flex items-center justify-between">
                <span className={`text-xs font-semibold ${isSelected ? 'text-blue-600 dark:text-blue-400' : 'text-neutral-500'}`}>
                  {isSelected ? 'Currently Applied' : 'Click to Apply Profile'}
                </span>
                {!isSelected && <ArrowRight className="w-3.5 h-3.5 text-neutral-400" />}
              </div>
            </div>
          );
        })}
      </div>

      {/* Deep-Dive Configuration Panel of Active Profile */}
      <GlassPanel
        id="active-profile-details-panel"
        title={`Active Configuration: ${currentProfile.name}`}
        action={
          <StatusBadge label="ACTIVE IN OPENCORE & PREBOOT" tone="emerald" />
        }
      >
        <div className="grid grid-cols-1 md:grid-cols-2 gap-6 text-xs font-mono">
          <div className="space-y-3">
            <div className="p-3 rounded-lg bg-black/5 dark:bg-white/5 space-y-1">
              <span className="text-[10px] text-neutral-400 uppercase tracking-wider block">Graphics &amp; GMUX Rule</span>
              <div className="text-neutral-900 dark:text-neutral-100 font-semibold">{currentProfile.gpuPolicy}</div>
              <p className="text-neutral-500 dark:text-neutral-400 text-[11px]">
                Currently routing through {activeGpu.modelName} ({activeGpu.vramMB} MB VRAM).
              </p>
            </div>

            <div className="p-3 rounded-lg bg-black/5 dark:bg-white/5 space-y-1">
              <span className="text-[10px] text-neutral-400 uppercase tracking-wider block">CPU Frequency Governor</span>
              <div className="text-neutral-900 dark:text-neutral-100 font-semibold">{currentProfile.cpuGovernor}</div>
              <p className="text-neutral-500 dark:text-neutral-400 text-[11px]">
                {hardwareProfile.cpu.physicalCores} Cores bound to {hardwareProfile.cpu.microarchitecture} frequency tables.
              </p>
            </div>
          </div>

          <div className="space-y-3">
            <div className="p-3 rounded-lg bg-black/5 dark:bg-white/5 space-y-1">
              <span className="text-[10px] text-neutral-400 uppercase tracking-wider block">Graphics Translation Engine</span>
              <div className="text-neutral-900 dark:text-neutral-100 font-semibold">{currentProfile.graphicsTranslation}</div>
              <p className="text-neutral-500 dark:text-neutral-400 text-[11px]">
                OVIR-GPU intermediate pipeline with cached state objects.
              </p>
            </div>

            <div className="p-3 rounded-lg bg-black/5 dark:bg-white/5 space-y-1">
              <span className="text-[10px] text-neutral-400 uppercase tracking-wider block">Acoustic &amp; Fan Policy</span>
              <div className="text-neutral-900 dark:text-neutral-100 font-semibold">{currentProfile.fanPolicy}</div>
              <p className="text-neutral-500 dark:text-neutral-400 text-[11px]">
                Dynamic fan controller keeps junction temperatures under 85°C.
              </p>
            </div>
          </div>
        </div>
      </GlassPanel>
    </div>
  );
};
