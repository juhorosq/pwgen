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

// global constants
#define PROG_NAME "pwgen"
#define VERSION "0.4.1"

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
  int count;           // how many random passwords to generate
  size_t pwlen;        // the length of each generated password
  size_t len_symbols;  // length of the string containing allowed characters
  char *symbols;       // string of characters allowed in password generation
};
size_t append_symbols(struct Configuration *conf, const char *src);

struct Node { // node for accessing symbol sets in a traversable (linked) list
	char *name;    // user-facing name of this symbol set
	char *data;    // pointer to the actual symbols string
	size_t *size;  // pointer to the number of characters in the symbols string (excluding '\0')

	struct Node *next;  // link to the next node in the list
};
struct Node *list_append(struct Node *llist, struct Node *newnode);
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
size_t fill_ascii_range(char *dest, char first, char last);
char *symset_alloc(const char *src, size_t len);

void parse_args(int argc, char **argv, struct Configuration *conf, const struct SymbolSets *ss);
enum usage_flag { help, brief, full, symsets, version };
void usage(enum usage_flag topic, const struct SymbolSets *ss);


void seed_RNG();
int rand_range(int upper_bound);
char *str_randomize(char *str, size_t len, const char *symbols, size_t len_symbols);


int main(int argc, char **argv)
{
  struct Configuration conf;
  struct SymbolSets ss;

  int i;
  char* password;

  init_symbolsets(&ss);               // generate symbol sets provided by the program
  parse_args(argc, argv, &conf, &ss); // configure by command line & apply defaults

  // generate password(s)
  seed_RNG();
  password = calloc(conf.pwlen + 1, sizeof(char));
  for (i = 0 ; i < conf.count ; i++) {
      str_randomize(password, conf.pwlen, conf.symbols, conf.len_symbols);
      printf("%s\n", password);
  }

  free(password);
  free(conf.symbols);
  return EXIT_SUCCESS;
}


/* Overwrite the first n characters of str with random characters from
 * the string symsbols, and set the n+1st character of str to '\0'.
 * XXX and all preceding characters are guaranteed to not be '\0'.
 *
 * The appearance of every character from symbols is equally likely; if a
 * character is repeated in symbols string, it is twice as likely to appear in
 * str, etc.
 */
char *str_randomize(char *str, size_t n, const char *symbols, size_t len_symbols)
{
	int i;

	for (i = 0 ; i < n ; i++) {
		str[i] = symbols[rand_range(len_symbols)];
	}
	str[n] = '\0';

	return str;
}

/* Return a random integer from the range [0, upper_bound)
 *
 * Remember to call seed_RNG once (and only once) first!
 */
int rand_range(int upper_bound)
{
	int reject_bound;
	int r;

	/* Since stdlib's rand() is uniformly distributed, we use rejection
	 * sampling to ensure that the result is also uniformly distributed
	 * on our range [0, upper_bound)
	 */
	reject_bound = RAND_MAX - (RAND_MAX % upper_bound);
	while ((r = rand()) >= reject_bound);

	return r % upper_bound;
}

/* Seed the pseudo-random number generator
 *
 * We get the seed from /dev/urandom since it's guaranteed to not block on read,
 * unlike /dev/random. Obviously, this only works on (most) *nix systems.
 */
void seed_RNG ()
{
  FILE *fp;
  unsigned int rseed;

  // This would be weak! Also a problem when executing program in a loop.
  //srand( time ( NULL ) );

  fp = fopen ( "/dev/urandom", "rb");
  fread (&rseed, sizeof(rseed), 1, fp);
  fclose (fp);
  srand (rseed);
  //XXX Error handling
}

/* Initialize the predefined symbol sets
 *
 * N.B. the len_XYZ variables in ss contain the number of characters in the
 * corresponding XYZ string, excluding the terminating zero! That is,
 * strlen(XYZ) == len_XYZ, and the memory allocated for string XYZ is
 * equal to (len_XYZ+1)*sizeof(char).
 */
