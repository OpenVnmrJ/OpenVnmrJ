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
8-12-04,gmb  created 
*/

/*
DESCRIPTION

   This Task Handler handles the FIFO synchronous signal via FIFO SW Interrupts
   system. For Example Setting VT, of report ready fo SystemSync 
*/

// #define TIMING_DIAG_ON    /* compile in timing diagnostics */

#ifndef ALLREADY_POSIX
 #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#endif
#include <string.h>
#include <vxWorks.h>
#include <stdioLib.h>
#include <semLib.h>
#include <rngLib.h>
#include <msgQLib.h>
#include <wdLib.h>
#include "instrWvDefines.h"
#include "taskPriority.h"
/* #include "hostAcqStructs.h" */
#include "expDoneCodes.h"
#include "errorcodes.h"
#include "logMsgLib.h"
#include "AParser.h"
#include "ACode32.h"
#include "Cntlr_Comm.h"
#include "cntlrStates.h"
#include "shandler.h"
#include "fifoFuncs.h"
#include "rf.h"
#include "nvhardware.h"
#include "upLink.h"
#include "sysUtils.h"
#include "Console_Stat.h"

#define ICATTEMPCOR
#if (defined(ICATTEMPCOR) && defined(SILKWORM))
#include "icat_reg.h"    // for print the icat temps. corrections,etc.  GMB
int icattempflag = 0;
#endif // ICATTEMPCOR

#define CASE (2)


extern MSG_Q_ID pFifoSwItrMsgQ;	  /* MsgQ for shandler */
extern RING_ID  pPending_SW_Itrs; /* SW interrupt Type/Usage for shandler */
extern RING_ID  pSyncActionArgs; /* shandler function arguments ring buffer */

extern MSG_Q_ID pMsgesToPHandlr;/* MsgQ for Msges to Problem Handler */


extern ACODE_ID  pTheAcodeObject; /* Acode object */

extern int  BrdType;    /* Type of Board, RF, Master, PFG, DDR, Gradient, Etc. */
extern int  BrdNum;     /* The Board types Ordinal number, i.e. rf1 or rf2 */

extern SEM_ID pSemOK2Tune;

extern SEM_ID pPrepSem;        /* Semaphore for Imaging Prep. */
extern int    prepflag;        /* Imaging prep flag used in shandler, SystemSyncUp() */

extern MSG_Q_ID pDataTagMsgQ; /* ddr to send SETUP_CMPLT to Recvproc */

extern int icatDelayFix;  /* a tick removed from systemsync delay on iCAT RF boards, and thus must be
                             added back into the total cunulative tick count for the RF  */

extern long long xgateCount;   /* count of xgate executions in the experiment */

/* extern int sampleHasChanged;	/* Global Sample change flag */
/* extern STATUS_BLOCK currentStatBlock; */

extern Console_Stat	*pCurrentStatBlock;

extern int tnlk_flag;	/* tn='lk' flag */
extern int tnlk_power;	/* power before tn='lk' */

extern int enableSpyFlag;  /* if > 0 then invoke the spy routines to monitor CPU usage  11/9/05 GMB */

extern void sendInfo2ConsoleStat(int tickmismatch);

WDOG_ID sHandlerWdTimeout;
int sHandlerTimeout;

int ShandlerTaskId = -1;

SEM_ID pRoboAckSem = NULL;
MSG_Q_ID pRoboAckMsgQ = NULL;



static long long zeroFidSyncTicks = 0;

void resetZeroFidSyncTicks()
{
    zeroFidSyncTicks = 0;
}

/* set shandler to a high priority 
 * called from the A32BrigdeFUncs.c for MRI read UserByte 
 */
MriUserByteShanderPriority()
{
    wvEvent(EVENT_SHANDLER_PRIOR_RAISED,NULL,NULL);
    taskPrioritySet(ShandlerTaskId,MRIUSERBYTE_SHANDLER_PRIORITY);
}

/* reset parser back to it's standard priority 
/* reset shandler back to it's standard priority 
 * called from the phandlers for errors, etc. 
 * in case shandler was left at higher priority from aborted MRI read User Byte
 */
resetShandlerPriority()
{
    wvEvent(EVENT_SHANDLER_PRIOR_LOWER,NULL,NULL);
    taskPrioritySet(ShandlerTaskId,SHANDLER_PRIORITY);
}


startShandler(int priority, int taskoptions, int stacksize)
{
   int sHandler();

   pRoboAckSem = semBCreate( SEM_Q_FIFO,SEM_EMPTY);
   pRoboAckMsgQ = msgQCreate(10,sizeof(int),MSG_Q_FIFO);

   if (taskNameToId("tSHandlr") == ERROR)
     taskSpawn("tSHandlr",priority,taskoptions,stacksize,sHandler,
		   pFifoSwItrMsgQ,2, 3,4,5,6,7,8,9,10);

   sHandlerWdTimeout = wdCreate();
   sHandlerTimeout = 0;
}

initShandler(int mask, int pos)
{
   initialSWItr(mask, pos);

   if (pFifoSwItrMsgQ == NULL)
       pFifoSwItrMsgQ = msgQCreate(64,sizeof(SHDLR_MSG),MSG_Q_PRIORITY);
   /* increase ring buffer from 64 to 64 * 3 to handle large arrays of temp. fixes Bug 9498  GMB   7/17/12  */
   if (pPending_SW_Itrs == NULL)
       pPending_SW_Itrs = rngCreate(64 * 3 * sizeof(int));
       /* pPending_SW_Itrs = rngCreate(64 * sizeof(SHDLR_MSG)); */
   if (pSyncActionArgs == NULL)
      pSyncActionArgs = rngCreate(1000 * sizeof(long));

   startShandler(SHANDLER_PRIORITY, STD_TASKOPTIONS /* | VX_FP_TASK */, XSTD_STACKSIZE + 4096 );
}

