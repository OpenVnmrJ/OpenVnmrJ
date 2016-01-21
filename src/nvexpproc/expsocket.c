/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <signal.h>

#include "sockets.h"
#include "errLogLib.h"
#if   !defined(LINUX) && !defined(__INTERIX)
#include <thread.h>
#endif
#include <pthread.h>
#include <signal.h>
#include "rngBlkLib.h"


#define  NOT_OPEN		-1	/* taken from sockets.c */
#define  MAX_SIMUL_CONNECTIONS	10

extern pthread_t main_threadId;

static Socket	*pApplSocket;

static pthread_t AcceptThreadId;

static RINGBLK_ID pAcceptQueue;


static int PortNum = 0;

static void savePortInfo( int portNum )
{
   PortNum = portNum;
}

/*
 * acqinfo2 file which is used by other programs to permit contact with Expproc
 *
 */
void delacqinfo2()
{
        char    filepath[256];
        strcpy(filepath, getenv( "vnmrsystem" ));       /* path to acqinfo2 */
        strcat(filepath,"/acqqueue/acqinfo2");
        unlink(filepath);
}

void wrtacqinfo2()
{
	char	LocalAcqHost[256], buf[ 256 ], filepath[256];
	int	bytes, fd;

	gethostname( LocalAcqHost, sizeof( LocalAcqHost ) );

	sprintf( &buf[ 0 ],"%d %s -1 -1 %d",
		  getpid(), LocalAcqHost, PortNum );

	strcpy(filepath, getenv( "vnmrsystem" ));	/* path to acqinfo2 */
	strcat(filepath,"/acqqueue/acqinfo2");
	if ( (fd = open(filepath,O_WRONLY | O_CREAT | O_TRUNC,0666)) == -1) {
		fprintf(stderr,"Could Not Open Acquisition Info File: '%s'\n",
			filepath);
		exit(1);
	}

	bytes = write(fd,buf,strlen(buf)+1);
	if ( (bytes == -1) ) {
		fprintf(stderr,"Could Not Write Acquisition Info File: '%s'\n",
			filepath);
		exit(1);
	}

	close(fd);
}


/*
 * initExpprocSocket()
 *
 * Creates the listen socket for communication to Expproc fro other processes
 *
 * instead of signal handler etc, this routine create a thread to handle the accept
 * thus removing the need for socketfuncs.c and asyncIO.c
 *
 *    Author greg Brissey 3/12/2006
 */
int initExpprocSocket()
{
	int	status, ival;
	int	applPort;
        void *AcceptConnection( void *arg);

	pApplSocket = createSocket( SOCK_STREAM );
	if (pApplSocket == NULL)		/* each call to program in */
	  return( -1 );				   /* sockets.c sets errno */
	ival = openSocket( pApplSocket );
	if (ival != 0)
	  return( -1 );
	ival = bindSocketAnyAddr( pApplSocket );
	if (ival != 0)
        {
	  errLogSysRet(ErrLogOp,debugInfo,"initExpprocSocket: bindSocketAnyAddr failed:" );
	  return( -1 );
         }
	applPort = returnSocketPort( pApplSocket );
	ival = listenSocket( pApplSocket );
	if (ival != 0)
        {
	  errLogSysRet(ErrLogOp,debugInfo,"initExpprocSocket: listenSocket failed:" );
	  return( -1 );
         }


 	 savePortInfo( applPort ); 
         pAcceptQueue = rngBlkCreate(128,"AcceptQ", 1);  /* accepted socket queue */

        /* create thread to handle the accept */
        status = pthread_create (&AcceptThreadId, NULL, AcceptConnection, (void*) pApplSocket);
	return( 0 );

}


/*
 * AcceptConnection() is a posix thread which waits on Accept,
 * when accept returns the newly created accept socket is placed on a queue
 * and a SIGIO signal is sent to the main thread to perform the read and processing
 * the socket message
 *
 *   Author greg Brissey 3/21/2006
 */
void *AcceptConnection( void *arg)
{
    int result;
    Socket  *pAcceptSocket;
    Socket   *pListenSocket;

    pListenSocket = (Socket *) arg;

    pthread_detach(AcceptThreadId);   /* if thread terminates no need to join it to recover resources */
   

    for ( ;; )
    {
        pAcceptSocket = (Socket *) malloc( sizeof( Socket ) );
        if (pAcceptSocket == NULL) {
                return( NULL );
        }
        pListenSocket = (Socket *) arg;

        memset( pAcceptSocket, 0, sizeof( Socket ) );
        result = acceptSocket_r( pListenSocket, pAcceptSocket );
	if (result < 0) {
		errLogSysRet(ErrLogOp,debugInfo,"acceptSocket_r" );
	}
        else
        {
		DPRINT2(+3,"AcceptConnection: 0x%lx, fd: %d\n", pAcceptSocket,pAcceptSocket->sd);
		rngBlkPut(pAcceptQueue, &pAcceptSocket, 1);
                pthread_kill(main_threadId,SIGIO); /* signal console socket msg arrival to main thread */
                /* kill(getpid(), SIGIO); */
        }
   }

}