void init_symbolsets(struct SymbolSets *ss)
{
	struct Node *p;  // list iterator
	char syms[128];  // buffer for building strings
	int len;         // length of string in the buffer

	// printable ASCII characters, including space (32-126)
	len = fill_ascii_range(syms, ' ', '~');
	ss->asciip = symset_alloc(syms, len);
	ss->len_asciip = len;
	// create the first node in our linked list of symbol sets
	p = ss->iter = mknode("asciip", ss->asciip, &(ss->len_asciip));

	// printable ASCII characters, without space (33-126)
	ss->asciipns = symset_alloc(syms + 1, len - 1);
	ss->len_asciipns = len - 1;
	p = list_append(p, mknode("asciipns", ss->asciipns, &(ss->len_asciipns)));

	// numbers 0-9 (ASCII 48-57)
	len = fill_ascii_range(syms, '0', '9');
	ss->numeric = symset_alloc(syms, len);
	ss->len_numeric = len;
	p = list_append(p, mknode("num", ss->numeric, &(ss->len_numeric)));

	// uppercase letters (65-90)
	len = fill_ascii_range(syms, 'A', 'Z');
	ss->ALPHABETIC = symset_alloc(syms, len);
	ss->len_ALPHABETIC = len;
	p = list_append(p, mknode("ALPHA", ss->ALPHABETIC, &(ss->len_ALPHABETIC)));

	// lowercase letters (97-122)
	len = fill_ascii_range(syms, 'a', 'z');
	ss->alphabetic = symset_alloc(syms, len);
	ss->len_alphabetic = len;
	p = list_append(p, mknode("alpha", ss->alphabetic, &(ss->len_alphabetic)));

	// all letters (*ALPHABETIC + *alphabetic)
	len = ss->len_ALPHABETIC + ss->len_alphabetic;
	strcpy(syms, ss->ALPHABETIC);
	strcpy(syms + ss->len_ALPHABETIC, ss->alphabetic);
	ss->Alphabetic = symset_alloc(syms, len);
	ss->len_Alphabetic = len;
	p = list_append(p, mknode("Alpha", ss->Alphabetic, &(ss->len_Alphabetic)));

	// uppercase alphanumeric characters (*ALPHABETIC + *numeric)
	len = ss->len_ALPHABETIC + ss->len_numeric;
	strcpy(syms, ss->ALPHABETIC);
	strcpy(syms + ss->len_ALPHABETIC, ss->numeric);
	ss->ALNUM = symset_alloc(syms, len);
	ss->len_ALNUM = len;
	p = list_append(p, mknode("ALNUM", ss->ALNUM, &(ss->len_ALNUM)));

	// lowercase alphanumeric characters (*alphabetic + *numeric)
	len = ss->len_alphabetic + ss->len_numeric;
	strcpy(syms, ss->alphabetic);
	strcpy(syms + ss->len_alphabetic, ss->numeric);
	ss->alnum = symset_alloc(syms, len);
	ss->len_alnum = len;
	p = list_append(p, mknode("alnum", ss->alnum, &(ss->len_alnum)));

	// uppercase & lowercase alphanumeric characters (*Alphabetic + *numeric)
	len = ss->len_Alphabetic + ss->len_numeric;
	strcpy(syms, ss->Alphabetic);
	strcpy(syms + ss->len_Alphabetic, ss->numeric);
	ss->Alnum = symset_alloc(syms, len);
	ss->len_Alnum = len;
	p = list_append(p, mknode("Alnum", ss->Alnum, &(ss->len_Alnum)));

	// punctuation characters (33-47, 58-64, 91-96, 123-126)
	len = fill_ascii_range(syms, '!', '/');
	len += fill_ascii_range(syms + len, ':', '@');
	len += fill_ascii_range(syms + len, '[', '`');
	len += fill_ascii_range(syms + len, '{', '~');
	ss->punct = symset_alloc(syms, len);
	ss->len_punct = len;
	p = list_append(p, mknode("punct", ss->punct, &(ss->len_punct)));
}

