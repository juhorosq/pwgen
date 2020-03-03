SHELL = /bin/sh
CC = gcc
CFLAGS = -g -O2 -Wall
DEBUGCFLAGS = -g -Og -Wall -fsanitize=address \
 -fsanitize=undefined -fsanitize=null -fsanitize=return -fsanitize=bounds \
 -fsanitize=signed-integer-overflow -fsanitize=object-size -fsanitize=enum

pwgen : pwgen.c
	$(CC) -o $@ -DNDEBUG $(CFLAGS) $<

pwgen-debug : pwgen.c
	$(CC) -o $@ $(DEBUGCFLAGS) $<

.PHONY: debug demo
debug : pwgen-debug
demo : pwgen
	./pwgen