killShandler()
{
   int tid;
   if ((tid = taskNameToId("tSHandlr")) != ERROR)
	taskDelete(tid);
}

/* place the sample changer status return semaphore back to it's proper initial state */
resetRoboAckSem()
{
  while (semTake(pRoboAckSem,NO_WAIT) != ERROR);	
}

resetRoboAckMsgQ()
{
  int bytes, dummy;

  /* clean out msgQ */
   while ( (bytes = msgQReceive(pRoboAckMsgQ, (char*) &dummy, sizeof( int ), NO_WAIT)) != ERROR );
}

/*  typically use for errors, and aborts to free Shandler which might 
    be waiting on a sample changer responce */
void giveRoboAckSem()
{
   if ( (pRoboAckSem != NULL) )
      semGive(pRoboAckSem);
}

/*  typically use for errors, and aborts to free Shandler which might 
    be waiting on a sample changer responce */
void abortRoboAckMsgQ()
{
   int abort;
   abort = -99;
   if ( (pRoboAckMsgQ != NULL) )
     msgQSend(pRoboAckMsgQ,(char*) &abort,sizeof(abort), WAIT_FOREVER, MSG_PRI_NORMAL);
}


sHandlerWdISR()
{
   sHandlerTimeout = 1;
   wdCancel(sHandlerWdTimeout);
#ifdef INSTRUMENT
        wvEvent(EVENT_SHDLR_WDISR,NULL,NULL);
#endif
}

/*--------------------------------------------------------------*/
/* ShandlerAA							*/
/* 	Abort sequence for Shandler.				*/
/*--------------------------------------------------------------*/
ShandlerAA()
{
   int tid;
   int bytes,dumint;
#ifdef INSTRUMENT
	wvEvent(EVENT_SHDLR_AA,NULL,NULL);
#endif
   if ((tid = taskNameToId("tSHandlr")) != ERROR)
   {
   	rngFlush(pSyncActionArgs);
   	rngFlush(pPending_SW_Itrs);
        while ( (bytes = msgQReceive(pFifoSwItrMsgQ, (char*) &dumint, sizeof( int ), NO_WAIT)) != ERROR )
        {
          DPRINT1(-1,"ShandlerAA()  clear up pFifoSwItrMsgQ Acodes:  0x%lx\n",dumint);
        }

   	/* fifoClrNoStart(pTheFifoObject); INOVA */
#ifdef INSTRUMENT
	wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
#endif
	semGive(pTheAcodeObject->pSemParseSuspend);
	taskRestart(tid);
   }
}

ShandlerReset()
{
   int bytes,dumint;
    if (pSyncActionArgs != NULL)
   	rngFlush(pSyncActionArgs);
    if (pPending_SW_Itrs != NULL)
   	rngFlush(pPending_SW_Itrs);
    while ( (bytes = msgQReceive(pFifoSwItrMsgQ, (char*) &dumint, sizeof( int ), NO_WAIT)) != ERROR )
    {
       DPRINT1(-1,"ShandlerAA()  clear up pFifoSwItrMsgQ Acodes:  0x%lx\n",dumint);
    }
}

/*************************************************************
*
*  signalHandler - Wait for Messages that indecate a signal 
*    Then perform appropriate recovery.
*
*					Author Greg Brissey 8-12-04
*/
sHandler(MSG_Q_ID msges)
{
   SHDLR_MSG signalMsg;
   int *val;
   int bytes;
   void signaldecode(long signaltype);
   void syncAction(long signaltype);

   ShandlerTaskId = taskIdSelf();

   DPRINT(1,"sHandler :Server LOOP Ready & Waiting.\n");
   FOREVER
   {
     bytes = msgQReceive(msges, (char*) &signalMsg, 
			  sizeof( SHDLR_MSG ), WAIT_FOREVER);
     DPRINT3(+1,"sHandler: recv: %d bytes, SW Itr#: %d, Signal Type: 0x%lx\n",
			bytes, signalMsg.SWItrId, signalMsg.acode);

     switch(signalMsg.SWItrId)
     {
        case 1:  DPRINT(-1,"sHandler(): SW Itr 1, signal\n");
                 if (signalMsg.acode & 0x80000)
       	            syncAction(signalMsg.acode & (~0x80000));
     	         else
       	            signaldecode( signalMsg.acode );
		 break;

        case 2:  DPRINT(-1,"sHandler(): SW Itr 2, signal\n");
                 syncAction(signalMsg.acode);
	         break;
        case 3:  DPRINT(-1,"sHandler(): SW Itr 3, signal\n");
                 /* parseCntDownReset(); */  /* handle direct in ISR */
	         break;
        case 4:  DPRINT(-1,"sHandler(): SW Itr 4, signal\n");
                 SystemSyncUp(signalMsg.acode);
	         break;
        default:
		 break;
     }
   } 
}

/*
 * SystemSyncUp - 
 * For the COntroller other than the master, a message is sent to the 
 * master nexus that this controller is ready for the i sync pulse
 * from gate 7 of the master FIFO.
 * The MasterNexus updates the state of the controllers to ready4sync
 *
 * In the case of the master, the master pends waiting for all the 'known'
 * controller to report that they are ready for sync. This is accomplished
 * by wait for all of the controller in cntlrState object to set their
 * state to ready4sync.
 * When all controller are ready the Cntrl FIFO is started.
 *
 *                                     Author: Greg Brissey 8/12/2004
 */
