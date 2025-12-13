#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include "preprocess.h"
#include "layout_detect.h"
#include "cell_finder.h"
#include "wordlist_finder.h"
#include "predict_letters.h"
#include "utils.h"
#include "rebuild_puzzle.h"
#include "solver.h"
#include "highlight.h"
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>



SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

SDL_Surface* loaded_surface = NULL;
SDL_Surface* processed_surface = NULL;

SDL_Texture* loaded_texture = NULL;
SDL_Texture* processed_texture = NULL;

TTF_Font* font = NULL;

SDL_Rect load_button, preprocess_button, segment_button, solve_button, preprocess_denoise_button, help_button;

const int BUTTON_WIDTH = 120;
const int BUTTON_HEIGHT = 40;
const int BUTTON_MARGIN = 10;




// Helpers




char* sdl_input_popup(const char* title) {
    SDL_Window* win = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 400, 120, SDL_WINDOW_SHOWN);
    if (!win) return NULL;

    SDL_Renderer* rend = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!rend) { SDL_DestroyWindow(win); return NULL; }

    if (!TTF_WasInit()) TTF_Init();
    TTF_Font* font = TTF_OpenFont("DejaVuSans-Bold.ttf", 16);

    SDL_StartTextInput();
    char input[512] = {0};
    int len = 0;
    int quit = 0;
    int ok_pressed = 0;

    SDL_Rect ok_button = {300, 70, 80, 30};
    SDL_Rect cancel_button = {200, 70, 80, 30};
    SDL_Rect input_box = {10, 30, 380, 30};

    SDL_Event e;
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) { quit = 1; input[0] = 0; break; }
            if (e.type == SDL_TEXTINPUT) {
                if (len + strlen(e.text.text) < sizeof(input)) {
                    strcat(input, e.text.text);
                    len = strlen(input);
                }
            }
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_BACKSPACE && len > 0) {
                    input[len-1] = 0;
                    len--;
                }
            }
            if (e.type == SDL_MOUSEBUTTONDOWN) {
                int mx = e.button.x, my = e.button.y;
                if (SDL_PointInRect(&(SDL_Point){mx,my}, &ok_button)) { quit = 1; ok_pressed = 1; }
                if (SDL_PointInRect(&(SDL_Point){mx,my}, &cancel_button)) { quit = 1; input[0] = 0; }
            }
        }

        // Clear background
        SDL_SetRenderDrawColor(rend, 50, 50, 50, 255);
        SDL_RenderClear(rend);

        // Draw input box background
        SDL_SetRenderDrawColor(rend, 30, 30, 30, 255);
        SDL_RenderFillRect(rend, &input_box);
        SDL_SetRenderDrawColor(rend, 200, 200, 200, 255);
        SDL_RenderDrawRect(rend, &input_box);

        // Draw typed text
        if (font) {
            SDL_Color white = {255, 255, 255, 255};
            SDL_Surface* txt_surf = TTF_RenderText_Blended(font, input, white);
            if (txt_surf) {
                SDL_Texture* tex = SDL_CreateTextureFromSurface(rend, txt_surf);
                SDL_Rect dst = {input_box.x + 5, input_box.y + (input_box.h - txt_surf->h)/2, txt_surf->w, txt_surf->h};
                SDL_RenderCopy(rend, tex, NULL, &dst);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(txt_surf);
            }
        }

        // Draw OK and Cancel buttons
        SDL_SetRenderDrawColor(rend, 70, 70, 70, 255);
        SDL_RenderFillRect(rend, &ok_button);
        SDL_RenderFillRect(rend, &cancel_button);
        SDL_SetRenderDrawColor(rend, 200, 200, 200, 255);
        SDL_RenderDrawRect(rend, &ok_button);
        SDL_RenderDrawRect(rend, &cancel_button);

        if (font) {
            SDL_Color white = {255, 255, 255, 255};
            SDL_Surface* ok_surf = TTF_RenderText_Blended(font, "OK", white);
            SDL_Surface* cancel_surf = TTF_RenderText_Blended(font, "Cancel", white);
            if (ok_surf && cancel_surf) {
                SDL_Texture* tex1 = SDL_CreateTextureFromSurface(rend, ok_surf);
                SDL_Texture* tex2 = SDL_CreateTextureFromSurface(rend, cancel_surf);
                SDL_Rect dst1 = {ok_button.x + (ok_button.w - ok_surf->w)/2,
                                 ok_button.y + (ok_button.h - ok_surf->h)/2,
                                 ok_surf->w, ok_surf->h};
                SDL_Rect dst2 = {cancel_button.x + (cancel_button.w - cancel_surf->w)/2,
                                 cancel_button.y + (cancel_button.h - cancel_surf->h)/2,
                                 cancel_surf->w, cancel_surf->h};
                SDL_RenderCopy(rend, tex1, NULL, &dst1);
                SDL_RenderCopy(rend, tex2, NULL, &dst2);
                SDL_DestroyTexture(tex1);
                SDL_DestroyTexture(tex2);
                SDL_FreeSurface(ok_surf);
                SDL_FreeSurface(cancel_surf);
            }
        }

        SDL_RenderPresent(rend);
        SDL_Delay(10);
    }

    SDL_StopTextInput();
    TTF_CloseFont(font);
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(win);

    if (!ok_pressed || strlen(input) == 0) return NULL;
    return strdup(input); // caller must free()
}



