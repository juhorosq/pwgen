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

#include "debug.h"
#include "random.h"

int rand_lt(int upper_bound)
{
	int r;

	/* Since stdlib's rand() is uniformly distributed on [0,RAND_MAX],
	 * we use rejection sampling to ensure that the result is also
	 * uniformly distributed on our range [0, upper_bound - 1].
	 */
	int reject_bound = RAND_MAX - (RAND_MAX % upper_bound);
	assert(reject_bound % upper_bound == 0);
	while ((r = rand()) >= reject_bound);

	return r % upper_bound;
}

char *str_randomize(char *str, size_t char_count, const char *symbols, size_t len_symbols)
{
	for (size_t i = 0 ; i < char_count ; ++i) {
		str[i] = symbols[rand_lt(len_symbols)];
	}

	return str;
}
