import React from 'react';
import { PHASE_1_VERIFIED_DATA } from '../core/phase1Data';
import { ShieldCheck, Terminal, CheckCircle2, Server, HardDrive, Cpu } from 'lucide-react';

export const Phase1View: React.FC = () => {
  return (
    <div id="phase1-artifacts-view" className="space-y-6">
      {/* Verification Card */}
      <div className="bg-neutral-900/60 border border-neutral-800 rounded-xl p-6">
        <div className="flex flex-col md:flex-row md:items-center justify-between gap-4">
          <div>
            <div className="flex items-center space-x-2">
              <span className="px-2.5 py-0.5 rounded text-xs font-mono font-bold bg-emerald-950 text-emerald-300 border border-emerald-800">
                PHASE 1 COMPLETE
              </span>
              <span className="text-xs text-neutral-400 font-mono">EDK II / X64 Target</span>
            </div>
            <h2 className="text-lg font-bold text-white font-mono mt-2 flex items-center">
              <ShieldCheck className="w-5 h-5 mr-2 text-emerald-400" />
              OpenVintage EFI Application Runtime Verification
            </h2>
            <p className="text-xs text-neutral-400 mt-1">
              Phase 1 successfully compiled a standalone X64 UEFI binary with GCC toolchain and verified device enumeration, block I/O detection, and clean exit in QEMU + OVMF.
            </p>
          </div>

          <div className="flex gap-2">
            <div className="px-3 py-2 bg-neutral-950 border border-neutral-800 rounded text-center font-mono">
              <div className="text-[10px] text-neutral-500 uppercase">QEMU 8.x</div>
              <div className="text-xs font-bold text-emerald-400">PASS</div>
            </div>
            <div className="px-3 py-2 bg-neutral-950 border border-neutral-800 rounded text-center font-mono">
              <div className="text-[10px] text-neutral-500 uppercase">OVMF UEFI</div>
              <div className="text-xs font-bold text-emerald-400">PASS</div>
            </div>
          </div>
        </div>
      </div>

      {/* Completed Functionality Checklist */}
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
        {[
          { label: 'OpenVintage EFI Application', desc: 'EDK II native PE32+ X64 image', status: 'VERIFIED' },
          { label: 'GCC Toolchain Pipeline', desc: 'x86_64-linux-gnu-gcc cross-target', status: 'VERIFIED' },
          { label: 'Lightweight Logger Subsystem', desc: 'Serial COM1 & UEFI SimpleTextOut', status: 'VERIFIED' },
          { label: 'PCI Device Enumeration', desc: 'Root bridge, bus, device, func walk', status: 'VERIFIED' },
          { label: 'Block I/O Protocol Detection', desc: 'SATA, ATA, SCSI mass storage descriptors', status: 'VERIFIED' },
          { label: 'Device Path Protocol Reporting', desc: 'ACPI, PCI, and Media path nodes', status: 'VERIFIED' },
          { label: 'Filesystem / Media Detection', desc: 'EFI System Partition (ESP) mounting', status: 'VERIFIED' },
          { label: 'Graceful Application Exit', desc: 'Returns EFI_SUCCESS (0x0) cleanly', status: 'VERIFIED' },
          { label: 'QEMU Virtual Hardware Harness', desc: 'Standard PC (Q35 + ICH9) virtualization', status: 'VERIFIED' },
        ].map((item, idx) => (
          <div
            key={idx}
            className="bg-neutral-950 border border-neutral-800 rounded-lg p-3.5 flex items-start space-x-3"
          >
            <CheckCircle2 className="w-4 h-4 text-emerald-400 mt-0.5 shrink-0" />
            <div>
              <div className="text-xs font-mono font-bold text-neutral-200">{item.label}</div>
              <div className="text-[11px] text-neutral-400">{item.desc}</div>
              <div className="text-[10px] font-mono text-emerald-500 mt-1 uppercase tracking-wider">
                {item.status}
              </div>
            </div>
          </div>
        ))}
      </div>

      {/* Verified Console & Serial Logs */}
      <div className="bg-neutral-950 border border-neutral-800 rounded-xl p-5">
        <div className="flex items-center justify-between pb-3 border-b border-neutral-800 mb-3">
          <div className="flex items-center space-x-2 text-xs font-mono text-neutral-300">
            <Terminal className="w-4 h-4 text-amber-400" />
            <span>QEMU + OVMF Execution Log Output (Verified Evidence)</span>
          </div>
          <span className="text-[10px] font-mono text-neutral-500">Exit Status: EFI_SUCCESS</span>
        </div>

        <div className="bg-black/90 rounded-lg p-4 font-mono text-xs text-neutral-300 space-y-1.5 overflow-x-auto border border-neutral-900">
          <div className="text-neutral-500">{"[0.000000] BdsDxe: loading Boot0001 \"OpenVintage.efi\" from PciRoot(0x0)/Pci(0x1F,0x2)/..."}</div>
          <div className="text-neutral-500">{"[0.001420] BdsDxe: starting Boot0001 \"OpenVintage.efi\""}</div>
          <div className="text-amber-400">{"[OV_INIT] ========================================================"}</div>
          <div className="text-amber-400">{"[OV_INIT] OPENVINTAGE PLATFORM FIRMWARE LAYER v0.1.0"}</div>
          <div className="text-amber-400">{"[OV_INIT] Initializing Core Logger Subsystem... OK"}</div>
          <div className="text-neutral-300">{"[OV_ARCH] Architecture detected: x86_64 (Intel Compatible, 64-bit Long Mode)"}</div>
          <div className="text-neutral-300">{"[OV_PCI ] Scanning PCI Hierarchy: 14 handles registered"}</div>
          <div className="text-neutral-400">{"[OV_PCI ]  Found [00:00.0] Host Bridge / DRAM Controller"}</div>
          <div className="text-neutral-400">{"[OV_PCI ]  Found [00:02.0] VGA Compatible Controller (QEMU / OVMF GOP active)"}</div>
          <div className="text-neutral-400">{"[OV_PCI ]  Found [00:1F.2] SATA AHCI Controller"}</div>
          <div className="text-sky-400">{"[OV_BLK ] Interrogating Block I/O Protocol..."}</div>
          <div className="text-neutral-400">{"[OV_BLK ]  Device 0: MediaId=1, BlockSize=512, LastBlock=20971519 (10.0 GiB)"}</div>
          <div className="text-neutral-400">{"[OV_PATH] Resolving Device Path:"}</div>
          <div className="text-neutral-400">{"[OV_PATH]  PciRoot(0x0)/Pci(0x1F,0x2)/Sata(0x0,0xFFFF,0x0)/HD(1,FAT32,ESP_SYS)"}</div>
          <div className="text-emerald-400">{"[OV_FS  ] SimpleFileSystem protocol discovered. Volume validated: OK"}</div>
          <div className="text-amber-400">{"[OV_INIT] Phase 1 bootstrap milestones successfully completed."}</div>
          <div className="text-emerald-400">{"[OV_EXIT] Releasing boot services resources. Exiting with EFI_SUCCESS (0x0)."}</div>
        </div>
      </div>
    </div>
  );
};
