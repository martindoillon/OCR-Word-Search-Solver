#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Function to compute Otsu's threshold on a grayscale histogram
static Uint8 compute_otsu_threshold(unsigned int hist[256], unsigned int total_pixels) {
    unsigned int sum = 0;
    for (int t = 0; t < 256; ++t) {
        sum += t * hist[t];
    }
    unsigned int sumB = 0;
    unsigned int wB = 0;
    unsigned int wF = 0;
    double varMax = 0.0;
    Uint8 threshold = 128;
    for (int t = 0; t < 256; ++t) {
        wB += hist[t];
        if (wB == 0) continue;
        wF = total_pixels - wB;
        if (wF == 0) break;
        sumB += t * hist[t];
        double mB = (double)sumB / wB;
        double mF = (double)(sum - sumB) / wF;
        double varBetween = (double)wB * wF * (mB - mF) * (mB - mF);
        if (varBetween > varMax) {
            varMax = varBetween;
            threshold = (Uint8)t;
        }
    }
    return threshold;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_image>\n", argc > 0 ? argv[0] : "program");
        return 1;
    }
    const char *input_path = argv[1];
    // Initialize SDL2
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }
    int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
    if ((IMG_Init(imgFlags) & imgFlags) == 0) {
        fprintf(stderr, "SDL_image Init Error: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    // Load the input image
    SDL_Surface *orig = IMG_Load(input_path);
    if (!orig) {
        fprintf(stderr, "Could not load image %s: %s\n", input_path, IMG_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    // Convert to 32-bit ARGB format for easy pixel access
    SDL_Surface *surf = SDL_ConvertSurfaceFormat(orig, SDL_PIXELFORMAT_ARGB8888, 0);
    SDL_FreeSurface(orig);
    if (!surf) {
        fprintf(stderr, "Convert surface error: %s\n", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    int width = surf->w;
    int height = surf->h;
    // Create a new surface for the binary (thresholded) image
    SDL_Surface *binSurf = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!binSurf) {
        fprintf(stderr, "Create surface error: %s\n", SDL_GetError());
        SDL_FreeSurface(surf);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Lock surfaces for direct pixel access
    SDL_LockSurface(surf);
    SDL_LockSurface(binSurf);
    Uint8 *pixels = (Uint8 *)surf->pixels;
    Uint8 *binPixels = (Uint8 *)binSurf->pixels;
    int surfPitch = surf->pitch;
    int binPitch = binSurf->pitch;

    // Allocate arrays for projections and binary map
    int *col_sum = (int *)calloc(width, sizeof(int));
    int *row_sum = (int *)calloc(height, sizeof(int));
    if (!col_sum || !row_sum) {
        fprintf(stderr, "Memory allocation error\n");
        free(col_sum);
        free(row_sum);
        SDL_UnlockSurface(surf);
        SDL_UnlockSurface(binSurf);
        SDL_FreeSurface(surf);
        SDL_FreeSurface(binSurf);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Prepare histogram for Otsu threshold
    unsigned int hist[256] = {0};
    for (int y = 0; y < height; ++y) {
        Uint8 *row = pixels + y * surfPitch;
        for (int x = 0; x < width; ++x) {
            Uint8 b = row[x * 4 + 0];
            Uint8 g = row[x * 4 + 1];
            Uint8 r = row[x * 4 + 2];
            Uint32 gray = (Uint32)(r * 299 + g * 587 + b * 114) / 1000;
            if (gray > 255) gray = 255;
            hist[gray] += 1;
        }
    }
    unsigned int total_pixels = width * height;
    Uint8 threshVal = compute_otsu_threshold(hist, total_pixels);
    // Map colors for black and white in the binary surface format
    Uint32 whitePixel = SDL_MapRGB(binSurf->format, 255, 255, 255);
    Uint32 blackPixel = SDL_MapRGB(binSurf->format, 0, 0, 0);

    int min_x = width, min_y = height;
    int max_x = -1, max_y = -1;
    for (int y = 0; y < height; ++y) {
        Uint8 *srcRow = pixels + y * surfPitch;
        Uint8 *dstRow = binPixels + y * binPitch;
        for (int x = 0; x < width; ++x) {
            Uint8 b = srcRow[x * 4 + 0];
            Uint8 g = srcRow[x * 4 + 1];
            Uint8 r = srcRow[x * 4 + 2];
            Uint32 gray = (Uint32)(r * 299 + g * 587 + b * 114) / 1000;
            if (gray > 255) gray = 255;
            if (gray < threshVal) {
                *(Uint32 *)(dstRow + x * 4) = blackPixel;
                col_sum[x] += 1;
                row_sum[y] += 1;
                if (x < min_x) min_x = x;
                if (x > max_x) max_x = x;
                if (y < min_y) min_y = y;
                if (y > max_y) max_y = y;
            } else {
                *(Uint32 *)(dstRow + x * 4) = whitePixel;
            }
        }
    }
    SDL_UnlockSurface(surf);
    SDL_UnlockSurface(binSurf);

    if (max_x < 0 || max_y < 0) {
        min_x = 0; min_y = 0;
        max_x = 0; max_y = 0;
    }

    int best_run_col = 0, best_col_start = -1, best_col_end = -1;
    int curr_run = 0, curr_start = -1;
    for (int cx = min_x; cx <= max_x; ++cx) {
        if (col_sum[cx] == 0) {
            if (curr_run == 0) curr_start = cx;
            curr_run++;
        } else {
            if (curr_run > best_run_col) {
                best_run_col = curr_run;
                best_col_start = curr_start;
                best_col_end = cx - 1;
            }
            curr_run = 0;
        }
    }
    if (curr_run > best_run_col) {
        best_run_col = curr_run;
        best_col_start = curr_start;
        best_col_end = max_x;
    }

    int best_run_row = 0, best_row_start = -1, best_row_end = -1;
    curr_run = 0; curr_start = -1;
    for (int ry = min_y; ry <= max_y; ++ry) {
        if (row_sum[ry] == 0) {
            if (curr_run == 0) curr_start = ry;
            curr_run++;
        } else {
            if (curr_run > best_run_row) {
                best_run_row = curr_run;
                best_row_start = curr_start;
                best_row_end = ry - 1;
            }
            curr_run = 0;
        }
    }
    if (curr_run > best_run_row) {
        best_run_row = curr_run;
        best_row_start = curr_start;
        best_row_end = max_y;
    }

    int split_vertical = 0;
    int split_horizontal = 0;
    if (best_run_col >= 1) split_vertical = 1;
    if (best_run_row >= 1) split_horizontal = 1;
    if (split_vertical && split_horizontal) {
        if (best_run_col >= best_run_row) {
            split_horizontal = 0;
        } else {
            split_vertical = 0;
        }
    }

    int grid_x, grid_y, grid_w, grid_h;
    int list_x, list_y, list_w, list_h;
    if (!split_vertical && !split_horizontal) {
        grid_x = min_x;
        grid_y = min_y;
        grid_w = max_x - min_x + 1;
        grid_h = max_y - min_y + 1;
        list_x = list_y = list_w = list_h = 0;
    } else if (split_vertical) {
        int gap_start = best_col_start;
        int gap_end = best_col_end;
        int c1_x1 = min_x;
        int c1_x2 = gap_start - 1;
        int c2_x1 = gap_end + 1;
        int c2_x2 = max_x;
        int c1_y1 = height, c1_y2 = -1;
        int c2_y1 = height, c2_y2 = -1;
        SDL_LockSurface(binSurf);
        Uint8 *binPix = (Uint8 *)binSurf->pixels;
        int binPitch2 = binSurf->pitch;
        for (int y = min_y; y <= max_y; ++y) {
            Uint32 *row = (Uint32 *)(binPix + y * binPitch2);
            if (c1_x1 <= c1_x2) {
                for (int x = c1_x1; x <= c1_x2; ++x) {
                    if (row[x] == blackPixel) {
                        if (y < c1_y1) c1_y1 = y;
                        if (y > c1_y2) c1_y2 = y;
                        break;
                    }
                }
            }
            if (c2_x1 <= c2_x2) {
                for (int x = c2_x1; x <= c2_x2; ++x) {
                    if (row[x] == blackPixel) {
                        if (y < c2_y1) c2_y1 = y;
                        if (y > c2_y2) c2_y2 = y;
                        break;
                    }
                }
            }
        }
        SDL_UnlockSurface(binSurf);
        long c1_black = 0, c2_black = 0;
        SDL_LockSurface(binSurf);
        binPix = (Uint8 *)binSurf->pixels;
        for (int y = c1_y1; y <= c1_y2; ++y) {
            Uint32 *row = (Uint32 *)(binPix + y * binPitch2);
            for (int x = c1_x1; x <= c1_x2; ++x) {
                if (row[x] == blackPixel) c1_black++;
            }
        }
        for (int y = c2_y1; y <= c2_y2; ++y) {
            Uint32 *row = (Uint32 *)(binPix + y * binPitch2);
            for (int x = c2_x1; x <= c2_x2; ++x) {
                if (row[x] == blackPixel) c2_black++;
            }
        }
        SDL_UnlockSurface(binSurf);
        int gridCluster1 = (c1_black >= c2_black);
        if (gridCluster1) {
            grid_x = c1_x1;
            grid_y = c1_y1;
            grid_w = c1_x2 - c1_x1 + 1;
            grid_h = c1_y2 - c1_y1 + 1;
            list_x = c2_x1;
            list_y = c2_y1;
            list_w = c2_x2 - c2_x1 + 1;
            list_h = c2_y2 - c2_y1 + 1;
        } else {
            grid_x = c2_x1;
            grid_y = c2_y1;
            grid_w = c2_x2 - c2_x1 + 1;
            grid_h = c2_y2 - c2_y1 + 1;
            list_x = c1_x1;
            list_y = c1_y1;
            list_w = c1_x2 - c1_x1 + 1;
            list_h = c1_y2 - c1_y1 + 1;
        }
    } else { // split_horizontal
        int gap_start = best_row_start;
        int gap_end = best_row_end;
        int c1_y1 = min_y;
        int c1_y2 = gap_start - 1;
        int c2_y1 = gap_end + 1;
        int c2_y2 = max_y;
        int c1_x1 = width, c1_x2 = -1;
        int c2_x1 = width, c2_x2 = -1;
        SDL_LockSurface(binSurf);
        Uint8 *binPix2 = (Uint8 *)binSurf->pixels;
        int binPitch3 = binSurf->pitch;
        for (int x = min_x; x <= max_x; ++x) {
            for (int y = c1_y1; y <= c1_y2; ++y) {
                Uint32 pixel = *(Uint32 *)(binPix2 + y * binPitch3 + x * 4);
                if (pixel == blackPixel) {
                    if (x < c1_x1) c1_x1 = x;
                    if (x > c1_x2) c1_x2 = x;
                    break;
                }
            }
            for (int y = c2_y1; y <= c2_y2; ++y) {
                Uint32 pixel = *(Uint32 *)(binPix2 + y * binPitch3 + x * 4);
                if (pixel == blackPixel) {
                    if (x < c2_x1) c2_x1 = x;
                    if (x > c2_x2) c2_x2 = x;
                    break;
                }
            }
        }
        SDL_UnlockSurface(binSurf);
        long c1_black = 0, c2_black = 0;
        SDL_LockSurface(binSurf);
        binPix2 = (Uint8 *)binSurf->pixels;
        for (int y = c1_y1; y <= c1_y2; ++y) {
            Uint32 *row = (Uint32 *)(binPix2 + y * binPitch3);
            for (int x = c1_x1; x <= c1_x2; ++x) {
                if (row[x] == blackPixel) c1_black++;
            }
        }
        for (int y = c2_y1; y <= c2_y2; ++y) {
            Uint32 *row = (Uint32 *)(binPix2 + y * binPitch3);
            for (int x = c2_x1; x <= c2_x2; ++x) {
                if (row[x] == blackPixel) c2_black++;
            }
        }
        SDL_UnlockSurface(binSurf);
        int gridCluster1 = (c1_black >= c2_black);
        if (gridCluster1) {
            grid_x = c1_x1;
            grid_y = c1_y1;
            grid_w = c1_x2 - c1_x1 + 1;
            grid_h = c1_y2 - c1_y1 + 1;
            list_x = c2_x1;
            list_y = c2_y1;
            list_w = c2_x2 - c2_x1 + 1;
            list_h = c2_y2 - c2_y1 + 1;
        } else {
            grid_x = c2_x1;
            grid_y = c2_y1;
            grid_w = c2_x2 - c2_x1 + 1;
            grid_h = c2_y2 - c2_y1 + 1;
            list_x = c1_x1;
            list_y = c1_y1;
            list_w = c1_x2 - c1_x1 + 1;
            list_h = c1_y2 - c1_y1 + 1;
        }
    }

    SDL_LockSurface(surf);
    Uint8 *outPixels = (Uint8 *)surf->pixels;
    int outPitch = surf->pitch;
    Uint32 redColor = SDL_MapRGB(surf->format, 255, 0, 0);
    Uint32 blueColor = SDL_MapRGB(surf->format, 0, 0, 255);
    // Draw grid box in red
    if (grid_w > 0 && grid_h > 0) {
        if (grid_y >= 0 && grid_y < height) {
            Uint32 *topRow = (Uint32 *)(outPixels + grid_y * outPitch);
            for (int x = grid_x; x < grid_x + grid_w && x < width; ++x) {
                topRow[x] = redColor;
            }
        }
        int by = grid_y + grid_h - 1;
        if (by >= 0 && by < height) {
            Uint32 *bottomRow = (Uint32 *)(outPixels + by * outPitch);
            for (int x = grid_x; x < grid_x + grid_w && x < width; ++x) {
                bottomRow[x] = redColor;
            }
        }
        if (grid_x >= 0 && grid_x < width) {
            for (int y = grid_y; y < grid_y + grid_h && y < height; ++y) {
                Uint32 *row = (Uint32 *)(outPixels + y * outPitch);
                row[grid_x] = redColor;
            }
        }
        int rx = grid_x + grid_w - 1;
        if (rx >= 0 && rx < width) {
            for (int y = grid_y; y < grid_y + grid_h && y < height; ++y) {
                Uint32 *row = (Uint32 *)(outPixels + y * outPitch);
                row[rx] = redColor;
            }
        }
    }
    // Draw word list box in blue
    if (list_w > 0 && list_h > 0) {
        if (list_y >= 0 && list_y < height) {
            Uint32 *topRow = (Uint32 *)(outPixels + list_y * outPitch);
            for (int x = list_x; x < list_x + list_w && x < width; ++x) {
                topRow[x] = blueColor;
            }
        }
        int by = list_y + list_h - 1;
        if (by >= 0 && by < height) {
            Uint32 *bottomRow = (Uint32 *)(outPixels + by * outPitch);
            for (int x = list_x; x < list_x + list_w && x < width; ++x) {
                bottomRow[x] = blueColor;
            }
        }
        if (list_x >= 0 && list_x < width) {
            for (int y = list_y; y < list_y + list_h && y < height; ++y) {
                Uint32 *row = (Uint32 *)(outPixels + y * outPitch);
                row[list_x] = blueColor;
            }
        }
        int rx = list_x + list_w - 1;
        if (rx >= 0 && rx < width) {
            for (int y = list_y; y < list_y + list_h && y < height; ++y) {
                Uint32 *row = (Uint32 *)(outPixels + y * outPitch);
                row[rx] = blueColor;
            }
        }
    }
    SDL_UnlockSurface(surf);

    if (SDL_SaveBMP(binSurf, "stage_bin.bmp") != 0) {
        fprintf(stderr, "Failed to save stage_bin.bmp: %s\n", SDL_GetError());
    }
    if (SDL_SaveBMP(surf, "stage_layout.bmp") != 0) {
        fprintf(stderr, "Failed to save stage_layout.bmp: %s\n", SDL_GetError());
    }
    FILE *fg = fopen("grid_roi.txt", "w");
    if (fg) {
        fprintf(fg, "%d %d %d %d", grid_x, grid_y, grid_w, grid_h);
        fclose(fg);
    } else {
        fprintf(stderr, "Failed to write grid_roi.txt\n");
    }
    FILE *fw = fopen("wordlist_roi.txt", "w");
    if (fw) {
        fprintf(fw, "%d %d %d %d", list_x, list_y, list_w, list_h);
        fclose(fw);
    } else {
        fprintf(stderr, "Failed to write wordlist_roi.txt\n");
    }

    // Clean up
    free(col_sum);
    free(row_sum);
    SDL_FreeSurface(surf);
    SDL_FreeSurface(binSurf);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
