/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*  March 1998: The name of the file refers to SVR4 IPC Message
    Queues, but the programs now use BSD_style sockets exclusively.  */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/socket.h>

#include "acqInterface.h"

#include "vnmrsys.h"
#include "comm.h"
#include "group.h"

/*  These defines were taken from acquisition.h, SCCS category vnmr  */

#define ACQ_COMM_ID   0
#define VNMR_COMM_ID  1
#define LOCAL_COMM_ID 2

#define INITIALIZE      1
#define CONFIRM         2
#define ADDRESS         3
#define ADDRESS_VNMR    4
#define ADDRESS_ACQ     5
#define VNMR_CONFIRM    6

#define RET_OK     0
#define RET_ERROR -1

#define READ_ERROR 10
#define OPEN_ERROR 20
#define BIND_ERROR 30
#define CONNECT_ERROR 40
#define SELECT_ERROR 50
#define ACCEPT_ERROR 60
#define FCNTL_READ_ERROR 70
#define FCNTL_WRITE_ERROR 80
#define MSG_LEN_ERROR 90

/*  End of defines taken from acquisition.h, SCCS category vnmr  */

#ifndef MAXPATRL
#define MAXPATHL 128		/* maximum path length in vnmr environment */
#endif 

static char  acqInterface[ MAXPATHL ] = { '\0' };
static char  programName[ MAXPATHL ] = { '\0' };

extern int   orig_euid;
extern int   orig_uid;
extern int   acq_errno;

extern COMM_INFO_STRUCT comm_addr[];
extern char *ipcGetHostName();
extern int getascii(int sfd, char buffer[], int bufsize );
extern int prepare_reply_socket(int connsid );
extern int wait_for_select(int fd, int wait_interval );
extern int set_comm_port(CommPort ptr);
extern int SendAsyncInova(CommPort to_addr, CommPort from_addr, char *msg);
extern int InitRecverAddr(char *host, int port, struct sockaddr_in *recver);
extern int get_comm(int index, int attr, char *val);
extern int set_comm(int index, int attr, char *val);
extern void init_acq(char *val);

/*------------------------------------------------------------------------
|	getAcqProcParam/0
+--------------------------------------------------------------------------*/
int getAcqProcParam( char *thisProgramName )
{
	int	plen;

	/*strcpy( &acqInterface[ 0 ], "Expproc" );*/
	init_acq( systemdir );
	get_comm( ACQ_COMM_ID, ADDRESS_ACQ, &acqInterface[ 0 ] );

	if (thisProgramName != NULL) {
		plen = strlen( thisProgramName );
		if (plen+1 <= sizeof( programName ))
		  strcpy( &programName[ 0 ], thisProgramName );
		else {
			strncpy(
			    &programName[ 0 ], thisProgramName, sizeof( programName ) - 1
			);
			programName[ sizeof( programName ) -1 ] = '\0';
		}
	}
	else
	  strcpy( &programName[ 0 ], "noName" );

	return( 1 );
}

void verifyExpproc()
{
	get_comm( ACQ_COMM_ID, CONFIRM, ipcGetHostName());
	get_comm( ACQ_COMM_ID, ADDRESS_ACQ, &acqInterface[ 0 ] );
}

/* changed name so that not confused with shared libacqcomm version */

char *
append_char4Acqi(char *string, char character)
{
	int	len;

	if (string == NULL)
	  return( NULL );

	len = strlen( string );
	*(string + len) = character;
	*(string + len + 1) = '\0';

	return( string );
}

/*  Use the local communication port - works with either
    the VNMR or the acquisition interface.		*/

int
deliverMessage4Acqi( char *interface, char *message )
{
        int		ival, mlen;
        CommPort tmp_from = &comm_addr[LOCAL_COMM_ID];

        if (message == NULL)
          return( -1 );
        mlen = strlen( message );
        if (mlen < 1)
          return( -1 );

	set_comm( LOCAL_COMM_ID, ADDRESS, interface );
   	ival = InitRecverAddr(tmp_from->host,tmp_from->port,&(tmp_from->messname));
	if (ival != RET_OK)
	  return(RET_ERROR);
	ival = SendAsyncInova( tmp_from, NULL, message );
        return( ival );
}


