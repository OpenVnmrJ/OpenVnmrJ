/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*  Extended, to include "direct" as well as "queued" processing in
    response to an interrupt.  Direct processing occurs in the interrupt
    routine itself.  Queued processing was originally the only form
    supported.  In the latter, the interrupt simply places an entry on
    the event queue.  Once the interrupt routine was complete, the event
    handler kernel then removed entries from this queue and called the
    callback program specified in the entry.

    For "server" sockets, this caused a problem.  The server (Expproc)
    calls accept, which returns a new file desciptor.  In our application,
    the server thn reads from this new socket until no more data is
    present.  However inevitable there was a pause between the SIGIO
    on the server socket and the call to accept.  If, in the interrum,
    another SIGIO arrived, the server socket would still show up as
    active.  Thus a second entry for the server socket could be placed
    on the event queue, event though only one connection had been made.
    Thus the server would call accept twice, leading to difficulties.

    Therefore "direct" processing was developed, to allow the accept
    call to be done in the interrupt routine itself.  A new program,
    setFdDirectAsync, arranges for this.    March, 19, 1998		*/

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/param.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include "eventHandler.h"
#include "errLogLib.h"

#ifndef LINUX
#include <sys/filio.h>
#endif
#ifdef CYGWIN
#include <asm/socket.h>
#endif

#define  QUEUED 0
#define  DIRECT	1

#define  NOT_OPEN		-1	/* taken from sockets.c */

typedef struct _asyncIoEntry {
	void	 *clientData;
	void	(*callback)();
	int	  entryType;
	int	  fd;
#ifdef DEBUG_ASYNC
        int       count;
#endif
} *ASYNC_IO_ENTRY;

#ifndef NOFILE
#define NOFILE 256
#endif

extern int find_maxfd( register fd_set *fdmaskp );

static struct _asyncIoEntry	asyncIoEntryArray[ NOFILE ];
static int			maxAsyncIoIndex = 0;
static int			asyncIoSetup = 0;


static void
setupAsyncIo()
{
	int	iter;

	for (iter = 0; iter < NOFILE; iter++) {
		asyncIoEntryArray[ iter ].clientData = NULL;
		asyncIoEntryArray[ iter ].callback = NULL;
		asyncIoEntryArray[ iter ].entryType = QUEUED;
		asyncIoEntryArray[ iter ].fd = NOT_OPEN;
	}
	asyncIoSetup = 131071;
}

static void
sigio_irpt( int signal )
{
	int		fd, iter, maxfd, nfound;
	fd_set		excptfd, readfd, writefd;
	struct timeval	nowait;
	ASYNC_IO_ENTRY	currentEntry;

/* Start by building a mask of possible active file descriptors  */

        (void) signal;
	FD_ZERO( &readfd );
	FD_ZERO( &writefd );
	for (iter = 0; iter < maxAsyncIoIndex; iter++) {
		currentEntry = &asyncIoEntryArray[ iter ];

		fd = currentEntry->fd;
		if (fd < 0 || fd >= FD_SETSIZE) {
			continue;			/* ignore bad entries */
		}

		FD_SET( fd, &readfd );
	}

	maxfd = find_maxfd( &readfd );
	if (maxfd < 0) {
		errLogRet(ErrLogOp,debugInfo, "programming error in find maxfd\n" );
		return;
	}
	else if (maxfd == 0) {
		errLogRet(ErrLogOp,debugInfo, "SIGIO received, but nothing asynchronous was found\n" );
		return;
	}

	excptfd = readfd;

/*  You have to tell select() not to wait; with an address of NULL, it
    waits until there is activity on one of the selected file descriptors.  */

	nowait.tv_sec = 0;
	nowait.tv_usec = 0;

	nfound = select( maxfd, &readfd, &writefd, &excptfd, &nowait );
	if (nfound < 1) {
		/*fprintf( stderr, "SIGIO received, but nothing active was found\n" );*/
		return;
	}

/*  Using the mask of known active file descriptors, queue up
    non-interrupt level processing for each active file descriptor.  */

	for (iter = 0; iter < maxAsyncIoIndex; iter++) {
		currentEntry = &asyncIoEntryArray[ iter ];

		fd = currentEntry->fd;
		if (fd < 0 || fd >= FD_SETSIZE) {
			continue;			/* ignore bad entries */
		}

	   /* Note the ioctl command is only for sockets ... */

		if (FD_ISSET( fd, &readfd )) {
			int ival;

			if (currentEntry->entryType == DIRECT)
			  (*currentEntry->callback)( currentEntry->clientData );
			else {
#ifdef LINUX
#ifdef CYGWIN
                                int zero = 0;
				ival = ioctl( fd, FIOASYNC, &zero );
#else
                                fcntl(fd, F_SETOWN, (int) getpid());
                                ival = fcntl(fd, F_GETFL);
                                ival |= O_ASYNC;
                                fcntl(fd, F_SETFL, ival);
#endif
#else
                                int zero = 0;
				ival = ioctl( fd, FIOASYNC, &zero );
#endif
				processNonInterrupt( SIGIO, (void *) fd );
			}
#ifdef DEBUG_ASYNC
			currentEntry->count++;
#endif
		}

	   /*  following might be amenable to a SIGPIPE entry in the event queue  */

		if (FD_ISSET( fd, &excptfd )) {
			/*setSocketNonAsync( thisSocket );*/
			errLogRet(ErrLogOp,debugInfo, "detected error on fd %d\n", fd );
		}
	}

	return;
}

