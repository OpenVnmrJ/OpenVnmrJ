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

/*  This program documents itself  */

#define  ESC	0x1B

main()
{
	printf( "Setting CODE parameter to TEK\n" );
	printf( "%c%%!0", ESC );
	printf( "Setting number of dialog lines to 32\n" );
	printf( "%cLLB0", ESC );
	printf( "Setting FLAGGING to IN/OUT\n" );
	printf( "%cNF3", ESC );
	printf( "Setting QUEUESIZE to 2500\n" );
	printf( "%cNQB\\4", ESC );		/* '\' is the escape char */
	printf( "Setting CODE parameter to ANSI\n" );
	printf( "%c%%!1", ESC );
}
