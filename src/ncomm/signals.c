/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>

#define SIZEOF_ARRAY( array )	( sizeof( array ) / sizeof( array[ 0 ] ) )

static int	signal_stuff_setup = 0;

static struct {
	int			signal;
	struct sigaction	oldaction;
	fd_set			fdmask;
	int			maxfd;
} signal_data[] = {
	{ SIGPIPE },
	{ SIGIO },
};


/*
 *	addMaskForSocket	sockets.c
 *	rmMaskForSocket		sockets.c
 *	is_fdmask_clear		fdmask.c
*/

extern void	addMaskForSocket( void *pSocket, fd_set *fdmaskp );
extern void 	rmMaskForSocket( void *pSocket, fd_set *fdmaskp );
extern int	is_fdmask_clear( fd_set *fdmaskp );

/*  This program asssumes it receives the address of a valid mask of
    signals.  It adds those signals we want blocked in our signal handlers.	*/

static void
maskForHandler( sigset_t *sigset )
/* sigset_t *sigset */
{
	sigaddset( sigset, SIGPIPE );
	sigaddset( sigset, SIGIO );

/*  You could also step through each member of the signal data array...  */

}

static int
find_signal_data( int signal )
/* int signal */
{
	int	iter;

	for (iter = 0; iter < SIZEOF_ARRAY( signal_data ); iter++) {
		if (signal == signal_data[ iter ].signal)
		  return( iter );
	}

	return( -1 );
}

int
registerSignalHandler( int signal, void (*program)(), char *socket )
/* int signal */
/* void (*program)() */
/* char *socket */
{
        int                     ival, retval, sig_index;
	sigset_t		omask, qmask;
        struct sigaction        curaction, newaction;

	sig_index = find_signal_data( signal );
	if (sig_index == -1) {
		return( -1 );
	}

        ival = sigaction( signal, NULL, &curaction );
        if (ival != 0) {
                return( ival );
        }

/*  No signals while we work with signal data... */

	sigemptyset( &qmask );
	sigaddset( &qmask, signal );
	sigprocmask( SIG_BLOCK, &qmask, &omask );

        if (curaction.sa_handler != program) {
		FD_ZERO( &signal_data[ sig_index ].fdmask );
		signal_data[ sig_index ].maxfd = 0;
		addMaskForSocket( socket, &signal_data[ sig_index ].fdmask );
		signal_data[ sig_index ].oldaction = curaction;

        	newaction.sa_flags = 0;
        	newaction.sa_handler = program;
        	newaction.sa_mask = curaction.sa_mask;
		maskForHandler( &newaction.sa_mask );

        	retval = sigaction( signal, &newaction, NULL );
	}
	else {
		addMaskForSocket( socket, &signal_data[ sig_index ].fdmask );
		retval = 0;
	}

/*  Interrupts OK now...  */

	sigprocmask( SIG_SETMASK, &omask, NULL );
        return( retval );
}

int
unregisterSignalHandler( int signal, char *socket )
/* int signal */
/* char *socket */
{
	int		retval, sig_index;
	sigset_t	omask, qmask;

	sig_index = find_signal_data( signal );
	if (sig_index == -1) {
		return( -1 );
	}


/*  No signals while we work with signal data... */

	sigemptyset( &qmask );
	sigaddset( &qmask, signal );
	sigprocmask( SIG_BLOCK, &qmask, &omask );

	rmMaskForSocket( socket, &signal_data[ sig_index ].fdmask );
	if (is_fdmask_clear( &signal_data[ sig_index ].fdmask )) {
        	retval = sigaction( signal, &signal_data[ sig_index ].oldaction, NULL );
	}
	else {
		retval = 0;
	}

/*  Interrupts OK now...  */

	sigprocmask( SIG_SETMASK, &omask, NULL );
	return( retval );
}
