#include "argparse/argparse.hpp"

#include "boop/api/helo.h"
#include "boop/io/aig.h"

#include <exception>
#include <iostream>

int main(int argc, char **argv) {
  argparse::ArgumentParser program("boop");

  program.add_argument("input");
  program.add_argument("output");

  program.add_argument("-v", "--verbose")
    .default_value(0)
    .scan<'i', int>();
  program.add_argument("--seed")
    .default_value(0)
    .scan<'i', int>();
  program.add_argument("--flow")
    .default_value(0)
    .scan<'i', int>();
  program.add_argument("--optimizer-flow")
    .default_value(0)
    .scan<'i', int>();
  program.add_argument("-j", "--jobs")
    .default_value(1)
    .scan<'i', int>();
  program.add_argument("-t", "--threads")
    .default_value(1)
    .scan<'i', int>();
  program.add_argument("--timeout")
    .default_value(0)
    .scan<'i', int>();
  program.add_argument("--partitioning")
    .default_value(false)
    .implicit_value(true);
  program.add_argument("--parallel-partitions")
    .default_value(1)
    .scan<'i', int>();
  program.add_argument("--opt-on-insert")
    .default_value(false)
    .implicit_value(true);
  program.add_argument("--partition-size")
    .default_value(1000)
    .scan<'i', int>();
  program.add_argument("--partition-size-min")
    .default_value(10)
    .scan<'i', int>();
  program.add_argument("--partition-input-max")
    .default_value(0)
    .scan<'i', int>();
  program.add_argument("--optimizer")
    .default_value(0)
    .scan<'i', int>();
  program.add_argument("--analyzer")
    .default_value(0)
    .scan<'i', int>();
  program.add_argument("--simulator")
    .default_value(0)
    .scan<'i', int>();
  program.add_argument("--solver")
    .default_value(0)
    .scan<'i', int>();
  program.add_argument("--partitioner")
    .default_value(0)
    .scan<'i', int>();
  program.add_argument("--sort-type")
    .default_value(-1)
    .scan<'i', int>();
  program.add_argument("--sort-initial")
    .default_value(false)
    .implicit_value(true);
  program.add_argument("--no-sort-per-node")
    .default_value(false)
    .implicit_value(true);
  program.add_argument("--reduction-method")
    .default_value(0)
    .scan<'i', int>();
  program.add_argument("--distance")
    .default_value(0)
    .scan<'i', int>();
  program.add_argument("--compatible")
    .default_value(false)
    .implicit_value(true);
  program.add_argument("--no-greedy")
    .default_value(false)
    .implicit_value(true);
  program.add_argument("--nonlinear-cost")
    .default_value(false)
    .implicit_value(true);
  program.add_argument("--words")
    .default_value(16)
    .scan<'i', int>();
  program.add_argument("--no-save")
    .default_value(false)
    .implicit_value(true);
  program.add_argument("--no-keep-stimuli")
    .default_value(false)
    .implicit_value(true);
  program.add_argument("--conflict-limit")
    .default_value(0)
    .scan<'i', int>();

  try {
    program.parse_args(argc, argv);
  } catch(const std::exception &e) {
    std::cerr << e.what() << "\n";
    std::cerr << program;
    return 1;
  }

  boop::AndNetwork ntk;
  int nLatches = boop::ReadAig(program.get<std::string>("input"), &ntk);

  boop::HeloParams params;
  params.nVerbose = program.get<int>("--verbose");
  params.nSeed = program.get<int>("--seed");
  params.nSchedulerFlow = program.get<int>("--flow");
  params.nOptimizerFlow = program.get<int>("--optimizer-flow");
  params.nJobs = program.get<int>("--jobs");
  params.nThreads = program.get<int>("--threads");
  params.nTimeout = program.get<int>("--timeout");
  params.fPartitioning = program.get<bool>("--partitioning");
  params.nParallelPartitions = program.get<int>("--parallel-partitions");
  params.fOptOnInsert = program.get<bool>("--opt-on-insert");
  params.nPartitionSize = program.get<int>("--partition-size");
  params.nPartitionSizeMin = program.get<int>("--partition-size-min");
  params.nPartitionInputMax = program.get<int>("--partition-input-max");
  params.nOptimizer = program.get<int>("--optimizer");
  params.nAnalyzer = program.get<int>("--analyzer");
  params.nSimulator = program.get<int>("--simulator");
  params.nSolver = program.get<int>("--solver");
  params.nPartitioner = program.get<int>("--partitioner");
  params.nSortType = program.get<int>("--sort-type");
  params.fSortInitial = program.get<bool>("--sort-initial");
  params.fSortPerNode = !program.get<bool>("--no-sort-per-node");
  params.nReductionMethod = program.get<int>("--reduction-method");
  params.nDistance = program.get<int>("--distance");
  params.fCompatible = program.get<bool>("--compatible");
  params.fGreedy = !program.get<bool>("--no-greedy");
  params.fNonlinearCost = program.get<bool>("--nonlinear-cost");
  params.nWords = program.get<int>("--words");
  params.fSave = !program.get<bool>("--no-save");
  params.fKeepStimuli = !program.get<bool>("--no-keep-stimuli");
  params.nConflictLimit = program.get<int>("--conflict-limit");

  boop::RunHelo(&ntk, params);
  boop::WriteAig(program.get<std::string>("output"), &ntk, nLatches);

  return 0;
}
