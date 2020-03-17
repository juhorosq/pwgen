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
#include <unistd.h>

#define PROGRAM_NAME "pwgen"
#define VERSION "0.5.1"

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

struct Configuration {
	int pwcount;         // how many random passwords to generate
	size_t pwlen;        // the length of each generated password
	size_t len_symbols;  // length of the string containing allowed characters
	char *symbols;       // string of characters allowed in password generation
};
size_t append_symbols(struct Configuration *conf, const char *src);

typedef struct Node Node;
struct Node { // node for accessing symbol sets in a traversable (forward linked) list
	char *name;    // user-facing name of this symbol set
	char *data;    // pointer to the actual symbols string
	size_t *size;  // pointer to the number of characters in the symbols string (excluding '\0')

	struct Node *next;  // link to the next node in the list
};
struct Node *list_append(struct Node *llist, struct Node *new_node);
struct Node *list_seek(struct Node *pos, const char *key);
struct Node *mknode(char *name, char *data, size_t *size);

struct SymbolSets {  // structure to hold predefined symbol sets
	struct Node *iter;   // The first node in a linked list of the contents of this struct

	char *asciip;        // printable ASCII characters, including space (32-126)
	size_t len_asciip;
	char *asciipns;      // printable ASCII characters, without space (33-126)
	size_t len_asciipns;
	char *numeric;       // numbers 0-9 (ASCII 48-57)
	size_t len_numeric;
	char *ALPHABETIC;    // uppercase letters (65-90)
	size_t len_ALPHABETIC;
	char *alphabetic;    // lowercase letters (97-122)
	size_t len_alphabetic;
	char *Alphabetic;    // all letters (*ALPHABETIC + *alphabetic)
	size_t len_Alphabetic;
	char *ALNUM;         // uppercase alphanumeric characters (*ALPHABETIC + *numeric)
	size_t len_ALNUM;
	char *alnum;         // lowercase alphanumeric characters (*alphabetic + *numeric)
	size_t len_alnum;
	char *Alnum;         // alphanumeric characters (*Alphabetic + *numeric)
	size_t len_Alnum;
	char *punct;         // punctuation characters (33-47, 58-64, 91-96, 123-126)
	size_t len_punct;
};
void init_symbolsets(struct SymbolSets *ss);
void free_symbolsets(struct SymbolSets *ss);
size_t fill_ascii_range(char *dest, char first, char last);
char *symbolset_alloc(const char *src, size_t len);

void configure(int argc, char **argv, struct Configuration *conf, const struct SymbolSets *ss);
enum usage_flag { help, brief, full, symbolsets, version };
void usage(enum usage_flag topic, const struct SymbolSets *ss);

unsigned int get_RNG_seed();
int rand_lt(int upper_bound);
char *str_randomize(char *str, size_t len, const char *symbols, size_t len_symbols);


/* This program generates randomized passwords out of a customizable set of
 * characters.
 *
 * For more details, run the program with -h flag or read the source ;-)
 */
int main(int argc, char **argv)
{
	struct Configuration conf;
	struct SymbolSets ss;

	init_symbolsets(&ss);               // generate symbol sets provided by the program
	configure(argc, argv, &conf, &ss);  // configure by command line & apply defaults
	free_symbolsets(&ss);               // symbols to use are now in conf.symbols

	srand(get_RNG_seed());

	char *password = calloc(conf.pwlen + 1, sizeof(*password)); // calloc (+1) ensures zero termination
	for (int i = 0 ; i < conf.pwcount ; ++i) {
		str_randomize(password, conf.pwlen, conf.symbols, conf.len_symbols);
		printf("%s\n", password);
	}
	free(password);
	free(conf.symbols);

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
	assert(upper_bound <= RAND_MAX);  //XXX Bad form: input-dependent
	int reject_bound = RAND_MAX - (RAND_MAX % upper_bound);
	assert(reject_bound % upper_bound == 0);
	while ((r = rand()) >= reject_bound);

	return r % upper_bound;
}

