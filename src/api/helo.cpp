#include "boop/api/helo.h"
#include "boop/config.h"

#ifdef BOOP_USE_ARGPARSE
#include "argparse/argparse.hpp"
#endif

#include "boop/apps/helo.h"
#include "boop/io/aig.h"
#include "boop/network/and_network.h"
#include "boop/util/string.h"

#include <exception>
#include <iostream>
#ifdef BOOP_USE_ARGPARSE
#include <string>
#endif

#ifdef BOOP_USE_ARGPARSE

namespace {

void ConfigureHeloProgram(argparse::ArgumentParser &program) {
  program.add_argument("input").nargs(argparse::nargs_pattern::optional);
  program.add_argument("output").nargs(argparse::nargs_pattern::optional);
  program.add_argument("-v", "--verbose").default_value(0).scan<'i', int>();
  program.add_argument("--seed").default_value(0).scan<'i', int>();
  program.add_argument("--flow").default_value(0).scan<'i', int>();
  program.add_argument("--optimizer-flow").default_value(0).scan<'i', int>();
  program.add_argument("-j", "--jobs").default_value(1).scan<'i', int>();
  program.add_argument("-t", "--threads").default_value(1).scan<'i', int>();
  program.add_argument("--timeout").default_value(0).scan<'i', int>();
  program.add_argument("--partitioning")
      .default_value(false)
      .implicit_value(true);
  program.add_argument("--parallel-partitions")
      .default_value(1)
      .scan<'i', int>();
  program.add_argument("--opt-on-insert")
      .default_value(false)
      .implicit_value(true);
  program.add_argument("--partition-size").default_value(1000).scan<'i', int>();
  program.add_argument("--partition-size-min")
      .default_value(10)
      .scan<'i', int>();
  program.add_argument("--partition-input-max")
      .default_value(0)
      .scan<'i', int>();
  program.add_argument("--optimizer").default_value(0).scan<'i', int>();
  program.add_argument("--optimizer-verbose").default_value(0).scan<'i', int>();
  program.add_argument("--analyzer").default_value(0).scan<'i', int>();
  program.add_argument("--analyzer-verbose").default_value(0).scan<'i', int>();
  program.add_argument("--simulator").default_value(0).scan<'i', int>();
  program.add_argument("--simulator-verbose").default_value(0).scan<'i', int>();
  program.add_argument("--sat").default_value(0).scan<'i', int>();
  program.add_argument("--sat-verbose").default_value(0).scan<'i', int>();
  program.add_argument("--partitioner").default_value(0).scan<'i', int>();
  program.add_argument("--partitioner-verbose")
      .default_value(0)
      .scan<'i', int>();
  program.add_argument("--sort-type").default_value(-1).scan<'i', int>();
  program.add_argument("--sort-initial")
      .default_value(false)
      .implicit_value(true);
  program.add_argument("--no-sort-per-node")
      .default_value(false)
      .implicit_value(true);
  program.add_argument("--reduction-method").default_value(0).scan<'i', int>();
  program.add_argument("--distance").default_value(0).scan<'i', int>();
  program.add_argument("--compatible")
      .default_value(false)
      .implicit_value(true);
  program.add_argument("--no-greedy").default_value(false).implicit_value(true);
  program.add_argument("--nonlinear-cost")
      .default_value(false)
      .implicit_value(true);
  program.add_argument("--words").default_value(16).scan<'i', int>();
  program.add_argument("--no-save").default_value(false).implicit_value(true);
  program.add_argument("--no-keep-stimuli")
      .default_value(false)
      .implicit_value(true);
  program.add_argument("--conflict-limit").default_value(0).scan<'i', int>();
}

void StoreHeloParams(BoopHeloParams *pParams,
                     const argparse::ArgumentParser &program) {
  pParams->nVerbose = program.get<int>("--verbose");
  pParams->nSeed = program.get<int>("--seed");
  pParams->nSchedulerFlow = program.get<int>("--flow");
  pParams->nOptimizerFlow = program.get<int>("--optimizer-flow");
  pParams->nJobs = program.get<int>("--jobs");
  pParams->nThreads = program.get<int>("--threads");
  pParams->nTimeout = program.get<int>("--timeout");
  pParams->fPartitioning = program.get<bool>("--partitioning");
  pParams->nParallelPartitions = program.get<int>("--parallel-partitions");
  pParams->fOptOnInsert = program.get<bool>("--opt-on-insert");
  pParams->nPartitionSize = program.get<int>("--partition-size");
  pParams->nPartitionSizeMin = program.get<int>("--partition-size-min");
  pParams->nPartitionInputMax = program.get<int>("--partition-input-max");
  pParams->nOptimizer = program.get<int>("--optimizer");
  pParams->nAnalyzer = program.get<int>("--analyzer");
  pParams->nSimulator = program.get<int>("--simulator");
  pParams->nSat = program.get<int>("--sat");
  pParams->nPartitioner = program.get<int>("--partitioner");
  pParams->nOptimizerVerbose = program.get<int>("--optimizer-verbose");
  pParams->nAnalyzerVerbose = program.get<int>("--analyzer-verbose");
  pParams->nSimulatorVerbose = program.get<int>("--simulator-verbose");
  pParams->nSatVerbose = program.get<int>("--sat-verbose");
  pParams->nPartitionerVerbose = program.get<int>("--partitioner-verbose");
  pParams->nSortType = program.get<int>("--sort-type");
  pParams->fSortInitial = program.get<bool>("--sort-initial");
  pParams->fSortPerNode = !program.get<bool>("--no-sort-per-node");
  pParams->nReductionMethod = program.get<int>("--reduction-method");
  pParams->nDistance = program.get<int>("--distance");
  pParams->fCompatible = program.get<bool>("--compatible");
  pParams->fGreedy = !program.get<bool>("--no-greedy");
  pParams->fNonlinearCost = program.get<bool>("--nonlinear-cost");
  pParams->nWords = program.get<int>("--words");
  pParams->fSave = !program.get<bool>("--no-save");
  pParams->fKeepStimuli = !program.get<bool>("--no-keep-stimuli");
  pParams->nConflictLimit = program.get<int>("--conflict-limit");
}

} // namespace

