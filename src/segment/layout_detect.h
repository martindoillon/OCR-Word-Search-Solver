#ifndef LAYOUT_DETECT_H
#define LAYOUT_DETECT_H

// Runs the entire old main() pipeline on <input_image>.
// Creates output/grid_roi.txt, output/wordlist_roi.txt, output/*.bmp
// Returns 0 on success, non-zero on error.
int detect_layout(const char *input_image);

#endif

