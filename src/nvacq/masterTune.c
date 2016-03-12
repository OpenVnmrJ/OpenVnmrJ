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
#ifndef ALLREADY_POSIX
 #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#endif
#include <string.h>
#include <vxWorks.h>
#include <stdioLib.h>
#include <sysLib.h>
#include <semLib.h>
#include <rngLib.h>
#include <msgQLib.h>

#include "nvhardware.h"
#include "errorcodes.h"
#include "instrWvDefines.h"
#include "taskPriority.h"

#include "logMsgLib.h"

#include "NDDS_Obj.h"
#include "NDDS_PubFuncs.h"
#include "NDDS_SubFuncs.h"

#include "cntlrStates.h"
#include "Console_Stat.h"
#include "Cntlr_Comm.h"
#include "master.h"
#include "tune.h"
#include "nsr.h"


extern int DebugLevel;

extern int  BrdType;    /* Boardtype, RF, Master, PFG, DDR, Gradient, Etc. */
extern int  BrdNum;     /* The Board types Ordinal number, i.e. rf1 or rf2 */
extern int nsrMixerBand[];
extern Console_Stat     *pCurrentStatBlock;     /* Acqstat-like status block */

extern SEM_ID pSemOK2Tune;

void enableProbeTuneIsr(int);
void masterTune();
void probeISR(int);
void tuneISR(int);

SEM_ID pTuneItrSem = NULL;
MSG_Q_ID  pTuneMsgQ = NULL;

/*
 * Start the appropriate task for the controller type.
 *
 *     Author:  Greg Brissey 12/20/04
 */
void initialTuneTask()
{
   startTuneTask(TUNE_TASK_PRIORITY, STD_TASKOPTIONS, XSTD_STACKSIZE);
}

/*
 *   Spawn the Master Tune Task
 *
 *     Author:  Greg Brissey 12/20/04
 */
startTuneTask(int priority, int taskoptions, int stacksize)
{
   if (pTuneItrSem == NULL)
   {
      pTuneItrSem = semBCreate(SEM_Q_FIFO,SEM_EMPTY);
      if ( (pTuneItrSem == NULL) )
      {
        errLogSysRet(LOGIT,debugInfo,
	   "startMasterTuneTask: Failed to allocate pTuneItrSem Semaphore:");
        return(ERROR);
      }
   }

   // enableProbeTuneIsr(1);
   
   if (taskNameToId("tTune") == ERROR)
      taskSpawn("tTune",priority,0,stacksize,masterTune,1,2,3,4,5,6,7,8,9,10);
}


void enableProbeTuneIsr(int enable)
{
   if ( ! enable ) {
      DPRINT(-1,"Removing Probe/Tune from interrupt service list\n");
      fpgaIntRemove(probeISR,0);
      set_field(MASTER,probe_id_int_enable,0);
      fpgaIntRemove(tuneISR,0);
      set_field(MASTER,tune_int_enable,0);
   }
   else {
      DPRINT(-1,"Connecting Probe/Tune to interrupt service list\n");
      hsspi(1,0x4801); // clear int status if needed
      hsspi(1,0x4800);
      fpgaIntConnect(probeISR,0,1<<MASTER_probe_id_int_enable_pos);
      set_field(MASTER,probe_id_int_enable,1);
      set_field(MASTER,probe_id_int_clear,0);
      set_field(MASTER,probe_id_int_clear,1);
      fpgaIntConnect(tuneISR, 0,1<<MASTER_tune_int_enable_pos);
      set_field(MASTER,tune_int_enable,1);
      set_field(MASTER,tune_int_clear,0);
      set_field(MASTER,tune_int_clear,1);
   }
}

void probeISR(int zero)
{
unsigned int value;
   logMsg("probeISR interrupt: parameter %d\n", zero,2,3,4,5,6);
   value = hsspi(1,0xC00000);
   value &= 0xB7FFFF; value |= 0x40000;
   value = hsspi(1,value);
   value &= 0xB7FFFF;
   value = hsspi(1,value);
/*   set_field(MASTER,probe_id_int_clear,0);      // clear interrupt
/*   set_field(MASTER,probe_id_int_clear,1);
 */
}

void tuneISR(int zero)
{
unsigned int value, clrCnt;
   if (DebugLevel > 0)
      logMsg("tuneISR interrupt: parameter %d\n", zero,2,3,4,5,6);
   clrCnt = 0;
   while (clrCnt < 10)
   { 
      hsspi(1, (NSR_INT_ADDR | 0x1) );	// set clear interrupt
      hsspi(1, (NSR_INT_ADDR | 0x0) );	// clr clear interrupt
      value = hsspi(1, (NSR_READ | NSR_INT_ADDR) );
      if ( ! (value&0x1) ) break;
      clrCnt++;
      DPRINT2(-1,"tuneISR: val=0x%x, clrCnt=%d",value,clrCnt);
   }
   semGive(pTuneItrSem);
}

