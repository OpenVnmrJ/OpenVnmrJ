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
DESCRIPTION

   This Task Handlers the Problems or Exceptions that happen to the
   system. For Example Fifo Errors (FORP,FOO,etc).

   The logic for handling these execption is encapsulated in this task
   thus there is one central place where the logic is placed.

*/

#define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#include <string.h>
#include <vxWorks.h>
#include <stdioLib.h>
#include <rngLib.h>
#include <semLib.h>
#include <msgQLib.h>
#include <wdLib.h>
#include "hostAcqStructs.h"
#include "hostMsgChannels.h"
#include "expDoneCodes.h"
#include "errorcodes.h"
#include "logMsgLib.h"
#include "namebufs.h"
#include "hardware.h"
#include "timeconst.h"
#include "taskPrior.h"
#include "fifoObj.h"
#include "stmObj.h"
#include "adcObj.h"
#include "autoObj.h"
#include "tuneObj.h"
#include "spinObj.h"
#include "sysflags.h"
#include "instrWvDefines.h"

extern SEM_ID  pSemSAStop; /* Binary  Semaphore used to Stop upLinker for SA */

extern MSG_Q_ID pUpLinkMsgQ;	/* MsgQ used between UpLinker and STM Object */
extern MSG_Q_ID pMsgesToPHandlr;/* MsgQ for Msges to Problem Handler */
extern MSG_Q_ID pMsgesToHost;	/* MsgQ used for Msges to routed upto Expproc */

/* Hardware Objects */
extern FIFO_ID		pTheFifoObject;
extern STMOBJ_ID	pTheStmObject;
extern ADC_ID		pTheAdcObject;
extern AUTO_ID		pTheAutoObject;
extern TUNE_ID		pTheTuneObject;
extern SPIN_ID		pTheSpinObject;

/* Exception Msges to Phandler, e.g. FOO, etc. */
extern EXCEPTION_MSGE HardErrorException;
extern EXCEPTION_MSGE GenericException;

extern RING_ID  pSyncActionArgs;  /* Buffer for 'Sync Action' (e.g. SETVT) function args */

/* Fixed & Dynamic Named Buffers */
extern DLB_ID  pDlbDynBufs;
extern DLB_ID  pDlbFixBufs;

extern int     SA_Criteria;/* SA, EXP_FID_CMPLT, BS_CMPLT, IL_CMPLT */
extern unsigned long SA_Mod; /* modulo for SA, ie which fid to stop at 'il'*/

typedef struct {
		int expCase;
                int Status; 
                int Event;
		} STAT_MSGE;

static void reset2SafeState();

static STAT_MSGE statMsg = { CASE, 0, 0 };

static int pHandlerTid;
static int pHandlerPriority;

WDOG_ID pHandlerWdTimeout;
int pHandlerTimeout;


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
static long pSafeStateCodes[] = { 
        CL_AP_BUS_SLCT | 0x0C68,                /* lpfg */
        CL_AP_BUS_WRT  | 0x0C00,                /* lpfg */
        CL_AP_BUS_SLCT | 0x0C69,                /* lpfg */
        CL_AP_BUS_WRT  | 0x0C00,                /* lpfg */

        CL_AP_BUS_SLCT | 0x0C58,                /* ppfg */
        CL_AP_BUS_WRT  | 0x0C00,                /* ppfg */
        CL_AP_BUS_SLCT | 0x0C59,                /* ppfg */
        CL_AP_BUS_WRT  | 0x0C00,                /* ppfg */
        CL_AP_BUS_WRT  | 0x0C00,                /* ppfg */
        CL_AP_BUS_WRT  | 0x0C00,                /* ppfg */
	
        CL_AP_BUS_SLCT | 0x0a20,                /* lk2kHz, tune off */
        CL_AP_BUS_WRT  | 0x0a80,
        CL_AP_BUS_INCWR| 0x0a01,
        CL_AP_BUS_SLCT | 0x0a20,                /* turn of hi/lo xmtr */
        CL_AP_BUS_WRT  | 0x0a50,
        CL_AP_BUS_INCWR| 0x0a00,

        CL_AP_BUS_SLCT | 0x0a76,                /* reset Hiband WFG */
        CL_AP_BUS_WRT  | 0x0a01,

        HALTOP, 0x00000000 };

