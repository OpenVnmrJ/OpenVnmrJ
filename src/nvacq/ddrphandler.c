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
8-5-04,gmb  created 
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
#include "sysflags.h"
#include "instrWvDefines.h"
#include "Cntlr_Comm.h"
#include "Monitor_Cmd.h"
#include "upLink.h"
#include "fifoFuncs.h"
#include "cntlrStates.h"
#include "nexus.h"
#include "dataObj.h"

#define HOSTNAME_SIZE 80
extern char hostName[HOSTNAME_SIZE];

extern SEM_ID  pSemSAStop; /* Binary  Semaphore used to Stop upLinker for SA */

/* extern MSG_Q_ID pUpLinkMsgQ;	/* MsgQ used between UpLinker and STM Object */
extern MSG_Q_ID pMsgesToPHandlr;/* MsgQ for Msges to Problem Handler */
extern MSG_Q_ID pDataTagMsgQ;   /* MsgQ used for Sending msg to recvproc upLink task */

extern int failAsserted;

extern DATAOBJ_ID pTheDataObject;   /* FID statblock, etc. */

Monitor_Cmd ExceptionMsg =  { 0, 0, 0, 0, 0, 0, 0, 0 } ; 

extern RING_ID  pSyncActionArgs;  /* Buffer for 'Sync Action' (e.g. SETVT) function args */

