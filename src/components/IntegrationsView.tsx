import React from 'react';
import { 
  Boxes, 
  CheckCircle2, 
  AlertTriangle, 
  Layers, 
  ShieldCheck, 
  ArrowRight,
  HardDriveDownload,
  Info,
  Sparkles
} from 'lucide-react';
import { IntegrationStatus, HardwareProfileData } from '../core/openvintageState';
import { GlassPanel } from './GlassPanel';
import { StatusBadge } from './StatusBadge';

interface IntegrationsViewProps {
  integrations: IntegrationStatus[];
  profile: HardwareProfileData;
  onOpenDeployment: () => void;
}

export const IntegrationsView: React.FC<IntegrationsViewProps> = ({
  integrations,
  profile,
  onOpenDeployment,
}) => {
  return (
    <div id="integrations-page-view" className="space-y-6">
      {/* Intro Header */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4 p-5 rounded-xl glass-card-light dark:glass-card-dark border border-black/10 dark:border-white/10">
        <div>
          <h2 className="text-xl font-bold tracking-tight text-neutral-900 dark:text-neutral-100 flex items-center gap-2">
            <Boxes className="w-5 h-5 text-violet-500" />
            <span>Subsystem &amp; Ecosystem Integrations</span>
          </h2>
          <p className="text-xs text-neutral-500 dark:text-neutral-400 mt-0.5">
            Orchestration hooks for OpenVintage Boot Manager, OpenCore Legacy Patcher (OCLP), and rEFInd.
          </p>
        </div>

        <button
          onClick={onOpenDeployment}
          className="inline-flex items-center gap-1.5 px-3 py-1.5 rounded-lg bg-blue-600 hover:bg-blue-700 text-white text-xs font-medium transition-colors shadow-xs self-start sm:self-auto"
        >
          <HardDriveDownload className="w-3.5 h-3.5" />
          <span>Stage Ecosystem in EFI</span>
        </button>
      </div>

      {/* Integrations List */}
      <div className="space-y-4">
        {integrations.map((item) => {
          return (
            <GlassPanel
              key={item.id}
              id={`integration-card-${item.id}`}
              className="p-5"
            >
              <div className="flex flex-col lg:flex-row lg:items-center justify-between gap-4">
                <div className="space-y-2 max-w-3xl">
                  <div className="flex items-center gap-2.5 flex-wrap">
                    <h3 className="text-base font-bold text-neutral-900 dark:text-neutral-100">
                      {item.name}
                    </h3>
                    <span className="text-xs font-mono text-neutral-400">
                      {item.version}
                    </span>
                    <StatusBadge
                      label={item.statusLabel}
                      tone={
                        item.status === 'INSTALLED'
                          ? 'emerald'
                          : item.status === 'RECOMMENDED'
                          ? 'amber'
                          : 'sky'
                      }
                      dot={true}
                    />
                  </div>

                  <p className="text-xs text-neutral-600 dark:text-neutral-300 leading-relaxed">
                    {item.description}
                  </p>

                  <div className="p-3 rounded-lg bg-black/5 dark:bg-white/5 space-y-1 text-xs">
                    <div className="font-semibold text-neutral-800 dark:text-neutral-200">
                      Hardware Rationale for {profile.marketingName}:
                    </div>
                    <p className="text-neutral-500 dark:text-neutral-400 text-[11px] leading-relaxed">
                      {item.rationale}
                    </p>
                  </div>

                  <div className="text-[11px] font-mono text-neutral-400">
                    EFI Staging Payload: <span className="text-neutral-700 dark:text-neutral-300">{item.efiPayloadPath}</span>
                  </div>
                </div>

                {/* Integration Action */}
                <div className="shrink-0 flex flex-col sm:flex-row lg:flex-col gap-2">
                  <button
                    id={`btn-deploy-integration-${item.id}`}
                    onClick={onOpenDeployment}
                    className="px-3.5 py-2 rounded-lg bg-blue-600 hover:bg-blue-700 text-white text-xs font-medium transition-colors shadow-xs flex items-center justify-center gap-1.5"
                  >
                    <span>{item.actionLabel}</span>
                    <ArrowRight className="w-3 h-3" />
                  </button>
                </div>
              </div>
            </GlassPanel>
          );
        })}
      </div>
    </div>
  );
};
