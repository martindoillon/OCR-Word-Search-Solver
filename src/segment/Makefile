CC = gcc
CFLAGS = -Wall -O2 `sdl2-config --cflags`
LDFLAGS = `sdl2-config --libs` -lSDL2_image

# Source files
UTILS = utils.c
LAYOUT = layout_detect.c
CELL = cell_finder.c
WORD = wordlist_finder.c

# Targets
all: dl_test cf_test wf_test

dl_test: dl_test.c $(LAYOUT) $(UTILS)
	$(CC) $(CFLAGS) -o $@ dl_test.c $(LAYOUT) $(UTILS) $(LDFLAGS)

cf_test: cf_test.c $(CELL) $(UTILS)
	$(CC) $(CFLAGS) -o $@ cf_test.c $(CELL) $(UTILS) $(LDFLAGS)

wf_test: wf_test.c $(WORD) $(UTILS)
	$(CC) $(CFLAGS) -o $@ wf_test.c $(WORD) $(UTILS) $(LDFLAGS)

clean:
	rm -f dl_test cf_test wf_test *.o

.PHONY: all clean

