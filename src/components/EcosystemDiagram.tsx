import React, { useState } from 'react';
import { Layers, GitBranch, Cpu, Monitor, Zap, Archive, ArrowRight, ShieldCheck, CheckCircle2 } from 'lucide-react';

export const EcosystemDiagram: React.FC = () => {
  const [selectedNode, setSelectedNode] = useState<string>('resolver');

  const nodeDetails: Record<string, { title: string; subtitle: string; description: string; items: string[] }> = {
    openvintage: {
      title: 'OpenVintage Root Layer',
      subtitle: 'Master Ecosystem Orchestrator',
      description:
        'The overarching system platform binding hardware enumeration, execution routing, intermediate compilation, and resource scheduling.',
      items: [
        'Phase 5 Complete: Full architectural integration & optimization',
        '32 / 32 automated self-tests passing cleanly under UEFI/QEMU',
        'Unified Bootloader, HAL DXE, OVIR-GPU, OVIR-CPU, and Scheduler',
      ],
    },
    ovcore: {
      title: 'OVCore Subsystem',
      subtitle: 'Foundational Platform Primitives & Caching',
      description:
        'Provides cross-platform memory allocators, high-resolution timers, multi-tier unified caching, empirical benchmarking, and system diagnostics.',
      items: [
        'OvUnifiedCache: Coordinated CPU, shader, pipeline, and compatibility caches',
        'OvBenchmark: Real TSC cycle measurements verifying compiler speedup',
        'OvDiagnostics: Complete hardware, memory, API, and quirk telemetry',
      ],
    },
    resolver: {
      title: 'OVResolver (Integrated Decision Engine)',
      subtitle: 'Dynamic Multi-Dimensional Workload Arbiter',
      description:
        'Coordinates CPU architecture, GPU architecture, API requirements, thermal state, RAM/VRAM constraints, and cache hits into a unified execution plan.',
      items: [
        'Synthesizes CPU translation necessity (ARM64 <-> x86-64)',
        'Resolves GPU backend paths (Native, Translated, Simplified, Fallback)',
        'Clamps texture sizes & handles compute shader fallbacks gracefully',
      ],
    },
    scheduler: {
      title: 'OVScheduler & Resource Manager',
      subtitle: 'Dynamic Resource & Thermal Coordinator',
      description:
        'Manages CPU core topologies, thread pools, and enforces platform performance profiles (Balanced, Performance, MaxPerformance, BatteryLowPower).',
      items: [
        'Strict hardware capability gating prevents invalid profiles',
        'Cooperative with host OS scheduler (zero permanent core pinning)',
        'Directs JIT compilation and shader tasks to background workers',
      ],
    },
    ovirGpu: {
      title: 'OVIR-GPU (Graphics Intermediate Representation)',
      subtitle: 'Universal Graphics Hub & Spoke Representation',
      description:
        'Eliminates M x N translation matrices by abstracting draw commands, render passes, pipeline states, and textures into an immutable IR directed acyclic graph (DAG).',
      items: [
        'Adapters: Metal 2/3, Vulkan, OpenGL Core, DirectX 11/12',
        'SPIR-V bytecode ingestion with FNV-1a hashing & bytecode cache',
        'Deterministic Pipeline State Object (PSO) caching',
      ],
    },
    ovirCpu: {
      title: 'OVIR-CPU (CPU Instruction IR & JIT Pipeline)',
      subtitle: 'Architecture-Neutral Binary Translation',
      description:
        'Decoupled CPU translation pipeline lowering guest machine instructions into canonical 3-address IR, optimizing safely, and emitting target machine code.',
      items: [
        'Isolated ARM64 and x86-64 multi-architecture decoders',
        'Safe optimizer: Constant folding, redundant move & dead code elimination',
        'Dynamic JIT execution with generational translation caching',
      ],
    },
  };

  const active = nodeDetails[selectedNode] || nodeDetails.resolver;

  return (
    <div id="ecosystem-diagram-container" className="bg-neutral-900/60 border border-neutral-800 rounded-xl p-6">
      <div className="flex flex-col lg:flex-row gap-6">
        {/* Visual Architecture Tree */}
        <div className="flex-1 bg-neutral-950 border border-neutral-800/80 rounded-lg p-5 font-mono text-xs overflow-x-auto">
          <div className="text-neutral-400 font-semibold mb-4 text-center tracking-wider text-sm flex items-center justify-center space-x-2">
            <Layers className="w-4 h-4 text-amber-400" />
            <span>OPENVINTAGE ECOSYSTEM ARCHITECTURE</span>
          </div>

          <div className="flex flex-col items-center space-y-4 min-w-[580px]">
            {/* Master Node */}
            <button
              onClick={() => setSelectedNode('openvintage')}
              className={`px-6 py-2.5 rounded border transition-all font-bold tracking-wider ${
                selectedNode === 'openvintage'
                  ? 'bg-amber-500/20 border-amber-400 text-amber-300 ring-2 ring-amber-400/20'
                  : 'bg-neutral-900 border-neutral-700 text-neutral-200 hover:border-neutral-500'
              }`}
            >
              OpenVintage
            </button>

            {/* Down Connector */}
            <div className="w-0.5 h-4 bg-neutral-700"></div>

            {/* Level 1: Core, Resolver, Scheduler */}
            <div className="w-full flex justify-between items-center px-6 relative">
              <div className="absolute top-0 left-12 right-12 h-0.5 bg-neutral-700"></div>

              <div className="flex flex-col items-center z-10">
                <div className="w-0.5 h-3 bg-neutral-700"></div>
                <button
                  onClick={() => setSelectedNode('ovcore')}
                  className={`px-4 py-2 rounded border transition-all ${
                    selectedNode === 'ovcore'
                      ? 'bg-amber-500/20 border-amber-400 text-amber-300 ring-1 ring-amber-400/30'
                      : 'bg-neutral-900 border-neutral-700 text-neutral-300 hover:border-neutral-500'
                  }`}
                >
                  OVCore
                </button>
              </div>

              <div className="flex flex-col items-center z-10">
                <div className="w-0.5 h-3 bg-neutral-700"></div>
                <button
                  onClick={() => setSelectedNode('resolver')}
                  className={`px-4 py-2 rounded border transition-all ${
                    selectedNode === 'resolver'
                      ? 'bg-amber-500/20 border-amber-400 text-amber-300 ring-1 ring-amber-400/30'
                      : 'bg-neutral-900 border-neutral-700 text-neutral-300 hover:border-neutral-500'
                  }`}
                >
                  OVResolver
                </button>
              </div>

              <div className="flex flex-col items-center z-10">
                <div className="w-0.5 h-3 bg-neutral-700"></div>
                <button
                  onClick={() => setSelectedNode('scheduler')}
                  className={`px-4 py-2 rounded border transition-all ${
                    selectedNode === 'scheduler'
                      ? 'bg-amber-500/20 border-amber-400 text-amber-300 ring-1 ring-amber-400/30'
                      : 'bg-neutral-900 border-neutral-700 text-neutral-300 hover:border-neutral-500'
                  }`}
                >
                  OVScheduler
                </button>
              </div>
            </div>

            {/* Connectors to Level 2 */}
            <div className="w-full flex justify-between px-2 pt-2">
              {/* Left Wing: OVCore -> IRs */}
              <div className="w-1/3 flex flex-col items-center">
                <div className="w-0.5 h-3 bg-neutral-700"></div>
                <div className="w-full flex justify-around relative">
                  <div className="absolute top-0 left-6 right-6 h-0.5 bg-neutral-700"></div>

                  <div className="flex flex-col items-center z-10">
                    <div className="w-0.5 h-2 bg-neutral-700"></div>
                    <button
                      onClick={() => setSelectedNode('ovirGpu')}
                      className={`px-3 py-1.5 rounded border transition-all ${
                        selectedNode === 'ovirGpu'
                          ? 'bg-sky-500/20 border-sky-400 text-sky-300 ring-1 ring-sky-400/30'
                          : 'bg-neutral-900 border-neutral-700 text-neutral-300 hover:border-neutral-500'
                      }`}
                    >
                      OVIR-GPU
                    </button>
                    <div className="w-0.5 h-2 bg-neutral-700"></div>
                    <div className="text-[10px] text-neutral-500 text-center leading-tight bg-neutral-950 px-2 py-1 rounded border border-neutral-800">
                      Metal · Vulkan<br />OpenGL · DirectX
                    </div>
                  </div>

                  <div className="flex flex-col items-center z-10">
                    <div className="w-0.5 h-2 bg-neutral-700"></div>
                    <button
                      onClick={() => setSelectedNode('ovirCpu')}
                      className={`px-3 py-1.5 rounded border transition-all ${
                        selectedNode === 'ovirCpu'
                          ? 'bg-purple-500/20 border-purple-400 text-purple-300 ring-1 ring-purple-400/30'
                          : 'bg-neutral-900 border-neutral-700 text-neutral-300 hover:border-neutral-500'
                      }`}
                    >
                      OVIR-CPU
                    </button>
                    <div className="w-0.5 h-2 bg-neutral-700"></div>
                    <div className="text-[10px] text-neutral-500 text-center leading-tight bg-neutral-950 px-2 py-1 rounded border border-neutral-800">
                      ARM · x86-64<br />(Future RISC-V)
                    </div>
                  </div>
                </div>
              </div>

              {/* Center: Resolver decision link */}
              <div className="w-1/3 flex flex-col items-center justify-center">
                <div className="text-[10px] text-amber-400/80 bg-neutral-900/80 px-2.5 py-1.5 rounded border border-amber-900/40 text-center">
                  Path Arbitrament &amp;<br />Heuristic Matcher
                </div>
              </div>

              {/* Right Wing: Scheduler -> Resources */}
              <div className="w-1/3 flex flex-col items-center">
                <div className="w-0.5 h-3 bg-neutral-700"></div>
                <div className="w-full flex justify-around relative">
                  <div className="absolute top-0 left-6 right-6 h-0.5 bg-neutral-700"></div>

                  <div className="flex flex-col items-center z-10">
                    <div className="w-0.5 h-2 bg-neutral-700"></div>
                    <div className="px-2.5 py-1.5 bg-neutral-900 border border-neutral-800 text-neutral-400 rounded text-center text-[10px]">
                      CPU Cores &amp;<br />Topology
                    </div>
                  </div>

                  <div className="flex flex-col items-center z-10">
                    <div className="w-0.5 h-2 bg-neutral-700"></div>
                    <div className="px-2.5 py-1.5 bg-neutral-900 border border-neutral-800 text-neutral-400 rounded text-center text-[10px]">
                      Memory &amp;<br />Thermals
                    </div>
                  </div>
                </div>
              </div>
            </div>

            {/* Bottom Target Backends */}
            <div className="pt-3 w-full border-t border-neutral-800/80 flex justify-between text-[11px] text-neutral-400 px-4">
              <span className="flex items-center text-emerald-400">
                <CheckCircle2 className="w-3.5 h-3.5 mr-1" />
                Target: Legacy Intel Mac Hardware (2006-2015)
              </span>
              <span className="text-neutral-500">
                Future: Portable Linux, Windows, &amp; SolitaryOS
              </span>
            </div>
          </div>
        </div>

        {/* Detailed Inspector Panel */}
        <div className="lg:w-80 flex flex-col justify-between bg-neutral-950 border border-neutral-800 rounded-lg p-5">
          <div>
            <div className="flex items-center justify-between pb-3 border-b border-neutral-800">
              <span className="text-xs uppercase font-mono tracking-wider text-neutral-400">Subsystem Details</span>
              <span className="text-xs px-2 py-0.5 rounded bg-neutral-900 text-amber-400 font-mono border border-neutral-800">
                ACTIVE SPEC
              </span>
            </div>

            <div className="mt-4">
              <h2 className="text-base font-bold text-white font-mono">{active.title}</h2>
              <p className="text-xs text-amber-400/90 font-medium mb-2">{active.subtitle}</p>
              <p className="text-xs text-neutral-300 leading-relaxed mb-4">{active.description}</p>
            </div>

            <div className="space-y-2">
              <div className="text-[11px] font-mono text-neutral-400 uppercase tracking-wider">Key Directives</div>
              <ul className="space-y-1.5 text-xs text-neutral-300">
                {active.items.map((item, idx) => (
                  <li key={idx} className="flex items-start">
                    <span className="text-amber-400 mr-2">›</span>
                    <span>{item}</span>
                  </li>
                ))}
              </ul>
            </div>
          </div>

          <div className="mt-6 pt-4 border-t border-neutral-900 text-[11px] text-neutral-500">
            Click any architecture node to inspect interface contracts and design rules.
          </div>
        </div>
      </div>
    </div>
  );
};
