#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <math.h>
#include "highlight.h"
#include <string.h>

// Draw a thick line with transparency on an SDL_Surface
static void draw_thick_line_surface(SDL_Surface *surface, int x0, int y0, int x1, int y1, int thickness, Uint32 color) {
    //int w = x1 - x0;
    //int h = y1 - y0;
    float dx = x1 - x0;
    float dy = y1 - y0;
    float length = sqrtf(dx*dx + dy*dy);
    if (length == 0) return;

    float px = -dy / length;
    float py = dx / length;

    for (int t = -thickness/2; t <= thickness/2; t++) {
        int ox = (int)(px * t);
        int oy = (int)(py * t);

        // Simple Bresenham line drawing
        int steps = (int)length + 1;
        for (int i = 0; i <= steps; i++) {
            float fx = x0 + ox + dx * i / length;
            float fy = y0 + oy + dy * i / length;
            int ix = (int)fx;
            int iy = (int)fy;
            if (ix >= 0 && ix < surface->w && iy >= 0 && iy < surface->h) {
                Uint32 *pixels = (Uint32 *)surface->pixels;
                Uint32 dst = pixels[iy * surface->w + ix];

                // Alpha blending: result = src * alpha + dst * (1-alpha)
                Uint8 sr = (color >> 16) & 0xFF;
                Uint8 sg = (color >> 8) & 0xFF;
                Uint8 sb = color & 0xFF;
                Uint8 sa = (color >> 24) & 0xFF;

                Uint8 dr = (dst >> 16) & 0xFF;
                Uint8 dg = (dst >> 8) & 0xFF;
                Uint8 db = dst & 0xFF;

                float alpha = sa / 255.0f;
                Uint8 nr = (Uint8)(sr * alpha + dr * (1-alpha));
                Uint8 ng = (Uint8)(sg * alpha + dg * (1-alpha));
                Uint8 nb = (Uint8)(sb * alpha + db * (1-alpha));

                pixels[iy * surface->w + ix] = (0xFF << 24) | (nr << 16) | (ng << 8) | nb;
            }
        }
    }
}

int highlight_words_on_image(
    const char *input_path,
    const char *output_path,
    const char *solver_results_path,
    int gx, int gy, int gw, int gh,
    int rows, int cols
) {
    SDL_Surface *img = IMG_Load(input_path);
    if (!img) {
        fprintf(stderr, "Cannot load %s\n", input_path);
        return -1;
    }

    float cell_w = (float)gw / cols;
    float cell_h = (float)gh / rows;

    FILE *f = fopen(solver_results_path, "r");
    if (!f) {
        perror("solver_results.txt");
        SDL_FreeSurface(img);
        return -1;
    }

    int thickness = 8; // adjust thickness here
    Uint32 color = 0x80FF0000; // semi-transparent red

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        // skip empty lines
        if (strlen(line) < 3) continue;

        char word[128];
        int x0, y0, x1, y1;

        if (sscanf(line, "%127s %d %d %d %d", word, &x0, &y0, &x1, &y1) == 5) {
            // Valid word with coordinates

            // Clamp coordinates to grid bounds
            if (x0 < 0) x0 = 0;
            if (y0 < 0) y0 = 0;
            if (x1 >= cols) x1 = cols - 1;
            if (y1 >= rows) y1 = rows - 1;

            int sx = gx + x0 * cell_w + cell_w / 2;
            int sy = gy + y0 * cell_h + cell_h / 2;
            int ex = gx + x1 * cell_w + cell_w / 2;
            int ey = gy + y1 * cell_h + cell_h / 2;

            // Clamp pixel coordinates to image
            if (sx < 0) sx = 0;
            if (sy < 0) sy = 0;
            if (ex >= img->w) ex = img->w - 1;
            if (ey >= img->h) ey = img->h - 1;

            draw_thick_line_surface(img, sx, sy, ex, ey, thickness, color);
        }
        // else ignore NOT_FOUND lines automatically
    }

    fclose(f);

    if (IMG_SavePNG(img, output_path) != 0)
        fprintf(stderr, "Failed to save %s\n", output_path);

    SDL_FreeSurface(img);
    return 0;
}
