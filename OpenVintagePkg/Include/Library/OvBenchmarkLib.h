/** @file
  OpenVintage Reproducible Benchmark Framework Definition.
  Phase 5 Empirical Measurement of Baseline vs OpenVintage Optimized Execution.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OV_BENCHMARK_LIB_H_
#define OV_BENCHMARK_LIB_H_

#include <Uefi.h>

//
// Individual Benchmark Workload Result
//
typedef struct {
  CHAR16  WorkloadName[32];

  // Baseline Metrics (Unoptimized, No Folding, Dead Code Retained, Cold Cache)
  UINT64  BaselineCycles;
  UINT32  BaselineDurationUs;
  UINT64  BaselineMemoryBytes;
  UINT32  BaselineInstructionCount;

  // OpenVintage Optimized Metrics (Constant Folding, DCE, Move Elimination, Warm Cache)
  UINT64  OptimizedCycles;
  UINT32  OptimizedDurationUs;
  UINT64  OptimizedMemoryBytes;
  UINT32  OptimizedInstructionCount;

  // Differential Metrics (Measured)
  UINT64  CyclesSaved;
  UINT32  SpeedupPercent;           // e.g. 145 = 1.45x
  UINT32  InstructionReductionPercent;
  UINT64  MemorySavedBytes;
  UINT32  CacheHitLatencyUs;        // Warm cache hit retrieval duration
} OV_BENCHMARK_METRIC;

//
// Full Benchmark Suite Results
//
typedef struct {
  // Test 1: Arithmetic & ALU Constant Folding
  OV_BENCHMARK_METRIC AluBenchmark;

  // Test 2: Dead Code & Redundant Move Elimination
  OV_BENCHMARK_METRIC DceBenchmark;

  // Test 3: Translation Cache Cold-Miss vs Warm-Hit
  OV_BENCHMARK_METRIC CacheBenchmark;

  // Test 4: End-to-End JIT Decode-Optimize-Emit Pipeline
  OV_BENCHMARK_METRIC JitPipelineBenchmark;

  // Aggregate Metrics
  UINT64              TotalBaselineCycles;
  UINT64              TotalOptimizedCycles;
  UINT64              TotalCyclesSaved;
  UINT32              OverallSpeedupPercent;
  UINT32              CacheEfficiencyGainPercent;
} OV_BENCHMARK_RESULTS;

/**
  Initialize benchmark framework.

  @retval EFI_SUCCESS  Benchmark subsystem initialized.
**/
EFI_STATUS
EFIAPI
OvBenchmarkInitialize (
  VOID
  );

/**
  Execute empirical, reproducible benchmark suite on host processor.
  Measures real TSC clock cycles for baseline vs OpenVintage path.

  @param[out] Results  Structure receiving empirical measurements.

  @retval EFI_SUCCESS            Suite executed.
  @retval EFI_INVALID_PARAMETER  Results is NULL.
**/
EFI_STATUS
EFIAPI
OvBenchmarkRunSuite (
  OUT OV_BENCHMARK_RESULTS  *Results
  );

/**
  Format and print benchmark results to logging stream.

  @param[in] Results  Pointer to benchmark results.
**/
VOID
EFIAPI
OvBenchmarkPrintSummary (
  IN CONST OV_BENCHMARK_RESULTS  *Results
  );

#endif // OV_BENCHMARK_LIB_H_