/* Replace characters from the start of the string dest with the ASCII values
 * between characters first and last (inclusive).
 * Return the number of characters replaced (i.e. last - first + 1).
 */
size_t fill_ascii_range(char *dest, char first, char last)
{
	int i;
	for (i = 0; first + i <= last; i++)
		dest[i] = first + i;
	assert(last - first + 1 == i);
	return i;
}

/* Allocate memory for string of length len, copy the first len characters
 * from src array to it and return a pointer to the newly created string.
 *
 * New string will have a terminating '\0' after the last copied character.
 */
char *symset_alloc(const char *src, size_t len)
{
	char *dest = malloc((len + 1) * sizeof(char));
	strncpy(dest, src, len);
	dest[len] = '\0';

	return dest;
}

/* Add *newnode to the end of the linked list to which the *llist belongs.
 * Return a pointer to the newly added node.
 */
struct Node *list_append(struct Node *llist, struct Node *newnode)
{
	struct Node *p = llist;

	while (p->next != NULL) // seek to the end of the list
		p = p->next;
	p->next = newnode;
	return p->next;
}

/* Allocate memory for a new linked list node for our SymbolSets list,
 * initialize it with values from (name, data, size) and return a pointer
 * to this node.
 * Note that mknode will produce a detached node, you have to add it
 * to a list yourself, e.g. using list_append.
 * Note that the resulting node does not own *data and *size, but merely
 * pointers to them; in contrast *name is copied onto memory owned by Node.
 */
struct Node *mknode(char *name, char *data, size_t *size)
{
	struct Node *newnode = malloc(sizeof(struct Node));

	newnode->name = calloc(strlen(name) + 1, sizeof(char)); // last char (+1) initialized to 0
	strcpy(newnode->name, name);
	newnode->data = data;
	newnode->size = size;
	newnode->next = NULL;  // this node is not yet part of any list

	return newnode;
}

/* Find the first occurrence of a node named key in the linked list after
 * (and including) the position pointed to by *pos. Return a pointer to that
 * node, or NULL if no match was found until end of list was reached.
 */
struct Node *list_seek(struct Node *pos, const char *key)
{
	struct Node *p;

