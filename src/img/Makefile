# Makefile for preprocess.c

CC = gcc
CFLAGS = -Wall -Wextra `sdl2-config --cflags` -I/usr/include/SDL2
LDFLAGS = `sdl2-config --libs` -lSDL2_image -lm
TARGET = preprocess
SRC = preprocess.c
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
