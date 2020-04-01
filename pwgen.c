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
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define PROGRAM_NAME "pwgen"
#define VERSION "0.5.3"

#define AUTHORS "Juho Rosqvist"

#ifndef DEBUG_PRINT
#define DEBUG_PRINT 0
#endif
#include <stdarg.h>
void debug_info(const char *file, int line, const char *func, const char *fmt, ...)
{
	va_list argp;

	fprintf(stderr, "%s:%d:%s: ", file, line ,func);
	va_start(argp, fmt);
	vfprintf(stderr, fmt, argp);
	va_end(argp);
	fprintf(stderr, "\n");
}

#define debug_print(fmt, ...) do { if (DEBUG_PRINT) \
	debug_info(__FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__); } while (0)

typedef struct Node Node;
struct Node { // node for accessing symbol sets in a traversable (forward linked) list
	char *name;    // user-facing name of this symbol set
	char *data;    // pointer to the actual symbols string
	size_t size;   // number of characters in the symbols string (excluding '\0')

	struct Node *next;  // link to the next node in the list
};
struct Node *list_append(struct Node *llist, struct Node *new_node);
struct Node *list_seek(struct Node *pos, const char *key);
struct Node *mknode(char const *name, char const *data, size_t size);

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
void init_symbol_sets(struct Node **llist_root);
void free_symbol_sets(struct Node **llist_root);
size_t fill_ascii_range(char *dest, char first, char last);

enum usage_flag { help, brief, full, symbol_sets, version };
void usage(enum usage_flag topic, const struct Configuration *conf);

unsigned int get_RNG_seed(char const *file_name);
int rand_lt(int upper_bound);
char *str_randomize(char *str, size_t len, const char *symbols, size_t len_symbols);


/* This program generates randomized passwords out of a customizable set of
 * characters.
 *
 * For more details, run the program with -h flag or read the source ;-)
 */
int main(int argc, char **argv)
{
	struct Configuration conf = { 0, 0, 0, NULL, NULL, NULL };

	init_symbol_sets(&(conf.symbol_sets));  // generate symbol sets provided by the program
	assert(list_seek(conf.symbol_sets, DEFAULT_symbols));
	configure(&conf, argc, argv);           // configure by command line & apply defaults
	free_symbol_sets(&(conf.symbol_sets));  // symbols to use are now in conf.symbols

	assert(conf.len_active_symbols == strlen(conf.active_symbols));
	assert(0 < conf.len_active_symbols);
	assert(conf.len_active_symbols <= RAND_MAX);

	srand(get_RNG_seed(conf.seed_file));
	free(conf.seed_file); conf.seed_file = NULL;

	char *password = calloc(conf.pwlen + 1, sizeof(*password)); // calloc (+1) ensures zero termination
	for (int i = 0 ; i < conf.pwcount ; ++i) {
		str_randomize(password, conf.pwlen, conf.active_symbols, conf.len_active_symbols);
		printf("%s\n", password);
	}
	free(password);
	free(conf.active_symbols);

	return EXIT_SUCCESS;
}


/* Overwrite the first n characters of str with random characters from
 * the symbols string, and set the n+1st character of str to '\0'.
 *
 * The appearance of every character from symbols is equally likely; if a
 * character is repeated in symbols string, it is twice as likely to appear in
 * str, etc.
 *
 * If you want to guarantee that none of the first n characters will be '\0',
 * just make sure that len_symbols == strlen(symbols), e.g., by using
 * strlen(symbols) as the last argument.
 * We avoid doing this in the function body to make it easier for the compiler
 * to optimize; I'm not sure if this has any practical performance effect.
 * This guarantee should be in effect anyways, unless the user somehow manages
 * to sneak in a mid-string '\0' through command line options, or I screwed
 * up my string handling.
 */
char *str_randomize(char *str, size_t char_count, const char *symbols, size_t len_symbols)
{
	for (size_t i = 0 ; i < char_count ; ++i) {
		str[i] = symbols[rand_lt(len_symbols)];
	}
	str[char_count] = '\0';

	return str;
}

/* Return a (uniformly distributed) random integer from the
 * interval [0, upper_bound - 1].
 * Note: upper_bound may not exceed RAND_MAX.
 *
 * Remember to seed the RNG first!
 */
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

/* Initialize the predefined symbol sets.
 *
 * N.B. the len_XYZ variables in ss contain the number of characters in the
 * corresponding XYZ string, excluding the terminating zero. That is,
 * strlen(XYZ) == len_XYZ, and the memory allocated for string XYZ is
 * equal to (len_XYZ+1)*sizeof(char).
 *
 * The reason for all these dynamic allocations over static strings in the
 * program .text section is the desire to define them by ASCII character
 * ranges. This system also automates the string length calculations, which
 * should eliminate some pesky errors in case one modifies these character
 * sets.
 */
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

/* Free the memory allocated for predefined symbol sets. They are only used
 * during the program setup, after which they may be freed.
 */
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

/* Replace characters from the start of the string dest with the ASCII values
 * between characters first and last (inclusive).
 * Return the number of characters replaced (i.e. #last - #first + 1).
 *
 * Make sure that dest has enough space to hold the characters.
 */
size_t fill_ascii_range(char *dest, char first, char last)
{
	int i;

	for (i = 0; first + i <= last; ++i)
		dest[i] = first + i;
	assert(last - first + 1 == i);

	return i;
}

/* Allocate memory for a new linked list Node for our symbol sets list,
 * initialize it with values from (name, data, size) and return a pointer
 * to this node.
 * Note that mknode will produce a detached node, you have to add it
 * to a list yourself, e.g. using list_append.
 */
struct Node *mknode(char const *name, char const *symbols, size_t size)
{
	struct Node *new_node = malloc(sizeof(*new_node));

	new_node->name = calloc(strlen(name) + 1, sizeof(*(new_node->name)));
	strcpy(new_node->name, name);

	new_node->data = malloc((size + 1) * sizeof(*(new_node->data)));
	strncpy(new_node->data, symbols, size);
	new_node->data[size] = '\0';  // if strlen(src) < len, strncpy will pad with zeroes

	new_node->size = size;
	new_node->next = NULL;  // this node is not yet part of any list

	return new_node;
}

/* Add *new_node to the end of the linked list that contains the *llist Node.
 * Return a pointer to the newly added node.
 *
 * Note that list_append cannot add to an empty list. Use mknode to create
 * a list (of a single node) first and then list_append more to that node.
 */
struct Node *list_append(struct Node *llist, struct Node *new_node)
{
	struct Node *p = llist;

	// seek to the end of the list
	while (p->next) p = p->next;

	p->next = new_node;
	return p->next;
}

/* Find the first occurrence of a node named key in the linked list after
 * (and including) the position pointed to by pos. Return a pointer to that
 * node, or NULL if no match was found until end of list was reached.
 */
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

/* Process the command line and set program configuration accordingly.
 *
 * Command line interface is GNU getopt style.
 */
void configure(struct Configuration *conf, int argc, char **argv)
{
	// apply defaults (symbols default is applied at the end if nothing is selected)
	conf->pwcount = DEFAULT_pwcount;
	conf->pwlen   = DEFAULT_pwlen;
	conf->seed_file = malloc((strlen(DEFAULT_seed_file) + 1) * sizeof(*(conf->seed_file)));
	strcpy(conf->seed_file, DEFAULT_seed_file);

	// activate_symbols calls realloc on conf->active_symbols, so it must be allocated beforehand
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