static int numSafeStatCodes = sizeof(pSafeStateCodes)/sizeof(long);

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

/*************************************************************
*
*  probHandler - Wait for Messages that indecate some problem
*    with the system. Then perform appropriate recovery.
*
*					Author Greg Brissey 12-7-94
*/
pHandler(MSG_Q_ID msges)
{
   EXCEPTION_MSGE msge;
   int *val;
   int bytes;
   void recovery(EXCEPTION_MSGE *);

   DPRINT(1,"pHandler :Server LOOP Ready & Waiting.\n");
   FOREVER
   {
     markReady(PHANDLER_FLAGBIT);
     memset( &msge, 0, sizeof( EXCEPTION_MSGE ) );
     bytes = msgQReceive(pMsgesToPHandlr, (char*) &msge, 
			  sizeof( EXCEPTION_MSGE ), WAIT_FOREVER);
     markBusy(PHANDLER_FLAGBIT);
     DPRINT3(1,"pHandler: recv: %d bytes, Exception Type: %d, Event: %d \n",
			bytes, msge.exceptionType, msge.reportEvent);

      
     recovery( &msge );
   } 
}

enableInterrupts()
{

   stmItrpEnable(pTheFifoObject, (RTZ_ITRP_MASK | RPNZ_ITRP_MASK | 
			  MAX_SUM_ITRP_MASK | APBUS_ITRP_MASK));

   fifoItrpEnable(pTheFifoObject, FSTOPPED_I | FSTRTEMPTY_I | FSTRTHALT_I |
			   NETBL_I | FORP_I | TAGFNOTEMPTY_I | PFAMFULL_I |
			   SW1_I | SW2_I | SW3_I | SW4_I );

   autoItrpEnable(pTheAutoObject, AUTO_ALLITRPS); 
}