	p = pos;
	while (p != NULL) {
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
void parse_args(int argc, char **argv, struct Configuration *conf, const struct SymbolSets *ss)
{
  int c;
  //char *cvalue = NULL;
  struct Node *p;

  // apply defaults
  conf->count = 1;
  conf->pwlen = 8;
  conf->len_symbols = 0;
  //conf->symbols = NULL;
  //conf.len_symbols = 94;
  conf->symbols = calloc(conf->len_symbols + 1, sizeof(char)); // +1 to 0-terminate
  // by default allow all printable non-space ASCII characters (33-126)

  // process command line options
  while ((c = getopt(argc, argv, "S:c:l:hv")) != -1)
	switch (c) {
	case 'S':
		if ((p = list_seek(ss->iter, optarg)) != NULL) {
			append_symbols(conf, p->data);
		} else {
			printf("No such symbol set: %s\n", optarg);
			exit(EXIT_FAILURE);
		}
	    break;
	case 'c':
		conf->count = atoi(optarg);
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
	case ':': // option argument missing // will not happen unless option string starts with ':'
	    exit(EXIT_FAILURE);
	default:
	    exit(EXIT_FAILURE);
	}

  // process non-option arguments as (partial) character pool definitions
  for (c = optind; c < argc; c++) {
	  append_symbols(conf, argv[c]);
  }
  if (conf->len_symbols == 0) // use default symbols if none were selected
	  append_symbols(conf, ss->asciipns);
}

/* Add characters from a zero-terminated string src to the allowed symbols pool
 * conf->symbols (reallocating memory as needed) and update symbols count
 * conf->len_symbols.
 *
 * Return the number of characters added.
 */
size_t append_symbols(struct Configuration *conf, const char *src)
{
	int len = strlen(src);
    debug_print("%s(%s) len=%d conf->len=%zu\n", __func__, src, len, conf->len_symbols);
	conf->symbols = realloc(conf->symbols, (conf->len_symbols + len + 1) * sizeof(char));
	debug_print(" after realloc, conf->symbols={%s}\n", conf->symbols);
	strcat(conf->symbols, src);
	debug_print(" after strcat(conf->symbols, src), conf->symbols={%s}\n", conf->symbols);
	// *(conf->symbols + (conf->len_symbols + len + 1)) = '\0'; // off-by-1 (past the end)!
	//debug_print(" *(conf->symbols + (conf->len_symbols + len))=%d\n", *(conf->symbols + (conf->len_symbols + len + 1)));
	assert(*(conf->symbols + (conf->len_symbols + len)) == '\0');  // no +1 here!
	conf->len_symbols += len;
	return len;
}


void usage (enum usage_flag topic, const struct SymbolSets *ss)
{
  struct Node *p;

  switch (topic) {
  case help:
      printf("try `%s -h` for instructions\n", PROG_NAME );
      break;
  case brief:
      printf("usage: %s [option ...] [symbols ...]\n", PROG_NAME );
      break;
  case full:
      usage(brief, ss);
      printf("\ndescription:\n" );
      printf("  Generate random strings according to directives.\n\n");

      printf("  All characters from non-option arguments are combined into\n");
      printf("  a pool of symbols from which the random strings are formed.\n");
      printf("  Each symbol has an equal probability of being picked (counting\n");
      printf("  multiplicity). Some predefined symbol sets can be included by\n");
      printf("  using the -S option. If no symbols are specified, the program\n");
      printf("  assumes -S asciipns (all printable non-space ASCII characters)\n\n");

      printf("options:\n" );
      printf("  -c NUM, --count=N    the number of strings to generate (default %d)\n", 1);
      printf("  -l NUM, --length=N   the length of each generated string (default %d)\n", 8);
      printf("  -h, --help           print this message and exit\n");
      printf("  -v, --version        print version and license information and exit\n");
      printf("  -S <SET>, --symbols=<SET>\n");
      printf("                       append a predefined set of characters into the\n");
      printf("                       randomization pool. See below for values of <SET>.\n\n");
	  usage(symsets, ss);
	  break;
  case symsets:
      printf("predefined character sets:\n");
	  p = ss->iter;
	  while (p != NULL) {
		  printf("  %-15s%s\n", p->name, p->data);
		  p = p->next;
	  }
/*
      printf("  %s\t%s\n", "asciip", "printable ASCII characters including space");
      printf("  %s\t%s\n", "asciipns", "printable ASCII characters without space");
      printf("  %s\t%s\n", "numeric", "numbers 0-9");
      printf("  %s\t%s\n", "ALPHA", "uppercase letters");
      printf("  %s\t%s\n", "alpha", "lowercase letters");
      printf("  %s\t%s\n", "Alpha", "all letters");
      printf("  %s\t%s\n", "ALNUM", "uppercase alphanumeric");
      printf("  %s\t%s\n", "alnum", "lowercase alphanumeric");
      printf("  %s\t%s\n", "Alnum", "all alphanumeric");
      printf("  %s\t%s\n", "punct", "punctuation");
      printf("  \n");
*/
      break;
  case version:
      printf("%s version %s\n%s\n%s\n%s\n", PROG_NAME, VERSION,
          "License GPL-3.0-or-later <http://gnu.org/licenses/gpl.html>",
	      "This is free software: you are free to change and redistribute it.",
	      "There is NO WARRANTY, to the extent permitted by law."
	  );
	  break;
  }
}
