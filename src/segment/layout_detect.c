// detect_layout.c
// Detect grid and word-list ROIs, write grid_roi.txt and wordlist_roi.txt

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "layout_detect.h"
#include "utils.h"

//
//  MAIN -> FUNCTION
//
int detect_layout(const char *input)
{
    if (!input)
    {
        fprintf(stderr, "detect_layout: missing input image\n");
        return 1;
    }

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

    SDL_Surface *orig0 = IMG_Load(input);
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

    int w = bin->w;
    int h = bin->h;
    Uint32 black = SDL_MapRGB(bin->format, 0, 0, 0);

    int *col_sum = (int *)calloc(w, sizeof(int));
    int *row_sum = (int *)calloc(h, sizeof(int));
    if (!col_sum || !row_sum)
    {
        fprintf(stderr, "oom\n");
        free(col_sum);
        free(row_sum);
        SDL_FreeSurface(bin);
        SDL_FreeSurface(orig);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_LockSurface(bin);
    Uint8 *bp = (Uint8 *)bin->pixels;
    int pitch = bin->pitch;
    int min_x = w, min_y = h, max_x = -1, max_y = -1;

    for (int y = 0; y < h; ++y)
    {
        Uint32 *row = (Uint32 *)(bp + y * pitch);
        for (int x = 0; x < w; ++x)
        {
            if (row[x] == black)
            {
                col_sum[x]++;
                row_sum[y]++;
                if (x < min_x) min_x = x;
                if (x > max_x) max_x = x;
                if (y < min_y) min_y = y;
                if (y > max_y) max_y = y;
            }
        }
    }
    SDL_UnlockSurface(bin);

    if (max_x < 0 || max_y < 0)
    {
        min_x = min_y = 0;
        max_x = max_y = 0;
    }

    int best_run_col = 0, best_col_start = -1, best_col_end = -1;
    int curr_run = 0, curr_start = -1;
    for (int cx = min_x; cx <= max_x; ++cx)
    {
        if (col_sum[cx] == 0)
        {
            if (curr_run == 0) curr_start = cx;
            curr_run++;
        }
        else
        {
            if (curr_run > best_run_col)
            {
                best_run_col = curr_run;
                best_col_start = curr_start;
                best_col_end = cx - 1;
            }
            curr_run = 0;
        }
    }
    if (curr_run > best_run_col)
    {
        best_run_col = curr_run;
        best_col_start = curr_start;
        best_col_end = max_x;
    }

    int best_run_row = 0, best_row_start = -1, best_row_end = -1;
    curr_run = 0; curr_start = -1;
    for (int ry = min_y; ry <= max_y; ++ry)
    {
        if (row_sum[ry] == 0)
        {
            if (curr_run == 0) curr_start = ry;
            curr_run++;
        }
        else
        {
            if (curr_run > best_run_row)
            {
                best_run_row = curr_run;
                best_row_start = curr_start;
                best_row_end = ry - 1;
            }
            curr_run = 0;
        }
    }
    if (curr_run > best_run_row)
    {
        best_run_row = curr_run;
        best_row_start = curr_start;
        best_row_end = max_y;
    }

    bool split_vertical = (best_run_col >= 1);
    bool split_horizontal = (best_run_row >= 1);
    if (split_vertical && split_horizontal)
    {
        if (best_run_col >= best_run_row) split_horizontal = false;
        else split_vertical = false;
    }

    int grid_x = 0, grid_y = 0, grid_w = 0, grid_h = 0;
    int list_x = 0, list_y = 0, list_w = 0, list_h = 0;
    int grid_cols = 0, grid_rows = 0;
    int word_count = 0;

    if (!split_vertical && !split_horizontal)
    {
        grid_x = min_x;
        grid_y = min_y;
        grid_w = max_x - min_x + 1;
        grid_h = max_y - min_y + 1;
        list_x = list_y = list_w = list_h = 0;
    }
    else if (split_vertical)
    {
        int gap_s = best_col_start;
        int gap_e = best_col_end;
        int c1_x1 = min_x, c1_x2 = gap_s - 1;
        int c2_x1 = gap_e + 1, c2_x2 = max_x;
        int c1_y1 = h, c1_y2 = -1, c2_y1 = h, c2_y2 = -1;

        SDL_LockSurface(bin);
        bp = (Uint8 *)bin->pixels;
        pitch = bin->pitch;
        for (int y = min_y; y <= max_y; ++y)
        {
            Uint32 *row = (Uint32 *)(bp + y * pitch);
            if (c1_x1 <= c1_x2)
            {
                for (int x = c1_x1; x <= c1_x2; ++x)
                {
                    if (row[x] == black)
                    {
                        if (y < c1_y1) c1_y1 = y;
                        if (y > c1_y2) c1_y2 = y;
                        break;
                    }
                }
            }
            if (c2_x1 <= c2_x2)
            {
                for (int x = c2_x1; x <= c2_x2; ++x)
                {
                    if (row[x] == black)
                    {
                        if (y < c2_y1) c2_y1 = y;
                        if (y > c2_y2) c2_y2 = y;
                        break;
                    }
                }
            }
        }
        SDL_UnlockSurface(bin);

        long c1_black = 0, c2_black = 0;
        if (c1_y2 >= c1_y1 && c1_x2 >= c1_x1)
        {
            SDL_LockSurface(bin);
            bp = (Uint8 *)bin->pixels;
            pitch = bin->pitch;
            for (int y = c1_y1; y <= c1_y2; ++y)
            {
                Uint32 *row = (Uint32 *)(bp + y * pitch);
                for (int x = c1_x1; x <= c1_x2; ++x) if (row[x] == black) c1_black++;
            }
            SDL_UnlockSurface(bin);
        }
        if (c2_y2 >= c2_y1 && c2_x2 >= c2_x1)
        {
            SDL_LockSurface(bin);
            bp = (Uint8 *)bin->pixels;
            pitch = bin->pitch;
            for (int y = c2_y1; y <= c2_y2; ++y)
            {
                Uint32 *row = (Uint32 *)(bp + y * pitch);
                for (int x = c2_x1; x <= c2_x2; ++x) if (row[x] == black) c2_black++;
            }
            SDL_UnlockSurface(bin);
        }

        bool c1grid = (c1_black >= c2_black);
        if (c1grid)
        {
            grid_x = c1_x1; grid_y = c1_y1; grid_w = c1_x2 - c1_x1 + 1; grid_h = c1_y2 - c1_y1 + 1;
            list_x = c2_x1; list_y = c2_y1; list_w = c2_x2 - c2_x1 + 1; list_h = c2_y2 - c2_y1 + 1;
        }
        else
        {
            grid_x = c2_x1; grid_y = c2_y1; grid_w = c2_x2 - c2_x1 + 1; grid_h = c2_y2 - c2_y1 + 1;
            list_x = c1_x1; list_y = c1_y1; list_w = c1_x2 - c1_x1 + 1; list_h = c1_y2 - c1_y1 + 1;
        }
    }
    else
    {
        int gap_s = best_row_start;
        int gap_e = best_row_end;
        int c1_y1 = min_y, c1_y2 = gap_s - 1;
        int c2_y1 = gap_e + 1, c2_y2 = max_y;
        int c1_x1 = w, c1_x2 = -1, c2_x1 = w, c2_x2 = -1;

        SDL_LockSurface(bin);
        bp = (Uint8 *)bin->pixels;
        pitch = bin->pitch;
        for (int x = min_x; x <= max_x; ++x)
        {
            for (int y = c1_y1; y <= c1_y2; ++y)
            {
                Uint32 px = *(Uint32 *)(bp + y * pitch + x * 4);
                if (px == black)
                {
                    if (x < c1_x1) c1_x1 = x;
                    if (x > c1_x2) c1_x2 = x;
                    break;
                }
            }
            for (int y = c2_y1; y <= c2_y2; ++y)
            {
                Uint32 px = *(Uint32 *)(bp + y * pitch + x * 4);
                if (px == black)
                {
                    if (x < c2_x1) c2_x1 = x;
                    if (x > c2_x2) c2_x2 = x;
                    break;
                }
            }
        }
        SDL_UnlockSurface(bin);

        long c1_black = 0, c2_black = 0;
        if (c1_x2 >= c1_x1 && c1_y2 >= c1_y1)
        {
            SDL_LockSurface(bin);
            bp = (Uint8 *)bin->pixels;
            pitch = bin->pitch;
            for (int y = c1_y1; y <= c1_y2; ++y)
            {
                Uint32 *row = (Uint32 *)(bp + y * pitch);
                for (int x = c1_x1; x <= c1_x2; ++x) if (row[x] == black) c1_black++;
            }
            SDL_UnlockSurface(bin);
        }
        if (c2_x2 >= c2_x1 && c2_y2 >= c2_y1)
        {
            SDL_LockSurface(bin);
            bp = (Uint8 *)bin->pixels;
            pitch = bin->pitch;
            for (int y = c2_y1; y <= c2_y2; ++y)
            {
                Uint32 *row = (Uint32 *)(bp + y * pitch);
                for (int x = c2_x1; x <= c2_x2; ++x) if (row[x] == black) c2_black++;
            }
            SDL_UnlockSurface(bin);
        }

        bool c1grid = (c1_black >= c2_black);
        if (c1grid)
        {
            grid_x = c1_x1; grid_y = c1_y1; grid_w = c1_x2 - c1_x1 + 1; grid_h = c1_y2 - c1_y1 + 1;
            list_x = c2_x1; list_y = c2_y1; list_w = c2_x2 - c2_x1 + 1; list_h = c2_y2 - c2_y1 + 1;
        }
        else
        {
            grid_x = c2_x1; grid_y = c2_y1; grid_w = c2_x2 - c2_x1 + 1; grid_h = c2_y2 - c2_y1 + 1;
            list_x = c1_x1; list_y = c1_y1; list_w = c1_x2 - c1_x1 + 1; list_h = c1_y2 - c1_y1 + 1;
        }
    }

    // Determine grid rows/cols by projection inside grid ROI
    if (grid_w > 0 && grid_h > 0)
    {
        int *gcol = (int *)calloc(grid_w, sizeof(int));
        int *grow = (int *)calloc(grid_h, sizeof(int));
        if (gcol && grow)
        {
            SDL_LockSurface(bin);
            bp = (Uint8 *)bin->pixels;
            pitch = bin->pitch;
            for (int y = grid_y; y < grid_y + grid_h; ++y)
            {
                Uint32 *row = (Uint32 *)(bp + y * pitch);
                for (int x = grid_x; x < grid_x + grid_w; ++x)
                {
                    if (row[x] == black) { gcol[x - grid_x]++; grow[y - grid_y]++; }
                }
            }
            SDL_UnlockSurface(bin);

            int vlines = 0;
            bool inCluster = false;
            for (int j = 0; j < grid_w; ++j)
            {
                double fill = (double)gcol[j] / (double)grid_h;
                if (fill > 0.9)
                {
                    if (!inCluster) { vlines++; inCluster = true; }
                }
                else inCluster = false;
            }

            int hlines = 0;
            inCluster = false;
            for (int i = 0; i < grid_h; ++i)
            {
                double fill = (double)grow[i] / (double)grid_w;
                if (fill > 0.9)
                {
                    if (!inCluster) { hlines++; inCluster = true; }
                }
                else inCluster = false;
            }

            if (vlines >= 2 && hlines >= 2)
            {
                grid_cols = vlines - 1;
                grid_rows = hlines - 1;
            }
            else
            {
                grid_cols = 0;
                inCluster = false;
                for (int j = 0; j < grid_w; ++j)
                {
                    if (gcol[j] > 0)
                    {
                        if (!inCluster) { grid_cols++; inCluster = true; }
                    }
                    else inCluster = false;
                }
                grid_rows = 0;
                inCluster = false;
                for (int i = 0; i < grid_h; ++i)
                {
                    if (grow[i] > 0)
                    {
                        if (!inCluster) { grid_rows++; inCluster = true; }
                    }
                    else inCluster = false;
                }
            }
        }
        free(gcol);
        free(grow);
    }

    // Count words
    if (list_w > 0 && list_h > 0)
    {
        SDL_LockSurface(bin);
        bp = (Uint8 *)bin->pixels;
        pitch = bin->pitch;

        int best_gap_len = 0, gap_s = -1, gap_e = -1;
        int cur_len = 0, cur_s = -1;
        for (int x = list_x; x < list_x + list_w; ++x)
        {
            bool colHasBlack = false;
            for (int y = list_y; y < list_y + list_h; ++y)
            {
                Uint32 px = *(Uint32 *)(bp + y * pitch + x * 4);
                if (px == black) { colHasBlack = true; break; }
            }
            if (!colHasBlack)
            {
                if (cur_len == 0) cur_s = x;
                cur_len++;
            }
            else
            {
                if (cur_len > best_gap_len) { best_gap_len = cur_len; gap_s = cur_s; gap_e = x - 1; }
                cur_len = 0;
            }
        }
        if (cur_len > best_gap_len) { best_gap_len = cur_len; gap_s = cur_s; gap_e = list_x + list_w - 1; }

        int min_gap = list_w / 30;
        if (min_gap < 5) min_gap = 5;
        int columns = 1;
        if (best_gap_len >= min_gap && gap_s > list_x && gap_e < list_x + list_w - 1) columns = 2;

        int count_lines = 0;
        bool inLine = false;
        for (int y = list_y; y < list_y + list_h; ++y)
        {
            bool rowHasBlack = false;
            Uint32 *row = (Uint32 *)(bp + y * pitch);
            for (int x = list_x; x < list_x + list_w; ++x)
            {
                if (row[x] == black) { rowHasBlack = true; break; }
            }
            if (rowHasBlack)
            {
                if (!inLine) { count_lines++; inLine = true; }
            }
            else inLine = false;
        }
        if (columns == 1) word_count = count_lines;
        else
        {
            int left_x = list_x, left_w = gap_s - list_x;
            int right_x = gap_e + 1, right_w = list_x + list_w - right_x;
            int left_count = 0, right_count = 0;
            inLine = false;
            for (int y = list_y; y < list_y + list_h; ++y)
            {
                bool leftHas = false;
                Uint32 *row = (Uint32 *)(bp + y * pitch);
                for (int x = left_x; x < left_x + left_w; ++x) if (row[x] == black) { leftHas = true; break; }
                if (leftHas)
                {
                    if (!inLine) { left_count++; inLine = true; }
                }
                else inLine = false;
            }
            inLine = false;
            for (int y = list_y; y < list_y + list_h; ++y)
            {
                bool rightHas = false;
                Uint32 *row = (Uint32 *)(bp + y * pitch);
                for (int x = right_x; x < right_x + right_w; ++x) if (row[x] == black) { rightHas = true; break; }
                if (rightHas)
                {
                    if (!inLine) { right_count++; inLine = true; }
                }
                else inLine = false;
            }
            word_count = left_count + right_count;
        }

        SDL_UnlockSurface(bin);
    }

    make_dir_if_needed("output");
    make_dir_if_needed("output/grid");
    make_dir_if_needed("output/words");

    FILE *fg = fopen("output/grid_roi.txt", "w");
    if (fg)
    {
        fprintf(fg, "%d %d %d %d %d %d\n", grid_x, grid_y, grid_w, grid_h, grid_cols, grid_rows);
        fclose(fg);
    }
    else fprintf(stderr, "Failed to write output/grid_roi.txt\n");

    FILE *fw = fopen("output/wordlist_roi.txt", "w");
    if (fw)
    {
        fprintf(fw, "%d %d %d %d %d\n", list_x, list_y, list_w, list_h, word_count);
        fclose(fw);
    }
    else fprintf(stderr, "Failed to write output/wordlist_roi.txt\n");

    if (SDL_SaveBMP(bin, "output/stage_bin.bmp") != 0) fprintf(stderr, "save bin: %s\n", SDL_GetError());
    Uint32 red = SDL_MapRGB(orig->format, 255, 0, 0);
    Uint32 blue = SDL_MapRGB(orig->format, 0, 0, 255);
    draw_rect(orig, grid_x, grid_y, grid_w, grid_h, red);
    if (list_w > 0 && list_h > 0) draw_rect(orig, list_x, list_y, list_w, list_h, blue);
    if (SDL_SaveBMP(orig, "output/stage_layout.bmp") != 0) fprintf(stderr, "save layout: %s\n", SDL_GetError());

    free(col_sum);
    free(row_sum);
    SDL_FreeSurface(bin);
    SDL_FreeSurface(orig);
    IMG_Quit();
    SDL_Quit();
    return 0;
}

