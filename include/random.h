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
#ifndef PWGEN_RANDOM_H
#define PWGEN_RANDOM_H

/* Return a (uniformly distributed) random integer from the
 * interval [0, upper_bound - 1].
 * Note: upper_bound may not exceed RAND_MAX.
 *
 * Remember to seed the RNG first!
 */
int rand_lt(int upper_bound);

/* Overwrite the first n characters of str with random characters from
 * the beginning (first len_symbols characters) of the symbols string.
 *
 * The appearance of every character from symbols is equally likely; if a
 * character is repeated in symbols string, it is twice as likely to appear in
 * str, etc.
 */
char *str_randomize(char *str, size_t len, const char *symbols, size_t len_symbols);

#endif
