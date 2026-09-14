import React, { useState } from 'react';
import { 
  HardDrive, 
  Play, 
  Star, 
  Settings, 
  ShieldCheck, 
  CheckCircle2, 
  AlertCircle,
  ExternalLink,
  Info,
  Terminal,
  RotateCw
} from 'lucide-react';
import { BootTarget, PerformanceProfileType } from '../core/openvintageState';
import { GlassPanel } from './GlassPanel';
import { StatusBadge } from './StatusBadge';

interface BootViewProps {
  targets: BootTarget[];
  onSetDefault: (targetId: string) => void;
  onRefreshTargets: () => void;
}

export const BootView: React.FC<BootViewProps> = ({
  targets,
  onSetDefault,
  onRefreshTargets,
}) => {
  const [selectedTargetId, setSelectedTargetId] = useState<string>(targets[0]?.id || 'target-0');
  const [bootingMessage, setBootingMessage] = useState<string | null>(null);

  const selectedTarget = targets.find((t) => t.id === selectedTargetId) || targets[0];

  const handleSimulateBoot = (target: BootTarget) => {
    setBootingMessage(`Preparing OpenVintage EFI handoff for "${target.title}" at ${target.efiPath}...`);
    setTimeout(() => {
      setBootingMessage(`Handoff simulated successfully. System would chainload ${target.architecture} EFI stub.`);
      setTimeout(() => setBootingMessage(null), 4000);
    }, 1500);
  };

  return (
    <div id="boot-picker-page-view" className="space-y-6">
      {/* Intro Header */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4 p-5 rounded-xl glass-card-light dark:glass-card-dark border border-black/10 dark:border-white/10">
        <div>
          <h2 className="text-xl font-bold tracking-tight text-neutral-900 dark:text-neutral-100 flex items-center gap-2">
            <HardDrive className="w-5 h-5 text-blue-500" />
            <span>OpenVintage Boot Target Manager</span>
          </h2>
          <p className="text-xs text-neutral-500 dark:text-neutral-400 mt-0.5">
            Discover, audit, and configure EFI boot targets, chainloaders, and fallback operating systems.
          </p>
        </div>

        <div className="flex items-center gap-2">
          <button
            id="btn-rescan-boot-targets"
            onClick={onRefreshTargets}
            className="inline-flex items-center gap-1.5 px-3 py-1.5 rounded-lg border border-black/10 dark:border-white/10 bg-black/5 dark:bg-white/5 hover:bg-black/10 text-xs font-medium text-neutral-700 dark:text-neutral-300 transition-colors"
          >
            <RotateCw className="w-3.5 h-3.5" />
            <span>Rescan Volumes</span>
          </button>
        </div>
      </div>

      {/* Booting Notification Toast */}
      {bootingMessage && (
        <div className="p-3.5 rounded-xl bg-blue-600 text-white text-xs font-medium flex items-center gap-2.5 shadow-md animate-in fade-in">
          <Play className="w-4 h-4 animate-pulse" />
          <span>{bootingMessage}</span>
        </div>
      )}

      {/* Boot Targets Cards List */}
      <div className="space-y-3">
        {targets.map((target) => {
          const isSelected = selectedTargetId === target.id;

          return (
            <div
              key={target.id}
              id={`boot-entry-card-${target.id}`}
              onClick={() => setSelectedTargetId(target.id)}
              className={`p-4 rounded-xl border transition-all cursor-pointer ${
                isSelected
                  ? 'border-blue-500/50 bg-blue-500/5 dark:bg-blue-500/10 ring-1 ring-blue-500/30 shadow-xs'
                  : 'glass-card-light dark:glass-card-dark border-black/5 dark:border-white/5 hover:border-black/15 dark:hover:border-white/15'
              }`}
            >
              <div className="flex flex-col md:flex-row md:items-center justify-between gap-4">
                <div className="flex items-start gap-3.5">
                  <div className="p-2.5 rounded-xl bg-black/5 dark:bg-white/5 text-blue-600 dark:text-blue-400 shrink-0">
                    <HardDrive className="w-5 h-5" />
                  </div>

                  <div className="space-y-1">
                    <div className="flex items-center gap-2 flex-wrap">
                      <h3 className="text-sm font-bold text-neutral-900 dark:text-neutral-100">
                        {target.title}
                      </h3>
                      {target.isDefault && (
                        <span className="inline-flex items-center gap-1 px-2 py-0.5 rounded-full text-[10px] font-bold bg-blue-600 text-white">
                          <Star className="w-2.5 h-2.5 fill-current" />
                          <span>DEFAULT BOOT</span>
                        </span>
                      )}
                      <StatusBadge
                        label={target.compatibilityState === 'REQUIRES_OCLP' ? 'Requires OCLP' : 'Native EFI'}
                        tone={target.compatibilityState === 'REQUIRES_OCLP' ? 'amber' : 'emerald'}
                        dot={true}
                      />
                    </div>

                    <div className="text-xs font-mono text-neutral-500 dark:text-neutral-400 flex items-center gap-3 flex-wrap">
                      <span>Volume: {target.volumeName}</span>
                      <span>•</span>
                      <span>Arch: {target.architecture}</span>
                      <span>•</span>
                      <span className="text-blue-600 dark:text-blue-400">Profile: {target.recommendedProfile}</span>
                    </div>

                    <div className="text-[11px] font-mono text-neutral-400 dark:text-neutral-500 truncate max-w-xl">
                      {target.efiPath}
                    </div>
                  </div>
                </div>

                {/* Actions */}
                <div className="flex items-center gap-2 self-end md:self-center shrink-0">
                  {!target.isDefault && (
                    <button
                      id={`btn-set-default-${target.id}`}
                      onClick={(e) => {
                        e.stopPropagation();
                        onSetDefault(target.id);
                      }}
                      className="px-2.5 py-1.5 rounded-lg border border-black/10 dark:border-white/10 hover:bg-black/5 dark:hover:bg-white/5 text-xs font-medium text-neutral-700 dark:text-neutral-300 transition-colors"
                    >
                      Set Default
                    </button>
                  )}

                  <button
                    id={`btn-simulate-boot-${target.id}`}
                    onClick={(e) => {
                      e.stopPropagation();
                      handleSimulateBoot(target);
                    }}
                    className="inline-flex items-center gap-1.5 px-3 py-1.5 rounded-lg bg-blue-600 hover:bg-blue-700 active:bg-blue-800 text-white text-xs font-medium transition-colors shadow-xs"
                  >
                    <Play className="w-3.5 h-3.5" />
                    <span>Boot Entry</span>
                  </button>
                </div>
              </div>
            </div>
          );
        })}
      </div>

      {/* Target Inspector Glass Panel */}
      <GlassPanel
        id="boot-inspector-panel"
        title={`EFI Boot Entry Inspector: ${selectedTarget.title}`}
        action={
          <span className="text-xs font-mono text-neutral-400">
            Target ID: {selectedTarget.id}
          </span>
        }
      >
        <div className="grid grid-cols-1 md:grid-cols-2 gap-4 text-xs font-mono">
          <div className="space-y-2.5 p-3.5 rounded-lg bg-black/5 dark:bg-white/5">
            <div className="flex justify-between border-b border-black/5 dark:border-white/5 pb-1.5">
              <span className="text-neutral-400">Target Operating System:</span>
              <span className="text-neutral-900 dark:text-neutral-100 font-semibold">{selectedTarget.osName}</span>
            </div>
            <div className="flex justify-between border-b border-black/5 dark:border-white/5 pb-1.5">
              <span className="text-neutral-400">Version Identifier:</span>
              <span className="text-neutral-900 dark:text-neutral-100">{selectedTarget.version}</span>
            </div>
            <div className="flex justify-between border-b border-black/5 dark:border-white/5 pb-1.5">
              <span className="text-neutral-400">Binary Architecture:</span>
              <span className="text-neutral-900 dark:text-neutral-100">{selectedTarget.architecture}</span>
            </div>
            <div className="flex justify-between">
              <span className="text-neutral-400">Default Boot Priority:</span>
              <span className="text-neutral-900 dark:text-neutral-100">{selectedTarget.isDefault ? 'First Priority' : 'Standard'}</span>
            </div>
          </div>

          <div className="space-y-2.5 p-3.5 rounded-lg bg-black/5 dark:bg-white/5">
            <div className="flex justify-between border-b border-black/5 dark:border-white/5 pb-1.5">
              <span className="text-neutral-400">Volume Storage Device:</span>
              <span className="text-neutral-900 dark:text-neutral-100 font-semibold truncate max-w-[180px]">{selectedTarget.volumeName}</span>
            </div>
            <div className="flex justify-between border-b border-black/5 dark:border-white/5 pb-1.5">
              <span className="text-neutral-400">EFI Executable:</span>
              <span className="text-blue-600 dark:text-blue-400 truncate max-w-[180px]">{selectedTarget.efiPath}</span>
            </div>
            <div className="flex justify-between border-b border-black/5 dark:border-white/5 pb-1.5">
              <span className="text-neutral-400">Recommended Profile:</span>
              <span className="text-neutral-900 dark:text-neutral-100">{selectedTarget.recommendedProfile}</span>
            </div>
            <div className="flex justify-between">
              <span className="text-neutral-400">Patching State:</span>
              <span className="text-neutral-900 dark:text-neutral-100">{selectedTarget.compatibilityState}</span>
            </div>
          </div>
        </div>
      </GlassPanel>
    </div>
  );
};