tshow()
{
int i= hsspi(1,0xc800);
    printf("State of NSR interrupt bit is %x\n",i);
}

tclear()
{
int i=1,j=0;
   while (i!=0) {
       hsspi(1,0x4801);
       hsspi(1,0x4800);
       i=hsspi(1,0xc800);
       printf("j=%d State of NSR interrupt bit is %x\n",j,i);
   }
}

/*
 *   Master Tune Task
 *   Task waits for the tune interrupt semaphore to be given t
 *   If the pSemOK2Tune is free then continue with else just return
 *   ignoring it.
 *
 *     Author:  Greg Brissey 12/20/04
 */
void masterTune()
{
   int result;
   FOREVER
   {
       semTake(pTuneItrSem, WAIT_FOREVER);
       DPRINT(-1,"took semaphore\n");
       if ( (result = semTake(pSemOK2Tune,sysClkRateGet()*TUNE_TIMEOUT)) != OK)
       {
            /* Nope can not Tune do anything here necessary if any */
            continue;
       }
       DPRINT(-1,"no acquire, go tune\n");
       /* all controllers are involved in tuning */
       cntlrSetInUseAll(CNTLR_INUSE_STATE);
       tuneIt();
       semGive(pSemOK2Tune); // so we can 'go' again
   }
}

/*
 * tuneIt()
 * Called for Tuning the probe to any of the RF and/or lock
 * channels.
 * 
 * Once call it loops forever waiting on the tune switch
 * interrupt semaphore for changes in the channel or attenuation
 * When the channel turn to zero tuning operation is done
 * and the routine returns
 *
 * Since there are multiple controllers involved
 * The master 1st does what it needs to do then
 * send a message to all the other controllers to perform
 * there operations and the master wiat for them all
 * to report completion.
 *
 * Tune operation involves threes steps.
 *  1st. Make all controllers go 'quite'
 *  2nd  Have the actual tune enabled and/or change
 *       attenuation, etc..
 *  3rd  Finish tune
 *
 * Logic may change as we actually implement the specifics.
 *
 *     Author:  Greg Brissey 12/20/04
 */

/* for debuggin only */
int tstTuneChan = 0;
int tstTuneAttn;

