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
09-01-94       first draft
09-07-94       setupForAsync has a 2nd argument, the initial mask of
               signals to block
09-08-94       added methods to block and unblock signals
09-09-94       added method to print queue of processing pending at
               non-interrupt level.  reworked unblockAllEvents to
               unblock just the signals that blockAllEvents blocked.
               (suggestion of GB)
09-26-94       Store information about asynchronous events if
               DEBUG_ASYNC is set when this program is compiled.
*/

/*
DESCRIPTION

This program works with all consoles that use Wind River System's
VxWorks.  This program forms part of the host computer software, the
acquisition "procs", Expproc, Sendproc, Recvproc.  It implements an
asynchronous event handling system.  These "procs" run in response
to external events, which are presented (or delivered) to the
software as UNIX signals.  The response to each signal occurs in
two steps.

First is the processing that occurs as a direct result of delivering
the signal.  This is the interrupt-level processing. It may be that
all processing that is required in response to the external event can
be done in the interrupt program.  However, the interrupt cannot call
useful UNIX programs like malloc, because these programs are not
reentrant and subtle and catastrophic errors can result if they are
called from an interrupt program.

So we have created a second level of processing that happens at
a lower priority, at non-interrupt level.  The connection between
the two levels occurs through an event queue.  The interrupt program
adds an entry to the event queue.  Once all interrupts have been
handled and the system is (even if momentarily) quiet, this program,
the Event Handler then pulls entries off the queue and performs the
processing implied by that entry.  Each entry has a Callback program
and a argument for that program.  See processQueueEntry.  Since the
callback program runs at normal, non-interrupt level, it can call
any UNIX program, including malloc and free.

This design is based on the way the X window system and the VxWorks
kernel work.  For example, the VxWorks kernel has a Work Queue into
which program which respond to external interrupts can place entries.


INTERNAL

Compile switch:

Use DEBUG_ASYNC to store useful information about asynchronous events.
With DEBUG_ASYNC set when you compile this program, you get a routine
printEventStats which prints out what the programs have been storing.
*/


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

#include "errLogLib.h"
#include "listObj.h"
#include "eventQueue.h"
#include "eventHandler.h"

typedef struct asyncData {
	sigset_t	blockMask;
	EVENT_Q_OBJ	asyncEventQ;
	LIST_OBJ	listOfSignals;
} *ASYNC_HANDLE;

/*  The listOfSignals is a list of type EVENT_ITEM  */

typedef struct registeredEvent {
	int	  eventType;
	void	(*signalHandler)();
	void	(*callBackProg)();
#ifdef DEBUG_ASYNC
	int	  count;
#endif
} *EVENT_ITEM;


static ASYNC_HANDLE	thisHandle;


/*****************************************************************
*
*   setupForAsync -  set up your process to work with asynchronous events
*
*   
*/

int setupForAsync( int number_of_events, sigset_t *starting_set )
{
	if (thisHandle != NULL) {
		errLogRet(ErrLogOp,debugInfo, "process already setup for asynchronous events\n" );
		return( -1 );
	}

	thisHandle = (ASYNC_HANDLE) malloc( sizeof( struct asyncData ) );
	if (thisHandle == NULL) {
		errLogRet(ErrLogOp,debugInfo, "cannot allocate space for async data\n" );
		errno = ENOMEM;
		return( -1 );
	}

	thisHandle->asyncEventQ = createEventQueue( number_of_events );
	if (thisHandle->asyncEventQ == NULL) {
		errLogRet(ErrLogOp,debugInfo, "cannot allocate space for async event queue\n" );
		free( thisHandle );
		errno = ENOMEM;
		return( -1 );
	}

	if (starting_set == NULL)
	  sigemptyset( &(thisHandle->blockMask) );
	else
	  thisHandle->blockMask = *starting_set;

	thisHandle->listOfSignals = NULL;

	return( 0 );
}

/*****************************************************************
*
*   blockAllEvents -  block any asynchronous events from occurring
*
*   
*/

void blockAllEvents()
{

/*  For now, the current signal mask is lost...  */

	if (thisHandle != NULL)
	  sigprocmask( SIG_BLOCK, &(thisHandle->blockMask), NULL );
}

/*****************************************************************
*
*   unblockAllEvents -  unblock asynchronous events
*
*   
*/

void unblockAllEvents()
{

/*  unblock the signals that block all events (above) blocked  */

	if (thisHandle != NULL)
	  sigprocmask( SIG_UNBLOCK, &(thisHandle->blockMask), NULL );
}

/*****************************************************************
*
*   processNonInterrupt -  request processing at non-interrupt level
*
*   
*/

int processNonInterrupt( int typeOfEntry, void *data )
{
	int		ival;
	EVENT_Q_ENTRY	queueEntry;

	queueEntry.type = typeOfEntry;
	queueEntry.data = data;
	ival = addEventQueueEntry( thisHandle->asyncEventQ, &queueEntry );

	return( 0 );
}

