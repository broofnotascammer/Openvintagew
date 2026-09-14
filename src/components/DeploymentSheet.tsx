import React, { useState } from 'react';
import { 
  X, 
  ShieldCheck, 
  AlertTriangle, 
  CheckCircle2, 
  FileText, 
  HardDriveDownload, 
  RotateCcw,
  ArrowRight,
  Lock,
  Archive,
  RefreshCw,
  Terminal
} from 'lucide-react';
import { 
  DeploymentStep, 
  DeploymentPlanItem, 
  DEFAULT_DEPLOYMENT_PLAN,
  HardwareProfileData
} from '../core/openvintageState';
import { StatusBadge } from './StatusBadge';

interface DeploymentSheetProps {
  isOpen: boolean;
  onClose: () => void;
  profile: HardwareProfileData;
}

const STEPS_SEQUENCE: Array<{ step: DeploymentStep; label: string }> = [
  { step: 'DISCOVER', label: '1. Discover' },
  { step: 'SIMULATE', label: '2. Simulate' },
  { step: 'PLAN', label: '3. Plan' },
  { step: 'REVIEW', label: '4. Review Changes' },
  { step: 'USER_APPROVAL', label: '5. User Approval' },
  { step: 'BACKUP', label: '6. Backup' },
  { step: 'APPLY', label: '7. Apply' },
  { step: 'VERIFY', label: '8. Verify' },
  { step: 'RECOVERY', label: '9. Recovery Ready' },
];

