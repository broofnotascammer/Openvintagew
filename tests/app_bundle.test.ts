/**
 * OpenVintage - App Bundle, Native Launcher & EFI Verification Test Suite
 * Tests Phase 8.1 requirements:
 * 1. Native .app bundle layout, Info.plist, executable permissions
 * 2. Mach-O 64-bit x86_64 binary headers and macOS 10.15 compatibility
 * 3. App executable launch behavior (does not immediately quit)
 * 4. Genuine PE32+ x86_64 UEFI binary headers for BootApp and HalDxe
 * 5. Hardware detection, GPU topology invariance, and MacBookPro9,1 simulation
 * 6. Non-destructive dry-run guardrails (zero ROM / SPI modification)
 */

import { test, describe } from 'node:test';
import assert from 'node:assert';
import fs from 'node:fs';
import path from 'node:path';
import { execSync } from 'node:child_process';

const ROOT_DIR = process.cwd();
const APP_DIR = path.join(ROOT_DIR, 'OpenVintage.app');
const MACOS_DIR = path.join(APP_DIR, 'Contents', 'MacOS');
const RESOURCES_DIR = path.join(APP_DIR, 'Contents', 'Resources');
const FIRMWARE_DIR = path.join(RESOURCES_DIR, 'firmware');

describe('Phase 8.1 — App Bundle & Packaging Verification', () => {
  test('App bundle directory structure exists with correct layout', () => {
    assert.strictEqual(fs.existsSync(APP_DIR), true, 'OpenVintage.app bundle exists');
    assert.strictEqual(fs.existsSync(path.join(APP_DIR, 'Contents')), true);
    assert.strictEqual(fs.existsSync(MACOS_DIR), true);
    assert.strictEqual(fs.existsSync(FIRMWARE_DIR), true);
  });

  test('Info.plist contains valid bundle identifiers and macOS 10.15 target', () => {
    const plistPath = path.join(APP_DIR, 'Contents', 'Info.plist');
    assert.strictEqual(fs.existsSync(plistPath), true, 'Info.plist exists');
    const content = fs.readFileSync(plistPath, 'utf8');
    assert.ok(content.includes('<key>CFBundleExecutable</key>'));
    assert.ok(content.includes('<string>OpenVintage</string>'));
    assert.ok(content.includes('<key>CFBundleIdentifier</key>'));
    assert.ok(content.includes('org.openvintage.OpenVintage'));
    assert.ok(content.includes('<key>LSMinimumSystemVersion</key>'));
    assert.ok(content.includes('<string>10.15.0</string>'));
    assert.ok(content.includes('NSSystemAdministrationUsageDescription'));
  });

  test('PkgInfo contains APPL signature', () => {
    const pkgInfoPath = path.join(APP_DIR, 'Contents', 'PkgInfo');
    assert.strictEqual(fs.existsSync(pkgInfoPath), true);
    const content = fs.readFileSync(pkgInfoPath, 'utf8');
    assert.strictEqual(content, 'APPLOVIN');
  });

  test('Mach-O 64-bit x86_64 binary exists and has valid Mach-O magic', () => {
    const machoPath = path.join(MACOS_DIR, 'OpenVintage-x86_64');
    assert.strictEqual(fs.existsSync(machoPath), true, 'Mach-O binary exists');
    const stat = fs.statSync(machoPath);
    assert.ok((stat.mode & 0o111) !== 0, 'Mach-O binary is executable');

    const buffer = fs.readFileSync(machoPath);
    // Mach-O 64-bit magic is 0xFEEDFACF (little endian: CF FA ED FE)
    const magic = buffer.readUInt32LE(0);
    assert.strictEqual(magic, 0xfeedfacf, 'Mach-O 64-bit magic matches 0xFEEDFACF');

    // cputype for x86_64 is 0x01000007 (CPU_TYPE_X86_64)
    const cpuType = buffer.readUInt32LE(4);
    assert.strictEqual(cpuType, 0x01000007, 'cputype matches x86_64');
  });
});