static int
cmpEventType( EVENT_ITEM listItem, EVENT_ITEM argItem )
{
	if (listItem->eventType == argItem->eventType)
	  return( 0 );
	else
	  return( 1 );
}

static void
ourHandler( int signal )
{
	int		iter;
	EVENT_ITEM	currentItem;
	EVENT_Q_ENTRY	backup;

	for (iter = 0; ; iter++) {
		currentItem = getItem( thisHandle->listOfSignals, iter );
		if (currentItem == NULL)
		  break;

		if (currentItem->eventType == signal) {
#ifdef DEBUG_ASYNC
			currentItem->count++;
#endif
			if (currentItem->signalHandler)
			  (*(currentItem->signalHandler))( signal );
			else {
				backup.type = signal;
				backup.data = (void *) 0;

				addEventQueueEntry( thisHandle->asyncEventQ, &backup );
			}

			return;
		}
	}
	errLogRet(ErrLogOp,debugInfo, "received unregistered signal %d\n", signal );
}

static int
registerNewMask( LIST_OBJ listOfSignals, sigset_t *newMask )
{
	int			iter, ival;
	EVENT_ITEM		currentItem;
	struct sigaction	thisAction;

	for (iter = 0; ; iter++) {
		currentItem = getItem( listOfSignals, iter );
		if (currentItem == NULL)
		  break;

		ival = sigaction( currentItem->eventType, NULL, &thisAction );
		thisAction.sa_mask = *newMask;
		ival = sigaction( currentItem->eventType, &thisAction, NULL );
	}

	return( 0 );
}

/*  At this time we do not save the original action for the signal.
    If the signal is unregistered as one caught by this subsystem,
    it will revert to the default action for no signal handler will
    be present anymore.							*/

static int
registerOurHandler( int signal, sigset_t *mask )
{
	int			ival;
	struct sigaction	thisAction;

	thisAction.sa_mask = *mask;
	thisAction.sa_flags = 0;
	thisAction.sa_handler = ourHandler;

	ival = sigaction( signal, &thisAction, NULL );
	return( ival );
}

/*****************************************************************
*
*   registerAsyncHandlers -  specify what is to be called
*
*   If you have not called setupForAsync, this routine will
*   call it for you.  You will get no signals blocked (other
*   than the ones you register with this routine) and an
*   event queue with 1000 entries.
*   
*/

int registerAsyncHandlers(
	int eventType, 
	void (*signalHandler)(),
	void (*callBackProg)()
)
{
	int		seqn;
	sigset_t	oldMask;
	EVENT_ITEM	argItem;

	if (signalHandler == NULL && callBackProg == NULL) {
		errno = EINVAL;
		return( -1 );
	}

/*  By default you will get no signals blocked and an event queue of 1000 entries.  */

	if (thisHandle == NULL) {
		int	ival;

		ival = setupForAsync( DEFAULT_EVENTQUEUE_SIZE, NULL );
		if (ival != 0)				/* verify setup worked */
		  return( -1 );			   /* setupForAsync sets errno */
	}

	argItem = (EVENT_ITEM) malloc( sizeof( struct registeredEvent ) );
	if (argItem == NULL) {
		errLogRet(ErrLogOp,debugInfo,
	   "cannot allocate temporary space in register async handlers\n"
		);
		errno = ENOMEM;
		return( -1 );
	}

	argItem->eventType = eventType;
	seqn = searchItem( thisHandle->listOfSignals, argItem, cmpEventType );
	if (seqn >= 0) {
#ifdef DEBUG_ASYNC
		errLogRet(ErrLogOp,debugInfo,
	    "async handlers already registered for event type %d\n", eventType
		);
#endif
		errno = EALREADY;
		free( argItem );
		return( -1 );
	}

	sigaddset( &(thisHandle->blockMask), eventType );
	sigprocmask( SIG_BLOCK, &(thisHandle->blockMask), &oldMask );
	thisHandle->listOfSignals = appendItem( thisHandle->listOfSignals, argItem );
	registerNewMask( thisHandle->listOfSignals, &(thisHandle->blockMask) );
	registerOurHandler( eventType, &(thisHandle->blockMask) );
	argItem->signalHandler = signalHandler;
	argItem->callBackProg = callBackProg;
#ifdef DEBUG_ASYNC
	argItem->count = 0;
#endif
	sigprocmask( SIG_SETMASK, &oldMask, NULL );

	return( 0 );
}


static int
unregisterOurHandler( int signal )
{
	int	ival;

	ival = sigaction( signal, NULL, NULL );
	return( ival );
}

/*****************************************************************
*
*   unregisterAsyncHandlers
*
*   
*/

