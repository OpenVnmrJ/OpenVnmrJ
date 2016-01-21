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
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>


#include <errno.h>

#include "hostMsgChannels.h"
#include "msgQLib.h"
#include "errLogLib.h"
#include "eventHandler.h"
#include "procQfuncs.h"
#include "commfuncs.h"
#include "shrstatinfo.h"
#include "shrexpinfo.h"
#include "shrstatinfo.h"
#include "shrMLib.h"
#include "process.h"
#include "expDoneCodes.h"

/*

     Procproc is the conditional data processing engine.
     This task queues incoming messages to process Experiment
     data.

     From this queue data processing is started. Processing
     will be done in Foreground if a Vnmr is present and in the 
     Experiment directory. Porcessing will be done in the Background
     (i.e. fork a seperate Vnmr to do process) if no running Vnmr is in
     the Experiment directory or if this Experiment is an automation run.

     Rules of processing:
     --------------------

     Experiment, Error, Su and the Last NT processing are guaranteed to occur.

     Processing of a lower priority task will be terminated if possible
     and the high priority task will then be performed. 
     E.G. While performing Wbs processing a request for Wnt processing 
          is received, Wbs processing is stopped and Wnt processing begun.

     In Order of priority:
     Hightest Priority: Wexp, Werr, Wsu   - same priority  (mutually exclusive)  
     			Wnt
     Lowest Priority:   Wbs

     Multiple Processing of same type will be skip if that type of
     process or high priority processing is running. 
     E.G. While performing Wbs processing 10 more Wbs processing
          request arrive. These 10 are skipped (ignored).


    Wexp processing with wait will go to the head of the Wexp 
    processing to be done.
    E.G.  Automation Exp running, several experiment completed waiting on
          Wexp processing. The next Wexp is a WAIT "au(wait)" this Wexp 
	  is done ahead of the other waiting Wexp processing.

   
*/

MSG_Q_ID pRecvMsgQ;

char ProcName[256];
char Vnmrpath[256];

extern ExpInfoEntry ProcExpInfo;
extern int mapOutExp(ExpInfoEntry *expid);
extern int deliverMessage( char *interface, char *message );
extern void bgProcComplt(char *expIdStr, int proctype, int dCode, int pid);
extern int activeQtoBG(int oldfgbg, long key, int newfgbg, int procpid);
extern int sendproc2BG(ExpInfoEntry* pProcExpInfo, int proctype, char *cmdstring);
extern int procQTask();
extern int parser(char* str);
extern int initCmdParser();
extern void setupexcepthandler();
extern void initQExpInfo();
extern int recheckFG;


static void childItrp(int);

int main(int argc, char *argv[])
{
   sigset_t    blockmask;
   void TheGrimReaper(void*);
   void processMsge(void*);
   char *tmpptr;
 
   strncpy(ProcName,argv[0],256);
   ProcName[255] = '\0';
   tmpptr = getenv("vnmrsystem");            /* vnmrsystem */
   if (tmpptr != (char *) 0)
   {
      strcpy(Vnmrpath,tmpptr);	/* copy value into global */
   }
   else
   {
      strcpy(Vnmrpath,"/vnmr");	/* use /vnmr as default value */
   }
#ifdef LINUX
   strcat(Vnmrpath,"/bin/Vnmrbg");	/* path to background vnmr program */
#else
   strcat(Vnmrpath,"/bin/Vnmr");	/* path to background vnmr program */
#endif

  /* --- mask to block SIGALRM, SIGIO and SIGCHLD interrupts --- */
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

   umask(000); /* clear file creation mode mask,so that open has control */

  /* initializes these structures to zip */
   initQExpInfo();

   DPRINT1(1,"Starting: '%s'\n",ProcName);
   /* Lets use the syslog facility provide by the OS. */
   logSysInit("Procproc",PROCLOG_FACILITY);

   /* initialize exception handlers */
   setupexcepthandler();

   /* Map In the shared Exp Status */
   initExpStatus(0);

   /* initialize command parser */
   initCmdParser();

   initProcQs(1);
   initActiveQ(1);

   /* Initialize the Event Handlers Queue */
   setupForAsync(20, &blockmask);
   /* here we register both the signal handler to be called
       and the non-interrupt function to handle the I/O */
   registerAsyncHandlers(
			  SIGCHLD,	/* BG processing Signal */
			  childItrp,	/* this puts the event on the eventQ */
			  TheGrimReaper
			 );

   /* setup key database, and Message Queue, facilities */
   pRecvMsgQ = createMsgQ("Procproc", (int) PROCPROC_MSGQ_KEY, (int) PROC_MSG_SIZE);
   if (pRecvMsgQ == NULL)
      exit(1);

   setMsgQAsync(pRecvMsgQ,processMsge);

   /*
	registerAsyncHandlers() in setupMsgSig() setup the signal handler
        and non-interrupt routines
   */
   asyncMainLoop();

   shutdownComm();

   return(EXIT_SUCCESS);
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
  char MsgInbuf[PROC_MSG_SIZE];
  int rtn;

  /* Keep reading the Msg Q until no further Messages */
  do {
	   /* read msgQ don't block if none there */
           rtn = recvMsgQ(pRecvMsgQ, MsgInbuf, RECV_MSG_SIZE, NO_WAIT);
	   /* if we got a message then go ahead and parse it */
	   if (rtn > 0)
	   {
	      DPRINT2(1,"received %d bytes, MsgInbuf len %d bytes\n",rtn,strlen(MsgInbuf));
	      parser(MsgInbuf);
	      MsgInbuf[0] = '\0';
	   }
      }
      while(rtn != -1);	/* if no message continue on */

   procQTask();

   return;
}

