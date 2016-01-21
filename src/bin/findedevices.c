/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 
 */

/*  This program probes for ethernet interfaces.

    It opens the generic device (the main program has a list).  If
    this is successful, it then uses the dlattach program to probe
    for the actual device.  Assuming this is successful, it closes
    the generic device and then immediately reopens it.  This last
    bit of work-a-round prevents the program from reporting one
    additional bogus interface.  See probe_ethernet_interface.		*/

/*  You must be root to run this program; only root
    can access the actual ethernet interfaces.				*/

/*  sigalrm_irpt, strgetmsg, syncmsg_send_recv_nodata and dlattach
    are common with catcheaddr.c					*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/dlpi.h>


#define  MAXWAIT	20

/*  Placeholder (dummy) program.
    The main thread program should be in a System Call if the alarm goes
    off, the System Call should return immediately with a value of -1.
    At this time, we just interpret the failed system call as an error.

    A more sophisticated program would somehow record the timeout
    event (i. e., by setting a global value).  At this time, such
    sophistication does not appear necessary.				*/

static void
sigalrm_irpt()
{
}

void catch_sigalrm()
{
	void			sigalrm_irpt();
	sigset_t		qmask;
	struct sigaction	intserv;
	struct itimerval	timeval;

    /* --- set up signal handler --- */

	sigemptyset( &qmask );
	sigaddset( &qmask, SIGALRM );
	intserv.sa_handler = sigalrm_irpt;
	intserv.sa_mask = qmask;
	intserv.sa_flags = 0;

	sigaction( SIGALRM, &intserv, NULL );
}

/*  This program was taken from dlcommon.c, written by Sun Microsystems.
    Its name (probably) means stream get message.  It calls getmsg using
    the first 4 arguments.  The fifth serves to identify the program that
    called this one.  Besides calling getmsg, it also programs a timeout,
    presumably so if getmsg doesn't return quickly enough, the process
    itself can timeout and continue to execute.				*/

int
strgetmsg(
	int fd,
	struct strbuf *ctlp,
	struct strbuf *datap,
	int *flagsp,
	char *caller
)
{
	int	rc;
	char	errmsg[ 122 ];

	catch_sigalrm();

/* Start timer.  */

	if (alarm(MAXWAIT) < 0) {
		sprintf( &errmsg[ 0 ], "setting the alarm failed in %s", caller );
		perror( &errmsg[ 0 ] );
		return( -1 );
	}

/* Set flags argument and issue getmsg(). */

	*flagsp = 0;
	if ((rc = getmsg(fd, ctlp, datap, flagsp)) < 0) {
		sprintf( &errmsg[ 0 ], "getmsg failed in %s", caller );
		perror( &errmsg[ 0 ] );
		return( -1 );
	}

/*  Stop timer.  */

	if (alarm(0) < 0) {
		sprintf( &errmsg[ 0 ], "setting the alarm failed in %s", caller );
		perror( &errmsg[ 0 ] );
		return( -1 );
	}

/* Check for MOREDATA and/or MORECTL.
   This happens if you did not give getmsg enough space to write out
   its stuff; in other words, it represents a Programmer Error.  It
   should not happen in normal use.

   If you KNOW you will never return data, you don't have to test
   the MOREDATA bit.							*/


	if ((rc & (MORECTL | MOREDATA)) == (MORECTL | MOREDATA)) {
		sprintf(
	   &errmsg[ 0 ], "getmsg had more control and data in %s", caller
		);
		perror( &errmsg[ 0 ] );
		return( -1 );
	}
	if (rc & MORECTL) {
		sprintf( &errmsg[ 0 ], "getmsg had more control in %s", caller );
		perror( &errmsg[ 0 ] );
		return( -1 );
	}
	if (rc & MOREDATA) {
		sprintf( &errmsg[ 0 ], "getmsg had more data in %s", caller );
		perror( &errmsg[ 0 ] );
		return( -1 );
	}

/* Check for at least sizeof (long) control data portion. */

	if (ctlp->len < sizeof (long)) {
		sprintf( &errmsg[ 0 ],
	   "length of control data from getmsg unexpectedly short in %s", caller
		);
		perror( &errmsg[ 0 ] );
		return( -1 );
	}

	return( 0 );
}

/*  synchronous message send and receive with no data
    Although based on the dlpi programs from Sun Microsystems,
    this program was written here.				*/

