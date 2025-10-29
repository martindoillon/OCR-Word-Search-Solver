#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "grid.h"

Grid *create_grid(char **data, int rows, int cols) {
    Grid *g = malloc(sizeof(Grid));
    g->rows = rows;
    g->cols = cols;
    g->data = malloc(rows * sizeof(char *));
    for (int i = 0; i < rows; i++) {
        g->data[i] = malloc(cols * sizeof(char));
        for (int j = 0; j < cols; j++) {
            g->data[i][j] = toupper(data[i][j]);
        }
    }
    return g;
}

void free_grid(Grid *g) {
    for (int i = 0; i < g->rows; i++)
        free(g->data[i]);
    free(g->data);
    free(g);
}

void print_grid(Grid *g) {
    for (int i = 0; i < g->rows; i++) {
        for (int j = 0; j < g->cols; j++) {
            printf("%c ", g->data[i][j]);
        }
        printf("\n");
    }
}

