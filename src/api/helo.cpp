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

void StoreHeloParams(BoopHeloParams *params,
                     const argparse::ArgumentParser &program) {
  params->nVerbose = program.get<int>("--verbose");
  params->nSeed = program.get<int>("--seed");
  params->nSchedulerFlow = program.get<int>("--flow");
  params->nOptimizerFlow = program.get<int>("--optimizer-flow");
  params->nJobs = program.get<int>("--jobs");
  params->nThreads = program.get<int>("--threads");
  params->nTimeout = program.get<int>("--timeout");
  params->fPartitioning = program.get<bool>("--partitioning");
  params->nParallelPartitions = program.get<int>("--parallel-partitions");
  params->fOptOnInsert = program.get<bool>("--opt-on-insert");
  params->nPartitionSize = program.get<int>("--partition-size");
  params->nPartitionSizeMin = program.get<int>("--partition-size-min");
  params->nPartitionInputMax = program.get<int>("--partition-input-max");
  params->nOptimizer = program.get<int>("--optimizer");
  params->nAnalyzer = program.get<int>("--analyzer");
  params->nSimulator = program.get<int>("--simulator");
  params->nSat = program.get<int>("--sat");
  params->nPartitioner = program.get<int>("--partitioner");
  params->nOptimizerVerbose = program.get<int>("--optimizer-verbose");
  params->nAnalyzerVerbose = program.get<int>("--analyzer-verbose");
  params->nSimulatorVerbose = program.get<int>("--simulator-verbose");
  params->nSatVerbose = program.get<int>("--sat-verbose");
  params->nPartitionerVerbose = program.get<int>("--partitioner-verbose");
  params->nSortType = program.get<int>("--sort-type");
  params->fSortInitial = program.get<bool>("--sort-initial");
  params->fSortPerNode = !program.get<bool>("--no-sort-per-node");
  params->nReductionMethod = program.get<int>("--reduction-method");
  params->nDistance = program.get<int>("--distance");
  params->fCompatible = program.get<bool>("--compatible");
  params->fGreedy = !program.get<bool>("--no-greedy");
  params->fNonlinearCost = program.get<bool>("--nonlinear-cost");
  params->nWords = program.get<int>("--words");
  params->fSave = !program.get<bool>("--no-save");
  params->fKeepStimuli = !program.get<bool>("--no-keep-stimuli");
  params->nConflictLimit = program.get<int>("--conflict-limit");
}

} // namespace

#endif

BOOP_C_IMPL_START

void boop_helo_params_default(BoopHeloParams *params) {
  if (params == nullptr) {
    return;
  }
  params->nVerbose = 0;
  params->nSeed = 0;
  params->nSchedulerFlow = 0;
  params->nJobs = 1;
  params->nThreads = 1;
  params->fDeterministic = 1;
  params->nTimeout = 0;
  params->fPartitioning = 0;
  params->nParallelPartitions = 1;
  params->fOptOnInsert = 0;
  params->nOptimizer = 0;
  params->nAnalyzer = 0;
  params->nSimulator = 0;
  params->nSat = 0;
  params->nPartitioner = 0;
  params->nOptimizerVerbose = 0;
  params->nAnalyzerVerbose = 0;
  params->nSimulatorVerbose = 0;
  params->nSatVerbose = 0;
  params->nPartitionerVerbose = 0;
  params->nSortType = -1;
  params->fSortInitial = 0;
  params->fSortPerNode = 1;
  params->nOptimizerFlow = 0;
  params->nReductionMethod = 0;
  params->nDistance = 0;
  params->fCompatible = 0;
  params->fGreedy = 1;
  params->fNonlinearCost = 0;
  params->nWords = 16;
  params->fSave = 1;
  params->fKeepStimuli = 1;
  params->nConflictLimit = 0;
  params->nPartitionSize = 1000;
  params->nPartitionSizeMin = 10;
  params->nPartitionInputMax = 0;
}

#ifdef BOOP_USE_ARGPARSE

int boop_helo_params_parse_argv(BoopHeloParams *params, char *input_path,
                                int input_path_size, char *output_path,
                                int output_path_size, int argc,
                                const char *const argv[]) {
  if (params == nullptr || argv == nullptr || argc <= 0) {
    return 1;
  }
  argparse::ArgumentParser program("helo");
  ConfigureHeloProgram(program);
  try {
    program.parse_args(argc, argv);
    StoreHeloParams(params, program);
    if (const auto input = program.present<std::string>("input")) {
      if (!boop::CopyString(input_path, input_path_size, *input)) {
        std::cerr << "input path buffer is invalid or too small\n";
        return 1;
      }
    }
    if (const auto output = program.present<std::string>("output")) {
      if (!boop::CopyString(output_path, output_path_size, *output)) {
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

int boop_helo_optimize_file(const char *input_path, const char *output_path,
                            const BoopHeloParams *params) {
  if (input_path == nullptr || output_path == nullptr) {
    return 1;
  }
  BoopHeloParams default_params;
  if (params == nullptr) {
    boop_helo_params_default(&default_params);
    params = &default_params;
  }
  try {
    boop::AndNetwork ntk;
    int nLatches = boop::ReadAig(input_path, &ntk);
    boop::RunHelo(&ntk, *params);
    boop::WriteAig(output_path, &ntk, nLatches);
  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
  return 0;
}

BOOP_C_IMPL_END
