#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    int x, y;
} Point;

int read_grid(const char *path, char ***out_grid, int *rows, int *cols) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    size_t cap = 16, n = 0;
    char **grid = malloc(cap * sizeof(*grid));
    if (!grid) { fclose(f); return -2; }

    char *line = NULL;
    size_t len = 0;
    ssize_t got;
    int width = -1;

    while ((got = getline(&line, &len, f)) != -1) {
        
        while (got > 0 && (line[got-1] == '\n' || line[got-1] == '\r')) {
            line[--got] = '\0';
        }
        if (got == 0) continue;
        
        for (int i = 0; i < got; i++) {
            if (line[i] == ' ') {
                free(line);
                for (size_t k = 0; k < n; k++) free(grid[k]);
                free(grid);
                fclose(f);
                return -3;
            }
            if (!isupper((unsigned char)line[i])) {
                if (isalpha((unsigned char)line[i])) {
                    line[i] = (char)toupper((unsigned char)line[i]);
                } else { 
                    free(line);
                    for (size_t k = 0; k < n; k++) free(grid[k]);
                    free(grid);
                    fclose(f);
                    return -3;
                }
            }
        }

        if (width == -1) width = (int)got;
        if (got != width) {
            free(line);
            for (size_t k = 0; k < n; k++) free(grid[k]);
            free(grid);
            fclose(f);
            return -4;
        }

        if (n == cap) {
            cap *= 2;
            char **tmp = realloc(grid, cap * sizeof(*grid));
            if (!tmp) {
                free(line);
                for (size_t k = 0; k < n; k++) free(grid[k]);
                free(grid);
                fclose(f);
                return -2;
            }
            grid = tmp;
        }
        char *row = malloc((size_t)width + 1);
        if (!row) {
            free(line);
            for (size_t k = 0; k < n; k++) free(grid[k]);
            free(grid);
            fclose(f);
            return -2;
        }
        memcpy(row, line, (size_t)width + 1);
        grid[n++] = row;
    }

    free(line);
    fclose(f);

    if (n == 0 || width <= 0) {
        for (size_t k = 0; k < n; k++) free(grid[k]);
        free(grid);
        return -5;
    }

    *out_grid = grid;
    *rows = (int)n;
    *cols = width;
    return 0;
}

void str_uppercase(char *s) {
    for (; *s; ++s) *s = (char)toupper((unsigned char)*s);
}

int in_bounds(int x, int y, int cols, int rows) {
    return (x >= 0 && x < cols && y >= 0 && y < rows);
}

int search_word(char **grid, int rows, int cols, const char *word, Point *p0, Point *p1) {
    int L = (int)strlen(word);
    if (L == 0) return 0;
 
    const int dirs[8][2] = {
        { 1,  0}, {-1,  0}, { 0,  1}, { 0, -1},
        { 1,  1}, {-1, -1}, { 1, -1}, {-1,  1}
    };

    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            if (grid[y][x] != word[0]) continue;
            for (int d = 0; d < 8; d++) {
                int dx = dirs[d][0], dy = dirs[d][1];
                int xx = x, yy = y;
                int k;
                for (k = 1; k < L; k++) {
                    xx += dx; yy += dy;
                    if (!in_bounds(xx, yy, cols, rows)) break;
                    if (grid[yy][xx] != word[k]) break;
                }
                if (k == L) {
                    p0->x = x;  p0->y = y;
                    p1->x = xx; p1->y = yy;
                    return 1;
                }
            }
        }
    }
    return 0;
}
