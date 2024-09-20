/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*---------------------------------------------------------------------------
|	socket.c
|
|	This module contains procedures that initialize the socket
|	and sends messages down the socket to acqproc 
|	program.  It also contains code to display acqproc status
|	messages in the correct status field on the screen.
|
+---------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/file.h>
#include <errno.h>
#include <arpa/inet.h>

#ifdef NESSIE
#include "expQfuncs.h"
#else
#include "ACQ_SUN.h"
#endif
#include "mfileObj.h"
#include "acquisition.h"
#include "comm.h"
#include "variables.h"

extern COMM_INFO_STRUCT comm_addr[];

#ifdef NESSIE
#include "shrstatinfo.h"
extern int expQaddToTail(int priority, char* expidstr, char* expinfostr);
extern int expQaddToHead(int priority, char* expidstr, char* expinfostr);
extern int cmpTimeStamp( TIMESTAMP *ts1, TIMESTAMP *ts2 );
extern int getStatOpsCompl();
extern int getStatRecvGain();
extern int getStatRcvrNpErr();
extern int getStatNpErr();
extern int getStatGradTune(char *axis, char *type);
extern long getStatLockFreqAP();
extern int getStatSpinSet();
extern int SendAsyncInovaVnmr(CommPort to_addr, CommPort from_addr, char *msg);
extern int InitRecverAddr(char *host, int port, struct sockaddr_in *recver);
extern int set_comm_port(CommPort ptr);
extern int SendAsyncInova(CommPort to_addr, CommPort from_addr, char *msg);


#define  DELIMITER_1	' '
#define  DELIMITER_2	'\n'
#define  DELIMITER_3    ','

#define ASSIGN_A_KEY	-1
#define MAX_QNAME_LEN	20
#define VNMR_MSG_SIZE	512

char *cmd_for_acq[] = {
 "",		/* Entry 0 is dummy */
 "",		/* ACQQUEUE 1		Enter New Experiment into QUEUE */
 "",		/* REGISTERPORT 2	Register Acq. Display Update port */
 "",		/* UNREGPORT 3		Remove Acq. Display Update port */
 "abort",	/* ACQABORT 4		Abort Specified Acquisition */
 "fgcmplt",	/* FGREPLY 5		VNMR FG Complete Reply */
 "stop",	/* ACQSTOP 6		Stop Specified Acquisition */
 "superabort",	/* ACQSUPERABORT 7	Abort Acq & HAL to initial state */
 "acqdebug",	/* ACQDEBUG 8           Set acquisition debug flag */
 "ipctst",	/* IPCTST 9		Echo back to Vnmr */
 "parmchg",	/* PARMCHG 10		change parameter (wexp,wnt,wbt,etc.) */
 "auto",	/* AUTOMODE 11		change to automation mode */
 "normal",	/* NORMAL 12		return to normal mode */
 "resume",	/* RESUME 13		send resume to autoproc */
 "",		/* SUPPEND 14		send suppend to autoproc */
 "halt",	/* ACQHALT 15		send halt, an abort w/ Wexp processing */
 "sethw",	/* ACQHARDWARE 16	insert, eject, set DACs, etc. */
 "getStatBlock",/* READACQHW 17		read acquisition hardware parameters */
 "queuequery",	/* QUEQUERY 18		send acqproc queue status back */
 "reserveConsole", /* ACCESSQUERY 19    check user for access permission  */
 "",		/* RECONREQUEST 20	request reconnection to Vnmr */
 "transparent",	/* TRANSPARENT 21	*/
 "releaseConsole", /* RELEASECONSOLE 22	release console */
 "queryStatus", /* QUERYSTATUS 23	query status of system */
 "aupdt",	/* RTAUPDT 24    	Update real-time vars,acodes in acq(inova) */
 "atcmd", 	/* ATCMD 25        	Queue an atcmd, Expproc proc only */
 "robot",       /* ROBOT 26             Send command to Roboproc */
 "autoqmsg",    /* AUTOQMSG 27          Send command to Listeners */
 "dequeue",	/* ACQDEQUEUE 28	Remove queued Acquisition */
 "halt2"	/* ACQHALT2 29		send halt, an abort w/ no processing */
                    };

static int deliverMessage(char *interface, char *message );
#endif

int wait_for_select(int fd, int wait_interval );

#define TRUE 		1
#define FALSE 		0
#define SLEEP 		2
#define MAXRETRY 	4
#define ERROR 		0


#undef DEBUG

#ifdef  DEBUG
#define TPRINT0(str) \
	fprintf(stderr,str)
#define TPRINT1(str, arg1) \
	fprintf(stderr,str,arg1)
#define TPRINT2(str, arg1, arg2) \
	fprintf(stderr,str,arg1,arg2)
#define TPRINT3(str, arg1, arg2, arg3) \
	fprintf(stderr,str,arg1,arg2,arg3)
#else
#define TPRINT0(str) 
#define TPRINT1(str, arg2) 
#define TPRINT2(str, arg1, arg2) 
#define TPRINT3(str, arg1, arg2, arg3) 
#endif

struct codeWithMsg {
    int      code;
    char    *errmsg;
};


static char *append_char(char *string, char character )
{
	int	len;

	if (string == NULL)
	  return( NULL );

	len = strlen( string );
	*(string + len) = character;
	*(string + len + 1) = '\0';

	return( string );
}

int deliverMessageSuid(char *interface, char *message )
{

#ifdef NESSIE
	return (deliverMessage( interface, message ));
#else
	return(-1);
#endif

}

#ifdef NESSIE
static int deliverMessage(char *interface, char *message )
{
	int		stat;
	CommPort	tmp_addr = &comm_addr[LOCAL_COMM_ID];

	set_comm( LOCAL_COMM_ID, ADDRESS, interface );
	stat = InitRecverAddr(tmp_addr->host,tmp_addr->port,&(tmp_addr->messname));
	if (stat != RET_OK)
	  return(RET_ERROR);

	stat = SendAsyncInovaVnmr( tmp_addr, NULL, message );
	if (stat != RET_OK)
	  return(RET_ERROR);
	else
	  return(RET_OK);
}

