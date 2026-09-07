#include "boop/api/fast.h"

int main(int argc, char **argv) {
  char pInputPath[4096] = {};
  char pOutputPath[4096] = {};
  int nStatus = boop_fast_parse_argv(pInputPath, sizeof(pInputPath),
                                     pOutputPath, sizeof(pOutputPath), argc,
                                     argv);
  if (nStatus != 0) {
    return nStatus;
  }
  if (pInputPath[0] == '\0' || pOutputPath[0] == '\0') {
    boop_fast_print_help();
    return 1;
  }
  return boop_fast_optimize_file(pInputPath, pOutputPath);
}