#endif

BOOP_C_IMPL_START

void boop_helo_params_default(BoopHeloParams *pParams) {
  if (pParams == nullptr) {
    return;
  }
  pParams->nVerbose = 0;
  pParams->nSeed = 0;
  pParams->nSchedulerFlow = 0;
  pParams->nJobs = 1;
  pParams->nThreads = 1;
  pParams->fDeterministic = 1;
  pParams->nTimeout = 0;
  pParams->fPartitioning = 0;
  pParams->nParallelPartitions = 1;
  pParams->fOptOnInsert = 0;
  pParams->nOptimizer = 0;
  pParams->nAnalyzer = 0;
  pParams->nSimulator = 0;
  pParams->nSat = 0;
  pParams->nPartitioner = 0;
  pParams->nOptimizerVerbose = 0;
  pParams->nAnalyzerVerbose = 0;
  pParams->nSimulatorVerbose = 0;
  pParams->nSatVerbose = 0;
  pParams->nPartitionerVerbose = 0;
  pParams->nSortType = -1;
  pParams->fSortInitial = 0;
  pParams->fSortPerNode = 1;
  pParams->nOptimizerFlow = 0;
  pParams->nReductionMethod = 0;
  pParams->nDistance = 0;
  pParams->fCompatible = 0;
  pParams->fGreedy = 1;
  pParams->fNonlinearCost = 0;
  pParams->nWords = 16;
  pParams->fSave = 1;
  pParams->fKeepStimuli = 1;
  pParams->nConflictLimit = 0;
  pParams->nPartitionSize = 1000;
  pParams->nPartitionSizeMin = 10;
  pParams->nPartitionInputMax = 0;
}

#ifdef BOOP_USE_ARGPARSE

int boop_helo_params_parse_argv(BoopHeloParams *pParams, char *pInputPath,
                                int nInputPathSize, char *pOutputPath,
                                int nOutputPathSize, int argc,
                                const char *const argv[]) {
  if (pParams == nullptr || argv == nullptr || argc <= 0) {
    return 1;
  }
  argparse::ArgumentParser program("helo");
  ConfigureHeloProgram(program);
  try {
    program.parse_args(argc, argv);
    StoreHeloParams(pParams, program);
    if (const auto input = program.present<std::string>("input")) {
      if (!boop::copy_string(pInputPath, nInputPathSize, *input)) {
        std::cerr << "input path buffer is invalid or too small\n";
        return 1;
      }
    }
    if (const auto output = program.present<std::string>("output")) {
      if (!boop::copy_string(pOutputPath, nOutputPathSize, *output)) {
        std::cerr << "output path buffer is invalid or too small\n";
        return 1;
      }
    }
  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    std::cerr << program;
    return 1;
  }
  return 0;
}

void boop_helo_print_help(void) {
  argparse::ArgumentParser program("helo");
  ConfigureHeloProgram(program);
  std::cerr << program;
}

#endif

int boop_helo_optimize_file(const char *pInputPath, const char *pOutputPath,
                            const BoopHeloParams *pParams) {
  if (pInputPath == nullptr || pOutputPath == nullptr) {
    return 1;
  }
  BoopHeloParams paramsDefault;
  if (pParams == nullptr) {
    boop_helo_params_default(&paramsDefault);
    pParams = &paramsDefault;
  }
  try {
    boop::AndNetwork ntk;
    int nLatches = boop::ReadAig(pInputPath, &ntk);
    boop::RunHelo(&ntk, *pParams);
    boop::WriteAig(pOutputPath, &ntk, nLatches);
  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
  return 0;
}

BOOP_C_IMPL_END
