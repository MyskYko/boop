#pragma once

#include <cassert>

#include "boop/apps/helo_types.h"
#include "boop/config.h"
#include "boop/partitioner/partitioner.h"
#include "boop/rrr/analyzer/analyzer.h"
#include "boop/rrr/optimizer/optimizer.h"
#include "boop/rrr/simulator/simulator.h"
#include "boop/rrr/solver/sat_based_solver.h"
#include "boop/scheduler/helo_scheduler.h"
#include "boop/solver/cadical_solver.h"
#include "boop/solver/cardinality_encoder.h"
#include "boop/solver/logic_encoder.h"
#include "boop/solver/solver.h"

BOOP_HEADER_START

namespace boop {

template <typename Ntk> class Helo {
public:
  explicit Helo(const BoopHeloParams &params) : params_(params) {}

  void Run(Ntk *pNtk) const { RunWithPartitioner(pNtk); }

private:
  Cost GetTwoInputSize(Ntk *pNtk) const {
    Cost cost = 0;
    pNtk->ForEachInt([&](int nId) { cost += pNtk->GetNumFanins(nId) - 1; });
    return cost;
  }

  template <typename Opt, typename Prt>
  typename HeloScheduler<Ntk, Opt, Prt>::Parameter MakeSchedulerParams() const {
    typename HeloScheduler<Ntk, Opt, Prt>::Parameter par;
    par.fnObjective = [this](Ntk *pNtk) { return GetTwoInputSize(pNtk); };
    par.nVerbose = params_.nVerbose;
    par.nSeed = params_.nSeed;
    par.nFlow = params_.nSchedulerFlow;
    par.nJobs = params_.nJobs;
    par.nThreads = params_.nThreads;
    par.fDeterministic = params_.fDeterministic;
    par.nTimeout = params_.nTimeout;
    par.fPartitioning = params_.fPartitioning;
    par.nParallelPartitions = params_.nParallelPartitions;
    par.fOptOnInsert = params_.fOptOnInsert;
    par.parOpt.nVerbose = params_.nOptimizerVerbose;
    par.parOpt.nSortType = params_.nSortType;
    par.parOpt.fSortInitial = params_.fSortInitial;
    par.parOpt.fSortPerNode = params_.fSortPerNode;
    par.parOpt.nFlow = params_.nOptimizerFlow;
    par.parOpt.nReductionMethod = params_.nReductionMethod;
    par.parOpt.nDistance = params_.nDistance;
    par.parOpt.fCompatible = params_.fCompatible;
    par.parOpt.fGreedy = params_.fGreedy;
    par.parOpt.fNonlinearCost = params_.fNonlinearCost;
    par.parPrt.nVerbose = params_.nPartitionerVerbose;
    par.parPrt.nPartitionSize = params_.nPartitionSize;
    par.parPrt.nPartitionSizeMin = params_.nPartitionSizeMin;
    par.parPrt.nPartitionInputMax = params_.nPartitionInputMax;
    return par;
  }

  template <typename Par> void LowerAnalyzerParams(Par &par) const {
    par.nVerbose = params_.nAnalyzerVerbose;
  }

  template <typename Par> void LowerSimulatorParams(Par &par) const {
    par.nVerbose = params_.nSimulatorVerbose;
    par.nWords = params_.nWords;
    par.fSave = params_.fSave;
    par.fKeepStimuli = params_.fKeepStimuli;
  }

  template <typename Par> void LowerSatParams(Par &par) const {
    par.nVerbose = params_.nSatVerbose;
    par.nConflictLimit = params_.nConflictLimit;
  }

  template <typename Opt, typename Prt>
  void RunScheduler(
      Ntk *pNtk,
      const typename HeloScheduler<Ntk, Opt, Prt>::Parameter &params) const {
    HeloScheduler<Ntk, Opt, Prt> scheduler(pNtk, params);
    scheduler.Run();
  }

  template <typename Ana, typename Prt>
  void RunWithOptimizer(Ntk *pNtk,
                        const typename Ana::Parameter &parAna) const {
    switch (params_.nOptimizer) {
    case 0: {
      using Opt = rrr::Optimizer<Ntk, Ana>;
      typename HeloScheduler<Ntk, Opt, Prt>::Parameter par =
          MakeSchedulerParams<Opt, Prt>();
      par.parOpt.parAna = parAna;
      RunScheduler<Opt, Prt>(pNtk, par);
      break;
    }
    default:
      assert(false);
    }
  }

  template <typename Sim, typename Sat, typename Prt>
  void RunWithAnalyzer(Ntk *pNtk, const typename Sim::Parameter &parSim,
                       const typename Sat::Parameter &parSat) const {
    switch (params_.nAnalyzer) {
    case 0: {
      using Ana = rrr::Analyzer<Ntk, Sim, Sat>;
      typename Ana::Parameter parAna;
      LowerAnalyzerParams(parAna);
      parAna.parSim = parSim;
      parAna.parSat = parSat;
      RunWithOptimizer<Ana, Prt>(pNtk, parAna);
      break;
    }
    default:
      assert(false);
    }
  }

  template <typename Sat, typename Prt>
  void RunWithSimulator(Ntk *pNtk,
                        const typename Sat::Parameter &parSat) const {
    switch (params_.nSimulator) {
    case 0: {
      using Sim = rrr::Simulator<Ntk>;
      typename Sim::Parameter parSim;
      LowerSimulatorParams(parSim);
      RunWithAnalyzer<Sim, Sat, Prt>(pNtk, parSim, parSat);
      break;
    }
    default:
      assert(false);
    }
  }

  template <typename Prt> void RunWithSat(Ntk *pNtk) const {
    switch (params_.nSat) {
    case 0: {
      using Sol = solver::Solver<solver::CadicalSolver, solver::LogicEncoder,
                                 solver::CardinalityEncoder>;
      using Sat = rrr::SatBasedSolver<Ntk, Sol>;
      typename Sat::Parameter parSat;
      LowerSatParams(parSat);
      RunWithSimulator<Sat, Prt>(pNtk, parSat);
      break;
    }
    default:
      assert(false);
    }
  }

  template <typename Prt> void RunWithAnalyzerPre(Ntk *pNtk) const {
    switch (params_.nAnalyzer) {
    case 0:
      RunWithSat<Prt>(pNtk);
      break;
    default:
      assert(false);
    }
  }

  void RunWithPartitioner(Ntk *pNtk) const {
    switch (params_.nPartitioner) {
    case 0:
      RunWithAnalyzerPre<Partitioner<Ntk>>(pNtk);
      break;
    default:
      assert(false);
    }
  }

  const BoopHeloParams &params_;
};

template <typename Ntk>
inline void RunHelo(Ntk *pNtk, const BoopHeloParams &params) {
  Helo<Ntk>(params).Run(pNtk);
}

} // namespace boop

BOOP_HEADER_END