extern int enableSpyFlag;  /* if > 0 then invoke the spy routines to monitor CPU usage  11/9/05 GMB */

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
   if ((tid = taskNameToId("tEHandlr")) != ERROR)
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
     if (strncmp(recvIssue->cntlrId,hostName,16) != 0)
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
*  exceptionHandler - Wait for Messages that indicate some problem
*    with the system. Then perform appropriate recovery.
*
*					Author Greg Brissey 12-7-94
*/
pHandler(MSG_Q_ID msgQObj)
{
   CNTLR_COMM_MSG msge;
   int *val;
   int bytes;
   void recovery(CNTLR_COMM_MSG *, int externflag);
   void extrecovery(CNTLR_COMM_MSG *, int externflag);

   DPRINT(1,"pHandler :Server LOOP Ready & Waiting.\n");
   FOREVER
   {
     markReady(PHANDLER_FLAGBIT);
     memset( &msge, 0, sizeof( CNTLR_COMM_MSG ) );
     bytes = msgQReceive(pMsgesToPHandlr, (char*) &msge, 
			  sizeof( CNTLR_COMM_MSG ), WAIT_FOREVER);
     markBusy(PHANDLER_FLAGBIT);

     /* if aborted during the MRI read user byte, priorties maybe still high, so reset them. */
     resetParserPriority();
     resetShandlerPriority();

     DPRINT(-1,"vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n");
     DPRINT4(-1,"'%s': Exception from: -> '%s' <-, Type: %d, ErrorCode: %d \n",
			hostName, msge.cntlrId, msge.cmd, msge.errorcode);
     DPRINT(-1,"^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");

     /* they are all fatal so stop Spy if it was running */
     if (enableSpyFlag > 0)
     {
        spyReport();   /* report CPU usage */
        spyClkStop();  /* turn off spy clock interrupts, etc. */
     }

     /* resumeLedShow(); /* resume LED Idle display */

     if (strncmp(msge.cntlrId,hostName,16) == 0)
     {
        recovery( &msge, 0 );
     }
     else
     {
        extrecovery( &msge, 1 );
     }
     /* prevent false starts from Sync glitches, controller reboots, or FPGA reloads */
     if (msge.cmd != WARNING_MSG) cntrlFifoClearStartMode();

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
   PUBLSH_MSG pubMsg;

   DPRINT(-1,"Local Exception\n");
   switch( msge->cmd /* exceptionType */ )
   {

      case WARNING_MSG:
#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_WARNMSG,NULL,NULL);
#endif
  	        DPRINT2(-1,"WARNING: doneCode: %d, errorCode: %d\n",
			msge->cmd,msge->errorcode);

                pubMsg.tag = msge->warningcode; /* the tagId of FID we are on */
                pubMsg.donecode  = msge->cmd;   /* WARNING_MSG */
                pubMsg.errorcode  = msge->errorcode;
                pubMsg.crc32chksum  = 0;
                msgQSend(pDataTagMsgQ, (char *)(&pubMsg),sizeof(PUBLSH_MSG),
                          WAIT_FOREVER,MSG_PRI_URGENT);
		break;

      case HARD_ERROR:

#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_HARDERROR,NULL,NULL);
#endif
  	        DPRINT2(-10,"HARD_ERROR: doneCode: %d, errorCode: %d\n", msge->cmd, msge->errorcode);

		AbortExp(msge,externflag);

  	        DPRINT(0,"Done");
		break;
		
      case EXP_HALTED:
#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_EXPHALTED,NULL,NULL);
#endif

        /* This would tenitively be where the DDR has decided that */
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
        /* This would tenitively be where the DDR has decided that */
        /* the stop criteria has been meet  */
        break;

      case ALLOC_ERROR:
#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_ALLOCERROR,NULL,NULL);
#endif
          DPRINT(-1,"Memory allocation Failure\n");
		break;
      default:
		/* Who Cares */
    		errLogRet(LOGIT,debugInfo,
       		"phandler: Invalid Exception Type: %d, Event: %d", 
			msge->cmd, msge->errorcode);
		break;
    }

    if (msge->cmd != WARNING_MSG)
      resumeLedShow(); /* resume LED Idle display */


}


/*********************************************************************
*
* external exception recovery, based on the cmd code decides what recovery action 
*		to perform.
*
*					Author Greg Brissey 8-5-2004
*/
void extrecovery(CNTLR_COMM_MSG *msge, int externflag)
{
   char *token;
   int len;
   int cmd;
   int i,nMsg2Read;
   int bytes;
   PUBLSH_MSG pubMsg;
   extern int systemConRestart();

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
            if ((externflag == 0) || 
                ((strncmp(msge->cntlrId,"ddr",3) != 0) && (strncmp(hostName,"ddr1",16) == 0)) )
            {
#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_WARNMSG,NULL,NULL);
#endif
  	        DPRINT2(-1,"WARNING: doneCode: %d, errorCode: %d\n",
			msge->cmd,msge->errorcode);

                pubMsg.tag = msge->warningcode; /* the tagId of FID we are on */
                pubMsg.donecode  = msge->cmd;   /* WARNING_MSG */
                pubMsg.errorcode  = msge->errorcode;
                pubMsg.crc32chksum  = 0;
                msgQSend(pDataTagMsgQ, (char *)(&pubMsg),sizeof(PUBLSH_MSG),
                          WAIT_FOREVER,MSG_PRI_URGENT);
             }
	     break;

   
      case SOFT_ERROR:
#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_SOFTERROR,NULL,NULL);
#endif

  	        DPRINT2(0,"SOFT_ERROR: doneCode: %d, errorCode: %d\n",
			msge->cmd,msge->errorcode);
		break;

      case EXP_ABORTED:
#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_EXPABORTED,NULL,NULL);
#endif
  	        DPRINT(0,"Exp. Aborted");
                /* drops down into the HARD_ERROR case */

      case HARD_ERROR:

#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_HARDERROR,NULL,NULL);
#endif
  	        DPRINT2(0,"HARD_ERROR: doneCode: %d, errorCode: %d\n", msge->cmd, msge->errorcode);

		AbortExp(msge, externflag);

  	        DPRINT(0,"Done");
		break;
		

      case INTERACTIVE_ABORT:
#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_HARDERROR,NULL,NULL);
#endif
		break;


      case EXP_HALTED:
#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_EXPHALTED,NULL,NULL);
#endif
  	        DPRINT2(-10,"EXP_HALTED: doneCode: %d, errorCode: %d\n", msge->cmd, msge->errorcode);

		AbortExp(msge, externflag);

  	        DPRINT(0,"Done");
		break;
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

               /* set up some critriea to stop acquisition at some point? */
		/* SA_Criteria = 0; SA_Mod = 0L; */
		break;


      case ALLOC_ERROR:
#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_ALLOCERROR,NULL,NULL);
#endif
		break;
      
      case WATCHDOG:
		break;

      default:
		/* Who Cares */
    		errLogRet(LOGIT,debugInfo,
       		"phandler: Invalid Exception Type: %d, Event: %d", 
			msge->cmd, msge->errorcode);
		break;
   }

   if (msge->cmd != WARNING_MSG)
      resumeLedShow(); /* resume LED Idle display */

/*
   nMsg2Read = msgQNumMsgs(pMsgesToPHandlr);
   DPRINT1(-1,"Message in Q: %d\n",nMsg2Read);
*/
/*  INOVA
   while ( (bytes = msgQReceive(pMsgesToPHandlr, (char*) &msge, sizeof( CNTLR_COMM_MSG ), NO_WAIT)) != ERROR )
   {
     DPRINT1(0,"Read %d bytes from Phandler msgQ, (Bogus Errors at this point)\n",bytes);
   }
*/
}

