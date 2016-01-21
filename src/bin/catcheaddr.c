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

/*  This program was developed from limnetd_solaris.c, SCCS category
    limnet_solaris.  Programs not needed here were removed, including
    programs to get the current host name and the ethernet address.
    register_limnet became register_rarp and make_limnet_interface
    became make_rarp_interface.  The list of default interfaces was
    expanded to include the second ethernet interface (le1, ie1).

    Its function is to bind to the specified ethernet interface and
    read the next RARP packet.  (You should always specify the ethernet
    interface rather than relying on the default interface table.)  It
    then writes out the source address from the ethernet header on
    standard output.  All other messages are to be written to standard
    error, for the standard output is to be captured using the
    backquotes construct of the shell language.

    An interval alarm is used to print a message every few seconds
    to reassure the operator the Sun system is still alive.

    SDL-- added check to catch only ethernet-wide broadcasts


*/

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/dlpi.h>
#include <sys/ethernet.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

/*
 * Bootstrap Protocol (BOOTP).  RFC951 and RFC1048.
 *
 * $Header: /net/amazon/amazon2/wise_rep/VxWorks/unsupported/bootp2.1/bootp.h,v
1.1.2.1 1993/06/07 16:52:19 wise active $
 *
 *
 * This file specifies the "implementation-independent" BOOTP protocol
 * information which is common to both client and server.
 *
 *
 * Copyright 1988 by Carnegie Mellon.
 *
 * Permission to use, copy, modify, and distribute this program for any
 * purpose and without fee is hereby granted, provided that this copyright
 * and permission notice appear on all copies and supporting documentation,
 * the name of Carnegie Mellon not be used in advertising or publicity
 * pertaining to distribution of the program without specific prior
 * permission, and notice be given in supporting documentation that copying
 * and distribution is by permission of Carnegie Mellon and Stanford
 * University.  Carnegie Mellon makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 */


struct bootp {
        unsigned char   bp_op;          /* packet opcode type */
        unsigned char   bp_htype;       /* hardware addr type */
        unsigned char   bp_hlen;        /* hardware addr length */
        unsigned char   bp_hops;        /* gateway hops */
        unsigned long   bp_xid;         /* transaction ID */
        unsigned short  bp_secs;        /* seconds since boot began */
        unsigned short  bp_unused;
        struct in_addr  bp_ciaddr;      /* client IP address */
        struct in_addr  bp_yiaddr;      /* 'your' IP address */
        struct in_addr  bp_siaddr;      /* server IP address */
        struct in_addr  bp_giaddr;      /* gateway IP address */
        unsigned char   bp_chaddr[16];  /* client hardware address */
        unsigned char   bp_sname[64];   /* server host name */
        unsigned char   bp_file[128];   /* boot file name */
        unsigned char   bp_vend[64];    /* vendor-specific area */
};



#define	G2000	1
#define	INOVA	2

int	catch_type;

char *default_interface[] = {
	"/dev/le1",
	"/dev/le0",
	"/dev/ie1",
	"/dev/ie0",
	 NULL
};


void
print_ether_addr( char *e_addr )
{
	int	iter;

	for (iter = 0; iter < sizeof( struct ether_addr ) - 1; iter++)
	   if (catch_type == G2000)
	      printf( "%x:", (unsigned char) (*(e_addr++)) );
	   else
	      printf( "%02x", (unsigned char) (*(e_addr++)) );
	
	printf( "%02x", (unsigned char) (*e_addr) );
}

/*  This program isolates the extraction of the EtherNet
    interface from the argument list from the rest of the
    program.  If the EtherNet interface is not specified
    in the argument list, this program returns NULL.

    At this time the rule is very simple:  the 1st arg
    is the name of the EtherNet interface.		*/

char *
args_to_ethernet( int argc, char *argv[] )

/* The arguments to this program are the same as those
   defined for the main program in the C lnaguage.	*/

{
	if (argc < 2)
	  return( NULL );
	else
	  if (argc < 3)
	  {  catch_type=G2000;
	     return( argv[ 1 ] );
	  }
          else
	  {  if ( ! strcmp( argv[ 2 ], "G2000") )
	        catch_type=G2000;
	     else
		catch_type=INOVA;
	     return( argv[ 1] );
	  }
}