/*  I added a third argument to setup_signal_irpt so the process can readily 
    restore the original action for the signal.  Although the arguments aren't
    symmetric, setting up the sigaction is a bit of a hassle.  At this time we
    are just interested in specifying a new interrupt handler.			*/

int
setup_signal_irpt(int signal, void (*irpt_prog)(),
                  struct sigaction *oldaction )
{
	int			rval;
	struct sigaction	intserv;
	sigset_t		qmask;
 
  /* --- set up signal handler --- */

	sigemptyset( &qmask );
	sigaddset( &qmask, signal );
	intserv.sa_handler = irpt_prog;
	intserv.sa_mask = qmask;
	intserv.sa_flags = 0;

	rval = sigaction( signal, &intserv, oldaction );
	return( rval );
}

#ifdef OLD
static int
restore_signal_irpt(int signal, struct sigaction *oldaction )
{
	int	rval;

	rval = sigaction( signal, oldaction, NULL );
	return( rval );
}

static int		message_waiting;
static struct sigaction oldsigalrm_action;
static struct sigaction oldsigusr1_action;

static void
localInterfaceReady()
{
	int			ival;
	struct itimerval	turnItOff;

/*  stop the interval timer so it won't inadvertently go off and disrupt the process.  */

	turnItOff.it_interval.tv_sec = 0;
	turnItOff.it_interval.tv_usec = 0;
	turnItOff.it_value.tv_sec = 0;
	turnItOff.it_value.tv_usec = 0;
	ival = setitimer( ITIMER_REAL, &turnItOff, NULL );

/*  Tell the program a Message is waiting.  */

	message_waiting = 131071;
}
#endif  /* OLD */

static void
catchTimeout()
{
}
#endif

int talk2Acq(char *hostname, char *username, int cmd,
             char *msg_for_acq, char *msg_for_vnmr, int mfv_len )
{
#ifdef NESSIE
	int	 mlen;
	char	 msg[256*4];
	int	 ival;
	CommPort acq_addr  = &comm_addr[ACQ_COMM_ID];
        CommPort tmp_from =  &comm_addr[LOCAL_COMM_ID];

        (void) msg_for_vnmr;
        (void) mfv_len;
        tmp_from->path[0] = '\0';
        tmp_from->pid = getpid();
        strcpy(tmp_from->host,hostname);
        set_comm_port(tmp_from);	/* this call creates a socket */

	mlen = strlen( cmd_for_acq[cmd] ) +
	       strlen( tmp_from->path ) +
	       strlen( msg_for_acq ) + 2;
	if (mlen > 255) {
		acq_errno = MSG_LEN_ERROR;
		fprintf( stderr, "talk2Acq error - message len: %d greater than 255\n",mlen );
		return( -1 );
	}

        sprintf(msg,"%s%c%s%c%s%c%s%c%d%c%s",
                cmd_for_acq[cmd], DELIMITER_1,
                tmp_from->path,DELIMITER_2,
                username,DELIMITER_3,hostname,DELIMITER_3,tmp_from->pid,DELIMITER_2,
                msg_for_acq);

	if (SendAsyncInova( acq_addr, tmp_from, msg ) != RET_OK) {
		close( tmp_from->msgesocket );
		return( -1 );
	}

/*  Use `select' to wait for Acqproc to respond to us.	*/

	ival = wait_for_select( tmp_from->msgesocket, 20 );
	if (ival < 1) {
		if (ival < 0)
		  acq_errno = SELECT_ERROR;
		else if (ival == 0)
		  acq_errno = CONNECT_ERROR;
		close( tmp_from->msgesocket );
		return( -1 );
	}
	ival = accept( tmp_from->msgesocket, 0, 0 );
	close( tmp_from->msgesocket );		/* We are finished with */
						/* the original socket. */
	if (ival < 0) {
		acq_errno = ACCEPT_ERROR;
		return( -1 );
	}
	else
	  return( ival );	/* file descriptor for the connected socket.	*/
#endif
}

int send2Acq(int cmd, char *msg_for_acq )
{
        CommPort acq_addr  = &comm_addr[ACQ_COMM_ID];
        CommPort vnmr_addr = &comm_addr[VNMR_COMM_ID];
#ifdef NESSIE
	char		 maddr[256];

   TPRINT3("request cmd %d ('%s') with msg %s\n",cmd,cmd_for_acq[cmd],msg_for_acq);
	strcpy( maddr, cmd_for_acq[cmd] );
	append_char( maddr, DELIMITER_1 );
	strcat( maddr, vnmr_addr->path );
	append_char( maddr, DELIMITER_2 );
	strcat( maddr, msg_for_acq );
        TPRINT2("send message '%s' to %s\n",maddr,acq_addr->host);
	SendAsyncInova( acq_addr, vnmr_addr, maddr );
	return( 0 );
#else
	SendAsync( acq_addr, vnmr_addr, cmd, msg_for_acq );
	return( 0 );
#endif
}


/*  Returns 0 if successful, -1 if error.  */
/* passes user's name to allow verification of access by acqproc */
/*  9/13/91  GMB */

#ifndef NESSIE

