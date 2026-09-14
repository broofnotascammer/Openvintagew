import React from 'react';
import { 
  Sun, 
  Moon, 
  Monitor, 
  RefreshCw, 
  ShieldCheck, 
  HardDriveDownload,
  Cpu
} from 'lucide-react';
import { HardwareProfileData } from '../core/openvintageState';
import { StatusBadge } from './StatusBadge';

interface WindowHeaderProps {
  profile: HardwareProfileData;
  theme: 'light' | 'dark' | 'system';
  onThemeChange: (theme: 'light' | 'dark' | 'system') => void;
  onRefreshHardware: () => void;
  onOpenDeployment: () => void;
  isDetecting?: boolean;
}

export const WindowHeader: React.FC<WindowHeaderProps> = ({
  profile,
  theme,
  onThemeChange,
  onRefreshHardware,
  onOpenDeployment,
  isDetecting = false,
}) => {
  const activeGpu = profile.gpus[profile.activeGpuIndex] || profile.gpus[0];
  const isNative = profile.source === 'NATIVE';

  return (
    <header 
      id="macos-window-header"
      className="h-13 px-4 flex items-center justify-between border-b border-black/10 dark:border-white/10 select-none bg-neutral-100/50 dark:bg-neutral-900/40 backdrop-blur-md sticky top-0 z-30"
    >
      {/* Traffic Lights & Title */}
      <div className="flex items-center gap-4">
        {/* macOS Traffic Lights */}
        <div className="flex items-center gap-2" aria-hidden="true">
          <span className="w-3 h-3 rounded-full bg-[#ff5f56] border border-[#e0443e] inline-block shadow-xs transition-opacity hover:opacity-80" />
          <span className="w-3 h-3 rounded-full bg-[#ffbd2e] border border-[#dea123] inline-block shadow-xs transition-opacity hover:opacity-80" />
          <span className="w-3 h-3 rounded-full bg-[#27c93f] border border-[#1aab29] inline-block shadow-xs transition-opacity hover:opacity-80" />
        </div>

        {/* Application Name & Subtitle */}
        <div className="flex items-center gap-2.5">
          <div className="flex items-center gap-1.5 font-semibold text-sm tracking-tight text-neutral-900 dark:text-neutral-100">
            <Cpu className="w-4 h-4 text-blue-500" />
            <span>OpenVintage</span>
          </div>
          <span className="text-xs text-neutral-400 dark:text-neutral-500">v6.1.0</span>
        </div>
      </div>

      {/* Center Silicon / Target Indicator */}
      <div className="hidden md:flex items-center gap-2 text-xs">
        <StatusBadge
          id="header-hardware-source-badge"
          label={isNative ? 'Host Silicon (Native)' : `Simulated: ${profile.modelIdentifier}`}
          tone={isNative ? 'emerald' : 'amber'}
          pulse={isNative}
        />
        <div className="flex items-center gap-1.5 px-2.5 py-0.5 rounded-full bg-black/5 dark:bg-white/5 text-neutral-600 dark:text-neutral-300 border border-black/5 dark:border-white/5 font-mono text-[11px]">
          <span className="text-neutral-400">GPU:</span>
          <span className="font-medium truncate max-w-[170px]">{activeGpu?.modelName}</span>
        </div>
      </div>

      {/* Header Actions */}
      <div className="flex items-center gap-2">
        {/* Hardware Redetect Button */}
        <button
          id="btn-redetect-hardware"
          onClick={onRefreshHardware}
          disabled={isDetecting}
          title="Query hardware abstraction layer"
          className="inline-flex items-center gap-1.5 px-2.5 py-1.5 rounded-lg text-xs font-medium text-neutral-700 dark:text-neutral-300 hover:bg-black/5 dark:hover:bg-white/10 transition-colors disabled:opacity-50"
        >
          <RefreshCw className={`w-3.5 h-3.5 ${isDetecting ? 'animate-spin text-blue-500' : ''}`} />
          <span className="hidden sm:inline">Audit</span>
        </button>

        {/* Appearance Theme Selector */}
        <div 
          id="header-theme-selector"
          className="flex items-center p-0.5 rounded-lg bg-black/5 dark:bg-white/5 border border-black/5 dark:border-white/10 text-neutral-600 dark:text-neutral-400"
        >
          <button
            onClick={() => onThemeChange('light')}
            title="Light appearance"
            aria-label="Light appearance"
            className={`p-1.5 rounded-md transition-all ${
              theme === 'light' 
                ? 'bg-white dark:bg-neutral-800 text-blue-600 shadow-xs' 
                : 'hover:text-neutral-900 dark:hover:text-neutral-200'
            }`}
          >
            <Sun className="w-3.5 h-3.5" />
          </button>
          <button
            onClick={() => onThemeChange('dark')}
            title="Dark appearance"
            aria-label="Dark appearance"
            className={`p-1.5 rounded-md transition-all ${
              theme === 'dark' 
                ? 'bg-white dark:bg-neutral-800 text-blue-400 shadow-xs' 
                : 'hover:text-neutral-900 dark:hover:text-neutral-200'
            }`}
          >
            <Moon className="w-3.5 h-3.5" />
          </button>
          <button
            onClick={() => onThemeChange('system')}
            title="System appearance"
            aria-label="System appearance"
            className={`p-1.5 rounded-md transition-all ${
              theme === 'system' 
                ? 'bg-white dark:bg-neutral-800 text-blue-500 shadow-xs' 
                : 'hover:text-neutral-900 dark:hover:text-neutral-200'
            }`}
          >
            <Monitor className="w-3.5 h-3.5" />
          </button>
        </div>

        {/* Deploy Safe EFI Action Button */}
        <button
          id="btn-header-deploy-efi"
          onClick={onOpenDeployment}
          className="inline-flex items-center gap-1.5 px-3 py-1.5 rounded-lg bg-blue-600 hover:bg-blue-700 active:bg-blue-800 text-white text-xs font-medium shadow-xs transition-colors"
        >
          <HardDriveDownload className="w-3.5 h-3.5" />
          <span>Deploy EFI</span>
        </button>
      </div>
    </header>
  );
};
