# Compiler
CC = gcc
CFLAGS = -Wall -Wextra

# SDL2 flags (only for preprocess)
SDL_CFLAGS = $(shell sdl2-config --cflags)
SDL_LDFLAGS = $(shell sdl2-config --libs) -lSDL2_image -lm

# Executable names
PREPROCESS_NAME = preprocess
NN_NAME = neural_net

# Source files
PREPROCESS_SRC = img/preprocess.c
NN_SRC = ocr/neural_net.c

# Object files
PREPROCESS_OBJ = $(PREPROCESS_SRC:.c=.o)
NN_OBJ = $(NN_SRC:.c=.o)

# Default target: build both
all: $(PREPROCESS_NAME) $(NN_NAME)

# preprocess target (SDL2)
$(PREPROCESS_NAME): $(PREPROCESS_OBJ)
	$(CC) $(PREPROCESS_OBJ) -o $@ $(SDL_CFLAGS) $(SDL_LDFLAGS)

# neural_net target
$(NN_NAME): $(NN_OBJ)
	$(CC) $(NN_OBJ) -o $@ $(CFLAGS) -lm

# Generic compilation rule
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean object files
clean:
	rm -f $(PREPROCESS_OBJ) $(NN_OBJ)

# Clean all executables and objects
fclean: clean
	rm -f $(PREPROCESS_NAME) $(NN_NAME)

# Rebuild everything
re: fclean all

# Run targets
run-preprocess: $(PREPROCESS_NAME)
	./$(PREPROCESS_NAME)

run-nn: $(NN_NAME)
	./$(NN_NAME)

.PHONY: all clean fclean re run-preprocess run-nn