static int poke_acqproc(char *hostname, char *username,
                        int cmd, char *message )
{
    char	 acqproc_msg[ 256 ];
    int		 ival;
    int		 meslength;
    int		 on = 1;
#ifdef WINBRIDGE
    CommPort acq_addr  = &comm_addr[INFO_COMM_ID];
#else
    CommPort acq_addr  = &comm_addr[ACQ_COMM_ID];
#endif
    CommPort vnmr_addr = &comm_addr[VNMR_COMM_ID];
    CommPort tmp_from  = &comm_addr[LOCAL_COMM_ID];

        TPRINT0("Poke started \n");
    tmp_from->path[0] = '\0';
    tmp_from->pid = vnmr_addr->pid;
    strcpy(tmp_from->host,hostname);

    tmp_from->messname.sin_family = AF_INET;
    tmp_from->messname.sin_addr.s_addr = INADDR_ANY;
    tmp_from->messname.sin_port = 0;

    tmp_from->msgesocket = socket( AF_INET, SOCK_STREAM, 0 );
    if (tmp_from->msgesocket < 0) {
        acq_errno = tmp_from->msgesocket;
        return( -1 );					/*  Not ABORT !! */
    }
/* Specify socket options */
        TPRINT0("Poke socket \n");

#ifndef LINUX
    setsockopt(tmp_from->msgesocket,SOL_SOCKET,SO_USELOOPBACK,(char *)&on,sizeof(on));
#endif
    setsockopt(tmp_from->msgesocket,SOL_SOCKET,(~SO_LINGER),(char *)&on,sizeof(on));

    if (bind(tmp_from->msgesocket,(caddr_t) &(tmp_from->messname),
             sizeof(tmp_from->messname)) != 0) {
        acq_errno = BIND_ERROR;
        close( tmp_from->msgesocket );
        return( -1 );					/*  Not ABORT !! */
    }
        TPRINT0("Poke bind \n");

    meslength = sizeof(tmp_from->messname);
    getsockname(tmp_from->msgesocket,&tmp_from->messname,&meslength);
    tmp_from->port = tmp_from->messname.sin_port;
 
    listen(tmp_from->msgesocket,5);        /* set up listening queue ?? */


    sprintf( &acqproc_msg[ 0 ], "%s,%.200s", username,message);
    
#ifdef WINBRIDGE
#ifdef DEBUG    
    fprintf(stdout, "DEBUG jgw: socket.c poke_acqproc username = %s\n", username);
    fprintf(stdout, "DEBUG jgw: socket.c poke_acqproc message = %s\n", message);
    fprintf(stdout, "DEBUG jgw: socket.c poke_acqproc acq_addr->port = %d\n", acq_addr->port);
    fprintf(stdout, "DEBUG jgw: socket.c poke_acqproc cmd = %d\n", cmd);
#endif
    close( tmp_from->msgesocket );
    return( -1 );
#endif
    if (SendAsync( acq_addr, tmp_from, cmd, &acqproc_msg[ 0 ] ) == RET_ERROR )
    {
        close( tmp_from->msgesocket );
	return( -1 );
    }

/*  Use `select' to wait for Acqproc to respond to us.	*/

    ival = wait_for_select( tmp_from->msgesocket, 20 );
    if (ival < 1) {
	if (ival < 0)
		  acq_errno = SELECT_ERROR;
	else if (ival == 0)
		  acq_errno = CONNECT_ERROR;
        close( tmp_from->msgesocket );
	return( -1 );
    }
    ival = accept( tmp_from->msgesocket, 0, 0 );
    close( tmp_from->msgesocket );		/* We are finished with */
					/* the original socket. */
    if (ival < 0) {
		acq_errno = ACCEPT_ERROR;
		return( -1 );
    }
    else
	 return( ival );	/* file descriptor for the connected socket.	*/
}
#endif

/*  Render the file descriptor blocking  */

static int render_blocking(int fd )
{
	int	flags, ival;

	flags = fcntl( fd, F_GETFL, 0 );
	if (flags == -1)
	  return( -2 );

	flags &= ~FNDELAY;
	ival = fcntl( fd, F_SETFL, flags );
	if (ival != 0)
	  return( -3 );
	return( 0 );
}

int prepare_reply_socket(int connsid )
{
	int	ival;

/*  Render the connected socket blocking  */

	ival = render_blocking( connsid );
	if (ival != 0) {
		if (ival == -2)
		  acq_errno = FCNTL_READ_ERROR;
		else
		  acq_errno = FCNTL_WRITE_ERROR;
		return( -1 );
	}

/*  Wait for Acqproc to reply or timeout  */

	ival = wait_for_select( connsid, 20 );
	if (ival < 1) {
		acq_errno = SELECT_ERROR;
		close( connsid );
		return( -1 );
	}

	return( 0 );
}

/*  Reads a message from socket described by ``sfd''.
    Note that in a normal return, the partner process
    has closed its side of the socket.			*/

int
getascii(int sfd, char buffer[], int bufsize )
{
        char    acqmsg[ 258 ];
        int     i, j, nchr;
 
        i = 0;
        while (131071) {
                nchr = read( sfd, &acqmsg[ 0 ], 256 );
                if (nchr < 0) {
                        perror( "socket read" );
                        return( -1 );
                }
                if (nchr == 0)
                  break;
 
        /*  Copy from temp buffer to permanent buffer  */
 
                for (j = 0; j < nchr; j++, i++) {
                        if (i > bufsize-2) {    /* Overran */
                                buffer[ bufsize-1 ] = '\0';
                                return( -1 );
                        }
                        buffer[ i ] = acqmsg[ j ];
                }
        }
 
	TPRINT1("got ascii: %s\n",acqmsg);
        buffer[ i ] = '\0';
	TPRINT1("converted ascii: %s\n",buffer);
        return( i );
}

/*  Similar to getascii except reads binary data, so it
    does not terminate the user buffer with a NUL character.	*/

#ifndef NESSIE
static int
getbinary(int sfd, char *buffer, int bufsize )
{
	int	i, j, nchr;
        char    quickbuf[ 512 ];

	i = 0;
	while (131071) {
		nchr = read( sfd, &quickbuf[ 0 ], sizeof( quickbuf ) );
		if (nchr < 0) {
			perror( "socket read" );
			return( -1 );
		}
		if (nchr == 0)
		  break;

        /*  Copy from temp buffer to permanent buffer  */
 
                for (j = 0; j < nchr; j++) {
			if (i < bufsize) {
                        	buffer[ i ] = quickbuf[ j ];
				i++;
			}
                }
	}

	return( i );
}
#endif

/*  This routine is written so the two character pointers can be identical.	*/

int talk_2_acqproc(char *hostname, char *username, int cmd,
                   char *msg_for_acqproc, char *msg_for_vnmr, int mfv_len )
{
	char	*tptr;
	int	 connsid, ival;

#ifdef NESSIE

        ival = talk2Acq(hostname, username,  cmd, msg_for_acqproc, msg_for_vnmr, mfv_len);
	if (ival < 0) {
		return( -1 );
	}
	else
	  connsid = ival;
#else

/*  When you poke the Acqproc here, you expect to receive a reply.  If the poke
    works, the return value is the socket from which to receive this reply.	*/

	ival = poke_acqproc(hostname, username, cmd, msg_for_acqproc );
	if (ival < 0) {
		return( -1 );
	}
	else
	  connsid = ival;
#endif

	ival = prepare_reply_socket( connsid );
	if (ival != 0) {
		return( -1 );
	}

	ival = getascii( connsid, msg_for_vnmr, mfv_len-1 );	/* room for NUL */

/* Eliminate the ^D character from the Acqproc response.	*/

	if ( (tptr = strchr( msg_for_vnmr, '\004' )) != NULL )
	  *tptr = '\0';
	close( connsid );

	return( 0 );
}

