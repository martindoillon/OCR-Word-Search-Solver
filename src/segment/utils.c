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

Uint8 compute_otsu_threshold(unsigned int hist[256], unsigned int total)
{
    unsigned int sum = 0;
    for (int t = 0; t < 256; ++t) sum += t * hist[t];

    unsigned int sumB = 0;
    unsigned int wB = 0;
    double varMax = 0.0;
    Uint8 threshold = 128;

    for (int t = 0; t < 256; ++t)
    {
        wB += hist[t];
        if (wB == 0) continue;

        unsigned int wF = total - wB;
        if (wF == 0) break;

        sumB += t * hist[t];

        double mB = (double)sumB / wB;
        double mF = (double)(sum - sumB) / wF;
        double varBetween = (double)wB * (double)wF * (mB - mF) * (mB - mF);

        if (varBetween > varMax)
        {
            varMax = varBetween;
            threshold = (Uint8)t;
        }
    }
    return threshold;
}

SDL_Surface *make_binary(SDL_Surface *src, Uint8 *out_thresh)
{
    int w = src->w;
    int h = src->h;
    unsigned int hist[256] = {0};

    SDL_LockSurface(src);
    Uint8 *sp = (Uint8 *)src->pixels;
    int spitch = src->pitch;

    for (int y = 0; y < h; ++y)
    {
        Uint8 *row = sp + y * spitch;
        for (int x = 0; x < w; ++x)
        {
            Uint8 b = row[x * 4 + 0];
            Uint8 g = row[x * 4 + 1];
            Uint8 r = row[x * 4 + 2];
            int gray = (r * 299 + g * 587 + b * 114) / 1000;
            hist[gray]++;
        }
    }
    SDL_UnlockSurface(src);

    Uint8 thr = compute_otsu_threshold(hist, (unsigned int)w * (unsigned int)h);
    if (out_thresh) *out_thresh = thr;

    SDL_Surface *bin =
        SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!bin) return NULL;

    Uint32 white = SDL_MapRGB(bin->format, 255, 255, 255);
    Uint32 black = SDL_MapRGB(bin->format, 0, 0, 0);

    SDL_LockSurface(src);
    SDL_LockSurface(bin);

    sp = (Uint8 *)src->pixels;
    spitch = src->pitch;
    Uint8 *bp = (Uint8 *)bin->pixels;
    int bpitch = bin->pitch;

    for (int y = 0; y < h; ++y)
    {
        Uint8 *srow = sp + y * spitch;
        Uint8 *brow = bp + y * bpitch;
        for (int x = 0; x < w; ++x)
        {
            Uint8 b = srow[x * 4 + 0];
            Uint8 g = srow[x * 4 + 1];
            Uint8 r = srow[x * 4 + 2];
            int gray = (r * 299 + g * 587 + b * 114) / 1000;
            *(Uint32 *)(brow + x * 4) = (gray < thr) ? black : white;
        }
    }

    SDL_UnlockSurface(bin);
    SDL_UnlockSurface(src);

    return bin;
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

