/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* adcObj.c  11.1 07/09/07 - ADC Object Source Modules */
/* 
 */


#define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#include <vxWorks.h>
#include <stdlib.h>
#include <vme.h>
#include <semLib.h>
#include "hardware.h"
#include "taskPrior.h"
#include "commondefs.h"
#include "logMsgLib.h"
#include "vmeIntrp.h"
#include "adcObj.h"


/*
modification history
--------------------
9-21-93,gmb  created 
*/

/*
DESCRIPTION


*/

static ADC_OBJ *adcObj;
SEM_ID pSemAdcOvrFlow;  /* cnting semphore given by ISR to trigger IST */

/*-------------------------------------------------------------
| Interrupt Service Tasks (IST) 
+--------------------------------------------------------------*/
/***********************************************************
*
* adcOvrFlow - Interrupt Service Task 
*
*  Action to take:
*   3. Update adcState, flush semaphore
*
*/
static VOID adcOvrFlow(register ADC_ID pAdcId)
{
   int adcReset();

   FOREVER
   {
     semTake(pSemAdcOvrFlow,WAIT_FOREVER); /* wait here for interrupt */

     semTake(pAdcId->pAdcMutex,WAIT_FOREVER); /*Mutual Exclusion Semiphore */

     pAdcId->adcState = ADC_OVERFLOW;

     semGive(pAdcId->pAdcMutex);	/* give Mutex back */

     semFlush(pAdcId->pSemAdcStateChg);/* release any task Block on statchg for FIFO */
   }
}

/*-------------------------------------------------------------
| ADC Object Public Interfaces
+-------------------------------------------------------------*/

/**************************************************************
*
*  adcCreate - create the ADC Object Data Structure & Semiphores
*
*
* RETURNS:
* OK - if no error, NULL - if mallocing or semiphore creation failed
*
*/ 

ADC_ID adcCreate(char* baseAddr,int vector,int level)
/* char* baseAddr - base address of ADC */
/* int   vector  - VME Interrupt vector number */
/* int   level   - VME Interrupt level */
{
    char sr;

     /* ------- malloc space for adc Object --------- */
     if ( (adcObj = (ADC_OBJ *) malloc( sizeof(ADC_OBJ) ) ) == NULL )
     {
       LOGMSG(ALL_PORTS,ERROR,"adcCreate: ");
       return(NULL);
     }

     /* zero out structure so we don't free something by mistake */
     memset(adcObj,0,sizeof(ADC_OBJ));

     /* ------ Translate Bus address to CPU Board local address ----- */
     if (sysBusToLocalAdrs(VME_AM_SUP_SHORT_IO,
                         baseAddr,&(adcObj->adcBaseAddr)) == -1)
     {
       LOGMSG(ALL_PORTS,ERROR,"adcCreate: ");
       free(adcObj);
       return(NULL);
     }

     /* ------ Test for Boards Presents ---------- */
     if ( vxMemProbe((char*) (adcObj->adcBaseAddr + ADC_SR), 
		     VX_READ, BYTE, &sr) == ERROR)
     { 
       LOGMSG(ALL_PORTS,OK,"adcCreate: Could not read ADC's Status register\n");
       free(adcObj);
       return(NULL);
     }
     else
     {
        adcObj->adcBrdVersion = (sr >> 4) & 0x0f;
        LOGMSG1(ALL_PORTS,OK,"adcCreate: ADC Board Version %d present.\n",
		adcObj->adcBrdVersion);
     }
     
     adcObj->pSemAdcStateChg = semBCreate(SEM_Q_FIFO,SEM_EMPTY);

     adcObj->pAdcMutex =  semMCreate(SEM_Q_PRIORITY | 
				            SEM_INVERSION_SAFE |
                                            SEM_DELETE_SAFE);

     pSemAdcOvrFlow = semCCreate(SEM_Q_FIFO,SEM_EMPTY);

     if ( (adcObj->pSemAdcStateChg == NULL) || 
          (adcObj->pAdcMutex == NULL) || 
          (pSemAdcOvrFlow == NULL) )
     {
        LOGMSG(ALL_PORTS,ERROR,"adcCreate: ");

	if (adcObj->pSemAdcStateChg != NULL)
	   semDelete(adcObj->pSemAdcStateChg);
	if (adcObj->pAdcMutex != NULL)
	   semDelete(adcObj->pAdcMutex);

	if (pSemAdcOvrFlow != NULL)
	   semDelete(pSemAdcOvrFlow);

        return(NULL);
     }

     if ( (vector < MIN_VME_ITRP_VEC) || (vector > MAX_VME_ITRP_VEC) )
     {
        LOGMSG3(ALL_PORTS,OK,"adcCreate: vector: 0x%x out of bounds (0x%x-0x%x)\n",
		vector,MIN_VME_ITRP_VEC,MAX_VME_ITRP_VEC);
	semDelete(adcObj->pSemAdcStateChg);
	semDelete(adcObj->pAdcMutex);
        free(adcObj);
	return(NULL);
     }
     else
     {
        adcObj->vmeItrVector = vector;
     }

     if ( (level >= MIN_VME_ITRP_LEVEL) && (level <= MAX_VME_ITRP_LEVEL) )
     {
       adcObj->vmeItrLevel = level;
     }
     else
     {
        LOGMSG3(ALL_PORTS,OK,"stmCreate: vme level: %d out of bounds (%d-%d)\n",
	   level,MIN_VME_ITRP_LEVEL,MAX_VME_ITRP_LEVEL);
	semDelete(adcObj->pSemAdcStateChg);
	semDelete(adcObj->pAdcMutex);
        free(adcObj);
        return(NULL);
     }

     adcObj->adcControl = 0;
     adcObj->adcState = OK;

     /*
     intConnect( INUM_TO_IVEC( adcObj->vmeItrVector ),  semGive, pSemAdcOvrFlow);
     */

     /* Spawn the Interrupt Service Tasks */

/*
     taskSpawn("tAdcOvrFlow", ADC_FULL_IST_PRIORTY, ADC_IST_TASK_OPTIONS,
		ADC_IST_STACK_SIZE, adcOvrFlow, adcObj, ARG2,
		ARG3,ARG4,ARG5,ARG6,ARG7,ARG8,ARG9,ARG10);
*/

     return( adcObj );
}


