// wordlist_finder.c
// Read output/wordlist_roi.txt and split words into letters into output/words/

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
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

static Uint8 compute_otsu_threshold(unsigned int hist[256], unsigned int total)
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
        double varBetween = (double)wB * wF * (mB - mF) * (mB - mF);
        if (varBetween > varMax)
        {
            varMax = varBetween;
            threshold = (Uint8)t;
        }
    }
    return threshold;
}

static SDL_Surface *make_binary(SDL_Surface *src, Uint8 *out_thr)
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
            if (gray < 0) gray = 0;
            if (gray > 255) gray = 255;
            hist[gray]++;
        }
    }
    SDL_UnlockSurface(src);

    Uint8 thr = compute_otsu_threshold(hist, (unsigned int)w * (unsigned int)h);
    if (out_thr) *out_thr = thr;

    SDL_Surface *bin = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
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
            if (gray < 0) gray = 0;
            if (gray > 255) gray = 255;
            *(Uint32 *)(brow + x * 4) = (gray < thr) ? black : white;
        }
    }
    SDL_UnlockSurface(bin);
    SDL_UnlockSurface(src);
    return bin;
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

static int read_wordlist_roi(const char *path, int *lx, int *ly, int *lw, int *lh, int *word_count)
{
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    int n = fscanf(f, "%d %d %d %d %d", lx, ly, lw, lh, word_count);
    fclose(f);
    return (n == 5) ? 0 : -1;
}

static int tighten_bbox(SDL_Surface *bin, Uint32 black, int x, int y, int w, int h, SDL_Rect *out)
{
    SDL_LockSurface(bin);
    Uint8 *bp = (Uint8 *)bin->pixels;
    int pitch = bin->pitch;
    int minx = x + w, miny = y + h, maxx = x - 1, maxy = y - 1;

    for (int yy = y; yy < y + h; ++yy)
    {
        Uint32 *row = (Uint32 *)(bp + yy * pitch);
        for (int xx = x; xx < x + w; ++xx)
        {
            if (row[xx] == black)
            {
                if (xx < minx) minx = xx;
                if (xx > maxx) maxx = xx;
                if (yy < miny) miny = yy;
                if (yy > maxy) maxy = yy;
            }
        }
    }
    SDL_UnlockSurface(bin);

    if (maxx < minx || maxy < miny) return -1;
    out->x = minx; out->y = miny; out->w = maxx - minx + 1; out->h = maxy - miny + 1;
    return 0;
}

