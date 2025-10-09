#include <stdio.h>
#include <stdlib.h>
#include <MagickWand/MagickWand.h>




int main(int argc, char **argv) 
{
    if (argc < 2) 
    {
	fprintf(stderr, "pas de fihier fournis%s <input_image>\n", argv[0]);
        return 1;
    }

    const char *input_path = argv[1];

    MagickWandGenesis();
    MagickWand *wand = NewMagickWand();

    if (MagickReadImage(wand, input_path) == MagickFalse) 
    {
        fprintf(stderr, "erreur lors de la lecture de l'image '%s'\n", input_path);
        DestroyMagickWand(wand);
        MagickWandTerminus();
        return 1;
    }

    MagickSetImageType(wand, GrayscaleType);

    MagickThresholdImage(wand, QuantumRange/2);

    if (MagickWriteImage(wand, "output_bw.png") == MagickFalse) 
    {
        char *description = MagickGetException(wand, NULL);
        fprintf(stderr, "Erreur : %s\n", description);
        MagickRelinquishMemory(description);
    } 
    
    else 
    {
        printf("Image 'output_bw.png' créée avec succès !\n");
    }

    wand = DestroyMagickWand(wand);
    MagickWandTerminus();

    return 0;
}

