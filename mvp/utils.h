#ifndef UTILS_H
#define UTILS_H

#include <SDL2/SDL.h>
#include <stdint.h>

int make_dir_if_needed(const char *path);

void draw_rect(SDL_Surface *s, int x, int y, int w, int h, Uint32 color);

#endif

