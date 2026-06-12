#pragma once

#include "boop/apps/helo_types.h"
#include "boop/config.h"

BOOP_C_HEADER_START

void boop_helo_params_default(BoopHeloParams *pParams);
#ifdef BOOP_USE_ARGPARSE
int boop_helo_params_parse_argv(BoopHeloParams *pParams, char *pInputPath,
                                int nInputPathSize, char *pOutputPath,
                                int nOutputPathSize, int argc,
                                const char *const argv[]);
void boop_helo_print_help(void);
#endif
int boop_helo_optimize_file(const char *pInputPath, const char *pOutputPath,
                            const BoopHeloParams *pParams);

BOOP_C_HEADER_END
