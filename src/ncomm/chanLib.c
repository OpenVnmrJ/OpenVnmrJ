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
08/16/94  improved error reporting, added description of interface
          and what each interface can return
08/24/94  improved interface to the free library call
08/25/94  converted to the ANSI C standard with respect to procedure declarations
08/26/94  mangen now generates a sensible .nr file
09/01/94  connectChannel may be called more than once
09/26/94  First round on asynchronous channels
09/27/94  reworked for mangen program -  no change in function
04/20/95  fixed problem when closing and then reopening an asynchronous channel
*/

/*
DESCRIPTION

Host Computer Channel Programs

The underlying paradigm of the channel programs is the Host Computer is the
client and the Console is the server.  The host computer calls connect; the
console programs call listen and accept.  Recall the connect call always
returns immediately while the accept call normally blocks the calling program
until a remote partner calls connect.

When the host is ready to complete a channel connection, it contacts the
Connection Broker on the console.  The Broker maintains a list of which
channels in the console are ready to accept a remote connection.  We expect
the console programs to start first, activating their side of the channels 
first.  Then the host computer programs start, complete the connection and
each channel is ready for use.

Suppose however the host computer starts first.  Two things can happen.  If
the Connection Broker is not ready, then the initial attempt to contact it
will fail.  See contact_remote_cb.  Not much can be done here; the host has
to try again.  The other possibility is the connection broker is ready, but
the channel itself is not.  In this situation the connectChannel call fails
with `errno' set to ESRCH.  The application then knows to either quit or
(more likely) wait a set amount of time and try again.

INTERNAL

Initially I adopted the following strategy:  The host computer then sends a
message saying, in effect, ready when you are.  Then for this channel the
paradigm reverses.  The host becomes the server and the console becomes the
client.  When the console side of the channel is ready, it sends a message
to this effect to the host computer.  The host then knows it can complete
the connection.

Unfortunately this takes a lot of program to implement.  Furthermore those in
the project who write applications using the channel programs seem of the
opinion that it is not required.  An alternate is sufficent, as follows:
If the console is not ready, the connectChannel call fails with `errno' set
to ESRCH.  The application then knows to either quit or (more likely) wait a
set amount of time and try again.

I wrote some programs to implement the first strategy.  At this time they are
not being used.  Each is being kept in the event we decide to proceed with
this strategy, but it not being compiled thru the use of a compiler switch.


Compile-time switches:

Set NOASYNC to exclude asynchronous channel programs.  You get these programs
by default, but they require the asynchronous event system, eventHandler.c,
eventQueue.c and listObj.c, to link successfully.

Set DEBUG_ASYNC to get useful information about asynchronous channels.
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* local include files */
/* Since this source file is the implementation, include the private
   include file, with its description of the hidden channel object.	*/

#include "errLogLib.h"
#include "chanLibP.h"

#define  ERROR_CHANNEL	 -1

/*  Stolen from the Krikkit Connection Broker !!  */
/*  These defines must be identical to the ones in the Connection Broker.  */

#define  WANT_TO_SEND		1
#define  PARSE_ERROR		2
#define  CHANNEL_ERROR		3
#define  MESSAGE_ERROR		4
#define  READY_TO_RECEIVE	5
#define  NOT_READY		6
#define  NOT_SETUP		7
#define  REMOTE_SYSTEM_DOWN	8
#define  PROGRAMMING_ERROR	9
#define  CLIENT_READY           10
#define  CHANNEL_IN_USE		11


/*  Stolen from the Krikkit Connection Broker !!  */
/*  This structure should be identical to the one in the Connection Broker.  */

typedef struct _connBrokerMessage {
	int	channel;
	int	message;
	int	console_read_address;
	int	host_read_address;
} connBrokerMessage;


static Channel	chanTable[ NUM_CHANS ];


#if 0
/*  It upsets the compiler when this static function is never called.  */
static void
sigpipe_prog()
{

/*  Eventually this program needs to get the read mask for SIGPIPE, call select
    with no waiting, look for active socket(s) in the returned masks and (perhaps)
    set errno to Connection Reset by Peer.					*/

}
#endif


static char	*remote_host = NULL;

