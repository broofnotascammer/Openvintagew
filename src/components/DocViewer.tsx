import React, { useState } from 'react';
import { FileText, BookOpen, CheckCircle, ListFilter } from 'lucide-react';

interface DocViewerProps {
  statusContent: string;
  archContent: string;
  readmeContent: string;
  changelogContent: string;
}

export const DocViewer: React.FC<DocViewerProps> = ({
  statusContent,
  archContent,
  readmeContent,
  changelogContent,
}) => {
  const [selectedDoc, setSelectedDoc] = useState<'status' | 'arch' | 'readme' | 'changelog'>('status');

  const docs = [
    { id: 'status' as const, label: 'PROJECT_STATUS.md', title: 'Live Project Status & Milestones' },
    { id: 'arch' as const, label: 'ARCHITECTURE.md', title: 'OpenVintage Architectural Specification' },
    { id: 'readme' as const, label: 'README.md', title: 'Master Project Overview' },
    { id: 'changelog' as const, label: 'CHANGELOG.md', title: 'Versioned Change History' },
  ];

  const getContent = () => {
    switch (selectedDoc) {
      case 'status':
        return statusContent;
      case 'arch':
        return archContent;
      case 'readme':
        return readmeContent;
      case 'changelog':
        return changelogContent;
    }
  };

  return (
    <div id="project-docs-view" className="space-y-6">
      <div className="bg-neutral-900/60 border border-neutral-800 rounded-xl p-6">
        <div className="flex flex-col md:flex-row md:items-center justify-between gap-4">
          <div>
            <h2 className="text-lg font-bold text-white font-mono flex items-center">
              <BookOpen className="w-5 h-5 mr-2 text-amber-400" />
              Project Documentation &amp; Tracking
            </h2>
            <p className="text-xs text-neutral-400 mt-1">
              Synchronized repository documentation keeping engineering milestones, architectural blueprints, and phase statuses transparent.
            </p>
          </div>
          <div className="flex space-x-1 bg-neutral-950 p-1 rounded-lg border border-neutral-800">
            {docs.map((d) => (
              <button
                key={d.id}
                onClick={() => setSelectedDoc(d.id)}
                className={`px-3 py-1.5 rounded text-xs font-mono transition-colors ${
                  selectedDoc === d.id
                    ? 'bg-amber-500/20 text-amber-300 font-bold border border-amber-500/40'
                    : 'text-neutral-400 hover:text-neutral-200'
                }`}
              >
                {d.label}
              </button>
            ))}
          </div>
        </div>
      </div>

      {/* Document Content View */}
      <div className="bg-neutral-950 border border-neutral-800 rounded-xl p-6">
        <div className="flex items-center justify-between pb-3 border-b border-neutral-800 mb-4 font-mono text-xs text-neutral-400">
          <div className="flex items-center space-x-2">
            <FileText className="w-4 h-4 text-amber-400" />
            <span className="text-white font-semibold">{docs.find((d) => d.id === selectedDoc)?.title}</span>
          </div>
          <span className="text-neutral-500">FORMAT: MARKDOWN</span>
        </div>

        <pre className="text-xs font-mono text-neutral-300 bg-neutral-900/50 p-4 rounded-lg border border-neutral-850 overflow-x-auto whitespace-pre-wrap leading-relaxed">
          {getContent()}
        </pre>
      </div>
    </div>
  );
};
