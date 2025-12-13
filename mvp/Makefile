# Compiler to use
CC = gcc

# Compiler flags (adjust as needed, -Wall is highly recommended)
# -g adds debugging symbols
CFLAGS = -Wall -Wextra

# Linker flags using sdl2-config for portability
LDFLAGS = `sdl2-config --cflags --libs` -lSDL2_image -lSDL2_ttf -lm

# Name of the final executable
TARGET = gui

# List of all source files
SRCS = gui.c \
       preprocess.c \
       layout_detect.c \
       cell_finder.c \
       wordlist_finder.c \
       predict_letters.c \
       utils.c \
       emnist_mlp.c \
       rebuild_puzzle.c \
       solver.c \
       highlight.c

# Automatically generate the list of object files (.o) from source files (.c)
OBJS = $(SRCS:.c=.o)

# Default target: builds the executable
all: $(TARGET)

# Rule to link the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Generic rule to compile .c files into .o files
# $< refers to the prerequisite (the .c file)
# $@ refers to the target (the .o file)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Phony targets don't correspond to actual files
.PHONY: all clean

# Clean target: removes generated files
clean:
	rm -f $(TARGET) $(OBJS)
