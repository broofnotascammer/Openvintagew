/**
 * OpenVintage - Frontend Component & Engine Test Suite (Phase 6.1)
 * Tests core UI logic, state transformations, hardware invariance,
 * compatibility matrices, performance profile switching, and deployment confirmation.
 */

import { test, describe } from 'node:test';
import assert from 'node:assert';
import {
  PRESET_SIMULATED_PROFILES,
  NATIVE_HOST_HARDWARE_PROFILE,
  TARGET_OS_LIST,
  evaluateOsCompatibility,
  PERFORMANCE_PROFILES,
  DEFAULT_BOOT_TARGETS,
  DEFAULT_INTEGRATIONS,
  DEFAULT_DEPLOYMENT_PLAN,
  HardwareProfileData,
} from '../src/core/openvintageState';

describe('1. Hardware Rendering & Profile Invariance', () => {
  test('Preset profiles contain valid CPU, Memory, and GPU descriptors', () => {
    assert.strictEqual(PRESET_SIMULATED_PROFILES.length >= 4, true);
    const mbp = PRESET_SIMULATED_PROFILES[0];
    assert.strictEqual(mbp.modelIdentifier, 'MacBookPro9,1');
    assert.strictEqual(mbp.cpu.microarchitecture, 'Ivy Bridge');
    assert.strictEqual(mbp.cpu.physicalCores, 4);
    assert.strictEqual(mbp.memory.totalMB, 16384);
    assert.strictEqual(mbp.gpus.length, 2);
  });

  test('Simulated vs Native Source distinction is strictly preserved', () => {
    const simulated = PRESET_SIMULATED_PROFILES[0];
    assert.strictEqual(simulated.source, 'SIMULATED');
    assert.ok(simulated.sourceLabel.includes('SIMULATED'));

    const native = NATIVE_HOST_HARDWARE_PROFILE;
    assert.strictEqual(native.source, 'NATIVE');
    assert.ok(native.sourceLabel.includes('REAL HARDWARE') || native.sourceLabel.includes('Native'));
  });

  test('Dual GPU display preserves physical inventory when active GPU is switched', () => {
    const profile: HardwareProfileData = { ...PRESET_SIMULATED_PROFILES[0] };
    assert.strictEqual(profile.gpus.length, 2);
    assert.strictEqual(profile.activeGpuIndex, 1); // Discrete default

    // Switch to Integrated
    profile.activeGpuIndex = 0;
    assert.strictEqual(profile.activeGpuIndex, 0);
    assert.strictEqual(profile.gpus.length, 2); // Inventory count remains 2
    assert.strictEqual(profile.gpus[0].isIntegrated, true);
    assert.strictEqual(profile.gpus[1].isDiscrete, true);

    // Switch back to Discrete
    profile.activeGpuIndex = 1;
    assert.strictEqual(profile.activeGpuIndex, 1);
    assert.strictEqual(profile.gpus.length, 2); // Still 2
  });
});