int processExpSock()
{
   int	nfound;
   fd_set  readfd;
   Socket  *pAcceptSocket;
   void readAcceptSocket( Socket *pSocket );
   int     rngBlkIsEmpty (register RINGBLK_ID ringId);
   int sockInQ;
   /* Start by building a mask of possible active file descriptors  */

   /* while queue is not empty keep reading sockets */
   /* while (rngBlkIsEmpty(pAcceptQueue) != Q_EMPTY) */
   while ((sockInQ = rngBlkNElem(pAcceptQueue)) > 0 )
   {
     
     DPRINT1(1,"Accepted Sockets in Queue: %d\n",sockInQ);
     rngBlkGet(pAcceptQueue, &pAcceptSocket, 1);
     DPRINT2(+3,"processExpSock: 0x%lx, fd: %d\n", pAcceptSocket,pAcceptSocket->sd);

     FD_ZERO( &readfd );
     FD_SET( pAcceptSocket->sd, &readfd);
   
     DPRINT1(+3,"readfd mask: 0x%lx\n",readfd);

   /* who's got input ? */
  try_again:
     if ( (nfound = select( pAcceptSocket->sd+1, &readfd, 0, 0, 0 ) ) < 0)
     {
        if (errno == EINTR)
           goto try_again;
        else
           errLogSysRet(ErrLogOp,debugInfo, "select Error:\n" );
     }
     DPRINT1(+3,"select: readfd mask: 0x%lx\n",readfd);

     if (nfound < 1)  /* Nobody */
     {
        /*fprintf( stderr, "SIGIO received, but nothing active was found\n" );*/
        free(pAcceptSocket);
        return(0);
     }

     if (FD_ISSET(pAcceptSocket->sd, &readfd) )
       readAcceptSocket( pAcceptSocket );
     
     free(pAcceptSocket);
   }

   return 0;
}

/*
 *   Ok major change for 1/6/99, Now the first long word (i.e 4 bytes) is the number of
 *   bytes to expect in the message.
 *   Past a certain size we malloc space otherwise just use the constbuf to avoid the
 *   overhead of malloc and free
 *   The corresponding file comm.c (vnmr) in libacqcom.a was modified to send this new
 *   information. (Vnmr. Acqi, Qtune, Ultimate-PSG (Jpsg)) all use this shared library)
 *
 *    Greg B.
 */
void readAcceptSocket( Socket *pSocket )
{
        char*			allocbuffer = NULL;
        char*			buffer = NULL;
	char	 		constbuf[ 4096 ];
        long			msgeSize;
	int	 		bcount;

	/* registerSocketNonAsync( pSocket ); */
	bcount = readSocketNonblocking( pSocket, (char*) &msgeSize, sizeof( long ) );
        DPRINT2(+2,"processAcceptSocket: bcount: %d, msgesize:  %d\n",bcount,msgeSize);
        if (bcount > 0)
        {
          /* malloc if we need to otherwise used constbuf */
          if (msgeSize >= sizeof( constbuf ) )
          {
            DPRINT(+3,"processAcceptSocket: Mallocing buffer.\n");
            allocbuffer = malloc(msgeSize+1);
	    buffer = allocbuffer;  
          }
          else
          {
	    buffer = constbuf;
          }

	  /* bcount = readSocketNonblocking( pSocket, buffer, bufsize ); */
	  /* Had to block signals because were getting interrupted system call errors */
	  bcount = readSocket( pSocket, buffer, msgeSize );
          DPRINT1(+3,"bcount: %d of msge\n",bcount);

	  if (bcount > 0) 
          {
	    buffer[ bcount ] = '\0';
            DPRINT2(+1,"processAcceptSocket: size: %d,  msge: '%s'\n",bcount,buffer);

	    /* Go Parse the Command and Do It */
	    parser( buffer );

	  }
          else
          {
             errLogSysRet(ErrLogOp,debugInfo,
		"processAcceptSocket: Read of Async Message failed: " );
             errLogRet(ErrLogOp,debugInfo,
		"processAcceptSocket: Message Ignored.\n" );
          }

          /* If a buffer was malloc'ed then it's time to free it */
          if (allocbuffer != NULL)
          {
            DPRINT1(1,"processAcceptSocket: Freeing Malloc buffer (0x%lx).\n",allocbuffer);
	    free(allocbuffer);
          }

        }
	else
	{
             errLogRet(ErrLogOp,debugInfo,
		"processAcceptSocket: No Message,  Ignored.\n" );
	}
	closeSocket( pSocket );
}
