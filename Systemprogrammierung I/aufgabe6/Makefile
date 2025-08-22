CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99
OBJ = creeper.o argumentParser.o
EXEC = creeper

all: $(EXEC)

.PHONY: all clean distclean

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c argumentParser.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f creeper.o $(EXEC)

distclean: clean
	rm -f argumentParser.o