/*  socket.c (VNMR) has a very similar program (with the same name),
    but with a crucial difference.  The VNMR version is based on the
    old Acqproc/UnityPLUS system.  In it the Acqproc received commands
    as numbers.  For NESSIE (INOVA) these numbers were changed to
    words.  The program in socket.c makes the translation from numbers
    to words.  The ACQI programs were written for INOVA and so they
    receive the words directly and do not need to translate numbers to
    words.

    The VNMR version also includes the host name as an argument; the
    ACQI version has to locate the host name on its own.

    The ACQI version completes the interaction with Expproc; the
    version in socket.c only proceeds to the point where this process
    has accepted the connection from Expproc.				*/

int
talk2Acq4Acqi( char *cmd_for_acq, char *msg_for_acq, char *msg_for_vnmr, int mfv_len )
{
	int		 connsid, ival, mlen;
	char		*maddr;
	CommPort	 acq_addr  = &comm_addr[ACQ_COMM_ID];
        CommPort	 tmp_from =  &comm_addr[LOCAL_COMM_ID];

        tmp_from->path[0] = '\0';
        tmp_from->pid = getpid();
        strcpy(tmp_from->host,ipcGetHostName());
        set_comm_port(tmp_from);	/* this call creates a socket */

	mlen = strlen( cmd_for_acq ) +
	       strlen( tmp_from->path ) +
	       strlen( msg_for_acq ) + 2;
	maddr = malloc( mlen+1 );
	if (maddr == NULL) {
		errno = ENOMEM;
		fprintf( stderr, "talk2Acq4Acqi - malloc for maddr failed \n" );
		return( -1 );
	}

	strcpy( maddr, cmd_for_acq );
	append_char4Acqi( maddr, DELIMITER_1 );
	strcat( maddr, tmp_from->path );
	append_char4Acqi( maddr, DELIMITER_2 );
	strcat( maddr, msg_for_acq );

	if (SendAsyncInova( acq_addr, tmp_from, maddr ) != RET_OK) {
		free( maddr );
		close( tmp_from->msgesocket );
		return( -1 );
	}

	free( maddr );

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
	  connsid = ival;

	ival = prepare_reply_socket( connsid );
	if (ival != 0) {
		return( -1 );
	}

	ival = getascii( connsid, msg_for_vnmr, mfv_len-1 );	/* room for NUL */

/* Eliminate the ^D character from the Acqproc response.	*/

	if ( (maddr = strchr( msg_for_vnmr, '\004' )) != NULL )
	  *maddr = '\0';
	close( connsid );

	return( 0 );
}

/*
    CAUTION  CAUTION  CAUTION  CAUTION  CAUTION  CAUTION  CAUTION  CAUTION

    This program doe NOT insert a Return Interface in the message it sends to Expproc  */
 
int send2Acq4Acqi(char *cmd_for_acq, char *msg_for_acq )
{
        int              ival, mlen;
        char            *maddr;
 
        if (strlen( &acqInterface[ 0 ] ) < 1)
          getAcqProcParam( "noName" );
 
        mlen = strlen( cmd_for_acq ) +
               strlen( msg_for_acq ) + 3;
        maddr = malloc( mlen+1 );
        if (maddr == NULL) {
                errno = ENOMEM;
                return( -1 );
        }
 
        strcpy( maddr, cmd_for_acq );
        append_char4Acqi( maddr, DELIMITER_1 );
        append_char4Acqi( maddr, DELIMITER_1 );
        append_char4Acqi( maddr, DELIMITER_2 );
        strcat( maddr, msg_for_acq );
 
        ival = deliverMessage4Acqi( &acqInterface[ 0 ], maddr );
	free( maddr );

        return( ival );
}