/*  This program is called from processQueueEntry (see eventHandler.c,
    SCCS category ncomm) and runs at normal (non-interrupt) level.  It
    calls the callback, as specified for the file descriptor in setFdAsync.
    For file descriptors set to be asynchronous with setFdDirectAsync,
    process_async_io will never be called; all processing for those file
    desciptors is done from the interrupt routine.			*/

static void
process_async_io( int fd )
{
   int	iter;
   ASYNC_IO_ENTRY currentEntry;

   for (iter = 0; iter < maxAsyncIoIndex; iter++)
   {
      currentEntry = &asyncIoEntryArray[ iter ];
      if (fd == currentEntry->fd)
      {
         int ival;

/*  It is essential that the socket be set to asynchronous again,
    otherwise the connect handler (conhandler, SCCS category expproc)
    can never read its messages from the console.  It is a bit
    inelegent to call io control directly, but we do not have the
    corresponding socket object.  A future version could correct this...  */

#ifdef LINUX
#ifdef CYGWIN
         int one = 1;

	 ival = ioctl( fd, FIOASYNC, &one );
#else
         fcntl(fd, F_SETOWN, (int) getpid());
         ival = fcntl(fd, F_GETFL);
         ival |= O_ASYNC;
         fcntl(fd, F_SETFL, ival);
#endif
#else
         int one = 1;

         ival = ioctl( fd, FIOASYNC, &one );
#endif
         (*currentEntry->callback)( currentEntry->clientData );
      }
   }
}

static int
locateUnusedEntry()
{
	int	iter, index;

	index = -1;
	for (iter = 0; iter < NOFILE; iter++)
	  if (asyncIoEntryArray[ iter ].fd == NOT_OPEN) {
		index = iter;
		break;
	  }

	return( index );
}

static int
locateEntryByFd( int fd )
{
	int	index, iter;

	index = -1;
	for (iter = 0; iter < maxAsyncIoIndex; iter++)
	  if (asyncIoEntryArray[ iter ].fd == fd) {
		index = iter;
		break;
	  }

	return( index );
}

