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
04/19/95   Corrected return values when errno is set to EFAULT
09/26/94   Added some more include files so it will compile on
           either SunOS or Solaris
09/01/94   Added openSocket; createSocket just allocates memory
           you must call openSocket to use the socket.  Improved
           the response to error situations; programs set errno
           to a useful value if something goes wrong.
08/26/94   started this history, converted to ANSI C with respect
           to procedure declarations.
*/

/*
DESCRIPTION

This is an internal object which currently only the channel objects
make use of

*/

#ifndef VNMRS_WIN32

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#ifdef FT3DIO
#include <netinet/tcp.h>
#endif

#if ( !defined(LINUX) && !defined(__INTERIX) )
 #include <sys/filio.h>
 #include <sys/sockio.h>
#endif

#include <netdb.h>
#include <arpa/inet.h>

#else   /* VNMRS_WIN32 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <Winsock2.h>
#include "inttypes.h"
#endif /* VNMRS_WIN32 */

#include <signal.h>

#define  NOT_OPEN	-1

#ifdef __INTERIX
typedef int socklen_t;
#endif

#include "errLogLib.h"
#include "sockets.h"

#ifndef VNMRJ
#ifndef NOASYNC
extern int setFdAsync( int fd, void *clientData, void (*callback)() );
extern int setFdNonAsync( int fd );
extern int setFdDirectAsync( int fd, void *clientData, void (*callback)() );
#endif
#endif


/*
    Socket Programming Paradigm

read (server)                      write (client)
---------------------------------------------------
   bind                               connect
   listen
   accept


If the interface is between two different systems, the read side binds
to an address on the local system; the write side connects to an address
on the remote system.

The programs here attempt to avoid enforcing this paradigm, relying on
the application to accomplish this.					*/

/*  Methods to:
        create a socket
        open a socket
        setup a socket
	bind a socket to an address
	bind a socket to the first available address following a base address
	listen for a connection
	accept a connection.
	contribute to a File Descriptor Mask, suitable for the select program
	determine if a socket is active, based on a File Descriptor Mask
        read from a socket
	write to a socket
	close a socket

    Note:  setup a socket provided because when a program accepts
           a connection, a new socket is created.  Therefore, we
           have two ways to make a new socket.  Notice that both
           the create a socket and the accept a connection return
           the address of a new socket object.

    Note:  The SO_REUSEADDR socket option allows a port address to
           be reused when the socket is closed.  However, if this
           option is set for a Client then one must wait 10 to 20
           minutes after closing a socket before connecting again
           to the port address.  In addition, no failures result
           when we do not set this option for a connected socket,
           the one returned from a connect program.  To summarize,
           SO_REUSEADDR appears only required for those sockets
           that are bound to an address, server sockets.

    Note:  Programs are expected to set `errno' in the event of an
           error.  Two common such situations are:
             EFAULT -  program called with NULL socket object.
             EBADF -   operation attempted on socket object
                       without opening the socket.  Call
                       openSocket first.				*/


#ifndef  DEFAULT_SOCKET_BUFFER_SIZE
#define  DEFAULT_SOCKET_BUFFER_SIZE	32768
#endif

/*  Put this method first, so createSocket can use it.  */
/*  Operations to be done on all sockets before their use.

    Tell the system to use loopback.

    Returns 0 unless an error occurs, in which case it returns -1.	*/

static int
setupSocket( Socket *pSocket )
/* Socket *pSocket; */
{
	int	ival, one, pid;
	int	optlen, rcvbuff, sendbuff;

	if (pSocket == NULL) {
		errno = EFAULT;
		return( -1 );
	}
	if (pSocket->sd == NOT_OPEN) {
		errno = EBADF;
		return( -1 );
	}

#ifndef VNMRS_WIN32

	pid = getpid();

   /* ioctlsocket() supports only the SIOCATMARK */
#ifndef __CYGWIN__
	ival = ioctl( pSocket->sd, FIOSETOWN, &pid );
	if (ival < 0) {
		errLogSysRet(ErrLogOp,debugInfo,"set ownership" );
		return( -1 );
	}

	ival = ioctl( pSocket->sd, SIOCSPGRP, &pid );
	if (ival < 0) {
		errLogSysRet(ErrLogOp,debugInfo,"set socket's process group" );
		return( -1 );
	}
#endif
#endif  /* VNMRS_WIN32 */

        one = 1;
#if  !defined( LINUX  )  && !defined( __INTERIX)
	/* though you can set this in Interix is seem to cause problems so don't do it */
        ival = setsockopt( pSocket->sd, SOL_SOCKET, SO_USELOOPBACK,
                (char *) &one, sizeof( one ) );
        if (ival < 0)  {
                errLogSysRet(ErrLogOp,debugInfo,"use loopback" );
                return( ival );
        }
#endif

	optlen = sizeof(sendbuff);
	sendbuff = DEFAULT_SOCKET_BUFFER_SIZE;
	if (setsockopt(pSocket->sd,SOL_SOCKET, SO_SNDBUF, (char *) &(sendbuff), optlen) < 0)
	  errLogSysRet(ErrLogOp,debugInfo,"tcp_socket(), SO_SNDBUF setsockopt error");

	optlen = sizeof(rcvbuff);
	rcvbuff = DEFAULT_SOCKET_BUFFER_SIZE;
	if (setsockopt(pSocket->sd,SOL_SOCKET, SO_RCVBUF, (char *) &(rcvbuff), optlen) < 0)
	  errLogSysRet(ErrLogOp,debugInfo,"tcp_socket(), SO_RCVBUF setsockopt error");

/*  Next two calls may not be needed, but they insure the socket starts
    out blocking and non-asynchronous.  (Latter attribute means no SIGIO
    is delivered when data arrives.)					*/

	setSocketBlocking( pSocket );
	setSocketNonAsync( pSocket );

	return( 0 );
}