int
syncmsg_send_recv_nodata(
int fd,
char *sendmsg,
int sendlen,
int *flagp,
char *recvmsg,
int recvlen,
int *retlenp
)
{
	struct	strbuf	ctl;

	ctl.maxlen = 0;
	ctl.len = sendlen;
	ctl.buf = sendmsg;

	if (putmsg(fd, &ctl, (struct strbuf *) NULL, *flagp) < 0) {
		perror( "putmsg" );
		return( -1 );
	}

	ctl.maxlen = recvlen;
	ctl.len = 0;
	ctl.buf = recvmsg;

	if (strgetmsg(fd, &ctl, (struct strbuf*)NULL, flagp, "dlokack") < 0)
	  return( -1 );

	*retlenp = ctl.len;
	return( 0 );
}

static int
dlattach( int fd, u_long ppa )
{
	int		flags, resplen;
	dl_attach_req_t attach_req;
	union {
		int		dl_primitive;
		dl_ok_ack_t	ok_ack;
		dl_error_ack_t	error_ack;
	} response;

	attach_req.dl_primitive = DL_ATTACH_REQ;
	attach_req.dl_ppa = ppa;

	flags = 0;
	if (syncmsg_send_recv_nodata(
		 fd,
		(char *) &attach_req,
		 sizeof( attach_req ),
		&flags,
		(char *) &response,
		 sizeof( response ),
		&resplen
	))
	  return( -1 );

	if (resplen < sizeof (dl_ok_ack_t)) {
		errno = EIO;
		return( -1 );
	}

	if (flags != RS_HIPRI) {
		/*err("dlokack:  DL_OK_ACK was not M_PCPROTO");*/
		errno = EIO;
		return( -1 );
	}

	if (response.dl_primitive != DL_OK_ACK) {
		if (response.error_ack.dl_errno == DL_BADPPA)
		  errno = ENXIO;		/* No Such Device or Address */
		else
		  errno = EIO;			/* Unknown I/O Error */
		return( -1 );
	}

	return( 0 );
}

static int
dldetach(fd)
int     fd;
{
	dl_detach_req_t detach_req;
	struct  strbuf  ctl;
	int     flags, ival;
 
        detach_req.dl_primitive = DL_DETACH_REQ;

	ctl.maxlen = 0;
	ctl.len = sizeof (detach_req);
	ctl.buf = (char *) &detach_req;

	flags = 0;

	ival = putmsg(fd, &ctl, (struct strbuf*) NULL, flags);
	if (ival < 0)
	  return( -1 );
	else
	  return( 0 );
	
}



static int
probe_ethernet_interface( char *generic_device )

/*  char *generic_device;	input  */

{
	int	 fd;
	u_long	 ppa;
	char	 errmsg[ 122 ];

	fd = open(generic_device, O_RDWR);

/*  If the open failed, the generic class of device does not exist.
    The entry can still be present in the file system; the call to
    open nevertheless gives No Such Device or Address.

    The process must have root or superuser access to proceed.	*/

	if (fd < 0) {
		return( -1 );
	}

	for (ppa = 0; ppa <= 4 ; ppa++) {
		if (dlattach( fd, ppa )) {
			close( fd );
			break;
		}

		printf( "%s%ld\n", generic_device, ppa );
		close( fd );

		fd = open(generic_device, O_RDWR);
		if (fd < 0) {
			sprintf( &errmsg[ 0 ], "failed to reopen %s", generic_device );
			perror( &errmsg[ 0 ] );
			return( -1 );
		}
	}

	return( 0 );
}

/* /dev/ce is for 1Gbit card */

void main(int argc, char *argv[])
{
   int	 iter, ival;
   char	*ethernet_interface;
   char	*ethernet_interfaces[] = {
   		"/dev/ce",
   		"/dev/eri",
   		"/dev/hme",
   		"/dev/le",
   		"/dev/ie",
   		 NULL
   };
   
   if (argc > 1)
   {
      iter = 1;
      while (iter < argc)
      {
         ival = probe_ethernet_interface( argv[iter] );
         iter++;
      }
   }
   else
   {
      for (iter = 0;
	     (ethernet_interface = ethernet_interfaces[ iter ]) != NULL;
	     iter++)
      {
         ival = probe_ethernet_interface( ethernet_interface );
      }
   }
   exit(0);
}
