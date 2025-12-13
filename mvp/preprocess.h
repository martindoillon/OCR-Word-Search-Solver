#ifndef PREPROCESS_H
#define PREPROCESS_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdint.h>

// Main API
SDL_Surface* preprocess(SDL_Surface* input);

SDL_Surface* preprocess_no_denoise(SDL_Surface* input);

SDL_Surface* make_binary(SDL_Surface *src, Uint8 *out_thresh);
SDL_Surface* grayscale(SDL_Surface* surface);
SDL_Surface* linear_contrast(SDL_Surface* surface);
SDL_Surface* rotate_surface(SDL_Surface* surface, double angle_degrees);
Uint8 compute_otsu_threshold(unsigned int hist[256], unsigned int total);

SDL_Surface* sobel_edges(SDL_Surface* gray);
double detect_rotation_angle_projection(SDL_Surface* bin);
SDL_Surface* denoise_binary(SDL_Surface* src);

#endif

