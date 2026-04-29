CC = gcc
CFLAGS = -Wall

all: orion eternal

orion: orion.c arena.h
	$(CC) $(CFLAGS) orion.c -o orion

eternal: eternal.c arena.h
	$(CC) $(CFLAGS) eternal.c -o eternal

clean:
	rm -f orion eternal