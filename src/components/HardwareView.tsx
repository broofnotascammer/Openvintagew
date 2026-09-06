import React, { useState } from 'react';
import { LEGACY_MAC_HARDWARE_PROFILES } from '../core/hardware';
import { HardwareProfile } from '../types';
import { Cpu, HardDrive, Monitor, AlertCircle, CheckCircle2, Shield } from 'lucide-react';

export const HardwareView: React.FC = () => {
  const [selectedId, setSelectedId] = useState<string>(LEGACY_MAC_HARDWARE_PROFILES[0].id);

  const current: HardwareProfile =
    LEGACY_MAC_HARDWARE_PROFILES.find((h) => h.id === selectedId) || LEGACY_MAC_HARDWARE_PROFILES[0];

  return (
    <div id="hardware-hal-view" className="space-y-6">
      {/* Intro Header */}
      <div className="bg-neutral-900/60 border border-neutral-800 rounded-xl p-6">
        <div>
          <h2 className="text-lg font-bold text-white font-mono flex items-center">
            <Monitor className="w-5 h-5 mr-2 text-amber-400" />
            Hardware Abstraction Layer (HAL) Database
          </h2>
          <p className="text-xs text-neutral-400 mt-1">
            Standardized silicon profiles for legacy Intel Mac platforms (2006–2015). Provides unified hardware capability abstractions to OVResolver, OVScheduler, and OVIR-GPU.
          </p>
        </div>
      </div>

      {/* Hardware Selector Bar */}
      <div className="flex gap-2 overflow-x-auto pb-2">
        {LEGACY_MAC_HARDWARE_PROFILES.map((profile) => (
          <button
            key={profile.id}
            onClick={() => setSelectedId(profile.id)}
            className={`px-3.5 py-2 rounded-lg text-xs font-mono whitespace-nowrap border transition-all ${
              selectedId === profile.id
                ? 'bg-amber-500/20 border-amber-400 text-amber-300 ring-1 ring-amber-400/30 font-semibold'
                : 'bg-neutral-900/60 border-neutral-800 text-neutral-400 hover:text-neutral-200 hover:border-neutral-700'
            }`}
          >
            {profile.systemName}
          </button>
        ))}
      </div>

      {/* Detailed Spec Sheet */}
      <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
        {/* CPU Panel */}
        <div className="bg-neutral-950 border border-neutral-800 rounded-xl p-5 space-y-4">
          <div className="flex items-center justify-between pb-2 border-b border-neutral-800">
            <h3 className="text-xs font-mono uppercase tracking-wider text-amber-400 flex items-center">
              <Cpu className="w-4 h-4 mr-1.5" />
              CPU Specifications
            </h3>
            <span className="text-[10px] font-mono px-2 py-0.5 rounded bg-neutral-900 text-neutral-300 border border-neutral-800">
              {current.cpu.architecture}
            </span>
          </div>

          <div className="space-y-2 text-xs font-mono">
            <div className="flex justify-between py-1 border-b border-neutral-900">
              <span className="text-neutral-400">Processor Model:</span>
              <span className="text-white font-semibold text-right">{current.cpu.model}</span>
            </div>
            <div className="flex justify-between py-1 border-b border-neutral-900">
              <span className="text-neutral-400">Microarchitecture:</span>
              <span className="text-amber-300">{current.cpu.microarchitecture}</span>
            </div>
            <div className="flex justify-between py-1 border-b border-neutral-900">
              <span className="text-neutral-400">Core / Thread Count:</span>
              <span className="text-white">{current.cpu.physicalCores} Cores / {current.cpu.logicalThreads} Threads</span>
            </div>
            <div className="flex justify-between py-1 border-b border-neutral-900">
              <span className="text-neutral-400">Clock Speeds:</span>
              <span className="text-white">{current.cpu.baseClockGhz} GHz (Turbo: {current.cpu.maxTurboGhz} GHz)</span>
            </div>
            <div className="flex justify-between py-1 border-b border-neutral-900">
              <span className="text-neutral-400">Thermal Design Power:</span>
              <span className="text-neutral-300">{current.cpu.tdpWatts} W</span>
            </div>
            <div className="pt-2">
              <span className="text-neutral-400 block mb-1.5">Instruction Extensions:</span>
              <div className="flex flex-wrap gap-1.5">
                {current.cpu.instructionSets.map((inst, i) => (
                  <span
                    key={i}
                    className="text-[10px] px-2 py-0.5 rounded bg-neutral-900 border border-neutral-800 text-sky-300"
                  >
                    {inst}
                  </span>
                ))}
              </div>
            </div>
          </div>
        </div>

        {/* GPU Panel */}
        <div className="bg-neutral-950 border border-neutral-800 rounded-xl p-5 space-y-4">
          <div className="flex items-center justify-between pb-2 border-b border-neutral-800">
            <h3 className="text-xs font-mono uppercase tracking-wider text-amber-400 flex items-center">
              <Monitor className="w-4 h-4 mr-1.5" />
              GPU &amp; Silicon Silicon Limits
            </h3>
            <span className="text-[10px] font-mono px-2 py-0.5 rounded bg-neutral-900 text-neutral-300 border border-neutral-800">
              {current.gpu.vendor}
            </span>
          </div>

          <div className="space-y-2 text-xs font-mono">
            <div className="flex justify-between py-1 border-b border-neutral-900">
              <span className="text-neutral-400">GPU Silicon Model:</span>
              <span className="text-white font-semibold text-right">{current.gpu.model}</span>
            </div>
            <div className="flex justify-between py-1 border-b border-neutral-900">
              <span className="text-neutral-400">Architecture Family:</span>
              <span className="text-amber-300">{current.gpu.architecture}</span>
            </div>
            <div className="flex justify-between py-1 border-b border-neutral-900">
              <span className="text-neutral-400">Dedicated VRAM:</span>
              <span className="text-emerald-400 font-bold">{current.gpu.vramMB} MB</span>
            </div>
            <div className="flex justify-between py-1 border-b border-neutral-900">
              <span className="text-neutral-400">Bus Interface:</span>
              <span className="text-neutral-300">{current.gpu.pcieGen}</span>
            </div>
            <div className="flex justify-between py-1 border-b border-neutral-900">
              <span className="text-neutral-400">Max Texture Dimension:</span>
              <span className="text-white">{current.gpu.limits.maxTextureSize} px</span>
            </div>
            <div className="flex justify-between py-1 border-b border-neutral-900">
              <span className="text-neutral-400">Max MSAA Supported:</span>
              <span className="text-white">{current.gpu.limits.msaaMax}x</span>
            </div>
            <div className="flex justify-between py-1 border-b border-neutral-900">
              <span className="text-neutral-400">Compute Shaders:</span>
              <span className={current.gpu.limits.computeSupported ? 'text-emerald-400' : 'text-rose-400'}>
                {current.gpu.limits.computeSupported ? 'Hardware Compliant' : 'Unavailable (Needs CPU Fallback)'}
              </span>
            </div>
          </div>
        </div>

        {/* Platform, Memory & Quirks */}
        <div className="bg-neutral-950 border border-neutral-800 rounded-xl p-5 space-y-4">
          <div className="flex items-center justify-between pb-2 border-b border-neutral-800">
            <h3 className="text-xs font-mono uppercase tracking-wider text-amber-400 flex items-center">
              <HardDrive className="w-4 h-4 mr-1.5" />
              Platform &amp; Hardware Quirks
            </h3>
            <span className="text-[10px] font-mono px-2 py-0.5 rounded bg-neutral-900 text-neutral-300 border border-neutral-800">
              {current.modelIdentifier}
            </span>
          </div>

          <div className="space-y-2 text-xs font-mono">
            <div className="flex justify-between py-1 border-b border-neutral-900">
              <span className="text-neutral-400">Total System RAM:</span>
              <span className="text-white">{(current.ramMB / 1024).toFixed(0)} GB</span>
            </div>
            <div className="flex justify-between py-1 border-b border-neutral-900">
              <span className="text-neutral-400">Storage Subsystem:</span>
              <span className="text-white">{current.storageType}</span>
            </div>
            <div className="flex justify-between py-1 border-b border-neutral-900">
              <span className="text-neutral-400">EFI Firmware Bitness:</span>
              <span className="text-emerald-400">{current.efiBitness}</span>
            </div>
            <div className="flex justify-between py-1 border-b border-neutral-900">
              <span className="text-neutral-400">Release Year:</span>
              <span className="text-neutral-300">{current.releaseYear}</span>
            </div>
          </div>

          <div className="pt-2">
            <span className="text-xs font-mono text-neutral-400 uppercase tracking-wider block mb-2 flex items-center">
              <AlertCircle className="w-3.5 h-3.5 mr-1 text-amber-400" />
              Hardware Compatibility Quirks
            </span>
            <div className="space-y-2">
              {current.gpu.quirks.map((quirk, i) => (
                <div key={i} className="text-xs text-neutral-300 bg-neutral-900/80 p-2.5 rounded border border-neutral-850 flex items-start">
                  <span className="text-amber-400 mr-2 font-bold font-mono">›</span>
                  <span>{quirk}</span>
                </div>
              ))}
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};
