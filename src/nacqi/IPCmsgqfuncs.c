/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*  Replacement for IPCsockfuncs.c, except Message Queues are employed     */
/*  CLASSIC C   CLASSIC C   CLASSIC C   CLASSIC C   CLASSIC C   CLASSIC C  */
/*  March 1998: The name of the file refers to SVR4 IPC Message		   */
/*  Queues, but the programs now use BSD_style sockets exclusively.  	   */


#include <stdio.h>
#include <pwd.h>
#include <signal.h>

#include "acquisition.h"
#include "group.h"

#include "interactextern.h"

#ifndef HOSTLEN
#define HOSTLEN  128
#endif

#ifndef MAXPATHL
#define MAXPATHL  128
#endif

#define ERROR		1

extern int	canvas_open;

static char	userName[ HOSTLEN ] = { '\0' };
static char	hostName[ HOSTLEN ] = { '\0' };
static char	procName[ HOSTLEN ] = { '\0' };
static char	processIDstr[ 12 ]  = { '\0' };
static int	ulen;
static long	LastInputTime = 0L; /* Last Time of User Interactive Input */
static int	connected = 0;


initIPCinfo( programName )
char *programName;
{
	int		 limitIndex, processID;
	struct passwd	*getpwuid();
	struct passwd	*pasinfo;
 
    /* --- get user's name --- */
    /*        get the password file information for user */

	Procpid = getpid();

	limitIndex = sizeof( userName ) - 1;
	pasinfo = getpwuid((int) getuid());
	if (strlen( pasinfo->pw_name ) > limitIndex) {
		strncpy( &userName[ 0 ], pasinfo->pw_name, limitIndex );
		userName[ limitIndex ] = '\0';
	}
	else
          strcat(&userName[ 0 ], pasinfo->pw_name);
	ulen = strlen( &userName[ 0 ] );

	limitIndex = sizeof( hostName ) - 1;
	gethostname( &hostName[ 0 ], limitIndex );
	hostName[ limitIndex ] = '\0';

	strcpy( &procName[ 0 ], "acqi" );

	processID = getpid();
	sprintf( &processIDstr[ 0 ], "%d", processID );

	getAcqProcParam( "acqi" );
}

/*  Important new feature for inova (new digital console):

    You may connect ACQI to the console while an acquisition is in progress.
    Of course you can only adjust shims and cannot run the lock display.

    If an acquisition is in progress then Expproc replies ACQUIRING.  We
    pass a flag to set_win_connect_state (called from acqconfirmer, below)
    so the GUI software will know what screens and buttons to display.

    It turns out you MUST NOT access the FIFO while an acquisition is in
    progress.  Thus you can't set the lock time constant.  Normally ACQI
    sets this on connect and on disconnect.

    07/14/1995									*/


acqconnect()
{
	int	ival;
	int	ok2acquire;
	char	NDCcommand[ 122 ], expprocReply[ 256 ];

	ok2acquire = 131071;	/* it's OK to acquire unless informed otherwise */
	show_panel_item(CONbutton, False);
#ifndef STANDALONE
	verifyExpproc();
	strcpy( &NDCcommand[ 0 ], "shim" );
	insertAuth( &NDCcommand[ 0 ], sizeof( NDCcommand ) );
	ival = talk2Acq4Acqi(
	   "reserveConsole", &NDCcommand[ 0 ], &expprocReply[ 0 ], sizeof( expprocReply )
	);
	if (ival < 0) {
		show_panel_item(CONbutton, True);
		return;
	}

/*  If an acquisition is in progress, the Expproc may send back
    the name of the Experiment Information File, to assist FID
    monitor.  Use strncmp to avoid confusion.			*/

	else if (strncmp( &expprocReply[ 0 ], "ACQUIRING", strlen( "ACQUIRING" ) ) == 0) {
		ok2acquire = 0;
	}
	else if (strcmp( &expprocReply[ 0 ], "OK") != 0) {
		show_panel_item(CONbutton, True);
	        if (strcmp( &expprocReply[ 0 ], "OK2") == 0) {  /* does an su need to be done ? */
			char	errmsg[100];

			sprintf(errmsg,"write('error','run su or go before using acqi')\n");
			sendasync_vnmr(errmsg);

		/* release access to the console, since Expproc reserves access
		   even if an su has not been done.  After all, the request for
		   access could be so an su can be performed.			*/

			strcpy( &NDCcommand[ 0 ], "shim" );
			insertAuth( &NDCcommand[ 0 ], sizeof( NDCcommand ) );
			ival = talk2Acq4Acqi(
	   "releaseConsole", &NDCcommand[ 0 ], &expprocReply[ 0 ], sizeof( expprocReply )
			);
		}
		return;
	}

	if (ok2acquire) {
		ival = setAcqiTimeConst();
		ival = setAcqiStatus();
	}
	ival = setAcqiInterval();			/*maybe move to confirmer?? */
	if (ival < 0) {
		show_panel_item(CONbutton, True);
		return;
	}
#endif /* not STANDALONE */
	acqconfirmer( ok2acquire );
}

