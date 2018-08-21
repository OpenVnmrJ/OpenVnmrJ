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

/* get proprer prototype for sigwait() */
#define _POSIX_PTHREAD_SEMANTICS

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#ifndef LINUX
#include <thread.h>
#endif
#include <pthread.h>


#include <errno.h>

#include "hostMsgChannels.h"
#include "msgQLib.h"
#include "errLogLib.h"
#include "procQfuncs.h"
/*
#include "eventHandler.h"
#include "commfuncs.h"
*/
#include "shrstatinfo.h"

#include "rcvrDesc.h"
#include "recvthrdfuncs.h"
#include "barrier.h"
#include "memorybarrier.h"

#include "flowCntrlObj.h"

MSG_Q_ID pRecvMsgQ;

char ProcName[256];

pthread_t main_threadId;


/* new for threads */
cntlr_crew_t TheRecvCrew;
barrier_t TheBarrier;
membarrier_t TheMemBarrier;
SHARED_DATA  TheSharedData;

FlowContrlObj *pTheFlowObj;

long long systemFreeRAM;
long long systemTotalRAM;

main(argc,argv)
int argc;
char *argv[];
{
   int rtn,stat,signo;
   char MsgInbuf[RECV_MSG_SIZE];
   sigset_t   blockmask,newmask,oldmask,zeromask;
   MSG_Q_ID msgId;
   void processMsge(void*);
   void asyncMainLoop(sigset_t sigMask);
   long long getFreeSystemRamSize();
   long long getSystemRamSize();

   strncpy(ProcName,argv[0],256);
   ProcName[255] = '\0';

  main_threadId = pthread_self();

  /* determine RAM of system */
  systemFreeRAM = getFreeSystemRamSize();
  systemTotalRAM = getSystemRamSize();

  /* set mask to block all signals, now any new threads will be spawned
     with this signal mask
   */
   sigfillset( &blockmask );

#ifdef PROFILING  /* enable these signals to allow profiling of threads */
   sigdelset(&blockmask, SIGPROF);
   sigdelset(&blockmask, SIGEMT);
#endif

   pthread_sigmask(SIG_BLOCK,&blockmask,NULL);

   DebugLevel = -3;

   umask(000); /* clear file creation mode mask,so that open has control */

#ifdef THREADED
  /* for threads */
   /* initCrew(&TheRecvCrew);  /* initialize the pthread crew structures */
   barrierInit(&TheBarrier, 2);  /* count will be reset as controllers subscribe */
   memset((char*) &TheSharedData,0,sizeof(SHARED_DATA));
   initMemBarrier(&TheMemBarrier, (void *) &TheSharedData);
   initCntlrStatus();
   buildProcPipeStage(NULL);
   pTheFlowObj = flowCntrlCreate();
#endif



   /* Make this program a daemon */
/*
 *  makeItaDaemon(1);
 */
    /*
     * have to comment out logSysInit() for now, for some reason when this routine closes all fd
     * the result is that NDDS will not start properly, initNDDS() return a NULL (not good)
     */  
   /* Lets use the syslog facility provide by the OS. */
   /* logSysInit("Recvproc",RECVLOG_FACILITY); */

#ifndef LINUX
  /* initialize LWS for  pthreads */
   thr_setconcurrency (5+1);  /* NDDS creates 5 itself  + main thread */
#endif

  /* initialize NDDS comunication with Expproc */
  /* got to do this before creating any threads , to avoid race conditions with NDDS */

   initiateNDDS(-1);   /* NDDS_Domain is set */
 
   /* initialize exception handlers */
   /* setupexcepthandler(); */
 
   /* Map in shared Exp Status */
   initExpStatus(0); 

   /* Map in Active Exp Q */
   initActiveExpQ(0);

   /* initialize command parser */
   initCmdParser();

   /* setup key database, and Message Queue, facilities */
   pRecvMsgQ = createMsgQ("Recvproc", (int) RECVPROC_MSGQ_KEY, (int) RECV_MSG_SIZE);
   if (pRecvMsgQ == NULL)
      exit(1);

   initProcQs(1);   /* zero out processing Queue */

   /* start up the Heart Beat thread for those interested */
   /* startHBThread("Recvproc"); HB no longer listened to,  1/29/07  GMB */

#ifndef THREADED

   /* start up the Heart Beat Listener for DDR 1 */
   startDDR_HBListener();

   /*  initial the NDDS subscription to the FID upload from the DDR(s) */
   initiateDataPub();

#endif  /* THREADED */

   /* initiateDataSub(); */
   cntlrDataUploadPatternSub();


  /* MsgQ Uses SIGUSR1 to indicate a message is present on the MsgQ */
 
#ifdef XXXX
  sigemptyset(&zeromask);
  sigemptyset(&newmask);
  sigemptyset(&oldmask);

  sigaddset(&newmask,SIGUSR1);

  pthread_sigmask(SIG_BLOCK,&newmask,&oldmask);
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


   /* catch ALL signals here */
   asyncMainLoop(blockmask);

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
      void processMsge(void*);

         pthread_sigmask(SIG_BLOCK,&sigMask,&oldMask);
         for( ; ; )
         {
            stat = sigwait(&sigMask, &signo);
            switch(signo)
            {
               case SIGIO:    /* async socket IO */
		    DPRINT(+1,"Recvproc: Received SIGIO, no action taken \n");
		    break;

               case SIGUSR1:
          	    DPRINT(+1,"Recvproc: Received SIGUSR1, processMsge() \n");
                    processMsge("getmsge");
		    break;

               case SIGUSR2:  /* internal thread signal */
		    /* DPRINT(+1,"Recvproc: Received SIGUSR2, No Action \n"); */
                    /* processInternalMsge(); maybe sometime in the future if a more general need is required */
		    DPRINT(+1,"Recvproc: Received SIGUSR2, ExpDone \n");
                    /* do this here so the function is called in the main thread context, 3/27/2006 GMB */
		    recvFidsCmplt();  
		    break;

	       case SIGCHLD: /* Child Died Signal */
                    DPRINT(+1,"Recvproc: Received SIGCHLD\n");
                    DPRINT(+1,"Recvproc: Should Never Happen!!\n");
                    /* TheGrimReaper(NULL);  /* Obtain childs status */
		    break;

	       case SIGALRM: /* Alarm */
                    DPRINT(+1,"Recvproc: Received SIGALRM, do nothing..\n");
		    break;

	       case SIGINT: /* cntrl C */
	       case SIGTERM: /* TERM */
                    DPRINT(+1,"Recvproc: Received SIGTERM\n");
                    excepthandler(signo);
		    break;

	       case SIGPIPE: /* Broken PIPE */
                    DPRINT(+1,"Recvproc: Received SIGPIPE, do nothing..\n");
		    break;

	       case SIGQUIT: /* QUIT */
                    DPRINT(+1,"Recvproc: Received SIGQUIT\n");
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
       rtn = recvMsgQ(pRecvMsgQ, MsgInbuf, RECV_MSG_SIZE, NO_WAIT);
       /* if we got a message then go ahead and parse it */
       if (rtn > 0)
       {
         DPRINT3(1,"received %d bytes, MsgInbuf len %d bytes, Msge: '%s'\n",rtn,strlen(MsgInbuf),
		((strlen(MsgInbuf) > 2) ? MsgInbuf : ""));
         parser(MsgInbuf);
         MsgInbuf[0] = '\0';
       }
     }
     while(rtn != -1);       /* if no message continue on */
         
  return;
}

long long getSystemRamSize()
{
    long numAvailablePages, numPages, pageSize;
    long long totalRAM, FreeRAM, UsedRAM;
    /* Note: Multiplying sysconf(_SC_PHYS_PAGES)  or
     * sysconf(_SC_AVPHYS_PAGES) by sysconf(_SC_PAGESIZE) to deter-
     * mine memory amount in bytes can exceed  the  maximum  values
     * representable in a long or unsigned long.
     */
     numPages = sysconf(_SC_PHYS_PAGES);
     pageSize = sysconf(_SC_PAGESIZE);
     totalRAM = (long long) numPages * (long long) pageSize;
     return totalRAM;
}

long long getFreeSystemRamSize()
{
    long numAvailablePages, numPages, pageSize;
    long long totalRAM, FreeRAM, UsedRAM;
    /* Note: Multiplying sysconf(_SC_PHYS_PAGES)  or
     * sysconf(_SC_AVPHYS_PAGES) by sysconf(_SC_PAGESIZE) to deter-
     * mine memory amount in bytes can exceed  the  maximum  values
     * representable in a long or unsigned long.
     */
     numAvailablePages = sysconf(_SC_AVPHYS_PAGES);
     numPages = sysconf(_SC_PHYS_PAGES);
     pageSize = sysconf(_SC_PAGESIZE);
     totalRAM = (long long) numPages * (long long) pageSize;
     FreeRAM = (long long) numAvailablePages * (long long) pageSize;
     UsedRAM = totalRAM - FreeRAM;
     /* printf(" numPages: %ld, available Pages: %ld, Used Pages: %ld, pageSize: %d\n", 
	numPages,numAvailablePages, numPages-numAvailablePages,pageSize); */
     /* printf("totalRAM: %llu, Free: %llu, Used: %llu\n",totalRAM,FreeRAM,UsedRAM); */
     return FreeRAM;
}
