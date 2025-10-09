#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Simple word-search (mots mêlés) solver in C
// Usage:
//   1) Prepare two files:
//      - grid.txt : each non-empty line is a row of letters (spaces are ignored). All rows must have same length.
//      - words.txt: one word per line (spaces ignored). Words can be uppercase or lowercase.
//   2) Compile:
//        gcc -std=c11 -O2 -o wordsearch_solver wordsearch_solver.c
//   3) Run:
//        ./wordsearch_solver grid.txt words.txt
//   Output: For each word the program prints where it was found (1-based row/col) and direction.

#define MAX_LINE 1024

char **load_grid(const char *filename, int *out_rows, int *out_cols) {
    FILE *f = fopen(filename, "r");
    if (!f) { perror("fopen grid"); return NULL; }

    char line[MAX_LINE];
    char **grid = NULL;
    int rows = 0;
    int cols = -1;

    while (fgets(line, sizeof(line), f)) {
        // trim newline
        char *p = line;
        while (*p && (*p == '\r' || *p == '\n')) *p++ = '\0';

        // remove spaces and tabs
        char compact[MAX_LINE];
        int j = 0;
        for (int i = 0; line[i] && line[i] != '\r' && line[i] != '\n'; ++i) {
            if (!isspace((unsigned char)line[i])) compact[j++] = (char)toupper((unsigned char)line[i]);
        }
        compact[j] = '\0';
        if (j == 0) continue; // skip empty lines

        if (cols == -1) cols = j;
        if (j != cols) {
            fprintf(stderr, "Erreur: lignes de grille de longueurs différentes (%d vs %d)\n", cols, j);
            fclose(f);
            // free allocated rows
            for (int k = 0; k < rows; ++k) free(grid[k]);
            free(grid);
            return NULL;
        }

        grid = realloc(grid, sizeof(char*) * (rows + 1));
        grid[rows] = malloc(cols + 1);
        strcpy(grid[rows], compact);
        rows++;
    }
    fclose(f);

    if (rows == 0) {
        fprintf(stderr, "Erreur: grille vide\n");
        return NULL;
    }

    *out_rows = rows;
    *out_cols = cols;
    return grid;
}

char **load_words(const char *filename, int *out_n) {
    FILE *f = fopen(filename, "r");
    if (!f) { perror("fopen words"); return NULL; }

    char line[MAX_LINE];
    char **words = NULL;
    int n = 0;

    while (fgets(line, sizeof(line), f)) {
        // trim newline
        char *p = line;
        while (*p && (*p == '\r' || *p == '\n')) *p++ = '\0';

        // remove spaces and tabs
        char compact[MAX_LINE];
        int j = 0;
        for (int i = 0; line[i] && line[i] != '\r' && line[i] != '\n'; ++i) {
            if (!isspace((unsigned char)line[i])) compact[j++] = (char)toupper((unsigned char)line[i]);
        }
        compact[j] = '\0';
        if (j == 0) continue; // skip empty lines

        words = realloc(words, sizeof(char*) * (n + 1));
        words[n] = malloc(j + 1);
        strcpy(words[n], compact);
        n++;
    }
    fclose(f);

    if (n == 0) {
        fprintf(stderr, "Aucun mot à chercher\n");
        return NULL;
    }

    *out_n = n;
    return words;
}

int search_from(char **grid, int rows, int cols, int r, int c, const char *word, int dr, int dc) {
    int L = (int)strlen(word);
    for (int k = 0; k < L; ++k) {
        int rr = r + k*dr;
        int cc = c + k*dc;
        if (rr < 0 || rr >= rows || cc < 0 || cc >= cols) return 0;
        if (grid[rr][cc] != word[k]) return 0;
    }
    return 1;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s grid.txt words.txt\n", argv[0]);
        return 1;
    }

    int rows, cols;
    char **grid = load_grid(argv[1], &rows, &cols);
    if (!grid) return 1;

    int n;
    char **words = load_words(argv[2], &n);
    if (!words) {
        for (int i = 0; i < rows; ++i) free(grid[i]); free(grid);
        return 1;
    }

    // 8 directions: N, NE, E, SE, S, SW, W, NW
    int dr[8] = {-1,-1, 0, 1, 1, 1, 0,-1};
    int dc[8] = { 0, 1, 1, 1, 0,-1,-1,-1};
    const char *dname[8] = {"N","NE","E","SE","S","SW","W","NW"};

    for (int w = 0; w < n; ++w) {
        const char *word = words[w];
        int L = (int)strlen(word);
        int found = 0;
        for (int i = 0; i < rows && !found; ++i) {
            for (int j = 0; j < cols && !found; ++j) {
                if (grid[i][j] != word[0]) continue;
                for (int d = 0; d < 8 && !found; ++d) {
                    if (search_from(grid, rows, cols, i, j, word, dr[d], dc[d])) {
                        printf("%s: trouve en (%d,%d) direction %s\n", word, i+1, j+1, dname[d]);
                        found = 1;
                    }
                }
            }
        }
        if (!found) printf("%s: non trouve\n", word);
    }

    // free memory
    for (int i = 0; i < rows; ++i) free(grid[i]); free(grid);
    for (int i = 0; i < n; ++i) free(words[i]); free(words);
    return 0;
}
 
