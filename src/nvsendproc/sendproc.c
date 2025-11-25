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
/* require for proper sigwait */
#define _POSIX_PTHREAD_SEMANTICS

#include <stdio.h>
// #include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#ifndef LINUX
#include <thread.h>
#endif
#include <pthread.h>


#include <errno.h>

#include "hostMsgChannels.h"
#include "msgQLib.h"
#include "errLogLib.h"
#include "threadfuncs.h"
#include "barrier.h"
/*
#include "eventHandler.h"
#include "commfuncs.h"
#include "ndds/ndds_cdr.h"
#include "NDDS_Obj.h"
#include "threadfuncs.h"
*/

extern void  excepthandler(int signo);
extern int parser(char* str);
extern void initCodeDownldSub();
extern int initCmdParser();
extern int barrierInit(barrier_t *barrier, int count);
extern void initiateNDDS(int debugLevel);

MSG_Q_ID pRecvMsgQ;

char ProcName[256];

cntlr_crew_t TheSendCrew;
barrier_t TheBarrier;

int main(int argc, char *argv[])
{
   sigset_t   blockmask;
   void processMsge(void*);
   void asyncMainLoop(sigset_t sigMask);

   strncpy(ProcName,argv[0],256);
   ProcName[255] = '\0';

   /* set mask to block all signals, now any new threads will be spawned
      with this signal mask
   */
   sigfillset( &blockmask );
   pthread_sigmask(SIG_BLOCK,&blockmask,NULL);

   /*
   sigemptyset( &blockmask );
   sigaddset( &blockmask, SIGALRM );
   sigaddset( &blockmask, SIGIO );
   sigaddset( &blockmask, SIGCHLD );
   sigaddset( &blockmask, SIGQUIT );
   sigaddset( &blockmask, SIGPIPE );
   sigaddset( &blockmask, SIGALRM );
   sigaddset( &blockmask, SIGTERM );
   sigaddset( &blockmask, SIGUSR1 );
   */
   /* sigaddset( &blockmask, SIGUSR2 );  ABort signal should not be masked */

   DebugLevel = -3;
   if ( ! access("/vnmr/acqbin/Sendlog",F_OK) )
   {
      DebugLevel = 1;
   }

   umask(000); /* clear file creation mode mask,so that open has control */

   initCrew(&TheSendCrew);  /* initialize the pthread crew structures */
   barrierInit(&TheBarrier, 2);  /* count will be reset as controllers subscribe */

   /* Make this program a daemon */
/*
 *  makeItaDaemon(1);
 */

    /*
     * have to comment out logSysInit() for now, for some reason when this routine closes all fd
     * the result is that NDDS will not start properly, initNDDS() return a NULL (not good)
     */
   /* Lets use the syslog facility provide by the OS. */
   /* logSysInit("Sendproc",LOG_LOCAL0); */

#ifndef LINUX
  /* initialize LWS for  pthreads */
   thr_setconcurrency (MAX_CREW_SIZE+5+1);  /*crew +  NDDS creates 5 itself  + main thread */
#endif

  /* initialize NDDS comunication with Expproc */
  /* got to do this before creating any threads , to avoid race conditions with NDDS */

   initiateNDDS(-1);   /* NDDS_Domain is set, argument is the debuglevel  for NDDS */

  /* initialize default signal handlers */
   /* setupexcepthandler(); */

   /* initialize command parser */
   initCmdParser();

   /* setup key database, and Message Queue, facilities */
   pRecvMsgQ = createMsgQ("Sendproc", (int)SENDPROC_MSGQ_KEY, (int)SEND_MSG_SIZE);
   if (pRecvMsgQ == NULL)
      exit(1);


  /* Msgq Uses SIGUSR1 to indicate a message is present on the MsgQ */
 
/*
  sigemptyset(&zeromask);
  sigemptyset(&newmask);
  sigemptyset(&oldmask);

  sigaddset(&newmask,SIGUSR1);

  pthread_sigmask(SIG_BLOCK,&newmask,&oldmask);
*/

  /* Start the HeartBeat NDDS Publication for those interested */
  /* startHBThread("Sendproc");    method not used anymore    1/29/07  GMB */

 /* When the controllers codes downld reply pub is seen, 
  * this creates a new thread to handle it
  */
#ifndef RTI_NDDS_4x
  cntlrCodeDwnldPatternSub();
#else  /* RTI_NDDS_4x */
  initCodeDownldSub();
#endif /* RTI_NDDS_4x */

  // sleep(10);  // allow discover to happen for debugging 

  // sendCodes("/export/home/tmpcodes/exp1.greg.405830");

  // sleep(180);

  asyncMainLoop(blockmask);

#ifdef XXXX
  /* wait for msgQ and then parse */
  while(1)
  {
     stat = sigwait(&newmask, &signo);
     if (signo == SIGUSR1)
     {
          DPRINT(+1,"Received SIGUSR1, processMsge() \n");
          processMsge("getmsge");
     }
     else if(signo == SIGUSR2)
     {
           DPRINT(+1,"Received SIGUSR2\n");
     }
  }
#endif
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
        int stat __attribute__((unused));
        int signo;
   void TheGrimReaper(void*);
   void processMsge(void*);

         pthread_sigmask(SIG_BLOCK,&sigMask,&oldMask);
         for( ; ; )
         {
            stat = sigwait(&sigMask, &signo);
            switch(signo)
            {
               case SIGIO:    /* async socket IO */
		    DPRINT(+1,"Sendproc: Received SIGIO, no action taken \n");
		    break;

               case SIGUSR1:
          	    DPRINT(+1,"Sendproc: Received SIGUSR1, processMsge() \n");
                    processMsge("getmsge");
		    break;

               case SIGUSR2:  /* internal NDDS pipe signal */
		    DPRINT(+1,"Sendproc: Received SIGUSR2, no action taken.\n");
		    break;

	       case SIGCHLD: /* Child Died Signal */
                    DPRINT(+1,"Sendproc: Received SIGCHLD\n");
                    DPRINT(+1,"Sendproc: SHould Never Happen!!\n");
                    // TheGrimReaper(NULL);  /* Obtain childs status */
		    break;

	       case SIGALRM: /* Alarm */
                    DPRINT(+1,"Sendproc: Received SIGALRM, do nothing..\n");
		    break;

	       case SIGINT: /* cntrl C */
	       case SIGTERM: /* TERM */
                    DPRINT(+1,"Sendproc: Received SIGTERM\n");
                    excepthandler(signo);
		    break;

	       case SIGPIPE: /* Broken PIPE */
                    DPRINT(+1,"Sendproc: Received SIGPIPE, do nothing..\n");
		    break;

	       case SIGQUIT: /* QUIT */
                    DPRINT(+1,"Sendproc: Received SIGQUIT\n");
                    excepthandler(signo);
		    break;

               default:
                    excepthandler(signo);
                    /* DPRINT1(+1,"Received non handled SIGNAL: %d\n",signo); */
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
       rtn = recvMsgQ(pRecvMsgQ, MsgInbuf, SEND_MSG_SIZE, NO_WAIT);
       /* if we got a message then go ahead and parse it */
       if (rtn > 0)
       {
         DPRINT3(1,"processMsge(): received %d bytes, MsgInbuf len %zd bytes, Msge: '%s'\n",rtn,strlen(MsgInbuf),
			((strlen(MsgInbuf) > 2) ? MsgInbuf : ""));
         parser(MsgInbuf);
         MsgInbuf[0] = '\0';
       }
       else
       {
         DPRINT1(1,"processMsge(): received %d bytes, No Message\n",rtn);
       }
     }
     while(rtn != -1);       /* if no message continue on */
         
  return;
}