SDL_Texture* surface_to_texture(SDL_Surface* surf) {
    if (!surf) return NULL;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    return tex;
}

// Recursively delete directory contents
void clear_directory(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) return;

    struct dirent *entry;
    char full_path[512];

    while ((entry = readdir(dir))) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                // Recursively delete subdirectory
                clear_directory(full_path);
                rmdir(full_path);
            } else {
                // Delete file
                remove(full_path);
            }
        }
    }
    closedir(dir);
}

// Clear output folder and everything inside it
void clear_output_directory() {
    clear_directory("output");
}


void resize_window_to_surface(SDL_Surface* s) {
    if (!s) return;

    int new_w = s->w + 20;
    int new_h = s->h + BUTTON_HEIGHT + 40;

    const int MAX_WIN_H = 900;
    if (new_h > MAX_WIN_H) new_h = MAX_WIN_H;

    SDL_SetWindowSize(window, new_w, new_h);
}


SDL_Surface* scale_surface_to_fit_window(SDL_Surface* surf, int max_w, int max_h) {
    if (!surf) return NULL;

    int new_w = surf->w;
    int new_h = surf->h;

    // compute scaling factor
    float scale_w = (float)max_w / surf->w;
    float scale_h = (float)max_h / surf->h;
    float scale = (scale_w < scale_h) ? scale_w : scale_h;

    // only scale if necessary
    if (scale < 1.0f) {
        new_w = (int)(surf->w * scale);
        new_h = (int)(surf->h * scale);

        SDL_Surface* scaled = SDL_CreateRGBSurfaceWithFormat(0, new_w, new_h, 32, surf->format->format);
        if (!scaled) return surf;

        SDL_Rect src_rect = {0, 0, surf->w, surf->h};
        SDL_Rect dst_rect = {0, 0, new_w, new_h};
        SDL_BlitScaled(surf, &src_rect, scaled, &dst_rect);

        return scaled;
    }

    return surf; // no scaling needed
}



void draw_button(SDL_Rect rect, const char* text) {
    SDL_SetRenderDrawColor(renderer, 70, 70, 70, 255);
    SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderDrawRect(renderer, &rect);

    if (font) {
        SDL_Color white = {255,255,255,255};
        SDL_Surface* txt_surf = TTF_RenderText_Blended(font, text, white);
        if (txt_surf) {
            SDL_Texture* txt_tex = SDL_CreateTextureFromSurface(renderer, txt_surf);
            SDL_Rect dst = { rect.x + (rect.w - txt_surf->w)/2, rect.y + (rect.h - txt_surf->h)/2, txt_surf->w, txt_surf->h };
            SDL_RenderCopy(renderer, txt_tex, NULL, &dst);
            SDL_DestroyTexture(txt_tex);
            SDL_FreeSurface(txt_surf);
        }
    }
}

