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
/*
modification history
--------------------
12-7-94,gmb  created 
4-29-97,rol  added argument to reset2SafeState, so if the connection to
             the host is lost (message type == LOST_CONN), reset2SafeState
             does not try to resynchronize the downlinker.
8-03-04.gmb  converted for nirvana
*/

/*
DESCRIPTION

   This Task Handlers the Problems or Exceptions that happen to the
   system. For Example Fifo Errors (FORP,FOO,etc).

   The logic for handling these execption is encapsulated in this task
   thus there is one central place where the logic is placed.

*/

#ifndef ALLREADY_POSIX
 #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#endif
#include <string.h>
#include <vxWorks.h>
#include <stdioLib.h>
#include <rngLib.h>
#include <semLib.h>
#include <msgQLib.h>
#include <wdLib.h>
#include "hostAcqStructs.h"
#include "expDoneCodes.h"
#include "errorcodes.h"
#include "logMsgLib.h"
#include "taskPriority.h"
/* #include "tuneObj.h" */
#include "sysflags.h"
#include "instrWvDefines.h"
#include "fifoFuncs.h"
#include "Cntlr_Comm.h"
#include "Monitor_Cmd.h"
#include "cntlrStates.h"
#include "Console_Stat.h"
#include "vtfuncs.h"
#include "nexus.h"

#define HOSTNAME_SIZE 80
extern char hostName[HOSTNAME_SIZE];

extern SEM_ID  pSemSAStop; /* Binary  Semaphore used to Stop upLinker for SA */

/* extern MSG_Q_ID pUpLinkMsgQ;	/* MsgQ used between UpLinker and STM Object */
extern MSG_Q_ID pMsgesToPHandlr;/* MsgQ for Msges to Problem Handler */
extern MSG_Q_ID pMsgesToHost;	/* MsgQ used for Msges to routed upto Expproc */

extern VT_ID pTheVTObject;

Monitor_Cmd ExceptionMsg =  { 0, 0, 0, 0, 0, 0, 0, 0, 0 } ; 

extern RING_ID  pSyncActionArgs;  /* Buffer for 'Sync Action' (e.g. SETVT) function args */

extern SEM_ID pSemOK2Tune;

extern int failAsserted;

extern int enableSpyFlag;  /* if > 0 then invoke the spy routines to monitor CPU usage  11/9/05 GMB */

extern void sendInfo2ConsoleStat(int tickmismatch);

/* Fixed & Dynamic Named Buffers */

extern int     SA_Criteria;/* SA, EXP_FID_CMPLT, BS_CMPLT, IL_CMPLT */
extern unsigned long SA_Mod; /* modulo for SA, ie which fid to stop at 'il'*/

static void resetSystem();
static void reset2SafeState();
static void resetNameBufs();
static void AbortExp(CNTLR_COMM_MSG *msge, int externflag);

static int pHandlerTid;
static int pHandlerPriority;

WDOG_ID pHandlerWdTimeout;
int pHandlerTimeout;

#define CASE (2)

/* Safe State Codes						*/
/* The first two words are high speed line safe states.		*/
/* The next words are waveform generator and pulse field	*/
/* gradient codes to disable their outputs.			*/
/* The last several Codes to enable the ADC1 on the STM and CTC */
/* on the ADC and generate a CTC is to clear the Rcvr Ovfl	*/
/* LED just incase its on, since the ADC will not clear this    */
/* unless it converts (CTC) a signal below the Recvr or ADC     */
/* Ovfl. Hey what can I say it's hardware and has Dick B.'s     */
/* Blessing. Greg B.	12/11/96				*/
/* To fix a DTM glitch problem, (on loading NT cnt a glitch is  */
/* produced that decrements the NT count resulting the the NOISE*/
/* watch-dog error, to get around this glitch on abort load the */
/* NT cnt with Zero and issue RELOAD cmd. A new FPGA fixes the  */
/* problem in hardware as it should. 10/18/97			*/
static long pSafeStateCodes[] = { 
};

static int numSafeStatCodes = sizeof(pSafeStateCodes)/(sizeof(long)*2);

static long pTurnOffSshaCodes[] = {
};

static int numTurnOffSshaCodes = sizeof(pTurnOffSshaCodes)/(sizeof(long)*2);

static long pHaltOpCodes[] = {
};

static int numHaltOpCodes = sizeof(pHaltOpCodes)/(sizeof(long)*2);

startPhandler(int priority, int taskoptions, int stacksize)
{
   int pHandler();

   if (taskNameToId("tPHandlr") == ERROR)
   {
     pHandlerPriority = priority;
     pHandlerTid = taskSpawn("tPHandlr",priority,taskoptions,stacksize,pHandler,
		   pMsgesToPHandlr,2, 3,4,5,6,7,8,9,10);
   
     pHandlerWdTimeout = wdCreate();
     pHandlerTimeout = 0;
   }

}

killPhandler()
{
   int tid;
   if ((tid = taskNameToId("tPHandlr")) != ERROR)
	taskDelete(tid);
}