/*********************************************************************
*
* recovery, based on the cmd code decides what recovery action 
*		to perform.
*
*					Author Greg Brissey 12-7-94
*/
void recovery(EXCEPTION_MSGE *msge)
{
   char *token;
   int len;
   int cmd;
   int i,nMsg2Read;
   int bytes;
   EXCEPTION_MSGE discardMsge;
   extern int systemConRestart();
   static void resetSystem();
   static void resetNameBufs();

   /* reset STM & ADC Intrp MsgQ to standard  msgQ, encase they have been switched  */
   stmRestoreMsgQ(pTheStmObject);

   switch( msge->exceptionType )
   {
      case PANIC:
#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_PANIC,NULL,NULL);
#endif
    		errLogRet(LOGIT,debugInfo,
       		"phandler: Panic Error: %d", msge->reportEvent);
		break;

      case WARNING_MSG:
#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_WARNMSG,NULL,NULL);
#endif
  	        DPRINT2(0,"WARNING: doneCode: %d, errorCode: %d\n",
			msge->exceptionType,msge->reportEvent);
    		/* errLogRet(LOGIT,debugInfo,
       		"phandler: Warning Message: %d", msge->reportEvent); */
                statMsg.Status = msge->exceptionType;
                statMsg.Event = msge->reportEvent; /* Error Code */
		msgQSend(pMsgesToHost,(char*) &statMsg,sizeof(statMsg),
					WAIT_FOREVER, MSG_PRI_NORMAL);
		break;

      case SOFT_ERROR:
#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_SOFTERROR,NULL,NULL);
#endif
		update_acqstate( ACQ_IDLE );

  	        DPRINT2(0,"SOFT_ERROR: doneCode: %d, errorCode: %d\n",
			msge->exceptionType,msge->reportEvent);
    		/* errLogRet(LOGIT,debugInfo,
       		"phandler: Soft Error: %d", msge->reportEvent); */
                statMsg.Status = msge->exceptionType;
                statMsg.Event = msge->reportEvent; /* Error Code */
		msgQSend(pMsgesToHost,(char*) &statMsg,sizeof(statMsg),
					WAIT_FOREVER, MSG_PRI_NORMAL);
		break;

      case EXP_ABORTED:
#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_EXPABORTED,NULL,NULL);
#endif
		stmItrpDisable(pTheFifoObject, STM_ALLITRPS);
  	        DPRINT(0,"Exp. Aborted");
      case HARD_ERROR:

#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_HARDERROR,NULL,NULL);
#endif
  	        DPRINT1(0,"HARD_ERROR: doneCode: %d, errorCode: %d\n",
			 msge->reportEvent);


   	/* donecode = EXP_ABORTED or  HARD_ERROR plus errorCode(FOO,etc..) */
        /* need to get this msge up to expproc as soon as possible since the
	/* receipt of this msge cause expproc to send the SIGUSR2 (abort cmd)
	/* to Sendproc, without this msge all the acodes will be sent down.
         */
                statMsg.Status = msge->exceptionType; /* HARD_ERROR */
                statMsg.Event = msge->reportEvent; /* Error Code */
		msgQSend(pMsgesToHost,(char*) &statMsg,sizeof(statMsg),
					WAIT_FOREVER, MSG_PRI_URGENT);


  	        DPRINT2(0,"stmHaltCode: doneCode: %d, errorCode: %d\n",
			msge->exceptionType,msge->reportEvent);
		stmHaltCode(pTheStmObject,(int) msge->exceptionType, 
				(int) msge->reportEvent);


                if ( msge->exceptionType  == EXP_ABORTED)
                 storeConsoleDebug( SYSTEM_ABORT );
                /* reprogram HSlines, ap registers to safe state */
                reset2SafeState( msge->exceptionType );
                resetSystem();  /* reset fifo, disable intrps, reset tasks & buffers */
		if (msge->reportEvent == (SFTERROR + MAXCT))
		{
		   /* if maxsum error reset the stm */
		   stmReset(pTheStmObject);
		   taskDelay(sysClkRateGet()/3);
		}

                resetNameBufs();   /* free all named buffers */

  	        DPRINT(0,"wait4SystemReady");
                wait4SystemReady(); /* pend till all activities are complete */
  	        DPRINT(0,"SystemReady");
	        clrDwnLkAbort();

  	        DPRINT(0,"enableInterrupts");
		enableInterrupts();

	        /* now inform Expproc System is Ready */
                statMsg.Status = SYSTEM_READY; /* Console Ready  */
                statMsg.Event = 0; /* Error Code */
		msgQSend(pMsgesToHost,(char*) &statMsg,sizeof(statMsg),
					WAIT_FOREVER, MSG_PRI_NORMAL);
		update_acqstate( ACQ_IDLE );
	        getstatblock();

		if (pTheTuneObject != NULL)
		  semGive( pTheTuneObject->pSemAccessFIFO );

  	        DPRINT(0,"Done");
		break;
		
      case EXP_HALTED:
#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_EXPHALTED,NULL,NULL);
#endif
		stmItrpDisable(pTheFifoObject, STM_ALLITRPS);

                statMsg.Status = msge->exceptionType;
                statMsg.Event = msge->reportEvent; /* Error Code */
		msgQSend(pMsgesToHost,(char*) &statMsg,sizeof(statMsg),
					WAIT_FOREVER, MSG_PRI_NORMAL);

    		errLogRet(LOGIT,debugInfo,
       		"phandler: Exp. Halted: %d", msge->reportEvent);
		/* 
                   Based on the premise the user now has achieve the S/N
	           or whatever from data that has already been obtained
                   therefore there is no need to wait for any data to 
		   be acquired. I.E. Halt Experiment with extreme prejudice!
                */

                /* reprogram HSlines, ap registers to safe state */
                reset2SafeState( msge->exceptionType );

                resetSystem();  /* reset fifo, disable intrps, reset tasks & buffers */

		update_acqstate( ACQ_IDLE );

  	        DPRINT2(0,"stmHaltCode: doneCode: %d, errorCode: %d\n",
			msge->exceptionType,msge->reportEvent);
		stmHaltCode(pTheStmObject,(int) msge->exceptionType, 
				(int) msge->reportEvent);
   		DPRINT(0,"EXP_HALTED: lower priority below upLinker\n");
                taskPrioritySet(pHandlerTid,(UPLINKER_TASK_PRIORITY+1));  
   	        taskPrioritySet(pHandlerTid,pHandlerPriority);  

                resetNameBufs();   /* free all named buffers */

   		stmInitial(pTheStmObject, 1, 1024, pUpLinkMsgQ, 0);

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
		if (pTheTuneObject != NULL)
		  semGive( pTheTuneObject->pSemAccessFIFO );

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

 		clearIntrpMsgQ(pTheStmObject);
                semGive(pSemSAStop);   /* release the UpLinker */
		stmHaltCode(pTheStmObject,(int) msge->exceptionType, 
				(int) msge->reportEvent);
  	        /* DPRINT(0,"stmSA"); stmSA(pTheStmObject); */
   		DPRINT(0,"STOP_CMPLT: lower priority below upLinker\n");
                taskPrioritySet(pHandlerTid,(UPLINKER_TASK_PRIORITY+1));  
   	        taskPrioritySet(pHandlerTid,pHandlerPriority);  

		SA_Criteria = 0;
		SA_Mod = 0L;

                resetNameBufs();   /* free all named buffers */

   		stmInitial(pTheStmObject, 1, 1024, pUpLinkMsgQ, 0);

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
		if (pTheTuneObject != NULL)
		  semGive( pTheTuneObject->pSemAccessFIFO );

		break;

      case LOST_CONN:
#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_LOSTCONN,NULL,NULL);
#endif
    		errLogRet(LOGIT,debugInfo,
       		"phandler: Host Closed Connection to Console: %d", msge->reportEvent);
		update_acqstate( ACQ_IDLE );
		stmItrpDisable(pTheFifoObject, STM_ALLITRPS);
                storeConsoleDebug( SYSTEM_ABORT );
                /* reprogram HSlines, ap registers to safe state */
                reset2SafeState( msge->exceptionType );
                resetSystem();  /* reset fifo, disable intrps, reset tasks & buffers */
		stmReset(pTheStmObject);
		taskDelay(sysClkRateGet()/3);
                resetNameBufs();   /* free all named buffers */
		enableInterrupts();
		if (pTheTuneObject != NULL)
		  semGive( pTheTuneObject->pSemAccessFIFO );
     		taskSpawn("tRestart",50,0,2048,systemConRestart,NULL,
				2,3,4,5,6,7,8,9,10);
	        clrDwnLkAbort();

	        break;

      case ALLOC_ERROR:
#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_ALLOCERROR,NULL,NULL);
#endif
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
		stmHaltCode(pTheStmObject,(int) HARD_ERROR, 
				(int) msge->reportEvent);

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

   		stmInitial(pTheStmObject, 1, 1024, pUpLinkMsgQ, 0);

	        clrDwnLkAbort();

  	        DPRINT(0,"enableInterrupts");
		enableInterrupts();

	        /* now inform Expproc System is Ready */
                statMsg.Status = SYSTEM_READY; /* Console Ready  */
                statMsg.Event = 0; /* Error Code */
		msgQSend(pMsgesToHost,(char*) &statMsg,sizeof(statMsg),
					WAIT_FOREVER, MSG_PRI_NORMAL);

	        getstatblock();
		if (pTheTuneObject != NULL)
		  semGive( pTheTuneObject->pSemAccessFIFO );

		break;
      
      case WATCHDOG:
#ifdef INSTRUMENT
        wvEvent(EVENT_PHDLR_WATCHDOG,NULL,NULL);
#endif
		/* Giv'm a bone */
    		errLogRet(LOGIT,debugInfo,
       		"phandler: Watch Dog: %d", msge->reportEvent);
		break;

      default:
		/* Who Cares */
    		errLogRet(LOGIT,debugInfo,
       		"phandler: Invalid Exception Type: %d, Event: %d", 
			msge->exceptionType, msge->reportEvent);
		break;
   }
/*
   nMsg2Read = msgQNumMsgs(pMsgesToPHandlr);
   DPRINT1(-1,"Message in Q: %d\n",nMsg2Read);
*/
   while ( (bytes = msgQReceive(pMsgesToPHandlr, (char*) &msge, sizeof( EXCEPTION_MSGE ), NO_WAIT)) != ERROR )
   {
     DPRINT1(0,"Read %d bytes from Phandler msgQ, (Bogus Errors at this point)\n",bytes);
   }
}