// Button actions
void button_load_image() {
    clear_output_directory();

    if (loaded_surface) { SDL_FreeSurface(loaded_surface); loaded_surface = NULL; }
    if (processed_surface) { SDL_FreeSurface(processed_surface); processed_surface = NULL; }
    if (loaded_texture) { SDL_DestroyTexture(loaded_texture); loaded_texture = NULL; }
    if (processed_texture) { SDL_DestroyTexture(processed_texture); processed_texture = NULL; }

    char* path = sdl_input_popup("Enter image path");
    if (!path) return;

    SDL_Surface* img = IMG_Load(path);
    if (!img) { printf("Failed to load %s\n", path); free(path); return; }

    loaded_surface = img;
    SDL_SaveBMP(loaded_surface, "output/loaded.bmp");

    resize_window_to_surface(loaded_surface);
    printf("Image loaded: %s\n", path);
    free(path);
}



void button_preprocess() {
    if (!loaded_surface) { printf("Load image first.\n"); return; }

    SDL_Surface* proc = preprocess_no_denoise(loaded_surface);
    if (!proc) { printf("Preprocessing failed.\n"); return; }

    // Get screen size
    SDL_DisplayMode dm;
    SDL_GetDesktopDisplayMode(0, &dm);
    int max_w = dm.w - 50; // padding
    int max_h = dm.h - 100;

    SDL_Surface* scaled = scale_surface_to_fit_window(proc, max_w, max_h);

    if (processed_surface) SDL_FreeSurface(processed_surface);
    processed_surface = scaled;

    if (scaled != proc) SDL_FreeSurface(proc); // free original if scaled

    SDL_SaveBMP(processed_surface, "output/processed.bmp");
    resize_window_to_surface(processed_surface);  // resize window to new surface
    printf("Preprocessing done.\n");
}

void button_preprocess_denoise() {
    if (!loaded_surface) { printf("Load image first.\n"); return; }

    SDL_Surface* proc = preprocess(loaded_surface);
    if (!proc) { printf("Preprocess+denoise failed.\n"); return; }

    SDL_DisplayMode dm;
    SDL_GetDesktopDisplayMode(0, &dm);
    int max_w = dm.w - 50;
    int max_h = dm.h - 100;

    SDL_Surface* scaled = scale_surface_to_fit_window(proc, max_w, max_h);

    if (processed_surface) SDL_FreeSurface(processed_surface);
    processed_surface = scaled;

    if (scaled != proc) SDL_FreeSurface(proc);

    SDL_SaveBMP(processed_surface, "output/processed.bmp");
    resize_window_to_surface(processed_surface);
    printf("Preprocess+denoise done.\n");
}




void button_segment() {
    if (!processed_surface) { printf("Preprocess image first.\n"); return; }

    SDL_SaveBMP(processed_surface, "output/processed.bmp");

    SDL_Surface *rc_surf = detect_layout("output/processed.bmp");
    if (!rc_surf) { printf("Layout detection failed\n"); return; }
    SDL_FreeSurface(rc_surf);

    extract_cells("output/processed.bmp");
    extract_wordlist("output/processed.bmp");

    SDL_Surface* seg_surf = IMG_Load("output/stage_cells.bmp");
    if (!seg_surf) { printf("Failed to load stage_cells.bmp\n"); return; }

    if (processed_surface) SDL_FreeSurface(processed_surface);
    processed_surface = seg_surf;

    resize_window_to_surface(processed_surface);
    printf("Segmentation done.\n");
}


void button_solve() {
    if (!processed_surface) { printf("Segment the image first.\n"); return; }

    printf("Solver started...\n");

    // always overwrite input
    SDL_SaveBMP(processed_surface, "output/current.bmp");

    remove("output/solved.png");
    remove("output/solver_results.txt");

    int ret = solve_and_highlight(
        "output/processed.bmp",
        "output/solved.png",
        "output/solver_results.txt"
    );

    if (ret == 0) {
        SDL_Surface* sol_surf = IMG_Load("output/solved.png");
        if (!sol_surf) { printf("Failed to load solved.png\n"); return; }

        if (processed_surface) SDL_FreeSurface(processed_surface);
        processed_surface = sol_surf;

        resize_window_to_surface(processed_surface);
        printf("Solver finished.\n");
    }
}

