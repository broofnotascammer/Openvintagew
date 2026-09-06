import React from 'react';
import { Cpu, ShieldCheck, Terminal, Layers } from 'lucide-react';
import { PerformanceProfile } from '../types';

interface HeaderProps {
  activeTab: string;
  setActiveTab: (tab: string) => void;
  currentProfile: PerformanceProfile;
  onProfileChange: (p: PerformanceProfile) => void;
}

export const Header: React.FC<HeaderProps> = ({
  activeTab,
  setActiveTab,
  currentProfile,
  onProfileChange,
}) => {
  const tabs = [
    { id: 'overview', label: 'Ecosystem Architecture' },
    { id: 'resolver', label: 'OVResolver Engine' },
    { id: 'scheduler', label: 'OVScheduler' },
    { id: 'hardware', label: 'Hardware Database (HAL)' },
    { id: 'phase1', label: 'Phase 1 UEFI Logs' },
    { id: 'docs', label: 'Specifications & Status' },
  ];

  return (
    <header id="ov-header" className="border-b border-neutral-800 bg-neutral-950 text-neutral-100">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
        <div className="flex flex-col md:flex-row md:items-center md:justify-between py-4 gap-4">
          <div className="flex items-center space-x-3">
            <div className="w-10 h-10 rounded-lg bg-neutral-800 border border-neutral-700 flex items-center justify-center text-amber-400 font-mono font-bold text-lg">
              OV
            </div>
            <div>
              <div className="flex items-center space-x-2">
                <h1 className="text-xl font-bold tracking-tight text-white font-mono">OPENVINTAGE</h1>
                <span className="inline-flex items-center px-2 py-0.5 rounded text-xs font-medium bg-emerald-950 text-emerald-300 border border-emerald-800">
                  <ShieldCheck className="w-3 h-3 mr-1 text-emerald-400" />
                  Phase 1 Verified
                </span>
                <span className="inline-flex items-center px-2 py-0.5 rounded text-xs font-medium bg-amber-950 text-amber-300 border border-amber-800">
                  Phase 2 Active
                </span>
              </div>
              <p className="text-xs text-neutral-400">
                Modular Compatibility &amp; Performance Platform for Legacy Intel Mac Hardware
              </p>
            </div>
          </div>

          <div className="flex items-center space-x-3">
            <div className="flex items-center bg-neutral-900 border border-neutral-800 rounded-lg p-1 text-xs">
              <span className="text-neutral-400 px-2 flex items-center">
                <Cpu className="w-3.5 h-3.5 mr-1" />
                Profile:
              </span>
              {(['battery', 'balanced', 'performance', 'max_performance', 'developer'] as PerformanceProfile[]).map(
                (p) => (
                  <button
                    key={p}
                    id={`profile-btn-${p}`}
                    onClick={() => onProfileChange(p)}
                    className={`px-2.5 py-1 rounded transition-colors uppercase font-mono text-[11px] ${
                      currentProfile === p
                        ? 'bg-amber-500/20 text-amber-300 border border-amber-500/50 font-semibold'
                        : 'text-neutral-400 hover:text-neutral-200'
                    }`}
                  >
                    {p.replace('_', ' ')}
                  </button>
                )
              )}
            </div>
          </div>
        </div>

        {/* Tab Navigation */}
        <nav id="ov-nav" className="flex space-x-1 border-t border-neutral-900 pt-2 overflow-x-auto">
          {tabs.map((tab) => (
            <button
              key={tab.id}
              id={`nav-tab-${tab.id}`}
              onClick={() => setActiveTab(tab.id)}
              className={`px-3.5 py-2 text-xs font-medium whitespace-nowrap rounded-t border-b-2 transition-all ${
                activeTab === tab.id
                  ? 'border-amber-400 text-amber-400 bg-neutral-900/60'
                  : 'border-transparent text-neutral-400 hover:text-neutral-200 hover:bg-neutral-900/30'
              }`}
            >
              {tab.label}
            </button>
          ))}
        </nav>
      </div>
    </header>
  );
};
