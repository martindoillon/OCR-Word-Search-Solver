#include "preprocess.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <err.h>
#include <SDL2/SDL.h>


// PUBLIC API: preprocess()
SDL_Surface* preprocess(SDL_Surface* input)
{
    SDL_Surface* gray = grayscale(input);
    SDL_Surface* contrast = linear_contrast(gray);
    SDL_FreeSurface(gray);

    SDL_Surface* binary = make_binary(contrast, NULL);

    
    // denoise the binary image
    SDL_Surface* clean = denoise_binary(binary);
    SDL_FreeSurface(binary);

    // detect dominant line orientation
    double angle = detect_rotation_angle_projection(clean);
    SDL_Surface* rotated = rotate_surface(clean, -angle);
    SDL_FreeSurface(clean);
    SDL_FreeSurface(contrast);
    
    return rotated;
}

SDL_Surface* preprocess_no_denoise(SDL_Surface* input)
{
    SDL_Surface* gray = grayscale(input);
    SDL_Surface* contrast = linear_contrast(gray);
    SDL_FreeSurface(gray);

    SDL_Surface* binary = make_binary(contrast, NULL);


    // detect rotation on raw binary
    double angle = detect_rotation_angle_projection(binary);
    SDL_Surface* rotated = rotate_surface(binary, -angle);

    SDL_FreeSurface(binary);
    SDL_FreeSurface(contrast);

    return rotated;
}


// -----------------------------------------------------------------------------
// GRAYSCALE
SDL_Surface* grayscale(SDL_Surface* surface)
{
    SDL_Surface* gray = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGB888, 0);
    if (!gray) return NULL;

    if (SDL_MUSTLOCK(gray)) SDL_LockSurface(gray);

    Uint32* pixels = (Uint32*)gray->pixels;
    int w = gray->w;
    int h = gray->h;
    SDL_PixelFormat* fmt = gray->format;

    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            Uint32 pixel = pixels[y * w + x];
            Uint8 r, g, b;
            SDL_GetRGB(pixel, fmt, &r, &g, &b);
            Uint8 gval = (Uint8)(0.299*r + 0.587*g + 0.114*b);
            pixels[y * w + x] = SDL_MapRGB(fmt, gval, gval, gval);
        }
    }

    if (SDL_MUSTLOCK(gray)) SDL_UnlockSurface(gray);
    return gray;
}

// -----------------------------------------------------------------------------
// LINEAR CONTRAST
SDL_Surface* linear_contrast(SDL_Surface* surface)
{
    SDL_Surface* result = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGB888, 0);
    if (!result) return NULL;

    if (SDL_MUSTLOCK(result)) SDL_LockSurface(result);

    Uint32* pixels = (Uint32*)result->pixels;
    int w = result->w;
    int h = result->h;
    SDL_PixelFormat* fmt = result->format;

    Uint8 Imin = 255, Imax = 0;

    // Find min & max
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            Uint8 r, g, b;
            SDL_GetRGB(pixels[y * w + x], fmt, &r, &g, &b);
            if (r < Imin) Imin = r;
            if (r > Imax) Imax = r;
        }
    }

    if (Imax == Imin)
    {
        if (SDL_MUSTLOCK(result)) SDL_UnlockSurface(result);
        return result;
    }

    // Stretch contrast
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            Uint8 r, g, b;
            SDL_GetRGB(pixels[y * w + x], fmt, &r, &g, &b);
            Uint8 newV = (Uint8)(((float)(r - Imin) / (Imax - Imin)) * 255.0f);
            pixels[y * w + x] = SDL_MapRGB(fmt, newV, newV, newV);
        }
    }

    if (SDL_MUSTLOCK(result)) SDL_UnlockSurface(result);
    return result;
}