/*  Replacement for confirmer - eventually this will replace the stub-trap below */

acqconfirmer( ok2acquire )
int	ok2acquire;
{
	char	systemdir[MAXPATHL];
	char	buff2[256];

#ifndef STANDALONE
    /* Arrange access to shared memory status block */
    /* access is released at ACQI disconnect        */

	initExpStatus( 0 );
#endif
    /* Set ACQI window to connect stat, with SHIM, LOCK, etc. buttons  */

	set_win_connect_state( ok2acquire );

#ifndef STANDALONE
	if (P_getstring(GLOBAL,"systemdir",systemdir,1,MAXPATHL))
	  strcpy(systemdir,"/vnmr");
	sprintf(buff2,"(umask 0; cat /dev/null > %s/acqqueue/acqi_%d)\n",
            systemdir,Procpid);
	system(buff2);
	kill(getppid(),SIGUSR2);  /* signal vnmr that acqi is connected */
	connected = 131071;
#endif
}

/*  reserve the new digital console for some kind of operation,
    as specified by the argument.  At this time the argument is
    ignored and the operation is always ACQUIRE.		*/

#define  MAX_RESERVE_CONSOLE_TRIES	3

int
reserveConsole( level )
char *level;
{
	int	iter, ival;
	char	NDCcommand[ 122 ], expprocReply[ 256 ];

	strcpy( &NDCcommand[ 0 ], "acquire" );
	insertAuth( &NDCcommand[ 0 ], sizeof( NDCcommand ) );

	for (iter = 0; iter < MAX_RESERVE_CONSOLE_TRIES; iter++) {
		ival = talk2Acq4Acqi(
	   "reserveConsole", &NDCcommand[ 0 ], &expprocReply[ 0 ], sizeof( expprocReply )
		);
		if (ival != 0)
		  return( -1 );

		if (strcmp( &expprocReply[ 0 ], "BUSY") == 0) {
			sleep( 1 );
			continue;
		}

	/*  Here is the successful return.  */

		else if (strcmp( &expprocReply[ 0 ], "OK") == 0)
		  return( 0 );
		else
		  return( -1 );
	}

/* If it never gets unbusy, then it didn't work  */

	return( -1 );
}

/*  See the comment at acqconnect concerning running ACQI during an acquisition
    Here we get from the GUI software whether a separate acquisition is in
    progress.  Do not set the lock time constant if this is so.			*/

disconnect()
{
	int	ival;
	char	NDCcommand[ 122 ], expprocReply[ 256 ], buff2[ 256 ],
		systemdir[ MAXPATHL ];

	inittimer( 0.0, 0.0, NULL );
#ifndef STANDALONE
	SendSHvals();
#endif /* (not) STANDALONE */
	show_panel_item(CONbutton, False);

    /* --- reset LastInputTime to none active (i.e., 0) --- */
	LastInputTime = 0L;

	if (canvas_open==1) LKquit();
	if (canvas_open==2) SHquit();
	if (canvas_open==3) FIDquit();
	if (canvas_open==4) FSHquit();

#ifndef STANDALONE

/*  exitproc() calls disconnect() - but ACQI may already have disconnected.
    Why does exitproc call disconnect?  So that Vnmr can direct ACQI to
    exit, even if it is connected.  However calling setDefaultStatus after
    ACQI has diconnected can cause problems, since the console may be in
    some different state when ACQI exits.  So we remember when ACQI
    connects and disconnects.

    When releasing access to the console, only release shim access.  The
    Expproc program (this is an INOVA-only source file) will release acquire
    access when the interactive acquisition completes.  See resetState in
    msgehandler.c, SCCS category Expproc.    April 23, 1997			*/
    
	if (connected) {
		if (can_acqi_acquire()) {
			ival = setDefaultTimeConst();
			ival = setDefaultStatus();
		}
		ival = setDefaultInterval();
		strcpy( &NDCcommand[ 0 ], "shim" );
		insertAuth( &NDCcommand[ 0 ], sizeof( NDCcommand ) );
		ival = talk2Acq4Acqi(
		   "releaseConsole", &NDCcommand[ 0 ], &expprocReply[ 0 ], sizeof( expprocReply )
		);
		connected = 0;
	}

/*  Release access to shared memory status block */

	expStatusRelease();
#endif /* (not) STANDALONE */

    /* --- remove the options unavailiable after Disconnect --- */

	show_panel_item(LKbutton, False);
	show_panel_item(SHbutton, False);
	show_panel_item(FIDbutton, False);
	show_panel_item(SIZEbutton, False);
	show_panel_item(quitbutton, True);

	set_win_disconnect_state();

	if (P_getstring(GLOBAL,"systemdir",systemdir,1,MAXPATHL))
	  strcpy(systemdir,"/vnmr");
	sprintf(buff2,"%s/acqqueue/acqi_%d",systemdir,Procpid);
	unlink(buff2);

	sendMessageAtDisconnect();	/* see interactscrn.c */

#ifndef STANDALONE
	kill(getppid(),SIGUSR2);  /* signal vnmr that acqi is disconnected */
#endif /* (not) STANDALONE */
}