/*  returns 0 (NOT a file or socket descriptor) if successful
    returns -1 if error.
      EFAULT -  the socket object is NULL
      other values for errno may be set by socket or the system calls
      of setupSocket.							*/

int
openSocket( Socket *pSocket )
/* Socket *pSocket - Socket object to receive an active socket file descriptor */
{
	if (pSocket == NULL) {
		errno = EFAULT;
		return( -1 );
	}

	pSocket->sd = socket( AF_INET, pSocket->protocol, 0 );
	if (pSocket->sd < 0) {
		errLogSysRet(ErrLogOp,debugInfo,"create socket" );
		return( -1 );
	}

	if (setupSocket( pSocket )) {
		close( pSocket->sd );
		pSocket->sd = NOT_OPEN;
		return( -1 );
	}

	return( 0 );
}

/*  Returns socket object if successful, -1 if failure.
      ENOMEM -  malloc failed, presumably out of memory.	*/

Socket *
createSocket( int type )
/*int type - protocol type, TCP or UDP */
{
	Socket	*tSocket;

	tSocket = (Socket *) malloc( sizeof( Socket ) );
	if (tSocket == NULL) {
		errLogSysRet(ErrLogOp,debugInfo,"memory, socket" );
		errno = ENOMEM;
		return( NULL );
	}

	memset( tSocket, 0, sizeof( Socket ) );
	tSocket->sd = NOT_OPEN;
	tSocket->protocol = type;

	return( tSocket );
}

/*
*   bindSocketSearch will attempt to bind the socket to the
*   base address.  If this fails, it will increment the
*   address until it succeeds in binding the socket.  It
*   will give up after 1000 tries though.
*
*   The return value is 0 (success) or -1 (error).  If successful,
*   the port it is bound to is stored in the Socket object.
*/

int
bindSocketSearch( Socket *pSocket, int baseAddr )
/* Socket *pSocket -  Socket object to be bound to a port address */
/* int baseAddr    -  address to start searching at */
{
	int			iter, ival;
	struct sockaddr_in	socketAddr;

	if (pSocket == NULL) {
		errno = EFAULT;
		return( -1 );
	}
	if (pSocket->sd == NOT_OPEN) {
		errno = EBADF;
		return( -1 );
	}

	memset( &socketAddr, 0, sizeof( socketAddr ) );
	iter = 0;

	while (iter < 1000) {
		socketAddr.sin_family = AF_INET;
#ifndef USE_HTONS
		socketAddr.sin_port = baseAddr + iter;
#else
		socketAddr.sin_port = 0xFFFF & htons(baseAddr + iter);
#endif
		socketAddr.sin_addr.s_addr = INADDR_ANY;
		ival = bind( pSocket->sd, (struct sockaddr *) &socketAddr, sizeof( socketAddr ) );
		if (ival == 0) {
			int	one;

#ifndef USE_HTONS
			pSocket->port = socketAddr.sin_port;
#else
			pSocket->port = 0xFFFF & ntohs(socketAddr.sin_port);
#endif
			one = 1;
        		ival = setsockopt( pSocket->sd, SOL_SOCKET, SO_REUSEADDR,
		                (char *) &one, sizeof( one ) );
			return( 0 );
		}
		else
		  iter++;
	}

	pSocket->port = -1;
	return( -1 );
}

/*
*    bindSocket only tries the one address
*/

