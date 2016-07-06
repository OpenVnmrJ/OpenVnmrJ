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
#include "Cntlr_Comm.h"
/* #include "master.h" */
#include "ddr.h"
#include "tune.h"
#include "rfinfo.h"
#include "sysUtils.h"

#define VPSYN(arg) volatile unsigned int *p##arg = (unsigned int *) (FPGA_BASE_ADR + arg)

extern int    DebugLevel;
extern int    BrdType;    /* Boardtype, RF, Master, PFG, DDR, Gradient, Etc. */
extern int    BrdNum;     /* The Board types Ordinal number, i.e. rf1 or rf2 */
extern SEM_ID pSemOK2Tune;

VPSYN(DDR_SoftwareGates);
VPSYN(DDR_FIFOOutputSelect);
void cntlrTune();

SEM_ID pTuneItrSem = NULL;
MSG_Q_ID  pTuneMsgQ = NULL;

/*
 * common start up function for the Tune task.
 * Start the appropraite task for the controller type.
 *
 *     Author:  Greg Brissey 12/20/04
 */

void initialTuneTask()
{
   startTuneTask(TUNE_TASK_PRIORITY, STD_TASKOPTIONS, XSTD_STACKSIZE);
}

/*
 *   Spawn the Non-Master Tune Task
 *
 *     Author:  Greg Brissey 12/20/04
 */
int startTuneTask(int priority, int taskoptions, int stacksize)
{
   if (pTuneMsgQ == NULL)
   {
      pTuneMsgQ = msgQCreate(10, sizeof(TUNE_MSG), MSG_Q_FIFO);
      if (pTuneMsgQ == NULL)
      {
          errLogSysRet(LOGIT,debugInfo,
	      "startCntlrTuneTask: Failed to allocate pTuneMsgQ MsgQ:");
          return(ERROR);
      }
   }
   
   if (taskNameToId("tTune") == ERROR)
      taskSpawn("tTune",priority,0,stacksize,cntlrTune,1,2,3,4,5,6,7,8,9,10);
}

/*
 *   Controller Tune Task
 *
 *   This task is active on all none master controllerss
 *
 *   Task waits for Messages on it's MsgQ 
 *   Sent via the CntlrPlexus in nexus.c
 *   Passes this message on to the decoder where the proper
 *   Operation for this controller is performed.
 *
 *     Author:  Greg Brissey 12/20/04
 */
void cntlrTune()
{
   void tuneDecode();
   TUNE_MSG Msge;
   int bytes;

   FOREVER
   {
     bytes = msgQReceive(pTuneMsgQ, (char*) &Msge,sizeof(TUNE_MSG), WAIT_FOREVER);
     DPRINT1(2,"cntlrTuneTask: got %d bytes\n",bytes);

     tuneDecode( &Msge );
   } 
     
}

/*
 *   The tune decoder where the proper
 *   operation for this controller is performed.
 *
 *     Author:  Greg Brissey 12/20/04
 */
tuneDecode(TUNE_MSG *msge)
{
   int result,cmpltCode;
   char msgstr[COMM_MAX_STR_SIZE+2];

   cmpltCode = CNTLR_TUNE_ACTION_CMPLT;
   DPRINT3( 1,"tuneDecode: cmd: %d, channel: %d, atten: %d \n",msge->cmd,msge->channel,msge->arg2);
    switch(msge->cmd)
    {
       case CNTLR_CMD_TUNE_QUIET:
       {
            cntlrTuneQuiet(msge);
            send2Master(CNTLR_CMD_STATE_UPDATE,cmpltCode,0,0,NULL);
       }
       break;

       case CNTLR_CMD_TUNE_ENABLE:
       {
            cntlrTuneEnable(msge);
            send2Master(CNTLR_CMD_STATE_UPDATE,cmpltCode,0,0,NULL);
       }
       break;

       case CNTLR_CMD_TUNE_FINI:
       {
            DPRINT( 1," -- CNTLR_CMD_TUNE_FINI --\n");
            cntlrTuneQuiet(msge);
            send2Master(CNTLR_CMD_STATE_UPDATE,cmpltCode,0,0,NULL);
       }
       break;

       default:
          break;
    }
}