SystemSyncUp(int acode)
{
    int errorcode,result;
    int prepFlag;
    char cntlrList[256];
    if (acode == SENDSYNC)
    {
        /* Master waits for all 'used' controller's stat to be ready4sync */
        /* 1st set master as ready */
        panelLedOn(WAIT4SYNC_LED);  /* from top down, 1st is dedicated to FPGA LogicAnalyzer trigger */
        rngBufGet(pSyncActionArgs,(char*) &prepFlag,sizeof(int));
        DPRINT1(-1,"SystemSyncUp: prepFlag = %d\n",prepFlag);
        cntlrSetState("master1",CNTLR_READY4SYNC,0);
        cntrlFifoCumulativeDurationClear();
        DPRINT(-1,"Waiting for Used Controllers to Report: Ready4Sync\n");

#ifdef TIMING_DIAG_ON    /* compile in timing diagnostics */
        TSPRINT("SystemSyncUp: Master Start:");
#endif
        /* Wait for all to complete.... */
        result = cntlrStatesCmpList(CNTLR_READY4SYNC, 30, cntlrList);  /* 1/2 minute to complete */

#ifdef TIMING_DIAG_ON    /* compile in timing diagnostics */
        TSPRINT("SystemSyncUp: Master All reported in:");
#endif
        if (result > 0)
        {
    	  errLogRet(LOGIT,debugInfo,
			"SystemSyncUp: Master Timed Out on Cntlrs 2B Ready for Sync (WAIT4ISYNC).\n");
    	  errLogRet(LOGIT,debugInfo, "SystemSyncUp:   Controllers: -> '%s' <- FAILED  to report Ready for Sync \n",
			cntlrList);
          sendException(HARD_ERROR, HDWAREERROR + CNTLR_ISYNC_ERR, 0,0,NULL);
        }
        else if (result < 0)
        {
    	  errLogRet(LOGIT,debugInfo,
			"SystemSyncUp: Exception Thrown by controller while Master waiting on Cntlrs 2B Ready for Sync (WAIT4ISYNC).\n");
    	  errLogRet(LOGIT,debugInfo, "SystemSyncUp:   Controller: -> '%s' <- Reported  Exception \n",
			cntlrList);
        }
        else
        {
	   DPRINT(-1,"All Cntlrs Reported Ready, Start FIFO \n");
           /* 
            * For Imaging Prep suspend, sethw command 'SYNC_FREE' give the semaphore about to be taken
            * When given the sequence can start immediately 
            */
           if (prepFlag == 1)
           {
              setAcqState(ACQ_SYNCED);   /* status update to Vnmrj */
              DPRINT(-1,"SystemSyncUp: take Prep Semaphore\n");
              semTake(pPrepSem, WAIT_FOREVER );  /* wait for sethw command from Vnmr to start sequence */
              DPRINT(-1,"SystemSyncUp: Prep Semaphore Obtained.\n");
              setAcqState(ACQ_ACQUIRE);   /* status update to Vnmrj */
           }
           panelLedOff(WAIT4SYNC_LED);  /* from top down, 1st is dedicated to FPGA LogicAnalyzer trigger */
	   startCntrlFifo();   /* already started for exp so don't call startFifo4Exp() */
        }
    }
    else if (acode == WAIT4ISYNC)   /* non master controllers come here */
    {
        /* non-master cntlrs report they are ready for SystemSync */
        cntrlFifoCumulativeDurationClear();
        DPRINT(-1,"Tell Master I'm Ready4sync\n");

#ifdef TIMING_DIAG_ON    /* compile in timing diagnostics */
        TSPRINT("SystemSyncUp: Cntlr send2master Rdy2Sync:");
#endif

        send2Master(CNTLR_CMD_READY4SYNC,CNTLR_READY4SYNC,0,0,NULL);
    }
    else
    {
        errLogRet(LOGIT,debugInfo,
			"SystemSyncUp(): Unknown Acode 0x%lx (%d) passed\n",acode,acode);
        sendException(HARD_ERROR, HDWAREERROR + CNTLR_ISYNC_ERR, 0,0,NULL);
    }
}

addPendingSWItr(int acode)
{
    /* int freebytes;*/
    DPRINT1(-1,"addPendingSWItr: add Acode 0x%lx to pending list\n",acode);
    taskLock();
    rngBufPut(pPending_SW_Itrs,(char*) &acode,sizeof(int));
    taskUnlock();
    /*
    FOund large 'temp' array i.e. setVT WAIT4VT overflows the ring buffer being used. 
    This diagnostic confirmed that this was what was happening.
    Increase ring buffer to be 64 * 3
        GMB   7/17/12   fixes BUG 9498
    freebytes = rngFreeBytes(pPending_SW_Itrs);
    DPRINT1(-1,"addPendingSWItr: Byes left in ing buffer: %d\n",freebytes);
    if  (freebytes <= 0) sendException(HARD_ERROR, SYSTEMERROR + FIFO_WRDS_LOST, 0,0,NULL);
    */
}



