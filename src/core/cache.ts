/**
 * OpenVintage - Multi-Tier Cache Subsystem
 * Tracks shader, pipeline, and resolution caches with validation hashes.
 */

import { CacheEntry } from '../types';

export const INITIAL_CACHE_ENTRIES: CacheEntry[] = [
  {
    id: 'c-sh-01',
    type: 'shader',
    sourceHash: 'sha256:7f83b1657ff1fc53b92dc18148a1d65dfc2d4b1fa3d677284addd200126d9069',
    compiledHash: 'glsl41:0x93bf114a890e',
    sizeBytes: 48200,
    hitCount: 1420,
    lastAccessed: '2 mins ago',
    targetHardwareId: 'mbp-mid2012-15',
  },
  {
    id: 'c-pipe-02',
    type: 'pipeline',
    sourceHash: 'sha256:12e79603f9082d41571d34190c102df2f8b5f903fb6ca8f407730e15ea2be3db',
    compiledHash: 'pso_bin:0x5e20cc901',
    sizeBytes: 124800,
    hitCount: 890,
    lastAccessed: '5 mins ago',
    targetHardwareId: 'mbp-mid2012-15',
  },
  {
    id: 'c-res-03',
    type: 'resolution',
    sourceHash: 'ov_cache_0x7b2190ae',
    compiledHash: 'route:native_gl41',
    sizeBytes: 1024,
    hitCount: 3410,
    lastAccessed: 'Just now',
    targetHardwareId: 'mbp-late2013-112',
  },
  {
    id: 'c-tex-04',
    type: 'texture',
    sourceHash: 'sha256:b5bb9d8014a0f9b1d61e21e796d78dccdf1352f23cd32812f4850b878ae4944c',
    compiledHash: 'swizzle_bc7:0x11029abf',
    sizeBytes: 2097152,
    hitCount: 215,
    lastAccessed: '12 mins ago',
    targetHardwareId: 'macpro-mid2010-51',
  },
];
