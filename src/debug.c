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
#include <stdarg.h>
#include <stdio.h>

#include "debug.h"

void debug_info(const char *file, int line, const char *func, const char *fmt, ...)
{
	va_list argp;

	fprintf(stderr, "%s:%d:%s: ", file, line ,func);
	va_start(argp, fmt);
	vfprintf(stderr, fmt, argp);
	va_end(argp);
	fprintf(stderr, "\n");
}