/* Get a seed for the pseudo-random number generator from a system source.
 *
 * Use this instead of e.g. time (which is predictable) to seed the PRNG.
 * We get the seed from /dev/urandom since it's guaranteed to not block on read,
 * unlike /dev/random. Obviously, this only works on (most) *nix systems.
 */
unsigned int get_RNG_seed()
{
	unsigned int rseed;

	FILE *fp = fopen("/dev/urandom", "rb");
	fread(&rseed, sizeof(rseed), 1, fp);
	fclose(fp);
	//XXX Error handling

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
 * program .text section is my desire to define them by ASCII character
 * ranges. This system also automates the string length calculations, which
 * should eliminate some pesky errors in case one modifies these character
 * sets.
 */
void init_symbolsets(struct SymbolSets *ss)
{
	struct Node *p;  // list iterator
	char syms[128];  // buffer for building strings
	size_t len;      // length of string in the buffer

	// printable ASCII characters, including space (32--126)
	len = fill_ascii_range(syms, ' ', '~');
	ss->asciip = symbolset_alloc(syms, len);
	ss->len_asciip = len;
	// the first node in our linked list must be manually connected
	p = ss->iter = mknode("asciip", ss->asciip, &(ss->len_asciip));

	// printable ASCII characters, without space (33--126)
	len = fill_ascii_range(syms, '!', '~');
	ss->asciipns = symbolset_alloc(syms, len);
	ss->len_asciipns = len;
	p = list_append(p, mknode("asciipns", ss->asciipns, &(ss->len_asciipns)));

	// numbers 0-9 (ASCII 48--57)
	len = fill_ascii_range(syms, '0', '9');
	ss->numeric = symbolset_alloc(syms, len);
	ss->len_numeric = len;
	p = list_append(p, mknode("num", ss->numeric, &(ss->len_numeric)));

	// uppercase letters (65--90)
	len = fill_ascii_range(syms, 'A', 'Z');
	ss->ALPHABETIC = symbolset_alloc(syms, len);
	ss->len_ALPHABETIC = len;
	p = list_append(p, mknode("ALPHA", ss->ALPHABETIC, &(ss->len_ALPHABETIC)));

	// lowercase letters (97--122)
	len = fill_ascii_range(syms, 'a', 'z');
	ss->alphabetic = symbolset_alloc(syms, len);
	ss->len_alphabetic = len;
	p = list_append(p, mknode("alpha", ss->alphabetic, &(ss->len_alphabetic)));

	// all letters (*ALPHABETIC + *alphabetic)
	len = ss->len_ALPHABETIC + ss->len_alphabetic;
	strcpy(syms, ss->ALPHABETIC);
	strcpy(syms + ss->len_ALPHABETIC, ss->alphabetic);
	ss->Alphabetic = symbolset_alloc(syms, len);
	ss->len_Alphabetic = len;
	p = list_append(p, mknode("Alpha", ss->Alphabetic, &(ss->len_Alphabetic)));

	// uppercase alphanumeric characters (*ALPHABETIC + *numeric)
	len = ss->len_ALPHABETIC + ss->len_numeric;
	strcpy(syms, ss->ALPHABETIC);
	strcpy(syms + ss->len_ALPHABETIC, ss->numeric);
	ss->ALNUM = symbolset_alloc(syms, len);
	ss->len_ALNUM = len;
	p = list_append(p, mknode("ALNUM", ss->ALNUM, &(ss->len_ALNUM)));

	// lowercase alphanumeric characters (*alphabetic + *numeric)
	len = ss->len_alphabetic + ss->len_numeric;
	strcpy(syms, ss->alphabetic);
	strcpy(syms + ss->len_alphabetic, ss->numeric);
	ss->alnum = symbolset_alloc(syms, len);
	ss->len_alnum = len;
	p = list_append(p, mknode("alnum", ss->alnum, &(ss->len_alnum)));

	// uppercase & lowercase alphanumeric characters (*Alphabetic + *numeric)
	len = ss->len_Alphabetic + ss->len_numeric;
	strcpy(syms, ss->Alphabetic);
	strcpy(syms + ss->len_Alphabetic, ss->numeric);
	ss->Alnum = symbolset_alloc(syms, len);
	ss->len_Alnum = len;
	p = list_append(p, mknode("Alnum", ss->Alnum, &(ss->len_Alnum)));

	// punctuation characters (33--47, 58--64, 91--96, 123--126)
	len = fill_ascii_range(syms, '!', '/');
	len += fill_ascii_range(syms + len, ':', '@');
	len += fill_ascii_range(syms + len, '[', '`');
	len += fill_ascii_range(syms + len, '{', '~');
	ss->punct = symbolset_alloc(syms, len);
	ss->len_punct = len;
	p = list_append(p, mknode("punct", ss->punct, &(ss->len_punct)));
}

/* Free the memory allocated for predefined symbol sets. They are only used
 * during the program setup, after which they may be freed.
 */
void free_symbolsets(struct SymbolSets *ss)
{
	struct Node *p = ss->iter;

	while (p) {
		struct Node *q = p->next;
		debug_print("freeing: 0x%016x: %s = {%s}", p, p->name, p->data);
		free(p->name); free(p->data); free(p);
		p = q;
	}
	ss->iter = NULL;
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

/* Allocate memory for string of length len, copy the first len characters
 * from src array to it and return a pointer to the newly created string.
 *
 * New string will have a terminating '\0' after the last copied character.
 */
char *symbolset_alloc(const char *src, size_t len)
{
	char *dest = malloc((len + 1) * sizeof(*dest)); // +1 for '\0'
	strncpy(dest, src, len);
	dest[len] = '\0';  // if strlen(src) < len, strncpy will pad with zeroes

	return dest;
}

/* Allocate memory for a new linked list Node for our SymbolSets list,
 * initialize it with values from (name, data, size) and return a pointer
 * to this node.
 * Note that mknode will produce a detached node, you have to add it
 * to a list yourself, e.g. using list_append.
 * Note that the resulting node does not own *data and *size, but merely
 * pointers to them; in contrast *name is copied onto memory owned by Node.
 */
struct Node *mknode(char *name, char *data, size_t *size)
{
	struct Node *new_node = malloc(sizeof(*new_node));

	new_node->name = calloc(strlen(name) + 1, sizeof(*(new_node->name)));
	strcpy(new_node->name, name);
	new_node->data = data;
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
 * Command line interface is GNU getopt style (defined in unistd.h),
 * but short options only (XXX contrary to help text) at the moment.
 */
void configure(int argc, char **argv, struct Configuration *conf, const struct SymbolSets *ss)
{
	// apply defaults (symbols default is applied at the end if nothing is selected)
	conf->pwcount = 1;
	conf->pwlen = 8;
	conf->len_symbols = 0;
	// append_symbols calls realloc on conf->symbols, so it must be allocated already
	conf->symbols = calloc(conf->len_symbols + 1, sizeof(char)); // +1 to 0-terminate

	int c;
	struct Node *p;

	// process command line options
	while ((c = getopt(argc, argv, "S:c:l:hv")) != -1) {
		switch (c) {
			case 'S':
				p = list_seek(ss->iter, optarg);
				if (p) {
					append_symbols(conf, p->data);
				} else {
					printf("No such symbol set: %s\n", optarg);
					exit(EXIT_FAILURE);
				}
				break;
			case 'c':
				conf->pwcount = atoi(optarg);
				break;
			case 'h':
				usage(full, ss);
				exit(EXIT_SUCCESS);
			case 'l':
				conf->pwlen = atoi(optarg);
				break;
			case 'v':
				usage(version, ss);
				exit(EXIT_SUCCESS);
				break;
			case '?': // invalid option
				exit(EXIT_FAILURE);
			case ':': // option argument missing // will not happen unless option string starts with ':'?
				exit(EXIT_FAILURE);
			default:
				exit(EXIT_FAILURE);
		}
	}

	// process non-option arguments as (partial) character pool definitions
	for (int i = optind; i < argc; ++i) {
		append_symbols(conf, argv[i]);
	}
	if (conf->len_symbols == 0) // use default symbols if none were selected
		append_symbols(conf, ss->asciipns);
}

/* Add characters from a zero-terminated string src to the allowed symbols pool
 * conf->symbols (reallocating memory as needed) and update symbols count
 * conf->len_symbols.
 * Failing to reallocate memory is fatal, and terminates the program.
 *
 * Return the number of characters added.
 */
size_t append_symbols(struct Configuration *conf, const char *src)
{
	size_t len = strlen(src);
	debug_print("%s(%s) len=%zu conf->len=%zu\n", __func__, src, len, conf->len_symbols);
	debug_print(" before realloc:     &symbols=0x%016x      symbols={%s}", conf->symbols, conf->symbols);

	char *new_symbols = realloc(conf->symbols, (conf->len_symbols + len + 1) * sizeof(*new_symbols));
	//debug_print(" after realloc (UB): &symbols=0x%016x      symbols={%s}", conf->symbols, conf->symbols); // undefined
	debug_print(" after realloc:  &new_symbols=0x%016x  new_symbols={%s}", new_symbols, new_symbols);
	if (new_symbols) {
		strcat(new_symbols, src);
		debug_print(" after strcat(new_symbols, src): new_symbols={%s}", new_symbols);
		conf->len_symbols += len;
		conf->symbols = new_symbols;
		assert(*(conf->symbols + conf->len_symbols) == '\0');  // no +1 here!
	}
	else {
		free(conf->symbols); // reallocation failed, so symbols wasn't freed
		fprintf(stderr, "%s: memory allocation failed\n", PROGRAM_NAME);
		exit(EXIT_FAILURE);
	}
	debug_print(" end of function: len=%zu conf->len=%zu conf->symbols={%s}", len, conf->len_symbols, conf->symbols);
	return len;
}

/* Print instructions on how to use the program.
 *
 * usage_flag argument controls which part of the information is displayed.
 */
void usage(enum usage_flag topic, const struct SymbolSets *ss)
{
	struct Node *p;

	switch (topic) {
		case help:
			printf("try `%s -h` for instructions\n", PROGRAM_NAME);
			break;
		case brief:
			printf("usage: %s [option ...] [symbols ...]\n", PROGRAM_NAME);
			break;
		case full:
			usage(brief, ss);
			printf("\ndescription:\n");
			printf("  Generate random strings according to directives.\n\n");

			printf("  All characters from non-option arguments are combined into\n");
			printf("  a pool of symbols from which the random strings are formed.\n");
			printf("  Each symbol has an equal probability of being picked (counting\n");
			printf("  multiplicity). Some predefined symbol sets can be included by\n");
			printf("  using the -S option. If no symbols are specified, the program\n");
			printf("  assumes -S asciipns (all printable non-space ASCII characters)\n\n");

			printf("options:\n");
			printf("  -c NUM, --count=N    the number of strings to generate (default %d)\n", 1);
			printf("  -l NUM, --length=N   the length of each generated string (default %d)\n", 8);
			printf("  -h, --help           print this message and exit\n");
			printf("  -v, --version        print version and license information and exit\n");
			printf("  -S <SET>, --symbols=<SET>\n");
			printf("                       append a predefined set of characters into the\n");
			printf("                       randomization pool. See below for values of <SET>.\n\n");
			usage(symbolsets, ss);
			break;
		case symbolsets:
			printf("predefined character sets:\n");
			p = ss->iter;
			while (p) {
				printf("  %-15s%s\n", p->name, p->data);
				p = p->next;
			}
			break;
		case version:
			printf("%s version %s\n%s\n%s\n%s\n", PROGRAM_NAME, VERSION
			      ,"License GPL-3.0-or-later <http://gnu.org/licenses/gpl.html>"
			      ,"This is free software: you are free to change and redistribute it."
			      ,"There is NO WARRANTY, to the extent permitted by law."
			      );
			break;
	}
}
