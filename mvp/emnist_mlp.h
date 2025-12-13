#ifndef EMNIST_MLP_H
#define EMNIST_MLP_H

#define INPUT_SIZE 784
#define HIDDEN_SIZE 128
#define OUTPUT_SIZE 26
#define WEIGHTS_FILE "mlp_weights.bin"

typedef struct {
    float W1[HIDDEN_SIZE][INPUT_SIZE];
    float b1[HIDDEN_SIZE];
    float W2[OUTPUT_SIZE][HIDDEN_SIZE];
    float b2[OUTPUT_SIZE];
} MLP;


void init_mlp(MLP* net);
int save_mlp(const char* filename, MLP* net);
int load_mlp(const char* filename, MLP* net);
void forward(MLP* net, float* input, float* hidden, float* output);
void train_one(MLP* net, float* x, int label, float lr);
int argmax(float* v);

#endif

