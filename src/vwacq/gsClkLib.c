/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* gsClkLib.c  11.1 07/09/07 - Green Springs DIGITAL-48 Clock Object Source */
/* 
 */

#include <vxWorks.h>
#include <iv.h>
#include <intLib.h>
#include <semLib.h>
#include "commondefs.h"
#include "gsClkLib.h"

/*
modification history
--------------------
9-2-93,gmb  created 
*/


/*
DESCRIPTION

This library contain the Green Spring DIGIT-48  timer facility. 
There are 4 timer clocks available each interrupting on a seperate
VME level ( 1-4 ).
 The main usage of this facility is to create 1 or more interval VME interrupts.
A secondary usage is a single call that will estimate the VME interrupt
latency of the system. 

Note that the resolution of the timers are 4usec.

AN example of usage follows:

 CLOCK_ID ClkId;

 pClkIstSem = semBCreate(SEM_Q_FIFO,SEM_EMPTY);  - clkHandler waits on this sem
 
 -- Spawn the vme Interrupt Service TASK (not ISR)  ---
  taskSpawn("tClkHdlr",LOG_PRIORTY,LOG_TASK_OPTIONS,
		LOG_STACK_SIZE,clkHandler,ARG1,ARG2,
		ARG3,ARG4,ARG5,ARG6,ARG7,ARG8,ARG9,ARG10);

 ClkId = gsClkCreate( 0x60, semGive, pClkIstSem );

 gsClkStart( ClkId, (unsigned long) (500000L / 4L) );

 taskDelay(500);

 gsClkStop(ClkId);
 gsClkDelete(ClkId);

The above creates a binary semaphore that the interrupt service task (clkHandler)
waits on. The task is spawned, and the interrupt clock started. Interrupting
at interval of .5 seconds (125000 ticks @ 4usec/tick = .5 sec)
The main task delays then stops and Deletes the clock.

The call to gsClkItrLatency(INTERNAL_CLK) will generate about 99 interrupts and
calculate the average VME interrupt latency (4usec resolution).

The call to gsClkItrLatency(EXTERNAL_CLK) will generate about 99 interrupts and
calculate the average VME interrupt latency. (max of 1usec resolution possible)

*/

static  int allocClks[] = { 0, 0, 0, 0, 0 };

/* ------- for interrupt latency calcs ----*/
static unsigned long intcnt = 0;
static int avgcnt = 0;
static int numitr = 0;

/********************************************************
*
* clkBaseOffset - calc base offset to a clock
*
* RETURNS:
*  Base Offset Address for the clock
*/
static unsigned long
clkBaseOffset(int clk)
{
   int clk_chip = CLK_1_OFFSET;
   int ip_pack = IP_A_OFFSET;
   switch(clk)
   {
     case 1:
	break;

     case 2:
	clk_chip = CLK_2_OFFSET;
	break;
     case 3:
	ip_pack = IP_B_OFFSET;
	break;
     case 4:
	clk_chip = CLK_2_OFFSET;
	ip_pack = IP_B_OFFSET;
	break;

     default:
	break;
   }

   return( ADD_OFFSET(clk_chip,ip_pack) );
}

/*******************************************
*
* gsClkReset - Resets given clock
*
* RETURNS:
*    void
*
* NOMANUAL
*/
void gsClkReset(CLOCK_ID pClkId)
/* pClkId - clock to reset */
{
   register unsigned char resetval = 0;

   *CLK_REG(TCR_OFFSET,pClkId->clkBaseAddr) = resetval;
}

/*******************************************
*
* gsClkResetZDS - Resets given clock ZDS, and gives 
*		  the VME interrupt acknowledge
*
* RETURNS:
*   void
*
* NOMANUAL
*/
void gsClkResetZDS(CLOCK_ID pClkId)
/* clk - clock ZDS VME interrupt to acknowledge */
{
   *CLK_REG(TSR_OFFSET,pClkId->clkBaseAddr) = (char) 1;
}

/*******************************************
*
* clkISR - Clock interrupt service routine
*
* RETURNS:
*  void
*
* NOMANUAL
*/
void clkISR(register CLOCK_ID pClkId)
{
   *CLK_REG(TSR_OFFSET,pClkId->clkBaseAddr) = (unsigned char) 1;
   if (pClkId->pIntrHandler != NULL)
     (*pClkId->pIntrHandler)(pClkId->UserArg);
}

