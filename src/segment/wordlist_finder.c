#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input_image> <wordlist_roi.txt>\n", argv[0]);
        return 1;
    }
    const char *imagePath = argv[1];
    const char *roiPath   = argv[2];
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }
    if (!(IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) & (IMG_INIT_PNG | IMG_INIT_JPG))) {
        fprintf(stderr, "SDL_image init Error: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_Surface *image = IMG_Load(imagePath);
    if (!image) {
        fprintf(stderr, "Could not load image: %s\n", IMG_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    // Read word list ROI coordinates
    FILE *froi = fopen(roiPath, "r");
    if (!froi) {
        fprintf(stderr, "Cannot open ROI file: %s\n", roiPath);
        SDL_FreeSurface(image);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    int roi_x, roi_y, roi_w, roi_h;
    if (fscanf(froi, "%d %d %d %d", &roi_x, &roi_y, &roi_w, &roi_h) != 4) {
        fprintf(stderr, "Invalid ROI format in %s\n", roiPath);
        fclose(froi);
        SDL_FreeSurface(image);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    fclose(froi);
    // Clamp ROI to image bounds
    if (roi_x < 0) roi_x = 0;
    if (roi_y < 0) roi_y = 0;
    if (roi_x + roi_w > image->w) roi_w = image->w - roi_x;
    if (roi_y + roi_h > image->h) roi_h = image->h - roi_y;
    if (roi_w <= 0 || roi_h <= 0) {
        fprintf(stderr, "ROI is out of image bounds or empty\n");
        SDL_FreeSurface(image);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Convert ROI to binary (black & white)
    int rw = roi_w;
    int rh = roi_h;
    Uint8 *binary = malloc(rw * rh);
    if (!binary) {
        fprintf(stderr, "Memory allocation failed\n");
        SDL_FreeSurface(image);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    if (SDL_MUSTLOCK(image)) SDL_LockSurface(image);
    for (int j = 0; j < rh; ++j) {
        for (int i = 0; i < rw; ++i) {
            int img_x = roi_x + i;
            int img_y = roi_y + j;
            Uint8 r, g, b;
            Uint32 pixel;
            Uint8 *pbase = (Uint8*)image->pixels + img_y * image->pitch + img_x * image->format->BytesPerPixel;
            switch (image->format->BytesPerPixel) {
                case 1: pixel = *pbase; break;
                case 2: pixel = *(Uint16*)pbase; break;
                case 3: {
                    Uint8 byte0 = pbase[0], byte1 = pbase[1], byte2 = pbase[2];
                    if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
                        pixel = byte0 << 16 | byte1 << 8 | byte2;
                    else
                        pixel = byte0 | byte1 << 8 | byte2 << 16;
                    break;
                }
                case 4: pixel = *(Uint32*)pbase; break;
                default: pixel = 0;
            }
            SDL_GetRGB(pixel, image->format, &r, &g, &b);
            Uint16 gray = (Uint16)r + g + b;
            gray /= 3;
            binary[j * rw + i] = (gray < 128 ? 1 : 0);
        }
    }
    if (SDL_MUSTLOCK(image)) SDL_UnlockSurface(image);
    SDL_FreeSurface(image);

    // (Optional) Remove outer border if present, similar logic as in cell_finder
    // Compute projections to detect full lines/columns of black
    int *histY = calloc(rh, sizeof(int));
    int *histX = calloc(rw, sizeof(int));
    if (!histY || !histX) {
        fprintf(stderr, "Memory allocation failed\n");
        free(histY); free(histX); free(binary);
        IMG_Quit(); SDL_Quit();
        return 1;
    }
    for (int y = 0; y < rh; ++y) {
        int count = 0;
        for (int x = 0; x < rw; ++x) { if (binary[y * rw + x]) count++; }
        histY[y] = count;
    }
    for (int x = 0; x < rw; ++x) {
        int count = 0;
        for (int y = 0; y < rh; ++y) { if (binary[y * rw + x]) count++; }
        histX[x] = count;
    }
    int top_cut = 0, bottom_cut = 0, left_cut = 0, right_cut = 0;
    while (top_cut < rh && histY[top_cut] > 0.95 * rw) top_cut++;
    while (bottom_cut < rh && histY[rh-1-bottom_cut] > 0.95 * rw) bottom_cut++;
    while (left_cut < rw && histX[left_cut] > 0.95 * rh) left_cut++;
    while (right_cut < rw && histX[rw-1-right_cut] > 0.95 * rh) right_cut++;
    free(histY);
    free(histX);
    int off_x = left_cut;
    int off_y = top_cut;
    int new_rw = rw - left_cut - right_cut;
    int new_rh = rh - top_cut - bottom_cut;
    if (new_rw <= 0 || new_rh <= 0) {
        fprintf(stderr, "ROI is mostly border, no content\n");
        free(binary);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Prepare for connected-component analysis (BFS)
    int maxPixels = new_rw * new_rh;
    int *queue_x = malloc(maxPixels * sizeof(int));
    int *queue_y = malloc(maxPixels * sizeof(int));
    if (!queue_x || !queue_y) {
        fprintf(stderr, "Memory allocation failed\n");
        free(queue_x); free(queue_y);
        free(binary);
        IMG_Quit(); SDL_Quit();
        return 1;
    }
    int letterIndex = 0;
    // Visit each pixel in the trimmed ROI area
    for (int by = off_y; by < off_y + new_rh; ++by) {
        for (int bx = off_x; bx < off_x + new_rw; ++bx) {
            if (binary[by * rw + bx] == 1) {  // found an unvisited black pixel (letter)
                // Perform BFS to get the whole connected component (letter)
                letterIndex++;
                int qh = 0, qt = 0;
                // Enqueue starting pixel
                queue_x[qt] = bx;
                queue_y[qt] = by;
                qt++;
                binary[by * rw + bx] = 0;  // mark visited (set to white to avoid reusing)
                // Bounding box for this component
                int min_x = bx, max_x = bx;
                int min_y = by, max_y = by;
                while (qh < qt) {
                    int cx = queue_x[qh];
                    int cy = queue_y[qh];
                    qh++;
                    // Check 8-connected neighbors
                    for (int dy = -1; dy <= 1; ++dy) {
                        for (int dx = -1; dx <= 1; ++dx) {
                            if (dx == 0 && dy == 0) continue;
                            int nx = cx + dx;
                            int ny = cy + dy;
                            // Ensure (nx, ny) is within trimmed ROI bounds
                            if (nx < off_x || nx >= off_x + new_rw || ny < off_y || ny >= off_y + new_rh) {
                                continue;
                            }
                            if (binary[ny * rw + nx] == 1) {
                                // Mark and enqueue neighbor pixel
                                binary[ny * rw + nx] = 0;
                                queue_x[qt] = nx;
                                queue_y[qt] = ny;
                                qt++;
                                // update bounding box
                                if (nx < min_x) min_x = nx;
                                if (nx > max_x) max_x = nx;
                                if (ny < min_y) min_y = ny;
                                if (ny > max_y) max_y = ny;
                            }
                        }
                    }
                }
                // Compute component width and height
                int comp_w = max_x - min_x + 1;
                int comp_h = max_y - min_y + 1;
                // Heuristic: skip components that are too large to be a single letter
                if ((comp_w > new_rw * 0.8 && comp_h > new_rh * 0.8) || (qt > maxPixels / 2)) {
                    // This might be a large non-letter region (e.g., a border or artifact), skip it
                    qt = 0;
                    qh = 0;
                    continue;
                }
                // Create surface for the letter
                SDL_Surface *letterSurface = SDL_CreateRGBSurface(0, comp_w, comp_h, 32,
                                                                  0x00FF0000, 0x0000FF00, 0x000000FF, 0);
                if (!letterSurface) {
                    fprintf(stderr, "SDL_CreateRGBSurface failed: %s\n", SDL_GetError());
                    continue;
                }
                Uint32 white = SDL_MapRGB(letterSurface->format, 255, 255, 255);
                Uint32 black = SDL_MapRGB(letterSurface->format, 0, 0, 0);
                SDL_FillRect(letterSurface, NULL, white);
                // Plot the pixels from the component (we can use the stored queue coordinates)
                for (int k = 0; k < qt; ++k) {
                    int px = queue_x[k] - min_x;
                    int py = queue_y[k] - min_y;
                    Uint32 *dstPixels = (Uint32*) letterSurface->pixels;
                    dstPixels[py * (letterSurface->pitch / 4) + px] = black;
                }
                // Save letter image
                char filename[32];
                snprintf(filename, sizeof(filename), "letter_%d.bmp", letterIndex);
                if (SDL_SaveBMP(letterSurface, filename) != 0) {
                    fprintf(stderr, "Error saving %s: %s\n", filename, SDL_GetError());
                }
                SDL_FreeSurface(letterSurface);
                // Reset queue for next component
                qt = 0;
                qh = 0;
            }
        }
    }

    // Clean up
    free(queue_x);
    free(queue_y);
    free(binary);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
