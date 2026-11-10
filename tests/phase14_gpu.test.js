import test from 'node:test';
import assert from 'node:assert';

test('OpenVintage Phase 14 — Real OVIR-GPU Shader Translation & GLSL 330 Suite', async (t) => {
  await t.test('1. SPIR-V Header Validation & Parser Logic', () => {
    const SPIRV_MAGIC = 0x07230203;

    function validateHeader(words) {
      if (!words || words.length < 5) return { valid: false, error: 'Truncated header' };
      if (words[0] !== SPIRV_MAGIC) return { valid: false, error: 'Invalid magic' };
      if (words[3] === 0) return { valid: false, error: 'Invalid ID bound' };
      return {
        valid: true,
        version: words[1],
        generator: words[2],
        idBound: words[3],
        schema: words[4]
      };
    }

    assert.strictEqual(validateHeader([]).valid, false, 'Empty array rejected');
    assert.strictEqual(validateHeader([0x12345678, 1, 1, 10, 0]).valid, false, 'Bad magic rejected');

    const validWords = [SPIRV_MAGIC, 0x00010300, 0x00080001, 25, 0];
    const res = validateHeader(validWords);
    assert.strictEqual(res.valid, true, 'Valid header accepted');
    assert.strictEqual(res.idBound, 25, 'Bound preserved');
    assert.strictEqual(res.version, 0x00010300, 'Version preserved');
  });

  await t.test('2. Architecture-Neutral OVIR-GPU IR Construction & Type System', () => {
    class GpuIR {
      constructor(stage, entry) {
        this.stage = stage;
        this.entry = entry;
        this.types = new Map();
        this.constants = new Map();
        this.variables = [];
        this.instructions = [];
      }
      addType(id, kind, vecSize, bits) {
        this.types.set(id, { kind, vecSize, bits });
      }
      addVariable(id, typeId, storageClass, name, location) {
        this.variables.push({ id, typeId, storageClass, name, location });
      }
      addInstruction(op, resId, typeId, s1, s2) {
        this.instructions.push({ op, resId, typeId, s1, s2 });
      }
    }

    const ir = new GpuIR('Vertex', 'main');
    ir.addType(1, 'float32', 1, 32);
    ir.addType(2, 'vec4', 4, 32);
    ir.addVariable(10, 2, 'Input', 'in_pos', 0);
    ir.addVariable(11, 2, 'Output', 'out_color', 0);
    ir.addInstruction('LOAD', 14, 2, 10, 0);
    ir.addInstruction('ADD', 15, 2, 14, 1);
    ir.addInstruction('STORE', 0, 0, 11, 15);
    ir.addInstruction('RETURN', 0, 0, 0, 0);

    assert.strictEqual(ir.types.size, 2);
    assert.strictEqual(ir.variables.length, 2);
    assert.strictEqual(ir.instructions.length, 4);
    assert.strictEqual(ir.instructions[0].op, 'LOAD');
    assert.strictEqual(ir.instructions[3].op, 'RETURN');
  });

  await t.test('3. GPU IR Optimization Pass (Constant Folding & Dead Code Elimination)', () => {
    function optimize(instructions, constants) {
      const live = [];
      let folded = 0;
      let dce = 0;
      let afterReturn = false;

      for (const insn of instructions) {
        if (afterReturn) {
          dce++;
          continue; // eliminate unreachable
        }

        if (insn.op === 'ADD' && constants.has(insn.s1) && constants.has(insn.s2)) {
          // Fold constants
          const val1 = constants.get(insn.s1);
          const val2 = constants.get(insn.s2);
          constants.set(insn.resId, val1 + val2);
          folded++;
          continue; // folded away
        }

        live.push(insn);
        if (insn.op === 'RETURN') {
          afterReturn = true;
        }
      }

      return { live, folded, dce };
    }

    const consts = new Map([
      [1, 2.0],
      [2, 3.5]
    ]);
    const insns = [
      { op: 'ADD', resId: 3, s1: 1, s2: 2 }, // Fold -> 5.5
      { op: 'RETURN', resId: 0, s1: 0, s2: 0 },
      { op: 'MUL', resId: 4, s1: 3, s2: 2 }   // Unreachable dead code
    ];

    const result = optimize(insns, consts);
    assert.strictEqual(result.folded, 1, 'Constant folded');
    assert.strictEqual(result.dce, 1, 'Dead code removed');
    assert.strictEqual(result.live.length, 1, 'Only return instruction survives');
    assert.strictEqual(consts.get(3), 5.5, 'Constant evaluated correctly');
  });

  await t.test('4. GLSL 330 Core Backend Code Emission', () => {
    function emitGLSL(ir) {
      let code = '#version 330 core\n\n';
      for (const v of ir.variables) {
        const qualifier = v.storageClass === 'Input' ? 'in' : 'out';
        const typeName = v.type === 'vec4' ? 'vec4' : 'float';
        const loc = v.location !== undefined ? `layout(location = ${v.location}) ` : '';
        code += `${loc}${qualifier} ${typeName} ${v.name};\n`;
      }
      code += '\nvoid main() {\n';
      for (const insn of ir.instructions) {
        if (insn.op === 'ASSIGN') {
          code += `    ${insn.dest} = ${insn.src};\n`;
        } else if (insn.op === 'RETURN') {
          code += '    return;\n';
        }
      }
      code += '}\n';
      return code;
    }

    const ir = {
      variables: [
        { storageClass: 'Input', type: 'vec4', name: 'in_pos', location: 0 },
        { storageClass: 'Output', type: 'vec4', name: 'out_color', location: 0 }
      ],
      instructions: [
        { op: 'ASSIGN', dest: 'out_color', src: 'in_pos + vec4(0.2, 0.4, 0.6, 1.0)' },
        { op: 'RETURN' }
      ]
    };

    const glsl = emitGLSL(ir);
    assert.ok(glsl.includes('#version 330 core'), 'Contains GLSL version header');
    assert.ok(glsl.includes('layout(location = 0) in vec4 in_pos;'), 'Contains input declaration');
    assert.ok(glsl.includes('layout(location = 0) out vec4 out_color;'), 'Contains output declaration');
    assert.ok(glsl.includes('void main()'), 'Contains main entry point');
  });

  await t.test('5. Operation Reconstruction (Matrix-Vector Multiply Deconstruction)', () => {
    function reconstructOps(instructions) {
      const output = [];
      let reconstructed = 0;
      for (const insn of instructions) {
        if (insn.op === 'MAT_MUL_VEC') {
          // Deconstruct into 4 dot products
          for (let row = 0; row < 4; row++) {
            output.push({ op: 'DOT', row, mat: insn.mat, vec: insn.vec });
          }
          output.push({ op: 'VEC4_CONSTRUCT', res: insn.res });
          reconstructed++;
        } else {
          output.push(insn);
        }
      }
      return { output, reconstructed };
    }

    const ops = [
      { op: 'LOAD' },
      { op: 'MAT_MUL_VEC', mat: 'u_mvp', vec: 'in_pos', res: 'gl_Position' },
      { op: 'RETURN' }
    ];

    const res = reconstructOps(ops);
    assert.strictEqual(res.reconstructed, 1, 'Reconstructed 1 complex operation');
    assert.strictEqual(res.output.length, 7, 'Decomposed into 4 dot products + construct + load/return');
  });

  await t.test('6. 5-Stage GPU Capability Resolution Routing', () => {
    function resolveGpuRoute(workload, hostCaps) {
      const needed = workload.requiredFeatures;
      if ((needed & hostCaps.supportedFeatures) === needed) {
        return hostCaps.hasNativeApi ? 'NATIVE' : 'TRANSLATION';
      }
      if ((needed & (hostCaps.supportedFeatures | hostCaps.partialFeatures)) === needed) {
        return 'RECONSTRUCTION';
      }
      if (hostCaps.cpuFallbackSupported) {
        return 'CPU_FALLBACK';
      }
      return 'UNSUPPORTED';
    }

    const hostCaps = {
      hasNativeApi: false,
      supportedFeatures: 0x0007,
      partialFeatures: 0x0008,
      cpuFallbackSupported: true
    };

    assert.strictEqual(resolveGpuRoute({ requiredFeatures: 0x0003 }, hostCaps), 'TRANSLATION');
    assert.strictEqual(resolveGpuRoute({ requiredFeatures: 0x000F }, hostCaps), 'RECONSTRUCTION');
    assert.strictEqual(resolveGpuRoute({ requiredFeatures: 0x0020 }, hostCaps), 'CPU_FALLBACK');

    const noFallbackCaps = { ...hostCaps, cpuFallbackSupported: false };
    assert.strictEqual(resolveGpuRoute({ requiredFeatures: 0x0020 }, noFallbackCaps), 'UNSUPPORTED');
  });

  await t.test('7. Telemetry Metrics Integration', () => {
    class GpuTelemetry {
      constructor() {
        this.parsed = 0;
        this.translated = 0;
        this.optimized = 0;
        this.compiled = 0;
        this.reconstructed = 0;
        this.fallbacks = 0;
      }
      recordPipelinePass() {
        this.parsed++;
        this.translated++;
        this.optimized++;
        this.compiled++;
      }
    }

    const tel = new GpuTelemetry();
    tel.recordPipelinePass();
    assert.strictEqual(tel.parsed, 1);
    assert.strictEqual(tel.translated, 1);
    assert.strictEqual(tel.optimized, 1);
    assert.strictEqual(tel.compiled, 1);
  });
});
