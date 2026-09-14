import React, { useState } from 'react';
import { 
  Cpu, 
  Monitor, 
  HardDrive, 
  Layers, 
  ShieldCheck, 
  Terminal, 
  Info,
  CheckCircle2,
  AlertTriangle,
  RotateCcw
} from 'lucide-react';
import { HardwareProfileData, GpuDevice } from '../core/openvintageState';
import { GlassPanel } from './GlassPanel';
import { StatusBadge } from './StatusBadge';

interface HardwareViewProps {
  profile: HardwareProfileData;
  onSelectGpu: (index: number) => void;
  onRefreshHardware: () => void;
  isDetecting?: boolean;
}

export const HardwareView: React.FC<HardwareViewProps> = ({
  profile,
  onSelectGpu,
  onRefreshHardware,
  isDetecting = false,
}) => {
  const activeGpu = profile.gpus[profile.activeGpuIndex] || profile.gpus[0];
  const isNative = profile.source === 'NATIVE';

  return (
    <div id="hardware-page-view" className="space-y-6">
      {/* Header Banner */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4 glass-card-light dark:glass-card-dark rounded-xl p-5 border border-black/10 dark:border-white/10">
        <div>
          <div className="flex items-center gap-2 mb-1">
            <StatusBadge
              id="hardware-source-tag"
              label={isNative ? 'Host Silicon (Native HAL)' : `Simulated: ${profile.modelIdentifier}`}
              tone={isNative ? 'emerald' : 'amber'}
            />
            <span className="text-xs font-mono text-neutral-400">
              SMBIOS: {profile.modelIdentifier}
            </span>
          </div>
          <h2 className="text-xl font-bold tracking-tight text-neutral-900 dark:text-neutral-100">
            {profile.marketingName}
          </h2>
          <p className="text-xs text-neutral-500 dark:text-neutral-400 mt-0.5">
            Physical silicon inventory, GMUX display controller topology, and UEFI firmware capabilities.
          </p>
        </div>

        <button
          onClick={onRefreshHardware}
          disabled={isDetecting}
          className="inline-flex items-center gap-1.5 px-3 py-1.5 rounded-lg border border-black/10 dark:border-white/10 bg-black/5 dark:bg-white/5 hover:bg-black/10 dark:hover:bg-white/10 text-xs font-medium text-neutral-700 dark:text-neutral-300 transition-colors self-start sm:self-auto disabled:opacity-50"
        >
          <RotateCcw className={`w-3.5 h-3.5 ${isDetecting ? 'animate-spin text-blue-500' : ''}`} />
          <span>Re-Audit Silicon</span>
        </button>
      </div>

      {/* CRITICAL SECTION: Physical GPU Inventory vs Active GPU */}
      <GlassPanel
        id="hardware-gpu-inventory-section"
        title="Physical GPU Inventory &amp; Topology (GMUX Multiplexer)"
        action={
          <div className="flex items-center gap-2">
            <span className="text-xs text-neutral-500 dark:text-neutral-400">
              Total Physical GPUs:
            </span>
            <span className="text-xs font-mono font-bold text-neutral-900 dark:text-neutral-100 px-2 py-0.5 rounded bg-black/5 dark:bg-white/5">
              {profile.gpus.length}
            </span>
          </div>
        }
      >
        <div className="space-y-4">
          <div className="p-3 rounded-lg bg-blue-500/5 dark:bg-blue-500/10 border border-blue-500/20 text-xs text-neutral-600 dark:text-neutral-300 flex items-start gap-2">
            <Info className="w-4 h-4 text-blue-500 shrink-0 mt-0.5" />
            <span>
              <strong>Hardware Inventory Rule:</strong> Both physical GPUs remain present in the hardware inventory regardless of which GPU is actively driving display output or compute tasks. Switching the active GPU reconfigures the GMUX multiplexer without detaching physical silicon.
            </span>
          </div>

          <div className="grid grid-cols-1 lg:grid-cols-2 gap-4">
            {profile.gpus.map((gpu: GpuDevice) => {
              const isActive = gpu.index === profile.activeGpuIndex;

              return (
                <div
                  key={gpu.id}
                  id={`gpu-physical-card-${gpu.id}`}
                  className={`rounded-xl p-4 border transition-all ${
                    isActive
                      ? 'bg-blue-500/5 dark:bg-blue-500/10 border-blue-500/40 shadow-xs ring-1 ring-blue-500/20'
                      : 'bg-black/2 dark:bg-white/2 border-black/5 dark:border-white/5 hover:border-black/15 dark:hover:border-white/15'
                  }`}
                >
                  <div className="flex items-start justify-between gap-3 mb-3">
                    <div>
                      <div className="flex items-center gap-2">
                        <h4 className="text-sm font-semibold text-neutral-900 dark:text-neutral-100">
                          {gpu.modelName}
                        </h4>
                      </div>
                      <span className="text-xs text-neutral-500 dark:text-neutral-400 font-mono">
                        {gpu.vendor} • {gpu.pcieGen}
                      </span>
                    </div>

                    <div className="flex items-center gap-2">
                      <StatusBadge
                        label="PRESENT"
                        tone="emerald"
                        dot={true}
                      />
                      {isActive ? (
                        <StatusBadge
                          label="ACTIVE"
                          tone="violet"
                          dot={true}
                          pulse={true}
                        />
                      ) : (
                        <button
                          id={`btn-activate-gpu-${gpu.id}`}
                          onClick={() => onSelectGpu(gpu.index)}
                          className="text-xs px-2.5 py-1 rounded bg-neutral-200 dark:bg-neutral-800 hover:bg-blue-600 hover:text-white dark:hover:bg-blue-600 text-neutral-700 dark:text-neutral-300 font-medium transition-colors"
                        >
                          Make Active
                        </button>
                      )}
                    </div>
                  </div>

                  <div className="grid grid-cols-2 sm:grid-cols-4 gap-3 py-3 border-y border-black/5 dark:border-white/5 text-xs font-mono">
                    <div>
                      <span className="text-[10px] text-neutral-400 uppercase tracking-wider block">Role</span>
                      <span className="text-neutral-900 dark:text-neutral-100 font-medium">
                        {gpu.isDiscrete ? 'Discrete (dGPU)' : 'Integrated (iGPU)'}
                      </span>
                    </div>
                    <div>
                      <span className="text-[10px] text-neutral-400 uppercase tracking-wider block">VRAM</span>
                      <span className="text-neutral-900 dark:text-neutral-100 font-medium">
                        {gpu.vramMB} MB
                      </span>
                    </div>
                    <div>
                      <span className="text-[10px] text-neutral-400 uppercase tracking-wider block">Metal</span>
                      <span className="text-neutral-900 dark:text-neutral-100 font-medium">
                        {gpu.metalLevel}
                      </span>
                    </div>
                    <div>
                      <span className="text-[10px] text-neutral-400 uppercase tracking-wider block">Max Texture</span>
                      <span className="text-neutral-900 dark:text-neutral-100 font-medium">
                        {gpu.maxTextureDimension}px
                      </span>
                    </div>
                  </div>

                  <div className="mt-3 flex items-center justify-between text-xs text-neutral-500 dark:text-neutral-400">
                    <span className="font-mono text-[11px]">{gpu.openGLVersion}</span>
                    <span className="font-mono text-[11px]">{gpu.status}</span>
                  </div>
                </div>
              );
            })}
          </div>
        </div>
      </GlassPanel>

      {/* Main Spec Panels: CPU & Memory */}
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        {/* CPU Panel */}
        <GlassPanel
          id="hardware-cpu-section"
          title="CPU Microarchitecture &amp; Extensions"
          action={
            <span className="text-xs font-mono px-2 py-0.5 rounded bg-black/5 dark:bg-white/5 text-neutral-600 dark:text-neutral-300">
              {profile.cpu.architecture}
            </span>
          }
        >
          <div className="space-y-3 text-xs font-mono">
            <div className="flex justify-between py-1.5 border-b border-black/5 dark:border-white/5">
              <span className="text-neutral-500 dark:text-neutral-400">Processor Model:</span>
              <span className="font-semibold text-neutral-900 dark:text-neutral-100 text-right">
                {profile.cpu.modelName}
              </span>
            </div>
            <div className="flex justify-between py-1.5 border-b border-black/5 dark:border-white/5">
              <span className="text-neutral-500 dark:text-neutral-400">Microarchitecture:</span>
              <span className="text-blue-600 dark:text-blue-400 font-medium">
                {profile.cpu.microarchitecture}
              </span>
            </div>
            <div className="flex justify-between py-1.5 border-b border-black/5 dark:border-white/5">
              <span className="text-neutral-500 dark:text-neutral-400">Core / Thread Topology:</span>
              <span className="text-neutral-900 dark:text-neutral-100">
                {profile.cpu.physicalCores} Physical Cores / {profile.cpu.logicalThreads} Logical Threads
              </span>
            </div>
            <div className="flex justify-between py-1.5 border-b border-black/5 dark:border-white/5">
              <span className="text-neutral-500 dark:text-neutral-400">Clock Frequency:</span>
              <span className="text-neutral-900 dark:text-neutral-100">
                {profile.cpu.baseClockGhz} GHz (Turbo: {profile.cpu.maxTurboGhz} GHz)
              </span>
            </div>
            <div className="flex justify-between py-1.5 border-b border-black/5 dark:border-white/5">
              <span className="text-neutral-500 dark:text-neutral-400">Thermal Design Power (TDP):</span>
              <span className="text-neutral-900 dark:text-neutral-100">
                {profile.cpu.tdpWatts} Watts
              </span>
            </div>

            {/* Instruction Extensions */}
            <div className="pt-2">
              <span className="text-neutral-500 dark:text-neutral-400 block mb-2">
                Instruction Set &amp; Extensions:
              </span>
              <div className="grid grid-cols-3 sm:grid-cols-4 gap-1.5">
                {Object.entries(profile.cpu.features).map(([key, enabled]) => (
                  <div
                    key={key}
                    className={`px-2 py-1 rounded text-center text-[11px] font-mono border ${
                      enabled
                        ? 'bg-emerald-500/10 border-emerald-500/30 text-emerald-700 dark:text-emerald-300'
                        : 'bg-neutral-500/5 border-neutral-400/20 text-neutral-400 line-through'
                    }`}
                  >
                    {key.toUpperCase()}
                  </div>
                ))}
              </div>
            </div>
          </div>
        </GlassPanel>

        {/* Memory & Display */}
        <div className="space-y-6">
          {/* Memory Panel */}
          <GlassPanel id="hardware-memory-section" title="System Memory (RAM)">
            <div className="space-y-2.5 text-xs font-mono">
              <div className="flex justify-between py-1 border-b border-black/5 dark:border-white/5">
                <span className="text-neutral-500 dark:text-neutral-400">Total Installed:</span>
                <span className="font-semibold text-neutral-900 dark:text-neutral-100">
                  {(profile.memory.totalMB / 1024).toFixed(0)} GB ({profile.memory.totalMB} MB)
                </span>
              </div>
              <div className="flex justify-between py-1 border-b border-black/5 dark:border-white/5">
                <span className="text-neutral-500 dark:text-neutral-400">Memory Type:</span>
                <span className="text-neutral-900 dark:text-neutral-100">{profile.memory.type}</span>
              </div>
              <div className="flex justify-between py-1 border-b border-black/5 dark:border-white/5">
                <span className="text-neutral-500 dark:text-neutral-400">Channel Architecture:</span>
                <span className="text-neutral-900 dark:text-neutral-100">{profile.memory.channels}</span>
              </div>
              <div className="flex justify-between py-1">
                <span className="text-neutral-500 dark:text-neutral-400">Theoretical Bandwidth:</span>
                <span className="text-blue-600 dark:text-blue-400 font-semibold">
                  {profile.memory.bandwidthGBs} GB/s
                </span>
              </div>
            </div>
          </GlassPanel>

          {/* Display Subsystem */}
          <GlassPanel id="hardware-display-section" title="Display Subsystem">
            <div className="space-y-2.5 text-xs font-mono">
              <div className="flex justify-between py-1 border-b border-black/5 dark:border-white/5">
                <span className="text-neutral-500 dark:text-neutral-400">Display Device:</span>
                <span className="text-neutral-900 dark:text-neutral-100">{profile.display.internalDisplay}</span>
              </div>
              <div className="flex justify-between py-1 border-b border-black/5 dark:border-white/5">
                <span className="text-neutral-500 dark:text-neutral-400">Native Resolution:</span>
                <span className="text-neutral-900 dark:text-neutral-100">{profile.display.nativeResolution}</span>
              </div>
              <div className="flex justify-between py-1 border-b border-black/5 dark:border-white/5">
                <span className="text-neutral-500 dark:text-neutral-400">Color Profile:</span>
                <span className="text-neutral-900 dark:text-neutral-100">{profile.display.colorSpace}</span>
              </div>
              <div className="flex justify-between py-1">
                <span className="text-neutral-500 dark:text-neutral-400">Max External Displays:</span>
                <span className="text-neutral-900 dark:text-neutral-100">
                  {profile.display.externalDisplaysSupported} Displays
                </span>
              </div>
            </div>
          </GlassPanel>
        </div>
      </div>

      {/* Firmware & PCI Bus Devices */}
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        {/* Firmware */}
        <GlassPanel id="hardware-firmware-section" title="Firmware &amp; Security">
          <div className="space-y-2.5 text-xs font-mono">
            <div className="flex justify-between py-1 border-b border-black/5 dark:border-white/5">
              <span className="text-neutral-500 dark:text-neutral-400">Firmware Type:</span>
              <span className="text-neutral-900 dark:text-neutral-100">{profile.firmware.type}</span>
            </div>
            <div className="flex justify-between py-1 border-b border-black/5 dark:border-white/5">
              <span className="text-neutral-500 dark:text-neutral-400">ROM Build Version:</span>
              <span className="text-neutral-900 dark:text-neutral-100 text-right truncate max-w-[200px]">
                {profile.firmware.version}
              </span>
            </div>
            <div className="flex justify-between py-1 border-b border-black/5 dark:border-white/5">
              <span className="text-neutral-500 dark:text-neutral-400">APFS Boot ROM Driver:</span>
              <StatusBadge
                label={profile.firmware.apfsSupport ? 'Supported' : 'Requires EFI Inject'}
                tone={profile.firmware.apfsSupport ? 'emerald' : 'amber'}
              />
            </div>
            <div className="flex justify-between py-1">
              <span className="text-neutral-500 dark:text-neutral-400">Apple Secure Boot:</span>
              <span className="text-neutral-900 dark:text-neutral-100">{profile.firmware.secureBootMode}</span>
            </div>
          </div>
        </GlassPanel>

        {/* PCI Devices */}
        <GlassPanel
          id="hardware-pci-section"
          title="PCI Bus Devices"
          action={
            <span className="text-xs text-neutral-400 font-mono">
              {profile.pciDevices.length} Enumerable
            </span>
          }
        >
          <div className="space-y-2 max-h-48 overflow-y-auto pr-1">
            {profile.pciDevices.map((pci, i) => (
              <div
                key={i}
                className="flex items-center justify-between p-2 rounded bg-black/5 dark:bg-white/5 text-xs font-mono"
              >
                <div className="space-y-0.5 truncate max-w-[70%]">
                  <div className="text-neutral-900 dark:text-neutral-100 font-medium truncate">
                    {pci.device}
                  </div>
                  <div className="text-[10px] text-neutral-400">
                    {pci.slot} • {pci.vendorId}:{pci.deviceId}
                  </div>
                </div>
                <StatusBadge label={pci.status} tone="emerald" />
              </div>
            ))}
          </div>
        </GlassPanel>
      </div>
    </div>
  );
};