pHandlerWdISR()
{
   pHandlerTimeout = 1;
   wdCancel(pHandlerWdTimeout);
#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_WDISR,NULL,NULL);
#endif
}

/* 
 * This routine is called from the context of the NDDS task n_rtu7400
 * 
 *   Author: Greg Brissey    8/04/2004
 */
static void ExceptionPub_CallBack(Cntlr_Comm *recvIssue)
{

    /* in the future we might do a few test and call differnet routines but for
       now we just relay the info on to a seperate Task to handle exceptions
       so NDDS can get on with it's business of receiving issue
    */

    /* if this was a exception message initiated from this controller then a msge was
     * already directly sent to the phandler, thus we can safely ignore this message
     */
     if (strcmp(recvIssue->cntlrId,hostName) != 0)
     {
        CNTLR_COMM_MSG excMsg;
        strncpy(excMsg.cntlrId, recvIssue->cntlrId,16);
        excMsg.cmd = recvIssue->cmd;
        excMsg.errorcode = recvIssue->errorcode;
        excMsg.warningcode = recvIssue->warningcode;
        excMsg.arg1 = recvIssue->arg1;
        excMsg.arg2 = recvIssue->arg2;
        excMsg.arg3 = recvIssue->arg3;
        excMsg.crc32chksum = recvIssue->crc32chksum;
        strncpy(excMsg.msgstr,recvIssue->msgstr,COMM_MAX_STR_SIZE);

        /* send error to exception handler task, it knows what to do */
        msgQSend(pMsgesToPHandlr, (char*) &excMsg,
                sizeof(CNTLR_COMM_MSG), NO_WAIT, MSG_PRI_NORMAL);
     }
     else
     {
          DPRINT1(-1,"NDDS callback for Local Exception from '%s', ignored.\n",recvIssue->cntlrId);
     }
}

/*
 * The controller sends it's exception directly to it's phandler via this call
 * the NDDS callback Exception subscription handler will ignore the Exception coming
 * from itself
 */
sendExceptionDirectly(Cntlr_Comm *recvIssue)
{
    CNTLR_COMM_MSG excMsg;
    strncpy(excMsg.cntlrId, recvIssue->cntlrId,16);
    excMsg.cmd = recvIssue->cmd;
    excMsg.errorcode = recvIssue->errorcode;
    excMsg.warningcode = recvIssue->warningcode;
    excMsg.arg1 = recvIssue->arg1;
    excMsg.arg2 = recvIssue->arg2;
    excMsg.arg3 = recvIssue->arg3;
    excMsg.crc32chksum = recvIssue->crc32chksum;
    strncpy(excMsg.msgstr,recvIssue->msgstr,COMM_MAX_STR_SIZE);

    /* send error to exception handler task, it knows what to do */
    msgQSend(pMsgesToPHandlr, (char*) &excMsg,
                sizeof(CNTLR_COMM_MSG), NO_WAIT, MSG_PRI_NORMAL);
}

initExceptionHandler()
{
   /* initialize msgQ used by Master or Controller comm channel */
   pMsgesToPHandlr = msgQCreate(20, sizeof(CNTLR_COMM_MSG), MSG_Q_PRIORITY);

   startPhandler(PHANDLER_PRIORITY, STD_TASKOPTIONS, XSTD_STACKSIZE);

    /* bring NDDS exception Pub/Sub for controller */
    initialExceptionComm((void *)ExceptionPub_CallBack);
}

/*************************************************************
*
*  probHandler - Wait for Messages that indicate some problem
*    with the system. Then perform appropriate recovery.
*
*					Author Greg Brissey 12-7-94
*/
pHandler(MSG_Q_ID msges)
{
   CNTLR_COMM_MSG msge;
   int *val;
   int bytes;
   void recovery(CNTLR_COMM_MSG *, int externflag);

   DPRINT(1,"pHandler :Server LOOP Ready & Waiting.\n");
   FOREVER
   {
     markReady(PHANDLER_FLAGBIT);
     memset( &msge, 0, sizeof( CNTLR_COMM_MSG ) );
     bytes = msgQReceive(pMsgesToPHandlr, (char*) &msge, 
			  sizeof( CNTLR_COMM_MSG ), WAIT_FOREVER);
     markBusy(PHANDLER_FLAGBIT);

     /* if aborted during the MRI read user byte, priorties maybe still high, so reset them. */
     resetParserPriority();   /* use this order to retain priority relationship */
     resetShandlerPriority();

     errLogRet(LOGIT,debugInfo,"vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n");
     errLogRet(LOGIT,debugInfo,"'%s': Exception from: -> '%s' <-, Type: %d, ErrorCode: %d \n",
			hostName, msge.cntlrId, msge.cmd, msge.errorcode);
     errLogRet(LOGIT,debugInfo,"^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");

     /* they are all fatal so stop Spy if it was running */
     if (enableSpyFlag > 0)
     {
        spyReport();   /* report CPU usage */
        spyClkStop();  /* turn off spy clock interrupts, etc. */
     }

     if (strcmp(msge.cntlrId,hostName) == 0)
     {
        recovery( &msge, 0 );
     }
     else
     {
        recovery( &msge, 1 );
     }
     /* prevent false starts from Sync glitches, controller reboots, or FPGA reloads */
     cntrlFifoClearStartMode();
   } 
}

