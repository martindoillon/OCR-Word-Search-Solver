#ifndef GRID_H
#define GRID_H

typedef struct {
    int rows;
    int cols;
    char **data;
} Grid;

Grid *create_grid(char **data, int rows, int cols);
void free_grid(Grid *g);
void print_grid(Grid *g);

#endif

