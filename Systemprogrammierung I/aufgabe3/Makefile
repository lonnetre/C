.PHONY: all clean

CC = gcc
CFLAGS = -std=c11 -pedantic -Wall -Werror -D_XOPEN_SOURCE=700

all: clash

clash: clash.o plist.o
	$(CC) $(CFLAGS) $^ -o $@

clash.o: clash.c plist.h
	$(CC) $(CFLAGS) -c $< -o $@

plist.o: plist.c plist.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o clash