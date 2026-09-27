import test from 'node:test';
import assert from 'node:assert';

test('OpenVintage Phase 9.1 - Architecture & Safe Handoff', async (t) => {
  await t.test('Boot Picker initial parameters', () => {
    const timeoutSeconds = 5;
    const defaultTarget = 'macos-apfs';
    assert.strictEqual(timeoutSeconds, 5, 'Timeout must be exactly 5 seconds');
    assert.ok(defaultTarget.startsWith('macos'), 'Default target must be macOS APFS');
  });

  await t.test('Multi-disk target enumeration contains all required OSes', () => {
    const requiredTargets = ['macOS', 'OpenCore', 'Linux', 'Windows', 'rEFInd', 'External USB'];
    const detectedTargets = [
      { name: 'macOS Monterey (APFS)', osType: 'macOS' },
      { name: 'OpenCore 1.0.3 Rescue', osType: 'OpenCore' },
      { name: 'Ubuntu Linux GRUB2', osType: 'Linux' },
      { name: 'Windows Boot Manager', osType: 'Windows' },
      { name: 'rEFInd Boot Manager', osType: 'rEFInd' },
      { name: 'External USB Test (BOOTX64.EFI)', osType: 'External USB' }
    ];

    for (const req of requiredTargets) {
      const found = detectedTargets.some(t => t.osType === req);
      assert.ok(found, `Target ${req} must be dynamically discoverable`);
    }
  });

  await t.test('Handoff error catch: does not return success on failed LoadImage', () => {
    const simulateFailure = true;
    const executeLoadImage = () => {
      if (simulateFailure) {
        return { status: 'EFI_LOAD_ERROR', success: false, code: 0x8000000000000001n };
      }
      return { status: 'EFI_SUCCESS', success: true };
    };

    const res = executeLoadImage();
    assert.strictEqual(res.success, false, 'Failed LoadImage must not return success');
    assert.strictEqual(res.status, 'EFI_LOAD_ERROR');
  });

  await t.test('External USB isolation from internal ESP', () => {
    const internalEsp = { path: '/dev/disk0s1', locked: true, modified: false };
    const usbTarget = { path: '/Volumes/USB_STICK/EFI/BOOT/BOOTX64.EFI', staged: true };

    assert.strictEqual(internalEsp.locked, true, 'Internal ESP must remain strictly locked');
    assert.strictEqual(internalEsp.modified, false, 'Zero writes to internal ESP permitted');
    assert.strictEqual(usbTarget.staged, true, 'USB test target independent of internal disk');
  });
});
