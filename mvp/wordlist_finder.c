#include "wordlist_finder.h"
#include "utils.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
    int x, y, w, h;
} WordRect;

typedef struct {
    int x, y, w, h;
} BBox;

typedef struct {
    SDL_Rect rect;
    int orig_idx;
} LetterEntry;

static int cmp_letter_x(const void *a, const void *b)
{
    SDL_Rect ra = ((LetterEntry*)a)->rect;
    SDL_Rect rb = ((LetterEntry*)b)->rect;
    if (ra.x < rb.x) return -1;
    if (ra.x > rb.x) return 1;
    return 0;
}


static void flood8(SDL_Surface *img, Uint32 black,
                   int x, int y, bool *visited, BBox *bb)
{
    int w = img->w, h = img->h;
    Uint32 *px = (Uint32*)img->pixels;

    int cap = 1024, sz = 0;
    int (*stack)[2] = malloc(cap * sizeof(int[2]));

    #define PUSH(A,B) \
        if (sz == cap) { \
            cap *= 2; \
            stack = realloc(stack, cap * sizeof(int[2])); \
        } \
        stack[sz][0] = (A); \
        stack[sz][1] = (B); \
        sz++;

    PUSH(x,y);
    visited[y*w + x] = true;

    bb->x = bb->w = x;
    bb->y = bb->h = y;

    while (sz > 0) {
        sz--;
        int cx = stack[sz][0], cy = stack[sz][1];

        if (cx < bb->x) bb->x = cx;
        if (cy < bb->y) bb->y = cy;
        if (cx > bb->w) bb->w = cx;
        if (cy > bb->h) bb->h = cy;

        for (int dy=-1; dy<=1; dy++)
            for (int dx=-1; dx<=1; dx++) {
                if (dx==0 && dy==0) continue;
                int nx = cx+dx, ny = cy+dy;
                if (nx<0 || ny<0 || nx>=w || ny>=h) continue;

                int idx = ny*w + nx;
                if (!visited[idx] && px[idx] == black) {
                    visited[idx] = true;
                    PUSH(nx, ny);
                }
            }
    }

    free(stack);

    bb->w = bb->w - bb->x + 1;
    bb->h = bb->h - bb->y + 1;
}

static int read_wordlist_roi(const char *path, int *lx, int *ly, int *lw, int *lh, int *word_count)
{
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    int n = fscanf(f, "%d %d %d %d %d", lx, ly, lw, lh, word_count);
    fclose(f);
    return (n == 5) ? 0 : -1;
}

