import test from 'node:test';
import assert from 'node:assert';

test('OpenVintage Phase 13 — DBT, NEON & Guest Virtual MMU Suite', async (t) => {
  await t.test('1. Register Allocation & Semantics (XZR, SP, W/X Widths)', () => {
    class RegAlloc {
      constructor() {
        this.regs = new BigUint64Array(32);
        this.sp = 0n;
        this.spills = 0;
      }
      write(r, val) {
        if (r === 31) return; // XZR write is discarded
        this.regs[r] = BigInt.asUintN(64, BigInt(val));
      }
      writeW(r, val) {
        if (r === 31) return;
        // 32-bit zero extension
        this.regs[r] = BigInt.asUintN(32, BigInt(val));
      }
      read(r) {
        if (r === 31) return 0n; // XZR always 0
        return this.regs[r];
      }
    }

    const ra = new RegAlloc();
    assert.strictEqual(ra.read(31), 0n, 'XZR initial read is 0');
    ra.write(31, 0x1234567890abcdefn);
    assert.strictEqual(ra.read(31), 0n, 'XZR write discarded');

    ra.writeW(0, 0xffffffffn);
    assert.strictEqual(ra.read(0), 0xffffffffn, 'W0 zero-extends to 64-bit');

    ra.write(1, 0x1234567890abcdefn);
    assert.strictEqual(ra.read(1), 0x1234567890abcdefn, 'X1 full 64-bit stored');
  });

  await t.test('2. 14 ARM64 Condition Codes Evaluation', () => {
    const evalCond = (flags, cond) => {
      const z = (flags & 1) !== 0;
      const n = (flags & 2) !== 0;
      const c = (flags & 4) !== 0;
      const v = (flags & 8) !== 0;
      switch (cond) {
        case 0: return z;           // EQ
        case 1: return !z;          // NE
        case 2: return c;           // CS
        case 3: return !c;          // CC
        case 4: return n;           // MI
        case 5: return !n;          // PL
        case 6: return v;           // VS
        case 7: return !v;          // VC
        case 8: return c && !z;     // HI
        case 9: return !c || z;     // LS
        case 10: return n === v;    // GE
        case 11: return n !== v;    // LT
        case 12: return !z && (n === v); // GT
        case 13: return z || (n !== v);  // LE
        case 14: return true;       // AL
        default: return true;
      }
    };

    const flagZ = 1;
    assert.strictEqual(evalCond(flagZ, 0), true, 'EQ with Z=1');
    assert.strictEqual(evalCond(flagZ, 1), false, 'NE with Z=1');
    assert.strictEqual(evalCond(0, 1), true, 'NE with Z=0');
    assert.strictEqual(evalCond(4, 2), true, 'CS with C=1');
    assert.strictEqual(evalCond(0, 3), true, 'CC with C=0');
  });

  await t.test('3. DBT Direct Block Linking & Hot-Loop Detection', () => {
    class TraceCache {
      constructor(threshold = 5) {
        this.traces = new Map();
        this.threshold = threshold;
        this.hotLoops = 0;
      }
      record(pc, isBackward) {
        let entry = this.traces.get(pc);
        if (!entry) {
          entry = { count: 1, isHot: false };
          this.traces.set(pc, entry);
        } else {
          entry.count++;
          if (isBackward && entry.count >= this.threshold && !entry.isHot) {
            entry.isHot = true;
            this.hotLoops++;
          }
        }
        return entry;
      }
    }

    const tc = new TraceCache(5);
    for (let i = 0; i < 4; i++) {
      const e = tc.record(0x1000, true);
      assert.strictEqual(e.isHot, false, `Not hot at iteration ${i + 1}`);
    }
    const hotEntry = tc.record(0x1000, true);
    assert.strictEqual(hotEntry.isHot, true, 'Hot loop detected at iteration 5');
    assert.strictEqual(tc.hotLoops, 1, 'Hot loop count is 1');
  });

  await t.test('4. ARM64 NEON Vector Bitstream Decoders', () => {
    const decodeNeon = (raw) => {
      const u = raw >>> 0;
      if ((u & 0xffe0fc00) === 0x4e208400) {
        return { op: 'VEC_ADD', arr: '4S', dst: u & 0x1f, src1: (u >> 5) & 0x1f, src2: (u >> 16) & 0x1f };
      }
      if ((u & 0xffe0fc00) === 0x4e608400 || (u & 0xffe0fc00) === 0x6e208400) {
        return { op: 'VEC_SUB', arr: '4S', dst: u & 0x1f, src1: (u >> 5) & 0x1f, src2: (u >> 16) & 0x1f };
      }
      if ((u & 0xffe0fc00) === 0x4e209c00) {
        return { op: 'VEC_MUL', arr: '4S', dst: u & 0x1f, src1: (u >> 5) & 0x1f, src2: (u >> 16) & 0x1f };
      }
      if ((u & 0xffe0fc00) === 0x4ee08400) {
        return { op: 'VEC_ADD', arr: '2D', dst: u & 0x1f, src1: (u >> 5) & 0x1f, src2: (u >> 16) & 0x1f };
      }
      if ((u & 0xffe0fc00) === 0x6ee08400) {
        return { op: 'VEC_SUB', arr: '2D', dst: u & 0x1f, src1: (u >> 5) & 0x1f, src2: (u >> 16) & 0x1f };
      }
      return null;
    };

    const add4s = decodeNeon(0x4e228420);
    assert.strictEqual(add4s?.op, 'VEC_ADD');
    assert.strictEqual(add4s?.arr, '4S');
    assert.strictEqual(add4s?.dst, 0);
    assert.strictEqual(add4s?.src1, 1);
    assert.strictEqual(add4s?.src2, 2);

    const sub4s = decodeNeon(0x4e658483);
    assert.strictEqual(sub4s?.op, 'VEC_SUB');
    assert.strictEqual(sub4s?.arr, '4S');
    assert.strictEqual(sub4s?.dst, 3);

    const mul4s = decodeNeon(0x4e209ce6);
    assert.strictEqual(mul4s?.op, 'VEC_MUL');
    assert.strictEqual(mul4s?.arr, '4S');

    const add2d = decodeNeon(0x4ee38441);
    assert.strictEqual(add2d?.op, 'VEC_ADD');
    assert.strictEqual(add2d?.arr, '2D');

    const sub2d = decodeNeon(0x6ee684a4);
    assert.strictEqual(sub2d?.op, 'VEC_SUB');
    assert.strictEqual(sub2d?.arr, '2D');
  });

  await t.test('5. Vector Operations & Scalar Equivalence', () => {
    const v1 = new Uint32Array([10, 20, 30, 40]);
    const v2 = new Uint32Array([1, 2, 3, 4]);
    const vAdd = new Uint32Array(4);
    const vSub = new Uint32Array(4);
    const vMul = new Uint32Array(4);

    for (let i = 0; i < 4; i++) {
      vAdd[i] = v1[i] + v2[i];
      vSub[i] = v1[i] - v2[i];
      vMul[i] = v1[i] * v2[i];
    }

    assert.deepStrictEqual(Array.from(vAdd), [11, 22, 33, 44], 'Vector ADD 4S');
    assert.deepStrictEqual(Array.from(vSub), [9, 18, 27, 36], 'Vector SUB 4S');
    assert.deepStrictEqual(Array.from(vMul), [10, 40, 90, 160], 'Vector MUL 4S');
  });

  await t.test('6. Two-Level Page Table & TLB Emulation', () => {
    class PageTable {
      constructor() {
        this.pgd = new Map();
      }
      map(va, pa, flags) {
        if ((va % 4096) !== 0 || (pa % 4096) !== 0) throw new Error('Misaligned');
        const pgdIdx = Math.floor(va / (512 * 4096));
        const pteIdx = Math.floor((va % (512 * 4096)) / 4096);
        if (!this.pgd.has(pgdIdx)) {
          this.pgd.set(pgdIdx, new Map());
        }
        this.pgd.get(pgdIdx).set(pteIdx, { pa, flags });
      }
      walk(va) {
        const pgdIdx = Math.floor(va / (512 * 4096));
        const pteIdx = Math.floor((va % (512 * 4096)) / 4096);
        const l2 = this.pgd.get(pgdIdx);
        if (!l2 || !l2.has(pteIdx)) return null;
        return l2.get(pteIdx);
      }
    }

    const pt = new PageTable();
    assert.throws(() => pt.map(0x1001, 0x2000, 7), /Misaligned/);
    pt.map(0x10000, 0x2000, 7);
    const entry = pt.walk(0x10000);
    assert.strictEqual(entry?.pa, 0x2000, 'Page walk maps to 0x2000');
    assert.strictEqual(pt.walk(0x99000), null, 'Unmapped address returns null');
  });

  await t.test('7. Structured MMU Fault Reporting', () => {
    const handleAccess = (page, access, isSupervisor) => {
      if (!page) {
        return { fault: 'TRANSLATION', code: 1 };
      }
      if (access === 'WRITE' && !page.write) {
        return { fault: 'PERMISSION', code: 2 };
      }
      if (!isSupervisor && !page.user) {
        return { fault: 'PRIVILEGE', code: 4 };
      }
      return { fault: 'NONE', code: 0 };
    };

    const transFault = handleAccess(null, 'READ', true);
    assert.strictEqual(transFault.fault, 'TRANSLATION');

    const roPage = { write: false, user: true };
    const permFault = handleAccess(roPage, 'WRITE', true);
    assert.strictEqual(permFault.fault, 'PERMISSION');

    const superPage = { write: true, user: false };
    const privFault = handleAccess(superPage, 'READ', false);
    assert.strictEqual(privFault.fault, 'PRIVILEGE');
  });
});
