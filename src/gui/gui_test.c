#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include "../img/preprocess.h"
#include "layout_detect.h"
#include "cell_finder.h"
#include "wordlist_finder.h"
#include "utils.h"

#define WINDOW_TITLE "Word Search Visualizer"

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_image>\n", argv[0]);
        return 1;
    }
    const char *img_path = argv[1];

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    int img_flags = IMG_INIT_PNG | IMG_INIT_JPG;
    if ((IMG_Init(img_flags) & img_flags) == 0) {
        fprintf(stderr, "IMG_Init: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Surface *orig0 = IMG_Load(img_path);
    if (!orig0) {
        fprintf(stderr, "IMG_Load: %s\n", IMG_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_Surface *orig = SDL_ConvertSurfaceFormat(orig0, SDL_PIXELFORMAT_ARGB8888, 0);
    SDL_FreeSurface(orig0);
    if (!orig) {
        fprintf(stderr, "ConvertSurface: %s\n", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Preprocess: grayscale + linear contrast + rotation
    SDL_Surface *prep = preprocess(orig, 0.0);  // rotation = 0 for now
    if (!prep) {
        fprintf(stderr, "Preprocess failed\n");
        SDL_FreeSurface(orig);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        WINDOW_TITLE,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        prep->w, prep->h, SDL_WINDOW_SHOWN
    );
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_FreeSurface(prep);
        SDL_FreeSurface(orig);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_FreeSurface(prep);
        SDL_FreeSurface(orig);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, prep);
    SDL_FreeSurface(prep);
    if (!texture) {
        fprintf(stderr, "SDL_CreateTextureFromSurface: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_FreeSurface(orig);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Main loop
    int quit = 0;
    SDL_Event e;
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) quit = 1;
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_q) quit = 1;
            }
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);  // ~60 FPS
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_FreeSurface(orig);
    IMG_Quit();
    SDL_Quit();
    return 0;
}