/*******************************************
*
* clkLatISR - Clock Interrupt Latency interrupt 
*	      service routine
*
* RETURNS:
*  void
*
* NOMANUAL
*/
void clkLatISR(register CLOCK_ID pClkId)
{
    unsigned long readCount();
    unsigned char stop = 0;

   *CLK_REG(TCR_OFFSET,pClkId->clkBaseAddr) = stop;
   *CLK_REG(TSR_OFFSET,pClkId->clkBaseAddr) = (unsigned char) 1;
    intcnt = readCount(pClkId);
    avgcnt += (0xffffff - intcnt);
    numitr++;
    logMsg("Itrp Latency: %ld\n", (0xffffff - intcnt) * pClkId->clkRes);
   *CLK_REG(TCR_OFFSET,pClkId->clkBaseAddr) = pClkId->TcrVal | 0x1;
}



/*******************************************
*
* gsClkCreate - Create interval VME interrupt clock
*
*
* RETURNS:
*  CLOCK_ID or NULL, if ERROR
* 
*/
CLOCK_ID gsClkCreate(int vector,FUNCPTR pUserFunc, int userArg)
/* vector - interrupt vector number (0x60 - 0xff) */
/* pUserFunc - User Function Call from Interrupt Service Routine */
/* userArg - an Argument to the User FUnction */
{
   void clkISR();
   register CLOCK_ID pClkObj;
   int i,clknum,vec;

   /* search for unused clock */
   /* reset all clocks to configured */
   for(i=1; i <= MAX_CLOCKS; i++)
   {
      if (allocClks[i] == 0 )
      {
	clknum = i;
	break;
      }
   }
   if (clknum == 0)
     return(NULL);


   /* create clock object structure */
   pClkObj = (CLOCK_ID) malloc(sizeof(CLOCK_OBJ));
   if ( pClkObj == NULL)
      return (NULL);

   allocClks[clknum]=clknum;

   if (pUserFunc == NULL)
     pClkObj->pIntrHandler = NULL;
   else
     pClkObj->pIntrHandler = pUserFunc;

   pClkObj->clkBaseAddr = (char *)clkBaseOffset(clknum);
   pClkObj->TcrVal = INTRP_RELOAD_INTCLK;
   pClkObj->clkNum = clknum;
   pClkObj->UserArg = userArg;
   pClkObj->vmeItrVector = vector;
   pClkObj->vmeItrLevel = clknum;
   pClkObj->clkRes = 4; 	 /* 4usec resolution */;

   gsClkReset(pClkObj);
   gsClkResetZDS(pClkObj);

   /* program vector */
   *CLK_REG(TIVR_OFFSET,pClkObj->clkBaseAddr) = pClkObj->vmeItrVector;
   vec = *CLK_REG(TIVR_OFFSET,pClkObj->clkBaseAddr);
   printf("VME Itr vector: 0x%x\n",(0xff & vec));

   intConnect( (VOIDFUNCPTR *) INUM_TO_IVEC ( pClkObj->vmeItrVector ),
		(VOIDFUNCPTR) clkISR,(int) pClkObj);

   sysIntEnable(pClkObj->vmeItrLevel);
 
   return(pClkObj);
}

/*******************************************
*
* gsClkStart - Start interval VME interrupt clock
*
* RETURNS:
*   OK
*
*/
int gsClkStart(register CLOCK_ID pClkId, unsigned long count)
/* CLOCK_ID pClkId - Clock to Start */
/* count - 	     Number of clock ticks  */
{
    unsigned long cnt;
    int stat;

    unsigned long readPlCount();
    void setPlCount();


    /* program count */
    setPlCount(pClkId, count);
    cnt = readPlCount(pClkId);
    printf("Clock Count: %ld, 0x%lx\n",cnt,cnt);

    *CLK_REG(TCR_OFFSET,pClkId->clkBaseAddr) = pClkId->TcrVal;
    stat = *CLK_REG(TCR_OFFSET,pClkId->clkBaseAddr);
    printf("Clk Control register: 0x%x \n",stat);

    /* start Periodic Interrupt Generator */
    *CLK_REG(TCR_OFFSET,pClkId->clkBaseAddr) = (char) (pClkId->TcrVal | 0x1);

    return(OK);
}

/*******************************************
*
* gsClkStop - Stop interval VME interrupt clock
*
*
* RETURNS:
*   OK
*/
int gsClkStop(CLOCK_ID pClkId)
/* CLOCK_ID pClkId - Clock to stop */
{
   *CLK_REG(TCR_OFFSET,pClkId->clkBaseAddr) = pClkId->TcrVal;
}

/*******************************************
*
* gsClkDelete - Deletes interval VME interrupt clock 
*
*
* RETURNS:
*   OK
*/
int gsClkDelete(CLOCK_ID pClkId)
/* CLOCK_ID pClkId - Clock to Delete */
{
   
    gsClkReset(pClkId);
    gsClkResetZDS(pClkId);
    allocClks[pClkId->clkNum]=0;
    sysIntDisable(pClkId->vmeItrLevel);
    free(pClkId);
    return(OK);
}

