import React, { useState } from 'react';
import {
  Settings,
  Sun,
  Moon,
  Monitor,
  RotateCcw,
  Check,
  Info,
  Lock,
  Terminal,
} from 'lucide-react';
import { GlassPanel } from './GlassPanel';
import { StatusBadge } from './StatusBadge';

interface SettingsViewProps {
  theme: 'light' | 'dark' | 'system';
  onThemeChange: (theme: 'light' | 'dark' | 'system') => void;
  hardwarePreference: 'NATIVE' | 'SIMULATED';
  onHardwarePreferenceChange: (pref: 'NATIVE' | 'SIMULATED') => void;
}

export const SettingsView: React.FC<SettingsViewProps> = ({
  theme,
  onThemeChange,
  hardwarePreference,
  onHardwarePreferenceChange,
}) => {
  const [strictGuardrails, setStrictGuardrails] = useState(true);
  const [requireApproval, setRequireApproval] = useState(true);
  const [backupRetentionDays, setBackupRetentionDays] = useState(30);
  const [loggingVerbosity, setLoggingVerbosity] = useState<'DEBUG' | 'INFO' | 'WARN' | 'ERROR'>('INFO');
  const [savedNotification, setSavedNotification] = useState<string | null>(null);

  const handleResetDefaults = () => {
    onThemeChange('system');
    onHardwarePreferenceChange('SIMULATED');
    setStrictGuardrails(true);
    setRequireApproval(true);
    setBackupRetentionDays(30);
    setLoggingVerbosity('INFO');
    setSavedNotification('Settings reset to recommended preview defaults.');
    setTimeout(() => setSavedNotification(null), 3000);
  };

  return (
    <div id="settings-page-view" className="space-y-6">
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4 p-5 rounded-xl glass-card-light dark:glass-card-dark border border-black/10 dark:border-white/10">
        <div>
          <h2 className="text-xl font-bold tracking-tight text-neutral-900 dark:text-neutral-100 flex items-center gap-2">
            <Settings className="w-5 h-5 text-neutral-500" />
            <span>OpenVintage Preferences</span>
          </h2>
          <p className="text-xs text-neutral-500 dark:text-neutral-400 mt-0.5">
            Appearance, simulation defaults, and security guardrail configuration for the browser preview.
          </p>
        </div>

        <button
          onClick={handleResetDefaults}
          className="inline-flex items-center gap-1.5 px-3 py-1.5 rounded-lg border border-black/10 dark:border-white/10 bg-black/5 dark:bg-white/5 hover:bg-black/10 text-xs font-medium text-neutral-700 dark:text-neutral-300 transition-colors self-start sm:self-auto"
        >
          <RotateCcw className="w-3.5 h-3.5" />
          <span>Reset Defaults</span>
        </button>
      </div>

      {savedNotification && (
        <div className="p-3 rounded-xl bg-emerald-500/15 border border-emerald-500/30 text-emerald-700 dark:text-emerald-300 text-xs flex items-center gap-2">
          <Check className="w-4 h-4" />
          <span>{savedNotification}</span>
        </div>
      )}

      <GlassPanel id="settings-appearance-panel" title="Appearance &amp; Visual Theme">
        <div className="space-y-4">
          <p className="text-xs text-neutral-500 dark:text-neutral-400">
            Choose the appearance used by the browser preview. The native macOS application uses the system visual-effect materials directly.
          </p>
          <div className="grid grid-cols-3 gap-3 max-w-md">
            <button id="settings-theme-light" onClick={() => onThemeChange('light')} className={`p-3 rounded-xl border flex flex-col items-center gap-2 transition-all ${theme === 'light' ? 'border-blue-500 bg-blue-500/10 text-blue-600 font-semibold ring-1 ring-blue-500/30' : 'border-black/5 dark:border-white/10 text-neutral-600 dark:text-neutral-300 hover:border-black/20'}`}>
              <Sun className="w-5 h-5" />
              <span className="text-xs">Light</span>
            </button>
            <button id="settings-theme-dark" onClick={() => onThemeChange('dark')} className={`p-3 rounded-xl border flex flex-col items-center gap-2 transition-all ${theme === 'dark' ? 'border-blue-500 bg-blue-500/10 text-blue-400 font-semibold ring-1 ring-blue-500/30' : 'border-black/5 dark:border-white/10 text-neutral-600 dark:text-neutral-300 hover:border-black/20'}`}>
              <Moon className="w-5 h-5" />
              <span className="text-xs">Dark</span>
            </button>
            <button id="settings-theme-system" onClick={() => onThemeChange('system')} className={`p-3 rounded-xl border flex flex-col items-center gap-2 transition-all ${theme === 'system' ? 'border-blue-500 bg-blue-500/10 text-blue-500 font-semibold ring-1 ring-blue-500/30' : 'border-black/5 dark:border-white/10 text-neutral-600 dark:text-neutral-300 hover:border-black/20'}`}>
              <Monitor className="w-5 h-5" />
              <span className="text-xs">System</span>
            </button>
          </div>
        </div>
      </GlassPanel>

      <GlassPanel id="settings-hardware-panel" title="Hardware Source">
        <div className="space-y-4">
          <p className="text-xs text-neutral-500 dark:text-neutral-400">
            The browser preview cannot access Darwin hardware or kernel interfaces. Native hardware discovery is performed by the real macOS application.
          </p>

          <div className="grid grid-cols-1 sm:grid-cols-2 gap-3 max-w-xl">
            <div id="settings-pref-native" className="p-3.5 rounded-xl border border-black/5 dark:border-white/10 opacity-65 cursor-not-allowed">
              <div className="flex items-center justify-between mb-1">
                <span className="text-xs font-bold text-neutral-900 dark:text-neutral-100">Host Native Silicon</span>
                <StatusBadge label="Native app" tone="emerald" />
              </div>
              <p className="text-[11px] text-neutral-500">Available in the native SwiftUI/AppKit application, where sysctl and IOKit are accessible.</p>
            </div>

            <div
              id="settings-pref-simulated"
              onClick={() => onHardwarePreferenceChange('SIMULATED')}
              className={`p-3.5 rounded-xl border cursor-pointer transition-all ${
                hardwarePreference === 'SIMULATED'
                  ? 'border-amber-500 bg-amber-500/10 ring-1 ring-amber-500/30'
                  : 'border-black/5 dark:border-white/10 hover:border-black/20'
              }`}
            >
              <div className="flex items-center justify-between mb-1">
                <span className="text-xs font-bold text-neutral-900 dark:text-neutral-100">Simulated Mac Profile (MBP 9,1)</span>
                <StatusBadge label="Simulated" tone="amber" />
              </div>
              <p className="text-[11px] text-neutral-500">Loads the mid-2012 dual-GPU MacBook Pro architecture for safe UI, compatibility, and resolver testing.</p>
            </div>
          </div>

          <div className="flex items-start gap-2 text-[11px] text-neutral-500 dark:text-neutral-400 p-3 rounded-lg bg-black/5 dark:bg-white/5">
            <Info className="w-3.5 h-3.5 shrink-0 mt-0.5" />
            <span>Never treat simulated hardware as a report about the physical machine. The native app is the authoritative Darwin hardware path.</span>
          </div>
        </div>
      </GlassPanel>

      <GlassPanel id="settings-security-panel" title="Security &amp; Privilege Guardrails">
        <div className="space-y-4 text-xs">
          <div className="flex items-center justify-between py-2 border-b border-black/5 dark:border-white/5">
            <div>
              <div className="font-semibold text-neutral-900 dark:text-neutral-100">Strict Guardrail Enforcement</div>
              <div className="text-neutral-500 text-[11px]">Validates path boundaries and disallows unsafe raw device operations.</div>
            </div>
            <input type="checkbox" checked={strictGuardrails} onChange={(e) => setStrictGuardrails(e.target.checked)} className="rounded border-neutral-400 text-blue-600 focus:ring-blue-500" />
          </div>

          <div className="flex items-center justify-between py-2 border-b border-black/5 dark:border-white/5">
            <div>
              <div className="font-semibold text-neutral-900 dark:text-neutral-100">Require User Confirmation for EFI Deployment</div>
              <div className="text-neutral-500 text-[11px]">Halts deployment at the approval gate for manual inspection of planned changes and hashes.</div>
            </div>
            <input type="checkbox" checked={requireApproval} onChange={(e) => setRequireApproval(e.target.checked)} className="rounded border-neutral-400 text-blue-600 focus:ring-blue-500" />
          </div>

          <div className="flex items-center justify-between py-2 border-b border-black/5 dark:border-white/5">
            <div>
              <div className="font-semibold text-neutral-900 dark:text-neutral-100">Logging Verbosity Level</div>
              <div className="text-neutral-500 text-[11px]">Controls preview diagnostics and native integration tracing.</div>
            </div>
            <select value={loggingVerbosity} onChange={(e) => setLoggingVerbosity(e.target.value as 'DEBUG' | 'INFO' | 'WARN' | 'ERROR')} className="text-xs font-mono p-1.5 rounded border border-black/10 dark:border-white/10 bg-white/50 dark:bg-neutral-900/60">
              <option value="DEBUG">DEBUG (Detailed)</option>
              <option value="INFO">INFO (Normal)</option>
              <option value="WARN">WARN (Warnings only)</option>
              <option value="ERROR">ERROR (Fatal only)</option>
            </select>
          </div>

          <div className="flex items-center gap-2 text-neutral-500 text-[11px]">
            <Lock className="w-3.5 h-3.5" />
            <span>Privileged operations belong to the native Core/Security boundary, never directly to browser JavaScript.</span>
          </div>

          <div className="flex items-center gap-2 text-neutral-500 text-[11px]">
            <Terminal className="w-3.5 h-3.5" />
            <span>Backup retention: {backupRetentionDays} days.</span>
          </div>
        </div>
      </GlassPanel>
    </div>
  );
};