static void
get_remote_host()
{
	char	*tmpaddr;

	tmpaddr = (char *) getenv( "NESSIE_CONSOLE" );
	if (tmpaddr == NULL) {
		errLogRet(ErrLogOp,debugInfo, "Error: NESSIE_CONSOLE environment parameter not defined\n" );
		errLogRet(ErrLogOp,debugInfo, "Cannot continue\n" );
		errLogRet(ErrLogOp,debugInfo, "Define this parameter and restart the program\n" );
		exit( 1 );
	}
	else
	  remote_host = tmpaddr;
}

/*  Stolen from the Krikkit Connection Broker !!  */
/*  This program should be identical to the one in the Connection Broker.  */

static int
parseRequest( char *textRequest, connBrokerMessage *pcbr )
{
	int	value;

	pcbr->channel = -1;
	pcbr->message = -1;
	pcbr->console_read_address = -1;
	pcbr->host_read_address = -1;
	if (textRequest == NULL)
	  return( -1 );
	if ((int)strlen(textRequest) < 1)
	  return( -1 );

/*  First character must be a digit...  */

	if ( *textRequest < '0' || '9' < *textRequest ) {
		return( -1 );
	}

/*  Channel number must be in the range  */

	value = atoi( textRequest );
	if (value < 0 || value >= NUM_CHANS) {
		return( -1 );
	}
	pcbr->channel = value;

/*  Scan past the first number in the text.  */

	while ( '0' <= *textRequest && *textRequest <= '9')
	  textRequest++;
	if (*textRequest != ',') {
		return( -1 );
	}
	textRequest++;		/* scan past the comma */

/*  Second field must start with a digit...  */

	if ( *textRequest < '0' || '9' < *textRequest ) {
		return( -1 );
	}

	value = atoi( textRequest );
	pcbr->message = value;

/*  Scan past the second number in the text.  */

	while ( '0' <= *textRequest && *textRequest <= '9')
	  textRequest++;

/*  May be only two fields in the message...  */

	if ((int)strlen( textRequest ) < 1) {
		return( 0 );
	}

/*  ... but field separator must be a comma...  */

	if (*textRequest != ',') {
		return( -1 );
	}
	textRequest++;

/*  ... and third field must start with a digit.  */

	if ( *textRequest < '0' || '9' < *textRequest ) {
		return( -1 );
	}

	value = atoi( textRequest );
	pcbr->console_read_address = value;

/*  Scan past the third number in the text.  */

	while ( '0' <= *textRequest && *textRequest <= '9')
	  textRequest++;

/*  May be only three fields in the message...  */

	if ((int)strlen( textRequest ) < 1) {
		return( 0 );
	}

/*  ... but field separator must be a comma...  */

	if (*textRequest != ',') {
		return( -1 );
	}
	textRequest++;

/*  ... and fourth field must start with a digit.  */

	if ( *textRequest < '0' || '9' < *textRequest ) {
		return( -1 );
	}
	value = atoi( textRequest );

	pcbr->host_read_address = value;
	return( 0 );
}

/*  puts -1 in pCbReply->message if error in local programming.	*/
/*  open and close the Connection Broker Socket (pccb) right here */

static void
contact_remote_cb( Socket *pccb, connBrokerMessage *pCbRequest, connBrokerMessage *pCbReply )
{
	char	 msg_for_cb[ 30 ], cb_text_reply[ 120 ];
	int	 ival;

	if (remote_host == NULL)
	  get_remote_host();

	ival = openSocket( pccb );
	if (ival != 0) {
		/*fprintf( stderr, "cannot open the connection broker socket\n" );*/
		pCbReply->message = CHANNEL_ERROR;
		return;
	}

	ival = connectSocket( pccb, remote_host, CB_PORT );
	if (ival != 0) {
		closeSocket( pccb );
		pCbReply->message = REMOTE_SYSTEM_DOWN;
		return;
	}

	sprintf( &msg_for_cb[ 0 ], "%d,%d",
		  pCbRequest->channel, pCbRequest->message
	);
	ival = write( pccb->sd, &msg_for_cb[ 0 ], (strlen( &msg_for_cb[ 0 ] )+1) );
	if (ival < 1) {
		closeSocket( pccb );
		pCbReply->message = -1;  /* is this a local programming error? */
		return;
	}

	ival = read( pccb->sd, &cb_text_reply[ 0 ], sizeof( cb_text_reply ) - 1 );
	closeSocket( pccb );

	ival= parseRequest( &cb_text_reply[ 0 ], pCbReply );
#ifdef DEBUG_ASYNC
	errLogRet(ErrLogOp,debugInfo, "Connection broker: channel: %d, message: %d\n",
		 pCbReply->channel, pCbReply->message );
 	errLogRet(ErrLogOp,debugInfo, "console read address: %d, host read address: %d\n",
		 pCbReply->console_read_address, pCbReply->host_read_address );
#endif
	if (pCbReply->channel != pCbRequest->channel) {
		pCbReply->message = PROGRAMMING_ERROR;
		return;
	}

	return;
}