int
bindSocket( Socket *pSocket, int baseAddr )
/* Socket *pSocket - Socket object to be bound to a port address. */
/* int baseAddr    - port address to bind the socket to. */
{
	int			ival, retval;
	struct sockaddr_in	socketAddr;

	if (pSocket == NULL) {
		errno = EFAULT;
		return( -1 );
	}
	if (pSocket->sd == NOT_OPEN) {
		errno = EBADF;
		return( -1 );
	}

	memset( &socketAddr, 0, sizeof( socketAddr ) );

	socketAddr.sin_family = AF_INET;
#ifndef USE_HTONS
	socketAddr.sin_port = baseAddr;
#else
	socketAddr.sin_port = 0xFFFF & htons(baseAddr);
#endif
	socketAddr.sin_addr.s_addr = INADDR_ANY;
	ival = bind( pSocket->sd, (struct sockaddr *) &socketAddr, sizeof( socketAddr ) );
	if (ival == 0) {
		int	one;

#ifndef USE_HTONS
		pSocket->port = socketAddr.sin_port;
#else
		pSocket->port = 0xFFFF & ntohs(socketAddr.sin_port); /* host order */
#endif
		one = 1;
        	ival = setsockopt( pSocket->sd, SOL_SOCKET, SO_REUSEADDR,
	                (char *) &one, sizeof( one ) );
		retval = 0;
	}
	else {
		pSocket->port = -1;
		retval = -1;
	}

	return( retval );
}

/*
*    bindSocketAnyAddr uses the INADDR_ANY to bind to the first
*    available address
*/

int
bindSocketAnyAddr( Socket *pSocket )
/* Socket *pSocket - Socket object to be bound to a port address. */
{
	int			ival, retval;
	struct sockaddr_in	socketAddr;

	if (pSocket == NULL) {
		errno = EFAULT;
		return( -1 );
	}
	if (pSocket->sd == NOT_OPEN) {
		errno = EBADF;
		return( -1 );
	}

	memset( &socketAddr, 0, sizeof( socketAddr ) );

	socketAddr.sin_family = AF_INET;
	socketAddr.sin_port = htons(0);
	socketAddr.sin_addr.s_addr = INADDR_ANY;
	ival = bind( pSocket->sd, (struct sockaddr *) &socketAddr, sizeof( socketAddr ) );
	if (ival == 0) 
        {
	    int	one;
            socklen_t namlen;

    	    namlen = sizeof( socketAddr );
    	    getsockname( pSocket->sd, (struct sockaddr *) &socketAddr, &namlen);
#ifndef USE_HTONS
	    pSocket->port = socketAddr.sin_port;
#else
	    pSocket->port = 0xFFFF & ntohs(socketAddr.sin_port); /* port is a short */
#endif
	    one = 1;
            ival = setsockopt( pSocket->sd, SOL_SOCKET, SO_REUSEADDR,
	               (char *) &one, sizeof( one ) );
	    retval = 0;
	}
	else 
        {
	    pSocket->port = -1;
	    retval = -1;
	}

	return( retval );
}

/*  Use acceptSocket and listenSocket with server sockets.  */

/*  Special Note:  acceptSocket assumes the socket has been assigned a
                   Port Address.  To reuse this Port Address, you must
                   close the original socket, the one referenced in the
                   argument to this program.				*/

/*  Special Note:  The program returns a new socket object, the socket
                   created by the accept call.				*/

Socket *
acceptSocket( Socket *pSocket )
/* Socket *pSocket -  starting Socket object. */
{
	int		 newsd;
	socklen_t	 fromlen;
	struct sockaddr	 from;
	Socket		*newSocket;

	if (pSocket == NULL) {
		errno = EFAULT;
		return( NULL );
	}
	if (pSocket->sd == NOT_OPEN) {
		errno = EBADF;
		return( NULL );
	}

/*  The accept program demands an address for its 3rd argument.  */

	fromlen = sizeof( from );
	newsd = accept( pSocket->sd, &from, &fromlen );
	if (newsd < 0) {
		errLogSysRet(ErrLogOp,debugInfo,"socket accept" );
		return( NULL );
	}

	newSocket = (Socket *) malloc( sizeof( Socket ) );
	if (newSocket == NULL) {
		close( newsd );
		return( NULL );
	}

	memset( newSocket, 0, sizeof( Socket ) );
	newSocket->sd = newsd;
	if (setupSocket( newSocket )) {
		close( newsd );
		free( (char *) newSocket );
		return( NULL );
	}

	return( newSocket );
}

/*  acceptSocket_r has the same function as acceptSocket, except
    the latter should not be called from an interrupt program,
    because it will call malloc.  acceptSocket_r accomodates a
    need for an accept socket object function callable from an
    interrupt program.  Of course, the _r version needs a socket
    object preallocated by the program that called it, whose
    address is passed to it as the 2nd argument.  The _r name is
    adopted from the name of UNIX programs (e.g. gethostbyname_r)
    usable in similar situations.        March 19, 1998		*/

