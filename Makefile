SHELL = /bin/sh
CC = gcc
CFLAGS = -g -O2 -Wall

DEBUGCFLAGS = -g -Og -Wall
#DEBUGCFLAGS += -DDEBUG_PRINT
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
	./$(demo_bin) --help
	./$(demo_bin)
	./$(demo_bin) -l 16 -c 3
	for i in 1 2 3; do ./$(demo_bin) -l 16 -c 1; done
	for i in 1 2 3; do ./$(demo_bin) -l 16 -c 1 --random-seed=no_such_file 2>/dev/null; done
	./$(demo_bin) -l 16 -c 1 --random-seed=no_such_file
	./$(demo_bin) --symbols=num
	./$(demo_bin) --count 10 --length=20 -Snum -- -+= .:
	./$(demo_bin) -l 50 -c 10 -S ALPHA -S ALPHA -S alpha  # 2/3rds uppercase, 1/3rd lowercase
	./$(demo_bin) -l 10 -c 20 ________x  # One x per word (on average)
	time ./$(demo_bin) -l 11 -c 79000000 -r/dev/zero " Hello" | grep "Hello Hello"  # should take about 10 seconds
