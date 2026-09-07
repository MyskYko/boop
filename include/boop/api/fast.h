#pragma once

#include "boop/config.h"

BOOP_C_HEADER_START

#ifdef BOOP_USE_ARGPARSE
int boop_fast_parse_argv(char *pInputPath, int nInputPathSize,
                         char *pOutputPath, int nOutputPathSize, int argc,
                         const char *const argv[]);
void boop_fast_print_help(void);
#endif
int boop_fast_optimize_file(const char *pInputPath, const char *pOutputPath);

BOOP_C_HEADER_END