readsocket()
{
	fprintf( stderr, "read socket program called, this process exits\n" );
	exit( 1 );
}

confirmer()
{
	fprintf( stderr, "confirmer program called, this process exits\n" );
	exit( 1 );
}

/*------------------------------------------------------------
|
|       Send a line to be displayed in the Vnmr error window.
|
+-----------------------------------------------------------*/
warning(msg)
  char *msg;			/* The message to be sent */
{
#ifndef STANDALONE
    char buf[1024];

    sprintf(buf,"write('error','%s')\n", msg);
    sendasync_vnmr(buf);
#else /* STANDALONE */
    fprintf(stderr,"VNMR WARNING: %s\n", msg);
#endif /* STANDALONE */
}

/*------------------------------------------------------------
|
|    sendasync()/4
|       connect to  an Async Process's Socket
|       then transmit a message to it and disconnect.
|
+-----------------------------------------------------------*/
sendasync_vnmr(message)
char *message;
{
    char vnmrMsgQ[255];
    int ival;

    if (P_getstring(GLOBAL, "vnmraddr", vnmrMsgQ, 1, sizeof( vnmrMsgQ ))) {
       fprintf(stderr,"VNMR address missing\n");
        return(ERROR);
    }

    initVnmrComm(vnmrMsgQ);
    sendToVnmr(message);

/*  ival = deliverMessage4Acqi( vnmrMsgQ, message ); */

    return( ival );
}

#define  DELIMITER_2	'\n'
#define  DELIMITER_3	','

static char *
buildAuthParam()
{
	char	*retaddr;
	int	 plen;

	if (userName[ 0 ] == '\0' || hostName[ 0 ] == '\0') {
		initIPCinfo( "acqi" );
	}
	plen = strlen( &userName[ 0 ] ) +
	       strlen( &hostName[ 0 ] ) +
	       strlen( &processIDstr[ 0 ] ) +
	       2 + 1;

	retaddr = (char *) allocateWithId( plen, "ipc" );
	if (retaddr == NULL)
	  return( NULL );

	strcpy( retaddr, &userName[ 0 ] );
	append_char4Acqi( retaddr, DELIMITER_3 );
	strcat( retaddr, &hostName[ 0 ] );
	append_char4Acqi( retaddr, DELIMITER_3 );
	strcat( retaddr, &processIDstr[ 0 ] );

	return( retaddr );
}

int
insertAuth( msg_for_acq, mfa_len )
char *msg_for_acq;
int mfa_len;
{
	int	 authLen, iter, mfa_current_len;
	char	*authInfo, *tptrstart, *tptrend;

	mfa_current_len = strlen( msg_for_acq );
	authInfo = buildAuthParam();
	authLen = strlen( authInfo );

	if (mfa_current_len + authLen + 1 >= mfa_len) {
		release( authInfo );
		return( -1 );
	}

	tptrstart = msg_for_acq + mfa_current_len;
	tptrend = msg_for_acq + mfa_current_len + authLen + 1;
	for (iter = 0; iter < mfa_current_len+1; iter++)
	  *(tptrend--) = *(tptrstart--);

	strncpy( msg_for_acq, authInfo, authLen );
	msg_for_acq[ authLen ] = DELIMITER_2;

	release( authInfo );
	return( 0 );
}


/*  Please consider the values returned by these programs
    to be const values and do not alter them.			*/

char *
ipcGetUserName()
{
	if (userName[ 0 ] == '\0')
	  initIPCinfo( "acqi" );

	return( &userName[ 0 ] );
}

char *
ipcGetHostName()
{
	if (hostName[ 0 ] == '\0')
	  initIPCinfo( "acqi" );

	return( &hostName[ 0 ] );
}

char *
ipcGetProcName()
{
	if (procName[ 0 ] == '\0')
	  initIPCinfo( "acqi" );

	return( &procName[ 0 ] );
}

