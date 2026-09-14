/**
 * OpenVintage - macOS Glass Architecture Desktop Application (Phase 6.1)
 * Hardware-Aware Boot Picker, Silicon Capability Translator, and Safe Deployment Engine.
 */

import React, { useState, useEffect } from 'react';
import { 
  WindowHeader 
} from './components/WindowHeader';
import { 
  Sidebar, 
  NavTab 
} from './components/Sidebar';
import { 
  OverviewView 
} from './components/OverviewView';
import { 
  HardwareView 
} from './components/HardwareView';
import { 
  SimulatorView 
} from './components/SimulatorView';
import { 
  CompatibilityView 
} from './components/CompatibilityView';
import { 
  PerformanceView 
} from './components/PerformanceView';
import { 
  BootView 
} from './components/BootView';
import { 
  IntegrationsView 
} from './components/IntegrationsView';
import { 
  DiagnosticsView 
} from './components/DiagnosticsView';
import { 
  SettingsView 
} from './components/SettingsView';
import { 
  DeploymentSheet 
} from './components/DeploymentSheet';
import {
  HardwareProfileData,
  PRESET_SIMULATED_PROFILES,
  NATIVE_HOST_HARDWARE_PROFILE,
  PERFORMANCE_PROFILES,
  PerformanceProfileDef,
  PerformanceProfileType,
  DEFAULT_BOOT_TARGETS,
  BootTarget,
  DEFAULT_INTEGRATIONS,
  IntegrationStatus,
} from './core/openvintageState';
import { Menu, X, ShieldCheck } from 'lucide-react';

