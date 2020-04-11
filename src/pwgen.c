/*  pwgen - randomized password generator
 *  Copyright (C) 2005-2020 Juho Rosqvist
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <getopt.h>
//#include <unistd.h>

#include "debug.h"
#include "gensyms.h"
#include "llist.h"
#include "random.h"

#define PROGRAM_NAME "pwgen"
#define VERSION "0.6.0"

#define AUTHORS "Juho Rosqvist"

struct Configuration {
	int pwcount;         // how many random passwords to generate
	size_t pwlen;        // the length of each generated password
	size_t len_active_symbols;  // length of the string containing allowed characters
	char *active_symbols;       // string of characters allowed in password generation
	char *seed_file;     // name of the file whence the random seed is read
	Node *symbol_sets;   // points to the root of the list of predefined symbol sets
};

#define DEFAULT_pwcount 1
#define DEFAULT_pwlen 8
#define DEFAULT_seed_file "/dev/urandom"
#define DEFAULT_symbols "asciipns"

size_t activate_symbols(struct Configuration *conf, const char *src);
void configure(struct Configuration *conf, int argc, char **argv);
unsigned int get_RNG_seed(char const *file_name);

enum usage_flag { help, brief, full, symbol_sets, version };
void usage(enum usage_flag topic, const struct Configuration *conf);


/* This program generates randomized passwords out of a customizable set of
 * characters.
 *
 * For more details, run the program with -h flag or read the source ;-)
 */
int main(int argc, char **argv)
{
	struct Configuration conf = { 0, 0, 0, NULL, NULL, NULL };

	init_symbol_sets(&(conf.symbol_sets));  // predefined symbol sets
	assert(list_seek(conf.symbol_sets, DEFAULT_symbols));
	configure(&conf, argc, argv);           // apply command options & defaults
	free_symbol_sets(&(conf.symbol_sets));

	assert(conf.len_active_symbols == strlen(conf.active_symbols));
	assert(0 < conf.len_active_symbols);
	assert(conf.len_active_symbols <= RAND_MAX);

	srand(get_RNG_seed(conf.seed_file));
	free(conf.seed_file); conf.seed_file = NULL;

	// ensure string termination by zeroing it first
	char *password = calloc(conf.pwlen + 1, sizeof(*password));

	for (int i = 0 ; i < conf.pwcount ; ++i) {
		str_randomize(password, conf.pwlen,
		              conf.active_symbols, conf.len_active_symbols);
		puts(password);
	}
	free(password);
	free(conf.active_symbols);

	return EXIT_SUCCESS;
}


/* Get a seed for the pseudo-random number generator from a system source.
 *
 * Recommended source is /dev/urandom since it may not block on read,
 * unlike /dev/random. Obviously, this only works on (most) *nix systems.
 * Will fall back to system time (predictable) if opening the file fails.
 */
unsigned int get_RNG_seed(char const *file_name)
{
	unsigned int rseed;

	FILE *fp = fopen(file_name, "rb");
	if (fp) {
		fread(&rseed, sizeof(rseed), 1, fp);
		fclose(fp);
	}
	else {
		perror(file_name);
		fprintf(stderr, "%s\n%s\n"
		       , "WARNING: fallback: using system time as random seed"
		       , "WARNING: system time is predictable!"
			   );
		rseed = time(0);
	}

	return rseed;
}

/* Process the command line and set program configuration accordingly.
 *
 * Command line interface is GNU getopt style. Program will terminate if
 * options -h or -v are encountered, or if the command line is invalid.
 */
