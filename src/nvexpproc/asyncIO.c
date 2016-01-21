/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
   just food for thought. Quoting the the definitive book on Sockets, by Stevens

   "Unfortunately signal-driven I/O is next to useless with TCP socket.
   They problem is the signal is generated too often, and the occence doesn't tell
   us what happened.
   The follow cause a SIGIO to occur.

   1. a connection request has completed on a listening socket.
   2. a discxonnect request has been initiated
   3. a disconnect request has been completed.
   4. half a connection has been shutdown
   5. data has arrived on a socket
   6. data has been sent from a socket (i.e., the output buffer has free space), or
   7. an asynchronous error occurred"
*/
/*  
    Extended, to include "direct" as well as "queued" processing in
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
    setFdDirectAsync, arranges for this.    March, 19, 1998		


    This is a modification of the asyncIo.c source. The signal handlers
    and queuing have been eliminated to follow the signal handling conventions
   of a threaded program.  Signal handlers are to be avoided.   Ther perferred
   methods of a single thread (the main) handle all signals via the sigwait()
   call.

   Now asyncMainLoop() use sigwait and handles the signals directly

           4/20/2004 Greg Brissey
*/

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/param.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include "eventHandler.h"
#include "errLogLib.h"

#ifdef SOLARIS
#include <sys/filio.h>
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

static struct _asyncIoEntry	asyncIoEntryArray[ NOFILE ];
static int			maxAsyncIoIndex = 0;
static int			asyncIoSetup = 0;
static fd_set			readFd,writeFd,excptFd;

/*
    Initialize the structure
*/
void
setupAsyncIO()
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

/*
     build a select file descriptor mask of active fd that are async IO
     using SIGIO signal.
*/
fd_set buildActiveFdMask()
{
    int iter,fd;
    fd_set activeFd;
    ASYNC_IO_ENTRY	currentEntry;

    FD_ZERO( &activeFd );
    for (iter = 0; iter < maxAsyncIoIndex; iter++) 
    {
        currentEntry = &asyncIoEntryArray[ iter ];
 
        fd = currentEntry->fd;
        if (fd < 0 || fd >= FD_SETSIZE) 
        {
           continue;                       /* ignore bad entries */
        }
        FD_SET( fd, &activeFd );
    }
    return(activeFd);
}

void
processIO()
{
   int	fd, iter, maxfd, nfound;
   fd_set excptfd, readfd, writefd;
   struct timeval nowait;
   ASYNC_IO_ENTRY currentEntry;

   /* Start by building a mask of possible active file descriptors  */

   FD_ZERO( &readfd );
   FD_ZERO( &writefd );

   excptfd = readfd = buildActiveFdMask();
   maxfd = find_maxfd( &readfd );
   if (maxfd < 0) 
   {
      errLogRet(ErrLogOp,debugInfo, "programming error in find maxfd\n" );
      return;
   }
   else if (maxfd == 0) 
   {
      errLogRet(ErrLogOp,debugInfo, "SIGIO received, but nothing asynchronous was found\n" );
      return;
   }

   /*  You have to tell select() not to wait; with an address of NULL, it
      waits until there is activity on one of the selected file descriptors.  */

   nowait.tv_sec = 0;
   nowait.tv_usec = 0;

   /* who's got input ? */
try_again:
   if ( (nfound = select( maxfd, &readfd, &writefd, &excptfd, &nowait ) ) < 0)
   {
      if (errno == EINTR)
         goto try_again;
      else
         errLogSysRet(ErrLogOp,debugInfo, "select Error:\n" );
   }

   if (nfound < 1) {  /* Nobody */
      /*fprintf( stderr, "SIGIO received, but nothing active was found\n" );*/
      return;
   }

   /*  Using the mask of known active file descriptors, queue up
      non-interrupt level processing for each active file descriptor.  */

   for (iter = 0; iter < maxAsyncIoIndex; iter++) 
   {
      currentEntry = &asyncIoEntryArray[ iter ];

      fd = currentEntry->fd;
      if (fd < 0 || fd >= FD_SETSIZE) 
      {
         continue;			/* ignore bad entries */
      }

      /* Note the ioctl command is only for sockets ... */
      if (FD_ISSET( fd, &readfd )) 
      {
         int ival, zero = 0;

         /* is either the accept SIGIO or the SIGUSR2 for the NDDS Pipe mechanism I'm using */
         if (currentEntry->entryType == DIRECT)   
         {
            (*currentEntry->callback)( currentEntry->clientData );
         }
         else 
         {
#ifdef LINUX
            fcntl(fd, F_SETOWN, (int) getpid());
            ival = fcntl(fd, F_GETFL);
            ival |= O_ASYNC;
            fcntl(fd, F_SETFL, ival);
#else
            ival = ioctl( fd, FIOASYNC, &zero );
#endif
            /* read from the new socket created via accept */
            (*currentEntry->callback)( currentEntry->clientData );   
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


static int
locateUnusedEntry()
{
   int iter, index;

   index = -1;
   for (iter = 0; iter < NOFILE; iter++)
   {
      if (asyncIoEntryArray[ iter ].fd == NOT_OPEN) {
         index = iter;
         break;
      }
   }

  return( index );
}

static int
locateEntryByFd( int fd )
{
   int	index, iter;

   index = -1;
   for (iter = 0; iter < maxAsyncIoIndex; iter++)
   {
      if (asyncIoEntryArray[ iter ].fd == fd) {
         index = iter;
         break;
      }
   }

   return( index );
}

int setFdAsync( int fd, void *clientData, void (*callback)() )
{
   int index, iter, ival;
   ASYNC_IO_ENTRY	currentEntry;

   if (fd < 0 || fd >= FD_SETSIZE) {
      errno = EINVAL;
      return( -1 );
   }
   if (callback == NULL) {
      errno = EINVAL;
      return( -1 );
   }
   else 
   {
      /*  This block of program allows the callback to be changed for
         a file descriptor which is already set to be asynchronous  */
      for (iter = 0; iter < maxAsyncIoIndex; iter++) 
      {
         currentEntry = &asyncIoEntryArray[ iter ];
         if (fd == currentEntry->fd) 
         {
            if (callback == currentEntry->callback &&
                  clientData == currentEntry->clientData) 
            {
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
   int index, iter;
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
   int index, iter, ival;
   ASYNC_IO_ENTRY	currentEntry;

   if (fd < 0 || fd >= FD_SETSIZE) {
      errno = EINVAL;
      return( -1 );
   }
   if (callback == NULL) {
      errno = EINVAL;
      return( -1 );
   }
   else 
   {
      /*  This block of program allows the callback to be changed for
          a file descriptor which is already set to be asynchronous  */
      for (iter = 0; iter < maxAsyncIoIndex; iter++) 
      {
         currentEntry = &asyncIoEntryArray[ iter ];
         if (fd == currentEntry->fd) 
         {
            if (callback == currentEntry->callback &&
                clientData == currentEntry->clientData) 
            {
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
