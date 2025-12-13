#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "solver.h"
#include "predict_letters.h"
#include "highlight.h"

#define MAX_GRID_ROWS 32
#define MAX_GRID_COLS 32
#define MAX_WORDS 512
#define MAX_WORD_LEN 32

int solve_and_highlight(const char *input_image,
                        const char *output_image,
                        const char *solver_results_file)
{
    // --- Read grid ROI ---
    int gx, gy, gw, gh, cols, rows;
    FILE *f = fopen("output/grid_roi.txt", "r");
    if (!f) { perror("grid_roi.txt"); return -1; }
    if (fscanf(f, "%d %d %d %d %d %d", &gx, &gy, &gw, &gh, &cols, &rows) != 6) {
        fprintf(stderr, "Invalid grid ROI format\n");
        fclose(f);
        return -1;
    }
    fclose(f);

    char grid[MAX_GRID_ROWS][MAX_GRID_COLS + 1];

    // --- Load letters from BMPs ---
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            char fname[512];
            snprintf(fname, sizeof(fname), "output/grid/%02d_%02d.bmp", c, r);
            char ch = predict_letter_from_file(fname);
            if (ch >= 'a' && ch <= 'z') ch -= 32;
            grid[r][c] = ch;
        }
        grid[r][cols] = '\0';
    }

    // --- Read wordlist ROI ---
    int lx, ly, lw, lh, word_count;
    f = fopen("output/wordlist_roi.txt", "r");
    if (!f) { perror("wordlist_roi.txt"); return -1; }
    if (fscanf(f, "%d %d %d %d %d", &lx, &ly, &lw, &lh, &word_count) != 5) {
        fprintf(stderr, "Invalid wordlist ROI format\n");
        fclose(f);
        return -1;
    }
    fclose(f);

    char words[MAX_WORDS][MAX_WORD_LEN];
    for (int w = 0; w < word_count; ++w) {
        int char_idx = 0;
        while (1) {
            char fname[512];
            snprintf(fname, sizeof(fname), "output/words/%02d_%02d.bmp", w, char_idx);
            FILE *ff = fopen(fname, "r");
            if (!ff) break;
            fclose(ff);
            char ch = predict_letter_from_file(fname);
            if (ch >= 'a' && ch <= 'z') ch -= 32;
            words[w][char_idx++] = ch;
            if (char_idx >= MAX_WORD_LEN - 1) break;
        }
        words[w][char_idx] = '\0';
    }

    // --- Solve words and save results ---
    FILE *out = fopen(solver_results_file, "w");
    if (!out) out = stdout;

    char *gridptr[MAX_GRID_ROWS];
    for (int r = 0; r < rows; ++r) gridptr[r] = grid[r];

    for (int w = 0; w < word_count; ++w) {
        char word[MAX_WORD_LEN];
        strcpy(word, words[w]);
        str_uppercase(word);

        Point p0, p1;
        int found = search_word(gridptr, rows, cols, word, &p0, &p1);

        if (found) {
            fprintf(out, "%s %d %d %d %d\n", word, p0.x, p0.y, p1.x, p1.y);
        } else {
            fprintf(out, "%s NOT_FOUND\n", word);
        }
    }

    if (out != stdout) fclose(out);

    printf("\nSolver output saved to %s\n", solver_results_file);

    // --- Highlight words on a copy of the original image ---
    if (highlight_words_on_image(input_image, output_image, solver_results_file,
                                 gx, gy, gw, gh, rows, cols) != 0) {
        fprintf(stderr, "Failed to highlight words on image\n");
        return -1;
    }

    printf("Highlighted image saved to %s\n", output_image);

    return 0;
}
