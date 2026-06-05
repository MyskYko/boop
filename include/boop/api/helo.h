#pragma once

#include <cassert>

#include "boop/config.h"
#include "boop/network/and_network.h"
#include "boop/partitioner/partitioner.h"
#include "boop/solver/cardinality_encoder.h"
#include "boop/solver/logic_encoder.h"
#include "boop/solver/solver.h"
#include "boop/solver/cadical_solver.h"
#include "boop/rrr/analyzer/analyzer.h"
#include "boop/rrr/optimizer/optimizer.h"
#include "boop/rrr/simulator/simulator.h"
#include "boop/rrr/solver/sat_based_solver.h"
#include "boop/scheduler/helo_scheduler.h"

BOOP_HEADER_START

namespace boop {

  struct HeloParams {
    // scheduler
    int nVerbose = 0;
    int nSeed = 0;
    int nSchedulerFlow = 0;
    int nJobs = 1;
    int nThreads = 1;
    bool fDeterministic = true;
    int nTimeout = 0;

    // partitioning
    bool fPartitioning = false;
    int nParallelPartitions = 1;
    bool fOptOnInsert = false;

    // implementation selection
    int nOptimizer = 0;
    int nAnalyzer = 0;
    int nSimulator = 0;
    int nSolver = 0;
    int nPartitioner = 0;

    // optimizer
    int nSortType = -1;
    bool fSortInitial = false;
    bool fSortPerNode = true;
    int nOptimizerFlow = 0;
    int nReductionMethod = 0;
    int nDistance = 0;
    bool fCompatible = false;
    bool fGreedy = true;
    bool fNonlinearCost = false;

    // simulator
    int nWords = 16;
    bool fSave = true;
    bool fKeepStimuli = true;

    // SAT
    int nConflictLimit = 0;

    // partitioner
    int nPartitionSize = 1000;
    int nPartitionSizeMin = 10;
    int nPartitionInputMax = 0;
  };

  template <typename Ntk>
  static inline Cost GetTwoInputSize(Ntk *pNtk) {
    Cost cost = 0;
    pNtk->ForEachInt([&](int nId) {
      cost += pNtk->GetNumFanins(nId) - 1;
    });
    return cost;
  }

  template <typename Opt, typename Prt>
  inline typename HeloScheduler<AndNetwork, Opt, Prt>::Parameter MakeSchedulerParams(const HeloParams &params) {
    typename HeloScheduler<AndNetwork, Opt, Prt>::Parameter par;

    par.fnObjective = GetTwoInputSize<AndNetwork>;
    par.nVerbose = params.nVerbose;
    par.nSeed = params.nSeed;
    par.nFlow = params.nSchedulerFlow;
    par.nJobs = params.nJobs;
    par.nThreads = params.nThreads;
    par.fDeterministic = params.fDeterministic;
    par.nTimeout = params.nTimeout;
    par.fPartitioning = params.fPartitioning;
    par.nParallelPartitions = params.nParallelPartitions;
    par.fOptOnInsert = params.fOptOnInsert;

    par.parOpt.nVerbose = params.nVerbose;
    par.parOpt.nSortType = params.nSortType;
    par.parOpt.fSortInitial = params.fSortInitial;
    par.parOpt.fSortPerNode = params.fSortPerNode;
    par.parOpt.nFlow = params.nOptimizerFlow;
    par.parOpt.nReductionMethod = params.nReductionMethod;
    par.parOpt.nDistance = params.nDistance;
    par.parOpt.fCompatible = params.fCompatible;
    par.parOpt.fGreedy = params.fGreedy;
    par.parOpt.fNonlinearCost = params.fNonlinearCost;

    par.parPrt.nVerbose = params.nVerbose;
    par.parPrt.nPartitionSize = params.nPartitionSize;
    par.parPrt.nPartitionSizeMin = params.nPartitionSizeMin;
    par.parPrt.nPartitionInputMax = params.nPartitionInputMax;

    return par;
  }

  template <typename Par>
  inline void LowerAnalyzerParams(Par &par, const HeloParams &params) {
    par.nVerbose = params.nVerbose;
  }

  template <typename Par>
  inline void LowerSimulatorParams(Par &par, const HeloParams &params) {
    par.nVerbose = params.nVerbose;
    par.nWords = params.nWords;
    par.fSave = params.fSave;
    par.fKeepStimuli = params.fKeepStimuli;
  }

