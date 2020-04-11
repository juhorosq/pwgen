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
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "llist.h"
#include "gensyms.h"

void init_symbol_sets(struct Node **llist_root)
{
	struct Node *iter;  // list iterator
	char syms[128];  // buffer for building strings
	size_t len;      // length of string in the buffer

	assert(*llist_root == NULL);  // only operate on empty list

	// printable ASCII characters, including space (32--126)
	len = fill_ascii_range(syms, ' ', '~');
	*llist_root = iter = mknode("asciip", syms, len);  // insert root node manually

	// printable ASCII characters, without space (33--126)
	len = fill_ascii_range(syms, '!', '~');
	iter = list_append(iter, mknode("asciipns", syms, len));

	// numbers 0-9 (ASCII 48--57)
	len = fill_ascii_range(syms, '0', '9');
	iter = list_append(iter, mknode("num", syms, len));

	// uppercase letters (65--90)
	len = fill_ascii_range(syms, 'A', 'Z');
	iter = list_append(iter, mknode("ALPHA", syms, len));

	// lowercase letters (97--122)
	len = fill_ascii_range(syms, 'a', 'z');
	iter = list_append(iter, mknode("alpha", syms, len));

	// don't repeat yourself: reuse above symbol sets)
	struct Node *q;     // temporary node pointer

	// all letters (ALPHA + alpha)
	q = list_seek(*llist_root, "ALPHA"); assert(q);
	strcpy(syms, q->data); len = q->size;
	q = list_seek(*llist_root, "alpha"); assert(q);
	strcpy(syms + len, q->data); len += q->size;
	iter = list_append(iter, mknode("Alpha", syms, len));

	// uppercase alphanumeric characters (ALPHA + num)
	q = list_seek(*llist_root, "ALPHA"); assert(q);
	strcpy(syms, q->data); len = q->size;
	q = list_seek(*llist_root, "num"); assert(q);
	strcpy(syms + len, q->data); len += q->size;
	iter = list_append(iter, mknode("ALNUM", syms, len));

	// lowercase alphanumeric characters (alpha + num)
	q = list_seek(*llist_root, "alpha"); assert(q);
	strcpy(syms, q->data); len = q->size;
	q = list_seek(*llist_root, "num"); assert(q);
	strcpy(syms + len, q->data); len += q->size;
	iter = list_append(iter, mknode("alnum", syms, len));

	// uppercase & lowercase alphanumeric characters (Alpha + num)
	q = list_seek(*llist_root, "Alpha"); assert(q);
	strcpy(syms, q->data); len = q->size;
	q = list_seek(*llist_root, "num"); assert(q);
	strcpy(syms + len, q->data); len += q->size;
	iter = list_append(iter, mknode("Alnum", syms, len));

	// punctuation characters (33--47, 58--64, 91--96, 123--126)
	len = fill_ascii_range(syms, '!', '/');
	len += fill_ascii_range(syms + len, ':', '@');
	len += fill_ascii_range(syms + len, '[', '`');
	len += fill_ascii_range(syms + len, '{', '~');
	iter = list_append(iter, mknode("punct", syms, len));
}

void free_symbol_sets(struct Node **llist_root)
{
	struct Node *iter = *llist_root;

	while (iter) {
		struct Node *next = iter->next;
		debug_print("freeing: 0x%016x: %s = {%s}", iter, iter->name, iter->data);
		free(iter->name); free(iter->data); free(iter);
		iter = next;
	}
	*llist_root = NULL;
}

size_t fill_ascii_range(char *dest, char first, char last)
{
	int i;

	for (i = 0; first + i <= last; ++i)
		dest[i] = first + i;
	assert(last - first + 1 == i);

	return i;
}

