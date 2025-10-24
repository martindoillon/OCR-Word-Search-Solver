#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

// Sigmoid activation function
float sigmoid(float z) {
    return 1.0f / (1.0f + expf(-z));
}

// Derivative of sigmoid
float sigmoid_prime(float z) {
    float s = sigmoid(z);
    return s * (1.0f - s);
}

// Structure
typedef struct {
    float w1, w2, w3, w4, w5, w6;  // Weights
    float b1, b2, b3;              // Biases
} XORNetwork;

// Initialize weights and biases randomly
void init_network(XORNetwork* net) {
    srand(time(NULL));
    net->w1 = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    net->w2 = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    net->w3 = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    net->w4 = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    net->w5 = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    net->w6 = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    net->b1 = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    net->b2 = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    net->b3 = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
}

// Feedforward
float feedforward(XORNetwork* net, float a, float b) {
    // Hidden layer
    float h1 = sigmoid(net->w1 * a + net->w2 * b + net->b1);
    float h2 = sigmoid(net->w3 * a + net->w4 * b + net->b2);
    // Output layer
    return sigmoid(net->w5 * h1 + net->w6 * h2 + net->b3);
}

// Train the network on XOR
void train(XORNetwork* net, float eta, int epochs) {
    float training_inputs[4][2] = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};
    float training_outputs[4] = {0, 1, 1, 0};

    for (int e = 0; e < epochs; e++) {
        float total_error = 0.0f;

        for (int i = 0; i < 4; i++) {
            float a = training_inputs[i][0];
            float b = training_inputs[i][1];
            float target = training_outputs[i];

            // Feedforward
            float h1 = sigmoid(net->w1 * a + net->w2 * b + net->b1);
            float h2 = sigmoid(net->w3 * a + net->w4 * b + net->b2);
            float output = sigmoid(net->w5 * h1 + net->w6 * h2 + net->b3);

            // Calculate error
            float error = target - output;
            total_error += fabs(error);

            // Backpropagation
            // Output layer gradient
            float delta_output = error * sigmoid_prime(net->w5 * h1 + net->w6 * h2 + net->b3);
            // Hidden layer gradients
            float delta_h1 = delta_output * net->w5 * sigmoid_prime(net->w1 * a + net->w2 * b + net->b1);
            float delta_h2 = delta_output * net->w6 * sigmoid_prime(net->w3 * a + net->w4 * b + net->b2);

            // Update weights and biases
            net->w5 += eta * delta_output * h1;
            net->w6 += eta * delta_output * h2;
            net->b3 += eta * delta_output;

            net->w1 += eta * delta_h1 * a;
            net->w2 += eta * delta_h1 * b;
            net->b1 += eta * delta_h1;

            net->w3 += eta * delta_h2 * a;
            net->w4 += eta * delta_h2 * b;
            net->b2 += eta * delta_h2;
        }

        printf("Epoch %d, Error: %.4f\n", e, total_error);
    }
}

void test(XORNetwork* net) {
    printf("\nTesting trained network:\n");
    printf("0 XOR 0 = %.4f\n", feedforward(net, 0, 0));
    printf("0 XOR 1 = %.4f\n", feedforward(net, 0, 1));
    printf("1 XOR 0 = %.4f\n", feedforward(net, 1, 0));
    printf("1 XOR 1 = %.4f\n", feedforward(net, 1, 1));
}

int main() {
    XORNetwork net;
    init_network(&net);

    printf("Training XOR network...\n");
    train(&net, 0.5f, 10000);

    test(&net);

    return 0;
}
