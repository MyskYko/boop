#include "boop/api/helo.h"

int main(int argc, char **argv) {
  char input_path[4096];
  char output_path[4096];
  BoopHeloParams params;
  boop_helo_params_default(&params);
  int status = boop_helo_params_parse_argv(&params, input_path, sizeof(input_path), output_path, sizeof(output_path), argc, argv);
  if(status != 0) {
    return status;
  }
  if(input_path[0] == '\0' || output_path[0] == '\0') {
    boop_helo_print_help();
    return 1;
  }
  return boop_helo_optimize_file(input_path, output_path, &params);
}