int setFdAsync( int fd, void *clientData, void (*callback)() )
{
	int		index, iter, ival;
	ASYNC_IO_ENTRY	currentEntry;

	if (fd < 0 || fd >= FD_SETSIZE) {
		errno = EINVAL;
		return( -1 );
	}
	if (callback == NULL) {
		errno = EINVAL;
		return( -1 );
	}

/*  Warning:  be sure your application calls setFdAsync (or registerFdAsync)
    from the normal processing level first, not from an interrupt.	*/

	if (asyncIoSetup == 0) {
		setupAsyncIo();
		ival = registerAsyncHandlers( SIGIO, sigio_irpt, process_async_io );
		if (ival != 0)
		  return( -1 );
	}

/*  Next block of program allows the callback to be changed for
    a file descriptor which is already set to be asynchronous  */

	else {
		for (iter = 0; iter < maxAsyncIoIndex; iter++) {
			currentEntry = &asyncIoEntryArray[ iter ];

			if (fd == currentEntry->fd) {
				if (callback == currentEntry->callback &&
				    clientData == currentEntry->clientData) {
					return( 0 );
				}

				currentEntry->callback = callback;
				currentEntry->clientData = clientData;

				return( 0 );
			}
		}
	}

/*  Come here if:
    1) we never found this file descriptor in the list of asynchronous
       file descriptors
    2) the list of asynchronous file descriptors was NULL (equivalent
       to an empty list)	*/

	index = locateUnusedEntry();
	if (index < 0) {
		DPRINT( -1,
	   "no unused entries in set file descriptor asynchronous\n"
		);
		return( -1 );
	}

	if (index + 1 > maxAsyncIoIndex)
	  maxAsyncIoIndex = index + 1;

	currentEntry = &asyncIoEntryArray[ index ];
	currentEntry->fd = fd;
	currentEntry->entryType = QUEUED;
	currentEntry->clientData = clientData;
	currentEntry->callback = callback;
#ifdef DEBUG_ASYNC
	currentEntry->count = 0;
#endif
	return( 0 );
}

int setFdNonAsync( int fd )
/* int fd -        fd to be used */
{
	int		index, iter;
	ASYNC_IO_ENTRY	currentEntry;

	if (fd < 0 || fd >= FD_SETSIZE) {
		errno = EINVAL;
		return( -1 );
	}

	index = locateEntryByFd( fd );
        if (index < 0)
	   return(0);  /* not here just return */

	currentEntry = &asyncIoEntryArray[ index ];
	currentEntry->fd = -1;
	currentEntry->clientData = NULL;
	currentEntry->callback = NULL;

	index = NOFILE - 1;
	for (iter = NOFILE - 1; iter >= 0; iter--)
	  if (asyncIoEntryArray[ iter ].fd != NOT_OPEN) {
		index = iter;
		break;
	  }

	if (index + 1 < maxAsyncIoIndex)
	  maxAsyncIoIndex = index + 1;

	return( 0 );
}

int setFdDirectAsync( int fd, void *clientData, void (*callback)() )
{
	int		index, iter, ival;
	ASYNC_IO_ENTRY	currentEntry;

	if (fd < 0 || fd >= FD_SETSIZE) {
		errno = EINVAL;
		return( -1 );
	}
	if (callback == NULL) {
		errno = EINVAL;
		return( -1 );
	}

/*  Warning:  be sure your application calls setFdAsync (or registerFdAsync)
    from the normal processing level first, not from an interrupt.	*/

	if (asyncIoSetup == 0) {
		setupAsyncIo();
		ival = registerAsyncHandlers( SIGIO, sigio_irpt, process_async_io );
		if (ival != 0)
		  return( -1 );
	}

/*  Next block of program allows the callback to be changed for
    a file descriptor which is already set to be asynchronous  */

	else {
		for (iter = 0; iter < maxAsyncIoIndex; iter++) {
			currentEntry = &asyncIoEntryArray[ iter ];

			if (fd == currentEntry->fd) {
				if (callback == currentEntry->callback &&
				    clientData == currentEntry->clientData) {
					return( 0 );
				}

				currentEntry->callback = callback;
				currentEntry->clientData = clientData;

				return( 0 );
			}
		}
	}

/*  Come here if:
    1) we never found this file descriptor in the list of asynchronous
       file descriptors
    2) the list of asynchronous file descriptors was NULL (equivalent
       to an empty list)	*/

	index = locateUnusedEntry();
	if (index < 0) {
		DPRINT( -1,
	   "no unused entries in set file descriptor asynchronous direct\n"
		);
		return( -1 );
	}

	if (index + 1 > maxAsyncIoIndex)
	  maxAsyncIoIndex = index + 1;

	currentEntry = &asyncIoEntryArray[ index ];
	currentEntry->fd = fd;
	currentEntry->entryType = DIRECT;
	currentEntry->clientData = clientData;
	currentEntry->callback = callback;
#ifdef DEBUG_ASYNC
	currentEntry->count = 0;
#endif
	return( 0 );
}