enableInterrupts()
{
}

/*********************************************************************
*
* recovery, based on the cmd code decides what recovery action 
*		to perform.
*
*					Author Greg Brissey 12-7-94
*/
void recovery(CNTLR_COMM_MSG *msge, int externflag)
{
   char *token;
   int len;
   int cmd;
   int i,nMsg2Read;
   int bytes;
   extern int systemConRestart();
   CNTLR_COMM_MSG discardedmsge;

   DPRINT(-1,"Local Exception\n");
   switch( msge->cmd /* exceptionType */ )
   {
      case PANIC:
#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_PANIC,NULL,NULL);
#endif
    		errLogRet(LOGIT,debugInfo,
       		"phandler: Panic Error: %d", msge->errorcode);
		break;

      case WARNING_MSG:
#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_WARNMSG,NULL,NULL);
#endif
  	        DPRINT2(-10,"WARNING: doneCode: %d, errorCode: %d\n",
			msge->cmd,msge->errorcode);
    		/* errLogRet(LOGIT,debugInfo,
       		"phandler: Warning Message: %d", msge->reportEvent); */

		/* the send2Expproc is commented out. The msg is send 
		/* via other ways. We got two warning msgs for the 
		/* following numbers:
        	/* PNEU_ERROR + PRESSURE;
         	/* PNEU_ERROR + NB_STACK;
         	/* PNEU_ERROR + VTTHRESH;
         	/* PNEU_ERROR + PS;
		/* VTERROR + NOGAS
                /* Don't send ADC overflows to Expproc */
                /* if ( (msge->errorcode != WARNINGS+ADCOVER) && 
                /*     (msge->errorcode != SFTERROR+LOCKLOST) )
                /*   send2Expproc(CASE,msge->cmd,msge->errorcode,0,NULL,0); 
                 */
		break;

      case SOFT_ERROR:
#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_SOFTERROR,NULL,NULL);
#endif
		/* update_acqstate( ACQ_IDLE ); INOVA */

  	        DPRINT2(0,"SOFT_ERROR: doneCode: %d, errorCode: %d\n",
			msge->cmd,msge->errorcode);
    		/* errLogRet(LOGIT,debugInfo,
       		"phandler: Soft Error: %d", msge->reportEvent); */
#ifdef INOVA
                statMsg.Status = msge->exceptionType;
                statMsg.Event = msge->reportEvent; /* Error Code */
		msgQSend(pMsgesToHost,(char*) &statMsg,sizeof(statMsg),
					WAIT_FOREVER, MSG_PRI_NORMAL);
#endif
		break;

      case EXP_ABORTED:
#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_EXPABORTED,NULL,NULL);
#endif
  	        DPRINT(0,"Exp. Aborted");

      case HARD_ERROR:

#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_HARDERROR,NULL,NULL);
#endif
  	        DPRINT2(0,"HARD_ERROR: doneCode: %d, errorCode: %d\n", msge->cmd, msge->errorcode);

		AbortExp(msge,externflag);

		/* fifoCloseLog(pTheFifoObject); */
  	        DPRINT(0,"Done");
		break;
		

      case INTERACTIVE_ABORT:

#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_HARDERROR,NULL,NULL);
#endif
/*
  	        DPRINT1(0,"INTERACTIVE_ABORT: doneCode: %d, errorCode: %d\n", msge->reportEvent);
*/

		AbortExp(msge,externflag);

		/* fifoCloseLog(pTheFifoObject); */
  	        DPRINT(0,"Done");
		break;


      case EXP_HALTED:
#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_EXPHALTED,NULL,NULL);
#endif
  	        DPRINT2(0,"EXP_HALTED: doneCode: %d, errorCode: %d\n", msge->cmd, msge->errorcode);

		AbortExp(msge,externflag);

		break;


      case STOP_CMPLT:
                /* the monitor set the SA_Criteria value,
		   when the upLinker obtained a FID that meet this
		   Criteria the upLinker sent this msge here and
		   then blocked itself
                  Our job is to
	          1. Foward the Stopped msge to Host
	          2. reset hardware
                  3. put hardware into a safe state
		  4. call stmSA, clears upLinker msgQ, send SA msge
	          5. reset SA_Criteria back to Zero
		  6. give the semaphore to restart upLinker
		  7. reset update task
		  8. reset parser task
	          9. Free the buffers
	         10. Re-enable Interrupts 
		*/

#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_STOPCMPLT,NULL,NULL);
#endif
#ifdef INOVA
                statMsg.Status = msge->exceptionType;
                statMsg.Event = msge->reportEvent; /* Error Code */
		msgQSend(pMsgesToHost,(char*) &statMsg,sizeof(statMsg),
					WAIT_FOREVER, MSG_PRI_NORMAL);

    		errLogRet(LOGIT,debugInfo,
       		"phandler: Exp. Stopped: %d", msge->reportEvent);

                /* reprogram HSlines, ap registers to safe state */
                reset2SafeState( msge->exceptionType );  

                resetSystem();  /* reset fifo, disable intrps, reset tasks & buffers */

  	        DPRINT2(0,"stmHaltCode: doneCode: %d, errorCode: %d\n",
			msge->exceptionType,msge->reportEvent);

                semGive(pSemSAStop);   /* release the UpLinker */
		/* stmHaltCode(pTheStmObject,(int) msge->exceptionType, 
				(int) msge->reportEvent);
		*/
  	        /* DPRINT(0,"stmSA"); stmSA(pTheStmObject); */
   		DPRINT(0,"STOP_CMPLT: lower priority below upLinker\n");
                taskPrioritySet(pHandlerTid,(UPLINKER_TASK_PRIORITY+1));  
   	        taskPrioritySet(pHandlerTid,pHandlerPriority);  

		SA_Criteria = 0;
		SA_Mod = 0L;

  	        DPRINT(0,"wait4DowninkerReady");
		wait4DownLinkerReady();	/* let downlinker become ready, then delete any left over buffers */

                resetNameBufs();   /* free all named buffers */

   		/* stmInitial(pTheStmObject, 1, 1024, pUpLinkMsgQ, 0); */

	        clrDwnLkAbort();

                wait4SystemReady(); /* pend till all activities are complete */

  	        DPRINT(0,"enableInterrupts");
		enableInterrupts();

	        /* now inform Expproc System is Ready */
                statMsg.Status = SYSTEM_READY; /* Console Ready  */
                statMsg.Event = 0; /* Error Code */
		msgQSend(pMsgesToHost,(char*) &statMsg,sizeof(statMsg),
					WAIT_FOREVER, MSG_PRI_NORMAL);

		update_acqstate( ACQ_IDLE );
	        getstatblock();
/*
		if (pTheTuneObject != NULL)
		  semGive( pTheTuneObject->pSemAccessFIFO );
*/

#endif
                sendSysReady();
		break;

      case LOST_CONN:
#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_LOSTCONN,NULL,NULL);
#endif
#ifdef INOVA
    		errLogRet(LOGIT,debugInfo,
       		"phandler: Host Closed Connection to Console: %d", msge->reportEvent);
		update_acqstate( ACQ_IDLE );
		/* stmItrpDisable(pTheStmObject, STM_ALLITRPS); */
                storeConsoleDebug( SYSTEM_ABORT );
                /* reprogram HSlines, ap registers to safe state */
                reset2SafeState( msge->exceptionType );
                resetSystem();  /* reset fifo, disable intrps, reset tasks & buffers */
		/* stmReset(pTheStmObject); */
                taskDelay(calcSysClkTicks(332));  /* taskDelay(20); */
                resetNameBufs();   /* free all named buffers */
		enableInterrupts();
/*
		if (pTheTuneObject != NULL)
		  semGive( pTheTuneObject->pSemAccessFIFO );
*/
     		taskSpawn("tRestart",50,0,2048,systemConRestart,NULL,
				2,3,4,5,6,7,8,9,10);
	        clrDwnLkAbort();

#endif
	        break;

      case ALLOC_ERROR:
#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_ALLOCERROR,NULL,NULL);
#endif
#ifdef INOVA
    		errLogRet(LOGIT,debugInfo,
       		"phandler: Memory Allocation Error: %d", msge->reportEvent);

		update_acqstate( ACQ_IDLE );

                statMsg.Status = HARD_ERROR; /* HARD_ERROR */
                statMsg.Event = msge->reportEvent; /* Error Code */
		msgQSend(pMsgesToHost,(char*) &statMsg,sizeof(statMsg),
					WAIT_FOREVER, MSG_PRI_URGENT);
                

		/* report HARD_ERROR to Recvproc */
  	        DPRINT2(0,"stmHaltCode: doneCode: %d, errorCode: %d\n",
			HARD_ERROR,msge->reportEvent);
/*
		stmHaltCode(pTheStmObject,(int) HARD_ERROR, 
				(int) msge->reportEvent);
*/

                /* reprogram HSlines, ap registers to safe state */
                reset2SafeState( msge->exceptionType );

                resetSystem();  /* reset fifo, disable intrps, reset tasks & buffers */

	      
#ifdef XXXX
                statMsg.Status = WARNING_MSG;
                statMsg.Event = WARNINGS + MEM_ALLOC_ERR; /* Error Code */
		msgQSend(pMsgesToHost,(char*) &statMsg,sizeof(statMsg),
					WAIT_FOREVER, MSG_PRI_NORMAL);
#endif

                resetNameBufs();   /* free all named buffers */

   		/* stmInitial(pTheStmObject, 1, 1024, pUpLinkMsgQ, 0); */

	        clrDwnLkAbort();

  	        DPRINT(0,"enableInterrupts");
		enableInterrupts();

	        /* now inform Expproc System is Ready */
                statMsg.Status = SYSTEM_READY; /* Console Ready  */
                statMsg.Event = 0; /* Error Code */
		msgQSend(pMsgesToHost,(char*) &statMsg,sizeof(statMsg),
					WAIT_FOREVER, MSG_PRI_NORMAL);

	        getstatblock();
/*
		if (pTheTuneObject != NULL)
		  semGive( pTheTuneObject->pSemAccessFIFO );
*/

#endif
		break;
      
      case WATCHDOG:
#ifdef INOVA
#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_WATCHDOG,NULL,NULL);
#endif
		/* Giv'm a bone */
    		errLogRet(LOGIT,debugInfo,
       		"phandler: Watch Dog: %d", msge->reportEvent);
#endif
		break;

      default:
		/* Who Cares */
    		errLogRet(LOGIT,debugInfo,
       		"phandler: Invalid Exception Type: %d, Event: %d", 
			msge->cmd, msge->errorcode);
		break;
   }
/*
   nMsg2Read = msgQNumMsgs(pMsgesToPHandlr);
   DPRINT1(-1,"Message in Q: %d\n",nMsg2Read);
*/
   if (msge->cmd != WARNING_MSG)
      resumeLedShow(); /* resume LED Idle display */

   while ( (bytes = msgQReceive(pMsgesToPHandlr, (char*) &discardedmsge, sizeof( CNTLR_COMM_MSG ), NO_WAIT)) != ERROR )
   {
     DPRINT(-1,"vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n");
     DPRINT1(0,"Read %d bytes from Phandler msgQ, (Bogus Errors at this point)\n",bytes);
     DPRINT4(-1,"'%s': Exception from: -> '%s' <-, Type: %d, ErrorCode: %d \n",
			hostName, discardedmsge.cntlrId, discardedmsge.cmd, discardedmsge.errorcode);
     DPRINT(-1,"^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");


   }
}

