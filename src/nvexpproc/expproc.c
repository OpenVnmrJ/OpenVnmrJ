/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#define _POSIX_PTHREAD_SEMANTICS

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <setjmp.h>
#include <signal.h>
#if   !defined(LINUX) && !defined(__INTERIX)
#include <thread.h>
#endif
#include <pthread.h>


#include <errno.h>

#include "hostMsgChannels.h"
#include "msgQLib.h"
#include "errLogLib.h"
#include "commfuncs.h"
#include "procQfuncs.h"
#include "REV_NUMS.h"

#include "sysUtils.h"

MSG_Q_ID pRecvMsgQ;

char ProcName[256];

char ConsoleNicHostname[48];

int  SystemVersionId = INOVA_SYSTEM_REV;

static sigjmp_buf ReStart_Env;


pthread_t main_threadId;

static void dummySigChld(int dummy)
{
 /*
  * The behavior of sigwait() has changed. Signals that are set to SIG_IGN 
  * no longer trigger the sigwait(). Since SIGCHLD is, by default, SIG_IGN,
  * it does not trigger sigwait(), and therefore the children are not
  * reaped. By setting the sigaction() of SIGCHLD to this dummy function, 
  * the sigwait() in asyncMainLoop() now is triggered by a SIGCHLD signal,
  * calling TheGrimReaper() to handle the child's exit.
  */
}

static void setSigChld()
{
    sigset_t		qmask;
    struct sigaction	intquit;

    /* --- set up interrupt handler --- */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGCHLD );
    intquit.sa_handler = dummySigChld;
    intquit.sa_mask = qmask;
    intquit.sa_flags = 0;
    sigaction(SIGCHLD,&intquit,NULL);
}

int main(int argc, char *argv[])
{
   sigset_t   blockmask;
   void asyncMainLoop(sigset_t sigMask);

   /* set mask to block all signals, now any new threads will be spawned
     with this signal mask
   */
   sigfillset( &blockmask );

#ifdef PROFILING  /* enable these signals to allow profiling of threads */
   sigdelset(&blockmask, SIGPROF);
   sigdelset(&blockmask, SIGEMT);
#endif

   pthread_sigmask(SIG_BLOCK,&blockmask,NULL);

   strncpy(ProcName,argv[0],256);
   ProcName[255] = '\0';

   main_threadId = pthread_self();

   strncpy(ConsoleNicHostname,get_console_hostname(),47);
   ConsoleNicHostname[47] = '\0';


   DebugLevel = -3;
   if ( ! access("/vnmr/acqbin/Explog",F_OK) )
   {
      DebugLevel = 1;
   }
   DPRINT1(1,"Starting: '%s'\n",ProcName);

   umask(000); /* clear file creation mode mask,so that open has control */

#ifndef DEBUG
   /* Make this program a daemon */
   makeItaDaemon(0);  /* make this program a daemon */
#endif

    /*
     * have to comment out logSysInit() for now, for some reason when this routine closes all fd
     * the result is that NDDS will not start properly, initNDDS() return a NULL (not good) 
     */ 
   /* Lets use the syslog facility provide by the OS. */
   /* logSysInit("Expproc",EXPLOG_FACILITY); */

#if   !defined(LINUX) && !defined(__INTERIX)
   /* initialize LWS for  pthreads */
   thr_setconcurrency (5+1);  /* NDDS creates 5 itself  + main thread */
#endif
 


   /* Open three file descriptor to be sure that stdin, stdout
    * and stderr messages from forked programs do not get sent
    * to the console
    */
   open("/dev/null", O_RDWR);
   open("/tmp/procout", O_WRONLY | O_CREAT | O_TRUNC,0666);
   open("/tmp/procerr", O_WRONLY | O_CREAT | O_TRUNC,0666);

   /* clean up semaphores not being used */
   semClean();	/* only Expproc does this */

   initiateNDDS(-1);
   usleep(100000);   /* 100 ms, let manager get started */

   /* initialize the shared Exp Status */
   initExpStatus(1);   /* zero out Exp Status */

   /* Map in Active Exp Q */
   initActiveExpQ(1);	/* zero out Active Exp Status */

   /* initialize command parser */
   initCmdParser();

   /* Initialize the Event Handlers Queue */
   /* setupForAsync(20, &blockmask);   thing of the past */

   initExpQs(1);   /* zero out Exp Queue */
   /* initActiveExpQ(1); */

   initProcQs(1);   /* init processing Q */
   initActiveQ(1);  /* init active processing Q */
   setSigChld();

   /* start up the Heart Beat thread for those interested */
   /* startHB_Monitor();    to cut down traffic not using this method anymore    1/26/07  GMB */

   /* setup key database, and Message Queue, facilities */
   pRecvMsgQ = createMsgQ("Expproc", (int) EXPPROC_MSGQ_KEY, (int) EXP_MSG_SIZE);
   if (pRecvMsgQ == NULL)
      exit(1);

   /* this create a async socket, it's address in /vnmr/acqqueue/acqinfo2, the call back routine is
      handled directly within the SIGIO handler (this is bad). processApplSocket() is the callback routine
      in socketfuncs.c wich registers processAcceptSocket()  wihch reads the stream from the socket 
      then passes this to parser().
      ------------------------------------------------------------------------------------------
      4/20/04  
      Now SIGIO IO is handle directly by asyncMainLoop(0 via thread perferred technique of sigwait()
      and the underlying asyncIO.c routines no longer use signal handlers. the processIO() is called
      from the new asyncMainLoop() below. The recommended handling of signals for threaded programs
      via sigwait()
				Greg Brissey
  */
   /* initApplSocket(); */	/* initialize the socket for VNMR, ACQI, etc. */
   initExpprocSocket();         /* initialize the socket for VNMR, ACQI, etc. */

   initiatePubSub();		/* create the console monitor publications & subscriptions */

   if ( sigsetjmp(ReStart_Env, 1) != 0)
   {
      /* Come here is the connection to the console is broken */
      DPRINT(1,"Restart Expproc to initiateAsyncChan w/ Console\n");
      /* controlHeartBeat(0); */
      initConsoleStatus();	/* reInit Acq Status */
      killTasks();  /* kill all spawned processes: Sendproc, Recvproc, etc... */
      delacqinfo2();
      resetState();
      expQclean();
      activeExpQclean();
      procQclean();
      activeQclean();
   }

   /*  Start the procs */
   restartTasks();

   /* fill in which signals we want sigwait to handle */
   /*
   sigemptyset( &blockmask );
   sigaddset( &blockmask, SIGALRM );
   sigaddset( &blockmask, SIGIO );
   sigaddset( &blockmask, SIGCHLD );
   sigaddset( &blockmask, SIGINT );
   sigaddset( &blockmask, SIGQUIT );
   sigaddset( &blockmask, SIGPIPE );
   sigaddset( &blockmask, SIGALRM );
   sigaddset( &blockmask, SIGTERM );
   sigaddset( &blockmask, SIGUSR1 );
   sigaddset( &blockmask, SIGUSR2 );
   */

   wrtacqinfo2();

   asyncMainLoop(blockmask);  /* blockmask has all bits set */

   exit(0);
}

