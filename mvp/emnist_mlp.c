#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <dirent.h>
#include <string.h>

#define INPUT_SIZE 784
#define HIDDEN_SIZE 128
#define OUTPUT_SIZE 26
#define WEIGHTS_FILE "mlp_weights.bin"

float frand() { return ((float)rand()/RAND_MAX)*0.1f - 0.05f; }
float sigmoid(float x) { return 1.0f / (1.0f + expf(-x)); }

void softmax(float* z, float* out) {
    float maxv = z[0];
    for (int i=1;i<OUTPUT_SIZE;i++) if(z[i]>maxv) maxv=z[i];
    float sum=0.0f;
    for(int i=0;i<OUTPUT_SIZE;i++){out[i]=expf(z[i]-maxv);sum+=out[i];}
    for(int i=0;i<OUTPUT_SIZE;i++) out[i]/=sum;
}

typedef struct {
    float W1[HIDDEN_SIZE][INPUT_SIZE];
    float b1[HIDDEN_SIZE];
    float W2[OUTPUT_SIZE][HIDDEN_SIZE];
    float b2[OUTPUT_SIZE];
} MLP;

void init_mlp(MLP* net){
    for(int i=0;i<HIDDEN_SIZE;i++){
        net->b1[i]=frand();
        for(int j=0;j<INPUT_SIZE;j++) net->W1[i][j]=frand();
    }
    for(int i=0;i<OUTPUT_SIZE;i++){
        net->b2[i]=frand();
        for(int j=0;j<HIDDEN_SIZE;j++) net->W2[i][j]=frand();
    }
}

int save_mlp(const char* filename, MLP* net){
    FILE* f=fopen(filename,"wb");
    if(!f)return -1;
    if(fwrite(net->W1,sizeof(float),HIDDEN_SIZE*INPUT_SIZE,f)!=HIDDEN_SIZE*INPUT_SIZE) {fclose(f); return -1;}
    if(fwrite(net->b1,sizeof(float),HIDDEN_SIZE,f)!=HIDDEN_SIZE) {fclose(f); return -1;}
    if(fwrite(net->W2,sizeof(float),OUTPUT_SIZE*HIDDEN_SIZE,f)!=OUTPUT_SIZE*HIDDEN_SIZE) {fclose(f); return -1;}
    if(fwrite(net->b2,sizeof(float),OUTPUT_SIZE,f)!=OUTPUT_SIZE) {fclose(f); return -1;}
    fclose(f); return 0;
}

int load_mlp(const char* filename, MLP* net){
    FILE* f=fopen(filename,"rb");
    if(!f)return -1;
    if(fread(net->W1,sizeof(float),HIDDEN_SIZE*INPUT_SIZE,f)!=HIDDEN_SIZE*INPUT_SIZE) {fclose(f); return -1;}
    if(fread(net->b1,sizeof(float),HIDDEN_SIZE,f)!=HIDDEN_SIZE) {fclose(f); return -1;}
    if(fread(net->W2,sizeof(float),OUTPUT_SIZE*HIDDEN_SIZE,f)!=OUTPUT_SIZE*HIDDEN_SIZE) {fclose(f); return -1;}
    if(fread(net->b2,sizeof(float),OUTPUT_SIZE,f)!=OUTPUT_SIZE) {fclose(f); return -1;}
    fclose(f); return 0;
}

//-------------------------------------------------------------
// Forward pass
//-------------------------------------------------------------
void forward(MLP* net, float* input, float* hidden, float* output) {
    for (int i = 0; i < HIDDEN_SIZE; i++) {
        float z = net->b1[i];
        for (int j = 0; j < INPUT_SIZE; j++)
            z += net->W1[i][j] * input[j];
        hidden[i] = sigmoid(z);
    }

    float z2[OUTPUT_SIZE];
    for (int i = 0; i < OUTPUT_SIZE; i++) {
        float z = net->b2[i];
        for (int j = 0; j < HIDDEN_SIZE; j++)
            z += net->W2[i][j] * hidden[j];
        z2[i] = z;
    }

    softmax(z2, output);
}

//-------------------------------------------------------------
// Training (SGD)
//-------------------------------------------------------------
void train_one(MLP* net, float* x, int label, float lr) {
    float hidden[HIDDEN_SIZE], out[OUTPUT_SIZE];
    forward(net, x, hidden, out);

    float target[OUTPUT_SIZE] = {0};
    target[label] = 1.0f;

    float delta2[OUTPUT_SIZE];
    for (int i = 0; i < OUTPUT_SIZE; i++) delta2[i] = out[i] - target[i];

    float delta1[HIDDEN_SIZE] = {0};
    for (int i = 0; i < HIDDEN_SIZE; i++) {
        float sum = 0.0f;
        for (int j = 0; j < OUTPUT_SIZE; j++)
            sum += net->W2[j][i] * delta2[j];
        delta1[i] = sum * hidden[i] * (1 - hidden[i]);
    }

    for (int i = 0; i < OUTPUT_SIZE; i++) {
        net->b2[i] -= lr * delta2[i];
        for (int j = 0; j < HIDDEN_SIZE; j++)
            net->W2[i][j] -= lr * delta2[i] * hidden[j];
    }

    for (int i = 0; i < HIDDEN_SIZE; i++) {
        net->b1[i] -= lr * delta1[i];
        for (int j = 0; j < INPUT_SIZE; j++)
            net->W1[i][j] -= lr * delta1[i] * x[j];
    }
}

