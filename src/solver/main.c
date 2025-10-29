#include <stdio.h>
#include <stdlib.h>
#include "grid.h"
#include "search.h"

int main() {
    
    char *data[] = {
	"TXXXXCHATXX",
        "IRXXXEXXXXX",  
        "GXAXXXXXXXX",
        "RXBXOURSXXX", 
        "EXRXXEXXXXE",
        "XXXALEIULPC",
        "XXXOMXENITT",
        "XXCXXIOLENO",
        "XEXXXELUIOP",
        "XXXXXXSUNMM"
    };

    int rows = 10;
    int cols = 11;  

    Grid *g = create_grid(data, rows, cols);

    printf("=== GRILLE DE JEU ===\n");
    print_grid(g);
    printf("======================\n\n");

    char *words[] = {"chat", "tigre", "ours", "pluie", "ecole", "mont"};
    int n = sizeof(words) / sizeof(words[0]);

    printf("=== RÉSULTATS DE LA RECHERCHE ===\n");
    find_words(g, words, n);
    printf("===============================\n");

    free_grid(g);
    return 0;
}