// -----------------------------------------------------------------------------
// ROTATION
SDL_Surface* rotate_surface(SDL_Surface* surface, double angle_degrees)
{
    int w = surface->w;
    int h = surface->h;
    double angle = angle_degrees * M_PI / 180.0;

    int new_w = (int)(fabs(w*cos(angle)) + fabs(h*sin(angle)));
    int new_h = (int)(fabs(w*sin(angle)) + fabs(h*cos(angle)));

    SDL_Surface* rotated = SDL_CreateRGBSurfaceWithFormat(
        0, new_w, new_h, 32, surface->format->format);
    if (!rotated) return NULL;

    if (SDL_MUSTLOCK(surface)) SDL_LockSurface(surface);
    if (SDL_MUSTLOCK(rotated)) SDL_LockSurface(rotated);

    Uint8* src_bytes = (Uint8*)surface->pixels;
    Uint8* dst_bytes = (Uint8*)rotated->pixels;

    int cx = w / 2;
    int cy = h / 2;
    int ncx = new_w / 2;
    int ncy = new_h / 2;

    for (int y = 0; y < new_h; y++)
    {
        for (int x = 0; x < new_w; x++)
        {
            double rx = x - ncx;
            double ry = y - ncy;

            int sx = (int)( cos(angle) * rx + sin(angle) * ry) + cx;
            int sy = (int)(-sin(angle) * rx + cos(angle) * ry) + cy;

            Uint32* dst_pixel = (Uint32*)(dst_bytes + y * rotated->pitch + x * 4);

            if (sx >= 0 && sx < w && sy >= 0 && sy < h)
            {
                Uint32* src_pixel = (Uint32*)(src_bytes + sy * surface->pitch + sx * 4);
                *dst_pixel = *src_pixel;
            }
            else
            {
                // Fill outside area with white instead of black
                *dst_pixel = SDL_MapRGB(surface->format, 255, 255, 255);
            }
        }
    }

    if (SDL_MUSTLOCK(surface)) SDL_UnlockSurface(surface);
    if (SDL_MUSTLOCK(rotated)) SDL_UnlockSurface(rotated);

    return rotated;
}


// -----------------------------------------------------------------------------
// OTSU THRESHOLD
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

// -----------------------------------------------------------------------------
// BINARIZATION
SDL_Surface* make_binary(SDL_Surface *src, Uint8 *out_thresh)
{
    int w = src->w;
    int h = src->h;
    unsigned int hist[256] = {0};

    SDL_LockSurface(src);
    Uint8 *sp = (Uint8 *)src->pixels;
    int spitch = src->pitch;

    // build histogram
    for (int y = 0; y < h; ++y)
    {
        Uint8 *row = sp + y * spitch;
        for (int x = 0; x < w; ++x)
        {
            Uint8 r = row[x*4+2];
            hist[r]++;
        }
    }
    SDL_UnlockSurface(src);

    Uint8 thr = compute_otsu_threshold(hist, (unsigned int)w * (unsigned int)h);
    if (out_thresh) *out_thresh = thr;

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
            Uint8 r = srow[x*4+2];
            *(Uint32 *)(brow + x*4) = (r < thr) ? black : white;
        }
    }

    SDL_UnlockSurface(bin);
    SDL_UnlockSurface(src);

    return bin;
}