  template <typename Par>
  inline void LowerSatParams(Par &par, const HeloParams &params) {
    par.nVerbose = params.nVerbose;
    par.nConflictLimit = params.nConflictLimit;
  }

  template <typename Ntk, typename Opt, typename Prt>
  inline void RunHelo(Ntk *pNtk, const typename HeloScheduler<Ntk, Opt, Prt>::Parameter &params) {
    HeloScheduler<Ntk, Opt, Prt> scheduler(pNtk, params);
    scheduler.Run();
  }

  template <typename Opt, typename Prt>
  inline void RunHeloImpl(AndNetwork *pNtk, const HeloParams &params) {
    HeloScheduler<AndNetwork, Opt, Prt> scheduler(pNtk, MakeSchedulerParams<Opt, Prt>(params));
    scheduler.Run();
  }

  template <typename Ana, typename Prt>
  inline void RunHeloWithOptimizer(AndNetwork *pNtk, const HeloParams &params, const typename Ana::Parameter &parAna) {
    switch(params.nOptimizer) {
    case 0: {
      using Opt = rrr::Optimizer<AndNetwork, Ana>;
      typename HeloScheduler<AndNetwork, Opt, Prt>::Parameter par = MakeSchedulerParams<Opt, Prt>(params);
      par.parOpt.parAna = parAna;
      RunHelo<AndNetwork, Opt, Prt>(pNtk, par);
      break;
    }
    default:
      assert(false);
    }
  }

  template <typename Sim, typename Sat, typename Prt>
  inline void RunHeloWithAnalyzer(AndNetwork *pNtk, const HeloParams &params, const typename Sim::Parameter &parSim, const typename Sat::Parameter &parSat) {
    switch(params.nAnalyzer) {
    case 0: {
      assert(params.nSimulator >= 0);
      assert(params.nSolver >= 0);
      using Ana = rrr::Analyzer<AndNetwork, Sim, Sat>;
      typename Ana::Parameter parAna;
      LowerAnalyzerParams(parAna, params);
      parAna.parSim = parSim;
      parAna.parSat = parSat;
      RunHeloWithOptimizer<Ana, Prt>(pNtk, params, parAna);
      break;
    }
    default:
      assert(false);
    }
  }

  template <typename Sat, typename Prt>
  inline void RunHeloWithSimulator(AndNetwork *pNtk, const HeloParams &params, const typename Sat::Parameter &parSat) {
    switch(params.nSimulator) {
    case -1:
    case 0: {
      using Sim = rrr::Simulator<AndNetwork>;
      typename Sim::Parameter parSim;
      if(params.nSimulator >= 0) {
        LowerSimulatorParams(parSim, params);
      }
      RunHeloWithAnalyzer<Sim, Sat, Prt>(pNtk, params, parSim, parSat);
      break;
    }
    default:
      assert(false);
    }
  }

  template <typename Prt>
  inline void RunHeloWithSatSolver(AndNetwork *pNtk, const HeloParams &params) {
    switch(params.nSolver) {
    case -1:
    case 0: {
      using Sol = solver::Solver<solver::CadicalSolver, solver::LogicEncoder, solver::CardinalityEncoder>;
      using Sat = rrr::SatBasedSolver<AndNetwork, Sol>;
      typename Sat::Parameter parSat;
      if(params.nSolver >= 0) {
        LowerSatParams(parSat, params);
      }
      RunHeloWithSimulator<Sat, Prt>(pNtk, params, parSat);
      break;
    }
    default:
      assert(false);
    }
  }

  template <typename Prt>
  inline void RunHeloWithAnalyzerPre(AndNetwork *pNtk, const HeloParams &params) {
    HeloParams paramsNew = params;
    switch(paramsNew.nAnalyzer) {
    case 0:
      break;
    default:
      paramsNew.nSolver = -1;
      paramsNew.nSimulator = -1;
      assert(false);
    }
    RunHeloWithSatSolver<Prt>(pNtk, paramsNew);
  }

  inline void RunHeloWithPartitioner(AndNetwork *pNtk, const HeloParams &params) {
    switch(params.nPartitioner) {
    case 0:
      RunHeloWithAnalyzerPre<Partitioner<AndNetwork>>(pNtk, params);
      break;
    default:
      assert(false);
    }
  }

  inline void RunHelo(AndNetwork *pNtk, const HeloParams &params) {
    RunHeloWithPartitioner(pNtk, params);
  }

} // namespace boop

BOOP_HEADER_END
