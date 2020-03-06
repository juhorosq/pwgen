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
  
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* global constants */
#define PROG_NAME "pwgen"
#define VERSION "0.3.1"

/* exit codes on error */
#define E_BADARGS 1

struct Configuration
{
  int count;       // how many random passwords to generate
  int length;      // the length of each generated password
  int len_symbols; // length of the allowed characters array
  char *symbols;   // array of characters allowed in password generation
};

/* arguments for bail() */
#define BADARGS 1

/* arguments for usage() */
#define HELP  0
#define BRIEF 1
#define LONG  2


void bail ( int reason, char* msg );
void generate ( char password[], int length, char chars[] );
void parseargs ( int argc, char* argv[], struct Configuration *config );
int rand_range ( int upper_bound );
void seed_RNG ();
void usage ( int verbosity );
void version ();


int main (int argc, char **argv)
{
  int i;
  char* password;

  struct Configuration config;
  /* defaults */
  config.count = 1;
  config.length = 8;
  config.len_symbols = 94;
  config.symbols = (char *) calloc(config.len_symbols + 1, sizeof(char)); // +1 to 0-terminate
  if (config.symbols != NULL) {
	  /* by default allow all printable non-space ASCII characters (33-126) */
	  for (i = 0; i < config.len_symbols; i++)
		  config.symbols[i] = '!' + i;
  }
  else {
	  exit(1); // memory allocation failed
  }

  /* parse command-line arguments */
  parseargs ( argc, argv, &config );

  password = (char *) calloc(config.length + 1, sizeof(char *));

  /* generate password(s) */
  seed_RNG();
  for ( i = 0 ; i < config.count ; i++ )
    {
      generate ( password, config.length, config.symbols);
      printf( "%s\n", password );
    }
  free(password);
  free(config.symbols);
}


void bail ( int reason, char* msg )
{

  switch ( reason )
    {
    case BADARGS:
      printf ( "%s: %s\n", PROG_NAME, msg );
      usage ( HELP );
      exit ( E_BADARGS );
      break;
    }
}


void generate (char *password, int length, char *chars)
{
  int i = 0;
  int len_chars = -1;


  /* check how many chars are available */
  while ( chars[++len_chars] != '\0' )
    ;

  /* generate the random password */
  for ( i = 0 ; i < length ; i++ )
    {
      password[i] = chars[rand_range(len_chars)];
    }

  password[length] = '\0';
}


void parseargs ( int argc, char* argv[], struct Configuration *config )
{
  int i,j;
  char* arg;

  i = 1; /* index of the 1st command-line argument */
  while ( i < argc )
    {
      arg = argv[i];

      if ( arg[0] == '-' && arg[1] == '-' )
	{
	  /* long form argument */
	  j = 2;
	  while ( arg[j] != '\0' )
	    {
	      /*
	      printf ( "%c", arg[j] );
	      */
	      j++;
	    }
	}

      else if ( arg[0] == '-' )
	{
	  /* short form argument */
	  j = 1;
	  while ( arg[j] != '\0' )
	    {
	      /*
	      printf ( "%c", arg[j] );
	      */
	      switch ( arg[j] )
		{
		case 'c': /* count */
		  i++;
		  if ( arg[j+1] != '\0' ) /* c wasn't last in the bundle */
		    {
		      bail ( BADARGS, strcat ( arg, ": invalid option arrangement" ) );
		    }
		  else if ( i >= argc ) /* c was the last argument */
		    {
		      bail ( BADARGS, "option requires an argument -- c" );
		    }
		  else
		    {
		      arg = argv[i];
		      j = 0;
		      while ( arg[j] != '\0' )
			{
			  if ( ! isdigit ( arg[j] ) )
			    {
			      bail( BADARGS, strcat ( arg, ": invalid argument to option -- c" ) );
			    }
			  j++;
			}

		      config->count = atoi ( argv[i] );
		      continue;
		    }
		  break;
		case 'h': /* help */
		  usage ( LONG );
		  exit ( 0 );
		  break;
		case 'l': /* length */
		  i++;
		  if ( arg[j+1] != '\0' ) /* l wasn't last in the bundle */
		    {
		      bail ( BADARGS, strcat ( arg, ": invalid option arrangement" ) );
		    }
		  else if ( i >= argc ) /* l was the last argument */
		    {
		      bail ( BADARGS, "option requires an argument -- l" );
		    }
		  else
		    {
		      arg = argv[i];
		      j = 0;
		      while ( arg[j] != '\0' )
			{
			  if ( ! isdigit ( arg[j] ) )
			    {
			      bail( BADARGS, strcat ( arg, ": invalid argument to option -- l" ) );
			    }
			  j++;
			}

		      config->length = atoi ( argv[i] );
		      continue;
		    }
		  break;
		case 'v': /* version */
		  printf( "%s version %s\n%s\n%s\n%s\n", PROG_NAME, VERSION,
				  "License GPL-3.0-or-later <http://gnu.org/licenses/gpl.html>",
				  "This is free software: you are free to change and redistribute it.",
				  "There is NO WARRANTY, to the extent permitted by law."
				);
		  exit ( 0 );
		  break;
		default:
		  bail( BADARGS, arg );
		  break;
		}
	      j++;
	    }
	}

      else
	{
	  bail ( BADARGS, arg );
	}

      i++;
    }
}

/**
 * Return a random integer from the range [0, upper_bound)
 *
 * Remember to call seed_RNG once first!
 */
int rand_range ( int upper_bound )
{
	int reject_bound;
	int r;

	/* Since stdlib's rand() is uniformly distributed, we use rejection
	 * sampling to ensure that the result is also uniformly distributed
	 * on our range [0, upper_bound)
	 */
	reject_bound = RAND_MAX - (RAND_MAX % upper_bound);
	while ( (r = rand()) >= reject_bound );

	return r % upper_bound;
}

/**
 * Seed the pseudo-random number generator
 */
void seed_RNG ()
{
  FILE *fp;
  unsigned int rseed;

  // This would be weak! Also a problem when executing program in a loop.
  //srand( time ( NULL ) );

  /* Read from /dev/urandom since it's guaranteed to not block on read,
   * unlike /dev/random
   */
  fp = fopen ( "/dev/urandom", "rb");
  fread (&rseed, sizeof(rseed), 1, fp);
  fclose (fp);
  srand (rseed);
  //XXX Error handling
}


void usage ( int verbosity )
{
  switch ( verbosity )
    {
    case HELP:
      printf ( "try `%s -h` for instructions\n", PROG_NAME );
      break;
    case BRIEF:
      printf ( "usage: %s <option> [option ...]\n", PROG_NAME );
      break;
    case LONG:
      usage ( BRIEF );
      printf ( "description:\n" );
      printf ( "  Generates random strings according to directives\n" );
      printf ( "  given as command-line options.\n" );
      printf ( "options:\n" );
      printf ( "  -c NUM, --count=NUM    generate NUM strings\n" );
      printf ( "  -h, --help             print this message and exit\n" );
      printf ( "  -l NUM, --length=NUM   generate strings of NUM characters\n" );
      printf ( "  -v, --version          print version number and exit\n" );
      break;
    }
}
