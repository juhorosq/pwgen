SHELL = /bin/sh
CC = gcc
CFLAGS = -g -O2 -Wall

DEBUGCFLAGS = -g -Og -Wall
#DEBUGCFLAGS = -DDEBUG_PRINT
DEBUGCFLAGS += -fsanitize=address
DEBUGCFLAGS += -fsanitize=undefined
DEBUGCFLAGS += -fsanitize=null
DEBUGCFLAGS += -fsanitize=return
DEBUGCFLAGS += -fsanitize=bounds
DEBUGCFLAGS += -fsanitize=signed-integer-overflow
DEBUGCFLAGS += -fsanitize=object-size
DEBUGCFLAGS += -fsanitize=enum

debug_bin = pwgen-debug
demo_bin  = $(debug_bin)

pwgen : pwgen.c
	$(CC) -o $@ -DNDEBUG $(CFLAGS) $<

$(debug_bin) : pwgen.c
	$(CC) -o $@ $(DEBUGCFLAGS) $<

.PHONY: debug demo
debug : $(debug_bin)
demo : $(demo_bin)
	./$(demo_bin) -v
	./$(demo_bin) -h
	./$(demo_bin)
	./$(demo_bin) -l 16 -c 3
	for i in 1 2 3; do ./$(demo_bin) -l 16 -c 1; done
	./$(demo_bin) -S num
	./$(demo_bin) -c 10 -S ALPHA -- -+= .:
	./$(demo_bin) -l 50 -c 10 -S ALPHA -S ALPHA -S alpha  # Twice as many letters in uppercase as in lowercase
	./$(demo_bin) -l 10 -c 20 ________x  # One x per word (on average)
	time ./$(demo_bin) -l 11 -c 30000000 " Hello" | (grep "Hello Hello" || true)  # should take less than 10 seconds
