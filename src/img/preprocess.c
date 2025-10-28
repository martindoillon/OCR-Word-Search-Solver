#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <err.h>

SDL_Surface* grayscale(SDL_Surface* surface);
SDL_Surface* linear_contrast(SDL_Surface* surface);

int main(int argc, char* argv[]) 
{
    
    if (argc < 2) 
    {
        printf("Usage: %s <image>\n", argv[0]);
        return 1;
    }

    
    //initialise surfaces & video 
    if (SDL_Init(SDL_INIT_VIDEO) != 0) 
    {
        printf("Erreur SDL_Init : %s\n", SDL_GetError());
        return 1;
    }

    
    //extention sdl_img to charge png format (default: bmp)
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

    printf("Image chargée : %dx%d\n", image->w, image->h);

    
    //grayscale
    SDL_Surface* gray = grayscale(image);

    if (!gray) 
    {
        SDL_FreeSurface(image);
        IMG_Quit();
        SDL_Quit();
        errx(EXIT_FAILURE,"Error convertion grayscale\n");
    }


    SDL_SaveBMP(gray, "gray.bmp");

    //contrast
    SDL_Surface* contrast = linear_contrast(gray);

    if (!contrast) 
    {
        SDL_FreeSurface(image);
        SDL_FreeSurface(gray);
        IMG_Quit();
        SDL_Quit();
        errx(EXIT_FAILURE, "Error contrast\n");
    }

    SDL_SaveBMP(contrast, "contrast.bmp");

    printf("'gray.bmp' & 'contrast.bmp' generated\n");

    SDL_FreeSurface(image);
    SDL_FreeSurface(gray);
    SDL_FreeSurface(contrast);
    IMG_Quit();
    SDL_Quit();
    return EXIT_SUCCESS;
}




SDL_Surface* grayscale(SDL_Surface* surface) 
{
    
    SDL_Surface* gray = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGB888, 0);
    if (!gray) 
	    return NULL;

    if (SDL_MUSTLOCK(gray)) 
	    SDL_LockSurface(gray);

    Uint32* pixels = (Uint32*)gray->pixels;
    int w = gray->w;
    int h = gray->h;
    SDL_PixelFormat* fmt = gray->format;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            Uint32 pixel = pixels[y * w + x];
            Uint8 r, g, b;
            SDL_GetRGB(pixel, fmt, &r, &g, &b);
            Uint8 grayValue = (Uint8)(0.299*r + 0.587*g + 0.114*b);
            pixels[y * w + x] = SDL_MapRGB(fmt, grayValue, grayValue, grayValue);
        }
    }

    if (SDL_MUSTLOCK(gray)) SDL_UnlockSurface(gray);
    return gray;
}





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
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            Uint8 r, g, b;
            SDL_GetRGB(pixels[y * w + x], fmt, &r, &g, &b);
            if (r < Imin) Imin = r;
            if (r > Imax) Imax = r;
        }
    }

    if (Imax == Imin) {
        if (SDL_MUSTLOCK(result)) SDL_UnlockSurface(result);
        return result;
    }

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            Uint8 r, g, b;
            SDL_GetRGB(pixels[y * w + x], fmt, &r, &g, &b);
            Uint8 newV = (Uint8)(((float)(r - Imin) / (Imax - Imin)) * 255.0f);
            pixels[y * w + x] = SDL_MapRGB(fmt, newV, newV, newV);
        }
    }

    if (SDL_MUSTLOCK(result)) SDL_UnlockSurface(result);
    return result;
}

