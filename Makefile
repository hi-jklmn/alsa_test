CC=gcc
CLIBS=-lasound
CFLAGS=-Wall -std=c99 -ggdb
EXE=alsa_test

target: main.c
	$(CC) $(CFLAGS) $(CLIBS) main.c -o $(EXE)

run: target
	./$(EXE)

