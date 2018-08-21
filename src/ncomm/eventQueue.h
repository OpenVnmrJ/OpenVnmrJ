/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCeventQueue
#define INCeventQueue

#ifndef  NUMBER_QUEUE_ENTRIES
#define  NUMBER_QUEUE_ENTRIES	1000
#endif

/*  The definition of the EventQueueEntry is public
    The definition of the EventQueue is private and has been moved to
    another include file.						*/

typedef struct EventQueueEntry {
	int	 type;
	void	*data;
} EVENT_Q_ENTRY;

typedef struct EventQueue *EVENT_Q_OBJ;

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__STDC__) || defined(__cplusplus)
extern EVENT_Q_OBJ createEventQueue( int queue_size );
extern int addEventQueueEntry( EVENT_Q_OBJ q, struct EventQueueEntry *pEntry );
extern int removeEventQueueEntry( EVENT_Q_OBJ q, struct EventQueueEntry *pEntry );
extern void printEventQueueEntries( FILE *filep, EVENT_Q_OBJ q );
#else
extern EVENT_Q_OBJ createEventQueue( );
extern int addEventQueueEntry();
extern int removeEventQueueEntry();
extern void printEventQueueEntries();
#endif

#ifdef __cplusplus
}

#endif /* __cplusplus  */
#endif  /* INCeventQueue */
