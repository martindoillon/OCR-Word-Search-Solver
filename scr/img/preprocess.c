#include <stdio.h>
#include <stdlib.h>
#include <MagickWand/MagickWand.h>
#include <math.h>

#define MIN(a,b) ((a) < (b) ? (a) : (b))

// Function to rotate image manually
void rotate_image(MagickWand *wand, double angle) {
    PixelWand *bg = NewPixelWand();
    PixelSetColor(bg, "white"); // Fill background with white after rotation
    MagickRotateImage(wand, bg, angle);
    bg = DestroyPixelWand(bg);
}

// Function to automatically detect skew angle (simple estimation using Hough transform approximation)
double detect_skew_angle(MagickWand *wand) {
    // MagickWand does not provide direct skew detection,
    // so a simple approach is to compute orientation via deskew
    // using MagickDeskewImage, which tries to find the skew automatically
    MagickBooleanType success = MagickDeskewImage(wand, 0.40 * QuantumRange); // threshold 40%
    if (success == MagickFalse) {
        return 0.0;
    }
    // Deskew already applied; return 0 as angle (MagickDeskewImage rotates internally)
    return 0.0;
}

int main(int argc, char **argv) 
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_image> [manual_angle]\n", argv[0]);
        return 1;
    }

    const char *input_path = argv[1];
    double manual_angle = 0.0;
    if (argc >= 3) {
        manual_angle = atof(argv[2]); // Optional: user can provide manual rotation angle
    }

    MagickWandGenesis();
    MagickWand *wand = NewMagickWand();

    if (MagickReadImage(wand, input_path) == MagickFalse) {
        fprintf(stderr, "Error reading image '%s'\n", input_path);
        DestroyMagickWand(wand);
        MagickWandTerminus();
        return 1;
    }

    // Manual deskew if user provided an angle
    if (fabs(manual_angle) > 0.01) {
        rotate_image(wand, manual_angle);
    }

    // Automatic deskew
    detect_skew_angle(wand);

    // Noise reduction
    MagickDespeckleImage(wand);

    // Contrast enhancement
    MagickContrastStretchImage(wand, 0.1); // stretch contrast by 10% shadows/highlights

    // Convert to grayscale
    MagickSetImageType(wand, GrayscaleType);

    // Convert to black & white (thresholding)
    MagickThresholdImage(wand, QuantumRange/2);

    // Save output
    if (MagickWriteImage(wand, "output_bw.png") == MagickFalse) {
        char *description = MagickGetException(wand, NULL);
        fprintf(stderr, "Error: %s\n", description);
        MagickRelinquishMemory(description);
    } else {
        printf("Image 'output_bw.png' created successfully!\n");