static int detect_columns(SDL_Surface *bin, Uint32 black, int lx, int ly, int lw, int lh, int *gap_s, int *gap_e)
{
    SDL_LockSurface(bin);
    Uint8 *bp = (Uint8 *)bin->pixels;
    int pitch = bin->pitch;
    int best_len = 0, best_s = -1, best_e_local = -1;
    int cur_len = 0, cur_s = -1;

    for (int x = lx; x < lx + lw; ++x)
    {
        bool hasBlack = false;
        for (int y = ly; y < ly + lh; ++y)
        {
            Uint32 px = *(Uint32 *)(bp + y * pitch + x * 4);
            if (px == black) { hasBlack = true; break; }
        }
        if (!hasBlack)
        {
            if (cur_len == 0) cur_s = x;
            cur_len++;
        }
        else
        {
            if (cur_len > best_len) { best_len = cur_len; best_s = cur_s; best_e_local = x - 1; }
            cur_len = 0;
        }
    }
    if (cur_len > best_len) { best_len = cur_len; best_s = cur_s; best_e_local = lx + lw - 1; }
    SDL_UnlockSurface(bin);

    int min_gap = lw / 30;
    if (min_gap < 5) min_gap = 5;

    if (best_len >= min_gap && best_s > lx && best_e_local < lx + lw - 1)
    {
        if (gap_s) *gap_s = best_s;
        if (gap_e) *gap_e = best_e_local;
        return 2;
    }
    return 1;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <original_image>\n", argc > 0 ? argv[0] : "wordlist_finder");
        return 1;
    }

    const char *img_path = argv[1];
    int lx, ly, lw, lh, word_count_hint;
    if (read_wordlist_roi("output/wordlist_roi.txt", &lx, &ly, &lw, &lh, &word_count_hint) != 0)
    {
        fprintf(stderr, "Error: couldn't read output/wordlist_roi.txt\n");
        return 1;
    }
    if (lw <= 0 || lh <= 0)
    {
        fprintf(stderr, "Invalid wordlist ROI\n");
        return 1;
    }

    make_dir_if_needed("output");
    make_dir_if_needed("output/words");

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

    SDL_Surface *orig0 = IMG_Load(img_path);
    if (!orig0)
    {
        fprintf(stderr, "IMG_Load: %s\n", IMG_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_Surface *orig = SDL_ConvertSurfaceFormat(orig0, SDL_PIXELFORMAT_ARGB8888, 0);
    SDL_FreeSurface(orig0);
    if (!orig)
    {
        fprintf(stderr, "ConvertSurface: %s\n", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    Uint8 thr = 0;
    SDL_Surface *bin = make_binary(orig, &thr);
    if (!bin)
    {
        fprintf(stderr, "make_binary failed\n");
        SDL_FreeSurface(orig);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    Uint32 black = SDL_MapRGB(bin->format, 0, 0, 0);

    int gap_s = -1, gap_e = -1;
    int ncols = detect_columns(bin, black, lx, ly, lw, lh, &gap_s, &gap_e);

    int rx[2], ry[2], rw[2], rh[2];
    int col_regions = (ncols == 2) ? 2 : 1;
    if (col_regions == 1)
    {
        rx[0] = lx; ry[0] = ly; rw[0] = lw; rh[0] = lh;
    }
    else
    {
        rx[0] = lx; rw[0] = gap_s - lx;
        rx[1] = gap_e + 1; rw[1] = (lx + lw) - rx[1];
        ry[0] = ry[1] = ly; rh[0] = rh[1] = lh;
    }

    SDL_Surface *overlay = SDL_ConvertSurfaceFormat(orig, SDL_PIXELFORMAT_ARGB8888, 0);
    if (!overlay)
    {
        fprintf(stderr, "overlay alloc fail\n");
        SDL_FreeSurface(bin);
        SDL_FreeSurface(orig);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    Uint32 red = SDL_MapRGB(overlay->format, 255, 0, 0);
    Uint32 green = SDL_MapRGB(overlay->format, 0, 255, 0);
    Uint32 blue = SDL_MapRGB(overlay->format, 0, 64, 255);

    SDL_LockSurface(bin);
    Uint8 *bp = (Uint8 *)bin->pixels;
    int pitch = bin->pitch;
    SDL_UnlockSurface(bin);

    int global_word_idx = 0;
    int total_letters = 0;

    for (int ci = 0; ci < col_regions; ++ci)
    {
        int cx = rx[ci], cy = ry[ci], cw = rw[ci], ch = rh[ci];

        int y = cy;
        while (y < cy + ch)
        {
            int y0 = -1;
            for (; y < cy + ch; ++y)
            {
                bool hasBlack = false;
                for (int xx = cx; xx < cx + cw; ++xx)
                {
                    Uint32 px = *(Uint32 *)(bp + y * pitch + xx * 4);
                    if (px == black) { hasBlack = true; break; }
                }
                if (hasBlack) { y0 = y; break; }
            }
            if (y0 < 0) break;

            int y1 = y0;
            for (int yy = y0 + 1; yy < cy + ch; ++yy)
            {
                bool hasBlack = false;
                for (int xx = cx; xx < cx + cw; ++xx)
                {
                    Uint32 px = *(Uint32 *)(bp + yy * pitch + xx * 4);
                    if (px == black) { hasBlack = true; break; }
                }
                if (!hasBlack) { y1 = yy - 1; y = yy; break; }
                if (yy == cy + ch - 1) { y1 = yy; y = yy + 1; }
            }

            int pad = 1;
            int ly0 = (y0 - pad < cy) ? cy : y0 - pad;
            int ly1 = (y1 + pad >= cy + ch) ? (cy + ch - 1) : y1 + pad;
            int lh2 = ly1 - ly0 + 1;

            draw_rect(overlay, cx, ly0, cw, lh2, red);

            int x = cx;
            int word_idx_local = 0;
            while (x < cx + cw)
            {
                int x0 = -1;
                for (; x < cx + cw; ++x)
                {
                    bool hasBlack = false;
                    for (int yy = ly0; yy <= ly1; ++yy)
                    {
                        Uint32 px = *(Uint32 *)(bp + yy * pitch + x * 4);
                        if (px == black) { hasBlack = true; break; }
                    }
                    if (hasBlack) { x0 = x; break; }
                }
                if (x0 < 0) break;

                int x1 = x0;
                for (int xx = x0 + 1; xx < cx + cw; ++xx)
                {
                    bool hasBlack = false;
                    for (int yy = ly0; yy <= ly1; ++yy)
                    {
                        Uint32 px = *(Uint32 *)(bp + yy * pitch + xx * 4);
                        if (px == black) { hasBlack = true; break; }
                    }
                    if (!hasBlack) { x1 = xx - 1; x = xx; break; }
                    if (xx == cx + cw - 1) { x1 = xx; x = xx + 1; }
                }

                int ww = x1 - x0 + 1;
                if (ww <= 0) continue;

                int padX = 0;
                int wx0 = (x0 - padX < cx) ? cx : x0 - padX;
                int wx1 = (x1 + padX >= cx + cw) ? (cx + cw - 1) : x1 + padX;
                int ww2 = wx1 - wx0 + 1;

                draw_rect(overlay, wx0, ly0, ww2, lh2, green);

                int col = wx0;
                int char_idx = 0;
                while (col < wx0 + ww2)
                {
                    int c0 = -1;
                    for (; col < wx0 + ww2; ++col)
                    {
                        bool hasBlack = false;
                        for (int yy = ly0; yy <= ly1; ++yy)
                        {
                            Uint32 px = *(Uint32 *)(bp + yy * pitch + col * 4);
                            if (px == black) { hasBlack = true; break; }
                        }
                        if (hasBlack) { c0 = col; break; }
                    }
                    if (c0 < 0) break;

                    int c1 = c0;
                    for (int xx = c0 + 1; xx < wx0 + ww2; ++xx)
                    {
                        bool hasBlack = false;
                        for (int yy = ly0; yy <= ly1; ++yy)
                        {
                            Uint32 px = *(Uint32 *)(bp + yy * pitch + xx * 4);
                            if (px == black) { hasBlack = true; break; }
                        }
                        if (!hasBlack) { c1 = xx - 1; col = xx; break; }
                        if (xx == wx0 + ww2 - 1) { c1 = xx; col = xx + 1; }
                    }

                    SDL_Rect tight;
                    if (tighten_bbox(bin, black, c0, ly0, c1 - c0 + 1, lh2, &tight) == 0)
                    {
                        draw_rect(overlay, tight.x, tight.y, tight.w, tight.h, blue);

                        SDL_Surface *letter = SDL_CreateRGBSurfaceWithFormat(0, tight.w, tight.h, 32, SDL_PIXELFORMAT_ARGB8888);
                        if (letter)
                        {
                            if (SDL_BlitSurface(orig, &tight, letter, NULL) == 0)
                            {
                                char name[512];
                                snprintf(name, sizeof(name), "output/words/%02d_%02d.bmp", global_word_idx, char_idx);
                                if (SDL_SaveBMP(letter, name) != 0) fprintf(stderr, "save %s: %s\n", name, SDL_GetError());
                                else total_letters++;
                            }
                            SDL_FreeSurface(letter);
                        }
                        char_idx++;
                    }
                }

                global_word_idx++;
                word_idx_local++;
            }

            // continue to next line
        }
    }

    if (SDL_SaveBMP(overlay, "output/stage_wordlist.bmp") != 0) fprintf(stderr, "save overlay: %s\n", SDL_GetError());

    SDL_FreeSurface(overlay);
    SDL_FreeSurface(bin);
    SDL_FreeSurface(orig);
    IMG_Quit();
    SDL_Quit();
    printf("Saved %d letters into output/words/ and overlay output/stage_wordlist.bmp\n", total_letters);
    return 0;
}
