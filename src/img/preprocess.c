#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <err.h>

//-----------------------------------------------
// INTERNAL HELPERS (not visible outside module)
//-----------------------------------------------
static SDL_Surface* grayscale(SDL_Surface* surface);
static SDL_Surface* linear_contrast(SDL_Surface* surface);
static SDL_Surface* rotate_surface(SDL_Surface* surface, double angle_degrees);

//-----------------------------------------------
// PUBLIC API: preprocess() — to be used by pipeline
//-----------------------------------------------
SDL_Surface* preprocess(SDL_Surface* input, double rotation_angle)
{
    if (!input)
        return NULL;

    SDL_Surface* gray = grayscale(input);
    if (!gray)
        return NULL;

    SDL_Surface* contrast = linear_contrast(gray);
    SDL_FreeSurface(gray);
    if (!contrast)
        return NULL;

    SDL_Surface* rotated = rotate_surface(contrast, rotation_angle);
    SDL_FreeSurface(contrast);
    if (!rotated)
        return NULL;

    return rotated;   // Pipeline receives the final surface
}

//-----------------------------------------------
// Grayscale
//-----------------------------------------------
static SDL_Surface* grayscale(SDL_Surface* surface)
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

//-----------------------------------------------
// Linear contrast
//-----------------------------------------------
static SDL_Surface* linear_contrast(SDL_Surface* surface)
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

//-----------------------------------------------
// Rotation
//-----------------------------------------------
static SDL_Surface* rotate_surface(SDL_Surface* surface, double angle_degrees)
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

    Uint32* src = (Uint32*)surface->pixels;
    Uint32* dst = (Uint32*)rotated->pixels;

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

            if (sx >= 0 && sx < w && sy >= 0 && sy < h)
                dst[y*new_w + x] = src[sy*w + sx];
            else
                dst[y*new_w + x] = SDL_MapRGB(surface->format, 0, 0, 0);
        }
    }

    if (SDL_MUSTLOCK(surface)) SDL_UnlockSurface(surface);
    if (SDL_MUSTLOCK(rotated)) SDL_UnlockSurface(rotated);

    return rotated;
}

