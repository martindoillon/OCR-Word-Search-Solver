#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "emnist_mlp.h"

#define IMG_SIZE 28

int preprocess_image(const char* path, float* input) {
    SDL_Surface* surface = IMG_Load(path);
    if (!surface) {
        fprintf(stderr, "Failed to load image: %s\n", path);
        return -1;
    }

    SDL_LockSurface(surface);
    int width = surface->w;
    int height = surface->h;
    uint32_t* pixels = (uint32_t*)surface->pixels;
    int pitch = surface->pitch / 4;


    if (height < 4.5f * width) {
        uint32_t white = SDL_MapRGB(surface->format, 255, 255, 255);
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (x == 0 || x == width - 1 || y == 0 || y == height - 1) {
                    pixels[y * pitch + x] = white;
                }
            }
        }
    }

    int min_x = width, min_y = height;
    int max_x = -1, max_y = -1;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint8_t r, g, b;
            SDL_GetRGB(pixels[y * pitch + x], surface->format, &r, &g, &b);
            if (r < 250 || g < 250 || b < 250) {
                if (x < min_x) min_x = x;
                if (x > max_x) max_x = x;
                if (y < min_y) min_y = y;
                if (y > max_y) max_y = y;
            }
        }
    }

    SDL_UnlockSurface(surface);

    if (max_x < min_x || max_y < min_y) {
        fprintf(stderr, "No letter found in image: %s\n", path);
        SDL_FreeSurface(surface);
        return -1;
    }

    int letter_w = max_x - min_x + 1;
    int letter_h = max_y - min_y + 1;

    SDL_Surface* letter = SDL_CreateRGBSurface(
        0, letter_w, letter_h, 32,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000
    );

    SDL_Rect src = { min_x, min_y, letter_w, letter_h };
    SDL_BlitSurface(surface, &src, letter, NULL);
    SDL_FreeSurface(surface);

    int target_size = IMG_SIZE - 10;
    float scale = (letter_w > letter_h)
        ? (float)target_size / letter_w
        : (float)target_size / letter_h;

    int new_w = (int)(letter_w * scale);
    int new_h = (int)(letter_h * scale);

    SDL_Surface* resized = SDL_CreateRGBSurface(
        0, new_w, new_h, 32,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000
    );

    SDL_Rect dst = { 0, 0, new_w, new_h };
    SDL_BlitScaled(letter, NULL, resized, &dst);
    SDL_FreeSurface(letter);

    SDL_Surface* canvas = SDL_CreateRGBSurface(
        0, IMG_SIZE, IMG_SIZE, 32,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000
    );

    SDL_FillRect(canvas, NULL, SDL_MapRGB(canvas->format, 255, 255, 255));

    SDL_Rect center = {
        (IMG_SIZE - new_w) / 2,
        (IMG_SIZE - new_h) / 2,
        new_w,
        new_h
    };

    SDL_BlitSurface(resized, NULL, canvas, &center);
    SDL_FreeSurface(resized);

    SDL_LockSurface(canvas);
    pixels = (uint32_t*)canvas->pixels;
    pitch = canvas->pitch / 4;

    for (int y = 0; y < IMG_SIZE; y++) {
        for (int x = 0; x < IMG_SIZE; x++) {
            uint8_t r, g, b;
            SDL_GetRGB(pixels[y * pitch + x], canvas->format, &r, &g, &b);
            input[y * IMG_SIZE + x] = r / 255.0f;
        }
    }

    SDL_UnlockSurface(canvas);
    SDL_FreeSurface(canvas);

    return 0;
}


char predict_letter_from_file(const char* path)
{
    float input[IMG_SIZE*IMG_SIZE];
    if (preprocess_image(path, input) != 0) {
        return '?';
    }

    MLP net;
    if (load_mlp(WEIGHTS_FILE, &net) != 0) {
        fprintf(stderr, "Failed to load weights from %s\n", WEIGHTS_FILE);
        return '?';
    }

    float hidden[HIDDEN_SIZE], output[OUTPUT_SIZE];
    forward(&net, input, hidden, output);
    int pred = argmax(output);

    return 'A' + pred;
}


#ifdef PREDICT_LETTERS_MAIN
int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s image_file\n", argv[0]);
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    float input[IMG_SIZE*IMG_SIZE];
    if (preprocess_image(argv[1], input) != 0) {
        SDL_Quit();
        return 1;
    }

    MLP net;
    if (load_mlp(WEIGHTS_FILE, &net) != 0) {
        fprintf(stderr, "Failed to load weights from %s\n", WEIGHTS_FILE);
        SDL_Quit();
        return 1;
    }

    float hidden[HIDDEN_SIZE], output[OUTPUT_SIZE];
    forward(&net, input, hidden, output);
    int pred = argmax(output);

    printf("Predicted letter: %c\n", 'A' + pred);

    SDL_Quit();
    return 0;
}
#endif

