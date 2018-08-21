/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 
*/

#include  <stdio.h>
#include  <stdlib.h>
#include  <unistd.h>
#include  <string.h>
#include  <ctype.h>

#ifdef UNIX
#include  <sys/file.h>
#else
#define  R_OK		4
#define  W_OK		2
#endif

#include "vnmrsys.h"
#include "group.h"
#include "shims.h"
#include "pvars.h"

static void print_line(const char *shim_label, const char *sv1, const char *sv2 );
static int get_system_globals(char *cname );
static int get_params(char *cname, int  argc, char *argv[] );
static void compare_shims();


int main(int argc, char *argv[] )
{
	int	ival;

	ival = get_system_globals( argv[ 0 ] );
	if (ival < 0)
	  exit(0);

	init_shimnames( NOTREE );		/* Use NOTREE to get  */
						/* all possible shims */
	ival = get_params( argv[ 0 ], argc-1, &argv[ 1 ] );
	if (ival < 0)
	  exit(0);

	compare_shims();
        exit(0);
}

static void compare_shims()
{
	const char	*cur_shim;
	char	 tbuf1[ 12 ], tbuf2[ 12 ];
	int	 iter, one_undef, two_undef, sh_val1, sh_val2;
	double	 rval1, rval2;

	for (iter = 0; iter < MAX_SHIMS; iter++) {
		cur_shim = get_shimname( iter );

                sh_val1 = sh_val2 = 0;
		one_undef = P_getreal( CURRENT, cur_shim, &rval1, 1 );
		two_undef = P_getreal( PROCESSED, cur_shim, &rval2, 1 );

	/*  If shim not defined in either set, move to the next.  */

		if (one_undef && two_undef)
		  continue;

		if ( !one_undef ) {
			sh_val1 = (int)(rval1 >= 0 ? rval1+0.1 : rval1-0.1);
			sprintf( &tbuf1[ 0 ], "%d", sh_val1 );
		}
		else
		  strcpy( &tbuf1[ 0 ], "undefined" );
		if ( !two_undef ) {
			sh_val2 = (int)(rval2 >= 0 ? rval2+0.1 : rval2-0.1);
			sprintf( &tbuf2[ 0 ], "%d", sh_val2 );
		}
		else
		  strcpy( &tbuf2[ 0 ], "undefined" );

		if (one_undef || two_undef || sh_val1 != sh_val2) {
			print_line( cur_shim, &tbuf1[ 0 ], &tbuf2[ 0 ] );
		}
	}
}

static void print_line(const char *shim_label, const char *sv1, const char *sv2 )
{
	printf( "  %-7s", shim_label );
	if (isdigit( *sv1 ))
	  printf( "  %-9s", sv1 );
	else
	  printf( " %-10s", sv1 );
	if (isdigit( *sv2 ))
	  printf( "  %-9s", sv2 );
	else
	  printf( " %-10s", sv2 );
	printf( "\n" );
}

static int get_system_globals(char *cname )
{
	char		*systemdir;
	char		 conpar_path[ MAXPATHL ];
	int		 ival;
	extern char	*getenv();

	systemdir = getenv( "vnmrsystem" );
	if (systemdir == NULL) {
		fprintf( stderr, "error in %s: 'vnmrsystem' not defined\n", cname );
		return( -1 );
	}

	if (strlen( systemdir ) > MAXPATHL-10) {
		fprintf( stderr,
	    "error in %s: 'vnmrsystem' has too many characters\n", cname
		);
		return( -1 );
	}

	strcpy( &conpar_path[ 0 ], systemdir );
#ifdef UNIX
	strcat( &conpar_path[ 0 ], "/conpar" );
#else
	strcat( &conpar_path[ 0 ], "conpar" );
#endif

	ival = access( &conpar_path[ 0 ], R_OK );
	if (ival != 0) {
		fprintf( stderr, "%s: cannot access %s\n", cname, &conpar_path[ 0 ] );
		perror( "reason" );
		return( -1 );
	} 
	ival = P_read( SYSTEMGLOBAL, &conpar_path[ 0 ] );
	return( ival );
}

static int get_params(char *cname, int  argc, char *argv[] )
{
	int	ival;

	if (argc != 2) {
		fprintf( stderr, "%s:  require 2 file names as parameters\n", cname );
		return( -1 );
	}

	ival = access( argv[ 0 ], R_OK );
	if (ival != 0) {
		fprintf( stderr, "%s: cannot access %s\n", cname, argv[ 0 ] );
		perror( "reason" );
		return( -1 );
	} 
	ival = P_read( CURRENT, argv[ 0 ] );
	if (ival != 0) {
		fprintf( stderr, "error reading shims from %s\n", argv[ 0 ] );
		return( -1 );
	}

	ival = access( argv[ 1 ], R_OK );
	if (ival != 0) {
		fprintf( stderr, "%s: cannot access %s\n", cname, argv[ 1 ] );
		perror( "reason" );
		return( -1 );
	} 
	ival = P_read( PROCESSED, argv[ 1 ] );
	if (ival != 0) {
		fprintf( stderr, "error reading shims from %s\n", argv[ 1 ] );
		return( -1 );
	}

	return( 0 );
}

/*  Below are various symbols required by routines in magiclib.a
    or unmrlib.a which are needed by the ``xdcvt'' program.  They
    are defined here to prevent the ``ld'' program from bringing
    in the VNMR module which defines them, as the VNMR version
    includes references to SUN libraries which we would rather not
    load.								*/

/*  This represents all the VNMR software symbols which need to be
    defined somehow so the application program can read a parameter
    set (CURPAR, PROCPAR, GLOBAL, CONPAR, etc.) and work with the
    parameters in that set.  (VNMR Source file pvars.c)			*/

int	Dflag = 0;
int	Eflag = 0;

/*  The next two actually do something.  */

char *skymalloc(memsize)
int memsize;
{
	return( (char *) malloc(memsize) );
}

void skyfree(memptr)
unsigned *memptr;
{
	free(memptr);
	return;
}

void unsetMagicVar(addr)
int addr;
{
}

void WerrprintfWithPos()
{
	printf( "WerrprintfWithPos called\n" );
}

void Werrprintf()
{
	printf( "Werrprintf called\n" );
}

void Wscrprintf()
{
	printf( "Wscrprintf called\n" );
}

void Wprintfpos()
{
	printf( "Wprintfpos called\n" );
}

void Winfoprintf()
{
	printf( "Winfoprintf called\n" );
}
