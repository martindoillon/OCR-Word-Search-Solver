#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

#define INPUT_SIZE 784
#define HIDDEN_SIZE 512
#define OUTPUT_SIZE 26

//-------------------------------------------------------------
// Utility
//-------------------------------------------------------------
float frand() { return ((float)rand() / RAND_MAX) * 0.1f - 0.05f; }
float sigmoid(float x) { return 1.0f / (1.0f + expf(-x)); }

void softmax(float* z, float* out) {
    float maxv = z[0];
    for (int i = 1; i < OUTPUT_SIZE; i++)
        if (z[i] > maxv) maxv = z[i];

    float sum = 0.0f;
    for (int i = 0; i < OUTPUT_SIZE; i++) {
        out[i] = expf(z[i] - maxv);
        sum += out[i];
    }
    for (int i = 0; i < OUTPUT_SIZE; i++)
        out[i] /= sum;
}

//-------------------------------------------------------------
// MLP Structure
//-------------------------------------------------------------
typedef struct {
    float W1[HIDDEN_SIZE][INPUT_SIZE];
    float b1[HIDDEN_SIZE];
    float W2[OUTPUT_SIZE][HIDDEN_SIZE];
    float b2[OUTPUT_SIZE];
} MLP;

void init_mlp(MLP* net) {
    for (int i = 0; i < HIDDEN_SIZE; i++) {
        net->b1[i] = frand();
        for (int j = 0; j < INPUT_SIZE; j++)
            net->W1[i][j] = frand();
    }
    for (int i = 0; i < OUTPUT_SIZE; i++) {
        net->b2[i] = frand();
        for (int j = 0; j < HIDDEN_SIZE; j++)
            net->W2[i][j] = frand();
    }
}

//-------------------------------------------------------------
// Save / Load Weights
//-------------------------------------------------------------
int save_mlp(const char* filename, MLP* net) {
    FILE* f = fopen(filename, "wb");
    if (!f) return -1;
    fwrite(net->W1, sizeof(float), HIDDEN_SIZE*INPUT_SIZE, f);
    fwrite(net->b1, sizeof(float), HIDDEN_SIZE, f);
    fwrite(net->W2, sizeof(float), OUTPUT_SIZE*HIDDEN_SIZE, f);
    fwrite(net->b2, sizeof(float), OUTPUT_SIZE, f);
    fclose(f);
    return 0;
}

int load_mlp(const char* filename, MLP* net) {
    FILE* f = fopen(filename, "rb");
    if (!f) return -1;
    fread(net->W1, sizeof(float), HIDDEN_SIZE*INPUT_SIZE, f);
    fread(net->b1, sizeof(float), HIDDEN_SIZE, f);
    fread(net->W2, sizeof(float), OUTPUT_SIZE*HIDDEN_SIZE, f);
    fread(net->b2, sizeof(float), OUTPUT_SIZE, f);
    fclose(f);
    return 0;
}

//-------------------------------------------------------------
// IDX loader
//-------------------------------------------------------------
int read_int(FILE* f) {
    uint8_t b[4];
    fread(b, 1, 4, f);
    return (b[0]<<24) | (b[1]<<16) | (b[2]<<8) | b[3];
}

float** load_images(const char* path, int* count) {
    FILE* f = fopen(path, "rb");
    if (!f) { printf("Cannot open %s\n", path); exit(1); }

    read_int(f); // magic
    int n = read_int(f);
    int rows = read_int(f);
    int cols = read_int(f);

    *count = n;
    int size = rows * cols;
    float** images = malloc(n * sizeof(float*));
    for (int i = 0; i < n; i++) images[i] = malloc(size * sizeof(float));

    for (int i = 0; i < n; i++)
        for (int j = 0; j < size; j++) {
            unsigned char pixel;
            fread(&pixel, 1, 1, f);
            images[i][j] = pixel / 255.0f;
        }
    fclose(f);
    return images;
}

unsigned char* load_labels(const char* path, int* count) {
    FILE* f = fopen(path, "rb");
    if (!f) { printf("Cannot open %s\n", path); exit(1); }

    read_int(f); // magic
    int n = read_int(f);
    *count = n;
    unsigned char* labels = malloc(n);
    fread(labels, 1, n, f);
    fclose(f);
    return labels;
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
void train_one(MLP* net, float* x, unsigned char label, float lr) {
    float hidden[HIDDEN_SIZE];
    float out[OUTPUT_SIZE];
    forward(net, x, hidden, out);

    float target[OUTPUT_SIZE] = {0};
    target[label - 1] = 1.0f;

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
// Test accuracy
//-------------------------------------------------------------
int argmax(float* v) {
    int idx = 0;
    for (int i = 1; i < OUTPUT_SIZE; i++)
        if (v[i] > v[idx]) idx = i;
    return idx;
}

float test_accuracy(MLP* net, float** images, unsigned char* labels, int count) {
    int correct = 0;
    float hidden[HIDDEN_SIZE], out[OUTPUT_SIZE];
    for (int i = 0; i < count; i++) {
        forward(net, images[i], hidden, out);
        int pred = argmax(out) + 1;
        if (pred == labels[i]) correct++;
    }
    return (float)correct / count;
}

//-------------------------------------------------------------
// Main
//-------------------------------------------------------------
int main() {
    srand(time(NULL));

    printf("Loading EMNIST...\n");
    int train_n, test_n;

    float** train_images = load_images("emnist-letters-train-images-idx3-ubyte", &train_n);
    unsigned char* train_labels = load_labels("emnist-letters-train-labels-idx1-ubyte", &train_n);

    float** test_images = load_images("emnist-letters-test-images-idx3-ubyte", &test_n);
    unsigned char* test_labels = load_labels("emnist-letters-test-labels-idx1-ubyte", &test_n);

    MLP net;
    if (load_mlp("emnist_weights.bin", &net) != 0) {
        printf("No existing weights, initializing randomly...\n");
        init_mlp(&net);
    } else {
        printf("Loaded weights from emnist_weights.bin\n");
    }

    float lr = 0.01f;
    int epochs = 10;

    for (int e = 0; e < epochs; e++) {
        printf("Epoch %d...\n", e+1);
        for (int i = 0; i < train_n; i++)
            train_one(&net, train_images[i], train_labels[i], lr);

        float acc = test_accuracy(&net, test_images, test_labels, test_n);
        printf("Accuracy: %.2f%%\n", acc * 100.0f);

        save_mlp("emnist_weights.bin", &net);
    }

    printf("Training complete, weights saved to emnist_weights.bin\n");
    return 0;
}