// -----------------------------------------------------------------------------
// REMOVE SINGLE-PIXEL NOISE FROM BINARY IMAGE
SDL_Surface* denoise_binary(SDL_Surface* src)
{
    int w = src->w;
    int h = src->h;
    SDL_Surface* clean = SDL_ConvertSurfaceFormat(src, SDL_PIXELFORMAT_ARGB8888, 0);
    if (!clean) return NULL;

    if (SDL_MUSTLOCK(src)) SDL_LockSurface(src);
    if (SDL_MUSTLOCK(clean)) SDL_LockSurface(clean);

    Uint32* sp = (Uint32*)src->pixels;
    Uint32* dp = (Uint32*)clean->pixels;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int black_count = 0;

            // Count black neighbors in 3x3 neighborhood
            for (int dy = -1; dy <= 1; dy++) {
                int ny = y + dy;
                if (ny < 0 || ny >= h) continue;
                for (int dx = -1; dx <= 1; dx++) {
                    int nx = x + dx;
                    if (nx < 0 || nx >= w) continue;

                    Uint32 px = sp[ny * w + nx];
                    Uint8 r = (px >> 16) & 0xFF;
                    Uint8 g = (px >> 8) & 0xFF;
                    Uint8 b = px & 0xFF;

                    // Use simple luminance threshold
                    int lum = (r + g + b) / 3;
                    if (lum < 128) black_count++;
                }
            }

            // Keep black only if at least 3 neighbors are black (more aggressive denoise)
            if (black_count >= 5) // 5 ca tue pour le lvl 2
                dp[y * w + x] = SDL_MapRGB(clean->format, 0, 0, 0);
            else
                dp[y * w + x] = SDL_MapRGB(clean->format, 255, 255, 255);
        }
    }

    if (SDL_MUSTLOCK(src)) SDL_UnlockSurface(src);
    if (SDL_MUSTLOCK(clean)) SDL_UnlockSurface(clean);

    return clean;
}



// -----------------------------------------------------------------------------
// SOBEL EDGES
SDL_Surface* sobel_edges(SDL_Surface* gray)
{
    int w = gray->w, h = gray->h;
    SDL_Surface* edge = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGB888);
    if (!edge) return NULL;

    Uint8* src = gray->pixels;
    Uint8* out = edge->pixels;

    int spitch = gray->pitch;
    int opitch = edge->pitch;

    for (int y = 1; y < h-1; y++)
    for (int x = 1; x < w-1; x++) {

        int gx =
            + src[(y-1)*spitch + (x+1)*4]
            + 2*src[y*spitch + (x+1)*4]
            + src[(y+1)*spitch + (x+1)*4]
            - src[(y-1)*spitch + (x-1)*4]
            - 2*src[y*spitch + (x-1)*4]
            - src[(y+1)*spitch + (x-1)*4];

        int gy =
            + src[(y+1)*spitch + (x-1)*4]
            + 2*src[(y+1)*spitch + x*4]
            + src[(y+1)*spitch + (x+1)*4]
            - src[(y-1)*spitch + (x-1)*4]
            - 2*src[(y-1)*spitch + x*4]
            - src[(y-1)*spitch + (x+1)*4];

        int v = sqrt(gx*gx + gy*gy);
        if (v > 255) v = 255;

        Uint8* p = out + y*opitch + x*4;
        p[0] = p[1] = p[2] = v;
    }

    return edge;
}

// -----------------------------------------------------------------------------
// DETECT ROTATION USING PROJECTION PROFILE
double detect_rotation_angle_projection(SDL_Surface* bin)
{
    int w = bin->w, h = bin->h;
    Uint32* pixels = (Uint32*)bin->pixels;

    double best_angle = 0.0;
    double max_variance = 0.0;

    // Wider search range, smaller increments
    for (double angle = -30.0; angle <= 30.0; angle += 0.25) {
        double rad = angle * M_PI / 180.0;
        double cos_a = cos(rad);
        double sin_a = sin(rad);

        double profile[4000] = {0};
        int profile_len = 0;

        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                Uint32 c = pixels[y*w + x];
                Uint8 r = (c >> 16) & 0xFF;
                if (r > 128) continue; // only black pixels

                int proj = (int)(x*cos_a + y*sin_a);
                int idx = proj + 2000;
                if (idx >= 0 && idx < 4000) {
                    profile[idx] += 1;
                    if (idx > profile_len) profile_len = idx;
                }
            }
        }

        double mean = 0, var = 0;
        for (int i = 0; i <= profile_len; i++) mean += profile[i];
        mean /= (profile_len+1);
        for (int i = 0; i <= profile_len; i++) {
            double diff = profile[i] - mean;
            var += diff * diff;
        }

        if (var > max_variance) {
            max_variance = var;
            best_angle = angle;
        }
    }

    return best_angle;
}

