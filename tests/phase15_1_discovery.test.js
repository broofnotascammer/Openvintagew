import test from 'node:test';
import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import os from 'node:os';
import fs from 'node:fs';

test('Phase 15.1: Real Native Host Detection', (t) => {
    const cpus = os.cpus();
    const totalMemBytes = os.totalmem();
    const platform = os.platform();
    const release = os.release();

    assert.ok(cpus.length >= 1, 'At least 1 host CPU thread reported');
    assert.ok(totalMemBytes > 0, 'Total host memory must be positive');
    assert.ok(['linux', 'darwin', 'win32'].includes(platform), 'Platform is a known OS');

    // Run native test binary
    const binPath = './bin/test_phase15_1_discovery';
    if (fs.existsSync(binPath)) {
        const output = execFileSync(binPath, [], { encoding: 'utf-8' });
        assert.match(output, /REAL HARDWARE DISCOVERY TEST SUITE/, 'Suite title present');
        assert.match(output, /Real Native Hardware Discovery/, 'Real discovery test ran');
        assert.match(output, /Mock Hardware Profile Explicit Separation/, 'Mock profile test ran');
        assert.match(output, /Multi-GPU Ranking & OVResolver/, 'Multi-GPU ranking test ran');
        assert.match(output, /ALL PHASE 15.1 DISCOVERY ASSERTIONS PASSED SUCCESSFULLY/, 'All C assertions passed');
    }
});

test('Phase 15.1: Real vs Mock Mode Behavioral Separation', (t) => {
    // Check that default mode is genuine detection
    const binPath = './bin/test_phase15_1_discovery';
    if (fs.existsSync(binPath)) {
        const output = execFileSync(binPath, [], { encoding: 'utf-8' });
        assert.match(output, /is_simulated_profile.*false|Real Host Hardware/, 'Real host hardware detected by default');
    }
});
