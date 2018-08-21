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
#include  <sys/types.h>
#include  <fcntl.h>
#include  <sys/file.h>
#include  <sys/stat.h>

#define  NUL		0x00				/*  Not NULL !!  */
#define  LF		0x0a
#define  EOM		0x19
#define  IBUFSIZ	280
#define  OBUFSIZ	512

main( argc, argv )
int argc;
char *argv[];
{
	char		ibuf[ IBUFSIZ+2 ], obuf[ OBUFSIZ ], tchar, *tptr;
	int		fd, finished, ival, optr, ovf_rpt;
	FILE		*fp;

/*  Check for correct number of arguments.  */

	if (argc != 3) {
		printf( "Use 2 arguments, input and output file\n" );
		exit( 1 );
	}

/*  Check for the two arguments referring to the same file.  */

	ival = cmp_pathnames( argv[ 1 ], argv[ 2 ] );
	if (ival == -1) {
		perror( "Problem with first file" );
		exit( 2 );
	}

/*  Ignore returned value of -2.  The 2nd file may not exist yet.  */

	else if (ival == 0) {
		printf( "Use distinct input and output files\n" );
		exit( 3 );
	}

/*  Open input as an ascii file  */

	fp = fopen( argv[ 1 ], "r" );
	if (fp == NULL) {
		perror( "Error opening input file" );
		exit( 4 );
	}

/*  Output is a binary file.  */

	fd = open( argv[ 2 ], O_WRONLY | O_CREAT | O_TRUNC );
	if (fd < 0 ) {
		perror( "Error creating output file" );
		exit( 5 );
	}
	else fchmod( fd, 0666 );

	optr = 0;
	while ( (tptr = fgets( &ibuf[ 0 ], IBUFSIZ, fp )) != NULL ) {

	/*  First verify the string is terminated with a LF  character. 
	    Program assumes that LF may not equal '\n'			*/

		ival = strlen( &ibuf[ 0 ] );
		if (ival > 0) {
			tptr = &ibuf[ ival-1 ];
			if ( (tchar = *tptr) != '\n') {
				printf( "line in file too long, truncated\n" );
				*(++tptr) = LF;
				*(++tptr) = NUL;
			}
			else *tptr = LF;
		}
		else {
			ibuf[ 0 ] = LF;
			ibuf[ 1 ] = NUL;
		}

	/*  Input string is now guaranteed to be terminated with LF, NUL  */

		tptr = &ibuf[ 0 ];
		while ( (tchar = *tptr++) != NUL ) {
			obuf[ optr++ ] = tchar | 0x80;
			if (optr >= OBUFSIZ) {
				ival = write( fd, &obuf[ 0 ], OBUFSIZ );
				if (ival != OBUFSIZ) {
					perror( "Error writing output file" );
					exit( 11 );
				}

				optr = 0;
			}
		}
	}

/*  Terminate the file with a ^Y character  */

	obuf[ optr ] = EOM;
	ival = write( fd, &obuf[ 0 ], OBUFSIZ );
	if (ival != OBUFSIZ) {
		perror( "Error writing output file" );
		exit( 11 );
	}

	close( fd );
	fclose( fp );
}

/*  Compares two path names by looking up; then comparing the corresponding
    I-node values.

    Returns:
	1		if the two names refer to different files
	0		if the two names refer to the same file
	-1		if the first name is invalid
	-2		if the second name is invalid			*/

int cmp_pathnames( p1, p2 )
char *p1;
char *p2;
{
	int		ival1, ival2;
	struct stat	buf1, buf2;

	ival1 = stat( p1, &buf1 );
	if (ival1 != 0) return( -1 );

	ival2 = stat( p2, &buf2 );
	if (ival2 != 0) return( -2 );

	return( buf1.st_ino != buf2.st_ino );
}