int
acceptSocket_r( Socket *pSocket, Socket *pAcceptSocket )
/* Socket *pSocket -  starting Socket object. */
/* Socket *pAcceptSocket -  address of Socket object for accept socket. */
{
	int		 newsd;
	socklen_t	 fromlen;
	struct sockaddr	 from;

	if (pSocket == NULL || pAcceptSocket == NULL) {
		errno = EFAULT;
		return( -1 );
	}
	if (pSocket->sd == NOT_OPEN) {
		errno = EBADF;
		return( -1 );
	}

/*  The accept program demands an address for its 3rd argument.  */

	fromlen = sizeof( from );
	newsd = accept( pSocket->sd, &from, &fromlen );
	if (newsd < 0) {
		errLogSysRet(ErrLogOp,debugInfo,"socket accept" );
		return( -1 );
	}

	memset( pAcceptSocket, 0, sizeof( Socket ) );
	pAcceptSocket->sd = newsd;
	if (setupSocket( pAcceptSocket )) {
		close( newsd );
		return( -1 );
	}

	return( 0 );
}

int
listenSocket( Socket *pSocket )
/* Socket *pSocket -  Socket object which is to accept connections. */
{
	if (pSocket == NULL) {
		errno = EFAULT;
		return( -1 );
	}
	if (pSocket->sd == NOT_OPEN) {
		errno = EBADF;
		return( -1 );
	}

	listen( pSocket->sd, 5 );
	return( 0 );
}

/*
*   Use connectSocket with client sockets.  A socket which
*   does a connect has no port address of its own...
*/

/* 
*  struct sockaddr_in {
*         int16  sin_family;
*         int16  sin_port
*         struct in_addr sin_addr;
*         char   sin_zero[8];
*  }
*      
*  struct in_addr { int32 s_addr };
*/

int
connectSocket( Socket *pSocket, char *hostName, int portAddr )
/* Socket *pSocket -  Socket object which is to attempt a connection. */
/* char *hostName  -  name of (perhaps) remote host */
/* int portAddr    -  port address of remote socket */
{
	struct hostent		*hp;
#if  !defined(__INTERIX) && !defined(MACOS)
	int			 result;
	struct hostent		hpstruct;
        int    hp_errno;
        char   hpIPBuffer[256];
#endif
	struct sockaddr_in	 sin;


	if (pSocket == NULL) {
		errno = EFAULT;
		return( -1 );
	}
	if (pSocket->sd == NOT_OPEN) {
		errno = EBADF;
		return( -1 );
	}

        /* Since this maybe used in threaded programs better use the reentrant version
         * of gethostbyname()
         * was hp = gethostbyname( hostName );
         */
#if  defined(__CYGWIN__) || defined(MACOS)
        hp = gethostbyname(hostName);
	if (hp == NULL) {
	    return( -1 );
	}
#else
        /* this linux variant of gethostbyname_r() is different from others  */
        result = gethostbyname_r(hostName,  &hpstruct, hpIPBuffer, 256, &hp, &hp_errno);
	if (result != 0) {
	    return( -1 );
	}
#endif


#ifdef XXXX    /* for debugging */
*        {
*        char **p;
*        unsigned char *ip;
*        fprintf(stdout,"hp->h_name: '%s'\n",hp->h_name);
*        ip = hp->h_addr;
*        fprintf(stdout,"%d.%d.%d.%d\n",*ip,*(ip+1),*(ip+2),*(ip+3));
*        ip = *(hp->h_addr_list);
*        fprintf(stdout,"%d.%d.%d.%d\n",*ip,*(ip+1),*(ip+2),*(ip+3));
*        fprintf(stdout,"\n---------------\n");
*        for (p=hp->h_addr_list; *p != 0; p++) 
*        {
*            ip =*p;
*            printf("%d.%d.%d.%d\n",*ip,*(ip+1),*(ip+2),*(ip+3));
*        }  
*        fflush(stdout);
*        }
#endif

	memset( (char *) &sin, 0, sizeof( struct sockaddr_in ) );
	sin.sin_family = hp->h_addrtype;
#ifndef USE_HTONS
	sin.sin_port = portAddr;
#else
	sin.sin_port = htons(portAddr);
#endif
        memcpy(&sin.sin_addr,*hp->h_addr_list, sizeof(struct in_addr));

/*
 *      result = connect( pSocket->sd, (struct sockaddr *) &sin, sizeof( struct sockaddr_in ) );
 *
 *      Above is the old method. However, connect() can be (and is) interrupted by signals.
 *      In this case, the connect call is "backgrounded" and the result is -1. In the case of
 *      deliverMessage, if -1 is returned, the message is not sent. The connect eventually
 *      succeeds and a select() call in Vnmrbg will say something is available. However,
 *      the message was never sent. There are two ways to handle interrupted connect() calls.
 *      The easy one supposedly works on Linux while the tedious one works everywhere.
 *      The easy one is used, but the tedious one is provided in case we ever need it.
 */
        
        while ( (connect(pSocket->sd, (struct sockaddr *) &sin, sizeof(struct sockaddr_in)) == -1)
                 && (errno != EISCONN) )
        {
           if (errno != EINTR)
              return(-1);
        }
        
#ifdef TEDIOUS
        if ( connect (fd, &name, namelen) == -1 )
        {
           struct pollfd junk;
           int some_more_junk;
           socklen_t yet_more_useless_junk;

           if ( errno != EINTR /* && errno != EINPROGRESS */ )
           {
              return(-1)
           }
           junk.fd = fd;
           junk.events = POLLOUT;
           while ( poll (&junk, 1, -1) == -1 )
              if ( errno != EINTR )
              {
                 return(-1);
              }
           yet_more_useless_junk = sizeof(some_more_junk);
           if ( getsockopt (fd, SOL_SOCKET, SO_ERROR,
                     &some_more_junk,
                     &yet_more_useless_junk) == -1 )
           {
                return(-1);
           }
           if ( some_more_junk != 0 )
           {
                return(-1);
           }
        }
#endif
  
	return( 0 );
}