static
void AbortExp(CNTLR_COMM_MSG *msge, int externflag)
{
    int result;
    char cntlrList[256];

   /* donecode = EXP_ABORTED or  HARD_ERROR plus errorCode(FOO,etc..) */
   /* need to get this msge up to expproc as soon as possible since the
      receipt of this msge cause expproc to send the SIGUSR2 (abort cmd)
      to Sendproc, without this msge all the acodes will be sent down.
   */

   /* This controller is sending the exception and fail has yet to be asserted */
   if ( (externflag == 0) && (failAsserted != 1) )
   {
      DPRINT(-1,"AbortExp: Asserting FalureLine\n");
      assertFailureLine();  /* assert failure, if this is the controller causing 
			     * error and not already asserted */
   }
     
   /* EXP_ABORT or HARD_ERROR */
   /*
   * statMsg.Status = (msge->exceptionType != INTERACTIVE_ABORT) ? msge->exceptionType : EXP_ABORTED; 
   * statMsg.Event = msge->reportEvent; /* Error Code */

#ifdef INOVA
   if (msge->reportEvent == (HDWAREERROR + STMERROR))
      reportNpErr(pTheStmObject->npOvrRun, 1 + pTheStmObject->activeIndex);
#endif

   /* stop lock/spinvt error/warning msgs */
   setLkInterLk(0);
   setVTinterLk(0);
   setSpinInterlk(0);

   /* ------------------------------------------------- */
   /* send abort message to Expproc */
   send2Expproc(CASE,msge->cmd,msge->errorcode,0,NULL,0); 

   /* send tickcount & active channel reset info to Console_Stat */
   sendInfo2ConsoleStat(0);

   /* Controller output from FIFO to Software registers */
   /* setFifoOutputSelect(SELECT_SW_CONTROLLED_OUTPUT); no longer needed, as per Debbie */

   /* force values that must serialized out to whomever */
   /* setAbortSerializedStates();  no longer needed, as per Debbie */

   /* clear Experiment downld codes,patterns,tables, etc.
    * and restart parser
    *
    */
    DPRINT(-1,"restart AParser\n");
    /* several things that could have the parser Suspended */
    givePrepSem();      /* only the master, go('prep') semaphore */
    /* giveRoboAckSem();   /* just a master */
    abortRoboAckMsgQ(); /* send sample change Ack msg with abort to release parser if pended */
    giveParseSem();
    parseCntDownReset();

    AParserAA();

    /* resetExpBufsAndParser(); replaced with AParserAA above */

    DPRINT(-1,"Waiting for Parser to be ready\n");
    wait4ParserReady(WAIT_FOREVER);

   /* ------------------------------------------------- */
   resetTnEqualsLkState();		// recover if we had tn='lk'
   /* ------------------------------------------------- */
   if ( pTheVTObject != NULL) 
      semGive(pTheVTObject->pVTRegSem);  // stop wainting for VT
   					 // liq spinner is different
   
   /* ------------------------------------------------- */

   /* Disable DMA, and clear any Queued DMA transfers */
   abortFifoBufTransfer();

   /* reset FIFO */
   cntrlFifoReset();

   /* ------------------------------------------------- */

    freeAllDwnldBufsSynced();   /* free download buffer after dwnld completion msg recv'd */

   DPRINT(-1,"wait4CnltrsReady");

   /* cntlrStatesCmp(CNTLR_EXCPT_CMPLT, 5); /* wait for controller to become ready for 5 sec max */
   result = cntlrStatesCmpList(CNTLR_EXCPT_CMPLT, 30, cntlrList);  /* 1/2 minute to complete */
   if (result > 0)
   {
        errLogRet(LOGIT,debugInfo,
			"AbortExp(): Master Timed Out on Cntlrs reporting Exception Complete.\n");
    	errLogRet(LOGIT,debugInfo, "ABortExp():   Controllers: -> '%s' <- FAILED  to report Exception Completed \n",
			cntlrList);
   }
   DPRINT(-1,"CnltrsAreReady");

   /* int cntlrStatesCmp(READYANDIDLE, WAIT_FOREVER or time in ticks) */
   /* wait4SystemReady(); /* pend till all activities are complete */

   /* send2Expproc(CASE,msge->cmd,msge->errorcode,0,NULL,0);  */

   send2Expproc(CASE,SYSTEM_READY,0,0,NULL,0); /* sendSysReady(); */

   setFidCtState(0, 0);
   setAcqState(ACQ_IDLE);

   if (pSemOK2Tune != NULL)
       semGive(pSemOK2Tune);     /* now allow tuning */

/*
   if ( msge->exceptionType  == EXP_ABORTED)
        storeConsoleDebug( SYSTEM_ABORT );
*/

   /* reprogram HSlines, ap registers to safe state */

#ifdef INOVA
   reset2SafeState( msge->exceptionType );
   resetSystem();  /* reset fifo, disable intrps, reset tasks & buffers */

   if (msge->reportEvent == (SFTERROR + MAXCT))
   {
       /* if maxsum error reset the stm */
       /* obtain lock on stm, i.e. not transfer data prior to reseting stm */
       /* only after wait for the stm to functional again do we release the lock */
   }

   DPRINT(0,"wait4DowninkerReady");
   wait4DownLinkerReady();    /* let downlinker become ready, then delete any left over buffers */

   resetNameBufs();   /* free all named buffers */

   DPRINT(0,"wait4SystemReady");
   wait4SystemReady(); /* pend till all activities are complete */
   DPRINT(0,"SystemReady");
   clrDwnLkAbort();

   DPRINT(0,"enableInterrupts");

   /* now inform Expproc System is Ready */
   statMsg.Status = SYSTEM_READY; /* Console Ready  */
   statMsg.Event = 0; /* Error Code */
   msgQSend(pMsgesToHost,(char*) &statMsg,sizeof(statMsg),
	WAIT_FOREVER, MSG_PRI_NORMAL);
   update_acqstate( ACQ_IDLE );
   getstatblock();
   
/*
   if (pTheTuneObject != NULL)
      semGive( pTheTuneObject->pSemAccessFIFO );
*/
#endif

   return;
}