static
void AbortExp(CNTLR_COMM_MSG *msge, int externflag)
{
   PUBLSH_MSG pubMsg;
   int stat,result;
   int type,num,encodedTypenNum;

   /* donecode = EXP_ABORTED or  HARD_ERROR plus errorCode(FOO,etc..) */
   /* need to get this msge up to expproc as soon as possible since the
      receipt of this msge cause expproc to send the SIGUSR2 (abort cmd)
      to Sendproc, without this msge all the acodes will be sent down.
   */
   /* EXP_HALTED, EXP_ABORT or HARD_ERROR */
/*
   if (msge->reportEvent == (HDWAREERROR + STMERROR))
      reportNpErr(pTheStmObject->npOvrRun, 1 + pTheStmObject->activeIndex);
*/

   if ((externflag == 0) && (failAsserted != 1))
   {
      DPRINT(-1,"AbortExp: Asserting FalureLine\n");
      assertFailureLine();  /* assert failure, if this is the controller causing 
			     * error and not already asserted */
   }
     
   clearUpLinkMsgs();
   abortTransferLimitWait();  /* release uplint from transferLimit pending condition */

   cntlrName2TypeNNum(msge->cntlrId, &type, &num);
   encodedTypenNum = ((type & 0xF) << 12) | (num & 0xFFF);

   /* send error up to Recvpro conly if this controller is the one issuing the exception */
   /*  changed this 10/14/05   GMB  */
   DPRINT4(+2,"externflag: %d, cntrlID: '%s', strncmp(msge->cntlrId,'ddr',3): %d, strncmp(hostName,'ddr1',16): %d\n",
	externflag,msge->cntlrId,strncmp(msge->cntlrId,"ddr",3),strncmp(hostName,"ddr1",16));
   if ((externflag == 0) || 
       ((strncmp(msge->cntlrId,"ddr",3) != 0) && (strncmp(hostName,"ddr1",16) == 0)) )
   {
      if (msge->errorcode != 966) /* don't send post sync error. It was from previous experiment */
      {
      pubMsg.tag = -1;
      pubMsg.donecode  = (encodedTypenNum << 16) | (msge->cmd & 0xFFFF);
      pubMsg.errorcode  = msge->errorcode;
      pubMsg.crc32chksum  = 0;
      DPRINT3(-1,"AbortExp(): doneCode: 0x%x, %d, errorcode: %d\n", pubMsg.donecode, (pubMsg.donecode & 0xFFFF), pubMsg.errorcode);
      msgQSend(pDataTagMsgQ, (char *)(&pubMsg),sizeof(PUBLSH_MSG),
                             WAIT_FOREVER,MSG_PRI_URGENT);
      }
   } 


   /* ------------------------------------------------- */

   /* Controller output from FIFO to Software registers */
   /* setFifoOutputSelect(SELECT_SW_CONTROLLED_OUTPUT); no longer needed, as per Debbie */

   /* force values that must serialized out to whomever */
   /* setAbortSerializedStates(); no longer needed, as per Debbie */

    ddr_stop_exp( 0 );

   /* Disable DMA, and clear any Queued DMA transfers */
   abortFifoBufTransfer();

   /* clear Experiment downld codes,patterns,tables, etc.
    * and restart parser
    *
    */
    DPRINT(-1,"restart AParser\n");
    /* several things that could have the parser Suspended */
    if ( dataAllocWillBlock(pTheDataObject)  == 1 )
       dataForceAllocUnBlock(pTheDataObject);   /* free any data allocation block of parser */
    giveParseSem();				/* free any shandler syncing block of parser */
    parseCntDownReset();			/* free any parse ahead blocj of parser */

    AParserAA();

    /* resetExpBufsAndParser(); replaced with AParserAA above */

    DPRINT(-1,"Waiting for Parser to be ready\n");
    wait4ParserReady(WAIT_FOREVER);
    DPRINT(-1,"Parser Ready\n");

   /* Disable DMA, and clear any Queued DMA transfers */
   abortFifoBufTransfer();

   /* reset FIFO */
   cntrlFifoReset();

    freeAllDwnldBufsSynced();   /* free download buffer after dwnld completion msg recv'd */
/*
   stat = reloadDDR();
   DPRINT1(-1,"reloadDDR return: %d\n",stat);
*/

   /* taskDelay(calcSysClkTicks(17)); /* taskDelay(1); */

   DPRINT(-1,"---------> DDR Waiting on Recvproc to Report Done\n");
   /* result = execFunc("wait4RecvprocDone",300, NULL, NULL, NULL, NULL); /* 5 sec, 5*60=300 */
   result = wait4RecvprocDone(300);  /* 5 sec, 5*60=300 */
   DPRINT1(-1,"DDR Waiting result: %d\n",result);
   if (result == -1)
   {
       errLogRet(LOGIT,debugInfo,
	  "DDR Timed Out Waiting for Data to complete transfer with Recvproc Done Msg.\n");
   }

   /* ------------------------------------------------- */
   zeroFidCtState();
   send2Master(CNTLR_CMD_STATE_UPDATE,CNTLR_EXCPT_CMPLT,0,0,NULL);
   DPRINT(-1,"sent Master Exception Completed State\n");


   /* ddr_stop_exp( 0 ); */

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
            taskDelay(calcSysClkTicks(17));  /* taskDelay(1);  wait one tick , 16.6 msec */

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

inittst()
{
  initExceptionHandler();
  initUpLink();
} 

/*   some cheat test routines */
sndWrn()
{
   PUBLSH_MSG pubMsg;
   int stat;

   pubMsg.tag = -1;
   pubMsg.donecode  = WARNING_MSG;
   pubMsg.errorcode  = WARNINGS + RECVOVER;
   pubMsg.crc32chksum  = 0;
   stat = msgQSend(pDataTagMsgQ, (char *)(&pubMsg),sizeof(PUBLSH_MSG),
                          WAIT_FOREVER,MSG_PRI_URGENT);
   return stat;

}
sndAbort()
{
   PUBLSH_MSG pubMsg;
   int stat;

   pubMsg.tag = -1;
   pubMsg.donecode  = EXP_ABORTED;
   pubMsg.errorcode  = EXP_ABORTED;
   pubMsg.crc32chksum  = 0;
   stat = msgQSend(pDataTagMsgQ, (char *)(&pubMsg),sizeof(PUBLSH_MSG),
                          WAIT_FOREVER,MSG_PRI_URGENT);

   return stat;
}