/*  Returns -1 if error in local programming.	*/

static int
initial_remote_contact( int channel, int *p_read_addr, int *p_write_addr )
{
	Channel			*pThisChan;
	connBrokerMessage	 cbRequest, cbReply;

	pThisChan = &chanTable[ channel ];

	cbRequest.channel = channel;
	cbRequest.message = WANT_TO_SEND;

	contact_remote_cb( pThisChan->pClientCB, &cbRequest, &cbReply );

	if (cbReply.message == READY_TO_RECEIVE) {
		*p_read_addr = cbReply.host_read_address;
		*p_write_addr = cbReply.console_read_address;
	}
		
	return( cbReply.message );
}

/*  The next program helped implement the when you are ready paradigm.  */

#if 0
static int
client_is_ready( channel, port, p_read_addr, p_write_addr )
int channel;
int port;
int *p_read_addr;
int *p_write_addr;
{
	connBrokerMessage	 cbRequest, cbReply;

	cbRequest.channel = channel;
	cbRequest.message = CLIENT_READY;
	cbRequest.host_read_address = port;

	contact_remote_cb( &cbRequest, &cbReply );

	if (cbReply.message == READY_TO_RECEIVE) {
		*p_read_addr = cbReply.host_read_address;
		*p_write_addr = cbReply.console_read_address;
	}

	return( cbReply.message );
}
#endif

/*  End of connection broker stuff.  */


#if 0
static Socket *
openServerSocket( type )
int type;
{
	int	 ival;
	Socket	*tSocket;

	tSocket = createSocket( type );
	if (tSocket == NULL) {
	    errLogRet(ErrLogOp,debugInfo, "Cannot create local socket\n" );
		return( NULL );
	}

	ival = bindSocketSearch( tSocket, CB_PORT );
	if (ival != 0) {
                closeSocket( tSocket );
		free( tSocket );
                return( NULL );
        }

	listenSocket( tSocket );

	return( tSocket );
}
#endif

#if 0
/*  This program is obsolete.  It combines the function of createSocket and
    connectSocket.  The connectSocket function cannot be done until connectChannel
    is called.  We want to make connectChannel interrupt-safe, which means
    connectChannel is not allowed to call createSocket, since the latter calls
    malloc.  For this reason openClientSocket is no longer used.		*/

static Socket *
openClientSocket( type, remote_addr )
int type;
int remote_addr;
{
	int	 ival;
	Socket	*tSocket;

	if (remote_host == NULL)
	  get_remote_host();

	tSocket = createSocket( type );
	if (tSocket == NULL) {
	    errLogRet(ErrLogOp,debugInfo, "Cannot create local socket\n" );
		return( NULL );
	}

	ival = connectSocket( tSocket, remote_host, remote_addr );
	if (ival != 0) {
	    errLogRet(ErrLogOp,debugInfo, "Cannot connect to remote socket\n" );
	    perror( "Connect socket" );
		closeSocket( tSocket );
		free( tSocket );
		return( NULL );
	}
	else {
		tSocket->port = remote_addr;
	}

	return( tSocket );
}
#endif

/*  The next program helped implement the when you are ready paradigm.  */

#if 0
static int
respond_to_not_ready( channel, p_addr )
int channel;
int *p_addr;
{
	int	 cb_message, local_addr;
	Socket	*tSocket;
	Channel *pThisChan;

	pThisChan = &chanTable[ channel ];

/*  No checks... this is a static function and it's assumed
    the calling program knows what it is doing...		*/

	tSocket = openServerSocket( SOCK_STREAM );
	if (tSocket == NULL) {
	    errLogRet(ErrLogOp,debugInfo, "Cannot create local socket\n" );
		pThisChan->socketInUse = 0;
		return( -1 );
	}

	local_addr = returnSocketPort( tSocket );
	cb_message = client_is_ready( channel, local_addr, p_addr );
	errLogRet(ErrLogOp,debugInfo, "client is ready returned %d\n", cb_message );
	switch (cb_message) {
	  case READY_TO_RECEIVE:
		tSocket = openClientSocket( SOCK_STREAM, *p_read_addr );
		if (tSocket == NULL) {
		    errLogRet(ErrLogOp,debugInfo, "Cannot create local socket\n" );
			pThisChan->socketInUse = 0;
			return( -1 );
		}
		else {
			pThisChan->state = CONNECTED;
			pThisChan->pClientReadS = tSocket;
		}
		break;

	  default:
		errLogRet(ErrLogOp,debugInfo, "client is ready returned %d\n", cb_message );
		return( -1 );
	}

	return( 0 );
}
#endif