static
void resetSystem()
{
#ifdef INOVA
   DPRINT(0,"AParserAA");
   AParserAA();

   /* PARALLEL_CHANS */
   freeSorter();

   DPRINT(0,"AupdtAA");
   AupdtAA();
   DPRINT(0,"ShandlerAA");
   ShandlerAA();
   DPRINT(0,"fifoStufferAA");
   /* fifoStufferAA(pTheFifoObject); */

   /* stmAdcOvldClear(pTheStmObject); */
   /* stmReset(pTheStmObject); */
   DPRINT(0,"Disable ADC Intrps");
   /* adcItrpDisable(pTheAdcObject,ADC_ALLITRPS); */
   /* adcReset(pTheAdcObject); */

   /* reset fifo Object not to use parallel channels */
   /* fifoClearPChanId(pTheFifoObject); */

   /* Reset Safe State again after tasks restarted */
   /* call to reset2SafeState removed; in each case where resetSystem */
   /* is called, reset2SafeState is called immediately previously.    */

/* it maybe safe here, but we have to flip flop priorities with the Stuffer */
   DPRINT2(0,"resetSystem: tid: 0x%lx, priority: %d\n",pHandlerTid,pHandlerPriority);
/*
   taskPrioritySet(pHandlerTid,(FIFO_STUFFER_PRIORITY+1));  
   setlksample();
   set2khz();
   taskPrioritySet(pHandlerTid,pHandlerPriority);  
*/
#endif
   return;
}

