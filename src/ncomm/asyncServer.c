/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*  Demonstration of an asynchronous program using software developed for
    the NDC project.  Requires eventHandler.c, eventQueue.c and listObj.c  */
 
#include <stdio.h>
#include <errno.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>

#include "listObj.h"
#include "eventQueue.h"
#include "eventHandler.h"

#define  NOT_OPEN  -1


static LIST_OBJ		pRegisterAsync = NULL;


static int
setup_a_socket( int sd )
{
	int	ival, one, pid, retval;

	retval = 0;
	pid = getpid();

	ival = ioctl( sd, FIOSETOWN, &pid );
	if (ival < 0) {
		perror( "set ownership" );
		retval = -1;
	}

	ival = ioctl( sd, SIOCSPGRP, &pid );
	if (ival < 0) {
		perror( "set socket's process group" );
		retval = -1;
	}

	one = 1;
	ival = ioctl( sd, FIOASYNC, &one );
	if (ival < 0)  {
		perror( "set nonblocking/asynchronous" );
		retval = -1;
	}

	return( retval );
}

static int
set_nonblocking( int fd )
{
	int	flags, ival;

	if ((flags = fcntl( fd, F_GETFL, 0 )) == -1){	/* get mode bits */
		perror("set nonblocking, can't get current setting" );
		return(-1);
	}    
	flags |=  FNDELAY;			   /* set to nonblocking */
	if (fcntl( fd, F_SETFL, flags ) == -1) {
		perror("set nonblocking, can't change current setting" );
		return(-1);
	}
}

static int
start_sigio( int fd )
{
	int	ival, one;

	one = 1;
	ival = ioctl( fd, FIOASYNC, &one );
	if (ival < 0)  {
		perror( "set asynchronous" );
		return( -1 );
	}

	return( 0 );
}

static int
stop_sigio( int fd )
{
	int	ival, zero;

	zero = 0;
	ival = ioctl( fd, FIOASYNC, &zero );
	if (ival < 0)  {
		perror( "set synchronous" );
		return( -1 );
	}

	return( 0 );
}


typedef struct async_io_entry {
	int	fd;
	void	(*program)();
} *AIOE;

/*  compare entries based on the File Descriptor  */

static int cmp_fd( AIOE stored, AIOE current )
{
	if (stored->fd == current->fd)
	  return( 0 );
	else
	  return( 1 );
}

/*  compare entries based on the complete entry  */

static int cmp_async_io_entry( AIOE stored, AIOE current )
{
	if (stored->fd == current->fd && stored->program == current->program)
	  return( 0 );
	else
	  return( 1 );
}

/*  callback program  */

static void
do_recv_from_client( int sd )
{
	int	nchars;
	char	buffer[ 120 ];

	do {
		nchars = read( sd, &buffer[ 0 ], 120 );
		if (nchars == 0) {
			printf( "socket close detected\n" );
			close( sd );
			exit();
		}
		else if (nchars < 1) {
			if (errno == EWOULDBLOCK) {
				break;
			}
			perror( "socket read error" );
			close( sd );
			exit();
		}
		buffer[ nchars ] = '\000';
		/*printf( "Characters received:\n!%s!\n", &buffer[ 0 ] );*/
		printf( "%s", &buffer[ 0 ] );
	}
	 while (1);

	start_sigio( sd );
}

/*  callback program  */

static void
do_accept( int sd )
{
	int			ival, new_sd, pid_2, pgrp;
	int			fromlen;
	struct sockaddr_in	from;

	fromlen = sizeof( from );
	new_sd = accept( sd, &from, &fromlen );
	setup_a_socket( new_sd );

/* Set the connected socket to non-blocking.  This is separate from
   setting it to asynchronous (raise SIGIO when data arrives).  When
   data arrives and the interrupt program is called, it will keep
   reading until EWOULDBLOCK is set.  See do_recv_from_client.		*/

	set_nonblocking( new_sd );

	ival = ioctl( new_sd, FIOGETOWN, &pid_2 );
	ival = ioctl( new_sd, SIOCGPGRP, &pgrp );

	registerAsyncIO( new_sd, do_recv_from_client );
}


/*  Interrupt handler (helper program)  */

static void
make_fd_rmask( int *maxfd_ptr, fd_set *readfdp )
{
	int	iter;
	AIOE	thisEntry;

	*maxfd_ptr = 0;
	FD_ZERO( readfdp );

	for (iter = 0; ; iter++) {
		thisEntry = (AIOE) getItem( pRegisterAsync, iter );
		if (thisEntry == NULL)
		  break;
		FD_SET( thisEntry->fd, readfdp );
		if (thisEntry->fd > *maxfd_ptr)
		  *maxfd_ptr = thisEntry->fd;
	}
}

/*  Interrupt handler (helper program)  */