/*----------------------------------------------------------------------*/
/* signal2_syncop                                                       */
/*      Stuffs an action operation tag into the fifo.                   */
/*      This tag is followed by:                                        */
/*      1. haltop in the fifo, thus stopping the Exp. 			*/
/*         When the requested action is complete the fifo will be	*/
/*         restarted.						        */
/*----------------------------------------------------------------------*/
int signal2_syncop(int tagword)
{
int instrwords[20];
int len,total;
   DPRINT1(2,"signal2_syncop tag=%d\n",tagword);

   /* issue SW Itr #2 */
   len = fifoEncodeSWItr(2, instrwords);
   len += fifoEncodeDuration(1, 0, &instrwords[len] ); /* haltop */
   writeCntrlFifoBuf(instrwords,len); 

   addPendingSWItr(tagword);
   return len;
}
/*----------------------------------------------------------------------*/
/* signal_syncop                                                        */
/*      Stuffs an action operation tag into the fifo.                   */
/*      This tag is followed by:                                        */
/*      1. haltop in the fifo, thus stopping the Exp. When the requested*/
/*         action is complete the fifo will be  restarted.              */
/*      2. delay in seconds, fifo not stopped                           */
/*      3. Nothing, operation is considered done immediately            */
/*----------------------------------------------------------------------*/
int signal_syncop(int tagword,long secs,long ticks)
{
  int instrwords[20];
  int len,total;

   DPRINT1(2,"signal_syncop: ticks: %ld\n",ticks);
 /* issue SW Itr #1 */
  len = fifoEncodeSWItr(1, instrwords);

  len += fifoEncodeDuration(1, 0, &instrwords[len] ); /* haltop */

#ifdef NOT_YET_IMPLEMENTED
  if ((ticks == -1L) || (secs == -1L))
      len += fifoEncodeDuration(1, 0, instrwords); /* haltop */
  else
      delay(secs, ticks);  /* put completion delay into fifo */
#endif

  addPendingSWItr(tagword);

  writeCntrlFifoBuf(instrwords,len); 

  return len;
}


/*----------------------------------------------------------------------*/
/* signalz_syncop                                                       */
/*      This syncing operation is intended for the DDR use only         */
/*       Using the SW itr 2,                                            */
/*      Stuffs an action operation tag into the fifo.                   */
/*      At present this sync is single purposed to sync the SEND_ZERO_FID */
/*      of the last FID, otherwise the experiment is permaturally       */
/*      considered done by Recvproc */
/*      This tag is followed by:                                        */
/*      1. No haltop in the fifo,                        			*/
/*         Since a 'zero' FID is being transfer there is a delay added  */
/*         before to provide enough time for the last acquired FID      */
/*         to be sent to Recvproc  and a delay after for enogh time for */
/*         the zero FID to be transfered.                               */
/*         
/*                                  Greg B.   3/8/2012                  */
/*----------------------------------------------------------------------*/
int signalz_syncop(int acode, int tagword)
{
   int instrwords[20];
   int len,total;

	if (BrdType == DDR_BRD_TYPE_ID)  // only the DDR may use this interrupt this way
   {
       DPRINT1(-7,"signalz_syncop tag=%d\n",tagword);

       /* issue SW Itr #4 */
       len = 0;
       // Note: 0x3fffff max delay for single TAC word  ~ 868ms
       len += fifoEncodeDuration(1, 40000000, &instrwords[len] ); /* 500ms delay */
       len += fifoEncodeDuration(1, 40000000, &instrwords[len] ); /* 500ms delay */
       // len += fifoEncodeSWItr(2, instrwords);
       len += fifoEncodeSWItr(2, &instrwords[len] );
       len += fifoEncodeDuration(1, 8000, &instrwords[len] ); /* 100us delay */
       len += fifoEncodeDuration(1, 40000000, &instrwords[len] ); /* 500ms delay */
       len += fifoEncodeDuration(1, 40000000, &instrwords[len] ); /* 500ms delay */
       DPRINT1(-9,"signalz_syncop TAC words: %d\n",len);
       writeCntrlFifoBuf(instrwords,len); 

       addPendingSWItr(acode);
       rngBufPut(pSyncActionArgs,(char*) &tagword,sizeof(int));
       zeroFidSyncTicks += (long long) ((40000000 * 4) + 8000 + 640);
   }
   else
   {
      len = 0;
      DPRINT(-9,"ERROR: signalz_syncop IS only Available on a DDRs!\n");
   } 
   return len;
}
/*----------------------------------------------------------------------*/

//static void Fifo_SW_ISR(int int_status, int val) 
void Fifo_SW_ISR(int int_status, int val) 
{
  SHDLR_MSG sMsg;
  char *buffer;
  int acode;

   /* EVENT_FIFO_SW1_ITR = 21, 2 = 22, 3 = 23, 4 = 24 */
  #ifdef INSTRUMENT
     wvEvent(20+val,NULL,NULL);
  #endif
   if (val == 3)
   {
     if (DebugLevel > 1)
       logMsg(" Fifo_SW_ISR: SW Itr#: %d\n",val,2,3,4,5,6);
      parseCntDownReset();
   }
   else
   {
     rngBufGet(pPending_SW_Itrs,(char*) &acode,sizeof(int));
     sMsg.SWItrId = val;
     sMsg.acode = acode;

     /* for MRIUSERBYTE it's needed to raise the priority of the shandler 
      * above all network and parser priority 
     */
     if (sMsg.acode == MRIUSERBYTE)
     {
         MriUserByteShanderPriority();
     }

     msgQSend(pFifoSwItrMsgQ, (char*) &sMsg, sizeof(SHDLR_MSG),
                        NO_WAIT, MSG_PRI_NORMAL);
     if (DebugLevel > 1)
         logMsg(" Fifo_SW_ISR: SW Itr#: %d, Acode: 0x%lx\n",val,acode,3,4,5,6);
   }
  #ifdef INSTRUMENT
     wvEvent(20+val,NULL,NULL);
  #endif

   return;
}