/*
   WARNING: Not to be used by any other tasks than phandler
     1. Stop Parser
     2. Stop DownLinker
     3. Stuff Fifo with safe states and run
     4. Give-n-Take Serial Port devices Mutex to allow serial cmd to finish
     5. Free Name buffers
     6. Raise Priority of DownLink to above phandler and resume task if suspend & wait, lower priority
     7. Raise Priority of Parser to above phandler and resume task if suspend & wait, lower priority
     Last three steps allows for an orderly and efficient clean up of resources (name buffers,etc.)
*/

/* reprogram HSlines, ap registers to safe state */

static void
reset2SafeState( int why )  /* argument is the type of exception */
{
   int pTmpId, TmpPrior;
   int tuneactive = 0;
#ifdef INOVA
   DPRINT(1,"reset2SafeState");
   DPRINT(1,"stopAPint");
   /* pcsAbortSort();     /* parallel channel sort to stop and return to APint() */
   /* abortAPint();   /* like stopAPint() but also cause the Parser to suspend itself */
   /* stopAPint(); */
   DPRINT(1,"stop downLinker");
   /* setDwnLkAbort();   /* like stop downLinker and set to dump further download to bit bucket */
   DPRINT(1,"reset2SafeState");
   DPRINT(0,"fifoSetNoStuff\n");
   /* fifoSetNoStuff(pTheFifoObject);    /* stop the stuffer in it's tracks */
   DPRINT(0,"Disable STM Intrps");
   /* stmItrpDisable(pTheStmObject, STM_ALLITRPS); */
   /* reset fifo Object not to use parallel channels */
   /* fifoClearPChanId(pTheFifoObject); */
   DPRINT(0,"fifoReset");

   /* Keep the MTS gradient amp disabled whenever possible! */
   /* gpaTuneSet(pTheAutoObject, SET_GPAENABLE, GPA_DISABLE_DELAY); */

   /* if an interactive abort do not reset the apbus, thus preventing the 
      all amplifier from going to Pulse Mode, thus amps that are in CW 
      will stay that way. This is/was a lockdisplay/shimming issue.
      Where exiting lock display set all the amps to Pulse Mode or blanked!, 
      then one shimmed and obtain a certain lock level. After su command 
      then amps would go back to CW thus injecting noise and lowing the
      lock level, If use wht directly to shimming they could neer re-attain 
      the lock level they had before.  But when enter and exit lock display 
      the level would jump back up. Thus causing no end of
      confusion....
      Thus for INTERACTIVE_ABORT the reset apbus bit is left out when reseting the FIFO.

      Oooops, but wait, we found an exception (of course), when exiting qtune the 
      reset of the apbus reset all the tune relays back to noraml, now of course 
      this does not happen thus leaving the console in tuning mode. Thus we added the 
      following test isTuneActive() and if it is then a full reset including the apbus
      is done.
   */
#ifdef INOVA
   tuneactive = isTuneActive();
   /* DPRINT1(-3,"Is Tune active: %d\n",tuneactive); */
      fifoReset();  /* this now clear the Fifo Buffer & restart the Stuffer */
   /* set initial value of parallel channel free buffer pointer to null */
   clearParallelFreeBufs();
#endif

#ifdef INOVA
   DPRINT(0,"Stuff FIFO DIRECTLY with Safe State & Run fifo");
   if (numSafeStatCodes > 0)
   {
      /*   WARNING: Does not pend for FF Almost Full, BEWARE !! */
      /*   Should only be used in phandler's reset2SafeState() */
      fifoStuffCode(pTheFifoObject, pSafeStateCodes, numSafeStatCodes);
      fifoStuffCode(pTheFifoObject, pTurnOffSshaCodes, numTurnOffSshaCodes);
      fifoStuffCode(pTheFifoObject, pHaltOpCodes, numHaltOpCodes);
      fifoStart(pTheFifoObject);
      fifoBusyWait4Stop(pTheFifoObject);  /* use BusyWait don't what any other lower priority tasks to run ! */
	DPRINT(1,"FIFO wait for stop completes in reset to safe state\n" );
   }
   activate_ssha();
   DPRINT(0,"getnGiveShimMutex\n");  
   /* allow any pending serial Shim commands to finish */
   getnGiveShimMutex();
   DPRINT(0,"getnGiveVTMutex\n");
   /* allow any pending serial VT commands to finish */
   getnGiveVTMutex();

/* Now Bump Priority of DownLiner so that it can Dump
   the remaining Data that SendProc wants to send it */

/* If we lost the connection to the host computer, the downlinker is
   is not going to get any more data; nor should this task (problem
   handler) wait for the downlinker to become ready.    April 1997  */

   DPRINT(0,"dlbFreeAllNotLoading Dyn");
   dlbFreeAllNotLoading(pDlbDynBufs);
   DPRINT(0,"dlbFreeAllNotLoading Fix");
   dlbFreeAllNotLoading(pDlbFixBufs);
   if (why != LOST_CONN)
   {
      pTmpId = taskNameToId("tDownLink");
      taskPriorityGet(pTmpId,&TmpPrior);
      taskPrioritySet(pTmpId,(pHandlerPriority-1));  

      /* downLinker has now A. ran and is suspended, 
			    B. Done and in READY state or
	 		    C. still waiting in a read of a socket
      */

      /* start wdog timeout of 20 seconds */ 
      wdStart(pHandlerWdTimeout,sysClkRateGet() * 20, pHandlerWdISR, 0);

      /* if it is busy and not suspend then wait till 
      /* it is Ready or Suspended, or Time-Out */
      while (downLinkerIsActive() &&  !taskIsSuspended(pTmpId) && !pHandlerTimeout)
            taskDelay(calcSysClkTicks(17));  /* wait one tick , 16msec */

      wdCancel(pHandlerWdTimeout);

      if (taskIsSuspended(pTmpId))
      {
        taskResume(pTmpId);
        wait4DownLinkerReady();
      }
      taskPrioritySet(pTmpId,TmpPrior);
      DPRINT(0,"downlinker resynchronized\n");
   }
   DPRINT(0,"dlbFreeAll Dyn");
   dlbFreeAll(pDlbDynBufs);
   DPRINT(0,"dlbFreeAll Fix");
   dlbFreeAll(pDlbFixBufs);

   /* 
     Now Bump Priority of Parser so that it can Finish Up  
     We don't have to worry as much about the state of the parser
     since it will completely restarted very soon. 
   */

   pTmpId = taskNameToId("tParser");
   taskPriorityGet(pTmpId,&TmpPrior);
   taskPrioritySet(pTmpId,(pHandlerPriority-1));  
   if (taskIsSuspended(pTmpId))
   {
     taskResume(pTmpId);
     wait4ParserReady(WAIT_FOREVER);
   }
   taskPrioritySet(pTmpId,TmpPrior);
   resetAPint();
   DPRINT(0,"A-code parser resynchronized\n");

   /* Parser might of made a buffer ready to stuff, so better clear it just incase */
   if (why != INTERACTIVE_ABORT)
      fifoReset(pTheFifoObject, RESETFIFOBRD);  /* this now also clear the Fifo Buffer & restart the Stuffer */
   else
      fifoReset(pTheFifoObject, RESETFIFO | RESETSTATEMACH | RESETTAGFIFO | RESETAPRDBKFIFO ); /* no apbus reset */
#endif
#endif
   
   return;
}