static void
respond_to_active_fds( int signal, fd_set *activefdp )
{
	int		iter;
	AIOE		asyncEntry;

	for (iter = 0; ; iter++) {
		asyncEntry = (AIOE) getItem( pRegisterAsync, iter );
		if (asyncEntry == NULL)
		  break;

		if (FD_ISSET( asyncEntry->fd, activefdp )) {
			stop_sigio( asyncEntry->fd );
			processNonInterrupt( signal, (void *) asyncEntry->fd );
		}
	}
}

/*  Interrupt handler */

static void
sigio_irpt( int signal )
{
	int			maxfd, nfound;
	fd_set			excptfd, readfd, writefd;
	struct timeval		nowait;

/*  You have to tell select() not to wait; with an address of NULL, it
    waits until there is activity on one of the selected file descriptors.  */

	nowait.tv_sec = 0;
	nowait.tv_usec = 0;

	make_fd_rmask( &maxfd, &readfd );
	maxfd++;
	nfound = select( maxfd, &readfd, &writefd, &excptfd, &nowait );
	respond_to_active_fds( signal, &readfd );
}

static int
make_a_socket()
{
	int	ival, one, pid, pid_2, pgrp, tsd;

	tsd = socket( AF_INET, SOCK_STREAM, 0 );
	if (tsd < 0)
	  return( tsd );

	pid = getpid();
	ival = ioctl( tsd, FIOSETOWN, &pid );
	if (ival < 0) {
		perror( "set ownership" );
		close( tsd );
		return( ival );
	}

	one = 1;
	ival = setsockopt( tsd, SOL_SOCKET, SO_USELOOPBACK,
		&one, sizeof( one ) );
	if (ival < 0)  {
		perror( "use loopback" );
		close( tsd );
		return( ival );
	}

	setup_a_socket( tsd );

	ival = ioctl( tsd, FIOGETOWN, &pid_2 );
	ival = ioctl( tsd, SIOCGPGRP, &pgrp );

	return( tsd );
}

/*  callback dispatch program  */

static void
processIOEntry( int fd )
{
	int	sequence;
	AIOE	searcher, found;

	searcher = (AIOE) malloc( sizeof( struct async_io_entry ) );
	searcher->fd = fd;
	searcher->program = NULL;

	sequence = searchItem( pRegisterAsync, searcher, cmp_fd );
	if (sequence < 0) {
		fprintf( stderr, "no entry found for f/d %d\n", fd );
		return;
	}
	found = (AIOE) getItem( pRegisterAsync, sequence );
	if (found == NULL || found->program == NULL) {
		fprintf( stderr, "no program for f/d %d\n", fd );
		return;
	}

	(*((found)->program))( fd );
	return;
}


int
registerAsyncIO( int fd, void (*program)() )
{
	AIOE		thisEntry;

	if (pRegisterAsync == NULL) {
		registerAsyncHandlers(
			SIGIO,
			sigio_irpt,
			processIOEntry
		);
	}

	thisEntry = (AIOE) malloc( sizeof( struct async_io_entry ) );
	if (thisEntry == NULL)
	  return( -1 );

	thisEntry->fd = fd;
	thisEntry->program = program;

/*  If there is no entry with this file descriptor, then add thisEntry to the list.
    If there is an entry, test if the associated programs are identical.  If not,
    delete the old entry and append the new entry.  (No action required if the old
    entry references the same program as the one referenced in the argument list.)

    You can just use the free program directly as the destructor.  See deleteEntry.	*/

	if (searchItem( pRegisterAsync, thisEntry, cmp_fd ) < 0) {
		pRegisterAsync = appendItem( pRegisterAsync, thisEntry );
	}
	else {
		if (searchItem( pRegisterAsync, thisEntry, cmp_async_io_entry ) >= 0)
		  return( 0 );

		pRegisterAsync = deleteItem( pRegisterAsync, thisEntry, cmp_fd, free );
		pRegisterAsync = appendItem( pRegisterAsync, thisEntry );
	}
}

main()
{
	int			bind_result, iter, ival, unix_sd;
	sigset_t		qmask, qmask_2;
	struct sockaddr_in	sin;

	setupForAsync( 20 );

	unix_sd = make_a_socket();
	if (unix_sd < 0)
	  exit();

/*  Give the socket a "name" by assigning it an Internet port value.
    The value below was selected at random.  The BIND routine associates
    the socket with the port address, as specified in the data structure
    SIN.								*/

	for (iter = 0; iter < 100; iter++) {
		sin.sin_port = IPPORT_RESERVED + iter;
		sin.sin_addr.s_addr = INADDR_ANY;
		bind_result = bind( unix_sd, &sin, sizeof( sin ) );
		printf( "bind returned %d\n", bind_result );
		if (bind_result == 0) {
			break;
		}
	}

	if (bind_result != 0) {
		perror( "bind error" );
		exit();
	}

	registerAsyncIO( unix_sd, do_accept );

/*  Now make the socket active with the LISTEN call  */

	listen( unix_sd, 5 );

	printf( "socket port number: %d\n", sin.sin_port );
	printf( "process ID: %d\n", getpid() );
	printf( "socket descriptor: %d\n", unix_sd );

	asyncMainLoop();
}