/**************************************************************
*
*  openChannel - setup local data for a Channel Object on the host computer.
*
*   returns:  channel number if successful
*             -1 if not successful, with `errno' set
*                EINVAL    channel number out-of-range
*                EBUSY     channel already in use
*                other values may be set by the socket library
*
*  For connectChannel to complete successfully, a process (or
*  task) on the console must have opened the same channel.  It
*  does not have to call listenChannel for connectChannel to
*  complete successfully.
*/

int openChannel( int channel, int access, int options )
/* int channel - select channel to use */
/* int access  - would be read, read/write; not used */
/* int options - not used currently */
{
	Channel	*pThisChan;

	if (channel < 0 || channel >= NUM_CHANS) {
		errno = EINVAL;
		return( -1 );
	}

	pThisChan = &chanTable[ channel ];
	if (pThisChan->socketInUse) {
		errno = EBUSY;
		return( -1 );
	}

/*  We expect createSocket to set errno if it fails...  */

	if ( (pThisChan->pClientCB = createSocket( SOCK_STREAM )) == NULL) {
		return( -1 );
	}
	if ( (pThisChan->pClientReadS = createSocket( SOCK_STREAM )) == NULL) {
		return( -1 );
	}
	else if (openSocket( pThisChan->pClientReadS ) != 0) {
		return( -1 );
	}

	if ( (pThisChan->pClientWriteS = createSocket( SOCK_STREAM )) == NULL) {
		return( -1 );
	}
	else if (openSocket( pThisChan->pClientWriteS ) != 0) {
		return( -1 );
	}

	pThisChan->socketInUse = 131071;
	pThisChan->state = INITIAL;

	return( channel );
}

/**************************************************************
*
*   connectChannel - make connection with corresponding process on the console
*
*   returns:  0 if successful
*             -1 if not successful, with `errno' set
*                EINVAL    channel number out-of-range
*                ESRCH     no console process prsent for
*                          this channel
*                ENXIO     most likely this results from no
*                          connection broker present on the
*                          console.
*                EBUSY     console channel is already
*                          connected to another process
*                other values may be set by the socket library
*
*   connectChannel must be called and complete successfully
*   before calling readChannel or writeChannel
*
*   if connectChannel fails with errno set to ENXIO or ESRCH,
*   you may call it again until the connection is successful.
*/

int connectChannel( int channel )
/* int channel - channel to connect */
{
	int	 cb_message, ival, remote_read_addr, remote_write_addr;
	Channel	*pThisChan;

	if (channel < 0 || channel >= NUM_CHANS) {
		errno = EINVAL;
		return( -1 );
	}

	pThisChan = &chanTable[ channel ];

	cb_message = initial_remote_contact(
		 channel,
		&remote_read_addr,
		&remote_write_addr
	);
	switch (cb_message) {
	  case READY_TO_RECEIVE:
		ival = connectSocket( pThisChan->pClientReadS,
				      remote_host,
				      remote_read_addr
		);
		if (ival != 0) {
			perror( "Cannot connect to remote read socket" );
			return( -1 );
		}

		ival = connectSocket( pThisChan->pClientWriteS,
				      remote_host,
				      remote_write_addr
		);
		if (ival != 0) {
			perror( "Cannot connect to remote write socket" );
			return( -1 );
		}
		else {
			pThisChan->state = CONNECTED;
		}

		break;

	  case NOT_READY:
		/*printf( "found remote connection broker but no remote server\n" );*/
		errno = ESRCH;
		return( -1 );

	  case CHANNEL_IN_USE:
		errLogRet(ErrLogOp,debugInfo, 
		   "channel %d on the console is already connected to another process\n",
		    channel );
		errno = EBUSY;
		return( -1 );

	  default:
		errno = ENXIO;
		return( -1 );
	}

	return( 0 );
}

