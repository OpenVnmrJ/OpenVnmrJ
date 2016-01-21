/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <signal.h>
#include <string.h>

#include "sockets.h"
#include "errLogLib.h"

#define  NOT_OPEN		-1	/* taken from sockets.c */
#define  MAX_SIMUL_CONNECTIONS	10


static Socket	*pApplSocket;
static Socket	 AcceptSocketArray[ MAX_SIMUL_CONNECTIONS ];

static int PortNum = 0;

static void savePortInfo( int portNum )
{
   PortNum = portNum;
}

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
    Ok major change for 1/6/99, Now the first long word (i.e 4 bytes) is the number of
    bytes to expect in the message.
    Past a certain size we malloc space otherwise just use the constbuf to avoid the
    overhead of malloc and free
    The corresponding file comm.c (vnmr) in libacqcom.a was modified to send this new
    information. (Vnmr. Acqi, Qtune, Ultimate-PSG (Jpsg)) all use this shared library)

    Greg B.
*/
void
processAcceptSocket( Socket *pSocket )
{
        char*			allocbuffer = NULL;
        char*			buffer = NULL;
	char	 		constbuf[ 4096 ];
        long			msgeSize;
	int	 		bcount;
  	sigset_t    savemask;

	registerSocketNonAsync( pSocket );
        blockSignals(&savemask);  /* Block all Signals */
	bcount = readSocketNonblocking( pSocket, (char*) &msgeSize, sizeof( long ) );
        DPRINT2(1,"processAcceptSocket: bcount: %d, msgesize:  %ld\n",bcount,msgeSize);
        if (bcount > 0)
        {
          /* malloc if we need to otherwise used constbuf */
          if (msgeSize >= sizeof( constbuf ) )
          {
            DPRINT(1,"processAcceptSocket: Mallocing buffer.\n");
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
          /* DPRINT1(-1,"bcount: %d of msge\n",bcount); */

          /* everthing read return signal mask back to what it was */
  	  unBlockSignals(&savemask);

	  if (bcount > 0) 
          {
	    buffer[ bcount ] = '\0';
            DPRINT2(1,"processAcceptSocket: size: %d,  msge: '%s'\n",bcount,buffer);

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
            DPRINT(1,"processAcceptSocket: Freeing Malloc buffer.\n");
	    free(allocbuffer);
          }

        }
	else
	{
             errLogSysRet(ErrLogOp,debugInfo,
		"processAcceptSocket: Read of Async Message Length failed: " );
             errLogRet(ErrLogOp,debugInfo,
		"processAcceptSocket: Message Ignored.\n" );
	}
	closeSocket( pSocket );
}

/*  This program may be (and is) called from a SIGIO interrupt program  */

void
processApplSocket( Socket *pSocket )
{
	int	index, iter, ival;

/*  Find an entry in the array of accept sockets that is available  */

	index = -1;
	for (iter = 0; iter < MAX_SIMUL_CONNECTIONS; iter++)
	  if (AcceptSocketArray[ iter ].sd == NOT_OPEN) {
		index = iter;
		break;
	  }

/*  Complain if none found - exceeded design specifications!  */

	if (index == -1) {
	    	DPRINT(-1, "interrupt lost on the application socket\n" );
		return;
	}

/*  Use the version of acceptSocket that can be called from an interrupt  */

	ival = acceptSocket_r( pSocket, &AcceptSocketArray[ index ] );
	if (ival != 0)
	  return;

	ival = registerSocketAsync( &AcceptSocketArray[ index ], processAcceptSocket );
#ifdef LINUX
        kill(getpid(), SIGIO);
#endif
}

int
initApplSocket()
{
	int	iter, ival;
	int	applPort;

	pApplSocket = createSocket( SOCK_STREAM );
	if (pApplSocket == NULL)		/* each call to program in */
	  return( -1 );				   /* sockets.c sets errno */
	ival = openSocket( pApplSocket );
	if (ival != 0)
	  return( -1 );
	ival = registerSocketDirectAsync( pApplSocket, processApplSocket );
	if (ival != 0)
	  return( -1 );
	ival = bindSocketAnyAddr( pApplSocket );
	applPort = returnSocketPort( pApplSocket );
	ival = listenSocket( pApplSocket );
	if (ival != 0)
	  return( -1 );

	savePortInfo( applPort );
	for (iter = 0; iter < MAX_SIMUL_CONNECTIONS; iter++)
	  AcceptSocketArray[ iter ].sd = NOT_OPEN;

	return( 0 );
}
