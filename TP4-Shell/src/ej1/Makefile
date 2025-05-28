CC      := gcc
CFLAGS  := -Wall -Wextra -std=c11

.PHONY: all clean test

all: ring tester

ring: ring.c
	$(CC) $(CFLAGS) -o $@ ring.c

tester: tester.c
	$(CC) $(CFLAGS) -o $@ tester.c

test: ring tester
	./tester.sh

clean:
	rm -f ring tester
