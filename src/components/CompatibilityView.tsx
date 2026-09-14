import React, { useState } from 'react';
import { 
  CheckCircle2, 
  AlertTriangle, 
  XCircle, 
  Cpu, 
  Monitor, 
  HardDrive, 
  ShieldCheck, 
  ExternalLink,
  Layers,
  Sparkles,
  Info
} from 'lucide-react';
import { 
  HardwareProfileData, 
  TARGET_OS_LIST, 
  evaluateOsCompatibility,
  OsCompatibilityMatrix
} from '../core/openvintageState';
import { GlassPanel } from './GlassPanel';
import { StatusBadge } from './StatusBadge';
import { NavTab } from './Sidebar';

interface CompatibilityViewProps {
  profile: HardwareProfileData;
  onNavigate: (tab: NavTab) => void;
}

export const CompatibilityView: React.FC<CompatibilityViewProps> = ({
  profile,
  onNavigate,
}) => {
  const [selectedOs, setSelectedOs] = useState<string>('macOS 12 Monterey');
  const matrix: OsCompatibilityMatrix = evaluateOsCompatibility(selectedOs, profile);

  return (
    <div id="compatibility-page-view" className="space-y-6">
      {/* Intro Header */}
      <div className="flex flex-col md:flex-row md:items-center justify-between gap-4 p-5 rounded-xl glass-card-light dark:glass-card-dark border border-black/10 dark:border-white/10">
        <div>
          <h2 className="text-xl font-bold tracking-tight text-neutral-900 dark:text-neutral-100 flex items-center gap-2">
            <CheckCircle2 className="w-5 h-5 text-emerald-500" />
            <span>OS Compatibility &amp; Patch Audit</span>
          </h2>
          <p className="text-xs text-neutral-500 dark:text-neutral-400 mt-1">
            Evaluates hardware instructions, Metal graphics levels, Cryptex requirements, and legacy patch bundles for {profile.marketingName}.
          </p>
        </div>

        {/* Quick Simulator Link */}
        <button
          onClick={() => onNavigate('simulator')}
          className="inline-flex items-center gap-1.5 px-3 py-1.5 rounded-lg border border-black/10 dark:border-white/10 bg-black/5 dark:bg-white/5 hover:bg-black/10 text-xs font-medium text-neutral-700 dark:text-neutral-300 transition-colors self-start md:self-auto"
        >
          <Layers className="w-3.5 h-3.5 text-amber-500" />
          <span>Simulate in Sandbox</span>
        </button>
      </div>

      {/* Target OS Picker Bar */}
      <div className="space-y-2">
        <label className="text-xs font-medium text-neutral-500 dark:text-neutral-400 uppercase tracking-wider block">
          Select Target Operating System:
        </label>
        <div className="flex gap-2 overflow-x-auto pb-2 scrollbar-thin">
          {TARGET_OS_LIST.map((os) => {
            const isSelected = selectedOs === os;
            const evalResult = evaluateOsCompatibility(os, profile);
            return (
              <button
                key={os}
                onClick={() => setSelectedOs(os)}
                className={`px-3 py-2 rounded-xl text-xs font-medium whitespace-nowrap border transition-all flex items-center gap-2 ${
                  isSelected
                    ? 'bg-blue-600 text-white border-blue-600 shadow-xs font-semibold'
                    : 'glass-card-light dark:glass-card-dark border-black/5 dark:border-white/10 text-neutral-600 dark:text-neutral-300 hover:border-black/20 dark:hover:border-white/20'
                }`}
              >
                <span>{os}</span>
                <span
                  className={`w-2 h-2 rounded-full ${
                    evalResult.rating === 'NATIVELY_SUPPORTED'
                      ? isSelected ? 'bg-white' : 'bg-emerald-500'
                      : evalResult.rating === 'SUPPORTED_OCLP'
                      ? isSelected ? 'bg-amber-300' : 'bg-amber-400'
                      : 'bg-rose-500'
                  }`}
                />
              </button>
            );
          })}
        </div>
      </div>

      {/* Compatibility Verdict Banner */}
      <div
        className={`p-5 rounded-xl border flex flex-col md:flex-row md:items-center justify-between gap-4 ${
          matrix.rating === 'NATIVELY_SUPPORTED'
            ? 'bg-emerald-500/10 border-emerald-500/30'
            : matrix.rating === 'SUPPORTED_OCLP'
            ? 'bg-amber-500/10 border-amber-500/30'
            : 'bg-rose-500/10 border-rose-500/30'
        }`}
      >
        <div className="space-y-1">
          <div className="flex items-center gap-2">
            <StatusBadge
              label={matrix.ratingLabel}
              tone={
                matrix.rating === 'NATIVELY_SUPPORTED'
                  ? 'emerald'
                  : matrix.rating === 'SUPPORTED_OCLP'
                  ? 'amber'
                  : 'rose'
              }
            />
            <span className="text-xs font-mono text-neutral-500 dark:text-neutral-400">
              Version: {matrix.version}
            </span>
          </div>
          <div className="text-sm font-semibold text-neutral-900 dark:text-neutral-100">
            {matrix.rationale}
          </div>
        </div>

        <div className="shrink-0 flex items-center gap-2">
          {matrix.integrationRequired === 'OCLP' && (
            <button
              onClick={() => onNavigate('integrations')}
              className="px-3 py-1.5 rounded-lg bg-amber-600 hover:bg-amber-700 text-white text-xs font-medium transition-colors shadow-xs"
            >
              Configure OCLP Integration
            </button>
          )}
          {matrix.integrationRequired === 'rEFInd' && (
            <button
              onClick={() => onNavigate('integrations')}
              className="px-3 py-1.5 rounded-lg bg-blue-600 hover:bg-blue-700 text-white text-xs font-medium transition-colors shadow-xs"
            >
              Configure rEFInd
            </button>
          )}
        </div>
      </div>

      {/* Detailed Criteria Matrix */}
      <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
        {/* CPU Requirements */}
        <GlassPanel id="compat-cpu-card" title="CPU Capability Audit">
          <div className="space-y-3 text-xs">
            <div className="flex items-center justify-between py-1.5 border-b border-black/5 dark:border-white/5">
              <span className="text-neutral-500 dark:text-neutral-400">64-Bit Intel Architecture</span>
              <StatusBadge
                label={matrix.checks.cpu64Bit.pass ? 'PASS' : 'FAIL'}
                tone={matrix.checks.cpu64Bit.pass ? 'emerald' : 'rose'}
              />
            </div>
            <div className="flex items-center justify-between py-1.5 border-b border-black/5 dark:border-white/5">
              <span className="text-neutral-500 dark:text-neutral-400">SSE4.2 Instructions</span>
              <StatusBadge
                label={matrix.checks.sse4_2.pass ? 'PASS' : 'FAIL'}
                tone={matrix.checks.sse4_2.pass ? 'emerald' : 'rose'}
              />
            </div>
            <div className="flex items-center justify-between py-1.5 border-b border-black/5 dark:border-white/5">
              <span className="text-neutral-500 dark:text-neutral-400">AVX2 Vector Instructions</span>
              <StatusBadge
                label={matrix.checks.avx2.pass ? 'NATIVE' : 'BYPASS REQUIRED'}
                tone={matrix.checks.avx2.pass ? 'emerald' : 'amber'}
              />
            </div>
            <p className="text-[11px] text-neutral-400 font-mono">
              Note: {matrix.checks.avx2.note}
            </p>
          </div>
        </GlassPanel>

        {/* GPU & Metal Requirements */}
        <GlassPanel id="compat-gpu-card" title="GPU &amp; Metal Graphics Acceleration">
          <div className="space-y-3 text-xs">
            <div className="flex items-center justify-between py-1.5 border-b border-black/5 dark:border-white/5">
              <span className="text-neutral-500 dark:text-neutral-400">Metal Graphics Acceleration</span>
              <StatusBadge
                label={matrix.checks.gpuMetal.pass ? 'SUPPORTED' : 'LEGACY GL'}
                tone={matrix.checks.gpuMetal.pass ? 'emerald' : 'amber'}
              />
            </div>
            <div className="flex items-center justify-between py-1.5 border-b border-black/5 dark:border-white/5">
              <span className="text-neutral-500 dark:text-neutral-400">Legacy GPU Driver Root Patch</span>
              <StatusBadge
                label={matrix.checks.legacyGpuPatches.required ? 'REQUIRED (OCLP)' : 'NOT NEEDED'}
                tone={matrix.checks.legacyGpuPatches.required ? 'amber' : 'emerald'}
              />
            </div>
            <div className="flex items-center justify-between py-1.5 border-b border-black/5 dark:border-white/5">
              <span className="text-neutral-500 dark:text-neutral-400">Cryptex OS.dmg Bypass</span>
              <StatusBadge
                label={matrix.checks.cryptexBypass.required ? 'REQUIRED' : 'STANDARD'}
                tone={matrix.checks.cryptexBypass.required ? 'amber' : 'neutral'}
              />
            </div>
            <p className="text-[11px] text-neutral-400 font-mono">
              Note: {matrix.checks.legacyGpuPatches.note}
            </p>
          </div>
        </GlassPanel>
      </div>

      {/* Deployment & Preboot Recommendations */}
      <GlassPanel id="compat-recommendation-card" title="OpenVintage Deployment Recommendation">
        <div className="p-4 rounded-xl bg-black/5 dark:bg-white/5 space-y-3 text-xs">
          <div className="flex items-center gap-2 font-semibold text-neutral-900 dark:text-neutral-100">
            <ShieldCheck className="w-4 h-4 text-blue-500" />
            <span>Recommended Preboot Strategy for {selectedOs}</span>
          </div>
          <p className="text-neutral-600 dark:text-neutral-300 leading-relaxed">
            {matrix.integrationRequired === 'OCLP'
              ? 'Deploy OpenVintage EFI as the primary boot manager. OpenVintage will chainload OpenCore Legacy Patcher (OCLP) with customized GMUX GPU switching hooks, bypassing non-AVX2 Cryptex requirements while preserving clean multi-boot options.'
              : matrix.integrationRequired === 'rEFInd'
              ? 'Deploy OpenVintage alongside rEFInd. This configuration gives you a high-resolution boot picker with direct EFI stub launching for modern Linux distributions and Windows.'
              : 'Direct native boot supported! No kernel extensions or binary patches are required. OpenVintage manages GPU selection and performance governors natively.'}
          </p>
          <div className="pt-2 flex items-center gap-3">
            <button
              onClick={() => onNavigate('boot')}
              className="text-xs text-blue-600 dark:text-blue-400 hover:underline flex items-center gap-1 font-medium"
            >
              <span>View Discovered Boot Entries</span>
              <ExternalLink className="w-3 h-3" />
            </button>
          </div>
        </div>
      </GlassPanel>
    </div>
  );
};