static
void resetSystem()
{
/*
   DPRINT(0,"Disable STM Intrps");
   stmItrpDisable(pTheFifoObject, STM_ALLITRPS);
   DPRINT(0,"fifoReset");
   fifoReset(pTheFifoObject, RESETFIFOBRD);
*/
/*
   DPRINT(0,"setDwnLkAbort");
   setDwnLkAbort();
*/
   DPRINT(0,"AParserAA");
   AParserAA();
   DPRINT(0,"AupdtAA");
   AupdtAA();
   DPRINT(0,"ShandlerAA");
   ShandlerAA();
/*
   DPRINT(0,"fifoFlushBuf");
   fifoFlushBuf(pTheFifoObject);
   DPRINT(0,"fifoResetStuffing");
   fifoResetStuffing(pTheFifoObject);
*/
   DPRINT(0,"fifoStufferAA");
   fifoStufferAA(pTheFifoObject);
/*
   DPRINT(0,"stmReset");
   stmItrpDisable(pTheFifoObject, STM_ALLITRPS);
*/

   stmAdcOvldClear(pTheStmObject);
   /* stmReset(pTheStmObject); */
   DPRINT(0,"Disable ADC Intrps");
   adcItrpDisable(pTheAdcObject,ADC_ALLITRPS);
   /* adcReset(pTheAdcObject); */

   /* Reset Safe State again after tasks restarted */
   /*reset2SafeState();*/  /* reprogram HSlines, ap registers to safe state */
   /* call to reset2SafeState removed; in each case where resetSystem */
   /* is called, reset2SafeState is called immediately previously.    */

/* it maybe safe here, but we have to flip flop priorities with the Stuffer */
   DPRINT2(0,"resetSystem: tid: 0x%lx, priority: %d\n",pHandlerTid,pHandlerPriority);
   taskPrioritySet(pHandlerTid,(FIFO_STUFFER_PRIORITY+1));  
   setlksample();
   set2khz();
   taskPrioritySet(pHandlerTid,pHandlerPriority);  
   return;
}