/*******************************************
*
* gsClkItrLatency - Determines an average VME interrupt Latency
*
* RETURNS:
*   OK
*
*/
int gsClkItrLatency(int clock)
/* clock - Clock to use as input, Internal or External */
{
   void clkLatISR();
   register CLOCK_ID pClkObj;
   int i,clknum,vec;

   intcnt = 0;
   avgcnt = 0;
   numitr = 0;

   /* create clock object structure */
   pClkObj = (CLOCK_ID) malloc(sizeof(CLOCK_OBJ));
   if ( pClkObj == NULL)
      return (NULL);

   clknum = 1;
   pClkObj->pIntrHandler = NULL;
   pClkObj->clkBaseAddr = (char *) clkBaseOffset(clknum);
   pClkObj->clkNum = clknum;
   pClkObj->UserArg = NULL;
   pClkObj->vmeItrVector = 0x60;
   pClkObj->vmeItrLevel = clknum;

   if (clock == INTERNAL_CLK)
   {
      pClkObj->TcrVal = INTRP_ROLLOVR_INTCLK;
      pClkObj->clkRes = 4;	 /* 4usec resolution */;
   }
   else
   {
      pClkObj->TcrVal = INTRP_ROLLOVR_EXTCLK;
      pClkObj->clkRes = 1;	 /* 1usec resolution */;
   }

   gsClkReset(pClkObj);
   gsClkResetZDS(pClkObj);

   /* program vector */
   *CLK_REG(TIVR_OFFSET,pClkObj->clkBaseAddr) = pClkObj->vmeItrVector;
   vec = *CLK_REG(TIVR_OFFSET,pClkObj->clkBaseAddr);
   printf("VME Itr vector: 0x%x\n",(0xff & vec));

   intConnect( (VOIDFUNCPTR *) INUM_TO_IVEC ( pClkObj->vmeItrVector ),
		(VOIDFUNCPTR) clkLatISR,(int) pClkObj);

   sysIntEnable(pClkObj->vmeItrLevel);

   gsClkStart(pClkObj, (unsigned long) (100000L / ((long) pClkObj->clkRes)));

   taskDelay(600);

   gsClkStop(pClkObj);

   printf("Total Intrps: %d, Avg Itrp Latency: %d\n",numitr,
	(avgcnt/numitr) * pClkObj->clkRes);

   gsClkDelete(pClkObj);

   return(OK);
}

/*******************************************
*
* setPlCount - Load Precounter registers of clock
*
* NOMANUAL
*/
void setPlCount(register CLOCK_ID pClkId, register unsigned long cnt)
/* CLOCK_ID pClkId - Clock to set preload counter */
/* cnt -	     number of clock ticks to load */
{
    *CLK_REG(CPRH_OFFSET,pClkId->clkBaseAddr) = (short) (cnt >> 16);
    *CLK_REG(CPRM_OFFSET,pClkId->clkBaseAddr) = (short) (cnt >> 8);
    *CLK_REG(CPRL_OFFSET,pClkId->clkBaseAddr) = (short) cnt;
}

/*******************************************
*
* readPlCount - Reads the value of the Precounter register
*
* RETURNS:
*  unsigned long count 
*
* NOMANUAL
*/
unsigned long readPlCount(register CLOCK_ID pClkId)
/* CLOCK_ID pClkId - Clock to sread preload counter */
{
   register unsigned long cnt;

    cnt = ( *CLK_REG(CPRH_OFFSET,pClkId->clkBaseAddr) ) << 16;
    cnt += ( *CLK_REG(CPRM_OFFSET,pClkId->clkBaseAddr) ) << 8;
    cnt += *CLK_REG(CPRL_OFFSET,pClkId->clkBaseAddr);
    return(cnt);
}

/*******************************************
*
* readCount - Reads the Counting register of the clock
*
* Clock must be stopped to obtain a reliable value
*
* RETURNS:
*  unsigned long count of countdown counter
*
* NOMANUAL
*/
unsigned long readCount(register CLOCK_ID pClkId)
/* CLOCK_ID pClkId - Clock to read countdown counter */
{
   register unsigned long cnt;

   cnt = ( *CLK_REG(CRH_OFFSET,pClkId->clkBaseAddr) ) << 16;
   cnt += ( *CLK_REG(CRM_OFFSET,pClkId->clkBaseAddr) ) << 8;
   cnt += *CLK_REG(CRL_OFFSET,pClkId->clkBaseAddr);
   return(cnt);
}