int unregisterAsyncHandlers( int eventType )
{
	int		seqn;
	sigset_t	oldMask;
	EVENT_ITEM	argItem;

	if (thisHandle == NULL) {
		errLogRet(ErrLogOp,debugInfo, "process not setup for asynchronous events\n" );
		return( -1 );
	}

	argItem = (EVENT_ITEM) malloc( sizeof( struct registeredEvent ) );
	if (argItem == NULL) {
		errLogRet(ErrLogOp,debugInfo,
	   "cannot allocate temporary space in unregister async handlers\n"
		);
		errno = ENOMEM;
		return( -1 );
	}

	argItem->eventType = eventType;
	seqn = searchItem( thisHandle->listOfSignals, argItem, cmpEventType );
	if (seqn < 0) {
#ifdef DEBUG_ASYNC
		errLogRet(ErrLogOp,debugInfo,
	    "async handlers not registered for event type %d\n", eventType
		);
#endif
		errno = ESRCH;
		free( argItem );
		return( -1 );
	}

	sigprocmask( SIG_BLOCK, &(thisHandle->blockMask), &oldMask );
	sigdelset( &(thisHandle->blockMask), eventType );
	thisHandle->listOfSignals = deleteItem(
		thisHandle->listOfSignals,
		argItem,
		cmpEventType,
		free
	);
	registerNewMask( thisHandle->listOfSignals, &(thisHandle->blockMask) );
	unregisterOurHandler( eventType );
	sigprocmask( SIG_SETMASK, &oldMask, NULL );

	return( 0 );
}


static void
processQueueEntry( LIST_OBJ listOfSignals, struct EventQueueEntry *currentEntry )
{
	int		iter;
	EVENT_ITEM	currentItem;

	for (iter = 0; ; iter++) {
		currentItem = getItem( listOfSignals, iter );
		if (currentItem == NULL)
		  break;

		if (currentItem->eventType == currentEntry->type) {
			if (currentItem->callBackProg)
			  (*(currentItem->callBackProg))( currentEntry->data );

			return;
		}
	}
	errLogRet(ErrLogOp,debugInfo, "received unregistered queue entry %d\n", currentEntry->type );
}


/*****************************************************************
*
*   asyncMainLoop
*
*   INTERNAL
*
*   The application calls this program when it is ready to switch into asychronous
*   mode.  This program never returns.  Further application level processing occurs
*   in processQueueEntry, for that routine can call the callback programs registered
*   with registerAsyncHandlers.  (The application can also specifies an interrupt
*   handler, so in principle application level processing could occur there too.)
*   
*   The program is written as an infinite loop using the for (;;) construct.  For
*   each traversal of the loop, it first processes each event queue entry until it
*   finds no more.  Signals are blocked while it examines the queue and unblocked
*   while processing takes place for each entry.  Once the processing completes,
*   signals are blocked again before re-examining the queue.  When the program
*   finds no entries are present on the queue, it calls sigsuspend to wait for
*   another event.  Notice that as it goes into sigsuspend signals are blocked.
*
*   We examine the queue first for in the time between calling registerAsyncHandlers
*   and calling asyncMainLoop, one or more signals can be delivered which result in
*   entries being placed on the event queue.
*   
*/

void asyncMainLoop()
{
	sigset_t		oldMask;
	struct EventQueueEntry	an_entry;

	sigprocmask( SIG_SETMASK, NULL, &oldMask );

	for (;;) {
		int	ival;

		sigprocmask( SIG_BLOCK, &(thisHandle->blockMask), NULL );
		while (removeEventQueueEntry( thisHandle->asyncEventQ, &an_entry ) == 0) {
			sigprocmask( SIG_UNBLOCK, &(thisHandle->blockMask), NULL );
			processQueueEntry( thisHandle->listOfSignals, &an_entry );
			sigprocmask( SIG_BLOCK, &(thisHandle->blockMask), NULL );
		}

		ival = sigsuspend( &oldMask );
	}
}

/*****************************************************************
*
*   printPendingEntries -  print out all pending entries
*
*   You may call this from an interrupt routine or signal handler
*   provided you do not object if fprintf is called.
*   
*/

void printPendingEntries( FILE *filep )
/* FILE *filep -  file pointer to receive the output or NULL for stdout */
{

/*  printEventQueueEntries will substitute stdout
    if given NULL as the file pointer */

	printEventQueueEntries( filep, thisHandle->asyncEventQ );
}

#ifdef DEBUG_ASYNC
void printEventStats( FILE *filep )
/* FILE *filep -  file pointer to receive the output or NULL for stdout */
{
	int		iter;
	EVENT_ITEM	currentItem;

	if (filep == NULL)
	  filep = stdout;

	for (iter = 0; ; iter++) {
		currentItem = getItem( thisHandle->listOfSignals, iter );
		if (currentItem == NULL)
		  break;

		fprintf( filep, "event type %d occurred %d times\n",
				  currentItem->eventType, currentItem->count
		);
	}
}
#endif
