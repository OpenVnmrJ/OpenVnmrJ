/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*
modification history
--------------------
9-08-94       added method to print the entries in a Queue
8-31-94       event queue definiton moved to eventQueueP.h
8-19-94       first draft
*/


/*
DESCRIPTION

An event queue is a facility to track asynchronous events.  An interrupt
program or a signal handler may add or remove entries from the queue,
since none of the procedures call malloc or free.  (createEventQueue is
an exception.)

Underlying these programs is the assumption that access to the queue is
serialized.

These programs do not know how to serialize the access; therefore the
application MUST ARRANGE for this or the results will not be predictable
except to say it is clear you won't like the results!

*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "eventQueueP.h"


#define ADDR_FIRST_ARRAY_ENTRY( q )	(&((q)->e_array[ 0 ]))
#define ADDR_LAST_ARRAY_ENTRY( q )	(&((q)->e_array[ ((q)->queue_size) - 1 ]))


/*  Underlying these programs is the assumption
    that access to the queue is serialized.	*/

static int
queue_is_empty( EVENT_Q_OBJ q )
{
	return( q->first == NULL && q->last == NULL );
}

static int
queue_is_full( EVENT_Q_OBJ q )
{
	if (q->first == ADDR_FIRST_ARRAY_ENTRY( q ) &&
	    q->last == ADDR_LAST_ARRAY_ENTRY( q ))
	  return( 131071 );
	if (q->first - q->last == 1)		/* NOT sizeof( EventQueueEntry ) !!!! */
	  return( 131071 );

	return( 0 );
}


/**************************************************************
*
*   createEventQueue
*
*   Never create a Queue from an interrupt routine
*   or a signal handler.
*/

EVENT_Q_OBJ createEventQueue( int queue_size )
/* int queue_size -  size of the queue expressed as number of entries */
{
	EVENT_Q_OBJ 	qaddr;
	int	 	memory_size;

	if (queue_size < 1)
	  queue_size = NUMBER_QUEUE_ENTRIES;
	memory_size = queue_size * sizeof( struct EventQueueEntry ) +
			sizeof( struct EventQueue );
	qaddr = (EVENT_Q_OBJ)  malloc( memory_size );
	if (qaddr == NULL)
	  return( NULL );

	memset( qaddr, 0, memory_size );
	qaddr->queue_size = queue_size;

/*  The memset operation has the size effect of rendering the queue empty.  */
	
	return( (EVENT_Q_OBJ) qaddr );
}

/**************************************************************
*
*   addEventQueueEntry
*
*/

int addEventQueueEntry( EVENT_Q_OBJ q, struct EventQueueEntry *pEntry )
/*  EVENT_Q_OBJ q         -  event queue object to receive entry */
/*  EVENT_Q_ENTRY *pEntry -  address of entry to be added */
{
	struct EventQueueEntry	*current_entry;

	if (queue_is_full( q )) {
		return( -1 );
	}

	if (queue_is_empty( q )) {
		current_entry = q->first = ADDR_FIRST_ARRAY_ENTRY( q );
	}
	else {
		if (q->last == ADDR_LAST_ARRAY_ENTRY( q ))
		  current_entry = ADDR_FIRST_ARRAY_ENTRY( q );
		else
		  current_entry = q->last + 1;	/* NOT sizeof( EventQueueEntry ) !!!! */
	}

	*current_entry = *pEntry;
	q->last = current_entry;

	return( 0 );
}

/**************************************************************
*
*   removeEventQueueEntry
*
*/

int removeEventQueueEntry( EVENT_Q_OBJ q, struct EventQueueEntry *pEntry )
/*  EVENT_Q_OBJ q         -  event queue object from which the entry is to be dequeued */
/*  EVENT_Q_ENTRY *pEntry -  address where entry is to be stored */
{
	struct EventQueueEntry	*current_entry;

	if (queue_is_empty( q ))
	  return( -1 );

	current_entry = q->first;
	*pEntry = *current_entry;

	if (q->first == q->last) {
		q->first = q->last = NULL;
	}
	else if (q->first == ADDR_LAST_ARRAY_ENTRY( q )) {
		q->first = ADDR_FIRST_ARRAY_ENTRY( q );
	}
	else {
		q->first += 1;			/* NOT sizeof( EventQueueEntry ) !!!! */
	}

	return( 0 );
}


static void
printEventQueueEntry( FILE *filep, int order, EVENT_Q_ENTRY *pEntry )
{
	fprintf( filep, "%d:   %d  %p\n", order, pEntry->type,  pEntry->data );
}

/**************************************************************
*
*   printEventQueueEntries
*
*/

void printEventQueueEntries( FILE *filep, EVENT_Q_OBJ q )
/*  FILE *filep -  output file stream or NULL for stdout */
/*  EVENT_Q_OBJ -  queue whoese entries are to be printed */
{
	int			 order;
	struct EventQueueEntry	*current_entry;

	if (filep == NULL)
	  filep = stdout;

	if (queue_is_empty( q )) {
		fprintf( filep, "queue object at %p is empty\n", q );
		return;
	}

	current_entry = q->first;
	order = 1;
	for (;;) {
		printEventQueueEntry( filep, order, current_entry );
		order++;
		if (current_entry == q->last)
		  break;
		else if (current_entry == ADDR_LAST_ARRAY_ENTRY( q ))
		  current_entry = ADDR_FIRST_ARRAY_ENTRY( q );
		else
		  current_entry = current_entry + 1;	/* NOT sizeof( EventQueueEntry ) !!!! */
	}
}