/**************************************************************
*
*  adcGetState - Obtains the current ADC Status
*
*  This routines Obtains the status of the ADC via 3 different modes.
*
*   NO_WAIT - return the present value immediately.
*   WAIT_FOREVER - waits till the ADC Status has changed 
*			and and returns this new value.
*   TIME_OUT - waits till the ADC Status has changed or 
*		    the number of <secounds> has elasped 
*		    (timed out) before returning.
*
*  NOTE: The Task that calls this routine with 
*	 WAIT_FOREVER or TIME_OUT will block !!
*     
*
*
* RETURNS:
* Adc state - if no error, TIME_OUT - if in TIME_OUT mode call timed out
*
*/ 

int adcGetState(ADC_ID pAdcId, int mode, int secounds)
/* ADC_ID pAdcId;   ADC Object */
/* int mode;	    mode of call, see above */
/* int secounds;    number of secounds to wait before timing out */
{
   int state;
   if (pAdcId == NULL)
     return(ERROR);
   switch(mode)
   {
     case NO_WAIT:
          state = pAdcId->adcState;
	  break;

     case WAIT_FOREVER: /* block if state has not changed */
	      semTake(pAdcId->pSemAdcStateChg, WAIT_FOREVER);  
          state = pAdcId->adcState;
	  break;

     case TIME_OUT:     /* block if state has not changed, until timeout */
          if ( semTake(pAdcId->pSemAdcStateChg, (sysClkRateGet() * secounds) ) != OK )
	         state = TIME_OUT;
          else 
             state = pAdcId->adcState;
          break;

     default:
	  state = ERROR;
	  break;
   }
   return(state);
}

/**************************************************************
*
*  adcItrpEnable - enable ADC interrupt 
*
*
* RETURNS:
*  OK  or Error if invalid ADC_ID pointer
* 
*/

int adcItrpEnable(register ADC_ID pAdcId)
/* ADC_ID pAdcId;   ADC Object */
{
   if (pAdcId == NULL)
     return(ERROR);
   pAdcId->adcControl |= BIT_MASK(ADC_EN_ITRP);
   *ADC_REG(ADC_CR,pAdcId->adcBaseAddr) = pAdcId->adcControl;
   sysIntEnable(pAdcId->vmeItrLevel);
}

