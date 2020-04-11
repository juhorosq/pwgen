/* This file is part of pwgen.
 * Copyright (C) 2005-2020 Juho Rosqvist
 *
 * Pwgen is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * You should have received a copy of the GNU General Public License
 * along with pwgen.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef PWGEN_GENSYMS_H
#define PWGEN_GENSYMS_H

#include "llist.h"

/* Initialize the predefined symbol sets.
 *
 * Note: the size variable of a node contains the number of characters in its
 * corresponding data string, excluding the terminating zero. That is,
 * strlen(n.data) == n.size, and the memory allocated for the data string is
 * equal to (size+1)*sizeof(char).
 *
 * The reason for all these dynamic allocations over static strings in the
 * program .text section is the desire to define them by ASCII character
 * ranges. This system also automates the string length calculations, which
 * should eliminate some pesky errors in case one modifies these character
 * sets.
 */
void init_symbol_sets(struct Node **llist_root);

/* Free the memory allocated for predefined symbol sets. They are only used
 * during the program setup, after which they may be freed.
 */
void free_symbol_sets(struct Node **llist_root);

/* Replace characters from the start of the string dest with the ASCII values
 * between characters first and last (inclusive).
 * Return the number of characters replaced (i.e. #last - #first + 1).
 *
 * Make sure that dest has enough space to hold the characters.
 */
size_t fill_ascii_range(char *dest, char first, char last);

#endif
