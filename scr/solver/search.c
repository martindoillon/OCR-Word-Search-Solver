// Exemple d'utilisation
int main(void) {
    // Définition de la grille
    char *my_grid[] = {
        "ABCD",
        "EFGH",
        "IJKL",
        "MNOP"
    };
    int rows = 4, cols = 4;
    char **grid = grid_from_array(my_grid, rows, cols);

    // Définition de la liste de mots
    char *words_list[] = { "ABCD", "GHI", "MNO", "JKL", "XYZ" };
    int word_count = sizeof(words_list)/sizeof(words_list[0]);
    char **words = words_from_list(words_list, word_count);

    // Recherche
    find_words(grid, rows, cols, words, word_count);

    // Libération mémoire
    free_grid(grid, rows);
    free_words(words, word_count);

    return 0;
}



"solver.c" 140L, 4226B                                                                                136,1         Bot