/*
   WARNING: Not to be used by any other tasks than phandler
     1.  Stop Parser
     2.  Stop DownLinker
     3a. Reset adc
     3b. Stuff Fifo with safe states and run
     4.  Give-n-Take Serial Port devices Mutex to allow serial cmd to finish
     5.  Free Name buffers
     6.  Raise Priority of DownLink to above phandler and resume task if suspend & wait, lower priority
     7. Raise Priority of Parser to above phandler and resume task if suspend & wait, lower priority
     Last three steps allows for an orderly and efficient clean up of resources (name buffers,etc.)
*/

/* reprogram HSlines, ap registers to safe state */

static void
reset2SafeState( int why )  /* argument is the type of exception */
{
   int pTmpId, TmpPrior;

   DPRINT(1,"reset2SafeState");
   DPRINT(1,"stopAPint");
   abortAPint();   /* like stopAPint() but also cause the Parser to suspend itself */
   /* stopAPint(); */
   DPRINT(1,"stop downLinker");
   setDwnLkAbort();   /* like stop downLinker and set to dump further download to bit bucket */
   DPRINT(1,"reset2SafeState");
   DPRINT(0,"fifoSetNoStuff\n");
   fifoSetNoStuff(pTheFifoObject);    /* stop the stuffer in it's tracks */
   DPRINT(0,"Disable STM Intrps");
   stmItrpDisable(pTheFifoObject, STM_ALLITRPS);
   DPRINT(0,"Reset ADC overflow bit");
   adcReset(pTheAdcObject);
   DPRINT(0,"fifoReset");
   fifoReset(pTheFifoObject, RESETFIFOBRD);  /* this now also clear the Fifo Buffer & restart the Stuffer */

   DPRINT(0,"Stuff FIFO DIRECTLY with Safe State & Run fifo");
   fifoLoadHsl(pTheFifoObject, STD_HS_LINES, pTheFifoObject->SafeHSLines);
/*   fifoLoadHsl(pTheFifoObject, EXT_HS_LINES, pTheFifoObject->SafeHSLinesExt);
   /* Each SafeState Code is actually two long words */
   if (numSafeStatCodes > 0)
   {
      /*   WARNING: Does not pend for FF Almost Full, BEWARE !! */
      /*   Should only be used in phandler's reset2SafeState() */
      fifoStuffCode(pTheFifoObject, pSafeStateCodes, numSafeStatCodes);
      fifoStart(pTheFifoObject);
      fifoBusyWait4Stop(pTheFifoObject);
   }

   /* reset gradient shimming relay to observe */
   spinTalk(pTheSpinObject, 'G', 0);

   DPRINT(0,"getnGiveShimMutex\n");  
   /* allow any pending serial Shim commands to finish */
   getnGiveShimMutex();
   DPRINT(0,"getnGiveVTMutex\n");
   /* allow any pending serial VT commands to finish */
   getnGiveVTMutex();
   DPRINT(0,"getnGiveSpinMutex\n");
   /* allow any pending serial SpinVT commands to finish */
   getnGiveSpinMutex();

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
      while (downLinkerIsActive() && !taskIsSuspended(pTmpId) && !pHandlerTimeout)
            taskDelay(1);  /* wait one tick , 16msec */

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
     wait4ParserReady();
   }
   taskPrioritySet(pTmpId,TmpPrior);
   resetAPint();
   DPRINT(0,"A-code parser resynchronized\n");
   
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
#ifdef XXX
   callTaskId = taskIdSelf();
   DwnLkrTaskId = taskNameToId("tDownLink");
   taskPriorityGet(callTaskId,&callTaskPrior);