#define DEVICE_PREFIX	"/dev/"

int
massage_device_name( char *old_device, char **new_device )

/*  char *old_device;	input  */
/*  char **new_device;	output */

/*  Please note the location addressed by new_device
    receives an address allocated from the heap.	*/

/*  The program is written to allow new_device
    to be the address of old_device.		*/

/*  Program massages the name of the (EtherNet) device.
    If it finds the name lacks /dev/, it prepends this 
    to the device name.  It assumes the string with the
    device name has enough space (current length + 5)
    to receive the prepended /dev/.

    Its purpose is to allow the limNET program to be
    started up with the interface specified as le0 or
    /dev/le0.

    This program should work with any UNIX device name,
    tty, sd, etc.					*/
    
{
	char	*tptr, *tptr_2;
	int	 devlen, iter, offset;

	devlen = strlen( old_device );
	if (strncmp( old_device, DEVICE_PREFIX, strlen( DEVICE_PREFIX ) ) == 0) {
		tptr_2 = malloc( devlen+1 );
		if (tptr_2 == NULL)
		  return( -1 );
		strcpy( tptr_2, old_device );
		*new_device = tptr_2;
		return( 0 );
	}
	else {
		offset = strlen( DEVICE_PREFIX );

		tptr_2 = malloc( devlen + offset + 1 );
		if (tptr_2 == NULL)
		  return( -1 );
		tptr = tptr_2 + devlen + offset;

/*  The terminating null is copied on the 1st cycle through the 1st loop  */

		for (iter = devlen; iter >= 0; iter--)
		  *(tptr--) = old_device[ iter ];
		for (iter = strlen( DEVICE_PREFIX ) - 1; iter >= 0; iter--)
		  *tptr-- = DEVICE_PREFIX[ iter ];

		*new_device = tptr_2;
		return( 0 );
	}
}

int
parse_interface( char *input, char *device, u_long *ppaptr )

/*  char *input;	input only */
/*  char *device;	output only */
/*  u_long *ppaptr;	output only */

/*  This program parses an old-fashioned EtherNet device name and
    extracts the new-fangled device and Physical Point of Attachment
    (PPA).  (Don't ask me why a simple unit number had to be
    transformed into a Physical Point of Attachment, nor why the
    kernel EtherNet programs for Solaris/SunOS 5.0 can't accept an
    old-fashioned device name with unit number.)

    The input must be in the form exemplified by /dev/ie0.  The
    program assumes the Device is all the characters upto the first
    digit.  The PPA is then the output from atoi with atoi given the
    address of the first digit for its argument.

    Notice the program works equally well with /dev/ie0 and ie0.

    This program assumes the device has as many writeable characters
    as are present in the input string.					*/

{
	while (isdigit( *input ) == 0 && *input)
	  *(device++) = *(input++);

	*device = '\0';
	if (*input == '\0')
	  *ppaptr = 0;
	else
	  *ppaptr = (u_long) atoi( input );

	return( 0 );
}

#define  MAXWAIT	20

/*  Placeholder (dummy) program.
    The main thread program should be in a System Call if the alarm goes
    off, the System Call should return immediately with a value of -1.
    At this time, we just interpret the failed system call as an error.

    A more sophisiticated program would somehow record the timeout
    event (i. e., by setting a global value.  At this time, such
    sophisitication does not appear necessary.

    Program is reused later when we await a RARP packet.  Program times
    out every few seconds, asking if the G+ console is connected.	*/

static void
sigalrm_irpt()
{
}

catch_sigalrm()
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

int
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

/*  The register protocol type program should receive the protocol type as an argument.  */

#define  BINDRESPLEN  ( (sizeof( dl_bind_ack_t ) + \
			 sizeof( struct ether_addr ) + sizeof( short ) + \
			 sizeof( long ) - 1) / sizeof( long ))