/**************************************************************
*
*   readChannel -  read data from the console
*
*   returns:  byte count (>0) if successful
*             0 if the remote partner has closed its
*                connection
*             -1 if not successful, with `errno' set
*                EINVAL    channel number out-of-range
*                other values may be set by the socket library
*
*/

int readChannel( int channel, char *datap, int bcount )
/* int channel -  channel to be used */
/* char *datap -  address to receive data */
/* int bcount  -  amount of expected data in characters */
{
	Channel	*pThisChan;

	if (channel < 0 || channel >= NUM_CHANS) {
		errno = EINVAL;
		return( -1 );
	}

	pThisChan = &chanTable[ channel ];

	return( readSocket( pThisChan->pClientReadS, datap, bcount ) );
}

/**************************************************************
*
*   writeChannel -  write data to the console
*
*   returns:  byte count (>0) if successful
*             -1 if not successful, with `errno' set
*                EINVAL    channel number out-of-range
*                other values may be set by the socket library
*/

int writeChannel( int channel, char *datap, int bcount )
/* int channel */
/* char *datap */
/* int bcount */
{
	int	 retval;
	Channel	*pThisChan;

	if (channel < 0 || channel >= NUM_CHANS) {
		errno = EINVAL;
		return( -1 );
	}

	pThisChan = &chanTable[ channel ];

	/* question whether register/unregister is needed... */
	/*registerSignalHandler( SIGPIPE, sigpipe_prog, pThisChan->pClientS );*/
	retval = writeSocket( pThisChan->pClientWriteS, datap, bcount );
	/*unregisterSignalHandler( SIGPIPE, pThisChan->pClientS );*/

	return( retval );
}

/**************************************************************
*
*   flushChannel -  discard any readable data in channel 
*
*   returns:  byte count (>0) of discard data if successful
*             0 if the remote partner has closed its
*                connection
*             -1 if not successful, with `errno' set
*                EINVAL    channel number out-of-range
*                other values may be set by the socket library
*
*/

int flushChannel( int channel)
/* int channel -  channel to be used */
{
	Channel	*pThisChan;

	if (channel < 0 || channel >= NUM_CHANS) {
		errno = EINVAL;
		return( -1 );
	}

	pThisChan = &chanTable[ channel ];

	return( flushSocket( pThisChan->pClientReadS) );
}

#ifndef NOASYNC
#include "listObj.h"
#include "eventHandler.h"

extern int setFdAsync( int fd, void *clientData, void (*callback)() );
extern int setFdNonAsync( int fd );

typedef struct _asyncChanEntry {
	int	  channel;
	void	(*callback)();
#ifdef DEBUG_ASYNC
	int	  count;
#endif
} *ASYNC_CHAN_ENTRY;

#ifdef DEBUG_ASYNC
static LIST_OBJ asyncChanList = NULL;
#endif


#if 0
/*  Given a file descriptor, is it part of a channel?  This routine
    returns the channel number if yes, or -1 if not.  SIGIOs can only
    get directly the identity of the File Descriptor that caused the
    interrupt.  At this time only the client read socket will need to
    be mapped to its channel.

    Currently the program relies on a Socket method (isSocketActive)
    to assist in mapping from file descriptor to channel number.	*/

static int
fd_to_channel( int fd )
{
	int	iter;

	for (iter = 0; iter < NUM_CHANS; iter++)
	  if (chanTable[ iter ].pClientReadS->sd == fd)
	    return( iter );

	return( -1 );
}
#endif


/**************************************************************
*
*   readChannelNonblocking
*
*   This program functions much like readChannel, except
*   it will not block your task if no data is pending when
*   it is called.  If some of your data is pending, but not
*   all, it will block your task until it receives all of
*   the data you requested.
*
*   returns:  count of data bytes received (>0) if successful
*             0  if remote partner has closed its side of the
*                channel
*             -1 if not successful, with `errno' set
*                EWOULDBLOCK  no data pending for this channel
*                EINVAL       channel number out-of-range
*                other values may be set by the socket library
*/

