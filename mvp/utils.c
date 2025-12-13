#include "utils.h"
#include <SDL2/SDL.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

int make_dir_if_needed(const char *path)
{
    int r = mkdir(path, 0775);
    if (r == 0 || errno == EEXIST) return 0;
    perror("mkdir");
    return -1;
}

void draw_rect(SDL_Surface *s, int x, int y, int w, int h, Uint32 color)
{
    if (!s || w <= 0 || h <= 0) return;

    Uint8 *p = (Uint8 *)s->pixels;
    int pitch = s->pitch;

    if (y >= 0 && y < s->h)
    {
        Uint32 *row = (Uint32 *)(p + y * pitch);
        for (int i = x; i < x + w && i < s->w; ++i)
            if (i >= 0) row[i] = color;
    }

    int by = y + h - 1;
    if (by >= 0 && by < s->h)
    {
        Uint32 *row = (Uint32 *)(p + by * pitch);
        for (int i = x; i < x + w && i < s->w; ++i)
            if (i >= 0) row[i] = color;
    }

    if (x >= 0 && x < s->w)
    {
        for (int j = y; j < y + h && j < s->h; ++j)
            if (j >= 0)
            {
                Uint32 *row = (Uint32 *)(p + j * pitch);
                row[x] = color;
            }
    }

    int rx = x + w - 1;
    if (rx >= 0 && rx < s->w)
    {
        for (int j = y; j < y + h && j < s->h; ++j)
            if (j >= 0)
            {
                Uint32 *row = (Uint32 *)(p + j * pitch);
                row[rx] = color;
            }
    }
}

