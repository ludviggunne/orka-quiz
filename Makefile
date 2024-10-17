CC=gcc
CFLAGS?=
CFLAGS+=-Wall -Wextra -Wpedantic

quiz: main.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf quiz *.o
