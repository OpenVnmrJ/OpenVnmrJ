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

#define _POSIX_PTHREAD_SEMANTICS

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

#include "sockets.h"
#include "errLogLib.h"

/*-----------------------------------------------------------------------
|     GLobal definitions
+-----------------------------------------------------------------------*/


int messocket;	/* message process socket descriptor */
struct sockaddr_in    messname;

#ifdef DEBUG
int Acqdebug = 0;			/* debug flag */
#else
int Acqdebug = 0;			/* debug flag */
#endif

char LocalAcqHost[256];	/* AcqProc's Host machine Name */
int  LocalAcqPid;	/* Acqproc's PID */
char vnmrsystem[128];	/* vnmrsystem path */

char ProcName[256];
pthread_t main_threadId;

/*  It turns out on Solaris that the gethostbyname program does not work
    when called from an interrupt.  The reason is believed to be internal
    static data structures in the gethostbyname program.  In any case the
    Acqproc program sometimes crashes in sendasync() at the call to
    gethostbyname.  To work around this, we call gethostbyname once and
    then export the returned address to the socket function programs.
    The address returned by gethostbyname is kept here.			*/
 
struct hostent	*this_hp;
static int initsocket();
static void sigio_irpt();
 
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
    sigset_t   blockmask;
    extern char  *getenv();
    void asyncMainLoop(sigset_t sigMask);


    strncpy(ProcName,argv[0],256);
    ProcName[255] = '\0';

    /* set mask to block all signals, now any new threads will be spawned
       with this signal mask
    */
    sigfillset( &blockmask );
    pthread_sigmask(SIG_BLOCK,&blockmask,NULL);

    main_threadId = pthread_self();

    DebugLevel=0;

    if (argc > 1)
    {
	ival =  1;
	while (ival < argc)
	{
	    if (strcmp(argv[ival], "-debug") == 0)
	    {
		DebugLevel = Acqdebug = 1;
		break;
	    }
	    ival++;
	}
    }
    /* Acqdebug = 1; */
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
    sprintf(LocalAcqHost,"%s/acqbin/Infolog",vnmrsystem);
    if ( ! access(LocalAcqHost,F_OK) )
    {
       DebugLevel = 1;
    }

    /* initialize NDDS domain comunication with Console */
    /* got to do this before creating any threads , to avoid race conditions with NDDS */
 
    initiateNDDS(-1);   /* NDDS_Domain is set */
 
    initExpStatus(0);  /* open shared Exp Status Struct (MMAP) */

    /* initialize acq process Status Update DataGram socket & registery queue */
    initregqueue();

    /* --- create the asyncronous message socket for Acqstat --- */
#ifdef __INTERIX
    initInfoprocSocket();
    gethostname(LocalAcqHost,sizeof(LocalAcqHost));
#else
    initsocket();
#endif

    /* write out IPC information for other process to access */
    wrtacqinfo();

    initinfo();    /* set stat structure initial default values  */

    this_hp = gethostbyname(LocalAcqHost);      /* see note at definition of this_hp */

    initStatusSub();   /* start BE subscription to Status from Master */

    /* this mode abandon, 1/26/07  GMB  */
    /* initHBSubs();  start the HB subscription for active tests, change active to inactive */


    /* acqinfo_svc(); never return, need to make thread safe and call in a separate thread */

#ifndef __INTERIX    /* not used in SFU, svc / rpc functions not supported */
    start_svc_thread();
#endif

    asyncMainLoop(blockmask);  /* blockmask has all bits set, i.e. handle all signals */

}


/*****************************************************************
*
*   asyncMainLoop
*
*   INTERNAL
*
*   The application calls this program when it is ready to switch into asychronous
*   mode.  This program never returns.
*   Previous asyncMainLoop used signal handler to queue up event to be handle later in non 
*   'signal handler context'.  However the use of signals and especially signal handler is
*   a bad idea in thread programs. Thus this routine handle signal in the thread appropriate
*   method.
*
*   Signals are handle in the main thread (and only one thread) via the sigwait() call.
*   This allows the async signals to be processed synchronously. And since no other thread
*   should call sigwait (unless it's expecitely waiting a a signal all other thread are
*   uneffected since these signal are block via the pthread_sigmask() call.
*
*   The program is written as an infinite loop using the for (;;) construct.
*   When sigwait returns with the signal number the case statemnt
*   handle the processing type.
*   Other signals cause coredump are still handled by signal handler but this is OK
*       since these signal are always fatal and cause program termination
*
*/
void asyncMainLoop(sigset_t sigMask)
{
      sigset_t          oldMask;
      int stat;
      int signo;
      void processMsge(void*);
      void sigio_irpt(void);
      void terminated(void);

         pthread_sigmask(SIG_BLOCK,&sigMask,&oldMask);
         for( ; ; )
         {
            stat = sigwait(&sigMask, &signo);
            switch(signo)
            {
               case SIGIO:    /* async socket IO */
                    /* DPRINT(-1,"Infoproc: Received SIGIO\n"); */
#ifdef __INTERIX
                    processInfoSock();
#else
                    sigio_irpt();
#endif
                    break;
 
               case SIGUSR1:
                    /* DPRINT(-1,"Infoproc: Received SIGUSR1, processMsge() \n"); */
                    break;
 
               case SIGUSR2:  /* NDDS console stat has changed */
                    /* DPRINT(-1,"Infoproc: Received SIGUSR2, call Statuscheck().\n"); */
                    Statuscheck();
		    /* Acqstate = update_statinfo(); */
		    /* SendAcqStat(); */
                    break;
 
               case SIGCHLD: /* Child Died Signal */
                    /* DPRINT(-1,"Infoproc: Received SIGCHLD\n"); */
                    /* DPRINT(-1,"Infoproc: Should Never Happen!!\n"); */
                    /* TheGrimReaper(NULL);  /* Obtain childs status */
                    break;
 
               case SIGALRM: /* Alarm */
                    /* DPRINT(-1,"Infoproc: Received SIGALRM, Statuscheck.\n"); */
		    Statuscheck();
                    break;
 
               case SIGINT: /* cntrl C */
               case SIGTERM: /* TERM */
               case SIGQUIT: /* QUIT */
                    /* DPRINT1(-1,"Infproc: Received SIGINIT, SIGTERM, or SIGQUIT; SigNumber: %d\n",signo); */
#ifndef __INTERIX
    		    close_rpc();
#endif
                    DestroyDomain();
                    exit(1);
                    break;
 
               case SIGPIPE: /* Broken PIPE */
                    /* DPRINT(-1,"Infproc: Received SIGPIPE, do nothing..\n"); */
                    /* DPRINT(-1,"         write to a closed pipe occured\n"); */
                    /* dontdie(); */
                    break;
 
               default:
#ifndef __INTERIX
    		    close_rpc();
#endif
                    exit(1);
                    break;
            }
         }
}


#ifndef __INTERIX

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

static void sigio_irpt()
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

}

#endif   /* __INTERIX */

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

/* now a threaded program, no more async signals handlers. */

block_signals()
{
}

unblock_signals()
{
}