initialSWItr(int mask, int pos)
{
  //int mask = get_mask(RF,sw_int_status);  // :TODO: this looks like a bug in all controllers but RF !?
  DPRINT1(2,"Fifo_SW_ISR connect mask %x\n", mask);
  fpgaIntConnect( Fifo_SW_ISR, 1, 1<<pos);//FF_SW1_IRQ);
  fpgaIntConnect( Fifo_SW_ISR, 2, 2<<pos);//FF_SW2_IRQ);
  fpgaIntConnect( Fifo_SW_ISR, 3, 4<<pos);//FF_SW3_IRQ);
  fpgaIntConnect( Fifo_SW_ISR, 4, 8<<pos);//FF_SW4_IRQ);
  cntrlFifoIntrpSetMask(mask);
}


deInstallSWItrs()
{
  DPRINT(2,"Fifo_SW_ISR disconnect %x\n");
  fpgaIntRemove( Fifo_SW_ISR, 1);
  fpgaIntRemove( Fifo_SW_ISR, 2);
  fpgaIntRemove( Fifo_SW_ISR, 3);
  fpgaIntRemove( Fifo_SW_ISR, 4);
}

/*********************************************************************
*
* signaldecode, based on the signal Type  decides what action 
*		to perform.
*
*					Author Greg Brissey 12-7-94
*/
void signaldecode(long signaltype)
{
   char *token;
   int len;
   int cmd;
   int i;
   int result;
   int tickmismatch=0;
   unsigned long sentcnt,ifcount;  /* instruction fifo count */
   long instrWrdsLost;
   char cntlrList[256];

   switch( signaltype )
   {
      case SETUP_CMPLT:
#ifdef INSTRUMENT
	wvEvent(EVENT_SHDLR_SETUPCMPLT,NULL,NULL);
#endif
	   DPRINT(0,"SETUP_CMPLT\n");
           ifcount = cntrlInstrCountTotal();
           sentcnt = cntrlGetCumDmaCnt();
           instrWrdsLost = ifcount - sentcnt;
           DPRINT3(-4,">> Fifo Instructions Sent: %lu, --> Recv'd: %lu, diff: %ld\n",
			sentcnt,ifcount, instrWrdsLost);

           if (enableSpyFlag > 0)
           {
              spyReport();   /* report CPU usage */
              spyClkStop();  /* turn off spy clock interrupts, etc. */
           }

	   if (BrdType == MASTER_BRD_TYPE_ID)
           {
               long long ticks;
              /* stop lock/spinvt error/warning msgs */
               setLkInterLk(0);
               setVTinterLk(0);
               setSpinInterlk(0);

               /* wait for 4 active controllers to indicate they are done */
               cntrlFifoCumulativeDurationGet(&ticks);
               cntlrSetFifoTicks("master1",ticks);  /* just incase it got into rollcal parameter */
               cntlrSetState("master1",CNTLR_SU_CMPLT,0);  /* just incase it got into rollcal parameter */
               /* cntlrStateShow(); */
               DPRINT(-1,"Master Waiting on Controllers to Report: CNTLR_SU_CMPLT");

#ifdef TIMING_DIAG_ON    /* compile in timing diagnostics */
               TSPRINT("Master Waiting on Controllers to Report: CNTLR_SU_CMPLT:");
#endif

               result = cntlrStatesCmpList(CNTLR_SU_CMPLT, 30, cntlrList);  /* 1/2 minute to complete */

#ifdef TIMING_DIAG_ON    /* compile in timing diagnostics */
               TSPRINT("Master Recvd all Controllers SU CMPLTED:");
#endif

               if (result > 0)
               { 
                  /* cntlrStateShow(); */
    	          errLogRet(LOGIT,debugInfo,
			"Master Timed Out Waiting on controllers to Ack SU Complete.\n");
    	          errLogRet(LOGIT,debugInfo, "Controllers: -> '%s' <- FAILED  to report SU Complete\n",
			cntlrList);
                  sendException(HARD_ERROR, HDWAREERROR + CNTLR_CMPLTSYNC_ERR, 0,0,NULL);
	       }
               else if (result < 0)
               {
    	         errLogRet(LOGIT,debugInfo,
			"Exception Thrown by controller while Master waiting on Cntlrs to Ack SU Complete.\n");
    	         errLogRet(LOGIT,debugInfo, "Controller: -> '%s' <- Reported  Exception \n",
			cntlrList);
               }
               else
	       {
                  DPRINT(-1,"Cnltrs Are Ready");
	       }
               resetTnEqualsLkState();
               setFidCtState(0, 0);
               setAcqState(ACQ_IDLE);
               semGive(pSemOK2Tune);     /* now allow tuning */
               execFunc("send2Expproc", (int*) CASE, (int*) SYSTEM_READY,0,0,NULL,0,NULL,NULL); /* sendSysReady(); */
               tickmismatch = cntlrStateReportFifoTicks();
               sendInfo2ConsoleStat(tickmismatch);
#ifdef CALC_DURATION_TESTS
               execFunc("prtFifoTime","Master Setup:", NULL, NULL, NULL, NULL, NULL,NULL,NULL);
#endif
           }
	   else if (BrdType == DDR_BRD_TYPE_ID)
           {
              long long ticks;
              int result;
              int wait4RecvprocDone(int trys);
              PUBLSH_MSG pubMsg;
              result = execFunc("ddrActive",NULL, NULL, NULL, NULL, NULL, NULL,NULL,NULL);
              DPRINT1(-1,"DDR active: %d\n",result);
              if (result == 1)   /* send msge to Recvproc only if an Active DDR */
              {
                 /* send Setup complete to Recvproc, via upLink */
                 pubMsg.tag = -1;
                 pubMsg.donecode  = SETUP_CMPLT;
                 pubMsg.errorcode  = 0;
                 pubMsg.crc32chksum  = 0;
                 msgQSend(pDataTagMsgQ, (char *)(&pubMsg),sizeof(PUBLSH_MSG),
                              WAIT_FOREVER,MSG_PRI_NORMAL);
              }

              DPRINT(-1,"---------> DDR Waiting on Recvproc to Report Done\n");

#ifdef TIMING_DIAG_ON    /* compile in timing diagnostics */
               TSPRINT("DDR Waiting on Recvproc Done:");
#endif
              result = execFunc("wait4RecvprocDone",(int*) 900, NULL, NULL, NULL, NULL, NULL,NULL,NULL); /* 15 sec, 15*60=900 */

#ifdef TIMING_DIAG_ON    /* compile in timing diagnostics */
               TSPRINT("DDR Recvd Recvproc Done:");
#endif

              DPRINT1(+1,"DDR Waiting result: %d\n",result);
              if (result == -1)
              {
    	          errLogRet(LOGIT,debugInfo,
			"DDR Timed Out Waiting for Recvproc to complete setup processing.\n");
                  sendException(HARD_ERROR, HDWAREERROR + DDR_CMPLTFAIL_ERR, 0,0,NULL);
              }
              else
              {
	         /* tell master we are done */
                 cntrlFifoCumulativeDurationGet(&ticks);

#ifdef TIMING_DIAG_ON    /* compile in timing diagnostics */
               TSPRINT("DDR Send Master CMPLT:");
#endif
                 send2MasterTicks(ticks);
                 send2Master(CNTLR_CMD_STATE_UPDATE,CNTLR_SU_CMPLT,0,0,NULL);
                 DPRINT(-1,"Sent Master Setup Completed State\n");
#ifdef CALC_DURATION_TESTS
               execFunc("prtFifoTime","DDR Setup:", NULL, NULL, NULL, NULL, NULL,NULL,NULL);
#endif
               }
               zeroFidCtState();
           }
           else
           {
               long long ticks;
               int rftype;
               cntrlFifoCumulativeDurationGet(&ticks);

               rftype = execFunc("getRfType",0,0,0,0,NULL,0,NULL,NULL); /* get rftype 0=VNMRS, 1=iCAT */
               if (rftype == 1)  // if iCAT RF
               {
#if (defined(ICATTEMPCOR) && defined(SILKWORM))
                  unsigned int measuredtemp,curtemp,avgtemp, phasetemp, sinegaintemp;
#endif // ICATTEMPCOR
                  // DPRINT3(-9, "SETUP_CMPLT: cumultiveTck: %lld, iCAT correction: %d, fix cum: %lld\n",
                  //       ticks, icatDelayFix, (ticks + ((long long)  icatDelayFix)) );

                  /* the ticks removed from systemsync delay on iCAT RF boards, and thus must be added back in */
                  // add back the icatDelayFix ticks to correct the cumulative ticks reported
                  ticks = ticks + (long long) ( icatDelayFix );

#if (defined(ICATTEMPCOR) && defined(SILKWORM))
                  if (icattempflag != 0) {
                     measuredtemp = execFunc("icat_spi_read",(int*) iCAT_Temperature,0,0,0,0,0,0,0);
                     curtemp = execFunc("icat_spi_read",(int*) iCAT_CurrentTemperature,0,0,0,0,0,0,0);
                     avgtemp = execFunc("icat_spi_read",(int*) iCAT_AverageTemperature,0,0,0,0,0,0,0);
                     phasetemp = execFunc("icat_spi_read",(int*) iCAT_PhaseTemperatureCoefficient,0,0,0,0,0,0,0);
                     sinegaintemp = execFunc("icat_spi_read",(int*) iCAT_SineGainTemperatureCoefficient,0,0,0,0,0,0,0);
                   
                     DPRINT6(-6,"++--++ Exp Cmplt:  iCAT Temp  Current: %f , Avg: %f, Corr: %f, Phase: %f, (0x%x), SineGain: %f\n",(curtemp/256.0)-128.0,(avgtemp/256.0)-128.0,(measuredtemp/256.0)-128.0, (float) phasetemp * (360.0/65536.0),phasetemp, ((double) sinegaintemp/32768.0));
                  }
#endif // ICATTEMPCOR

               }


#ifdef TIMING_DIAG_ON    /* compile in timing diagnostics */
               TSPRINT("Cntlr Send Master SU CMPLT:");
#endif

               send2MasterTicks(ticks);
	       /* tell master we are done */
               send2Master(CNTLR_CMD_STATE_UPDATE,CNTLR_SU_CMPLT,0,0,NULL);
               DPRINT(-1,"Sent Master Setup Completed State\n");

#ifdef CALC_DURATION_TESTS
               execFunc("prtFifoTime","Cntlr Setup:", NULL, NULL, NULL, NULL, NULL,NULL,NULL);
#endif

           }
           /* prevent false starts from Sync glitches, controller reboots, or FPGA reloads */
           cntrlFifoClearStartMode();

           resumeLedShow(); /* resume LED Idle display */

#ifdef INOVA
	   currentStatBlock.stb.AcqOpsComplFlags |= SETUP_CMPLT_FLAG;
	   setVTinterLk(0);
	   setLKInterlk(0);
	   setSpinInterlk(0);
           update_acqstate(ACQ_IDLE);
	   getstatblock();
   	   rngFlush(pSyncActionArgs);

           /* PARALLEL_CHANS */
           freeSorter();
	   clearParallelFreeBufs();

           set2ExpEndState();  /* reprogram HSlines, ap registers to safe state */
           stmHaltCode(pTheStmObject,(int)signaltype, 0); /* Donecode, Errorcode */

           fifoClrStart4Exp(pTheFifoObject);
	   if (pTheTuneObject != NULL)
	     semGive( pTheTuneObject->pSemAccessFIFO );
#endif

#ifdef TIMING_DIAG_ON    /* compile in timing diagnostics */
      resetTimeStamp();
#endif
		break;

      case EXP_COMPLETE:
#ifdef INSTRUMENT
	wvEvent(EVENT_SHDLR_EXPCMPLT,NULL,NULL);
#endif
	   DPRINT(0,"EXP_COMPLETE\n");
           ifcount = cntrlInstrCountTotal();
           sentcnt = cntrlGetCumDmaCnt();
           instrWrdsLost = ifcount - sentcnt;
           DPRINT3(-1,">> Fifo Instructions Sent: %lu, --> Recv'd: %lu, diff: %ld\n",
			      sentcnt,ifcount, instrWrdsLost);

           if (enableSpyFlag > 0)
           {
              spyReport();   /* report CPU usage */
              spyClkStop();  /* turn off spy clock interrupts, etc. */
           }

	        if (BrdType == MASTER_BRD_TYPE_ID)
           {
               long long ticks;
               /* stop lock/spinvt error/warning msgs */
               setLkInterLk(0);
               setVTinterLk(0);
               setSpinInterlk(0);

               cntrlFifoCumulativeDurationGet(&ticks);
               cntlrSetFifoTicks("master1",ticks);  /* just incase it got into rollcal parameter */
               cntlrSetState("master1",CNTLR_EXP_CMPLT,0);  /* just incase in got into rollcal parameter */
               /* cntlrStateShow(); */

               if ( instrWrdsLost != 0)
               {
    	          errLogRet(LOGIT,debugInfo,
			"Master: Lost Intruction Fifo words, sent: %lu, recv'd: %lu, LOST: %ld\n",
				sentcnt,ifcount,instrWrdsLost);
                  sendException(HARD_ERROR, SYSTEMERROR + FIFO_WRDS_LOST, 0,0,NULL);
               }


               DPRINT(-1,"Master Waiting on Controllers to Report: CNTLR_EXP_CMPLT");
               /* wait for 4 active controllers to indicate they are done */

               /* Wait for all to complete.... */
#ifdef TIMING_DIAG_ON    /* compile in timing diagnostics */
               TSPRINT("Master Waiting on Controllers to Report: CNTLR_EXP_CMPLT:");
#endif

               result = cntlrStatesCmpList(CNTLR_EXP_CMPLT, 30, cntlrList);  /* 15 seconds to complete */

#ifdef TIMING_DIAG_ON    /* compile in timing diagnostics */
               TSPRINT("Master Recvd all Controllers Reporting EXP CMPLTED:");
#endif

               if (result > 0)
               { 
                  cntlrStateShow();
    	            errLogRet(LOGIT,debugInfo,
			            "Master Timed Out Waiting on controllers to Ack Exp Complete.\n");
    	            errLogRet(LOGIT,debugInfo, "Controllers: -> '%s' <- FAILED  to report Exp Complete\n",
			            cntlrList);
                  sendException(HARD_ERROR, HDWAREERROR + CNTLR_CMPLTSYNC_ERR, 0,0,NULL);
	            }
               else if (result < 0)
               {
    	            errLogRet(LOGIT,debugInfo,
			            "Exception Thrown by controller while Master waiting on Cntlrs to Ack Exp Complete.\n");
    	            errLogRet(LOGIT,debugInfo, "Controller: -> '%s' <- Reported  Exception \n",
			               cntlrList);
               }
               else
	            {
                  DPRINT(-1,"Cnltrs Are Ready");
                  /* 10/13/2009  GMB, the next 5 statements were move into this block
                   * Prior to this, a post-sync error after recvproc had completed data upload, (cause unknown yet)
                   * would result in a system ready message sent to Expproc, which would allow a new exp to start, 
                   * in the mean time the phandler was processing the post-sync error, which could 
                   * result in deleting the new acodes being download, cause a No Acodes found error.
                   * phandler already was performing these statements below, thus the following statements should only 
                   * be done here if no error had occurred.
                  */
                  resetTnEqualsLkState();
                  setFidCtState(0, 0);
                  setAcqState(ACQ_IDLE);
                  semGive(pSemOK2Tune);     /* now allow tuning */
                  execFunc("send2Expproc",(int*) CASE, (int*) SYSTEM_READY,0,0,NULL,0,NULL, NULL); /* sendSysReady(); */
	            }
               tickmismatch = cntlrStateReportFifoTicks();
               sendInfo2ConsoleStat(tickmismatch);
#ifdef CALC_DURATION_TESTS
               execFunc("prtFifoTime","Master ExpCmplt:", NULL, NULL, NULL, NULL, NULL,NULL,NULL);
#endif
           }
	   else if (BrdType == DDR_BRD_TYPE_ID)
           {
               long long ticks;
              int result;
              int wait4RecvprocDone(int trys);

               if ( instrWrdsLost != 0)
               {
    	          errLogRet(LOGIT,debugInfo,
			"Master: Lost Intruction Fifo words, sent: %lu, recv'd: %lu, LOST: %ld\n",
				sentcnt,ifcount,instrWrdsLost);
                  sendException(HARD_ERROR, SYSTEMERROR + FIFO_WRDS_LOST, 0,0,NULL);
              }

              DPRINT(-1,"---------> DDR Waiting on Recvproc to Report Done\n");

#ifdef TIMING_DIAG_ON    /* compile in timing diagnostics */
               TSPRINT("DDR Waiting on Recvproc Done:");
#endif

              result = execFunc("wait4RecvprocDone",(int*)900, NULL, NULL, NULL, NULL, NULL,NULL,NULL); /* 15 sec, 15*60=900 */

#ifdef TIMING_DIAG_ON    /* compile in timing diagnostics */
               TSPRINT("DDR Recvd Recvproc Done:");
#endif

              DPRINT1(+1,"DDR Waiting result: %d\n",result);
              if (result == -1)
              {
    	          errLogRet(LOGIT,debugInfo,
			"DDR Timed Out Waiting for Data to complete transfer with Recvproc Done Msg.\n");
                  sendException(HARD_ERROR, HDWAREERROR + DDR_CMPLTFAIL_ERR, 0,0,NULL);
              }
              else
              {
	       /* tell master we are done */
               cntrlFifoCumulativeDurationGet(&ticks);

#ifdef TIMING_DIAG_ON    /* compile in timing diagnostics */
               TSPRINT("DDR Send Master EXP CMPLT:");
#endif

               send2MasterTicks((ticks - zeroFidSyncTicks));
               send2Master(CNTLR_CMD_STATE_UPDATE,CNTLR_EXP_CMPLT,0,0,NULL);
#ifdef CALC_DURATION_TESTS
               execFunc("prtFifoTime","Cntlr ExpCmplt:", NULL, NULL, NULL, NULL, NULL,NULL,NULL);
#endif
               DPRINT(-1,"Sent Master Exp. Completed State\n");
               }
               zeroFidCtState();
           }
           else
           {
               long long ticks;
               int rftype;
	       /* tell master we are done */
               cntrlFifoCumulativeDurationGet(&ticks);

               rftype = execFunc("getRfType",0,0,0,0,NULL,0,NULL,NULL); /* get rftype 0=VNMRS, 1=iCAT */
	            if (rftype == 1)  // if iCAT RF
               {
#if (defined(ICATTEMPCOR) && defined(SILKWORM))
                    unsigned int measuredtemp,curtemp,avgtemp, phasetemp, sinegaintemp;
#endif // ICATTEMPCOR
                   // DPRINT3(-9, "EXP_CMPLT: cumultiveTck: %lld, iCAT correction: %d, fix cum: %lld\n",
                   //      ticks, icatDelayFix, (ticks + ((long long)  icatDelayFix)) );

                   // add back the icatDelayFix ticks to correct the cumulative ticks reported
                   ticks = ticks + (long long) ( icatDelayFix );

                   // ticks must also be corrected for the 64 tick skewing with each xgate execution
                   ticks += (xgateCount) * (long long)(icatDelayFix);

#if (defined(ICATTEMPCOR) && defined(SILKWORM))
                  if (icattempflag != 0) {
                     measuredtemp = execFunc("icat_spi_read",(int*) iCAT_Temperature,0,0,0,0,0,0,0);
                     curtemp = execFunc("icat_spi_read",(int*)  iCAT_CurrentTemperature,0,0,0,0,0,0,0);
                     avgtemp = execFunc("icat_spi_read",(int*) iCAT_AverageTemperature,0,0,0,0,0,0,0);
                     phasetemp = execFunc("icat_spi_read",(int*) iCAT_PhaseTemperatureCoefficient,0,0,0,0,0,0,0);
                     sinegaintemp = execFunc("icat_spi_read",(int*) iCAT_SineGainTemperatureCoefficient,0,0,0,0,0,0,0);
                     DPRINT6(-6,"++--++ Exp Cmplt:  iCAT Temp  Current: %f , Avg: %f, Corr: %f, Phase: %f, (0x%x), SineGain: %f\n",(curtemp/256.0)-128.0,(avgtemp/256.0)-128.0,(measuredtemp/256.0)-128.0, (float) phasetemp * (360.0/65536.0),phasetemp, ((double) sinegaintemp/32768.0));
                 }
#endif // ICATTEMPCOR

               }

#ifdef TIMING_DIAG_ON    /* compile in timing diagnostics */
               TSPRINT("Cntlr Send Master EXP CMPLT:");
#endif

               send2MasterTicks(ticks);
               send2Master(CNTLR_CMD_STATE_UPDATE,CNTLR_EXP_CMPLT,0,0,NULL);
#ifdef CALC_DURATION_TESTS
               execFunc("prtFifoTime","Cntlr ExpCmplt:", NULL, NULL, NULL, NULL, NULL,NULL,NULL);
#endif
               if ( instrWrdsLost != 0)
               {
    	          errLogRet(LOGIT,debugInfo,
			"Master: Lost Intruction Fifo words, sent: %lu, recv'd: %lu, LOST: %ld\n",
				sentcnt,ifcount,instrWrdsLost);
                  sendException(HARD_ERROR, SYSTEMERROR + FIFO_WRDS_LOST, 0,0,NULL);
              }

               DPRINT(-1,"Sent Master Exp. Completed State\n");
           }
           /* prevent false starts from Sync glitches, controller reboots, or FPGA reloads */
           cntrlFifoClearStartMode();

           resumeLedShow(); /* resume LED Idle display */

#ifdef INOVA
	   currentStatBlock.stb.AcqOpsComplFlags |= EXP_CMPLT_FLAG;
	   setVTinterLk(0);
	   setLKInterlk(0);
	   setSpinInterlk(0);
           update_acqstate(ACQ_IDLE);
	   getstatblock();
   	   rngFlush(pSyncActionArgs);

           /* PARALLEL_CHANS */
           freeSorter();
	   clearParallelFreeBufs();

           set2ExpEndState();  /* reprogram HSlines, ap registers to safe state */

           fifoClrStart4Exp(pTheFifoObject);
	   if (pTheTuneObject != NULL)
	     semGive( pTheTuneObject->pSemAccessFIFO );
#endif
#ifdef TIMING_DIAG_ON    /* compile in timing diagnostics */
      resetTimeStamp();
#endif
		break;

      default:
		/* Who Cares */
		break;
   }
}