int
register_rarp( int fd )
{
	int		 flags, ival, resplen;
	long		 response[ BINDRESPLEN ];
	dl_bind_req_t	 bind_req;
	dl_bind_ack_t	*bind_ackp;
	struct strioctl	 sioc;

	bind_req.dl_primitive = DL_BIND_REQ;
	if (catch_type == G2000)
	   bind_req.dl_sap = (u_long) ETHERTYPE_REVARP;
	else
	   bind_req.dl_sap = (u_long) ETHERTYPE_IP;
	bind_req.dl_max_conind = 0;	/* max # of outstanding con_ind */
	bind_req.dl_service_mode = DL_CLDLS;
	bind_req.dl_conn_mgmt = 0;	/* if non-zero, is con-mgmt stream */
	bind_req.dl_xidtest_flg = 0;	/* auto init. of test and xid */

	flags = 0;
	if (syncmsg_send_recv_nodata(
		 fd,
		(char *) &bind_req,
		 sizeof( bind_req ),
		&flags,
		(char *) &response,
		 sizeof( response ),
		&resplen
	))
	  return( -1 );

	/*printf( "bind request returned response length of %d\n", resplen );*/

	if (resplen < sizeof(dl_bind_ack_t)) {
		return( -1 );
	}

	if (flags != RS_HIPRI) {
		/*err("dlbind:  DL_BIND_ACK was not M_PCPROTO");*/
		return( -1 );
	}

	bind_ackp = (dl_bind_ack_t *) &response[ 0 ];
	if (bind_ackp->dl_primitive != (u_long) DL_BIND_ACK)
	  return( -1 );

	sioc.ic_cmd = DLIOCRAW;
	sioc.ic_timout = -1;
	sioc.ic_len = 0;
	sioc.ic_dp = NULL;
	ival = ioctl( fd, I_STR, &sioc );
	/*printf( "stream IOCTL returned %d\n", ival );*/
	if (ival < 0) {
		perror( "stream IOCTL" );
		exit( 1 );
	}

/*  The example from Sun included this.  I can't justify it, but it
    certainly does not seem to hurt...					*/

	if (ioctl( fd, I_FLUSH, FLUSHR ) < 0) {
		perror("I_FLUSH");
		return( -1 );
	}

	return( 0 );
}

/*  This program verifies the proposed EtherNet interface.
    It works like this:

    1.  The original EtherNet is expected to be in the form
        /dev/le0, with a unit number.  The program parses this
        into a "generic" device name (/dev/le) which should
        exist in the file system, and the PPA (0), which is
        the way the kernel programs expect a reference to a
        particular unit.

    2.  The program tries to open the device.  If this fails,
        it returns an error status.

    3.  Using the PPA, the program tells the kernel software
        to attach a particular EtherNet interface to this
        file descriptor.  It then verifies the attach
        operation works.

    4.  If all this works, the proposed EtherNet interface
        has been verified.					*/

int
verify_ethernet_interface( char *enet_device )

/*  char *enet_device;	input  */

{
	char	*generic_device;
	int	 fd;
	u_long	 ppa;
	char	 errmsg[ 122 ];

	generic_device = malloc( strlen( enet_device ) + 1 );
	if (generic_device == NULL)
	  return( -1 );
	parse_interface( enet_device, generic_device, &ppa );

	fd = open(generic_device, O_RDWR);

/*  If the open failed, the generic class of device does not exist.
    The entry can still be present in the file system; the call to
    open nevertheless gives No Such Device or Address.

    The process must have root or superuser access to proceed.	*/

	if (fd < 0) {
		sprintf( &errmsg[ 0 ], "failed to open %s", generic_device );
		perror( &errmsg[ 0 ] );
		free( generic_device );
		return( -1 );
	}

	free( generic_device );

	if (dlattach( fd, ppa )) {
		sprintf(
	   &errmsg[ 0 ], "attach to %s failed", enet_device
		);
		perror( &errmsg[ 0 ] );
		close( fd );
		return( -1 );
	}

	return( fd );
}

