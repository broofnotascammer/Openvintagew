/**
 * OpenVintage - Systems Engineering Console
 * Lead Software Engineer & Systems Implementation AI
 */

import React, { useState } from 'react';
import { Header } from './components/Header';
import { OverviewView } from './components/OverviewView';
import { ResolverView } from './components/ResolverView';
import { SchedulerView } from './components/SchedulerView';
import { HardwareView } from './components/HardwareView';
import { Phase1View } from './components/Phase1View';
import { DocViewer } from './components/DocViewer';
import { PerformanceProfile } from './types';
import {
  PROJECT_STATUS_TEXT,
  ARCHITECTURE_TEXT,
  README_TEXT,
  CHANGELOG_TEXT,
} from './data/docsContent';

export default function App() {
  const [activeTab, setActiveTab] = useState<string>('overview');
  const [currentProfile, setCurrentProfile] = useState<PerformanceProfile>('balanced');

  return (
    <div className="min-h-screen bg-neutral-950 text-neutral-100 flex flex-col font-sans selection:bg-amber-500/30 selection:text-amber-200">
      <Header
        activeTab={activeTab}
        setActiveTab={setActiveTab}
        currentProfile={currentProfile}
        onProfileChange={setCurrentProfile}
      />

      <main className="flex-1 max-w-7xl w-full mx-auto px-4 sm:px-6 lg:px-8 py-6">
        {activeTab === 'overview' && <OverviewView onNavigateTab={setActiveTab} />}
        {activeTab === 'resolver' && <ResolverView />}
        {activeTab === 'scheduler' && (
          <SchedulerView
            currentProfile={currentProfile}
            onProfileChange={setCurrentProfile}
          />
        )}
        {activeTab === 'hardware' && <HardwareView />}
        {activeTab === 'phase1' && <Phase1View />}
        {activeTab === 'docs' && (
          <DocViewer
            statusContent={PROJECT_STATUS_TEXT}
            archContent={ARCHITECTURE_TEXT}
            readmeContent={README_TEXT}
            changelogContent={CHANGELOG_TEXT}
          />
        )}
      </main>

      <footer className="border-t border-neutral-900 py-4 bg-neutral-950/80 text-xs text-neutral-500">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 flex flex-col sm:flex-row justify-between items-center gap-2 font-mono">
          <div>
            OpenVintage Platform Ecosystem &middot; Systems Implementation AI &middot; Revision 2.0
          </div>
          <div className="flex items-center space-x-3 text-neutral-400">
            <span>Phase 1: Verified (EDK II/QEMU)</span>
            <span>&middot;</span>
            <span className="text-amber-400">Phase 2: Active</span>
          </div>
        </div>
      </footer>
    </div>
  );
}
