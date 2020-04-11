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
#ifndef PWGEN_LLIST_H
#define PWGEN_LLIST_H

typedef struct Node Node;
struct Node { // node for accessing symbol sets in a traversable (forward linked) list
	char *name;    // user-facing name of this symbol set
	char *data;    // pointer to the actual symbols string
	size_t size;   // number of characters in the symbols string (excluding '\0')

	struct Node *next;  // link to the next node in the list
};

/* Add *new_node to the end of the linked list that contains the *llist Node.
 * Return a pointer to the newly added node.
 *
 * Note that list_append cannot add to an empty list. Use mknode to create
 * a list (of a single node) first and then list_append more to that node.
 */
struct Node *list_append(struct Node *llist, struct Node *new_node);

/* Find the first occurrence of a node named key in the linked list after
 * (and including) the position pointed to by pos. Return a pointer to that
 * node, or NULL if no match was found until end of list was reached.
 */
struct Node *list_seek(struct Node *pos, const char *key);

/* Allocate memory for a new linked list Node for our symbol sets list,
 * initialize it with values from (name, data, size) and return a pointer
 * to this node.
 * Note that mknode will produce a detached node, you have to add it
 * to a list yourself, e.g. using list_append.
 */
struct Node *mknode(char const *name, char const *data, size_t size);

#endif