#ifndef NESSIE
/*  Based on talk to acqproc  */
struct ia_stat	shlk_block;
#endif

int getExpStatusInt(int index, int *val)
{
#ifdef NESSIE

   if (index == EXP_VALID)
   {
      *val = 1;
   }
   else if (index == EXP_LKPOWER)
   {
      *val = getStatLkPower();
   }
   else if (index == EXP_LKGAIN)
   {
      *val = getStatLkGain();
   }
   else if (index == EXP_LKPHASE)
   {
      *val = getStatLkPhase();
   }
   else if (index == EXP_RCVRGAIN)
   {
      *val = getStatRecvGain();
   }
   else if (index == EXP_SPINACT)
   {
      *val = getStatSpinAct();
   }
   else if (index == EXP_SPINSET)
   {
      *val = getStatSpinSet();
   }
   else if (index == EXP_LOCKFREQAP)
   {
      *val = getStatLockFreqAP();
   }
   else if (index == EXP_GTUNEPX)
   {
      *val = getStatGradTune("x","p");
   }
   else if (index == EXP_GTUNEIX)
   {
      *val = getStatGradTune("x","i");
   }
   else if (index == EXP_GTUNESX)
   {
      *val = getStatGradTune("x","s");
   }
   else if (index == EXP_GTUNEPY)
   {
      *val = getStatGradTune("y","p");
   }
   else if (index == EXP_GTUNEIY)
   {
      *val = getStatGradTune("y","i");
   }
   else if (index == EXP_GTUNESY)
   {
      *val = getStatGradTune("y","s");
   }
   else if (index == EXP_GTUNEPZ)
   {
      *val = getStatGradTune("z","p");
   }
   else if (index == EXP_GTUNEIZ)
   {
      *val = getStatGradTune("z","i");
   }
   else if (index == EXP_GTUNESZ)
   {
      *val = getStatGradTune("z","s");
   }
   else if (index == EXP_GSTATUS)
   {
      *val = getStatGradTune("{","p"); /* Kluge for now to get GradError word */
       /**val = getStatGradError();*/
   }
   else if (index == EXP_NPERR)
   {
      *val = getStatNpErr();
   }
   else if (index == EXP_RCVRNPERR)
   {
      *val = getStatRcvrNpErr();
   }

#else

   if (index == EXP_VALID)
   {
      *val = shlk_block.valid_data;
   }
   else if (index == EXP_LKPOWER)
   {
      *val = shlk_block.lk_power;
   }
   else if (index == EXP_LKGAIN)
   {
      *val = shlk_block.lk_gain;
   }
   else if (index == EXP_LKPHASE)
   {
      *val = shlk_block.lk_phase;
   }
   else if (index == EXP_RCVRGAIN)
   {
      *val = shlk_block.rcvr_gain;
   }
   else if (index == EXP_SPINSET)
   {
      *val = shlk_block.spinspd;
   }
   else if (index == EXP_SPINACT)
   {
      *val = shlk_block.spinspd;
   }
   else if (index == EXP_LOCKFREQAP)
   {
      *val = -1;
   }
#endif
   return(0);
}

int getExpStatusShim(int index, int *val)
{
#ifdef NESSIE

   *val = getStatShimValue(index);

#else

   *val = shlk_block.sh_dacs[ index ];

#endif

 return(0);
}

#ifdef NESSIE
static int usleep2(int interval )
{
	int			ival, sec, usec;
	sigset_t		qmask;
	struct itimerval	oval, tval;
	struct sigaction	sigalarm_vec, oldalarm_vec;

	if (interval < 0) return( -1 );
 
	sigprocmask( SIG_BLOCK, NULL, &qmask );		/* Get current mask of signals */
	sigaddset( &qmask, SIGALRM );			/* add SIGALRM to the mask to  */
	sigalarm_vec.sa_handler = catchTimeout;		/* be used when the signal is  */
	sigalarm_vec.sa_mask    = qmask;		/* delivered */
	sigalarm_vec.sa_flags = 0;
	ival = sigaction( SIGALRM, &sigalarm_vec, &oldalarm_vec );
	if (ival != 0) {
		perror( "sigaction failure" );
		return( -1 );
	}

/*  Save a microsecond by not dividing if the result would be zero  */

	if (interval < 1000000) {
		sec  = 0;
		usec = interval;
	}
	else {
		sec  = interval / 1000000;
		usec = interval % 1000000;
	}
	tval.it_value.tv_sec  = sec;
	tval.it_value.tv_usec = usec;

/*  The interval timer causes SIGALRM's to be delivered every "interval"
    (the value of the argument to the program) microseconds.  Although
    one would think only one SIGALRM should be required, if this process
    gets swapped out or for other reason is delayed between the arming
    of the timer and before the call to "pause", then this process will
    not ever get the SIGALRM and thus will hang indefinitely.

    Repetition of the SIGALRM avoids this difficulty.  It does make it
    imperative that the interval timer get turned off BEFORE the interrupt
    is unregistered with "sigaction" - for if sigaction was called first
    and a SIGALRM was delivered, the default action is for this process
    to exit.  To the user, it would appear as if the process had crashed.  */

	tval.it_interval.tv_sec  = sec;
	tval.it_interval.tv_usec = usec;

	ival = setitimer( ITIMER_REAL, &tval, &oval );
	if (ival != 0) {
		perror( "set timer failure" );
		return( -1 );
	}

/*  Following call suspends this process until something
    happens, here the interval timer expiring.			*/

	ival = pause();

/*  Restore original SIGALRM program and original interval timer.
    Order of these two operations is critical -  see note above.  */

	ival = setitimer( ITIMER_REAL, &oval, NULL );
	ival = sigaction( SIGALRM, &oldalarm_vec, NULL );
	return( 0 );
}
#endif