describe('2. Compatibility Matrix & OS Evaluation', () => {
  test('Target OS list includes modern and legacy operating systems', () => {
    assert.ok(TARGET_OS_LIST.includes('macOS 12 Monterey'));
    assert.ok(TARGET_OS_LIST.includes('macOS 10.15 Catalina'));
    assert.ok(TARGET_OS_LIST.includes('macOS 13 Ventura'));
    assert.ok(TARGET_OS_LIST.includes('Linux 6.x LTS (Ubuntu/Debian)'));
  });

  test('Evaluates Monterey on Ivy Bridge requires OCLP root patches', () => {
    const mbp = PRESET_SIMULATED_PROFILES[0];
    const result = evaluateOsCompatibility('macOS 12 Monterey', mbp);
    assert.strictEqual(result.rating, 'SUPPORTED_OCLP');
    assert.strictEqual(result.integrationRequired, 'OCLP');
    assert.strictEqual(result.checks.legacyGpuPatches.required, true);
  });

  test('Evaluates Catalina on Ivy Bridge as natively supported', () => {
    const mbp = PRESET_SIMULATED_PROFILES[0];
    const result = evaluateOsCompatibility('macOS 10.15 Catalina', mbp);
    assert.strictEqual(result.rating, 'NATIVELY_SUPPORTED');
    assert.strictEqual(result.integrationRequired, 'None');
  });

  test('Cryptex bypass is flagged for non-AVX2 CPUs on Ventura and Sonoma', () => {
    const mbp = PRESET_SIMULATED_PROFILES[0]; // Ivy Bridge (No AVX2)
    assert.strictEqual(mbp.cpu.features.avx2, false);
    const result = evaluateOsCompatibility('macOS 13 Ventura', mbp);
    assert.strictEqual(result.checks.cryptexBypass.required, true);

    const haswell = PRESET_SIMULATED_PROFILES[1]; // Haswell (Has AVX2)
    assert.strictEqual(haswell.cpu.features.avx2, true);
    const resultHaswell = evaluateOsCompatibility('macOS 13 Ventura', haswell);
    assert.strictEqual(resultHaswell.checks.cryptexBypass.required, false);
  });
});

describe('3. Performance Profiles & Policies', () => {
  test('All 6 performance profiles exist with detailed policies', () => {
    assert.strictEqual(PERFORMANCE_PROFILES.length, 6);
    const profileIds = PERFORMANCE_PROFILES.map((p) => p.id);
    assert.deepStrictEqual(profileIds, [
      'MAX_PERFORMANCE',
      'GAMING',
      'BALANCED',
      'EFFICIENCY',
      'COMPATIBILITY',
      'CUSTOM',
    ]);
  });

  test('Performance profiles enforce honest engineering disclaimers', () => {
    for (const prof of PERFORMANCE_PROFILES) {
      assert.ok(prof.honestyDisclaimer.length > 20);
      assert.ok(prof.gpuPolicy.length > 5);
      assert.ok(prof.cpuGovernor.length > 5);
    }
  });
});

describe('4. Boot Target Management', () => {
  test('Default boot targets include macOS, Linux, and EFI utilities', () => {
    assert.ok(DEFAULT_BOOT_TARGETS.length >= 6);
    const defaultBoot = DEFAULT_BOOT_TARGETS.find((b) => b.isDefault);
    assert.ok(defaultBoot !== undefined);
    assert.strictEqual(defaultBoot?.isDefault, true);
  });

  test('Boot entries contain EFI paths and architecture specifications', () => {
    for (const target of DEFAULT_BOOT_TARGETS) {
      assert.ok(target.efiPath.startsWith('/'));
      assert.strictEqual(target.architecture, 'x86_64');
      assert.ok(target.recommendedProfile.length > 0);
    }
  });
});

describe('5. Deployment Plan & Security Guardrails', () => {
  test('Auditable deployment plan contains SHA-256 hashes and safety tarball', () => {
    assert.strictEqual(DEFAULT_DEPLOYMENT_PLAN.length, 4);
    const backupItem = DEFAULT_DEPLOYMENT_PLAN.find((i) => i.action === 'BACKUP');
    assert.ok(backupItem !== undefined);
    assert.ok(backupItem?.targetPath.includes('Backup'));

    for (const item of DEFAULT_DEPLOYMENT_PLAN) {
      assert.strictEqual(item.sha256.length, 64); // Full SHA-256
      assert.ok(item.sizeBytes > 0);
    }
  });

  test('Ecosystem integrations reflect OCLP and rEFInd status', () => {
    assert.strictEqual(DEFAULT_INTEGRATIONS.length, 3);
    const oclp = DEFAULT_INTEGRATIONS.find((i) => i.id === 'oclp');
    assert.ok(oclp !== undefined);
    assert.strictEqual(oclp?.status, 'RECOMMENDED');
  });
});