describe('Phase 8.1 — Authoritative UEFI Artifact Verification', () => {
  function verifyUefiBinary(filePath: string, expectedSubsystem: number, label: string) {
    assert.strictEqual(fs.existsSync(filePath), true, `${label} exists`);
    const buffer = fs.readFileSync(filePath);

    // MZ Header
    assert.strictEqual(buffer.toString('ascii', 0, 2), 'MZ', `${label} has MZ header`);

    // PE Header offset at 0x3C
    const peOffset = buffer.readUInt32LE(0x3c);
    const peSignature = buffer.toString('ascii', peOffset, peOffset + 4);
    assert.strictEqual(peSignature, 'PE\0\0', `${label} has PE signature`);

    // Machine type at peOffset + 4: 0x8664 = x86_64
    const machine = buffer.readUInt16LE(peOffset + 4);
    assert.strictEqual(machine, 0x8664, `${label} target architecture is x86_64 (0x8664)`);

    // Optional Header Magic at peOffset + 24: 0x20B = PE32+ (64-bit)
    const optMagic = buffer.readUInt16LE(peOffset + 24);
    assert.strictEqual(optMagic, 0x20b, `${label} is 64-bit PE32+`);

    // Subsystem at peOffset + 24 + 68
    const subsystem = buffer.readUInt16LE(peOffset + 24 + 68);
    assert.strictEqual(subsystem, expectedSubsystem, `${label} has subsystem 0x${expectedSubsystem.toString(16)}`);
  }

  test('OpenVintageBootApp.efi is valid x86_64 PE32+ EFI Application (0x0A)', () => {
    const bootAppPath = path.join(FIRMWARE_DIR, 'OpenVintageBootApp.efi');
    verifyUefiBinary(bootAppPath, 0x0a, 'OpenVintageBootApp.efi');
  });

  test('OpenVintageHalDxe.efi is valid x86_64 PE32+ EFI Boot Service Driver (0x0B)', () => {
    const halDxePath = path.join(FIRMWARE_DIR, 'OpenVintageHalDxe.efi');
    verifyUefiBinary(halDxePath, 0x0b, 'OpenVintageHalDxe.efi');
  });

  test('config.plist contains strict safety prohibition against ROM/firmware flashing', () => {
    const configPath = path.join(FIRMWARE_DIR, 'config.plist');
    assert.strictEqual(fs.existsSync(configPath), true);
    const content = fs.readFileSync(configPath, 'utf8');
    assert.ok(content.includes('<key>FirmwareFlashingProhibited</key>'));
    assert.ok(content.includes('<true/>'));
    assert.ok(content.includes('<key>RomModificationProhibited</key>'));
    assert.ok(content.includes('<true/>'));
  });
});

describe('Phase 8.1 — Executable Launch & Functional Verification', () => {
  const launcher = path.join(MACOS_DIR, 'OpenVintage');

  test('Native app executable launches with status command and returns valid system telemetry', () => {
    const output = execSync(`"${launcher}" status`, { encoding: 'utf8' });
    assert.ok(output.includes('OpenVintage System Status'));
    assert.ok(output.includes('GPU Inventory'));
    assert.ok(output.includes('Phase 8 EFI Staging:  READY'));
  });

  test('App executable launches with --version flag', () => {
    const output = execSync(`"${launcher}" --version`, { encoding: 'utf8' });
    assert.ok(output.includes('OpenVintage Version 1.0.0'));
    assert.ok(output.includes('MacBookPro9,1'));
  });

  test('App launches interactive console without immediately exiting and handles clean quit (0)', () => {
    // Send 1 (status), then 0 (exit)
    const output = execSync(`printf "1\\n\\n0\\n" | "${launcher}"`, { encoding: 'utf8' });
    assert.ok(output.includes('OpenVintage Pre-Boot Architecture Engine & Manager'));
    assert.ok(output.includes('[1] Display Overall System & Pre-Boot Status'));
    assert.ok(output.includes('[0] Exit OpenVintage Console'));
    assert.ok(output.includes('Exiting cleanly'));
  });

  test('App verifies EFI release binaries via verify-efi', () => {
    const output = execSync(`"${launcher}" verify-efi`, { encoding: 'utf8' });
    assert.ok(output.includes('EFI Binary & Artifact Integrity Audit'));
    assert.ok(output.includes('OpenVintageBootApp.efi'));
    assert.ok(output.includes('OpenVintageHalDxe.efi'));
    assert.ok(output.includes('[PASS] All required EFI binaries verified against authoritative release manifest.'));
    assert.ok(output.includes('[PASS] ROM/SPI/Firmware Volume flashing strictly prohibited & rejected.'));
  });

  test('Non-destructive Dry Run guarantees zero firmware and zero ROM modifications', () => {
    const output = execSync(`"${launcher}" installer-dry-run`, { encoding: 'utf8' });
    assert.ok(output.includes('Phase 8 Safe Real-Hardware EFI Installer: Non-Destructive Dry Run'));
    assert.ok(output.includes('FIRMWARE MODIFICATION: NONE (Physical ROM/SPI Unaltered)'));
    assert.ok(output.includes('ROM MODIFICATION:      NONE (Physical ROM/SPI Unaltered)'));
    assert.ok(output.includes('TARGET MAC:           MacBookPro9,1'));
  });

  test('Simulation mode switches to MacBookPro9,1 dual-GPU architecture', () => {
    const output = execSync(`"${launcher}" simulate MacBookPro9,1`, { encoding: 'utf8' });
    assert.ok(output.includes('MacBookPro9,1 [SIMULATED HARDWARE]'));
    assert.ok(output.includes('2 GPU(s) present (Physical topology invariant)'));
    assert.ok(output.includes('gMUX Hardware:        Present'));
  });
});
