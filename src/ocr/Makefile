# Makefile for neural_net.c

CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -lm
TARGET = neural_net
SRC = neural_net.c
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
