#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "grid.h"

typedef struct {
    char direction[3];
    int dx, dy;
} Direction;

Direction dirs[] = {
    {"N", -1, 0},
    {"S", 1, 0},
    {"E", 0, 1},
    {"O", 0, -1},
    {"NE", -1, 1},
    {"NO", -1, -1},
    {"SE", 1, 1},
    {"SO", 1, -1}
};

int in_bounds(Grid *g, int x, int y) {
    return x >= 0 && x < g->rows && y >= 0 && y < g->cols;
}

int search_from(Grid *g, const char *word, int x, int y, Direction dir) {
    int len = strlen(word);
    for (int i = 0; i < len; i++) {
        int nx = x + dir.dx * i;
        int ny = y + dir.dy * i;
        if (!in_bounds(g, nx, ny) || g->data[nx][ny] != toupper(word[i]))
            return 0;
    }
    return 1;
}

void find_word(Grid *g, const char *word) {
    for (int i = 0; i < g->rows; i++) {
        for (int j = 0; j < g->cols; j++) {
            for (int d = 0; d < 8; d++) {
                if (search_from(g, word, i, j, dirs[d])) {
                    printf("%s trouvé en (%d, %d) direction %s\n", word, i, j, dirs[d].direction);
                }
            }
        }
    }
}

void find_words(Grid *g, char **words, int n) {
    for (int i = 0; i < n; i++) {
        find_word(g, words[i]);
    }
}

