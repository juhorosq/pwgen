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
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "llist.h"

struct Node *mknode(char const *name, char const *symbols, size_t size)
{
	struct Node *new_node = malloc(sizeof(*new_node));

	new_node->name = calloc(strlen(name) + 1, sizeof(*(new_node->name)));
	strcpy(new_node->name, name);

	new_node->data = malloc((size + 1) * sizeof(*(new_node->data)));
	strncpy(new_node->data, symbols, size);
	new_node->data[size] = '\0';  // if strlen(symbols) < size, strncpy will pad with zeroes

	new_node->size = size;
	new_node->next = NULL;  // this node is not yet part of any list

	return new_node;
}

struct Node *list_append(struct Node *llist, struct Node *new_node)
{
	struct Node *p = llist;

	// seek to the end of the list
	while (p->next) p = p->next;

	p->next = new_node;
	return p->next;
}

struct Node *list_seek(struct Node *pos, const char *key)
{
	struct Node *p = pos;

	while (p) {
		if (strcmp(p->name, key) == 0)
			break;
		else
			p = p->next;
	}
	return p;
}
