/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <setjmp.h>
#include <signal.h>


#include <errno.h>

#include "hostMsgChannels.h"
#include "msgQLib.h"
#include "errLogLib.h"
#include "eventHandler.h"
#include "commfuncs.h"
#include "procQfuncs.h"
#include "REV_NUMS.h"

MSG_Q_ID pRecvMsgQ;

char ProcName[256];

int  SystemVersionId = INOVA_SYSTEM_REV;

static sigjmp_buf ReStart_Env;
static void childItrp(int);

main(argc,argv)
int argc;
char *argv[];
{
   int rtn;
   sigset_t   blockmask;
   void TheGrimReaper(void*);
   void processMsge(void*);
   int cnt;

   strncpy(ProcName,argv[0],256);
   ProcName[255] = '\0';

   sigemptyset( &blockmask );
   sigaddset( &blockmask, SIGALRM );
   sigaddset( &blockmask, SIGIO );
   sigaddset( &blockmask, SIGCHLD );
   sigaddset( &blockmask, SIGQUIT );
   sigaddset( &blockmask, SIGPIPE );
   sigaddset( &blockmask, SIGALRM );
   sigaddset( &blockmask, SIGTERM );
   sigaddset( &blockmask, SIGUSR1 );
   sigaddset( &blockmask, SIGUSR2 );

   DebugLevel = 0;
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

   /* Lets use the syslog facility provide by the OS. */
   logSysInit("Expproc",EXPLOG_FACILITY);

   /* Open three file descriptor to be sure that stdin, stdout
    * and stderr messages from forked programs do not get sent
    * to the console
    */
   open("/dev/null", O_RDWR);
   open("/tmp/procout", O_WRONLY | O_CREAT | O_TRUNC,0666);
   open("/tmp/procerr", O_WRONLY | O_CREAT | O_TRUNC,0666);

   /* initialize the exception handlers */
   setupexcepthandler();

   /* clean up semaphores not being used */
   semClean();	/* only Expproc does this */

   /* initialize the shared Exp Status */
   initExpStatus(1);   /* zero out Exp Status */

   /* Map in Active Exp Q */
   initActiveExpQ(1);	/* zero out Active Exp Status */

   /* initialize command parser */
   initCmdParser();

   /* Initialize the Event Handlers Queue */
   setupForAsync(20, &blockmask);

   initExpQs(1);   /* zero out Exp Queue */
   /* initActiveExpQ(1); */

   initProcQs(1);   /* init processing Q */
   initActiveQ(1);  /* init active processing Q */
   
   /* here we register both the signal handler to be called
       and the non-interrupt function to handle the I/O */
   registerAsyncHandlers(
			  SIGCHLD,	/* Child Died Signal */
			  childItrp,	/* this puts the event on the eventQ */
			  TheGrimReaper /* Obtain childs status */
			 );

   /* setup key database, and Message Queue, facilities */
   pRecvMsgQ = createMsgQ("Expproc", (int) EXPPROC_MSGQ_KEY, (int) EXP_MSG_SIZE);
   if (pRecvMsgQ == NULL)
      exit(1);

   setMsgQAsync(pRecvMsgQ,processMsge);
   initApplSocket();		/* initialize the socket for VNMR, ACQI, etc. */

   controlHeartBeat(0);
   if ( sigsetjmp(ReStart_Env, 1) != 0)
   {
      /* Come here is the connection to the console is broken */
      DPRINT(1,"Restart Expproc to initiateAsyncChan w/ Console\n");
      controlHeartBeat(0);
      initConsoleStatus();	/* reInit Acq Status */
      killTasks();  /* kill all spawned processes: Sendproc, Recvproc, etc... */
      delacqinfo2();
      resetState();
      expQclean();
      activeExpQclean();
      procQclean();
      activeQclean();
      closeChannel(chanId);
      chanId = -1;
      closeChannel(chanIdSync);
      chanIdSync = -1;
   }

   /* This does not return until a connection to the console
    * has been established
    */
   chanId = initiateAsyncChan(EXPPROC_CHANNEL);

   /* set up the timer but don't start heartbeat until the rest
    * of the procs are going
    */
   if ( chanId != -1 )
   {
      setupHeartBeat();
   }

   /* try to connect to updating channel, if fails here will try again when
      an actual update command is received. For Mercury, there is no separate
      updating channel, so this will fail.
   */
   cnt = 6;
   chanIdSync = -1;
   while( ((cnt > 0) && (chanIdSync == -1)) )
   {
      chanIdSync = connectChan(UPDTPROC_CHANNEL);
      cnt--;
   }

   /*  Start the procs */
   restartTasks();

   /*  Enable heartbeat */
   controlHeartBeat(1);
   wrtacqinfo2();

   asyncMainLoop();

   exit(0);
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

/**************************************************************
*
*  childItrp - Routine envoked on receiving the SIGCHLD
*
*  This catches the SIGCHLD Signal add place the 'event' on the
*  event Q. Then in non-interrupt mode the register function will
*  be called to handle the actual I/O.
*  
* RETURNS:
* void 
*
*       Author Greg Brissey 9/6/94
*/
static void
childItrp(int signal)
{
    /* Place the SIGCHLD interrupt & int onto the eventQ, the non-
       interrupt function (processMsge) will be called with msgId as an argument */
    processNonInterrupt( SIGCHLD, (void*) NULL );
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

#ifndef WCOREDUMP
#define  WCOREDUMP( statval )   ((( (statval) & 0x80) != 0) ? 1 : 0)
#endif
 
void
TheGrimReaper(void* arg)
{
    int expid;
    int item;
    int coredump;
    int pid;
    int status;
    int termsig;
    int result;
    int kidstatus;
    char ActiveId[256];
    int  activetype,fgbg,apid;
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
	   DPRINT1(1,"GrimReaper: Unknown Died.\n",whodied);
	}
   }
   /* Now that we are done catching kids, lets check for processing to do */
   expQTask();

   return;
}

resetExpproc()
{
    int stat;
    DPRINT(3,"resetExpproc\n");
    siglongjmp(ReStart_Env,1);
}