/*  Get a Stat Block from the console.

    Program may be called by either the Gemini/VersaBus console version or
    the New Digital Console version of VNMR.

    Therefore you should only use entry points such as getExpStatusInt or
    getExpStatusShim to access values in the stat block.			*/

int
get_ia_stat(char *hostname, char *username )
{

#ifdef NESSIE
	int		connsid, iter, ival, initOpsCompl, newOpsCompl, updated;
	TIMESTAMP	timeStamp, timeStamp2;
	char		expprocReply[ 122 ];

	gettimeofday( &timeStamp, NULL);
	initOpsCompl = getStatOpsCompl();
	ival = talk2Acq( hostname, username, READACQHW, "", &expprocReply[ 0 ], sizeof( expprocReply ) );
	if (ival < 0)
	  return( -1 );
	else
	  connsid = ival;

	ival = prepare_reply_socket( connsid );
	if (ival != 0) {
		close( connsid );
		return( -1 );
	}
	ival = getascii( connsid, &expprocReply[ 0 ], sizeof( expprocReply ) - 1 );
	close( connsid );

	updated = 0;
	for (iter = 0; iter < 50; iter++) {		/* increased from 10 to 50, July 1997 */
		usleep2( 20000 );			/* 20 ms, 0.02 s */
		getStatTimeStamp( &timeStamp2 );
		newOpsCompl = getStatOpsCompl();
		if (cmpTimeStamp( &timeStamp2, &timeStamp ) > 0 &&
		    newOpsCompl != initOpsCompl) {
			updated = 1;
			break;
		}
	}

	if (updated == 0)
	  return( -1 );
	else
	  return( 0 );
#else
	int	connsid, ival;
#if defined(WINBRIDGE) && defined(DEBUG)
        fprintf(stdout, "DEBUG jgw: socket.c get_ia_stat calling poke_acqproc\n");
        fprintf(stdout, "DEBUG jgw: socket.c get_ia_stat hostname = %s\n", hostname);
        fprintf(stdout, "DEBUG jgw: socket.c get_ia_stat username = %s\n", username);
        fprintf(stdout, "DEBUG jgw: socket.c get_ia_stat cmd = %d\n", READACQHW);
#endif
	ival = poke_acqproc(hostname, username, READACQHW, "1,24,");
	if (ival < 0) {
		return( -1 );
	}
	else
	  connsid = ival;

	ival = prepare_reply_socket( connsid );
	if (ival != 0) {
		return( -1 );
	}

	memset( &shlk_block, 0, sizeof( struct ia_stat ) );
	ival = getbinary( connsid, &shlk_block, sizeof( struct ia_stat ) );
	close( connsid );

	if (ival > 0)
	  return( 0 );
	else
	  return( -1 );
#endif
}

/*  Wait for selection of a single file descriptor or time-out.
    Returns value returned by ``select''			*/

#define WAIT_TIME_OUT 0	/* select return 0 if timed out */
#define WAIT_ERROR -1		/* select return -1 if error */

int
wait_for_select(int fd, int wait_interval )
{
	int		ival;
	fd_set		readfds;
	struct timeval	timeout;
        int 		done = 0;

        FD_ZERO( &readfds);
        FD_SET(fd, &readfds);
	timeout.tv_sec  = wait_interval;
	timeout.tv_usec = 0;

        while ( !done )
        {
	   ival = select( fd+1, &readfds, NULL, NULL, &timeout );
	   switch ( ival )
           {
	     case WAIT_TIME_OUT:  
				    done = 1;
				    break;
	
	     case WAIT_ERROR:
				if ( errno != EINTR )
			          done = 1;
				break;

	     /* Valid file descriptor ready */
	     default:			done = 1;
					break;
           }
        }
        
	return( ival );
}

int open_ia_stat(char *parpath, int removefile, int valid_test)
{
#ifdef NESSIE
        (void) parpath;
        (void) removefile;
        (void) valid_test;
        return(1);
#else
        int             parfd;
        struct ia_stat *statbuf;

        statbuf = 0;
        if ((parfd = open(parpath, O_RDONLY)) >= 0)
        {
           statbuf = &shlk_block;
           if (read(parfd,statbuf,sizeof(struct ia_stat)) < 0 )
           {
              statbuf = 0;
           }
           close(parfd);
           if (removefile)
              unlink(parpath);
           if (statbuf && statbuf->valid_data != valid_test)
              statbuf = 0;
        }
        return(statbuf != 0);
#endif
}

/* Functions required by PSG. Not pretty but it works */

/*-----------------------------------------------------------------
* nessie functions
*  used by psg.c
*
*  sendExpproc is here, rather than in PSG, because there are
*  different versions for INOVA and non-INOVA systems.  Placing
*  it here allows the same source and object PSG programs to
*  work on INOVA and non-INOVA systems
*
*  tunecmd_convert is used by programs in specfreq.c, part of Vnmr
*-------------------------------------------------------------------*/

typedef  short  codeint;

#ifdef NESSIE
int setup_comm()   /* mapin ExpQ */
{
   init_comm_addr();
   return(0);
}
 
int sendExpproc(char *acqaddrstr, char *filename, char *info, int nextflag)
{
#if ( !defined(MACOS) && !defined(NOACQ) )
   int stat;
   CommPort acq_addr = &comm_addr[ACQ_COMM_ID];
 
   initExpQs(0);   /* map in queue don't clear */
   if (!nextflag)
      expQaddToTail(NORMALPRIO, filename, info);
   else
      expQaddToHead(NORMALPRIO, filename, info);

   set_comm( ACQ_COMM_ID, ADDRESS, acqaddrstr );
   stat = InitRecverAddr(acq_addr->host,acq_addr->port,&(acq_addr->messname));
   if (stat != RET_OK) {
       expQdelete(NORMALPRIO, filename);
       return(RET_ERROR);
   }
   stat = SendAsyncInova( acq_addr, NULL, "chkExpQ" );
   if (stat != RET_OK) {
       expQdelete(NORMALPRIO, filename);
       return(RET_ERROR);
   }
   else
#endif
      return(RET_OK);
}