//-------------------------------------------------------------
// Argmax / Accuracy
//-------------------------------------------------------------
int argmax(float* v) {
    int idx = 0;
    for (int i = 1; i < OUTPUT_SIZE; i++)
        if (v[i] > v[idx]) idx = i;
    return idx;
}

float test_accuracy(MLP* net, float** images, int* labels, int count) {
    int correct = 0;
    float hidden[HIDDEN_SIZE], out[OUTPUT_SIZE];
    for (int i = 0; i < count; i++) {
        forward(net, images[i], hidden, out);
        if (argmax(out) == labels[i]) correct++;
    }
    return (float)correct / count;
}

//-------------------------------------------------------------
// Load 28x28 BMP image
//-------------------------------------------------------------
int load_bmp(const char* path, float* input) {
    SDL_Surface* surface = SDL_LoadBMP(path);
    if (!surface) {
        fprintf(stderr, "Failed to load %s\n", path);
        return -1;
    }

    if (surface->w != 28 || surface->h != 28) {
        fprintf(stderr, "Error: BMP %s not 28x28\n", path);
        SDL_FreeSurface(surface);
        return -1;
    }

    SDL_LockSurface(surface);
    uint32_t* pixels = (uint32_t*)surface->pixels;
    for (int y = 0; y < 28; y++)
        for (int x = 0; x < 28; x++) {
            uint32_t px = pixels[y * surface->pitch / 4 + x];
            uint8_t r = px & 0xFF;
            input[y * 28 + x] = r / 255.0f;
        }
    SDL_UnlockSurface(surface);
    SDL_FreeSurface(surface);
    return 0;
}

//-------------------------------------------------------------
// Load dataset from output folder
//-------------------------------------------------------------
int load_dataset(const char* folder, float*** images_out, int** labels_out) {
    DIR* dir = opendir(folder);
    if (!dir) { perror("opendir"); return -1; }

    int count = 0;
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL)
        if (strstr(entry->d_name, ".bmp")) count++;
    rewinddir(dir);

    float** images = malloc(sizeof(float*) * count);
    int* labels = malloc(sizeof(int) * count);

    int idx = 0;
    char path[512];
    while ((entry = readdir(dir)) != NULL) {
        if (!strstr(entry->d_name, ".bmp")) continue;
        snprintf(path, sizeof(path), "%s/%s", folder, entry->d_name);
        images[idx] = malloc(sizeof(float) * 784);
        if (load_bmp(path, images[idx]) != 0) {
            free(images[idx]);
            continue;
        }

        labels[idx] = entry->d_name[0] - 'A';
        idx++;
    }
    closedir(dir);

    *images_out = images;
    *labels_out = labels;
    return idx;
}

//-------------------------------------------------------------
// Shuffle
//-------------------------------------------------------------
void shuffle_dataset(float** images, int* labels, int n) {
    for (int i = n-1; i > 0; i--) {
        int j = rand() % (i+1);
        float* tmp_img = images[i];
        images[i] = images[j];
        images[j] = tmp_img;
        int tmp_lbl = labels[i];
        labels[i] = labels[j];
        labels[j] = tmp_lbl;
    }
}



//-------------------------------------------------------------
// Main
//-------------------------------------------------------------
#ifdef EMNIST_MLP_MAIN
int main() {
    srand(time(NULL));
    SDL_Init(SDL_INIT_VIDEO);

    MLP net;

    // Attempt to load existing weights
    if (load_mlp(WEIGHTS_FILE, &net) == 0)
        printf("Loaded existing weights from '%s'.\n", WEIGHTS_FILE);
    else {
        printf("No saved weights found, initializing new network.\n");
        init_mlp(&net);
    }

    printf("Loading dataset...\n");
    float** images;
    int* labels;
    int n = load_dataset("output", &images, &labels);
    printf("Loaded %d images\n", n);

    float lr = 0.01f;
    int epochs = 50;

    for (int e = 0; e < epochs; e++) {
        shuffle_dataset(images, labels, n);
        for (int i = 0; i < n; i++)
            train_one(&net, images[i], labels[i], lr);

        float acc = test_accuracy(&net, images, labels, n);
        printf("Epoch %d/%d, Accuracy: %.2f%%\n", e+1, epochs, acc*100.0f);
    }

    printf("\nPredictions on first 20 images:\n");
    float hidden[HIDDEN_SIZE], out[OUTPUT_SIZE];
    for (int i = 0; i < 20 && i < n; i++) {
        forward(&net, images[i], hidden, out);
        int pred = argmax(out);
        printf("File %d: True=%c, Pred=%c\n", i, 'A'+labels[i], 'A'+pred);
    }

    save_mlp(WEIGHTS_FILE, &net);
    printf("Weights saved to '%s'.\n", WEIGHTS_FILE);

    for (int i = 0; i < n; i++) free(images[i]);
    free(images);
    free(labels);

    SDL_Quit();
    return 0;
}
#endif