void button_help() {
    const char* message =
    "Word Search Solver Help\n\n"
    "This application provides 5 main buttons for processing word search images:\n"
    "1. Load Image\n"
    "2. Preprocess\n"
    "3. Pre+Denoise\n"
    "4. Segment\n"
    "5. Solve\n\n"
    "The buttons are designed to be used from left to right in order.\n\n"
    "You must choose between 'Preprocess' or 'Pre+Denoise':\n"
    "- Preprocess: Cleans up the image for easier processing.\n"
    "- Pre+Denoise: Cleans up and removes noise (random artifacts, specks, or unwanted marks) from the image.\n"
    "  Use Pre+Denoise if the image is noisy or scanned with artifacts; otherwise, Preprocess is sufficient.\n\n"
    "At each stage, all output files are saved in the 'output' directory in real time.\n"
    "You can check this folder to see the loaded image, processed images, segmented cells, and the final solved result.\n";

    int win_width = 600;
    int win_height = 600;

    SDL_Window* win = SDL_CreateWindow("Help", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       win_width, win_height, SDL_WINDOW_SHOWN);
    if (!win) return;

    SDL_Renderer* rend = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!rend) { SDL_DestroyWindow(win); return; }

    if (!TTF_WasInit()) TTF_Init();
    TTF_Font* popup_font = TTF_OpenFont("DejaVuSans-Bold.ttf", 16);
    if (!popup_font) popup_font = font; // fallback

    // OK button fixed at bottom center
    SDL_Rect ok_button = { (win_width - 100)/2, win_height - 50, 100, 30 };

    int quit = 0;
    SDL_Event e;
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) quit = 1;
            if (e.type == SDL_MOUSEBUTTONDOWN) {
                int mx = e.button.x, my = e.button.y;
                if (SDL_PointInRect(&(SDL_Point){mx,my}, &ok_button)) quit = 1;
            }
        }

        // Background
        SDL_SetRenderDrawColor(rend, 50, 50, 50, 255);
        SDL_RenderClear(rend);

        // Draw text
        if (popup_font) {
            SDL_Color white = {255,255,255,255};
            SDL_Surface* txt_surf = TTF_RenderText_Blended_Wrapped(popup_font, message, white, win_width - 20);
            if (txt_surf) {
                SDL_Texture* tex = SDL_CreateTextureFromSurface(rend, txt_surf);
                SDL_Rect dst = {10, 10, txt_surf->w, txt_surf->h};
                SDL_RenderCopy(rend, tex, NULL, &dst);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(txt_surf);
            }
        }

        // Draw OK button
        SDL_SetRenderDrawColor(rend, 70, 70, 70, 255);
        SDL_RenderFillRect(rend, &ok_button);
        SDL_SetRenderDrawColor(rend, 200, 200, 200, 255);
        SDL_RenderDrawRect(rend, &ok_button);

        if (popup_font) {
            SDL_Color white = {255,255,255,255};
            SDL_Surface* ok_surf = TTF_RenderText_Blended(popup_font, "OK", white);
            if (ok_surf) {
                SDL_Texture* tex = SDL_CreateTextureFromSurface(rend, ok_surf);
                SDL_Rect dst = { ok_button.x + (ok_button.w - ok_surf->w)/2,
                                 ok_button.y + (ok_button.h - ok_surf->h)/2,
                                 ok_surf->w, ok_surf->h };
                SDL_RenderCopy(rend, tex, NULL, &dst);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(ok_surf);
            }
        }

        SDL_RenderPresent(rend);
        SDL_Delay(10);
    }

    TTF_CloseFont(popup_font);
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(win);
}

