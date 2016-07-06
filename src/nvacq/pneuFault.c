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
#include <taskLib.h>
#include <msgQLib.h>
#include <wdLib.h>
#include "logMsgLib.h"
#include "Console_Stat.h"
#include "taskPriority.h"
#include "errorcodes.h"
#include "expDoneCodes.h"
#include "nvhardware.h"
#include "master.h"


/* pneuFault.h */
struct pneuInfo {
    SEM_ID pneuFaultSem;
    int    status;
    int    pneuErr;
    int    pneuInterlock;
} PNEU_OBJ;

typedef struct pneuInfo *PNEU_ID;

void     startPneuFault(void);
PNEU_ID  pneuFaultCreate(void);
void     pneuFaultTask(void);
void     pneuFaultISR(void);
void     clearPneuFault(void);
int      getPneuFault(void);
void     setPneuFaultInterlock(int state);
int      getPneuFaultInterlock(void);


/* pneuFault.h */

PNEU_ID  pPneuInfo = NULL;

void startPneuFault()
{
int tid;
PNEU_ID s;
   s = pneuFaultCreate();
   if (s == 0) 
   { 
      DPRINT(-1,"pneuFaultCreate failed\n");
      return;
   }
   DPRINT( 1,"pneuFault started \n");
}

PNEU_ID pneuFaultCreate()
{
   /* create a pneumatics fault object */
   if ( (pPneuInfo = (PNEU_ID) malloc( sizeof(PNEU_OBJ)) ) == NULL )
   {
      errLogSysRet(LOGIT,debugInfo,"pneuFaultCreate: Could not Allocate Space:");
      return(NULL);
   }
   /* clear the object */
   memset( pPneuInfo,0,sizeof(PNEU_OBJ) );

   /* create a binary semaphore */
   pPneuInfo->pneuFaultSem = semBCreate(SEM_Q_FIFO, SEM_EMPTY);
   if (pPneuInfo->pneuFaultSem == NULL)
   {
      free(pPneuInfo);
      pPneuInfo = NULL;
      return(NULL);
   }

   if (taskNameToId("tPneuFault") == ERROR)
      taskSpawn("tPneuFault",PNEUFAULT_PRIORITY,STD_TASKOPTIONS,STD_STACKSIZE,
			(FUNCPTR)pneuFaultTask,1,2,3,4,5,6,7,8,9,10);
   /* initialize the ISR */
   fpgaIntConnect(pneuFaultISR, pPneuInfo,
                              (1<<MASTER_pneumatic_fault_int_status_pos) );
   set_field(MASTER,pneumatic_fault_int_enable,1);
   return(pPneuInfo);
   DPRINT( 1,"pneuFault Created \n");
}

void pneuShow()
{
   if (pPneuInfo == NULL) 
   {   DPRINT(-1,"pPneuInfo is NULL\n");
       return;
   }
   DPRINT1(-1,"semaphore=%x\n",pPneuInfo->pneuFaultSem);
   DPRINT1(-1,"status=%x\n",pPneuInfo->status);
   switch (pPneuInfo->pneuErr - 1900)
   {  
      case 1:		// PS
            DPRINT1(-1,"pneuErr=%d POWEr SUPPLY\n",pPneuInfo->pneuErr);
            break;
      case 2:		// VT THRESHOLD
            DPRINT1(-1,"pneuErr=%d VT THRESHOLD EXCEEDED\n",pPneuInfo->pneuErr);
            break;
      case 3:		// NB STACK
            DPRINT1(-1,"pneuErr=%d NB STACK FAULT\n",pPneuInfo->pneuErr);
            break;
      case 4:		// PRESSURE
            DPRINT1(-1,"pneuErr=%d INTAKE PRESSURE < 20 PSI\n",pPneuInfo->pneuErr);
            break;
      default:		// Huh?
            DPRINT1(-1,"pneuErr=%d NONE\n",pPneuInfo->pneuErr);
            break;
   }
   switch (pPneuInfo->pneuInterlock)
   {
      case 0:
            DPRINT1(-1,"pneuInterlock=%d NONE\n",pPneuInfo->pneuInterlock);
            break;
      case HARD_ERROR:
            DPRINT1(-1,"pneuInterlock=%d HARD_ERROR\n",pPneuInfo->pneuInterlock);
            break;
      case WARNING_MSG:
            DPRINT1(-1,"pneuInterlock=%d WARNING_MSG\n",pPneuInfo->pneuInterlock);
            break;
      default:
            DPRINT1(-1,"pneuInterlock=%d Unknown\n",pPneuInfo->pneuInterlock);
            break;
   }
}


void pneuFaultISR()
{
   if (pPneuInfo == NULL) return;
   if (pPneuInfo->pneuInterlock < 1) return;  // pin='n'
   semGive(pPneuInfo->pneuFaultSem);
}

void pneuFaultTask()
{
int stat,errLvl;
   FOREVER {
      semTake(pPneuInfo->pneuFaultSem, WAIT_FOREVER);

      stat = pPneuInfo->status = hsspi(2,0x3000000);
      DPRINT1(-1,"Pneu Fault: stat 0x%x\n",stat);
      pPneuInfo->pneuErr = 0;
      if ( ! (stat & 0x0001))	// power supply fault
         pPneuInfo->pneuErr = PNEU_ERROR + PS;
      if (stat & 0x0020)		// VT Threshold exceeded
         pPneuInfo->pneuErr = PNEU_ERROR + VTTHRESH;
      if ( ! (stat & 0x0040))	// NB stack fault
         pPneuInfo->pneuErr = PNEU_ERROR + NB_STACK;
      if (stat & 0x0080)		// pressure switch fault
         pPneuInfo->pneuErr = PNEU_ERROR + PRESSURE;
      /* If status word does not indicate what caused error, assume it is power supply */
      if (pPneuInfo->pneuErr == 0)
         pPneuInfo->pneuErr = PNEU_ERROR + PS;
      DPRINT1(-1,"Pneu Fault: %d\n",pPneuInfo->pneuErr);
      sendException(pPneuInfo->pneuInterlock,pPneuInfo->pneuErr, 0,0,NULL);
   }
}


void clearPneuFault()
{
   if (pPneuInfo == NULL) return;
   pPneuInfo->status = hsspi(2,0x3000000);
/* what if status still shows an error, no new interrupt to reset */
   hsspi(2,0x5000001);		// clear the bit
   pPneuInfo->pneuErr = 0;
}

void testAndClearPneuFault()
{
   if (pPneuInfo == NULL) return;
   pPneuInfo->status = hsspi(2,0x3000000);
   if (pPneuInfo->status & 0x0020)
   {
      int led;
      int limits = getVTAirLimits();
      led = (pPneuInfo->status>>13) & 0x3ff;
      if ( (led & limits) == limits)
      { 
         DPRINT(-1,"reset Pneumatics\n");
         hsspi(2,0x5000001);		// clear the bit
         pPneuInfo->pneuErr = 0;
      }
   }
   else
   {
      // If we had a power supply error and it is now okay, clear pneuErr.
      if ( (pPneuInfo->pneuErr == PNEU_ERROR + PS) &&
           (pPneuInfo->status & 0x0001))
         pPneuInfo->pneuErr = 0;
   }
}

int getPneuFault()
{
   if (pPneuInfo == NULL) return(0);
   return( pPneuInfo->pneuErr );
}

void setPneuFaultInterlock(int state)
{
   if (pPneuInfo == NULL) return;
   pPneuInfo->pneuInterlock = state;
}

int getPneuFaultInterlock()
{
   if (pPneuInfo == NULL) return(0);
   return( pPneuInfo->pneuInterlock );
}