int readChannelNonblocking( int channel, char *datap, int bcount )
/* int channel -  channel to be used */
/* char *datap -  address to receive data */
/* int bcount  -  amount of expected data in characters */
{
	Channel	*pThisChan;

	if (channel < 0 || channel >= NUM_CHANS) {
		errno = EINVAL;
		return( -1 );
	}

	pThisChan = &chanTable[ channel ];

	return( readSocketNonblocking( pThisChan->pClientReadS, datap, bcount ) );
}


/**************************************************************
*
*   registerChannelAsync
*
*   Use this program when you want a channel to work
*   asynchronously.  You must provide a "callback",
*   a program which will be called when data arrives
*   on this channel.  You must also call asyncMainLoop
*   before your callback program can be called.  The
*   callback will receive as its argument the channel
*   which has received data.  The callback program
*   must read the data.
*
*   returns:  0 if successful
*             -1 if not successful, with `errno' set
*                EINVAL       channel number out-of-range
*                             or no callback specified
*                EBADF        connectChannel must complete
*                             successfully before this
*                             program can be called
*
*/

int registerChannelAsync( int channel, void (*callback)() )
/* int channel -        channel to be used */
/* void (*callback)() - routine to be called when data is ready on the channel */
{
	int channelFd, ival;

	if (channel < 0 || channel >= NUM_CHANS) {
		errno = EINVAL;
		return( -1 );
	}
	if (callback == NULL) {
		errno = EINVAL;
		return( -1 );
	}
	if (chanTable[ channel ].state != CONNECTED) {
		errno = EBADF;
		return( -1 );
	}

	channelFd = chanTable[ channel ].pClientReadS->sd;
	ival = setFdAsync( channelFd, (void *) channel, callback );
	setSocketAsync( chanTable[ channel ].pClientReadS );

	return( ival );
}

/**************************************************************
*
*   registerChannelNonAsync
*/

int registerChannelNonAsync( int channel )
/* int channel -        channel to be used */
{
	int	channelFd, ival;

	if (channel < 0 || channel >= NUM_CHANS) {
		errno = EINVAL;
		return( -1 );
	}
	if (chanTable[ channel ].state != CONNECTED) {
		errno = EBADF;
		return( -1 );
	}
	if (chanTable[ channel ].pClientReadS == NULL)
	  return( 0 );

	channelFd = chanTable[ channel ].pClientReadS->sd;
	ival = setFdNonAsync( channelFd );
	return( ival );
}

#ifdef DEBUG_ASYNC
void
printAsyncChanStats( FILE *filep )
{
	int			iter;
	ASYNC_CHAN_ENTRY	currentEntry;

	if (filep == NULL)
	  filep = stdout;

	for (iter = 0; ; iter++) {
		currentEntry = getItem( asyncChanList, iter );
		if (currentEntry == NULL)
		  break;

		fprintf( filep, "asynchronous channel %d received %d events\n",
				 currentEntry->channel, currentEntry->count
		);
	}
}
#endif /* DEBUG_ASYNC */
#endif /* not NOASYNC */


/**************************************************************
*
*   closeChannel - close I/O devices, stop use of the channel
*
*   returns:  0 if successful
*             -1 if not successful, with `errno' set
*                EINVAL    channel number out-of-range
*                other values may be set by the socket library
*/

int closeChannel( int channel )
/* int channel - channel to close */
{
	Channel			*pThisChan;

	if (channel < 0 || channel >= NUM_CHANS) {
		errno = EINVAL;
		return( -1 );
	}

#ifndef NOASYNC
	registerChannelNonAsync( channel );
#endif

	pThisChan = &chanTable[ channel ];

	closeSocket( pThisChan->pClientReadS );
	closeSocket( pThisChan->pClientWriteS );

/*  Note:  The Client-of-Connection-Broker socket (pClientCB) was closed in
           contact_remote_cb, where it is passed as the first argument.  The
           socket object is free'd here, since contact_remote_cb is to be
           interrupt safe (callable from an interrupt) and cannot call free.	*/

	if( pThisChan->pClientReadS != NULL )
	  free( pThisChan->pClientReadS );
	if( pThisChan->pClientWriteS != NULL )
	  free( pThisChan->pClientWriteS );
	if( pThisChan->pClientCB != NULL )
	  free( pThisChan->pClientCB );
	pThisChan->pClientReadS = NULL;
	pThisChan->pClientWriteS = NULL;
	pThisChan->pClientCB = NULL;
	pThisChan->socketInUse = 0;

	return( 0 );
}