int sendNvExpproc(char *acqaddrstr, char *filename, char *info, int nextflag)
{
#if ( !defined(MACOS) && !defined(NOACQ) )
   int stat;
    char cmdstr[256];
   CommPort acq_addr = &comm_addr[ACQ_COMM_ID];
 
   cmdstr[0] = 0;
   sprintf(cmdstr,"%s, %s,",filename,info);
   if (!nextflag)
      sprintf(cmdstr,"add2Qtail %s, %s,",filename,info);
   else
      sprintf(cmdstr,"add2Qhead %s, %s,",filename,info);

   set_comm( ACQ_COMM_ID, ADDRESS, acqaddrstr );
   stat = InitRecverAddr(acq_addr->host,acq_addr->port,&(acq_addr->messname));
   if (stat != RET_OK) {
       expQdelete(NORMALPRIO, filename);
       return(RET_ERROR);
   }
   stat = SendAsyncInova( acq_addr, NULL, cmdstr );
   if (stat != RET_OK) {
       expQdelete(NORMALPRIO, filename);
       return(RET_ERROR);
   }
   else
#endif
      return(RET_OK);
}

int tunecmd_convert(codeint *buffer, int bufferlen)
{
#ifndef LINUX
        codeint  *tempbuf, *tempbuffree;
        codeint  oldbufferlen, ncodes, j;
        tempbuf = (codeint *) malloc( 20 * sizeof( codeint ) );
        tempbuffree = tempbuf;  /* to free buffer later */
 
        for(j=0; j<20; j++)
           tempbuf[j] = buffer[j];   /* next step will convert from tempbuf to buffer
                                     those are nessie codes */
        *buffer++ = *tempbuf++ ;  /* SET_TUNE is in first element of buffer */
        *buffer++ = *tempbuf++ ;  /* channel is in */
                                  /* both pointers are at length location */
        oldbufferlen = *tempbuf++ ; /* save length of old pts code,
                                      tempbuf pointer is at starting of old code */
        *buffer++ = 0 ;          /* set 0 to new length location and move buffer
                                     pointer to starting point of new code */
        ncodes = gen_apbcout(buffer, tempbuf, oldbufferlen) ;
                                 /* convert old pts codes to nessie styled pts codes*/
        *(buffer-1) = ncodes ;   /* new ptscode length is in buffer */
        tempbuf = tempbuf + oldbufferlen + 1;   /* move tempbuf pointer to band location
                                                +1 because cnt-1 in
                                                freq_device.c:do_tune_acode() */
        buffer = buffer + ncodes;     /* move buffer pointer to band location */
        *buffer++ = *tempbuf++ ;      /* transfer band from addr to xbuffer */
 
        free(tempbuffree);
        return (ncodes+4);   /* this is the nessie tune acode buffer length */
                             /* +4 --> + 1 for SET_TUNE (32)
                                       + 1 for channel #
                                       + 1 for nessie styled pts code count (11)
                                       + 1 for band select (last item of nessie code) */
#else
      (void) buffer;
      (void) bufferlen;
      return 0;
#endif

}

#else
setup_comm()
{
   init_comm_addr();
   return(0);
}
 
sendExpproc(char *acqaddr, char *filename, char *info, int nextflag)
{
   return(-1);  /* always error */
}

int  
tunecmd_convert(codeint *buffer, int bufferlen)
{
   return(bufferlen);         /* same old codes and length */
}

#endif

int initVnmrComm(char *addr)
{
   CommPort vnmr_addr = &comm_addr[VNMR_COMM_ID];

   INIT_VNMR_ADDR();
   SET_VNMR_ADDR(addr);
   VNMR_ADDR_OK();
#ifndef NESSIE
   InitRecverAddr(vnmr_addr->host,vnmr_addr->port,&(vnmr_addr->messname) );
#endif
   return(vnmr_addr->pid);
}

int sendToVnmr(char *msge )
{
        CommPort vnmr_addr = &comm_addr[VNMR_COMM_ID];
#ifdef NESSIE
	return( deliverMessage( vnmr_addr->path, msge ) );
#else
	SendAsync2( vnmr_addr, msge );
	return( 0 );
#endif
}

/*
 * Spin controls
 * 
 * spinSetSpeed is the requested spinning speed
 * spinOnOff controls whether spinning is active.
 * The function getInfoSpinSpeed() makes these decisions
 * and returns either spinSetSpeed or 0, depending on spinOnOff.
 *
 * spinSetRate is the requested air flow rate
 * spinUseRate controls whether spinning speed or air flow rate is active.
 *
 * spinSelect controls which type of spin controller the
 * speed request is sent to.  If spinSelect < 0, then
 * the spinner control selection is based on spinSwitchValue.
 * If spinSetValue >= spinSwitchValue then the high speed
 * spinner is selected; otherwise the low speed spinner is selected.
 * If spinSelect >= 0, then if spinSetValue >= spinSelect
 * then the high speed spinner is selected; otherwise the
 * low speed spinner is selected.
 * The function getInfoSpinner() makes these decisions and
 * returns the appropriate value.
 */

#define INFO_STR_SIZE 256

/*******************/
struct _sharedVnmrInfo
/*******************/
/* Each file block contains the following header        */
{
   int 	  VersID;		/* Structure ID         */
   char   autoDir[INFO_STR_SIZE];		/* automation directory */
   char   gradType[INFO_STR_SIZE];

   int    spinOnOff;
   int    spinSetSpeed;
   int    spinUseRate;
   int    spinSetRate;
   int    spinSelect;
   int    spinSwitchSpeed;
   int    spinExpControl;
   int    spinErrorControl;
   int    insertEjectExpControl;
   int    tempOnOff;
   int    tempSetPoint;
   int    tempExpControl;
   int    tempErrorControl;
};

static struct _sharedVnmrInfo *sharedVnmrInfo;
static MFILE_ID inmd = NULL;
static struct _sharedVnmrInfo localOnly;

