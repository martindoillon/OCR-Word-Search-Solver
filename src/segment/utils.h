#ifndef UTILS_H
#define UTILS_H

#include <SDL2/SDL.h>
#include <stdint.h>

int make_dir_if_needed(const char *path);

Uint8 compute_otsu_threshold(unsigned int hist[256], unsigned int total);

SDL_Surface *make_binary(SDL_Surface *src, Uint8 *out_thresh);

void draw_rect(SDL_Surface *s, int x, int y, int w, int h, Uint32 color);

#endif

