#ifndef HIGHLIGHT_H
#define HIGHLIGHT_H

#include "solver.h"

int highlight_words_on_image(
    const char *input_path,
    const char *output_path,
    const char *solver_results_path,
    int gx, int gy, int gw, int gh,
    int rows, int cols
);

#endif