int openVnmrInfo(char *dir)
{
   char filename[256];

   sprintf(filename,"%s/acq/info",dir);
   inmd = mOpen(filename,sizeof(struct _sharedVnmrInfo) ,O_RDWR | O_CREAT);
   if (inmd == NULL)
   {
      sharedVnmrInfo = &localOnly;
   }
   else
   {
      inmd->newByteLen = inmd->mapLen;
      sharedVnmrInfo = (struct _sharedVnmrInfo *) inmd->offsetAddr;
   }
   return(0);
}

int closeVnmrInfo()
{
   if (inmd != NULL)
      mClose(inmd);
   inmd = NULL;
   return(0);
}

static int getInfoStr(char *retstr, char *arg, int maxlen)
{
  if (maxlen >= INFO_STR_SIZE)
  {
     strcpy(retstr,arg);
  }
  else
  {
     strncpy(retstr,arg,maxlen);
     retstr[maxlen-1] = '\0';
  }
  return(0);
}
static int setInfoStr(char *arg, char *given)
{
  int len;
  len = strlen(given);

  if (len > INFO_STR_SIZE)
  {
     strncpy(arg,given,INFO_STR_SIZE);
     arg[INFO_STR_SIZE-1] = '\0';
  }
  else
  {
     strcpy(arg,given);
  }
  return(0);
}

int setAutoDir(char *str)
{
   return( setInfoStr(sharedVnmrInfo->autoDir,str) );
}
int getAutoDir(char *str, int maxlen)
{
   return( getInfoStr(str,sharedVnmrInfo->autoDir, maxlen) );
}

int setInfoSpinOnOff(int val)
{
   sharedVnmrInfo->spinOnOff = htonl(val);
   return(0);
}
int getInfoSpinOnOff()
{
   return(ntohl(sharedVnmrInfo->spinOnOff));
}

int setInfoSpinSetSpeed(int val)
{
   sharedVnmrInfo->spinSetSpeed = htonl(val);
   return(0);
}
int getInfoSpinSetSpeed()
{
   return(ntohl(sharedVnmrInfo->spinSetSpeed));
}

int setInfoSpinUseRate(int val)
{
   sharedVnmrInfo->spinUseRate = htonl(val);
   return(0);
}
int getInfoSpinUseRate()
{
   return(ntohl(sharedVnmrInfo->spinUseRate));
}

int setInfoSpinSetRate(int val)
{
   sharedVnmrInfo->spinSetRate = htonl(val);
   return(0);
}
int getInfoSpinSetRate()
{
   return(ntohl(sharedVnmrInfo->spinSetRate));
}

int setInfoSpinSelect(int val)
{
   sharedVnmrInfo->spinSelect = htonl(val);
   return(0);
}
int getInfoSpinSelect()
{
   return(ntohl(sharedVnmrInfo->spinSelect));
}

int setInfoSpinSwitchSpeed(int val)
{
   sharedVnmrInfo->spinSwitchSpeed = htonl(val);
   return(0);
}
int getInfoSpinSwitchSpeed()
{
   return(ntohl(sharedVnmrInfo->spinSwitchSpeed));
}

/* No corresponding set functions for getInfoSpinSpeed()
 * and getInfoSpinner()
 */
int getInfoSpinSpeed()
{
   return( (ntohl(sharedVnmrInfo->spinOnOff)) ?
            ntohl(sharedVnmrInfo->spinSetSpeed) : 0);
}
int getInfoSpinner()
{
   int tmp;

   /* Note: ntohl returns an unsigned int, which is never < 0 */
   tmp = ntohl(sharedVnmrInfo->spinSelect);
   return( (tmp < 0) ?
            ntohl(sharedVnmrInfo->spinSwitchSpeed) :
            ntohl(sharedVnmrInfo->spinSelect));
}

int setInfoSpinExpControl(int val)
{
   sharedVnmrInfo->spinExpControl = htonl(val);
   return(0);
}
int getInfoSpinExpControl()
{
   return(ntohl(sharedVnmrInfo->spinExpControl));
}

int setInfoSpinErrorControl(int val)
{
   sharedVnmrInfo->spinErrorControl = htonl(val);
   return(0);
}
int getInfoSpinErrorControl()
{
   return(ntohl(sharedVnmrInfo->spinErrorControl));
}

int setInfoInsertEjectExpControl(int val)
{
   sharedVnmrInfo->insertEjectExpControl = htonl(val);
   return(0);
}
int getInfoInsertEjectExpControl()
{
   return(ntohl(sharedVnmrInfo->insertEjectExpControl));
}

/* Temperature control */

int setInfoTempOnOff(int val)
{
   sharedVnmrInfo->tempOnOff = htonl(val);
   return(0);
}
int getInfoTempOnOff()
{
   return(ntohl(sharedVnmrInfo->tempOnOff));
}

int setInfoTempSetPoint(int val)
{
   sharedVnmrInfo->tempSetPoint = htonl(val);
   return(0);
}
int getInfoTempSetPoint()
{
   return(ntohl(sharedVnmrInfo->tempSetPoint));
}

int setInfoTempExpControl(int val)
{
   sharedVnmrInfo->tempExpControl = htonl(val);
   return(0);
}
int getInfoTempExpControl()
{
   return(ntohl(sharedVnmrInfo->tempExpControl));
}

int setInfoTempErrorControl(int val)
{
   sharedVnmrInfo->tempErrorControl = htonl(val);
   return(0);
}
int getInfoTempErrorControl()
{
   return(ntohl(sharedVnmrInfo->tempErrorControl));
}

#define NAME_LENGTH 32
#define VAL_LENGTH 128

/*******************/
struct _magicVar
/*******************/
/* Each file block contains the following header        */
{
   double   max;
   double   min;
   double   step;
   varInfo  *addr;
   int   type;
   int   active;
   int   arraySize;
   int   dgroup;
   char  val[VAL_LENGTH];
   char  vName[NAME_LENGTH];
};

static struct _magicVar *sharedTclInfo;
static int    allocMagicVar = 0;
static int    numMagicVar = 0;
static MFILE_ID magicHdl = NULL;
static char   TclInfoFile[256];
static char   *saveBuf = NULL;