tuneIt ()
{
   int result,waitTimeout,cmpltCode;
   int channel, atten, arg2, arg3, len;
   int done;
int tmpInt;
   char msgStr[80];
   char cntlrList[128];
   msgStr[0] = 0;
   len = 0;
   done = 0;
   waitTimeout = 30; /* 30 seconds to complete. */
   cmpltCode = CNTLR_TUNE_ACTION_CMPLT;
   FOREVER
   {
      /* Debounce the circuit */
      taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
      if (tstTuneChan)
      {
         channel = tstTuneChan;
         atten = tstTuneAttn;
      }
      else
      {
         /* read SPI */
         channel = hsspi(1,0x8000);	// read switches at address 0
         channel = hsspi(1,0x8000);
         atten   = channel;
         channel &= 0xf;		// lower 4 bits only
         atten    = (atten>>4) & 0xf;// upper 4 bits only
      }
      DPRINT2(-1,"Channel=%d (0x%x)\n",channel, channel);
      DPRINT2(-1,"Atten=%d (0x%x)\n",atten, atten);

      /*  if done return */
      if ( (channel == 0) || (channel > 9) )
      {
         DPRINT(-1,"channel = 0, returning...\n");
         /* 1st command all controller to go quite */
         cntlrSetStateAll(CNTLR_NOT_READY);
         cntlrSetState("master1",cmpltCode,0);
         send2AllCntlrs(CNTLR_CMD_TUNE_FINI,channel,atten,arg3,msgStr,len);
         DPRINT(-1,"Wait on controllers to complete FINI \n");
         /* Wait for all to complete.... */
         result = cntlrStatesCmpList(CNTLR_TUNE_ACTION_CMPLT, 
			waitTimeout, cntlrList);  /* one minute to complete */
         if (result > 0)
         {
            DPRINT(-1,"========>> TUNE Fini: FAILURE \n");
            DPRINT1(-1,"   Controllers: '%s' FAILED to finish before timeout\n",
                          cntlrList);
         }

         /* 2nd let the master do its thing */
         setNSR4Observe(0,0,1);

         hsspi(1, (NSR_TM_ADDR | 0x0) );  // turn off light
	 pCurrentStatBlock->Acqstate = ACQ_IDLE;
         /* double check the status, ring bell if not clear */
         /* tmpInt = hsspi(1,0xc800);
         /* if (tmpInt)
         /*    DPRINT(-1," *\n*\n*\n*\n");
         /* DPRINT1(-1,"tmpInt=%x\n",tmpInt);
         /* */

         return;
      }

      /* 1st command all controller to go quiet */
      cntlrSetStateAll(CNTLR_NOT_READY);
      cntlrSetState("master1",cmpltCode,0);	// the master knows
      send2AllCntlrs(CNTLR_CMD_TUNE_QUIET,channel,atten,arg3,msgStr,len);
      DPRINT(-1,"Wait on controllers to complete QUIET\n");
      /* Wait for all to complete.... */
      result = cntlrStatesCmpList(CNTLR_TUNE_ACTION_CMPLT,
			 waitTimeout, cntlrList);  /* one minute to complete */
      if (result > 0)
      {
         DPRINT(-1,"========>> TUNE Quite: FAILURE \n");
         DPRINT1(-1,"   Controllers: '%s' FAILED  to go quiet before timeout\n",
                        cntlrList);
      }

      /* 2nd  let the master do its thing */
      /* setNSR4Tune(channel,gain,hilo); */
      set2sw_fifo(0);                   // allow direct control of registers
      auxWriteReg(5, 0);                // bypass CMA if present
      auxWriteReg(0, 0);                // put the LO relay to standard..
      setNSR4Tune(channel,60,nsrMixerBand[channel-1]);
      taskDelay(calcSysClkTicks(83));  /* taskDelay(5), wait 83 ms for relays to settle */

      /* 3rd command all controller to tune, each controller should know 
	 what it needs to do */
      cntlrSetStateAll(CNTLR_NOT_READY);
      cntlrSetState("master1",cmpltCode,0);
      send2AllCntlrs(CNTLR_CMD_TUNE_ENABLE,channel,atten,arg3,msgStr,len);
      DPRINT(-1,"Wait on controllers to complete ENABLE\n");
      /* wait for acknowledgement */
      result = cntlrStatesCmpList(CNTLR_TUNE_ACTION_CMPLT,
			 waitTimeout, cntlrList);  /* one minute to complete */
      if (result > 0)
      {
         DPRINT(-1,"========>> TUNE ENABLE: FAILURE \n");
         DPRINT1(-1,"     Controllers: '%s' FAILED  to enable before timeout\n",
                        cntlrList);
      }

      /* turn on control light */
      hsspi(1, (NSR_TM_ADDR | 0x1) );  // turn on light
      pCurrentStatBlock->Acqstate = ACQ_TUNING;

      /* double check the status, ring bell if not clear */
      /* tmpInt = hsspi(1,0xc800);
      /* if (tmpInt)
      /*    DPRINT(-1," *\n*\n*\n*\n");
      /* DPRINT1(-1,"tmpInt=%x\n",tmpInt); */
      /* wait for interrupt; to stop, change channel or set new attenuation */
      DPRINT(-1,"Waiting for next button change...\n");
      semTake(pTuneItrSem, WAIT_FOREVER);
      DPRINT(-1,"Took another semaphore");
   }
}
 
fakeTuneItr(int chan, int attn)
{
   tstTuneChan = chan;
   tstTuneAttn = attn;
  
   semGive(pTuneItrSem);
}

void tuneTest()
{
    char answer;
    int done=0;
    char retchar;
    char ians;

    printf("T u n e   T e s t\n\n");
    while ( ! done )
    {
       printf("Enter 1 to select channel. Current channel is %d\n", tstTuneChan);
       printf("Enter 2 to select atten.   Current atten   is %d\n", tstTuneAttn);
       printf("Enter 0 to exit\n");
       printf("\nEnter 1, 2, or 0: ");
       answer = getchar();
       retchar=' ';
       while (retchar != '\n')
          retchar = getchar();
       switch(answer) {
       case '1':
           printf("Enter Channel (1-9): ");
           ians = getchar();
           retchar=' ';
           while (retchar != '\n')
              retchar = getchar();
           if ( (ians >= '1') && (ians <= '9'))
           {
              tstTuneChan = (int) (ians - '0');
              semGive(pTuneItrSem);
           }
           else
              printf("Channel must be 1-9\n");

           break;
       case '2':
           printf("Enter Atten (0-9): ");
           ians = getchar();
           retchar=' ';
           while (retchar != '\n')
              retchar = getchar();
           if ( (ians >= '0') && (ians <= '9'))
           {
              tstTuneAttn = (int) (ians - '0');
              if (tstTuneChan)
              {
                 semGive(pTuneItrSem);
              }
           }
           else
              printf("Attn must be 0-9\n");
           break;
       case '0':
           done = 1;
           if (tstTuneChan)
           {
              tstTuneChan = 0;
              semGive(pTuneItrSem);
           }
           break;
       default:
           printf("Select 1,2, or 0\n");
           break;
       }
       printf("\n\n");
   }
}


killTune()
{
   int tid;
   if ((tid = taskNameToId("tTune")) != ERROR)
      taskDelete(tid);
}
