# pwgen
Random password generator

This is a small program that creates randomized strings from predefined or
user-provided sets of symbols, or a combination of these. Although it tries to
produce randomness of reasonable quality, I have not tested this rigorously.

The program is **not intended for serious security applications**.

## Dependencies

### Build Time Dependencies

No dependencies besides C standard library.
Any C compiler supporting C99 or later should build the program.

Tested with GCC on Debian GNU/Linux.

### Run Time Dependencies

The program reads data from the host system's `/dev/urandom` device to seed
its pseudo-random number generator. That device is only available on
Unix-flavored systems. A work-around for other systems ought to be trivial.

## Building

A Makefile for GNU Make and GCC is provided for convenience; the command `make`
will build the program. Alternatively, it is very simple to build by directly
invoking your compiler, for example `gcc -o pwgen pwgen.c` suffices on my
system.

## Using

Use `./pwgen -h` on the command line to see usage instructions and option
descriptions.  You may also use the command `make demo` to see some example
invocations of the program.

For more details, study the source code.

## License and Disclaimers

`pwgen` Copyright (C) 2005-2020 Juho Rosqvist

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License in the [LICENSE](LICENSE)
file or <https://gnu.org/licenses/gpl.html> for more details.

