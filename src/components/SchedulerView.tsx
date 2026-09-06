import React, { useState } from 'react';
import { PerformanceProfile, SchedulerState, HardwareProfile } from '../types';
import { LEGACY_MAC_HARDWARE_PROFILES } from '../core/hardware';
import { OVScheduler } from '../core/scheduler';
import { Cpu, Flame, Activity, Zap, CheckCircle, ShieldAlert } from 'lucide-react';

interface SchedulerViewProps {
  currentProfile: PerformanceProfile;
  onProfileChange: (p: PerformanceProfile) => void;
}

export const SchedulerView: React.FC<SchedulerViewProps> = ({
  currentProfile,
  onProfileChange,
}) => {
  const [selectedHwId, setSelectedHwId] = useState<string>(LEGACY_MAC_HARDWARE_PROFILES[0].id);

  const hw = LEGACY_MAC_HARDWARE_PROFILES.find((h) => h.id === selectedHwId) || LEGACY_MAC_HARDWARE_PROFILES[0];
  const schedulerState: SchedulerState = OVScheduler.initialize(hw, currentProfile);

  const profiles: { id: PerformanceProfile; title: string; desc: string }[] = [
    { id: 'battery', title: 'Battery Saver', desc: 'Minimal worker thread pool, relaxed clock frequencies, deferred shader warmup' },
    { id: 'balanced', title: 'Balanced (Default)', desc: 'Dynamic worker elasticity, cooperative OS scheduling, balanced thermal curve' },
    { id: 'performance', title: 'Performance', desc: 'Pre-warmed shader compilation, prioritized render and translation threads' },
    { id: 'max_performance', title: 'Max Performance', desc: 'Dedicated core pinning for heavy translation tasks, maximum memory headroom' },
    { id: 'developer', title: 'Developer / Debug', desc: 'Full instrumentation, diagnostic frame timing, memory leak assertions enabled' },
  ];

  return (
    <div id="scheduler-view" className="space-y-6">
      {/* Intro Banner */}
      <div className="bg-neutral-900/60 border border-neutral-800 rounded-xl p-6">
        <div className="flex flex-col md:flex-row md:items-center justify-between gap-4">
          <div>
            <h2 className="text-lg font-bold text-white font-mono flex items-center">
              <Cpu className="w-5 h-5 mr-2 text-amber-400" />
              OVScheduler &amp; System Resource Manager
            </h2>
            <p className="text-xs text-neutral-400 mt-1">
              Dynamically orchestrates CPU core topology, worker thread pools, translation pipelines, and thermal throttling thresholds across legacy Intel architectures.
            </p>
          </div>
          <div className="flex items-center space-x-2">
            <span className="text-xs text-neutral-400 font-mono">Platform:</span>
            <select
              value={selectedHwId}
              onChange={(e) => setSelectedHwId(e.target.value)}
              className="bg-neutral-950 border border-neutral-800 text-neutral-200 text-xs rounded px-2.5 py-1.5 font-mono focus:border-amber-400 outline-none"
            >
              {LEGACY_MAC_HARDWARE_PROFILES.map((h) => (
                <option key={h.id} value={h.id}>
                  {h.systemName} ({h.cpu.physicalCores}C/{h.cpu.logicalThreads}T)
                </option>
              ))}
            </select>
          </div>
        </div>
      </div>

      {/* Profiles Selection Grid */}
      <div className="grid grid-cols-1 md:grid-cols-5 gap-3">
        {profiles.map((p) => {
          const isActive = currentProfile === p.id;
          return (
            <button
              key={p.id}
              onClick={() => onProfileChange(p.id)}
              className={`p-4 rounded-xl text-left border transition-all flex flex-col justify-between ${
                isActive
                  ? 'bg-amber-500/10 border-amber-400 ring-1 ring-amber-400/30'
                  : 'bg-neutral-900/40 border-neutral-800 hover:border-neutral-700'
              }`}
            >
              <div>
                <div className="flex items-center justify-between mb-1.5">
                  <span className={`text-xs font-mono font-bold uppercase ${isActive ? 'text-amber-300' : 'text-neutral-200'}`}>
                    {p.title}
                  </span>
                  {isActive && <CheckCircle className="w-3.5 h-3.5 text-amber-400" />}
                </div>
                <p className="text-[11px] text-neutral-400 leading-snug">{p.desc}</p>
              </div>
              <div className="mt-3 text-[10px] font-mono text-neutral-500 uppercase tracking-wider">
                {p.id === 'max_performance' ? 'Pinning Enabled' : 'Cooperative OS'}
              </div>
            </button>
          );
        })}
      </div>

      {/* Topology & Core Mapping */}
      <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
        {/* Core Allocation Matrix */}
        <div className="lg:col-span-2 bg-neutral-900/40 border border-neutral-800 rounded-xl p-5">
          <div className="flex items-center justify-between pb-3 border-b border-neutral-800 mb-4">
            <h3 className="text-xs font-mono uppercase tracking-wider text-neutral-300 flex items-center">
              <Activity className="w-4 h-4 mr-1.5 text-amber-400" />
              CPU Core Topology &amp; Worker Mapping
            </h3>
            <span className="text-[11px] font-mono text-neutral-400">
              {hw.cpu.model} ({hw.cpu.physicalCores} Phys / {hw.cpu.logicalThreads} Log)
            </span>
          </div>

          <div className="grid grid-cols-2 sm:grid-cols-3 md:grid-cols-4 gap-3">
            {schedulerState.cores.map((core) => (
              <div
                key={core.coreId}
                className="bg-neutral-950 border border-neutral-800 rounded-lg p-3 space-y-2"
              >
                <div className="flex justify-between items-center text-[11px] font-mono">
                  <span className="text-white font-bold">Core #{core.coreId}</span>
                  <span className={`text-[10px] px-1.5 py-0.2 rounded uppercase ${
                    core.isHyperthread ? 'bg-neutral-800 text-neutral-400' : 'bg-amber-950 text-amber-400 border border-amber-800/60'
                  }`}>
                    {core.isHyperthread ? 'SMT / HT' : 'Physical'}
                  </span>
                </div>

                <div className="text-[11px] font-mono text-neutral-400 flex justify-between">
                  <span>Role:</span>
                  <span className={`capitalize font-medium ${
                    core.assignedRole === 'primary' ? 'text-emerald-400' :
                    core.assignedRole === 'worker' ? 'text-sky-400' :
                    core.assignedRole === 'io' ? 'text-amber-400' : 'text-neutral-500'
                  }`}>
                    {core.assignedRole}
                  </span>
                </div>

                {/* Progress bar */}
                <div>
                  <div className="flex justify-between text-[10px] font-mono text-neutral-500 mb-1">
                    <span>Load</span>
                    <span>{core.loadPercent}%</span>
                  </div>
                  <div className="w-full bg-neutral-800 h-1.5 rounded-full overflow-hidden">
                    <div
                      className={`h-full rounded-full ${
                        core.loadPercent > 80 ? 'bg-rose-500' : core.loadPercent > 50 ? 'bg-amber-400' : 'bg-emerald-500'
                      }`}
                      style={{ width: `${core.loadPercent}%` }}
                    ></div>
                  </div>
                </div>
              </div>
            ))}
          </div>

          <div className="mt-4 pt-3 border-t border-neutral-900 text-xs text-neutral-400 flex flex-wrap gap-4 font-mono text-[11px]">
            <span className="flex items-center">
              <span className="w-2.5 h-2.5 rounded-full bg-emerald-500 mr-1.5 inline-block"></span>
              Primary Render Thread
            </span>
            <span className="flex items-center">
              <span className="w-2.5 h-2.5 rounded-full bg-amber-400 mr-1.5 inline-block"></span>
              IO &amp; Driver Dispatch
            </span>
            <span className="flex items-center">
              <span className="w-2.5 h-2.5 rounded-full bg-sky-400 mr-1.5 inline-block"></span>
              Worker &amp; Translation Pool
            </span>
          </div>
        </div>

        {/* Telemetry & Thermal Status */}
        <div className="bg-neutral-950 border border-neutral-800 rounded-xl p-5 space-y-4">
          <h3 className="text-xs font-mono uppercase tracking-wider text-neutral-400 border-b border-neutral-800 pb-2 flex items-center">
            <Flame className="w-4 h-4 mr-1.5 text-amber-400" />
            Thermal &amp; Memory Telemetry
          </h3>

          <div className="space-y-3">
            {/* Thermals */}
            <div className="p-3 bg-neutral-900/60 border border-neutral-800 rounded-lg">
              <div className="flex justify-between items-center mb-2">
                <span className="text-xs font-mono text-neutral-400">Package Temperature:</span>
                <span className={`text-sm font-mono font-bold ${
                  schedulerState.thermalTempC >= 85 ? 'text-rose-400' : 'text-amber-400'
                }`}>
                  {schedulerState.thermalTempC}°C
                </span>
              </div>
              <div className="w-full bg-neutral-800 h-2 rounded-full overflow-hidden">
                <div
                  className={`h-full transition-all duration-300 ${
                    schedulerState.thermalTempC >= 85 ? 'bg-rose-500' : 'bg-amber-500'
                  }`}
                  style={{ width: `${Math.min(100, (schedulerState.thermalTempC / 100) * 100)}%` }}
                ></div>
              </div>
              <div className="flex justify-between text-[10px] text-neutral-500 font-mono mt-1">
                <span>Idle 40°C</span>
                <span>TDP Limit 100°C</span>
              </div>
            </div>

            {/* Memory Pressure */}
            <div className="p-3 bg-neutral-900/60 border border-neutral-800 rounded-lg">
              <div className="flex justify-between items-center mb-2">
                <span className="text-xs font-mono text-neutral-400">Physical Memory Usage:</span>
                <span className="text-sm font-mono font-bold text-white">
                  {(schedulerState.memoryUsageMB / 1024).toFixed(1)} / {(schedulerState.memoryCapacityMB / 1024).toFixed(0)} GB
                </span>
              </div>
              <div className="w-full bg-neutral-800 h-2 rounded-full overflow-hidden">
                <div
                  className="h-full bg-sky-500"
                  style={{ width: `${(schedulerState.memoryUsageMB / schedulerState.memoryCapacityMB) * 100}%` }}
                ></div>
              </div>
            </div>

            {/* Scheduling Rule Assertions */}
            <div className="p-3 bg-neutral-900/60 border border-neutral-800 rounded-lg space-y-2 text-xs font-mono">
              <div className="text-neutral-400 text-[11px] uppercase tracking-wider">Scheduler Rules Applied</div>
              <div className="flex items-center text-emerald-400 text-[11px]">
                <CheckCircle className="w-3.5 h-3.5 mr-1.5 shrink-0" />
                <span>OS Cooperative Scheduling: {schedulerState.cooperativeOsScheduling ? 'Active' : 'Overridden'}</span>
              </div>
              <div className="flex items-center text-emerald-400 text-[11px]">
                <CheckCircle className="w-3.5 h-3.5 mr-1.5 shrink-0" />
                <span>Worker Pool Size: {schedulerState.workerPoolSize} background threads</span>
              </div>
              {schedulerState.thermalThrottled && (
                <div className="flex items-center text-rose-400 text-[11px]">
                  <ShieldAlert className="w-3.5 h-3.5 mr-1.5 shrink-0" />
                  <span>Thermal clamp: Background JIT down-throttled</span>
                </div>
              )}
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};
