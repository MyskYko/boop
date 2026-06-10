#pragma once

#include "boop/config.h"

BOOP_C_HEADER_START

typedef struct BoopHeloParams {
  // scheduler
  int nVerbose;
  int nSeed;
  int nSchedulerFlow;
  int nJobs;
  int nThreads;
  int fDeterministic;
  int nTimeout;
  int fPartitioning;
  int nParallelPartitions;
  int fOptOnInsert;

  // module selection
  int nOptimizer;
  int nAnalyzer;
  int nSimulator;
  int nSat;
  int nPartitioner;

  // optimizer
  int nOptimizerVerbose;
  int nSortType;
  int fSortInitial;
  int fSortPerNode;
  int nOptimizerFlow;
  int nReductionMethod;
  int nDistance;
  int fCompatible;
  int fGreedy;
  int fNonlinearCost;

  // analyzer
  int nAnalyzerVerbose;

  // simulator
  int nSimulatorVerbose;
  int nWords;
  int fSave;
  int fKeepStimuli;

  // SAT
  int nSatVerbose;
  int nConflictLimit;

  // partitioner
  int nPartitionerVerbose;
  int nPartitionSize;
  int nPartitionSizeMin;
  int nPartitionInputMax;
} BoopHeloParams;

BOOP_C_HEADER_END
