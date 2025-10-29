OCR Word Search Solver Project

This repository contains the source code for a project that solves word search puzzles using OCR and a neural network. The src directory contains all source files and a Makefile to build the programs.

Requirements:

- GCC (or any compatible C compiler)
- SDL2 and SDL2_image development libraries

Installing SDL2 on Linux:

Debian/Ubuntu:
sudo apt update
sudo apt install build-essential libsdl2-dev libsdl2-image-dev

Building the project:

1. Navigate to the src directory:
cd src

2. Run make to build both executables:
make

3. Optionally, run the programs:
   
./preprocess       # Image loading, grayscale, contrast, and manual rotation

./neural_net       # Neural network proof-of-concept

Cleaning build files:

To remove object files:
make clean

To remove object files and executables:
make fclean

To rebuild everything from scratch:
make re
