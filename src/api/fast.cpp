#include "boop/api/fast.h"

#include "boop/apps/fast.h"
#include "boop/io/aig.h"
#include "boop/network/and_network.h"
#include "boop/util/string.h"

#ifdef BOOP_USE_ARGPARSE
#include "argparse/argparse.hpp"
#endif

#include <exception>
#include <iostream>
#ifdef BOOP_USE_ARGPARSE
#include <string>
#endif

#ifdef BOOP_USE_ARGPARSE

namespace {

void ConfigureFastProgram(argparse::ArgumentParser &program) {
  program.add_argument("input").nargs(argparse::nargs_pattern::optional);
  program.add_argument("output").nargs(argparse::nargs_pattern::optional);
}

} // namespace

#endif

BOOP_C_IMPL_START

#ifdef BOOP_USE_ARGPARSE

int boop_fast_parse_argv(char *pInputPath, int nInputPathSize,
                         char *pOutputPath, int nOutputPathSize, int argc,
                         const char *const argv[]) {
  if (argv == nullptr || argc <= 0) {
    return 1;
  }
  argparse::ArgumentParser program("fast");
  ConfigureFastProgram(program);
  try {
    program.parse_args(argc, argv);
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

void boop_fast_print_help(void) {
  argparse::ArgumentParser program("fast");
  ConfigureFastProgram(program);
  std::cerr << program;
}

#endif

int boop_fast_optimize_file(const char *pInputPath, const char *pOutputPath) {
  if (pInputPath == nullptr || pOutputPath == nullptr) {
    return 1;
  }
  try {
    boop::AndNetwork network;
    int nLatches = boop::ReadAig(pInputPath, &network);
    boop::RunFast<boop::AndNetwork>(&network);
    boop::WriteAig(pOutputPath, &network, nLatches);
  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
  return 0;
}

BOOP_C_IMPL_END