/*-------------------------------------------------------------------------
|
|   FGcomplt()
|   Called when the FG processing complete message from Vnmr is 
|   received.
|   Then check for more conditional processing.
|
|                               Author Greg Brissey 9/8/94
+--------------------------------------------------------------------------*/
int FGcomplt(char* str)
{
  char *value;
  int FGkey;
  char ActiveId[EXPID_LEN];
  int activetype,fgbg,pid,dCode;
  int ret,when_select;
  extern char *proctypeName(int);

  value = strtok(NULL,"\n");
  value = strtok(NULL," ,");
  FGkey = atoi(value);
  value = strtok(NULL," ,");
  when_select = atoi(value);

  DPRINT2(1,"FGcomplt: FGkey= %d when_select= %d\n",
	     FGkey,when_select);
  activeQget(&activetype, ActiveId, &fgbg, &pid, &dCode );
  DPRINT5(1,"FGcomplt: '%s' for '%s' completed, rKey=%d, gKey=%d DoneCode=%d\n",
	     proctypeName(activetype),ActiveId,pid,FGkey,dCode);
  ret = 1;
  if (when_select == 0)
  {
     if (activeQdelete(FG,FGkey) == -1)
     {
        DPRINT1(1,"FGcmplt: FGkey %d has no match\n",FGkey);
        ret = 0;
     }
  }
  else
  {
       char msge[1024];
       int pos;
       int pid;

       value = strtok(NULL,"\n");
       strcpy(msge,value);
       pos = strlen(msge) - 1;
       if (msge[pos] == ',')
          msge[pos] = '\0';
       DPRINT1(1,"FGcomplt: send command %s to BG\n", msge);
       if ( (pid = sendproc2BG(&ProcExpInfo, activetype, msge)) )
       {
	    activeQtoBG(FG, FGkey, BG, pid);
            ret = 0;
       }
  }
  if (ret)
  {
    /* if last of processing then delete shared exp info file */
    if ( ((activetype == WERR) && (dCode != WARNING_MSG) && (dCode != EXP_STARTED) ) ||
         (activetype == WEXP) || 
         (activetype == WEXP_WAIT) )
    {
       if (ProcExpInfo.ExpInfo->ExpFlags & AUTOMODE_BIT)
       {
          MSG_Q_ID pAutoMsgQ;

          pAutoMsgQ = openMsgQ("Autoproc");
          if (pAutoMsgQ != NULL)
          {
             char msgestring[EXPINFO_STR_SIZE + 32];
             /* Inform Autoproc of completion so it can update the doneQ */
             if (activetype == WERR)
             {
                sprintf(msgestring,"cmplt 1 %s",ProcExpInfo.ExpInfo->DataFile);
                /* The -4 after strlen(msgestring) is to remove the ".fid" from DataFile */
                sendMsgQ(pAutoMsgQ, msgestring, strlen(msgestring)-4, MSGQ_NORMAL,
                        WAIT_FOREVER); /* NO_WAIT */
             }
             else
             {
                sprintf(msgestring,"cmplt 0 %s",ProcExpInfo.ExpInfo->DataFile);
                /* The -4 after strlen(msgestring) is to remove the ".fid" from DataFile */
                sendMsgQ(pAutoMsgQ, msgestring, strlen(msgestring)-4, MSGQ_NORMAL,
                        WAIT_FOREVER); /* NO_WAIT */
             }
             sendMsgQ(pAutoMsgQ, "resume", strlen("resume"), MSGQ_NORMAL,
                             WAIT_FOREVER); /* NO_WAIT */
             closeMsgQ(pAutoMsgQ);
          }
          else
          {
             errLogRet(ErrLogOp,debugInfo,
                 "bgProcComplt: Could open Autoproc MsgQ\n");
          }
       }
       if( strcmp(ProcExpInfo.ExpId,ActiveId) == 0)
       {
         mapOutExp(&ProcExpInfo);
       }
       unlink(ActiveId);
       deliverMessage("Expproc","chkExpQ");
    }
  }

  /* lets check for processing to do */
   /*
    * Just got a message from FG. Next time FG does not respond, send another acqsend('check')
    * Done in procQTask process.c
    */
   recheckFG = 1;
   procQTask();
   return(0);
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
    processNonInterrupt( SIGCHLD, (void*) BG );
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

void
TheGrimReaper(void* arg)
{
    int coredump;
    int pid;
    int status;
    int termsig;
    int kidstatus;
    char ActiveId[256];
    int  activetype,fgbg,apid,dCode;
    extern char *proctypeName(int);

    DPRINT(1,"|||||||||||||||||||  SIGCHLD   ||||||||||||||||||||||||\n");
    DPRINT(1,"TheGrimReaper(): At Work !!!\n");

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
        {
          termsig = WTERMSIG( kidstatus );
          coredump = WCOREDUMP( kidstatus );
        }
        else
        {
          coredump = termsig = 0;
        }

        DPRINT4(1,"Procproc - TheGrimReaper: Child Pid: %d, Status: %d, Core Dumped: %d, Termsig: %d\n",
            	pid,status,coredump,termsig);

        activeQget(&activetype, ActiveId, &fgbg, &apid, &dCode );
	DPRINT5(1,"Processing for: '%s', pid: %d, type: %s, DoneCode: %d %s.\n",
	         ActiveId, apid, proctypeName(activetype), dCode,
                 (termsig) ? "Terminated" : "Completed");
        if (apid == pid)
        {
	   bgProcComplt(ActiveId, activetype, dCode, pid);
           deliverMessage("Expproc","chkExpQ");
        }
    }

   /* Now that we are done catching kids, lets check for processing to do */
   procQTask();

   return;
}
