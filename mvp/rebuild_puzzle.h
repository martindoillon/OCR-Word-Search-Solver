#ifndef REBUILD_PUZZLE_H
#define REBUILD_PUZZLE_H

#include "solver.h"
#include "highlight.h"

int solve_and_highlight(const char *input_image,
                        const char *output_image,
                        const char *solver_results_file);

#endif
