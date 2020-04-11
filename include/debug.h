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
#ifndef PWGEN_DEBUG_H
#define PWGEN_DEBUG_H

#ifdef DEBUG_PRINT
#  define DEBUG_PRINT 1
#else
#  define DEBUG_PRINT 0
#endif

#define debug_print(fmt, ...) do { \
	if (DEBUG_PRINT) \
	debug_info(__FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__); \
} while (0)

void debug_info(const char *file, int line, const char *func, const char *fmt, ...);

#endif // PWGEN_DEBUG_H
