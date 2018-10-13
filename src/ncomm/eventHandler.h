/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef __INCeventHandlerh
#define __INCeventHandlerh

#ifndef  DEFAULT_EVENTQUEUE_SIZE
#define  DEFAULT_EVENTQUEUE_SIZE	1000
#endif /* DEFAULT_EVENTQUEUE_SIZE */

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__STDC__) || defined(__cplusplus)
extern int setupForAsync( int number_of_events, sigset_t *starting_set );
extern void blockAllEvents();
extern void unblockAllEvents();
extern int registerAsyncHandlers(
        int eventType, 
        void (*signalHandler)(),
        void (*callBackProg)()
);
extern int unregisterAsyncHandlers( int eventType );
extern int processNonInterrupt( int typeOfEntry, void *data );
extern void asyncMainLoop();
extern void printPendingEntries( FILE *filep );
#else
extern int setupForAsync();
extern void blockAllEvents();
extern void unblockAllEvents();
extern int registerAsyncHandlers();
extern int unregisterAsyncHandlers();
extern int processNonInterrupt();
extern void asyncMainLoop();
extern void printPendingEntries();
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus  */

#endif /* __INCeventHandlerh */
