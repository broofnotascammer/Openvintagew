import React from 'react';
import {
  LayoutDashboard,
  Cpu,
  Layers,
  CheckCircle2,
  Zap,
  HardDrive,
  Boxes,
  Activity,
  Settings,
  ShieldCheck,
} from 'lucide-react';
import { StatusBadge } from './StatusBadge';

export type NavTab = 
  | 'overview' 
  | 'hardware' 
  | 'simulator' 
  | 'compatibility' 
  | 'performance' 
  | 'boot' 
  | 'integrations' 
  | 'diagnostics' 
  | 'settings';

interface SidebarProps {
  activeTab: NavTab;
  onTabChange: (tab: NavTab) => void;
  hardwareSource: 'NATIVE' | 'SIMULATED';
  activeProfileName: string;
  className?: string;
}

export const Sidebar: React.FC<SidebarProps> = ({
  activeTab,
  onTabChange,
  hardwareSource,
  activeProfileName,
  className = '',
}) => {
  const navItems: Array<{
    id: NavTab;
    label: string;
    icon: React.ComponentType<{ className?: string }>;
    badge?: string;
    badgeTone?: 'emerald' | 'amber' | 'sky' | 'violet';
  }> = [
    { id: 'overview', label: 'Overview', icon: LayoutDashboard },
    { id: 'hardware', label: 'Hardware', icon: Cpu },
    { 
      id: 'simulator', 
      label: 'Simulator', 
      icon: Layers, 
      badge: hardwareSource === 'SIMULATED' ? 'Active' : undefined,
      badgeTone: 'amber' 
    },
    { id: 'compatibility', label: 'Compatibility', icon: CheckCircle2 },
    { id: 'performance', label: 'Performance', icon: Zap },
    { id: 'boot', label: 'Boot Picker', icon: HardDrive },
    { id: 'integrations', label: 'Integrations', icon: Boxes },
    { id: 'diagnostics', label: 'Diagnostics', icon: Activity },
    { id: 'settings', label: 'Settings', icon: Settings },
  ];

  return (
    <aside
      id="macos-sidebar"
      aria-label="Navigation Sidebar"
      className={`w-56 shrink-0 flex flex-col justify-between py-4 px-2 select-none macos-sidebar-light dark:macos-sidebar-dark ${className}`}
    >
      <div className="space-y-4">
        {/* Section Header */}
        <div className="px-3 pt-1">
          <span className="text-[11px] font-medium uppercase tracking-wider text-neutral-400 dark:text-neutral-500">
            System Subsystems
          </span>
        </div>

        {/* Navigation List */}
        <nav className="space-y-0.5" role="tablist">
          {navItems.map((item) => {
            const Icon = item.icon;
            const isSelected = activeTab === item.id;

            return (
              <button
                key={item.id}
                id={`sidebar-tab-${item.id}`}
                role="tab"
                aria-selected={isSelected}
                onClick={() => onTabChange(item.id)}
                className={`w-full flex items-center justify-between px-3 py-2 rounded-lg text-xs font-medium transition-all group ${
                  isSelected
                    ? 'bg-blue-600/15 text-blue-700 dark:bg-blue-500/20 dark:text-blue-300 font-semibold shadow-xs'
                    : 'text-neutral-600 hover:text-neutral-900 hover:bg-black/5 dark:text-neutral-400 dark:hover:text-neutral-200 dark:hover:bg-white/5'
                }`}
              >
                <div className="flex items-center gap-2.5">
                  <Icon
                    className={`w-4 h-4 transition-colors ${
                      isSelected
                        ? 'text-blue-600 dark:text-blue-400'
                        : 'text-neutral-400 group-hover:text-neutral-600 dark:text-neutral-500 dark:group-hover:text-neutral-300'
                    }`}
                  />
                  <span>{item.label}</span>
                </div>

                {item.badge && (
                  <span
                    className={`text-[10px] px-1.5 py-0.2 rounded font-mono ${
                      item.badgeTone === 'amber'
                        ? 'bg-amber-500/15 text-amber-600 dark:text-amber-300'
                        : 'bg-neutral-500/15 text-neutral-600 dark:text-neutral-400'
                    }`}
                  >
                    {item.badge}
                  </span>
                )}
              </button>
            );
          })}
        </nav>
      </div>

      {/* Sidebar Footer / Security Sentinel */}
      <div className="px-2 pt-4 border-t border-black/5 dark:border-white/5 space-y-2 text-xs">
        <div className="flex items-center gap-2 text-neutral-500 dark:text-neutral-400 px-1">
          <ShieldCheck className="w-3.5 h-3.5 text-emerald-500 shrink-0" />
          <span className="text-[11px] truncate">Guardrails: Enforced</span>
        </div>
        <div className="px-2 py-1.5 rounded-md bg-black/5 dark:bg-white/5 text-[11px] text-neutral-500 dark:text-neutral-400 font-mono truncate">
          {activeProfileName}
        </div>
      </div>
    </aside>
  );
};
