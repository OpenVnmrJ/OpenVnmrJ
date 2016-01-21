/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*  test, evaluation and demonstration program ...  not part of any product.  */

#include <stdio.h>

#include "eventQueue.h"

struct EventQueue *pQueue;

main()
{
	char			 inputbuf[ 122 ];
	char			*aval;
	int			 ival;
	struct EventQueueEntry	 current_entry;

	pQueue = (struct EventQueue *) createEventQueue( 4 );
	while (1) {
		printf( "add/remove/print/quit (a/r/p/q): " );
		aval = fgets( &inputbuf[ 0 ], sizeof( inputbuf ) - 1, stdin );
		if (aval == 0) {
			return( 0 );
		}

		switch (inputbuf[ 0 ]) {
		  case 'a':
		  case 'A':
			printf( ": " );
			aval = fgets( &inputbuf[ 0 ], sizeof( inputbuf ) - 1, stdin );
			if (aval == NULL)
			  continue;
			aval = (char *) malloc( strlen( &inputbuf[ 0 ] ) + 1 );
			strcpy( aval, &inputbuf[ 0 ] );
			current_entry.type = 0;
			current_entry.data = aval;
			ival = addEventQueueEntry( pQueue, &current_entry );
			if (ival != 0) {
				printf( "queue full\n" );
			}
			break;

		  case 'r':
		  case 'R':
			ival = removeEventQueueEntry(pQueue, &current_entry );
			if (ival != 0) {
				printf( "queue empty\n" );
			}
			else {
				printf( "%s", current_entry.data );
			}
			break;

		  case 'p':
		  case 'P':
		  case 'd':
		  case 'D':
			printEventQueueEntries( stdout, pQueue );
			break;

		  case 'q':
		  case 'Q':
			return( 0 );

		  default:
			break;
		}
	}
}
