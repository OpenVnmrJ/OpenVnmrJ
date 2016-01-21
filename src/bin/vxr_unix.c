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

/*  Starts with a VXR text file, converting it into UNIX format.  */

#include  <stdio.h>
#include  <sys/types.h>
#include  <sys/stat.h>

#define  NUL		0x00				/*  Not NULL !!  */
#define  LF		0x0a
#define  EOM		0x19
#define  OBUFSIZ	280

main( argc, argv )
int argc;
char *argv[];
{
	char		ibuf[ 514 ], obuf[ OBUFSIZ+2 ], tchar, *tptr;
	int		fd, finished, iptr, ival, ovf_rpt;
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

	fd = open( argv[ 1 ], 0 );		/*  Read only  */
	if (fd < 0) {
		perror( "Error opening input file" );
		exit( 4 );
	}

	fp = fopen( argv[ 2 ], "w+" );		/*  Write only  */
	if (fp == NULL) {
		perror( "Error creating output file" );
		exit( 5 );
	}

/*  Process each block in the input file.  */

	tptr = &obuf[ 0 ];
	ovf_rpt = 0;				/*  Overflow not reported  */
	do {
		ival = read( fd, &ibuf[ 0 ], 512 );
		if (ival < 0) {
			perror( "Error reading from file\n" );
			exit( 11 );
		}

/*  End of file should never occur, becuase a ^Y should be found first.  */

		else if (ival == 0) {
			printf( "End of file detected\n" );
			close( fd );
			fclose( fp );
			exit( 0 );
		}

		finished = 0;
		iptr = 0;

	/*  Process each character in the buffer  */

		do {
			tchar = ibuf[ iptr++ ] & 0x7f;
			if (tchar == LF || tchar == EOM || tchar == NUL) {

		/*  If the cuurent character is ^Y and no characters
		    have been copied to the output buffer, do NOT write
		    the empty line out.					*/

				if (tchar != EOM || tptr != &obuf[ 0 ]) {
					*tptr++ = LF;
					*tptr   = NUL;
					fputs( &obuf[ 0 ], fp );
				}
				if ( !( finished = (tchar == EOM) ) ) {
					tptr = &obuf[ 0 ];
					ovf_rpt = 0;
				}
			}
			else if (tptr - &obuf[ 0 ] < OBUFSIZ)
			 *tptr++ = tchar;
			else {
				if ( !ovf_rpt )
				 printf( "Line too long\n" );
				ovf_rpt = 131071;
			}
		}
		while (iptr < ival && !finished);
	}
	while ( !finished );

	if (tptr != &obuf[ 0 ] ) {
		*tptr++ = LF;
		*tptr   = '\000';
		fputs( &obuf[ 0 ], fp );
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
