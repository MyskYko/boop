#pragma once

#include "boop/apps/helo_types.h"

BOOP_C_HEADER_START

void boop_helo_params_default(BoopHeloParams *params);
#ifdef BOOP_USE_ARGPARSE
int boop_helo_params_parse_argv(BoopHeloParams *params, char *input_path, int input_path_size, char *output_path, int output_path_size, int argc, const char *const argv[]);
void boop_helo_print_help(void);
#endif
int boop_helo_optimize_file(const char *input_path, const char *output_path, const BoopHeloParams *params);

BOOP_C_HEADER_END
