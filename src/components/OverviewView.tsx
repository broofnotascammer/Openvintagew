import React from 'react';
import { 
  Laptop, 
  Cpu, 
  Monitor, 
  Zap, 
  HardDrive, 
  CheckCircle2, 
  Boxes, 
  ArrowRight,
  ShieldCheck,
  Activity,
  Layers,
  Sparkles,
  Info
} from 'lucide-react';
import { 
  HardwareProfileData, 
  PerformanceProfileDef, 
  BootTarget, 
  IntegrationStatus 
} from '../core/openvintageState';
import { GlassPanel, MetricCard } from './GlassPanel';
import { StatusBadge } from './StatusBadge';
import { NavTab } from './Sidebar';

interface OverviewViewProps {
  profile: HardwareProfileData;
  activeProfile: PerformanceProfileDef;
  bootTargets: BootTarget[];
  integrations: IntegrationStatus[];
  onNavigate: (tab: NavTab) => void;
  onSelectGpu: (index: number) => void;
  onOpenDeployment: () => void;
}

export const OverviewView: React.FC<OverviewViewProps> = ({
  profile,
  activeProfile,
  bootTargets,
  integrations,
  onNavigate,
  onSelectGpu,
  onOpenDeployment,
}) => {
  const activeGpu = profile.gpus[profile.activeGpuIndex] || profile.gpus[0];
  const isNative = profile.source === 'NATIVE';
  const defaultBoot = bootTargets.find((b) => b.isDefault) || bootTargets[0];
  const oclpIntegration = integrations.find((i) => i.id === 'oclp');

  return (
    <div id="overview-dashboard-view" className="space-y-6">
      {/* Top Banner: Mac Hardware Identity */}
      <div className="relative overflow-hidden rounded-2xl glass-card-light dark:glass-card-dark border border-black/10 dark:border-white/10 p-6">
        <div className="flex flex-col lg:flex-row lg:items-center justify-between gap-6 relative z-10">
          <div className="space-y-2.5 max-w-2xl">
            <div className="flex items-center gap-2">
              <StatusBadge
                id="overview-source-badge"
                label={isNative ? '● Hardware Detected: Native Silicon' : `● Hardware Source: Simulated (${profile.modelIdentifier})`}
                tone={isNative ? 'emerald' : 'amber'}
                pulse={isNative}
              />
              <span className="text-xs font-mono text-neutral-500 dark:text-neutral-400">
                Model: {profile.modelIdentifier}
              </span>
            </div>

            <h1 className="text-2xl font-bold tracking-tight text-neutral-900 dark:text-neutral-100 flex items-center gap-3">
              <Laptop className="w-7 h-7 text-blue-600 dark:text-blue-400 shrink-0" />
              <span>{profile.marketingName}</span>
            </h1>

            <p className="text-xs text-neutral-600 dark:text-neutral-300 leading-relaxed">
              OpenVintage hardware-aware boot orchestration platform. Coordinating dual-GPU GMUX switching,
              subsystem capability translation, and verified EFI staging for legacy Intel silicon.
            </p>
          </div>

          {/* Quick Hardware Snapshot pill card */}
          <div className="shrink-0 flex flex-col sm:flex-row gap-3">
            <div className="rounded-xl p-3.5 bg-black/5 dark:bg-white/5 border border-black/5 dark:border-white/5 space-y-1 min-w-[170px]">
              <span className="text-[11px] font-medium text-neutral-500 dark:text-neutral-400 uppercase tracking-wider block">
                Active Graphics
              </span>
              <div className="text-sm font-semibold text-neutral-900 dark:text-neutral-100 truncate">
                {activeGpu.modelName}
              </div>
              <div className="flex items-center gap-1.5 text-xs text-neutral-500 dark:text-neutral-400">
                <span className="w-2 h-2 rounded-full bg-blue-500" />
                <span>{profile.gpus.length} GPUs in Topology</span>
              </div>
            </div>

            <div className="rounded-xl p-3.5 bg-black/5 dark:bg-white/5 border border-black/5 dark:border-white/5 space-y-1 min-w-[170px]">
              <span className="text-[11px] font-medium text-neutral-500 dark:text-neutral-400 uppercase tracking-wider block">
                Deployment Gate
              </span>
              <div className="text-sm font-semibold text-emerald-600 dark:text-emerald-400 flex items-center gap-1.5">
                <ShieldCheck className="w-4 h-4" />
                <span>Ready &amp; Verified</span>
              </div>
              <button
                onClick={onOpenDeployment}
                className="text-xs text-blue-600 dark:text-blue-400 hover:underline flex items-center gap-1 pt-0.5"
              >
                <span>Launch Deployment Flow</span>
                <ArrowRight className="w-3 h-3" />
              </button>
            </div>
          </div>
        </div>
      </div>

      {/* Primary Telemetry Grid */}
      <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4">
        {/* CPU Spec Metric */}
        <MetricCard
          id="metric-cpu"
          label="Processor"
          value={profile.cpu.modelName}
          subValue={`${profile.cpu.physicalCores} Cores / ${profile.cpu.logicalThreads} Threads @ ${profile.cpu.baseClockGhz} GHz`}
          icon={<Cpu className="w-4 h-4" />}
          statusBadge={
            <StatusBadge
              label={profile.cpu.features.avx2 ? 'AVX2 Native' : 'SSE4.2 / AVX1'}
              tone={profile.cpu.features.avx2 ? 'emerald' : 'sky'}
              dot={false}
            />
          }
          onClick={() => onNavigate('hardware')}
        />

        {/* Active GPU Metric */}
        <MetricCard
          id="metric-gpu"
          label="Active GPU Topology"
          value={activeGpu.modelName}
          subValue={`${activeGpu.vramMB} MB VRAM • ${activeGpu.metalLevel}`}
          icon={<Monitor className="w-4 h-4" />}
          statusBadge={
            <StatusBadge
              label={activeGpu.isDiscrete ? 'Discrete GK107' : 'Integrated Ring'}
              tone={activeGpu.isDiscrete ? 'violet' : 'sky'}
            />
          }
          onClick={() => onNavigate('hardware')}
        />

        {/* Performance Profile Metric */}
        <MetricCard
          id="metric-performance"
          label="Performance Profile"
          value={activeProfile.name}
          subValue={activeProfile.badge}
          icon={<Zap className="w-4 h-4" />}
          statusBadge={<StatusBadge label="Optimized" tone="emerald" />}
          onClick={() => onNavigate('performance')}
        />

        {/* Default Boot Target Metric */}
        <MetricCard
          id="metric-boot"
          label="Default Boot Target"
          value={defaultBoot.title}
          subValue={`${bootTargets.length} Boot Targets Discovered`}
          icon={<HardDrive className="w-4 h-4" />}
          statusBadge={
            <StatusBadge
              label={defaultBoot.compatibilityState === 'REQUIRES_OCLP' ? 'OCLP Required' : 'Native'}
              tone={defaultBoot.compatibilityState === 'REQUIRES_OCLP' ? 'amber' : 'emerald'}
            />
          }
          onClick={() => onNavigate('boot')}
        />
      </div>

      {/* Multi-GPU Topology & Physical Inventory Glance */}
      <GlassPanel
        id="overview-gpu-topology-panel"
        title="Physical Graphics Inventory &amp; GMUX Status"
        action={
          <span className="text-xs text-neutral-500 dark:text-neutral-400 font-mono">
            {profile.gpus.length} Physical Devices Present
          </span>
        }
      >
        <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
          {profile.gpus.map((gpu) => {
            const isActive = gpu.index === profile.activeGpuIndex;
            return (
              <div
                key={gpu.id}
                id={`overview-gpu-card-${gpu.id}`}
                className={`p-4 rounded-xl border transition-all ${
                  isActive
                    ? 'border-blue-500/40 bg-blue-500/5 dark:bg-blue-500/10 shadow-xs'
                    : 'border-black/5 dark:border-white/5 bg-black/2 dark:bg-white/2 hover:border-black/15 dark:hover:border-white/15'
                }`}
              >
                <div className="flex items-start justify-between gap-3 mb-2">
                  <div className="space-y-0.5">
                    <div className="text-sm font-semibold text-neutral-900 dark:text-neutral-100 flex items-center gap-2">
                      <span>{gpu.modelName}</span>
                    </div>
                    <span className="text-xs text-neutral-500 dark:text-neutral-400">
                      {gpu.isDiscrete ? 'Discrete Silicon (PCIe)' : 'Integrated Intel Ring Bus'}
                    </span>
                  </div>

                  <div className="shrink-0 flex items-center gap-1.5">
                    {isActive ? (
                      <StatusBadge label="ACTIVE" tone="emerald" />
                    ) : (
                      <button
                        onClick={() => onSelectGpu(gpu.index)}
                        className="text-xs px-2.5 py-1 rounded-md bg-neutral-200 dark:bg-neutral-800 hover:bg-blue-600 hover:text-white dark:hover:bg-blue-600 text-neutral-700 dark:text-neutral-300 font-medium transition-colors"
                      >
                        Switch Active
                      </button>
                    )}
                  </div>
                </div>

                <div className="grid grid-cols-3 gap-2 pt-2 border-t border-black/5 dark:border-white/5 text-xs">
                  <div>
                    <span className="text-[10px] text-neutral-400 uppercase tracking-wider block">VRAM</span>
                    <span className="font-mono text-neutral-800 dark:text-neutral-200">{gpu.vramMB} MB</span>
                  </div>
                  <div>
                    <span className="text-[10px] text-neutral-400 uppercase tracking-wider block">Metal Support</span>
                    <span className="font-mono text-neutral-800 dark:text-neutral-200">{gpu.metalLevel}</span>
                  </div>
                  <div>
                    <span className="text-[10px] text-neutral-400 uppercase tracking-wider block">Physical State</span>
                    <span className="font-mono text-emerald-600 dark:text-emerald-400">Present</span>
                  </div>
                </div>
              </div>
            );
          })}
        </div>
      </GlassPanel>

      {/* Subsystem Health & Quick Actions Grid */}
      <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
        {/* Compatibility Sandbox */}
        <div
          id="overview-card-compat"
          onClick={() => onNavigate('compatibility')}
          className="glass-card-light dark:glass-card-dark glass-card-interactive-light dark:glass-card-interactive-dark rounded-xl p-5 cursor-pointer flex flex-col justify-between"
        >
          <div className="space-y-2">
            <div className="w-8 h-8 rounded-lg bg-emerald-500/10 text-emerald-600 dark:text-emerald-400 flex items-center justify-center">
              <CheckCircle2 className="w-4 h-4" />
            </div>
            <h3 className="text-sm font-semibold text-neutral-900 dark:text-neutral-100">
              OS Compatibility Matrix
            </h3>
            <p className="text-xs text-neutral-500 dark:text-neutral-400 leading-relaxed">
              Audit support criteria for macOS Monterey, Ventura, Sonoma, Sequoia, Linux, and Windows 11 on this silicon.
            </p>
          </div>
          <div className="mt-4 pt-3 border-t border-black/5 dark:border-white/5 flex items-center justify-between text-xs font-medium text-emerald-600 dark:text-emerald-400">
            <span>Audit OS Targets</span>
            <ArrowRight className="w-3.5 h-3.5" />
          </div>
        </div>

        {/* Performance Controller */}
        <div
          id="overview-card-perf"
          onClick={() => onNavigate('performance')}
          className="glass-card-light dark:glass-card-dark glass-card-interactive-light dark:glass-card-interactive-dark rounded-xl p-5 cursor-pointer flex flex-col justify-between"
        >
          <div className="space-y-2">
            <div className="w-8 h-8 rounded-lg bg-blue-500/10 text-blue-600 dark:text-blue-400 flex items-center justify-center">
              <Zap className="w-4 h-4" />
            </div>
            <h3 className="text-sm font-semibold text-neutral-900 dark:text-neutral-100">
              Hardware Performance Profiles
            </h3>
            <p className="text-xs text-neutral-500 dark:text-neutral-400 leading-relaxed">
              Switch between Maximum Performance, Gaming, Balanced, Efficiency, and Compatibility profiles.
            </p>
          </div>
          <div className="mt-4 pt-3 border-t border-black/5 dark:border-white/5 flex items-center justify-between text-xs font-medium text-blue-600 dark:text-blue-400">
            <span>Configure Profiles</span>
            <ArrowRight className="w-3.5 h-3.5" />
          </div>
        </div>

        {/* Integrations Hub */}
        <div
          id="overview-card-integrations"
          onClick={() => onNavigate('integrations')}
          className="glass-card-light dark:glass-card-dark glass-card-interactive-light dark:glass-card-interactive-dark rounded-xl p-5 cursor-pointer flex flex-col justify-between"
        >
          <div className="space-y-2">
            <div className="w-8 h-8 rounded-lg bg-violet-500/10 text-violet-600 dark:text-violet-400 flex items-center justify-center">
              <Boxes className="w-4 h-4" />
            </div>
            <h3 className="text-sm font-semibold text-neutral-900 dark:text-neutral-100">
              Ecosystem Integrations
            </h3>
            <p className="text-xs text-neutral-500 dark:text-neutral-400 leading-relaxed">
              Verify status of OpenCore Legacy Patcher (OCLP) root patches, rEFInd chainloader, and OpenVintage preboot core.
            </p>
          </div>
          <div className="mt-4 pt-3 border-t border-black/5 dark:border-white/5 flex items-center justify-between text-xs font-medium text-violet-600 dark:text-violet-400">
            <span>Manage Integrations</span>
            <ArrowRight className="w-3.5 h-3.5" />
          </div>
        </div>
      </div>
    </div>
  );
};
