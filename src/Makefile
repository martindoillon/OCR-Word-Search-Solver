# Compiler and flags
CC = gcc
CFLAGS = -Wall -O2

# Executables
EXECS = img_exec ocr_exec segment_exec solver_exec ui_exec

# Source files
IMG_SRC = img/img_io.c img/preprocess.c
OCR_SRC = ocr/neural_net.c
SEGMENT_SRC = segment/cell_finder.c segment/detect_layout.c segment/wordlist_finder.c
SOLVER_SRC = solver/grid.c solver/search.c
UI_SRC = ui/gui.c ui/render.c

# Default target: build all executables
all: $(EXECS)

# Build img executable
img_exec: $(IMG_SRC)
	$(CC) $(CFLAGS) -o img_exec $(IMG_SRC)

# Build ocr executable
ocr_exec: $(OCR_SRC)
	$(CC) $(CFLAGS) -o ocr_exec $(OCR_SRC)

# Build segment executable
segment_exec: $(SEGMENT_SRC)
	$(CC) $(CFLAGS) -o segment_exec $(SEGMENT_SRC)

# Build solver executable
solver_exec: $(SOLVER_SRC)
	$(CC) $(CFLAGS) -o solver_exec $(SOLVER_SRC)

# Build ui executable
ui_exec: $(UI_SRC)
	$(CC) $(CFLAGS) -o ui_exec $(UI_SRC)

# Clean all executables
clean:
	rm -f $(EXECS)
