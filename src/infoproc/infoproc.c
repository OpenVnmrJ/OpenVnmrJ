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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/resource.h>

#include <signal.h>
#include <netinet/in.h>
#include <netdb.h>
#include <setjmp.h>


/*-----------------------------------------------------------------------
|     GLobal definitions
+-----------------------------------------------------------------------*/


int messocket;	/* message process socket descriptor */
struct sockaddr_in    messname;

#ifdef DEBUG
int Acqdebug = 1;			/* debug flag */
#else
int Acqdebug = 0;			/* debug flag */
#endif

char LocalAcqHost[256];	/* AcqProc's Host machine Name */
int  LocalAcqPid;	/* Acqproc's PID */
char vnmrsystem[128];	/* vnmrsystem path */


/*  It turns out on Solaris that the gethostbyname program does not work
    when called from an interrupt.  The reason is believed to be internal
    static data structures in the gethostbyname program.  In any case the
    Acqproc program sometimes crashes in sendasync() at the call to
    gethostbyname.  To work around this, we call gethostbyname once and
    then export the returned address to the socket function programs.
    The address returned by gethostbyname is kept here.			*/
 
struct hostent	*this_hp;
static int initsocket();
static void setup_sigio();
 
/*-----------------------------------------------------------------------
|
|    Main Acqproc Loop, wait for messages
|
+-----------------------------------------------------------------------*/
main(argc,argv)
int argc;
char *argv[];
{
    char *tmpptr;
    int  ival;
    extern char  *getenv();


    if (argc > 1)
    {
	ival =  1;
	while (ival < argc)
	{
	    if (strcmp(argv[ival], "-debug") == 0)
	    {
		Acqdebug = 1;
		break;
	    }
	    ival++;
	}
    }
    if (!Acqdebug)
    {
     	freopen("/dev/null","r",stdin);
     	freopen("/dev/console","a",stdout);
     	freopen("/dev/console","a",stderr);
    }
		
    /* initialize environment parameter vnmrsystem value */
    tmpptr = getenv("vnmrsystem");            /* vnmrsystem */
    if (tmpptr != (char *) 0)
    {
	strcpy(vnmrsystem,tmpptr);	/* copy value into global */
    }
    else
    {
	strcpy(vnmrsystem,"/vnmr");	/* use /vnmr as default value */
    }

    /* initialize acq process Status Update DataGram socket & registery queue*/
    initregqueue();

    initsocket();

    setup_sigio();

    setuppipehandler();

    setupquithandler();

    /* write out IPC information for other process to access */
    wrtacqinfo();

    initinfo();

    this_hp = gethostbyname(LocalAcqHost);      /* see note at definition of this_hp */

#ifdef USE_RPC
    acqinfo_svc();
#endif
}


/*-----------------------------------------------------------------------
|	wrtacqinfo()/0
|	write the acquisitions pid, and socket port numbers out for
|	  access by other processes
+-----------------------------------------------------------------------*/
wrtacqinfo()
{
    char filepath[256];
    char buf[256];
    int fd;
    int bytes;
    int pid;

    LocalAcqPid = pid = getpid();
    /* get Host machine name */
    gethostname(LocalAcqHost,sizeof(LocalAcqHost));

    if (Acqdebug)
        fprintf(stderr," msge ports = %d\n", messname.sin_port);
    sprintf(buf,"%d %s %d %d %d",pid,LocalAcqHost, -9, -9, messname.sin_port);

    strcpy(filepath,vnmrsystem);	/* path to acqinfo */
    strcat(filepath,"/acqqueue/acqinfo");
    if ( (fd = open(filepath,O_WRONLY | O_CREAT | O_TRUNC,0666)) == -1)
    {
        fprintf(stderr,"Could Not Open Acquisition Info File: '%s'\n",
            filepath);
        exit(1);
    }
    bytes = write(fd,buf,strlen(buf)+1);
    if ( (bytes == -1) )
    {
        fprintf(stderr,"Could Not Write Acquisition Info File: '%s'\n",
            filepath);
        exit(1);
    }
    close(fd);
}



/*  This program was moved from socketfuncs.c  */

/*-------------------------------------------------------------------
|
|   initsockets()/0
|   initialize the inter-process communications sockets
|	1. create a socket.
|	2. Bind a name to this socket so others may connect to it
|	3. Make socket non-blocking (i.e., accept returns if no
|		connection instead of waiting for one )
|
+-------------------------------------------------------------------*/
static int initsocket()
{
    socklen_t namlen;


/*===================================================================*/
    /* --- create the asyncronous message socket for Acqstat --- */
/*===================================================================*/

    messocket = make_a_socket();	/* create a socket */
    if (messocket == -1)
    {
	perror("INITSOCKET(): socket error");
        exit(1);
    }
    setup_a_socket( messocket );
    render_socket_async( messocket );

    if (Acqdebug)
      fprintf(stderr,
	"INITSOCKET(): async sockets: %d  created\n", messocket);
    /* --- bind a name to the socket so that others may connect to it --- */

    /* messname.sin_port = IPPORT_RESERVED + 4; */
    memset( &messname, 0, sizeof( messname ) );
    messname.sin_family = AF_INET;
    messname.sin_port = htons(0);
    messname.sin_addr.s_addr = INADDR_ANY;
    /* name socket */
    if (bind(messocket,(struct sockaddr *)&messname,sizeof(messname)) != 0)
    {
	perror("INITSOCKET(): messocket bind error");
        exit(1);
    }

    listen(messocket,5);	/* set up listening queue ?? */

    if (Acqdebug)
    {
      fprintf(stderr,"INITSOCKET(): bind:worked\n");
      fprintf(stderr,"INITSOCKET(): listen:queue set up async \n");
    }

    /* retrieve system given names and ports for sockets */

    namlen = sizeof(messname);
    getsockname(messocket,(struct sockaddr *)&messname,&namlen);
}