/*****************************************************************
*
*   asyncMainLoop
*
*   INTERNAL
*
*   The application calls this program when it is ready to switch into asychronous
*   mode.  This program never returns.  
*   Previous asyncMainLoop used signal handler to queue up event to be handle later in none
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
*   SIGIO (SIGPOLL) are handled by processIO() within asyncIO.c, async sockets.
*   SIGUSR1 are System V message Q
*   SIGUSR2 used to be an abort but is now ignored  for a better scheme
*   SIGCHLD call the GrimReaper 
*   Other signals cause coredump are still handled by signal handler but this is OK
*       since these signal are always fatal and cause program termination
*
*/
void asyncMainLoop(sigset_t sigMask)
{
	sigset_t		oldMask;
        int stat;
        int signo;
   void TheGrimReaper(void*);
   void processMsge(void*);
   extern int MonCmdPipe[2];

         pthread_sigmask(SIG_BLOCK,&sigMask,&oldMask);
         for( ; ; )
         {
            stat = sigwait(&sigMask, &signo);
            switch(signo)
            {
               case SIGIO:    /* async socket IO */
		    DPRINT(+1,"Expproc: Received SIGIO, processIO() \n");
		    processExpSock();
		    break;

               case SIGUSR1:
                    DPRINT(+1,"Expproc: Received SIGUSR1, processMsgQ() \n");
                    processMsge("getmsge");
		    break;

               case SIGUSR2:  /* internal NDDS pipe signal */
		    DPRINT(+1,"Expproc: Received SIGUSR2, processIO() \n");
                    processMonitorCmd(MonCmdPipe);
		    break;

	       case SIGCHLD: /* Child Died Signal */
                    DPRINT(+1,"Expproc: Received SIGCHLD\n");
                    TheGrimReaper(NULL);  /* Obtain childs status */
		    break;

	       case SIGALRM: /* Alarm */
                    DPRINT(+1,"Expproc: Received SIGALRM, do nothing..\n");
		    break;

	       case SIGINT: /* cntrl C */
	       case SIGTERM: /* TERM */
                    DPRINT(+1,"Expproc: Received SIGTERM\n");
                    excepthandler(signo);
		    break;

	       case SIGPIPE: /* Broken PIPE */
                    DPRINT(+1,"Expproc: Received SIGPIPE, do nothing..\n");
		    break;

	       case SIGQUIT: /* QUIT */
                    DPRINT(+1,"Expproc: Received SIGQUIT\n");
                    excepthandler(signo);
		    break;

               default:
                    excepthandler(signo);
                    /* DPRINT1(+1,"Expproc: Received non handled SIGNAL: %d\n",signo); */
		    break;
            }   
         }
}

