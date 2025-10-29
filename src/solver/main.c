#include <stdio.h>
#include "grid.h"
#include "search.h"

int main() {
    char *data[] = {
        "CHAT",
        "ARBRE",
        "TORTU",
        "SERPE",
        "TIGRE"
    };

    int rows = 5;
    int cols = 5;

    Grid *g = create_grid(data, rows, cols);
    print_grid(g);

    char *words[] = {"chat", "tigre", "arbre", "tortue"};
    int n = 4;

    printf("\nRésultats de la recherche:\n");
    find_words(g, words, n);

    free_grid(g);
    return 0;
}

