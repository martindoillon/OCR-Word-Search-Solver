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

// Neural network structure
typedef struct {
    float w1, w2, w3, w4, w5, w6;  // Weights
    float b1, b2, b3;              // Biases
} XNORNetwork;

// Initialize weights and biases randomly
void init_network(XNORNetwork* net) {
    srand(time(NULL));
    net->w1 = ((float)rand() / RAND_MAX) * 1.0f - 0.5f;
    net->w2 = ((float)rand() / RAND_MAX) * 1.0f - 0.5f;
    net->w3 = ((float)rand() / RAND_MAX) * 1.0f - 0.5f;
    net->w4 = ((float)rand() / RAND_MAX) * 1.0f - 0.5f;
    net->w5 = ((float)rand() / RAND_MAX) * 1.0f - 0.5f;
    net->w6 = ((float)rand() / RAND_MAX) * 1.0f - 0.5f;
    net->b1 = ((float)rand() / RAND_MAX) * 1.0f - 0.5f;
    net->b2 = ((float)rand() / RAND_MAX) * 1.0f - 0.5f;
    net->b3 = ((float)rand() / RAND_MAX) * 1.0f - 0.5f;
}

// Feedforward
float feedforward(XNORNetwork* net, float a, float b) {
    float h1_input = net->w1 * a + net->w2 * b + net->b1;
    float h2_input = net->w3 * a + net->w4 * b + net->b2;
    float h1 = sigmoid(h1_input);
    float h2 = sigmoid(h2_input);
    float out_input = net->w5 * h1 + net->w6 * h2 + net->b3;
    return sigmoid(out_input);
}

// Train the network on XNOR
void train(XNORNetwork* net, float eta, int epochs) {
    float training_inputs[4][2] = { {0,0}, {0,1}, {1,0}, {1,1} };
    float training_outputs[4] = {1, 0, 0, 1}; // XNOR target

    for (int e = 0; e < epochs; e++) {
        float total_error = 0.0f;

        for (int i = 0; i < 4; i++) {
            float a = training_inputs[i][0];
            float b = training_inputs[i][1];
            float target = training_outputs[i];

            // Forward pass
            float h1_input = net->w1 * a + net->w2 * b + net->b1;
            float h2_input = net->w3 * a + net->w4 * b + net->b2;
            float h1 = sigmoid(h1_input);
            float h2 = sigmoid(h2_input);
            float out_input = net->w5 * h1 + net->w6 * h2 + net->b3;
            float output = sigmoid(out_input);

            // Error
            float error = target - output;
            total_error += fabs(error);

            // Backpropagation
            float delta_output = error * sigmoid_prime(out_input);
            float delta_h1 = delta_output * net->w5 * sigmoid_prime(h1_input);
            float delta_h2 = delta_output * net->w6 * sigmoid_prime(h2_input);

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

        if (e % 1000 == 0) { // print every 1000 epochs
            printf("Epoch %d, Total Error: %.4f\n", e, total_error);
        }
    }
}

// Test the network
void test(XNORNetwork* net) {
    printf("\nTesting trained network (XNOR):\n");
    printf("0 XNOR 0 = %.4f\n", feedforward(net, 0, 0));
    printf("0 XNOR 1 = %.4f\n", feedforward(net, 0, 1));
    printf("1 XNOR 0 = %.4f\n", feedforward(net, 1, 0));
    printf("1 XNOR 1 = %.4f\n", feedforward(net, 1, 1));
}

int main() {
    XNORNetwork net;
    init_network(&net);

    printf("Training XNOR network...\n");
    train(&net, 0.5f, 10000);

    test(&net);

    return 0;
}
