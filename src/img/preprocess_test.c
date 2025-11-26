#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include "preprocess.h"

int main(int argc, char **argv)
{
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <input_image> <rotation_angle> <output_image>\n", argv[0]);
        return 1;
    }

    const char *input_path = argv[1];
    double angle = atof(argv[2]);
    const char *output_path = argv[3];

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }

    int img_flags = IMG_INIT_PNG | IMG_INIT_JPG;
    if ((IMG_Init(img_flags) & img_flags) == 0) {
        fprintf(stderr, "IMG_Init error: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Surface *img0 = IMG_Load(input_path);
    if (!img0) {
        fprintf(stderr, "IMG_Load error: %s\n", IMG_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Surface *img = SDL_ConvertSurfaceFormat(img0, SDL_PIXELFORMAT_ARGB8888, 0);
    SDL_FreeSurface(img0);
    if (!img) {
        fprintf(stderr, "ConvertSurface error: %s\n", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Surface *processed = preprocess(img, angle);
    SDL_FreeSurface(img);
    if (!processed) {
        fprintf(stderr, "preprocess failed\n");
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    if (SDL_SaveBMP(processed, output_path) != 0) {
        fprintf(stderr, "SDL_SaveBMP error: %s\n", SDL_GetError());
        SDL_FreeSurface(processed);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_FreeSurface(processed);
    IMG_Quit();
    SDL_Quit();

    printf("Saved processed image to %s\n", output_path);
    return 0;
}

