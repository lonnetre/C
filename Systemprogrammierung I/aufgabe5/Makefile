CC = gcc
CFLAGS = -std=c11 -pedantic -Wall -Werror -D_GNU_SOURCE

.PHONY: all clean test

all: test test-ref

test: test.o halde.o
	$(CC) $(CFLAGS) -o $@ $^
	./$@

test-ref: test.o halde-ref.o
	$(CC) $(CFLAGS) -o $@ $^
	./$@

%.o: %.c halde.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f test test-ref halde.o test.o