export default function App() {
  const [activeTab, setActiveTab] = useState<NavTab>('overview');
  const [theme, setTheme] = useState<'light' | 'dark' | 'system'>('dark');
  const [hardwarePreference, setHardwarePreference] = useState<'NATIVE' | 'SIMULATED'>('SIMULATED');
  
  // Hardware profile state (starts with MBP 9,1 Ivy Bridge dual-GPU preset or host native)
  const [profile, setProfile] = useState<HardwareProfileData>(
    PRESET_SIMULATED_PROFILES[0] // MacBookPro9,1
  );

  // Active Performance Profile
  const [activeProfileId, setActiveProfileId] = useState<PerformanceProfileType>('BALANCED');
  const activeProfile = PERFORMANCE_PROFILES.find((p) => p.id === activeProfileId) || PERFORMANCE_PROFILES[2];

  // Boot Targets
  const [bootTargets, setBootTargets] = useState<BootTarget[]>(DEFAULT_BOOT_TARGETS);

  // Integrations Status
  const [integrations, setIntegrations] = useState<IntegrationStatus[]>(DEFAULT_INTEGRATIONS);

  // UI state
  const [isDeploymentOpen, setIsDeploymentOpen] = useState<boolean>(false);
  const [isDetecting, setIsDetecting] = useState<boolean>(false);
  const [mobileMenuOpen, setMobileMenuOpen] = useState<boolean>(false);

  // Sync theme with HTML root class
  useEffect(() => {
    const root = document.documentElement;
    if (theme === 'dark') {
      root.classList.add('dark');
      root.classList.remove('light');
    } else if (theme === 'light') {
      root.classList.remove('dark');
      root.classList.add('light');
    } else {
      // System mode
      const prefersDark = window.matchMedia('(prefers-color-scheme: dark)').matches;
      if (prefersDark) {
        root.classList.add('dark');
        root.classList.remove('light');
      } else {
        root.classList.remove('dark');
        root.classList.add('light');
      }
    }
  }, [theme]);

  // Handle Hardware Re-audit
  const handleRefreshHardware = () => {
    setIsDetecting(true);
    setTimeout(() => {
      setIsDetecting(false);
    }, 700);
  };

  // Switch Active GPU (preserves physical inventory!)
  const handleSelectGpu = (index: number) => {
    if (index >= 0 && index < profile.gpus.length) {
      setProfile((prev) => ({
        ...prev,
        activeGpuIndex: index,
      }));
    }
  };

  // Set Default Boot Target
  const handleSetDefaultBoot = (targetId: string) => {
    setBootTargets((prev) =>
      prev.map((t) => ({
        ...t,
        isDefault: t.id === targetId,
      }))
    );
  };

  // Switch between Native and Simulated preference
  const handleHardwarePreferenceChange = (pref: 'NATIVE' | 'SIMULATED') => {
    setHardwarePreference(pref);
    if (pref === 'NATIVE') {
      setProfile(NATIVE_HOST_HARDWARE_PROFILE);
    } else {
      setProfile(PRESET_SIMULATED_PROFILES[0]);
    }
  };

  // Apply simulated customized profile
  const handleApplySimulatedProfile = (newProfile: HardwareProfileData) => {
    setProfile(newProfile);
    setActiveTab('hardware');
  };

  return (
    <div className="min-h-screen bg-black text-neutral-100 flex flex-col items-center justify-center p-0 sm:p-4 md:p-6 lg:p-8 font-sans selection:bg-blue-500/30 selection:text-blue-200 transition-colors relative overflow-hidden">
      {/* Background macOS desktop wallpaper glow to enable rich glassmorphic refraction */}
      <div 
        className="fixed inset-0 pointer-events-none" 
        aria-hidden="true" 
      >
        <div className="absolute inset-0 bg-black" />
        {/* Dynamic deep glass refraction glows */}
        <div className="absolute -top-[15%] left-[10%] w-[650px] h-[650px] rounded-full bg-blue-600/15 blur-[130px] dark:opacity-85 opacity-30 pointer-events-none" />
        <div className="absolute -bottom-[15%] right-[10%] w-[700px] h-[700px] rounded-full bg-purple-600/15 blur-[150px] dark:opacity-80 opacity-25 pointer-events-none" />
        <div className="absolute top-[35%] right-[25%] w-[500px] h-[500px] rounded-full bg-indigo-500/10 blur-[120px] dark:opacity-65 opacity-20 pointer-events-none" />
        <div className="absolute inset-0 bg-[radial-gradient(ellipse_80%_80%_at_50%_0%,rgba(120,119,198,0.12),transparent)]" />
      </div>

      {/* Main macOS Desktop Applet Window Frame */}
      <div 
        id="macos-applet-window"
        className="relative z-10 w-full max-w-7xl h-[100vh] sm:h-[90vh] sm:min-h-[640px] flex flex-col rounded-none sm:rounded-2xl macos-window-light dark:macos-window-dark overflow-hidden shadow-2xl border-0 sm:border border-black/10 dark:border-white/12"
      >
        {/* macOS Window Titlebar & Controls */}
        <WindowHeader
          profile={profile}
          theme={theme}
          onThemeChange={setTheme}
          onRefreshHardware={handleRefreshHardware}
          onOpenDeployment={() => setIsDeploymentOpen(true)}
          isDetecting={isDetecting}
        />

        {/* Mobile Sidebar Toggle Header (Small screens only) */}
        <div className="md:hidden flex items-center justify-between px-4 py-2 border-b border-black/5 dark:border-white/5 bg-black/2 dark:bg-white/2">
          <button
            onClick={() => setMobileMenuOpen(!mobileMenuOpen)}
            className="flex items-center gap-2 text-xs font-medium text-neutral-700 dark:text-neutral-300"
          >
            {mobileMenuOpen ? <X className="w-4 h-4" /> : <Menu className="w-4 h-4" />}
            <span className="capitalize">{activeTab}</span>
          </button>
          <div className="text-[11px] font-mono text-neutral-400">
            {profile.marketingName}
          </div>
        </div>

        {/* Core Workspace Layout: Sidebar + Main Content Area */}
        <div className="flex-1 flex overflow-hidden relative">
          {/* Desktop Persistent Sidebar */}
          <Sidebar
            activeTab={activeTab}
            onTabChange={(tab) => {
              setActiveTab(tab);
              setMobileMenuOpen(false);
            }}
            hardwareSource={profile.source}
            activeProfileName={profile.modelIdentifier}
            className="hidden md:flex"
          />

          {/* Mobile Overlay Sidebar */}
          {mobileMenuOpen && (
            <div className="md:hidden absolute inset-0 z-40 bg-neutral-900/80 backdrop-blur-md flex">
              <Sidebar
                activeTab={activeTab}
                onTabChange={(tab) => {
                  setActiveTab(tab);
                  setMobileMenuOpen(false);
                }}
                hardwareSource={profile.source}
                activeProfileName={profile.modelIdentifier}
                className="w-64 h-full shadow-2xl"
              />
              <div className="flex-1" onClick={() => setMobileMenuOpen(false)} />
            </div>
          )}

          {/* Main Scrollable View Area */}
          <main 
            id="macos-content-canvas"
            className="flex-1 overflow-y-auto p-4 sm:p-6 lg:p-8 space-y-6"
            role="main"
          >
            {activeTab === 'overview' && (
              <OverviewView
                profile={profile}
                activeProfile={activeProfile}
                bootTargets={bootTargets}
                integrations={integrations}
                onNavigate={setActiveTab}
                onSelectGpu={handleSelectGpu}
                onOpenDeployment={() => setIsDeploymentOpen(true)}
              />
            )}

            {activeTab === 'hardware' && (
              <HardwareView
                profile={profile}
                onSelectGpu={handleSelectGpu}
                onRefreshHardware={handleRefreshHardware}
                isDetecting={isDetecting}
              />
            )}

            {activeTab === 'simulator' && (
              <SimulatorView
                currentProfile={profile}
                onApplyProfile={handleApplySimulatedProfile}
              />
            )}

            {activeTab === 'compatibility' && (
              <CompatibilityView
                profile={profile}
                onNavigate={setActiveTab}
              />
            )}

            {activeTab === 'performance' && (
              <PerformanceView
                currentProfile={activeProfile}
                hardwareProfile={profile}
                onSelectProfile={setActiveProfileId}
              />
            )}

            {activeTab === 'boot' && (
              <BootView
                targets={bootTargets}
                onSetDefault={handleSetDefaultBoot}
                onRefreshTargets={handleRefreshHardware}
              />
            )}

            {activeTab === 'integrations' && (
              <IntegrationsView
                integrations={integrations}
                profile={profile}
                onOpenDeployment={() => setIsDeploymentOpen(true)}
              />
            )}

            {activeTab === 'diagnostics' && (
              <DiagnosticsView
                profile={profile}
                activeProfile={activeProfile}
                bootTargets={bootTargets}
                integrations={integrations}
              />
            )}

            {activeTab === 'settings' && (
              <SettingsView
                theme={theme}
                onThemeChange={setTheme}
                hardwarePreference={hardwarePreference}
                onHardwarePreferenceChange={handleHardwarePreferenceChange}
              />
            )}
          </main>
        </div>

        {/* macOS Style Window Status Footer */}
        <footer 
          id="macos-window-footer"
          className="h-8 px-4 border-t border-black/10 dark:border-white/10 flex items-center justify-between text-[11px] font-mono text-neutral-500 dark:text-neutral-400 bg-neutral-100/40 dark:bg-neutral-950/40 select-none shrink-0"
        >
          <div className="flex items-center gap-3">
            <span>OpenVintage v6.1.0</span>
            <span>&middot;</span>
            <span className="text-emerald-600 dark:text-emerald-400 flex items-center gap-1">
              <ShieldCheck className="w-3 h-3" />
              <span>Core HAL: 272/272 Tests Passing</span>
            </span>
          </div>

          <div className="flex items-center gap-3">
            <span className="hidden sm:inline">Active Target: {profile.modelIdentifier}</span>
            <span>&middot;</span>
            <span className="text-blue-600 dark:text-blue-400">{activeProfile.name}</span>
          </div>
        </footer>
      </div>

      {/* 9-Step Deployment Modal Sheet */}
      <DeploymentSheet
        isOpen={isDeploymentOpen}
        onClose={() => setIsDeploymentOpen(false)}
        profile={profile}
      />
    </div>
  );
}