int
find_ethernet( int argc, char *argv[], char **device )
{
	int	 fd;
	char	*enet_iface;
	char	**default_iface_table;

	enet_iface = args_to_ethernet( argc, argv );
	if (enet_iface == NULL) {
		default_iface_table = &default_interface[ 0 ];
		while (*default_iface_table != NULL) {
			massage_device_name( *default_iface_table, &enet_iface );
			fd = verify_ethernet_interface( enet_iface );
			if (fd >= 0) {
				*device = enet_iface;
				return( fd );
			}	
			default_iface_table++;
			free( enet_iface );
		}
	}
	else {
		massage_device_name( enet_iface, &enet_iface );
		fd = verify_ethernet_interface( enet_iface );
		if (fd >= 0) {
			*device = enet_iface;
			return( fd );
		}
		else
		  free( enet_iface );
	}

/*  If the program comes here, it did not find
    an EtherNet interface that works.		*/

	return( -1 );
}

/*  The make protocol interface program should receive the protocol type as an argument  */

int
make_rarp_interface( int argc, char *argv[] )
{
	char	*enet_device;
	int	 enet_fd;

	enet_fd = find_ethernet( argc, argv, &enet_device );
	if (enet_fd < 0) {
		return( -1 );
	}

	register_rarp( enet_fd );

	return( enet_fd );
}

int
get_rarp_packet( int enet_fd )
{
int			 flags;
struct strbuf		 data;
struct ether_header	*pkt_header;
struct ether_header     *tmp_header;
unsigned char            byte;
char			 rarp_packet[ 1536 ];
int                      i;
int                      broadcast = 0;
int			 size,sport, dport;
char			*tmp_charptr;

   data.buf = &rarp_packet[ 0 ];
   data.maxlen = sizeof( rarp_packet );
   data.len = 0;
   flags = 0;


   size = sizeof(struct ether_header)+sizeof(struct ip)+
          sizeof(struct udphdr)+sizeof(struct bootp);


   /*printf("getting rarp packet.\n");*/

   for (;;) {
      if (getmsg( enet_fd, NULL, &data, &flags ) != 0)
      {             
         if (errno == EINTR) {
            fprintf( stderr,
		"Timed-out, is it connected and has it been rebooted?\n");
            continue;
         }
	 else
	    return( -1 );
      }
      else
         /* printf("checking data buffer.\n"); */
         tmp_header = (struct ether_header *) data.buf;
         /*print_ether_addr( (char *) &tmp_header->ether_dhost );*/
         /*printf("\n"); */
         broadcast = 1;
         for(i=0; i<6; i++)
         {
            byte=(unsigned char) *&tmp_header->ether_dhost.ether_addr_octet[i];
            /*printf("byte: %x\n", byte);*/
            if( byte != (unsigned char) 0xff )
            {
               broadcast=0;
               /* printf( "got general broadcast.\n"); */
            }
         }
         if (catch_type == INOVA) 
         {
            /* printf("datasize=%d", data.len); */
            /* printf("  should be=%d\n", size); */
	    tmp_charptr = (char *)(data.buf + 
		       sizeof (struct ether_header)+sizeof(struct ip));
            sport = *tmp_charptr * 16 + *(tmp_charptr+1);
	    dport = *(tmp_charptr+2) * 16 + *(tmp_charptr+3);
	    /* printf("ports: %d %d\n",sport,dport); */
            if ((data.len!=size) || (dport!=67) || (sport!=68) ) {
               broadcast = 0;
            }
         }
         if(!broadcast)
         {
            /*printf("continue.\n");*/
            continue;
         }
                     

		  break;
	}

	pkt_header = (struct ether_header *) data.buf;
        print_ether_addr( (char *) &pkt_header->ether_shost );
    	printf( "\n" );

/*  remainder of the RARP packet conforms to a struct ether_arp
    see <netinet/if_ether.h> and <netinet/arp.h>		*/

	return( 0 );
}

static
setup_timer()
{
	struct itimerval	timeval;

	catch_sigalrm();

	timeval.it_value.tv_sec = 6;
	timeval.it_value.tv_usec = 0;
	timeval.it_interval.tv_sec = 6;
	timeval.it_interval.tv_usec = 0;
	setitimer( ITIMER_REAL, &timeval, NULL );
}

main( int argc, char *argv[] )
{
	int	enet_fd;

	enet_fd = make_rarp_interface( argc, argv );
	if (enet_fd < 0)
	  exit( 1 );

/*  Be aware that make_rarp_interface may itself arrange to catch SIGALRM... */

	setup_timer();
	get_rarp_packet( enet_fd );
}