void
addMaskForSocket( Socket *pSocket, fd_set *fdmaskp )
/* Socket *pSocket */
/* fd_set *fdmaskp */
{
	if (pSocket == NULL) {
		errno = EFAULT;
		return;
	}
	if (pSocket->sd == NOT_OPEN) {
		errno = EBADF;
		return;
	}

	FD_SET( pSocket->sd, fdmaskp );
}

void
rmMaskForSocket( Socket *pSocket, fd_set *fdmaskp )
/* Socket *pSocket */
/* fd_set *fdmaskp */
{
	if (pSocket == NULL) {
		errno = EFAULT;
		return;
	}
	if (pSocket->sd == NOT_OPEN) {
		errno = EBADF;
		return;
	}

	FD_CLR( pSocket->sd, fdmaskp );
}

/*  Predicate:  returns 0 if false, 1 if true  */

int
isSocketActive( Socket *pSocket, fd_set *fdmaskp )
/* Socket *pSocket */
/* fd_set *fdmaskp */
{
	if (pSocket == NULL) {
		errno = EFAULT;
		return( 0 );
	}
	if (pSocket->sd == NOT_OPEN) {
		errno = EBADF;
		return( 0 );
	}

	return(FD_ISSET( pSocket->sd, fdmaskp ));
}

/*  You really should only call this method for server sockets... */

int
returnSocketPort( Socket *pSocket )
/* Socket *pSocket */
{
	if (pSocket == NULL) {
		errno = EFAULT;
		return( -1 );
	}

	return( pSocket->port );
}

int
setSocketNonblocking( Socket *pSocket )
{
	int	flags;

	if (pSocket == NULL) {
		errno = EFAULT;
		return( -1 );
	}
	if (pSocket->sd == NOT_OPEN) {
		errno = EBADF;
		return( -1 );
	}
  /* win32 - The WSAAsyncSelect and WSAEventSelect functions automatically set a socket to nonblocking mode. */
#ifndef VNMRS_WIN32
	if ((flags = fcntl( pSocket->sd, F_GETFL, 0 )) == -1){	/* get mode bits */
		errLogSysRet(ErrLogOp,debugInfo,"set nonblocking, can't get current setting" );
		return(-1);
	}    
	flags |=  FNDELAY;			   /* set to nonblocking */
	if (fcntl( pSocket->sd, F_SETFL, flags ) == -1) {
		errLogSysRet(ErrLogOp,debugInfo,"set nonblocking, can't change current setting" );
		return(-1);
	}
#else  /* WIN32 native */
	flags = 1;
    if (ioctlsocket(pSocket->sd, FIONBIO, &flags) != 0) 
	{
		errLogSysRet(ErrLogOp,debugInfo,"set nonblocking, can't change current setting" );
		return(-1);
	}
#endif  /* VNMRS_WIN32 */
	return( 0 );
}