void getTclInfoFileName(char tmp[], char addr[])
{
   char *ptr;

   sprintf(tmp,"/tmp/%s",addr);
   ptr = &tmp[0];
   while (*ptr != '\0')
   {
      if (*ptr == ' ')
         *ptr = '_';
      ptr++;
   }
}

int closeTclInfo()
{
   if (magicHdl != NULL)
      mClose(magicHdl);
   magicHdl = NULL;
   if (saveBuf != NULL)
      free(saveBuf);
   saveBuf = NULL;
   allocMagicVar = 0;
   return(0);
}

void exitTclInfo()
{
   closeTclInfo();
   unlink(TclInfoFile);
}

int openTclInfo(char *filename, int num)
{
   if ((num > allocMagicVar) || (magicHdl == NULL) )
   {
      closeTclInfo();
      magicHdl = mOpen(filename,sizeof(struct _magicVar) * num,
                       O_RDWR | O_CREAT);
      strcpy(TclInfoFile,filename);
      magicHdl->newByteLen = magicHdl->mapLen;
      sharedTclInfo = (struct _magicVar *) magicHdl->offsetAddr;
      allocMagicVar = num;
      saveBuf = (char *) malloc( sizeof(struct _magicVar) * num );
   }
   numMagicVar = num;
   return(0);
}

int readTclInfo(char *filename, int num)
{
   if ((num > allocMagicVar) || (magicHdl == NULL) )
   {
      closeTclInfo();
      magicHdl = mOpen(filename,sizeof(struct _magicVar) * num,
                       O_RDONLY );
      if (magicHdl == NULL)
         return(-1);
      magicHdl->newByteLen = magicHdl->mapLen;
      sharedTclInfo = (struct _magicVar *) magicHdl->offsetAddr;
      allocMagicVar = num;
   }
   numMagicVar = num;
   return(0);
}

char *getMagicVarAttr(int index, int *type, int *active, int *size,
                      int *dgroup, double *max, double *min, double *step)
{
   struct _magicVar *ptr;

   ptr = sharedTclInfo + index;
   *type = ptr->type;
   *active = ptr->active;
   *size = ptr->arraySize;
   *dgroup = ptr->dgroup;
   *max = ptr->max;
   *min = ptr->min;
   *step = ptr->step;
   return(ptr->val);
}

char *varVal(int index)
{
   struct _magicVar *ptr;
   ptr = sharedTclInfo + index;
/*
printf("varVal for index %d: shared= 0x%x offset= 0x%x val= 0x%x \n",
        index, sharedTclInfo, ptr, ptr->val);
 */
   return(ptr->val);
}

void setMagicVar(int index, varInfo *v, char *name)
{
   struct _magicVar *ptr;

   if (sharedTclInfo == NULL)
    return;
   ptr = sharedTclInfo + index;
   ptr->addr = v;
   ptr->val[0] = '\0';
   if (name != NULL)
   {
      strncpy(ptr->vName,name,NAME_LENGTH-1);   
      ptr->vName[NAME_LENGTH-2] = '\0';
   }
   if (v)
   {
      if (v->T.basicType == T_STRING)
      {
         if (v->R)	
         {
            strncpy(ptr->val,v->R->v.s,VAL_LENGTH-1);   
            ptr->val[VAL_LENGTH-2] = '\0';
         }
      }
      else
      {
         if (v->R)	
           sprintf(ptr->val,"%g",v->R->v.r);
      }
      ptr->type = v->subtype;
      ptr->active = v->active;
      ptr->arraySize = v->T.size;
      ptr->dgroup = v->Dgroup;
      ptr->max = v->maxVal;
      ptr->min = v->minVal;
      ptr->step = v->step;
   }
   else
   {
      ptr->type = 0;
      ptr->active = 0;
      ptr->arraySize = 0;
      ptr->dgroup = 0;
      ptr->max = 0.0;
      ptr->min = 0.0;
      ptr->step = 0.0;
   }
}

int updateMagicVar()
{
   register struct _magicVar *ptr;
   register int index;
   if (numMagicVar == 0)
      return(0);
   memcpy(saveBuf,(char *) sharedTclInfo,sizeof(struct _magicVar) * numMagicVar);
   for (index= 0; index < numMagicVar; index++)
   {
      ptr = sharedTclInfo + index;
      if (ptr->addr)
         setMagicVar(index, ptr->addr, NULL);
   }
   return(memcmp(saveBuf,(char *) sharedTclInfo,sizeof(struct _magicVar) * numMagicVar));
}

void unsetMagicVar(varInfo *v)
{
   register struct _magicVar *ptr;
   register int index;

   for (index= 0; index < numMagicVar; index++)
   {
      ptr = sharedTclInfo + index;
      if (v == ptr->addr)
      {
         ptr->addr = 0;
         return;
      }
   }
}

void resetMagicVar(varInfo *v, char *name)
{
   register struct _magicVar *ptr;
   register int index;

   for (index= 0; index < numMagicVar; index++)
   {
      ptr = sharedTclInfo + index;
      if ((ptr->addr == 0) && !strcmp(ptr->vName,name) )
      {
         setMagicVar(index, v, NULL);
         return;
      }
   }
}


#ifndef NESSIE
void closeSocket(char *dummy)
{
     fprintf(stderr,"closeSocket: Wrong libacqcomm shared library being used...\n");
}
void readSocket(char *pSocket, char *datap, int bcount )
{
     fprintf(stderr,"readSocket: Wrong libacqcomm shared library being used...\n");
}
void readProtectedSocket(char *pSocket, char *datap, int bcount )
{
     fprintf(stderr,"readProtectedSocket: Wrong libacqcomm shared library being used...\n");
}
void openSocket(char *dummy)
{
     fprintf(stderr,"openSocket: Wrong libacqcomm shared library being used...\n");
}
void createSocket(int type)
{
     fprintf(stderr,"createSocket: Wrong libacqcomm shared library being used...\n");
}
void connectSocket(char *pSocket, char *hostName, int portAddr )
{
     fprintf(stderr,"connectSocket: Wrong libacqcomm shared library being used...\n");
}
void writeSocket(char *pSocket, const char *datap, int bcount )
{
     fprintf(stderr,"writeSocket: Wrong libacqcomm shared library being used...\n");
}
#endif