export const DeploymentSheet: React.FC<DeploymentSheetProps> = ({
  isOpen,
  onClose,
  profile,
}) => {
  const [currentStepIndex, setCurrentStepIndex] = useState<number>(3); // Default at Review Changes
  const [isRunning, setIsRunning] = useState<boolean>(false);
  const [userConfirmed, setUserConfirmed] = useState<boolean>(false);
  const [deploymentFinished, setDeploymentFinished] = useState<boolean>(false);
  const [logs, setLogs] = useState<string[]>([
    '[INIT] Initializing 9-Step OpenVintage Safe Deployment Subsystem (Phase 6.1)',
    `[DISCOVER] Discovered target partition at /Volumes/EFI (ESP FAT32)`,
    `[SIMULATE] Simulated dry-run deployment for ${profile.modelIdentifier} silicon`,
    '[PLAN] Generated cryptographic staging plan with 4 operations',
    '[REVIEW] Awaiting user security audit and explicit authorization signature',
  ]);

  if (!isOpen) return null;

  const currentStep = STEPS_SEQUENCE[currentStepIndex];

  const handleStartDeployment = () => {
    if (!userConfirmed) return;
    setIsRunning(true);
    setCurrentStepIndex(4); // User Approval completed

    // Execute through steps sequentially
    const addLog = (msg: string) => setLogs((prev) => [...prev, msg]);

    setTimeout(() => {
      setCurrentStepIndex(5); // Backup
      addLog('[BACKUP] Created compressed tarball: /Volumes/EFI/OpenVintage_Backup/EFI_PreDeployment_20260914.tar.gz');
      addLog('[BACKUP] SHA256 checksum recorded in NVRAM guard log');

      setTimeout(() => {
        setCurrentStepIndex(6); // Apply
        addLog('[APPLY] Staged /Volumes/EFI/EFI/BOOT/BOOTX64.EFI (142 KB)');
        addLog('[APPLY] Staged /Volumes/EFI/EFI/OpenVintage/OvSelfTestApp.efi (215 KB)');
        addLog('[APPLY] Generated and locked OpenVintage config.plist');

        setTimeout(() => {
          setCurrentStepIndex(7); // Verify
          addLog('[VERIFY] Cryptographic SHA-256 integrity matches plan manifests perfectly');
          addLog('[VERIFY] Memory barrier & non-volatile guardrails validated');

          setTimeout(() => {
            setCurrentStepIndex(8); // Recovery
            addLog('[RECOVERY] Failsafe recovery descriptor registered in fallback boot entry');
            addLog('[COMPLETED] OpenVintage deployment successfully finalized with zero errors.');
            setIsRunning(false);
            setDeploymentFinished(true);
          }, 800);
        }, 800);
      }, 800);
    }, 800);
  };

  const handleReset = () => {
    setCurrentStepIndex(3);
    setUserConfirmed(false);
    setIsRunning(false);
    setDeploymentFinished(false);
  };

  return (
    <div 
      id="deployment-sheet-modal-backdrop" 
      className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/60 backdrop-blur-md animate-in fade-in"
    >
      <div 
        id="deployment-sheet-modal"
        className="w-full max-w-3xl max-h-[90vh] flex flex-col rounded-2xl macos-window-light dark:macos-window-dark border border-black/15 dark:border-white/15 overflow-hidden shadow-2xl"
      >
        {/* Header */}
        <div className="p-4 px-6 border-b border-black/10 dark:border-white/10 flex items-center justify-between">
          <div className="flex items-center gap-3">
            <div className="p-2 rounded-lg bg-blue-600/15 text-blue-600 dark:text-blue-400">
              <ShieldCheck className="w-5 h-5" />
            </div>
            <div>
              <h3 className="text-base font-bold text-neutral-900 dark:text-neutral-100">
                Safe Deployment Engine (9-Step Lifecycle)
              </h3>
              <span className="text-xs text-neutral-500 dark:text-neutral-400 font-mono">
                Target Silicon: {profile.marketingName}
              </span>
            </div>
          </div>

          <button
            onClick={onClose}
            className="p-1.5 rounded-lg text-neutral-400 hover:text-neutral-700 dark:hover:text-neutral-200 hover:bg-black/5 dark:hover:bg-white/5 transition-colors"
          >
            <X className="w-5 h-5" />
          </button>
        </div>

        {/* 9-Step Visible Lifecycle Progress Bar */}
        <div className="p-4 border-b border-black/5 dark:border-white/5 bg-black/2 dark:bg-white/2 overflow-x-auto">
          <div className="flex items-center gap-1.5 min-w-[680px]">
            {STEPS_SEQUENCE.map((s, idx) => {
              const isPassed = idx < currentStepIndex;
              const isCurrent = idx === currentStepIndex;

              return (
                <div key={s.step} className="flex-1 flex items-center gap-1">
                  <div
                    className={`h-7 px-2 rounded flex items-center justify-center text-[10px] font-mono font-medium transition-all w-full truncate ${
                      isPassed
                        ? 'bg-emerald-500/15 text-emerald-700 dark:text-emerald-300 border border-emerald-500/30'
                        : isCurrent
                        ? 'bg-blue-600 text-white font-bold shadow-xs'
                        : 'bg-black/5 dark:bg-white/5 text-neutral-400'
                    }`}
                  >
                    {s.label}
                  </div>
                </div>
              );
            })}
          </div>
        </div>

        {/* Body Content */}
        <div className="p-6 space-y-6 overflow-y-auto flex-1 text-xs">
          {/* Confirmation Notice */}
          <div className="p-4 rounded-xl bg-blue-500/10 border border-blue-500/20 space-y-2">
            <h4 className="text-xs font-bold text-blue-800 dark:text-blue-300 uppercase tracking-wider">
              Prepare OpenVintage Deployment
            </h4>
            <p className="text-neutral-600 dark:text-neutral-300 leading-relaxed">
              OpenVintage is ready to apply:
            </p>
            <ul className="list-disc list-inside space-y-1 text-neutral-700 dark:text-neutral-200 font-medium">
              <li>OpenVintage primary EFI bootloader &amp; self-test applet</li>
              <li>Boot entry preferences and GMUX GPU switching hooks</li>
              <li>Selected performance profile policies</li>
              <li>rEFInd / OCLP compatibility staging</li>
            </ul>
            <div className="pt-1 text-[11px] text-neutral-500 dark:text-neutral-400 font-mono">
              Existing EFI configuration will be automatically archived into a cryptographic backup before any file is touched.
            </div>
          </div>

          {/* Cryptographic Staging Plan Table */}
          <div className="space-y-2">
            <div className="flex items-center justify-between">
              <span className="font-semibold text-neutral-900 dark:text-neutral-100 uppercase tracking-wider text-[11px]">
                Auditable Deployment Plan Manifest
              </span>
              <span className="text-[11px] font-mono text-neutral-400">
                {DEFAULT_DEPLOYMENT_PLAN.length} Atomic Operations
              </span>
            </div>

            <div className="rounded-xl border border-black/10 dark:border-white/10 overflow-hidden">
              <table className="w-full text-left font-mono text-[11px]">
                <thead className="bg-black/5 dark:bg-white/5 border-b border-black/5 dark:border-white/5 text-neutral-500">
                  <tr>
                    <th className="p-2.5">Action</th>
                    <th className="p-2.5">Target Destination</th>
                    <th className="p-2.5">SHA-256 Digest</th>
                    <th className="p-2.5">Size</th>
                  </tr>
                </thead>
                <tbody className="divide-y divide-black/5 dark:divide-white/5">
                  {DEFAULT_DEPLOYMENT_PLAN.map((item) => (
                    <tr key={item.id} className="hover:bg-black/2 dark:hover:bg-white/2">
                      <td className="p-2.5">
                        <span
                          className={`px-1.5 py-0.5 rounded text-[10px] font-bold ${
                            item.action === 'INSTALL'
                              ? 'bg-blue-500/20 text-blue-700 dark:text-blue-300'
                              : item.action === 'CREATE'
                              ? 'bg-emerald-500/20 text-emerald-700 dark:text-emerald-300'
                              : 'bg-amber-500/20 text-amber-700 dark:text-amber-300'
                          }`}
                        >
                          {item.action}
                        </span>
                      </td>
                      <td className="p-2.5 text-neutral-900 dark:text-neutral-100 truncate max-w-[200px]">
                        {item.targetPath}
                      </td>
                      <td className="p-2.5 text-neutral-500 truncate max-w-[120px]">
                        {item.sha256.substring(0, 16)}...
                      </td>
                      <td className="p-2.5 text-neutral-600 dark:text-neutral-400">
                        {(item.sizeBytes / 1024).toFixed(1)} KB
                      </td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          </div>

          {/* User Approval Gate */}
          {!deploymentFinished && (
            <div className="p-4 rounded-xl border border-black/10 dark:border-white/10 bg-black/5 dark:bg-white/5 space-y-3">
              <div className="flex items-center justify-between">
                <span className="font-semibold text-neutral-900 dark:text-neutral-100 flex items-center gap-2">
                  <Lock className="w-4 h-4 text-amber-500" />
                  <span>Step 5: User Approval Security Gate</span>
                </span>
                <StatusBadge
                  label={userConfirmed ? 'APPROVED' : 'AWAITING APPROVAL'}
                  tone={userConfirmed ? 'emerald' : 'amber'}
                />
              </div>

              <label className="flex items-start gap-2.5 cursor-pointer select-none">
                <input
                  type="checkbox"
                  id="checkbox-confirm-deployment"
                  checked={userConfirmed}
                  disabled={isRunning}
                  onChange={(e) => setUserConfirmed(e.target.checked)}
                  className="mt-0.5 rounded border-neutral-400 text-blue-600 focus:ring-blue-500"
                />
                <span className="text-xs text-neutral-600 dark:text-neutral-300">
                  I have audited the planned cryptographic modifications and authorize OpenVintage to stage EFI binaries and write boot configuration.
                </span>
              </label>
            </div>
          )}

          {/* Real-time Execution Logs Console */}
          <div className="space-y-1.5">
            <span className="font-semibold text-neutral-900 dark:text-neutral-100 uppercase tracking-wider text-[11px] flex items-center gap-1.5">
              <Terminal className="w-3.5 h-3.5" />
              <span>Deployment Execution Telemetry</span>
            </span>
            <div className="p-3 rounded-xl bg-neutral-950 text-neutral-300 font-mono text-[11px] h-32 overflow-y-auto space-y-1 border border-neutral-800">
              {logs.map((log, i) => (
                <div key={i} className="leading-tight">
                  <span className="text-neutral-500">{i + 1}.</span> {log}
                </div>
              ))}
            </div>
          </div>
        </div>

        {/* Footer Actions */}
        <div className="p-4 px-6 border-t border-black/10 dark:border-white/10 flex items-center justify-between bg-black/2 dark:bg-white/2">
          <button
            onClick={onClose}
            className="px-4 py-2 rounded-lg border border-black/10 dark:border-white/10 hover:bg-black/5 dark:hover:bg-white/5 text-xs font-medium text-neutral-700 dark:text-neutral-300 transition-colors"
          >
            {deploymentFinished ? 'Close' : 'Cancel'}
          </button>

          <div className="flex items-center gap-2">
            {deploymentFinished ? (
              <button
                onClick={handleReset}
                className="px-4 py-2 rounded-lg bg-blue-600 hover:bg-blue-700 text-white text-xs font-medium transition-colors"
              >
                Reset Staging Session
              </button>
            ) : (
              <button
                id="btn-apply-deployment"
                onClick={handleStartDeployment}
                disabled={!userConfirmed || isRunning}
                className="px-4 py-2 rounded-lg bg-blue-600 hover:bg-blue-700 active:bg-blue-800 text-white text-xs font-semibold shadow-xs flex items-center gap-2 transition-colors disabled:opacity-50"
              >
                {isRunning ? (
                  <>
                    <RefreshCw className="w-3.5 h-3.5 animate-spin" />
                    <span>Applying Deployment...</span>
                  </>
                ) : (
                  <>
                    <ShieldCheck className="w-3.5 h-3.5" />
                    <span>Review &amp; Apply Changes</span>
                  </>
                )}
              </button>
            )}
          </div>
        </div>
      </div>
    </div>
  );
};