int
setSocketBlocking( Socket *pSocket )
{
	int	flags;

	if (pSocket == NULL) {
		errno = EFAULT;
		return( -1 );
	}
	if (pSocket->sd == NOT_OPEN) {
		errno = EBADF;
		return( -1 );
	}

#ifndef VNMRS_WIN32
	if ((flags = fcntl( pSocket->sd, F_GETFL, 0 )) == -1){	/* get mode bits */
		errLogSysRet(ErrLogOp,debugInfo,"set blocking, can't get current setting" );
		return(-1);
	}    
	flags &= ~FNDELAY;			   /* set to blocking */
	if (fcntl( pSocket->sd, F_SETFL, flags ) == -1) {
		errLogSysRet(ErrLogOp,debugInfo,"set blocking, can't change current setting" );
		return(-1);
	}
#else   /* WIN32 native */
	flags = 0;
    if (ioctlsocket(pSocket->sd, FIONBIO, &flags) != 0) 
	{
		errLogSysRet(ErrLogOp,debugInfo,"set nonblocking, can't change current setting" );
		return(-1);
	}
#endif  /* VNMRS_WIN32 */

	return( 0 );
}

int
setSocketAsync( Socket *pSocket )
{
	int ival;
#if ( !defined(LINUX) || defined(__CYGWIN__) )
        int one;
#endif

	if (pSocket == NULL) {
		errno = EFAULT;
		return( -1 );
	}
	if (pSocket->sd == NOT_OPEN) {
		errno = EBADF;
		return( -1 );
	}

#if defined(LINUX) && !defined(__CYGWIN__) || defined(__INTERIX)
        fcntl(pSocket->sd, F_SETOWN, (int) getpid());
        ival = fcntl(pSocket->sd, F_GETFL);
        ival |= O_ASYNC;
        fcntl(pSocket->sd, F_SETFL, ival);
#else  /* Solaris & Cygwin */
	one = 1;
	ival = ioctl( pSocket->sd, FIOASYNC, &one );
	if (ival < 0)  {
		errLogSysRet(ErrLogOp,debugInfo,"set asynchronous" );
		return( -1 );
	}
#endif

	return( 0 );
}

int
setSocketNonAsync( Socket *pSocket )
{
	int	ival, zero;

	if (pSocket == NULL) {
		errno = EFAULT;
		return( -1 );
	}
	if (pSocket->sd == NOT_OPEN) {
		errno = EBADF;
		return( -1 );
	}

	zero = 0;
#if  defined(LINUX) || defined(__INTERIX)
        fcntl(pSocket->sd, F_SETOWN, (int) getpid());
        ival = fcntl(pSocket->sd, F_GETFL);
#ifdef MACOS
        ival |= O_FSYNC;
#else
        ival |= O_SYNC;
#endif
        fcntl(pSocket->sd, F_SETFL, ival);
#else
	ival = ioctl( pSocket->sd, FIOASYNC, &zero );
#endif
	if (ival < 0)  {
		errLogSysRet(ErrLogOp,debugInfo,"set synchronous" );
		return( -1 );
	}

	return( 0 );
}

int
readSocket( Socket *pSocket, char *datap, int bcount )
/* Socket *pSocket */
/* char *datap */
/* int bcount */
{
	int	nleft, nbytes;

	if (pSocket == NULL) {
		errno = EFAULT;
		return( -1 );
	}
	if (pSocket->sd == NOT_OPEN) {
		errno = EBADF;
		return( -1 );
	}

/*  readSocket is written like this because for some reason you do
    not always get the count of chars from read, even if the socket
    descriptor is set to block the process.  So we keep reading until
    we get all the chars we asked or an error occurs.

    Note that when readSocket returnd, EOF may have occurred.  This
    is equivalent to the peer process (the one writing to the socket)
    closing its side of the socket.  At this time the socket object
    is not aware itself that EOF has occurred.  If it occurs after
    a partial transfer, readSocket may return a count less than the
    count it was called with.  Each subsequent call to read though
    will return 0.  So the next time readSocket gets called it will
    return 0, thus notifying the application of the EOF event.		*/

	nleft = bcount;
	while (nleft > 0) {
		nbytes = read( pSocket->sd, datap, nleft );
		if (nbytes < 0) {
#ifdef XXX
			if (errno == EINTR)
		        {
			  errLogSysRet(ErrLogOp,debugInfo,"readSocket: Interrupted, read %d bytes Restart read\n",nbytes);
			  /* perror( "readSocket: " ); */
			  continue;
		        }
#endif
			errLogSysRet(ErrLogOp,debugInfo, "readSocket" );
			return( nbytes );
		}
		else if (nbytes == 0)
		  break;

		nleft -= nbytes;
		datap += nbytes;
	}

	return( bcount - nleft );
}