#endif


   retrys=0;

   /* start wdog timeout of 7 seconds */ 
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
	DPRINT1(-1,"resetNameBufs: retrys: %d\n",retrys);
#ifdef XXX
        /* Lower priority to allow downlinker in */
	if (callTaskPrior <= DOWNLINKER_TASK_PRIORITY)
           taskPrioritySet(callTaskId,(DOWNLINKER_TASK_PRIORITY+1));  
	Tdelay(10);

	DPRINT1(-1,"resetNameBufs: retrys: %d\n",retrys);
        if (retrys > 7)
        {
	   if (taskIsSuspended(DwnLkrTaskId))
	   {
	      DPRINT(-1,"resetNameBufs: clearCurrentNameBuffer()\n");
  	      clearCurrentNameBuffer();
           }
        }
#endif
   }
   wdCancel(pHandlerWdTimeout);
   /* taskPrioritySet(callTaskId,callTaskPrior);   */

  /*    resumeDownLink(); */
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
   DPRINT(0,"Stuff FIFO through Normal channels with Safe State & Run fifo");
   fifoLoadHsl(pTheFifoObject, STD_HS_LINES, pTheFifoObject->SafeHSLines);
/*    fifoLoadHsl(pTheFifoObject, EXT_HS_LINES, pTheFifoObject->SafeHSLinesExt);
/* NOMERCURY for now */
   /* reset gradient shimming relay to observe */
   spinTalk(pTheSpinObject, 'G', 0);

   /* Each SafeState Code is actually two long words */
   callTaskId = taskIdSelf();
   taskPriorityGet(callTaskId,&callTaskPrior);
   /* Lower priority to allow stuffer in, if needed  */
   /* if priority <= to that of the stuffer then lower the priority of this task */
   /* thus allowing the stuffer to stuff the fifo */
   DPRINT2(0,"set2ExpEndState: tid: 0x%lx, priority: %d\n",callTaskId,callTaskPrior);
   if (callTaskPrior <= FIFO_STUFFER_PRIORITY)
   {
      DPRINT(0,"Set priority lower than stuffer\n");
      taskPrioritySet(callTaskId,(FIFO_STUFFER_PRIORITY+1));  
   }
   if (numSafeStatCodes > 0)
   {
      fifoStuffIt(pTheFifoObject, pSafeStateCodes, numSafeStatCodes);/* into Fifo Buffer */
      fifoStart(pTheFifoObject);
      fifoWait4Stop(pTheFifoObject);
   }
   setlksample();
   set2khz();
   /* reset priority back if changed */
   if (callTaskPrior <= FIFO_STUFFER_PRIORITY)
   {
      DPRINT(0,"Set priority Back \n");
      taskPrioritySet(callTaskId,callTaskPrior);  
   }
   return;
}