/*
 * set the controller into the quiet mode for tune
 * action depends on type of controller (BrdType)
 *
 *     Author:  Frits Vosman 12/27/04
 */
cntlrTuneQuiet(TUNE_MSG *msge)
{
   DPRINT( 1," -- CNTLR_CMD_TUNE_QUIET --\n");
   switch(BrdType)
   {
      case MASTER_BRD_TYPE_ID:
         DPRINT(-1,"cntlrTuneQuiet: Master, should never get here");
         break;
      case RF_BRD_TYPE_ID:
         DPRINT(1,"cntlrTuneQuiet: RF Tune Off");
         /* TuneOFF(); */
         execFunc("TuneOFF",NULL, NULL, NULL, NULL, NULL, NULL,NULL,NULL);
         break;
      case PFG_BRD_TYPE_ID:
      case LPFG_BRD_TYPE_ID:
         DPRINT(1,"cntlrTuneQuiet: PFG ignores");
         break;
      case GRAD_BRD_TYPE_ID:
         DPRINT(1,"cntlrTuneQuiet: Grad ignores");
         break;
      case LOCK_BRD_TYPE_ID:
         DPRINT(1,"cntlrTuneQuiet: Lock ignores");
         break;
      case DDR_BRD_TYPE_ID:
         *pDDR_SoftwareGates &= ~0x8;
         *pDDR_FIFOOutputSelect = 1;
         DPRINT(1,"cntlrTuneQuiet: DDR RG On");
         break;
      default:
         DPRINT(-1,"cntlrTuneQuiet: Unknown cntlr, ignored");
         break;
   }
}

/*
 * set the controller into the quiet mode for tune
 * action depends on type of controller (BrdType)
 *
 *     Author:  Frits Vosman 12/27/04
 */
cntlrTuneEnable(TUNE_MSG *msge)
{
   int activeChannel;
   activeChannel = msge->channel - 1;
   DPRINT1( 1," -- CNTLR_CMD_TUNE_ENABLE --  chan: %d\n",activeChannel);
   switch(BrdType)
   {
      case MASTER_BRD_TYPE_ID:
         DPRINT(1,"cntlrTuneEnable: Master, should not get here");
         break;
      case RF_BRD_TYPE_ID:
         if (BrdNum ==  activeChannel)
         {
            DPRINT3(-1,"cntlrTuneEnable: RF chan=%d, activate=%d, yes, attn=%d",
                                              BrdNum, activeChannel,msge->arg2);
            /* TuneON(frequency,power); */
            /* TuneON(msge->arg2*5); */
            execFunc("TuneON",(void*)(msge->arg2*5), NULL, NULL, NULL, NULL, NULL,NULL,NULL);
         }
         else
         {
            DPRINT2(1,"cntlrTuneEnable: RF chan=%d, activate=%d, no",
                                              BrdNum, activeChannel);
         }
         break;
      case LPFG_BRD_TYPE_ID:
      case PFG_BRD_TYPE_ID:
         DPRINT(1,"cntlrTuneEnable: PFG ignores");
         break;
      case GRAD_BRD_TYPE_ID:
         DPRINT(1,"cntlrTuneEnable: Grad ignores");
         break;
      case LOCK_BRD_TYPE_ID:
         DPRINT(1,"cntlrTuneEnable: Lock ignores");
         break;
      case DDR_BRD_TYPE_ID:
         *pDDR_FIFOOutputSelect = 0;
         *pDDR_SoftwareGates |= 0x8;
         DPRINT(1,"cntlrTuneEnable: DDR RG Off");
         break;
      default:
         DPRINT(-1,"cntlrTuneEnable: Unknown cntlr, ignored");
         break;
   }
}

/*
 * testing routine and parameters
 *
 */
fakeTuneItr(int chan, int attn)
{
   semGive(pTuneItrSem);
}


killTune()
{
   int tid;
   if ((tid = taskNameToId("tTune")) != ERROR)
      taskDelete(tid);
}