/*----------------------------------------------------------------------*/
/* resetNameBufs							*/
/*	Removes named buffers.  Lowers priority in case downlinker is	*/
/*	pending on new buffers (e.g. interleaving). Waits then removes	*/
/*	some more.							*/
/*----------------------------------------------------------------------*/
static
void resetNameBufs()   /* free all named buffers */
{
   int retrys;
   int callTaskId,DwnLkrTaskId;
   int callTaskPrior;

   /* Since reset2SafeState should have left the downLinker in a ready
      state we now longer need change priority/etc. here to get it to clean
      up.     8/6/97
   */

   retrys=0;

   /* start wdog timeout of 7 seconds */ 
#ifdef INOVA
   pHandlerTimeout = 0;
   wdStart(pHandlerWdTimeout,sysClkRateGet() * 7, pHandlerWdISR, 0);

   /* free buffers until all are free or watch-dog timeout has occurred */
   while ( ((dlbUsedBufs(pDlbDynBufs) > 0) || (dlbUsedBufs(pDlbFixBufs) > 0)) &&
		(!pHandlerTimeout) )
   {
        retrys++;
   	DPRINT(0,"dlbFreeAll Dyn");
   	dlbFreeAll(pDlbDynBufs);
   	DPRINT(0,"dlbFreeAll Fix");
   	dlbFreeAll(pDlbFixBufs);
	Tdelay(1);
	DPRINT1(0,"resetNameBufs: retrys: %d\n",retrys);
   }
   wdCancel(pHandlerWdTimeout);
#endif
}

