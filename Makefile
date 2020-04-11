# This file is part of pwgen.
# Copyright (C) 2005-2020 Juho Rosqvist
#
# Pwgen is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Pwgen is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with pwgen.  If not, see <https://www.gnu.org/licenses/>.

SHELL = /bin/sh

srcdir := src
hdrdir := include

bindir := bin
depdir := dep
objdir := obj

trg := $(bindir)/pwgen
dbg_suff := debug
tst_suff := test

srcfiles := $(wildcard $(srcdir)/*.c)
depfiles := $(patsubst $(srcdir)/%.c,$(depdir)/%.d,$(srcfiles))
objfiles := $(patsubst $(srcdir)/%.c,$(objdir)/%.o,$(srcfiles))

# Per-target variables; apply to their dependencies as well.
$(trg)             : CFLAGS += -O2 -DNDEBUG
$(trg)-$(tst_suff) : CFLAGS += -g $(sanitizers)
$(trg)-$(dbg_suff) : CFLAGS += -g -Og -DDEBUG_PRINT $(sanitizers)

sanitizers =
sanitizers += -fsanitize=address
sanitizers += -fsanitize=undefined
sanitizers += -fsanitize=null
sanitizers += -fsanitize=return
sanitizers += -fsanitize=bounds
sanitizers += -fsanitize=signed-integer-overflow
sanitizers += -fsanitize=object-size
sanitizers += -fsanitize=enum

CC = gcc
CFLAGS += -std=c99 -Wall -Wpedantic
CPPFLAGS += -iquote $(hdrdir)

# Use the C preprocessor to auto-generate dependencies from source files;
# the dependency files are included at the end of this file, ane make will
# automatically re-run itself if any of the included files is updated.
MAKEDEPEND = $(CC) -E -MM -MF $(patsubst $(srcdir)/%.c,$(depdir)/%.d,$<) \
 -MT $(patsubst $(srcdir)/%.c,$(objdir)/%.o,$<) \
 -MT $(patsubst $(srcdir)/%.c,$(depdir)/%.d,$<) \
 $(CPPFLAGS) $<

COMPILE.c = $(CC) -c -o $@ $(CFLAGS) $(CPPFLAGS) $<
COMPILE.o = $(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^

.PHONY: all
all : $(trg)
	./run-tests.sh $<

.PHONY: debug demo test clean realclean

debug : $(trg)-$(dbg_suff)

demo : $(trg)
	./run-demo.sh $<
test : $(trg)-$(tst_suff)
	./run-tests.sh $<

clean :
	rm -f $(wildcard $(objdir)/*.o $(depdir)/*.d)
realclean :
	rm -rf $(bindir) $(depdir) $(objdir)

$(trg) : $(objfiles) | $(bindir)
	$(COMPILE.o)
$(trg)-$(dbg_suff) : $(patsubst %.o,%-$(dbg_suff).o,$(objfiles)) | $(bindir)
	$(COMPILE.o)
$(trg)-$(tst_suff) : $(patsubst %.o,%-$(tst_suff).o,$(objfiles)) | $(bindir)
	$(COMPILE.o)

$(objdir)/%.o : $(srcdir)/%.c | $(objdir)
	$(COMPILE.c)
$(objdir)/%-$(dbg_suff).o : $(srcdir)/%.c | $(objdir)
	$(COMPILE.c)
$(objdir)/%-$(tst_suff).o : $(srcdir)/%.c | $(objdir)
	$(COMPILE.c)

$(bindir) $(depdir) $(objdir) :
	mkdir -p $@

# Auto-generate dependencies.
$(depdir)/%.d : $(srcdir)/%.c | $(depdir)
	$(MAKEDEPEND)

# Prevent deletion of depfiles as intermediate targets.
$(depfiles) :

# Don't update dependency information when cleaning.
ifeq (,$(trim $(filter clean realclean,$(MAKECMDGOALS))))
-include $(depfiles)
endif