int
readProtectedSocket( Socket *pSocket, char *datap, int bcount )
/* Socket *pSocket */
/* char *datap */
/* int bcount */
{
        int     nleft, nbytes;
#ifndef VNMRS_WIN32
   sigset_t    blockmask, savemask;
#endif

        if (pSocket == NULL) {
                errno = EFAULT;
                return( -1 );
        }
        if (pSocket->sd == NOT_OPEN) {
                errno = EBADF;
                return( -1 );
        }

/*  readSocket is written like this because for some reason you do
    not always get the count of chars from read, even if the socket
    descriptor is set to block the process.  So we keep reading until
    we get all the chars we asked or an error occurs.

    Note that when readSocket returnd, EOF may have occurred.  This
    is equivalent to the peer process (the one writing to the socket)
    closing its side of the socket.  At this time the socket object
    is not aware itself that EOF has occurred.  If it occurs after
    a partial transfer, readSocket may return a count less than the
    count it was called with.  Each subsequent call to read though
    will return 0.  So the next time readSocket gets called it will
    return 0, thus notifying the application of the EOF event.          */
#ifndef VNMRS_WIN32
   sigemptyset( &blockmask );
   sigaddset( &blockmask, SIGALRM );
   sigprocmask( SIG_BLOCK, &blockmask, &savemask );
#endif
        nleft = bcount;
        while (nleft > 0) {
                nbytes = read( pSocket->sd, datap, nleft );
                if (nbytes < 0) {
#ifdef XXX
                        if (errno == EINTR)
                        {
                          errLogSysRet(ErrLogOp,debugInfo,"readSocket: Interrupted, read %d bytes Restart read\n",nbytes);
                          /* perror( "readSocket: " ); */
                          continue;
                        }
#endif
                        errLogSysRet(ErrLogOp,debugInfo, "readSocket" );
#ifndef VNMRS_WIN32
			sigprocmask( SIG_SETMASK, &savemask, NULL );
#endif
                        return( nbytes );
                }
                else if (nbytes == 0)
                  break;

                nleft -= nbytes;
                datap += nbytes;
        }

#ifndef VNMRS_WIN32
        sigprocmask( SIG_SETMASK, &savemask, NULL );
#endif
        return( bcount - nleft );
}




/*  Next program is expected to be used in asynchronous applications.
    It works much like readSocket, except we use the FIONREAD option
    with ioctl to ascertain if any data are present.  If no data are
    are present, the program sets errno to EWOULDBLOCK and returns
    -1.  Notice that we don't actually set the socket to non-blocking.
    If data is present, then we keep reading data until either the
    count of data is received or an error occurs.			*/

int
readSocketNonblocking( Socket *pSocket, char *datap, int bcount )
/* Socket *pSocket */
/* char *datap */
/* int bcount */
{
	int	nbytes, nleft, rcount;

	if (pSocket == NULL) {
		errno = EFAULT;
		return( -1 );
	}
	if (pSocket->sd == NOT_OPEN) {
		errno = EBADF;
		return( -1 );
	}

	if (ioctl( pSocket->sd, FIONREAD, &nbytes ) != 0) {
		return( -1 );
	}
	/* errLogRet(ErrLogOp,debugInfo,"readSocketNonblocking: nbytes: %d\n",nbytes); */
	if (nbytes < 1) {
#ifndef VNMRS_WIN32
		errno = EWOULDBLOCK;
#endif
		return( -1 );
	}

/*  At least one character of data is available.  Block the process
    until the rest of the chars arrive (or EOF occurs, or an error).	*/

	rcount = 0;

/*  Rest is adopted from readSocket - we do not use readSocket
    directly as here we want to continue in the event of an
    interrupt (nbytes == -1 and errno == EINTR)			*/

	nleft = bcount;
	while (nleft > 0) {
		nbytes = read( pSocket->sd, datap, nleft );
		if (nbytes < 0) {
			if (errno == EINTR)
			  continue;
			errLogSysRet(ErrLogOp,debugInfo,"readSocket" );
			return( nbytes );
		}
		else if (nbytes == 0)
		  break;

		nleft -= nbytes;
		datap += nbytes;
		rcount += nbytes;
	}

	return( rcount );
}

int
writeSocket( Socket *pSocket, const char *datap, int bcount )
/* Socket *pSocket */
/* char *datap */
/* int bcount */
{
        ssize_t written = 0;
        ssize_t nwrite;
	if (pSocket == NULL) {
		errno = EFAULT;
		return( -1 );
	}
	if (pSocket->sd == NOT_OPEN) {
		errno = EBADF;
		return( -1 );
	}
        while (written < bcount)
        {
           nwrite = write( pSocket->sd, datap+written, bcount - written );
           if (nwrite < 0)
           {
              if (errno != EINTR)
                 return(-1);
           }
           else
           {
              written += nwrite;
           }
              
        }

//	return( write( pSocket->sd, datap, bcount ) );
        return(bcount);
}