/**************************************************************
*
*  adcItrpDisable - disable ADC interrupt 
*
*
* RETURNS:
*  OK  or Error if invalid ADC_ID pointer
* 
*/

int adcItrpDisable(register ADC_ID pAdcId)
/* ADC_ID pAdcId;   ADC Object */
{
   if (pAdcId == NULL)
     return(ERROR);
   pAdcId->adcControl &= ~BIT_MASK(ADC_EN_ITRP);
   *ADC_REG(ADC_CR,pAdcId->adcBaseAddr) = pAdcId->adcControl;
   sysIntDisable(pAdcId->vmeItrLevel);
}

/**************************************************************
*
*  adcReset - Resets ADC OverFlow
*
*  Functions resetable - state machine, FIFO, APbus & High
*  			 Speed Lines
*
* RETURNS:
* 
*/
int adcReset(register ADC_ID pAdcId)
/* ADC_ID pAdcId;   ADC Object */
{
   if (pAdcId == NULL)
     return(ERROR);
   *ADC_REG(ADC_CR,pAdcId->adcBaseAddr) = 
	  ( pAdcId->adcControl | BIT_MASK(ADC_RSET_OVRFLW) );

    semTake(pAdcId->pAdcMutex,WAIT_FOREVER); /*Mutual Exclusion Semiphore */

    pAdcId->adcState = OK;

    semGive(pAdcId->pAdcMutex);	/* give Mutex back */

   return(OK);
}
/**************************************************************
*
*  adcShiftData - sets bits for data shifting of ADC
*
*
* RETURNS:
* 
*/
int adcShiftData(register ADC_ID pAdcId, int shift)
/* ADC_ID pAdcId;   ADC Object */
/* int    shift;    bits to shift data down */
{
    if ( (shift < 0) || (shift > 15) )
	return(ERROR);
    pAdcId->adcControl &= 0x0f;
    pAdcId->adcControl |= ( (shift << 4) & 0xf0 );
    *ADC_REG(ADC_CR,pAdcId->adcBaseAddr) = pAdcId->adcControl;

    return(OK);
}

/**************************************************************
*
*  adcChanSelect - select Observe of Lock Channel to Digitize
*
*
* RETURNS:
* 
*/
int adcChanSelect(register ADC_ID pAdcId, int select)
/* ADC_ID pAdcId;   ADC Object */
/* int    select;   Channel select 0-Observe, 1-Lock */
{
    if (pAdcId == NULL)
    {
      return(ERROR);
    }
    if ( (select < 0) || (select > 1) )
    {
       LOGMSG1(ALL_PORTS,OK,"adcChanSelect: select %d out of bounds 0-1\n",select);
       return(ERROR);
    }
    pAdcId->adcControl &= ~BIT_MASK(ADC_OBS_LOCK);
    pAdcId->adcControl |= (select & BIT_MASK(ADC_OBS_LOCK));
    *ADC_REG(ADC_CR,pAdcId->adcBaseAddr) = pAdcId->adcControl;

    return(OK);
}

/**************************************************************
*
*   adcShow - display the ADC Object inforamtion
*
*  RETURN
*   void
*/
void adcShow(register ADC_ID pAdcId,int level)
/* ADC_ID pAdcId;   ADC Object */
/* int level       - level of information */
{
   printf("\n\n-------------------------------------------------------------\n\n");
   printf("ADC Object: 0x%lx\n", pAdcId);
   printf("ADC State: %d\n",pAdcId->adcState);
   printf("Board Verion: %d\n",pAdcId->adcBrdVersion);
   printf("ADC: Address-0x%lx, VME Vector-0x%x, VME Level-%d\n",
		pAdcId->adcBaseAddr,pAdcId->vmeItrVector,pAdcId->vmeItrLevel);
   if (level > 0)
   {
     printf("\nADC State Sync Semiphore");
     semShow(pAdcId->pSemAdcStateChg,1);
     printf("\nADC Mutex Semiphore");
     semShow(pAdcId->pAdcMutex,1);
   }
}