// MAIN
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    if (SDL_Init(SDL_INIT_VIDEO) < 0) { printf("SDL Init Error: %s\n", SDL_GetError()); return 1; }
    if (!(IMG_Init(IMG_INIT_JPG|IMG_INIT_PNG) & (IMG_INIT_JPG|IMG_INIT_PNG))) { printf("IMG_Init Error: %s\n", IMG_GetError()); SDL_Quit(); return 1; }
    if (TTF_Init() < 0) { printf("TTF_Init Error: %s\n", TTF_GetError()); SDL_Quit(); return 1; }

    font = TTF_OpenFont("DejaVuSans-Bold.ttf", 16);
    if (!font) printf("Warning: failed to load font\n");

    window = SDL_CreateWindow("Word Search GUI", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1000, 400, SDL_WINDOW_SHOWN);
    if (!window) { printf("SDL_CreateWindow Error: %s\n", SDL_GetError()); SDL_Quit(); return 1; }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) { printf("SDL_CreateRenderer Error: %s\n", SDL_GetError()); SDL_DestroyWindow(window); SDL_Quit(); return 1; }

    int quit = 0;
    SDL_Event e;

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) quit = 1;
            if (e.type == SDL_MOUSEBUTTONDOWN) {
                int mx = e.button.x, my = e.button.y;
                if (SDL_PointInRect(&(SDL_Point){mx,my}, &load_button)) button_load_image();
                if (SDL_PointInRect(&(SDL_Point){mx,my}, &preprocess_button)) button_preprocess();
                if (SDL_PointInRect(&(SDL_Point){mx,my}, &preprocess_denoise_button)) button_preprocess_denoise();
                if (SDL_PointInRect(&(SDL_Point){mx,my}, &segment_button)) button_segment();
                if (SDL_PointInRect(&(SDL_Point){mx,my}, &solve_button)) button_solve();
                if (SDL_PointInRect(&(SDL_Point){mx,my}, &help_button)) button_help();
            }
        }

        int y_img = 10;
        
        int win_w, win_h;
	SDL_GetWindowSize(window, &win_w, &win_h);

	int y_buttons = win_h - BUTTON_HEIGHT - 10;

	load_button = (SDL_Rect){10, y_buttons, BUTTON_WIDTH, BUTTON_HEIGHT};
	preprocess_button = (SDL_Rect){10 + BUTTON_WIDTH + BUTTON_MARGIN,
		                       y_buttons, BUTTON_WIDTH, BUTTON_HEIGHT};
	preprocess_denoise_button = (SDL_Rect){10 + 2*(BUTTON_WIDTH + BUTTON_MARGIN),
		                               y_buttons, BUTTON_WIDTH, BUTTON_HEIGHT};
	segment_button = (SDL_Rect){10 + 3*(BUTTON_WIDTH + BUTTON_MARGIN),
		                    y_buttons, BUTTON_WIDTH, BUTTON_HEIGHT};
	solve_button = (SDL_Rect){10 + 4*(BUTTON_WIDTH + BUTTON_MARGIN),
		                  y_buttons, BUTTON_WIDTH, BUTTON_HEIGHT};
	help_button = (SDL_Rect){10 + 5*(BUTTON_WIDTH + BUTTON_MARGIN),
		                 y_buttons, BUTTON_WIDTH, BUTTON_HEIGHT};


        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderClear(renderer);

        if (loaded_surface) {
            SDL_Texture* tex = surface_to_texture(loaded_surface);
            SDL_Rect dst = {10, y_img, loaded_surface->w, loaded_surface->h};
            SDL_RenderCopy(renderer, tex, NULL, &dst);
            SDL_DestroyTexture(tex);
        }

        if (processed_surface) {
            SDL_Texture* tex = surface_to_texture(processed_surface);
            SDL_Rect dst = {10, y_img, processed_surface->w, processed_surface->h};
            SDL_RenderCopy(renderer, tex, NULL, &dst);
            SDL_DestroyTexture(tex);
        }

        draw_button(load_button, "Load Image");
        draw_button(preprocess_button, "Preprocess");
        draw_button(preprocess_denoise_button, "Pre+Denoise");
        draw_button(segment_button, "Segment");
        draw_button(solve_button, "Solve");
        draw_button(help_button, "Help");

        SDL_RenderPresent(renderer);
    }

    if (loaded_surface) SDL_FreeSurface(loaded_surface);
    if (processed_surface) SDL_FreeSurface(processed_surface);
    if (font) TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}
