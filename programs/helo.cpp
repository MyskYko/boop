#include "boop/api/helo.h"

int main(int argc, char **argv) {
  char pInputPath[4096];
  char pOutputPath[4096];
  BoopHeloParams params;
  boop_helo_params_default(&params);
  int nStatus =
      boop_helo_params_parse_argv(&params, pInputPath, sizeof(pInputPath),
                                  pOutputPath, sizeof(pOutputPath), argc, argv);
  if (nStatus != 0) {
    return nStatus;
  }
  if (pInputPath[0] == '\0' || pOutputPath[0] == '\0') {
    boop_helo_print_help();
    return 1;
  }
  return boop_helo_optimize_file(pInputPath, pOutputPath, &params);
}
