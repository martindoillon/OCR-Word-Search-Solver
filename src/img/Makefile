CC = gcc
CFLAGS = -Wall -O2 `sdl2-config --cflags`
LDFLAGS = `sdl2-config --libs` -lSDL2_image -lm

# Source files
PREPROCESS = preprocess.c

# Targets
all: preprocess_test

preprocess_test: preprocess_test.c $(PREPROCESS)
	$(CC) $(CFLAGS) -o $@ preprocess_test.c $(PREPROCESS) $(LDFLAGS)

clean:
	rm -f preprocess_test *.o

.PHONY: all clean

