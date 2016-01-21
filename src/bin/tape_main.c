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
#include <stdio.h>
#define  NINETRACK	1
#define  STREAMER	2

main( argc, argv )
int argc;
char *argv[];
{
	char	tchar;
	int	argp, tdevice, tspec, unum;

	if (argc < 2) {
		tape_help();
		exit( 2 );
	}

	tspec = 0;
	tdevice = STREAMER;
	unum = 0;
	argp = 1;
	while ( argp < argc ) {
		if (*argv[ argp ] != '-') {		/*  Not a switch  */
			if (tdevice == NINETRACK)
			 ntape_main( argc-argp, argv+argp );
			else
			 stape_main( argc-argp, argv+argp );
			exit();
		}

/*  Get the 2nd character of the current argument string.  */

		tchar = *(argv[ argp++ ]+1);
		switch (tchar) {

		 case 'q':
		 case 'Q':
		 case 's':
		 case 'S':
			if (tspec) {
				printf(
				  "Can't specify device more than once\n"
				);
				exit( 10 );
			}
			tdevice = STREAMER;
			tspec = 131071;
			break;

		 case 'n':
		 case 'N':
		 case 'h':
		 case 'H':
		 case '9':
			if (tspec) {
				printf(
				  "Can't specify device more than once\n"
				);
				exit( 10 );
			}
			tdevice = NINETRACK;
			tspec = 131071;
			break;

/*		 case 'u':
		 case 'U':
			unum = atoi( argv[ argp++ ] );
			if (unum < 0 || unum > 3) {
				printf( "Invalud unit number\n" );
				exit( 1 );
			}
			break;				*/

		 default:
			printf( "Invalid switch %c\n", tchar );
			exit( 1 );
		}
	}

	printf( "tape:  You must specify an option\n" );
}

tape_help()
{
	printf( "Usage:\n\n" );
	printf( "    tape -q <option>\n" );
	printf( "    tape -s <option>      to access the streaming tape\n\n" );
	printf( "    tape -9 <option>\n" );
	printf( "    tape -h <option>\n" );
	printf( "    tape -n <option>      to access the 9-track tape\n\n" );
	printf( "The default is the streaming tape.\n\n" );
	printf( "You must specify an option.  Use the 'help' option for\n" );
	printf( "more information about the individual interface\n" );
}