static int detect_columns(SDL_Surface *orig, Uint32 black, int lx, int ly, int lw, int lh, int *gap_s, int *gap_e)
{
    SDL_LockSurface(orig);
    Uint8 *bp = (Uint8 *)orig->pixels;
    int pitch = orig->pitch;
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
    SDL_UnlockSurface(orig);

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

int extract_wordlist(const char *img_path)
{
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
        return 1;
    }

    SDL_Surface *orig0 = IMG_Load(img_path);
    if (!orig0)
    {
        fprintf(stderr, "IMG_Load: %s\n", IMG_GetError());
        return 1;
    }
    SDL_Surface *orig = SDL_ConvertSurfaceFormat(orig0, SDL_PIXELFORMAT_ARGB8888, 0);
    SDL_FreeSurface(orig0);
    if (!orig)
    {
        fprintf(stderr, "ConvertSurface: %s\n", SDL_GetError());
        return 1;
    }

    Uint32 black = SDL_MapRGB(orig->format, 0, 0, 0);

    int gap_s = -1, gap_e = -1;
    int ncols = detect_columns(orig, black, lx, ly, lw, lh, &gap_s, &gap_e);

    // compute column regions
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
        SDL_FreeSurface(orig);
        return 1;
    }

    Uint32 red = SDL_MapRGB(overlay->format, 255, 0, 0);
    Uint32 green = SDL_MapRGB(overlay->format, 0, 255, 0);
    //Uint32 blue = SDL_MapRGB(overlay->format, 0, 64, 255);

    SDL_LockSurface(orig);
    Uint8 *bp = (Uint8 *)orig->pixels;
    int pitch = orig->pitch;
    SDL_UnlockSurface(orig);

    int total_letters = 0;

    WordRect *word_rects = malloc(word_count_hint * sizeof(WordRect));
    int rect_count = 0;

    // 1. Détection des lignes (rectangles rouges)
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

            draw_rect(overlay, lx, ly0, lw, lh2, red);

            if (rect_count < word_count_hint)
            {
                word_rects[rect_count].x = cx;
                word_rects[rect_count].y = ly0;
                word_rects[rect_count].w = cw;
                word_rects[rect_count].h = lh2;
                rect_count++;
            }
        }
    }
	// 2. Letter detection and extraction with conditional vertical splitting
	for (int ri = 0; ri < rect_count; ++ri)
	{
	    SDL_Rect R = { lx, word_rects[ri].y, lw, word_rects[ri].h };
	    draw_rect(overlay, R.x, R.y, R.w, R.h, green);

	    int W = orig->w;
	    Uint32 *px = (Uint32*)orig->pixels;

	    bool *visited = calloc(W * orig->h, sizeof(bool));
	    //int char_idx = 0;

	    // Collect all letters for this word first
	    LetterEntry *letters = malloc(1024 * sizeof(LetterEntry));
	    int letter_count = 0;

	    for (int y = R.y; y < R.y + R.h; ++y)
	    {
		for (int x = R.x; x < R.x + R.w; ++x)
		{
		    int idx = y * W + x;
		    if (visited[idx]) continue;

		    if (px[idx] == black)
		    {
		        BBox bb;
		        flood8(orig, black, x, y, visited, &bb);

		        if (bb.w < 2 || bb.h < 3) continue;

		        // Handle wide blobs (vertical splitting)
		        if (bb.w > 1.3 * bb.h)
		        {
		            int *hist = calloc(bb.w, sizeof(int));
		            for (int xx = bb.x; xx < bb.x + bb.w; xx++)
		                for (int yy = bb.y; yy < bb.y + bb.h; yy++)
		                    if (px[yy * W + xx] == black)
		                        hist[xx - bb.x]++;

		            int split_start = bb.x;
		            for (int i = 1; i < bb.w - 1; i++)
		            {
		                if (hist[i] <= 2)
		                {
		                    int split_x = bb.x + i;
		                    int w_seg = split_x - split_start;
		                    if (w_seg > 1)
		                    {
		                        SDL_Rect seg = { split_start, bb.y, w_seg, bb.h };
		                        letters[letter_count].rect = seg;
		                        letters[letter_count].orig_idx = letter_count;
		                        letter_count++;
		                    }
		                    split_start = split_x;
		                }
		            }

		            int w_last = bb.x + bb.w - split_start;
		            if (w_last > 1)
		            {
		                SDL_Rect last = { split_start, bb.y, w_last, bb.h };
		                letters[letter_count].rect = last;
		                letters[letter_count].orig_idx = letter_count;
		                letter_count++;
		            }

		            free(hist);
		        }
		        else
		        {
		            SDL_Rect tight = { bb.x, bb.y, bb.w, bb.h };
		            letters[letter_count].rect = tight;
		            letters[letter_count].orig_idx = letter_count;
		            letter_count++;
		        }
		    }
		}
	    }

	    // Sort letters left-to-right
	    qsort(letters, letter_count, sizeof(LetterEntry), cmp_letter_x);

	    // Save letters in left-to-right order
	    for (int i = 0; i < letter_count; i++)
	    {
		SDL_Rect Rletter = letters[i].rect;
		SDL_Surface *letter = SDL_CreateRGBSurfaceWithFormat(0, Rletter.w, Rletter.h, 32, SDL_PIXELFORMAT_ARGB8888);
		if (letter && SDL_BlitSurface(orig, &Rletter, letter, NULL) == 0)
		{
		    char name[512];
		    snprintf(name, sizeof(name), "output/words/%02d_%02d.bmp", ri, i);
		    SDL_SaveBMP(letter, name);
		    total_letters++;
		}
		if (letter) SDL_FreeSurface(letter);
	    }

	    free(letters);
	    free(visited);
	}

    free(word_rects);

    if (SDL_SaveBMP(overlay, "output/stage_wordlist.bmp") != 0)
        fprintf(stderr, "save overlay: %s\n", SDL_GetError());

    SDL_FreeSurface(overlay);
    SDL_FreeSurface(orig);

    printf("Saved %d letters into output/words/ and overlay output/stage_wordlist.bmp\n", total_letters);
    return 0;
}

