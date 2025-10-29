#include <stdio.h>
#include <stdlib.h>
#include "grid.h"
#include "search.h"

int main() {
    // Grille corrigée où TOUS les mots existent réellement
    char *data[] = {
        "TXXXXXXCHAT",  // CHAT → O
        "IRXXXEXXXXX",  // TIGRE ↓
        "GXAXXXXXXXX",
        "RXBXOURSXXX",  // OURS → E
        "EXRXXEXXXXE",
        "XXXALEIULPC",
        "XXXOMXENITT",
        "XXCXXIOLENO",  // ECOLE → O
        "XEXXXELUIOP",  // PLUIE ↖
        "XXXXXXSUNMM"   // MONT ↑
    };

    int rows = 10;
    int cols = 11;  // ⚠️ 11 colonnes car "TXXXXXXCHAT" = 11 caractères

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

