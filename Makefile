CC=gcc
CFLAGS?=
CFLAGS+=-Wall -Wextra -Wpedantic

PREFIX?=.

quiz: main.c
	$(CC) $(CFLAGS) -o $@ $<

install:
	install -Dm755 quiz $(PREFIX)/bin/quiz

clean:
	rm -rf quiz *.o

.PHONY: install clean