/**************************************************************
*
*  processMsge - Routine envoked to read message Q and call parser
*
*   This Function is the Non-interrupt function called to handle the
*   msgeQ interrupt as register in setupMsgSig() via registerAsyncHandlers()
*   (proccomm.c)
*
* RETURNS:
* void
*
*       Author Greg Brissey 9/6/94
*/
void processMsge(void *notin)
{
  char MsgInbuf[RECV_MSG_SIZE];
  int rtn;
 
 /* Keep reading the Msg Q until no further Messages */
  do {
       /* read msgQ don't block if none there */
       rtn = recvMsgQ(pRecvMsgQ, MsgInbuf, RECV_MSG_SIZE, NO_WAIT);
       /* if we got a message then go ahead and parse it */
       if (rtn > 0)
       {
         DPRINT2(1,"received %d bytes, MsgInbuf len %d bytes\n",rtn,strlen(MsgInbuf));
	 DPRINT1(1,"Expproc received command: %s\n", &MsgInbuf[ 0 ] );
         parser(MsgInbuf);
         MsgInbuf[0] = '\0';
       }
     }
     while(rtn != -1);       /* if no message continue on */
         
  return;
}

/*-------------------------------------------------------------------------
|
|   TheGrimReaper()
|   Get the Status of the died children so that it may rest in peace
|   (i.e., get status of exited BG process so it doesn't become a Zombie)
|   Then check for more conditional processing.
|
|                               Author Greg Brissey 9/8/94
+--------------------------------------------------------------------------*/

#ifndef  WCOREDUMP
#define  WCOREDUMP( statval )   ((( (statval) & 0x80) != 0) ? 1 : 0)
#endif
 
void
TheGrimReaper(void* arg)
{
    int coredump;
    int pid;
    int status;
    int termsig;
    int kidstatus;
    char *whodied;
    extern char *proctypeName(int);
    extern char *registerDeath(pid_t);

    DPRINT(1,"|||||||||||||||||||  SIGCHLD   ||||||||||||||||||||||||\n");
    DPRINT(1,"Expproc GrimReaper(): At Work\n");

    /* --- GrimReaper get all exited or signal children before leaving --- */
    /*     Note:  1st argument to waitpid is -1 to specify any child process.  */
 
    while ((pid = waitpid( -1, &kidstatus, WNOHANG | WUNTRACED )) > 0)
    {
        if ( WIFSTOPPED(kidstatus) )  /* Is this an exiting or stopped Process */
           continue;                  /* If a STOPPED Process go to next waitpid() */
 
        /* if non-zero if normal termination of child */
        if (WIFEXITED( kidstatus ) != 0)
          status = WEXITSTATUS( kidstatus );
        else
          status = 0;

        /* child terminated due to an uncaught signal */
        if (WIFSIGNALED( kidstatus ) != 0)
          termsig = WTERMSIG( kidstatus );
        else
          termsig = 0;

	/* child core dumped */
        coredump = WCOREDUMP( kidstatus );
 
        DPRINT4(1,"GrimReaper: Child Pid: %d, Status: %d, Core Dumped: %d, Termsig: %d\n",
            	pid,status,coredump,termsig);

	whodied = registerDeath(pid);
        if (whodied)
	{
	   DPRINT1(1,"GrimReaper: '%s' Died.\n",whodied);
	}
	else
	{
	   DPRINT(1,"GrimReaper: Unknown Died.\n");
	}
   }
   /* Now that we are done catching kids, lets check for processing to do */
   expQTask();

   return;
}

void resetExpproc()
{
    DPRINT(3,"resetExpproc\n");
    siglongjmp(ReStart_Env,1);
}
