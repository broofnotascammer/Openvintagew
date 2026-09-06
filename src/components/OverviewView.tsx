import React from 'react';
import { EcosystemDiagram } from './EcosystemDiagram';
import { ShieldCheck, Cpu, GitBranch, ArrowRight, Zap, Layers, RefreshCw } from 'lucide-react';

interface OverviewViewProps {
  onNavigateTab: (tab: string) => void;
}

export const OverviewView: React.FC<OverviewViewProps> = ({ onNavigateTab }) => {
  return (
    <div id="overview-view" className="space-y-6">
      {/* Hero Systems Summary */}
      <div className="bg-neutral-900/60 border border-neutral-800 rounded-xl p-6">
        <div className="flex flex-col lg:flex-row lg:items-center justify-between gap-6">
          <div className="space-y-2 max-w-3xl">
            <div className="inline-flex items-center space-x-2 px-2.5 py-1 rounded bg-amber-500/10 border border-amber-500/30 text-amber-300 text-xs font-mono">
              <span className="w-2 h-2 rounded-full bg-amber-400 animate-pulse"></span>
              <span>OPENVINTAGE LEAD SYSTEMS ENGINEERING CONSOLE</span>
            </div>
            <h2 className="text-xl font-bold text-white font-mono tracking-tight">
              Modular Compatibility &amp; Performance Platform for Legacy Intel Hardware
            </h2>
            <p className="text-xs text-neutral-300 leading-relaxed">
              OpenVintage decouples legacy platforms from inflexible API-to-API translation matrices through intermediate representations (OVIR-GPU, OVIR-CPU), intelligent runtime path resolution (OVResolver), dynamic core topology scheduling (OVScheduler), and multi-tier hash-keyed caching.
            </p>
          </div>

          <div className="bg-neutral-950 border border-neutral-800/80 rounded-xl p-4 flex flex-col justify-center space-y-3 shrink-0 font-mono text-xs">
            <div className="text-[11px] text-neutral-400 uppercase tracking-wider">Phase Milestones</div>
            <div className="flex items-center justify-between space-x-4">
              <span className="text-neutral-300">Phase 1: UEFI X64 App</span>
              <span className="text-emerald-400 font-bold">COMPLETED</span>
            </div>
            <div className="flex items-center justify-between space-x-4">
              <span className="text-neutral-300">Phase 2: Core Architecture</span>
              <span className="text-amber-400 font-bold">IN PROGRESS</span>
            </div>
            <div className="flex items-center justify-between space-x-4">
              <span className="text-neutral-500">Phase 3: OVIR Pipeline</span>
              <span className="text-neutral-500">PENDING</span>
            </div>
          </div>
        </div>
      </div>

      {/* Interactive Architecture Diagram */}
      <EcosystemDiagram />

      {/* Quick Action Exploration Grid */}
      <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
        <div
          onClick={() => onNavigateTab('resolver')}
          className="bg-neutral-950 border border-neutral-800 hover:border-amber-400/50 rounded-xl p-5 cursor-pointer transition-all group"
        >
          <div className="flex items-center justify-between mb-2">
            <div className="w-8 h-8 rounded bg-amber-500/10 border border-amber-500/20 flex items-center justify-center text-amber-400">
              <Zap className="w-4 h-4" />
            </div>
            <ArrowRight className="w-4 h-4 text-neutral-600 group-hover:text-amber-400 transition-colors" />
          </div>
          <h3 className="text-sm font-bold text-white font-mono group-hover:text-amber-300 transition-colors">
            OVResolver Sandbox
          </h3>
          <p className="text-xs text-neutral-400 mt-1.5 leading-relaxed">
            Test how workloads (Metal, Vulkan, OpenGL, DirectX) are routed across Ivy Bridge, Westmere, Haswell, and Penryn silicon.
          </p>
        </div>

        <div
          onClick={() => onNavigateTab('scheduler')}
          className="bg-neutral-950 border border-neutral-800 hover:border-amber-400/50 rounded-xl p-5 cursor-pointer transition-all group"
        >
          <div className="flex items-center justify-between mb-2">
            <div className="w-8 h-8 rounded bg-sky-500/10 border border-sky-500/20 flex items-center justify-center text-sky-400">
              <Cpu className="w-4 h-4" />
            </div>
            <ArrowRight className="w-4 h-4 text-neutral-600 group-hover:text-amber-400 transition-colors" />
          </div>
          <h3 className="text-sm font-bold text-white font-mono group-hover:text-amber-300 transition-colors">
            OVScheduler Controller
          </h3>
          <p className="text-xs text-neutral-400 mt-1.5 leading-relaxed">
            Inspect core affinity, hyperthread distribution, worker thread elasticity, and thermal throttling profile rules.
          </p>
        </div>

        <div
          onClick={() => onNavigateTab('hardware')}
          className="bg-neutral-950 border border-neutral-800 hover:border-amber-400/50 rounded-xl p-5 cursor-pointer transition-all group"
        >
          <div className="flex items-center justify-between mb-2">
            <div className="w-8 h-8 rounded bg-emerald-500/10 border border-emerald-500/20 flex items-center justify-center text-emerald-400">
              <Layers className="w-4 h-4" />
            </div>
            <ArrowRight className="w-4 h-4 text-neutral-600 group-hover:text-amber-400 transition-colors" />
          </div>
          <h3 className="text-sm font-bold text-white font-mono group-hover:text-amber-300 transition-colors">
            Hardware Database (HAL)
          </h3>
          <p className="text-xs text-neutral-400 mt-1.5 leading-relaxed">
            Examine the physical specs, VRAM constraints, and silicon quirks for Intel HD, AMD TeraScale, and Nvidia Kepler GPUs.
          </p>
        </div>
      </div>
    </div>
  );
};
