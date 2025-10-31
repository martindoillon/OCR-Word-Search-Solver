CC      ?= gcc
CFLAGS  ?= -O2 -Wall -Wextra
LDFLAGS ?= -lSDL2 -lSDL2_image -lm

ALL = detect_layout cell_finder wordlist_finder

all: $(ALL)

detect_layout: detect_layout.c
	$(CC) detect_layout.c -o $@ $(CFLAGS) $(LDFLAGS)

cell_finder: cell_finder.c
	$(CC) cell_finder.c -o $@ $(CFLAGS) $(LDFLAGS)

wordlist_finder: wordlist_finder.c
	$(CC) wordlist_finder.c -o $@ $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(ALL) *.o *.bmp *.csv *_roi.txt
	rm -rf output

.PHONY: all clean