void configure(struct Configuration *conf, int argc, char **argv)
{
	// apply defaults (symbols default is applied at the end if nothing is selected)
	conf->pwcount = DEFAULT_pwcount;
	conf->pwlen   = DEFAULT_pwlen;
	conf->seed_file = malloc((strlen(DEFAULT_seed_file) + 1) * sizeof(*(conf->seed_file)));
	strcpy(conf->seed_file, DEFAULT_seed_file);

	// activate_symbols calls realloc on conf->active_symbols,
	// so memory for the string must be allocated beforehand
	conf->len_active_symbols = 0;
	conf->active_symbols = calloc(conf->len_active_symbols + 1, sizeof(*(conf->active_symbols)));

	struct option longopts[] = {
		// { char* name, int has_arg,       int *flag, int val },
		{ "symbols",     required_argument, NULL,      'S' },
		{ "count",       required_argument, NULL,      'c' },
		{ "length",      required_argument, NULL,      'l' },
		{ "random-seed", required_argument, NULL,      'r' },
		{ "help",        no_argument,       NULL,      'h' },
		{ "version",     no_argument,       NULL,      'v' },
		{ 0, 0, 0, 0 }
	};

	int opt;           // holds the option character returned by getopt*
	int option_index;  // getopt_long stores the option index to longopts here
	struct Node *p;    // list iterator

	// process command line options
	while ((opt = getopt_long(argc, argv, "S:c:l:r:hv", longopts, &option_index)) != -1) {
		switch (opt) {
			case 'S':
				if (strcmp(optarg, "help") == 0) {
					usage(symbol_sets, conf);
					exit(EXIT_SUCCESS);
				}

				p = list_seek(conf->symbol_sets, optarg);
				if (p) {
					activate_symbols(conf, p->data);
				}
				else {
					fprintf(stderr, "%s: no such symbol set: %s\n"
						   , argv[0], optarg);
					fprintf(stderr, "Try `%s --help` or `%s --symbols=help`\n"
						   , PROGRAM_NAME, PROGRAM_NAME);
					exit(EXIT_FAILURE);
				}
				break;
			case 'c':
				conf->pwcount = atoi(optarg);
				break;
			case 'h':
				usage(full, conf);
				exit(EXIT_SUCCESS);
				break;
			case 'l':
				conf->pwlen = atoi(optarg);
				break;
			case 'r':
				conf->seed_file = realloc(conf->seed_file, (strlen(optarg) + 1) * sizeof(*(conf->seed_file)));
				if (conf->seed_file) {
					strcpy(conf->seed_file, optarg);
				}
				else {
					free(conf->seed_file);
					fprintf(stderr, "%s: memory allocation failed\n", argv[0]);
					exit(EXIT_FAILURE);
				}
				break;
			case 'v':
				usage(version, conf);
				exit(EXIT_SUCCESS);
				break;
			case '?': // invalid option; getopt_long already printed an error message
				usage(help, conf);
				exit(EXIT_FAILURE);
				break;
			default:
				fprintf(stderr, "%s: unexpected error while parsing options!\n", argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	// process non-option arguments as (partial) character pool definitions
	for (int i = optind; i < argc; ++i) {
		activate_symbols(conf, argv[i]);
	}
	if (conf->len_active_symbols == 0) // use default symbols if none were selected
		activate_symbols(conf, list_seek(conf->symbol_sets, DEFAULT_symbols)->data);
}

/* Add characters from a zero-terminated string src to the allowed symbols pool
 * conf->active_symbols (reallocating memory as needed) and update symbols count
 * conf->len_active_symbols.
 * Failing to reallocate memory is fatal, and terminates the program.
 *
 * Return the number of characters added to conf->active_symbols.
 */
size_t activate_symbols(struct Configuration *conf, const char *src)
{
	size_t len = strlen(src);
	debug_print("%s(%s) len=%zu conf->len=%zu\n", __func__, src, len, conf->len_active_symbols);
	debug_print(" before realloc:     &symbols=0x%016x      symbols={%s}", conf->active_symbols, conf->active_symbols);

	char *new_symbols = realloc(conf->active_symbols, (conf->len_active_symbols + len + 1) * sizeof(*new_symbols));
	debug_print(" after realloc:  &new_symbols=0x%016x  new_symbols={%s}", new_symbols, new_symbols);
	if (new_symbols) {
		strcat(new_symbols, src);
		debug_print(" after strcat(new_symbols, src): new_symbols={%s}", new_symbols);
		conf->len_active_symbols += len;
		conf->active_symbols = new_symbols;
		assert(*(conf->active_symbols + conf->len_active_symbols) == '\0');  // no +1 here!
	}
	else {
		free(conf->active_symbols); // reallocation failed, so active_symbols wasn't freed
		fprintf(stderr, "%s: memory allocation failed\n", PROGRAM_NAME);
		exit(EXIT_FAILURE);
	}
	debug_print(" end of function: len=%zu conf->len=%zu conf->active_symbols={%s}", len, conf->len_active_symbols, conf->active_symbols);
	return len;
}

/* Print instructions on how to use the program.
 *
 * usage_flag argument controls which part of the information is displayed.
 */
void usage(enum usage_flag topic, const struct Configuration *conf)
{
	struct Node *p;

	switch (topic) {
		case help:
			fprintf(stderr, "try `%s -h` for instructions\n", PROGRAM_NAME);
			break;
		case brief:
			printf("usage: %s [option ...] [--] [symbols ...]\n", PROGRAM_NAME);
			break;
		case full:
			usage(brief, conf);
			printf("\ndescription:\n");
			printf("  Generate random strings according to directives.\n\n");

			printf("  All characters from non-option arguments are combined into\n");
			printf("  a pool of symbols from which the random strings are formed.\n");
			printf("  Each symbol has an equal probability of being picked (counting\n");
			printf("  multiplicity). Some predefined symbol sets can be included by\n");
			printf("  using the -S option. If no symbols are specified, the program\n");
			printf("  runs as if `-S %s` option was given.\n", DEFAULT_symbols);

			printf("\noptions:\n");
			printf("  -c <N>, --count=<N>  generate <N> strings (default: %d)\n", DEFAULT_pwcount);
			printf("  -l <N>, --length=<N> each string will have <N> characters (default: %d)\n", DEFAULT_pwlen);
			printf("  -h, --help           print this message and exit\n");
			printf("  -v, --version        print version and license information and exit\n");
			printf("  -S <SET>, --symbols=<SET>\n");
			printf("                       append a predefined set of symbols into the\n");
			printf("                       randomization pool. Can be used multiple times.\n");
			printf("                       If <SET> is `help`, display all predefined symbol\n");
			printf("                       sets and exit.\n");
			printf("  -r <FILE>, --random-seed=<FILE>\n");
			printf("                       read random seed from <FILE> (default: %s)\n", DEFAULT_seed_file);

			printf("\npredefined symbol sets:\n");
			usage(symbol_sets, conf);
			break;
		case symbol_sets:
			p = conf->symbol_sets;
			while (p) {
				printf("  %-10s%s\n", p->name, p->data);
				p = p->next;
			}
			break;
		case version:
			printf("%s version %s\n%s\n%s\n%s\n\nWritten by %s\n", PROGRAM_NAME, VERSION
			      ,"License GPL-3.0-or-later <http://gnu.org/licenses/gpl.html>"
			      ,"This is free software: you are free to change and redistribute it."
			      ,"There is NO WARRANTY, to the extent permitted by law."
			      , AUTHORS
			      );
			break;
	}
}