/* programs safe HS line, turns off WFG, etc, set lock filters,etc.. */
/* WARNING this function will changed the calling task priority if it is
   lower than the Stuffer, to greater than the stuffer while putting fifo words
   into the fifo buffer 
*/
void set2ExpEndState()  /* reprogram HSlines, ap registers to safe state, Normal end */
{
   int callTaskId;
   int callTaskPrior;
   DPRINT(0,"set2ExpEndState");

#ifdef INOVA
   /* Keep the MTS gradient amp disabled whenever possible! */
   gpaTuneSet(pTheAutoObject, SET_GPAENABLE, GPA_DISABLE_DELAY);

   DPRINT(0,"Stuff FIFO through Normal channels with Safe State & Run fifo");
   fifoResetStufferFlagNSem(pTheFifoObject);
   
   callTaskId = taskIdSelf();
   taskPriorityGet(callTaskId,&callTaskPrior);
   /* Lower priority to allow stuffer in, if needed  */
   /* if priority <= to that of the stuffer then lower the priority of this task */
   /* thus allowing the stuffer to stuff the fifo */
   DPRINT2(0,"set2ExpEndState: tid: 0x%lx, priority: %d\n",callTaskId,callTaskPrior);
   setlksample();
   set2khz();
   activate_ssha();
   /* reset priority back if changed */
   if (callTaskPrior <= FIFO_STUFFER_PRIORITY)
   {
      DPRINT(0,"Set priority Back \n");
      taskPrioritySet(callTaskId,callTaskPrior);  
   }
#endif
   return;
}

sendSysReady()
{
   send2Expproc(CASE,SYSTEM_READY,0,0,NULL,0); 
}
