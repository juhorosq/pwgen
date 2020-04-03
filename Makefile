SHELL = /bin/sh

trg        := bin/pwgen
trg-debug  := bin/pwgen-debug
trg-test   := bin/pwgen-test

.PHONY: all debug demo test clean mrproper
all : $(trg) test

CC = gcc
CFLAGS = -g -Wall

$(trg)       : CFLAGS += -O2 -DNDEBUG
$(trg-test)  : CFLAGS += $(sanitizers)
$(trg-debug) : CFLAGS += -Og -DDEBUG_PRINT $(sanitizers)

sanitizers =
sanitizers += -fsanitize=address
sanitizers += -fsanitize=undefined
sanitizers += -fsanitize=null
sanitizers += -fsanitize=return
sanitizers += -fsanitize=bounds
sanitizers += -fsanitize=signed-integer-overflow
sanitizers += -fsanitize=object-size
sanitizers += -fsanitize=enum


$(trg) : pwgen.c
	$(CC) -o $@ $(CFLAGS) $<
$(trg-test) : pwgen.c
$(trg-debug) : pwgen.c
	$(CC) -o $@ $(CFLAGS) $<

debug : $(trg-debug)

demo : $(trg)
	./run-demo.sh $<

test : $(trg-test)
	./run-tests.sh $<

clean :
	rm -f $(trg-debug)

mrproper : clean
	rm -f $(trg) $(trg-test)
