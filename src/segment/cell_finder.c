// cell_finder.c
// Read output/grid_roi.txt and split grid into per-cell BMPs into output/grid/

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

static int make_dir_if_needed(const char *path)
{
    int r = mkdir(path, 0775);
    if (r == 0 || errno == EEXIST) return 0;
    perror("mkdir");
    return -1;
}

static void draw_rect(SDL_Surface *s, int x, int y, int w, int h, Uint32 color)
{
    if (!s || w <= 0 || h <= 0) return;
    Uint8 *p = (Uint8 *)s->pixels;
    int pitch = s->pitch;

    if (y >= 0 && y < s->h)
    {
        Uint32 *row = (Uint32 *)(p + y * pitch);
        for (int i = x; i < x + w && i < s->w; ++i) if (i >= 0) row[i] = color;
    }
    int by = y + h - 1;
    if (by >= 0 && by < s->h)
    {
        Uint32 *row = (Uint32 *)(p + by * pitch);
        for (int i = x; i < x + w && i < s->w; ++i) if (i >= 0) row[i] = color;
    }
    if (x >= 0 && x < s->w)
    {
        for (int j = y; j < y + h && j < s->h; ++j) if (j >= 0)
        {
            Uint32 *row = (Uint32 *)(p + j * pitch);
            row[x] = color;
        }
    }
    int rx = x + w - 1;
    if (rx >= 0 && rx < s->w)
    {
        for (int j = y; j < y + h && j < s->h; ++j) if (j >= 0)
        {
            Uint32 *row = (Uint32 *)(p + j * pitch);
            row[rx] = color;
        }
    }
}

static int read_grid_roi(const char *path, int *gx, int *gy, int *gw, int *gh, int *cols, int *rows)
{
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    int n = fscanf(f, "%d %d %d %d %d %d", gx, gy, gw, gh, cols, rows);
    fclose(f);
    return (n == 6) ? 0 : -1;
}

static inline int split_round(int total, int parts, int c)
{
    return (c * total + parts / 2) / parts;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <original_image>\n", argc > 0 ? argv[0] : "cell_finder");
        return 1;
    }

    const char *img_path = argv[1];
    int gx, gy, gw, gh, cols, rows;
    if (read_grid_roi("output/grid_roi.txt", &gx, &gy, &gw, &gh, &cols, &rows) != 0)
    {
        fprintf(stderr, "Error: couldn't read output/grid_roi.txt\n");
        return 1;
    }
    if (cols <= 0 || rows <= 0 || gw <= 0 || gh <= 0)
    {
        fprintf(stderr, "Invalid grid ROI or size\n");
        return 1;
    }

    make_dir_if_needed("output");
    make_dir_if_needed("output/grid");

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    int flags = IMG_INIT_PNG | IMG_INIT_JPG;
    if ((IMG_Init(flags) & flags) == 0)
    {
        fprintf(stderr, "IMG_Init: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Surface *src0 = IMG_Load(img_path);
    if (!src0)
    {
        fprintf(stderr, "IMG_Load: %s\n", IMG_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_Surface *src = SDL_ConvertSurfaceFormat(src0, SDL_PIXELFORMAT_ARGB8888, 0);
    SDL_FreeSurface(src0);
    if (!src)
    {
        fprintf(stderr, "ConvertSurface: %s\n", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    int *xcuts = (int *)malloc((cols + 1) * sizeof(int));
    int *ycuts = (int *)malloc((rows + 1) * sizeof(int));
    if (!xcuts || !ycuts)
    {
        fprintf(stderr, "oom\n");
        free(xcuts);
        free(ycuts);
        SDL_FreeSurface(src);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    for (int c = 0; c <= cols; ++c) xcuts[c] = gx + split_round(gw, cols, c);
    for (int r = 0; r <= rows; ++r) ycuts[r] = gy + split_round(gh, rows, r);

    SDL_Surface *overlay = SDL_ConvertSurfaceFormat(src, SDL_PIXELFORMAT_ARGB8888, 0);
    if (!overlay)
    {
        fprintf(stderr, "overlay alloc fail\n");
        free(xcuts);
        free(ycuts);
        SDL_FreeSurface(src);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    Uint32 red = SDL_MapRGB(overlay->format, 255, 0, 0);

    for (int r = 0; r < rows; ++r)
    {
        for (int c = 0; c < cols; ++c)
        {
            int x0 = xcuts[c];
            int x1 = xcuts[c + 1];
            int y0 = ycuts[r];
            int y1 = ycuts[r + 1];
            int cw = x1 - x0;
            int ch = y1 - y0;
            if (cw <= 0 || ch <= 0) continue;

            int margin = 0;
            if (cw >= 8 && ch >= 8) margin = 2;
            else if (cw >= 5 && ch >= 5) margin = 1;

            int sx = x0 + margin;
            int sy = y0 + margin;
            int sw = cw - 2 * margin;
            int sh = ch - 2 * margin;
            if (sw <= 0 || sh <= 0) { sx = x0; sy = y0; sw = cw; sh = ch; }

            SDL_Rect srcR = { sx, sy, sw, sh };
            SDL_Surface *cell = SDL_CreateRGBSurfaceWithFormat(0, sw, sh, 32, SDL_PIXELFORMAT_ARGB8888);
            if (!cell) continue;
            if (SDL_BlitSurface(src, &srcR, cell, NULL) != 0)
            {
                SDL_FreeSurface(cell);
                continue;
            }

            char fname[512];
            snprintf(fname, sizeof(fname), "output/grid/%02d_%02d.bmp", c, r);
            if (SDL_SaveBMP(cell, fname) != 0) fprintf(stderr, "save %s: %s\n", fname, SDL_GetError());
            SDL_FreeSurface(cell);

            draw_rect(overlay, x0, y0, cw, ch, red);
        }
    }

    draw_rect(overlay, gx, gy, gw, gh, red);
    if (SDL_SaveBMP(overlay, "output/stage_cells.bmp") != 0) fprintf(stderr, "save overlay: %s\n", SDL_GetError());

    free(xcuts);
    free(ycuts);
    SDL_FreeSurface(overlay);
    SDL_FreeSurface(src);
    IMG_Quit();
    SDL_Quit();
    printf("Saved grid cells to output/grid/ and overlay output/stage_cells.bmp\n");
    return 0;
}