static
make_fd_rmask( maxfd_ptr, readfdp )
int *maxfd_ptr;
fd_set *readfdp;
{
	*maxfd_ptr = 0;
	FD_ZERO( readfdp );

	FD_SET( messocket, readfdp );
	if (messocket > *maxfd_ptr)
	  *maxfd_ptr = messocket;

#ifdef GEMPLUS
        add_async_sockets( maxfd_ptr, readfdp );
#endif
	return( 0 );
}

static void
sigio_irpt()
{
	int			maxfd, nfound;
	fd_set			readfd;
	struct timeval		nowait;

/*  You have to tell select() not to wait; with an address of NULL, it
    waits until there is activity on one of the selected file descriptors.  */

	nowait.tv_sec = 0;
	nowait.tv_usec = 0;

	do {
		make_fd_rmask( &maxfd, &readfd );
		maxfd++;

		nfound = select( maxfd, &readfd, NULL, NULL, &nowait );
		if (nfound < 0) {
			if (errno != EINTR)
			  perror( "select error" );
		}
		else {
			if (FD_ISSET( messocket, &readfd ))
			  Smessage();

#ifdef GEMPLUS
			check_async_sockets( &readfd );
#endif
		}
	} while (nfound != 0);

	setup_sigio();
}

static void setup_sigio()
{
    struct sigaction	intserv;
    sigset_t		qmask;
    void		sigio_irpt();
 
    /* --- set up signal handler --- */

    sigemptyset( &qmask );
    sigaddset( &qmask, SIGALRM );
    sigaddset( &qmask, SIGCHLD );
    sigaddset( &qmask, SIGIO );
    intserv.sa_handler = sigio_irpt;
    intserv.sa_mask = qmask;
    intserv.sa_flags = 0;

    sigaction( SIGIO, &intserv, NULL );
}

static void
dontdie()
{
    if (Acqdebug)
        fprintf(stderr,"DONTDIE(): write to a closed pipe occured\n");
}
/*-------------------------------------------------------------------------
|
|   Setup the interrupt handler for the asyn messages on the message socket
|
+--------------------------------------------------------------------------*/
setuppipehandler()
{
    sigset_t            qmask;
    struct sigaction    intpipe;

    sigemptyset( &qmask );
    sigaddset( &qmask, SIGPIPE );

/* --- set up sigpipe handler write to a closed pipe --- */

    intpipe.sa_handler = dontdie;
    intpipe.sa_mask = qmask;
    intpipe.sa_flags = 0;
    sigaction(SIGPIPE,&intpipe,0L);
}



/*-------------------------------------------------------------------------
|
|   Setup the exception handlers for fatal type errors 
|    
+--------------------------------------------------------------------------*/

static void
terminated()
{
#ifdef USE_RPC
    close_rpc();
#endif
    exit(1);
}

static void
SigQuit()
{
#ifdef USE_RPC
    close_rpc();
#endif
    exit(1);
}

setupquithandler()
{
    sigset_t		qmask;
    struct sigaction	intquit;
    struct sigaction	segquit;

    /* --- set up interrupt handler --- */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGINT );
    intquit.sa_handler = terminated;
    intquit.sa_mask = qmask;
    intquit.sa_flags = 0;
    sigaction(SIGINT,&intquit,0L);

    /* --- set up interrupt handler --- */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGTERM );
    intquit.sa_handler = terminated;
    intquit.sa_mask = qmask;
    intquit.sa_flags = 0;
    sigaction(SIGTERM,&intquit,0L);

    /* --- set up quit signal exception handler --- */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGQUIT );
    segquit.sa_handler = SigQuit;
    segquit.sa_mask = qmask;
    sigaction(SIGQUIT,&segquit,0L);
}


/*-------------------------------------------------------------------------
|
|   block / unblock those signals caught on a regular or routine basis
|   Prevents occasional failure of the "showstat" program, caused by the
|   read call in infosockets returning prematurely because a signal was
|   caught.
|
|   You could also prevent this by requesting that any pending system
|   called by restarted.  However this must be done for each signal.
|   See GNETfuncs.c in SCCS category gacqproc           February 1997
+--------------------------------------------------------------------------*/

block_signals()
{
	sigset_t	sigblock_mask;

	sigemptyset( &sigblock_mask );
	sigaddset( &sigblock_mask, SIGALRM );
	sigaddset( &sigblock_mask, SIGCHLD );
	sigaddset( &sigblock_mask, SIGIO );
	sigaddset( &sigblock_mask, SIGUSR2 );
	sigprocmask( SIG_BLOCK, &sigblock_mask, NULL );
}

unblock_signals()
{
	sigset_t	sigblock_mask;

	sigemptyset( &sigblock_mask );
	sigaddset( &sigblock_mask, SIGALRM );
	sigaddset( &sigblock_mask, SIGCHLD );
	sigaddset( &sigblock_mask, SIGIO );
	sigaddset( &sigblock_mask, SIGUSR2 );
	sigprocmask( SIG_UNBLOCK, &sigblock_mask, NULL );
}