int
closeSocket( Socket *pSocket )
/* Socket *pSocket */
{
	if (pSocket == NULL) {
		errno = EFAULT;
		return( -1 );
	}
	if (pSocket->sd == NOT_OPEN) {
		errno = EBADF;
		return( -1 );
	}

	shutdown( pSocket->sd, 2 );
	close( pSocket->sd );
	pSocket->sd = NOT_OPEN;

	return( 0 );
}


/* read any data in socket and throw it away until no data is present */

int flushSocket( Socket *pSocket)
/* Socket *pSocket */
{
   char buf[2048],*bufptr;
   int	nbytes, nleft, rcount;
   int notempty;

   if (pSocket == NULL) {
	errno = EFAULT;
	return( -1 );
   }
   if (pSocket->sd == NOT_OPEN) {
	errno = EBADF;
	return( -1 );
   }

   notempty = 1;
   rcount = 0;
   while( notempty )
   {
	if (ioctl( pSocket->sd, FIONREAD, &nbytes ) != 0) {
		return( -1 );
	}
	if (nbytes < 1) {
	   notempty = 0;
	}
        else
	{

	  if (nbytes > 2048)
            nleft = 2048;
          else
	    nleft = nbytes;

	  bufptr = buf;
	  while (nleft > 0) 
          {
		nbytes = read( pSocket->sd, bufptr, nleft );
		if (nbytes < 0) {
			if (errno == EINTR)
			  continue;
			errLogSysRet(ErrLogOp,debugInfo,"readSocket" );
			return( nbytes + rcount );
		}
		else if (nbytes == 0)
		  break;

		nleft -= nbytes;
		bufptr += nbytes;
		rcount += nbytes;
	 }

       }
   }
   return( rcount );
}

#ifndef VNMRJ
#ifndef NOASYNC

int
registerSocketAsync( Socket *pSocket, void (*callback)() )
{
	int	ival;

	if (pSocket == NULL) {
		errno = EFAULT;
		return( -1 );
	}
	if (pSocket->sd == NOT_OPEN) {
		errno = EBADF;
		return( -1 );
	}

	ival = setFdAsync( pSocket->sd, pSocket, callback );
	setSocketAsync( pSocket );

	return( ival );
}

int
registerSocketNonAsync( Socket *pSocket )
{
	int	ival;

	if (pSocket == NULL) {
		errno = EFAULT;
		return( -1 );
	}
	if (pSocket->sd == NOT_OPEN) {
		errno = EBADF;
		return( -1 );
	}

	ival = setFdNonAsync( pSocket->sd );

	return( ival );
}

/*  See asyncIo.c.  At this time there is no method to turn this off  */

int
registerSocketDirectAsync( Socket *pSocket, void (*callback)() )
{
	int	ival;

	if (pSocket == NULL) {
		errno = EFAULT;
		return( -1 );
	}
	if (pSocket->sd == NOT_OPEN) {
		errno = EBADF;
		return( -1 );
	}

	ival = setFdDirectAsync( pSocket->sd, pSocket, callback );
	if (ival != 0)
	  return( ival );

	ival = setSocketAsync( pSocket );

	return( ival );
}
#endif
#endif

#ifdef FT3DIO

Socket sVnmrNet;
static int createVSocket(int type, Socket *tSocket )
/*int type - protocol type, TCP or UDP */
{

     memset( tSocket, 0, sizeof( Socket ) );
     tSocket->sd = -1;
     tSocket->protocol = type;
     return(0);
}


/* Copied from smagic */
void net_write(char *netAddr, char *netPort, char *message)
{
     int  bNew, port, k, len;
     char *d;
     char addr[128];

     if (netAddr == NULL || netPort == NULL || message == NULL)
        return;
     createVSocket( SOCK_STREAM, &sVnmrNet );
     d = netAddr;
     while (*d == ' ') d++;
     strcpy(addr, d);
     d = netPort;
     while (*d == ' ') d++;
     port = atoi(d);
     if (sVnmrNet.sd > 0)
        closeSocket(&sVnmrNet);
     if (openSocket(&sVnmrNet) == -1)
        return;
     setsockopt(sVnmrNet.sd,SOL_SOCKET,(~SO_LINGER),(char *)&k,sizeof(k));
     setsockopt(sVnmrNet.sd,SOL_SOCKET,TCP_NODELAY,(char *)&k,sizeof(k));
     bNew = 0;
     while (bNew < 8) {
         k = connectSocket(&sVnmrNet,addr,port);
         if (k == 0)
             break;
         if (bNew > 6)
             return;
         bNew++;
         sleep(1);
     }
     if (sVnmrNet.sd < 0)
         return;
     len = strlen(message);
     k =  writeSocket( &sVnmrNet, message, len);
     closeSocket(&sVnmrNet);
}


#endif
