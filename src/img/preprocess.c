#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <err.h>

SDL_Surface* grayscale(SDL_Surface* surface);
SDL_Surface* linear_contrast(SDL_Surface* surface);
SDL_Surface* rotate_surface(SDL_Surface* surface, double angle_degrees);

int main(int argc, char* argv[]) 
{
    if (argc < 2) 
    {
        printf("Usage: %s <image>\n", argv[0]);
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) 
    {
        printf("Erreur SDL_Init : %s\n", SDL_GetError());
        return 1;
    }

    if (!(IMG_Init(IMG_INIT_PNG))) 
    {
        printf("Error IMG_Init : %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Surface* image = IMG_Load(argv[1]);
    if (!image) 
    {
        printf("Error loading img %s\n", IMG_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    printf("Image loaded: %dx%d\n", image->w, image->h);

    // Grayscale
    SDL_Surface* gray = grayscale(image);
    if (!gray) errx(EXIT_FAILURE, "Error converting to grayscale");

    SDL_SaveBMP(gray, "gray.bmp");

    // Contrast
    SDL_Surface* contrast = linear_contrast(gray);
    if (!contrast) errx(EXIT_FAILURE, "Error applying contrast");

    SDL_SaveBMP(contrast, "contrast.bmp");

    printf("'gray.bmp' & 'contrast.bmp' generated\n");

    // Ask user for rotation angle
    double angle;
    printf("Enter rotation angle in degrees: ");
    if (scanf("%lf", &angle) != 1) angle = 0.0;

    SDL_Surface* rotated = rotate_surface(contrast, angle);
    if (!rotated) errx(EXIT_FAILURE, "Error rotating image");

    SDL_SaveBMP(rotated, "rotated.bmp");
    printf("'rotated.bmp' generated\n");

    SDL_FreeSurface(image);
    SDL_FreeSurface(gray);
    SDL_FreeSurface(contrast);
    SDL_FreeSurface(rotated);
    IMG_Quit();
    SDL_Quit();
    return 0;
}

// Convert to grayscale
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
            Uint8 grayValue = (Uint8)(0.299*r + 0.587*g + 0.114*b);
            pixels[y * w + x] = SDL_MapRGB(fmt, grayValue, grayValue, grayValue);
        }
    }

    if (SDL_MUSTLOCK(gray)) 
	    SDL_UnlockSurface(gray);
    return gray;
}

// Apply linear contrast
SDL_Surface* linear_contrast(SDL_Surface* surface) 
{
    SDL_Surface* result = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGB888, 0);
    if (!result) 
	    return NULL;

    if (SDL_MUSTLOCK(result)) 
	    SDL_LockSurface(result);

    Uint32* pixels = (Uint32*)result->pixels;
    int w = result->w;
    int h = result->h;
    SDL_PixelFormat* fmt = result->format;

    Uint8 Imin = 255, Imax = 0;
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

    if (SDL_MUSTLOCK(result)) 
	    SDL_UnlockSurface(result);
    return result;
}

// Manual rotation (nearest neighbor)
SDL_Surface* rotate_surface(SDL_Surface* surface, double angle_degrees)
{
    int w = surface->w;
    int h = surface->h;
    double angle = angle_degrees * M_PI / 180.0;

    // Compute new dimensions
    int new_w = (int)(fabs(w*cos(angle)) + fabs(h*sin(angle)));
    int new_h = (int)(fabs(w*sin(angle)) + fabs(h*cos(angle)));

    SDL_Surface* rotated = SDL_CreateRGBSurfaceWithFormat(0, new_w, new_h, 32, surface->format->format);
    if (!rotated) return NULL;

    if (SDL_MUSTLOCK(surface)) SDL_LockSurface(surface);
    if (SDL_MUSTLOCK(rotated)) SDL_LockSurface(rotated);

    Uint32* src_pixels = (Uint32*)surface->pixels;
    Uint32* dst_pixels = (Uint32*)rotated->pixels;

    int cx = w / 2;
    int cy = h / 2;
    int ncx = new_w / 2;
    int ncy = new_h / 2;

    for (int y = 0; y < new_h; y++)
    {
        for (int x = 0; x < new_w; x++)
        {
            // Coordinates relative to new center
            double rx = x - ncx;
            double ry = y - ncy;

            // Map back to original image
            int sx = (int)( cos(angle) * rx + sin(angle) * ry) + cx;
            int sy = (int)(-sin(angle) * rx + cos(angle) * ry) + cy;

            if (sx >= 0 && sx < w && sy >= 0 && sy < h)
                dst_pixels[y*new_w + x] = src_pixels[sy*w + sx];
            else
                dst_pixels[y*new_w + x] = SDL_MapRGB(surface->format, 0, 0, 0);
        }
    }

    if (SDL_MUSTLOCK(surface)) SDL_UnlockSurface(surface);
    if (SDL_MUSTLOCK(rotated)) SDL_UnlockSurface(rotated);

    return rotated;
}
