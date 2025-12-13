#ifndef SOLVER_H
#define SOLVER_H

typedef struct {
    int x, y;
} Point;

#define MAX_WORDS 512
#define MAX_WORD_LEN 32


int read_grid_file(const char *path, char ***out_grid, int *rows, int *cols);

void str_uppercase(char *s);

int search_word(char **grid, int rows, int cols, const char *word, Point *p0, Point *p1);

#endif

