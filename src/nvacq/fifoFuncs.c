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
#include <vxWorks.h>
#include <stdlib.h>
#include <semLib.h>
#include <msgQLib.h>
#include "instrWvDefines.h"
#include "logMsgLib.h"
#include "fifoFuncs.h"
#include "taskPriority.h"
#include "expDoneCodes.h"
#include "errorcodes.h"
#include "nvhardware.h"

#define CNTRL_FIFO_LEVEL -1

#define INST_FIFO_SIZE 1000
/*
modification history
--------------------
6-01-04,gmb  created 
*/

/*

Fifo Object Routines

*/

union ltw {
  unsigned long long ldur;
  unsigned int  idur[2];
  unsigned char cdur[8];
};

/* for Error messages, can append the board type & number */
extern int  BrdType;    /* Type of Board, RF, Master, PFG, DDR, Gradient, Etc. */
extern int  BrdNum;     /* The Board types Ordinal number, i.e. rf1 or rf2 */

extern int failAsserted;
extern int warningAsserted;

static FIFO_REGS *pFifo = NULL;

static unsigned int fifoIntMask = 0;
static unsigned int fifoInstrSize = 0;

// set defaults, but new controllers like LPFG may initialize with setFifoMasks(...)
static unsigned int failMask = FF_FAILURE;
static unsigned int warnMask = FF_WARNING;  
static unsigned int underflowMask = FF_UNDERFLOW;
static unsigned int overflowMask = FF_OVERFLOW;
static unsigned int startedMask = FF_STARTED;
static unsigned int finishedMask = FF_FINISHED;
static unsigned int invalidOpcodeMask = FF_INVALID_OPCODE;
static unsigned int dataAmmtMask = FF_DATA_AMMT;
static unsigned int instrAmfMask = FF_INSTR_AMF;

static SEM_ID pSemFifoStop = NULL;

static void Fifo_BasicIsr(int int_status, int val) ;
MSG_Q_ID pMsgs2FifoIST = NULL;

#ifdef LPFG_DEBUG
#include "pfg_reg.h"
unsigned int fifoOverflowCnt = 0;
unsigned int fifoOverflowLastStat = 0;
unsigned int fifoOverflowDataFifoCount = 0;
unsigned int fifoOverflowInstrFifoCount = 0;
unsigned int fifoOverflowDmaReqCount = 0;
unsigned int fifoOverflowDmaReqValue = 0;

void prtLastFifoOverflow()
{
  printf("FIFO overflow count = %d, Data FIFO count = %d, Instruction FIFO count = %d, DMA req count = %d\n",
	 fifoOverflowCnt, fifoOverflowDataFifoCount, fifoOverflowInstrFifoCount, fifoOverflowDmaReqCount);
  printf("FIFO overflow interrupt status = 0x%x\n", fifoOverflowLastStat);
  printf("FIFO overflow dma request value = 0x%x = %d\n", fifoOverflowDmaReqValue, fifoOverflowDmaReqValue);
}
#endif

void cntrlResetIntStatus()
{
   unsigned int word;
   /* enable these so status can be read */
   word = *pFifo->pFifoIntStatus;
   *pFifo->pFifoIntClear = 0;
   *pFifo->pFifoIntClear = word;
   *pFifo->pFifoIntEnable = fifoIntMask;   /* the 3 basic interrupts  */
   return;
}

giveCntrlFifoStopped()
{
    if (pSemFifoStop != NULL)
       semGive(pSemFifoStop);
}



/***********************************************************
*
* fifoIST - Interrupt Service Task
*
* Actions to be taken:
* 1. Wait for Error Message
* 2. SendException appropraite for Error
*    FIFO Underflow:
*/
static VOID fifoIST()
{
   int error, errorcode, ival;
   int encodeBrd;

   encodeBrd = (BrdType < 8) | (BrdNum & 0xFF);

   FOREVER
   {
      ival = msgQReceive(pMsgs2FifoIST,(char*) &error,sizeof(int),WAIT_FOREVER);
      DPRINT3(+0,"fifoIST: recv'd %d bytes, errorcode: %d, 0x%lx\n",ival,error,error);
      if (ival == ERROR)
      {
         printf("fifoIST MSG Q ERROR\n");
      }
      /* DPRINT(+0,"fifoIST: sendException\n"); */
      if ((error > 99) && (error < 200))
         sendException(WARNING_MSG, error, 0,0,NULL);
      else
         sendException(HARD_ERROR, error, 0,0,NULL);
   }
}

/*-------------------------------------------------------------
| FIFO Object Public Interfaces
+-------------------------------------------------------------*/

/**************************************************************
*
*  cntrlFifoInit - create the Fifo Object Data Structure & Semaphore
*
*
* RETURNS:
* OK - if no error, NULL - if mallocing or semaphore creation failed
*
*/ 

void  cntrlFifoInit(FIFO_REGS *pFifoRegs, unsigned int intMask, int fifoInstSize)
{
   int tid;

   if (pFifoRegs == NULL)
       return;

   fifoInstrSize = fifoInstSize;
   fifoIntMask = intMask;
   pFifo = pFifoRegs;
   printf("reg ptr: 0x%lx, Int mask: 0x%lx, instr fifo size: %d\n",pFifoRegs,intMask,fifoInstSize);

  /* ------- Create the Resource needed by the FIFO -------- */

   DPRINT1(-1,"calling fifoBufCreate: buffer size = %d\n",500);

   pSemFifoStop = semBCreate(SEM_Q_FIFO,SEM_EMPTY);
   pMsgs2FifoIST = msgQCreate(10,sizeof(int),MSG_Q_FIFO);  /* Fifo Errors, e.g FOO */

   if (pSemFifoStop == NULL)
   {
        errLogSysRet(LOGIT,debugInfo,
	   "cntrlFifoInit: Failed to allocate some resource:");
        return;
   }

   /* ----- reset board and get status register */
   cntrlFifoReset();  /* reset board, also diables all interrupts */

   /* ------- Connect VME interrupt vector to proper Semaphore to Give ----- */
   if ( fpgaIntConnect( Fifo_BasicIsr, 0, fifoIntMask) == ERROR)
   {
     errLogSysRet(LOGIT,debugInfo,
	"cntrlFifoInit: Could not connect FIFO itrp: ");
     pFifo = NULL;
     return;
   }

   cntrlResetIntStatus();

   tid = taskSpawn("tFifoIST", FIFO_IST_PRIORTY, FIFO_IST_TASK_OPTIONS,
                   XSTD_STACKSIZE, fifoIST, pMsgs2FifoIST,0,
                   0,0,0,0,0,0,0,0);
   if ( tid == ERROR)
   {   
        errLogSysRet(LOGIT,debugInfo,
           "cntrlFifoInit: could not spawn FIFO IST: ");
   }   

   return;
}

setFifoMasks(unsigned overflow, unsigned underflow, unsigned finished, 
             unsigned started, unsigned failure, unsigned warning, 
             unsigned dataAmmt, unsigned instrAmf, unsigned invalidOpcode)
{
  overflowMask =  overflow;
  underflowMask = underflow;
  finishedMask =  finished;    
  startedMask =   started;     
  failMask =   failure;      
  warnMask =   warning;      
  dataAmmtMask =  dataAmmt;
  instrAmfMask =  instrAmf;     
  invalidOpcodeMask = invalidOpcode;
}

/**************************************************************
*
*  cntrlFifoStart - Start FIFO if not running 
*
*
* RETURNS:
*  OK - If FIFO was Started, 1 - If already running, -1 if Object ID Null
* 
*   Question: if fifoState semaphore needs to flushed when 
*         fifo starts ?
*/

int cntrlFifoStart()
{
    if (pFifo == NULL)
     return(-1);

    DPRINT(2,"fifoStart");
    /* if FIFO already running do not restart */
    if ( (*pFifo->pFifoStatus & 0x1) == 0)
    {
     *pFifo->pFifoControl = 0;
     *pFifo->pFifoControl = 1;

#ifdef INSTRUMENT
     	wvEvent(EVENT_FIFOSTART,NULL,NULL);
#endif
    }
     return(1);
}


/**************************************************************
*
*  cntrlFifoStartOnSync - Start FIFO synchronize with signal selected
*			(lockgate)
*
* RETURNS:
*  OK - If FIFO was Started, 1 - If already running, -1 Object Id Null
* 
*   Question: if fifoState semaphore needs to flushed when 
*         fifo starts ?
*/

int cntrlFifoStartOnSync()
{
   if (pFifo == NULL)
      return(-1);

    /* if FIFO already running do not restart */
    if ( (*pFifo->pFifoStatus & 0x1) == 0)
    {
      *pFifo->pFifoControl = 0;
      *pFifo->pFifoControl = 2;
    }
    return(OK);
}

/**************************************************************
*
*  cntrlFifoStartAndSync - Start FIFO, and switch to StartOnSync
*
*
* RETURNS:
*  OK - If FIFO was Started, 1 - If already running, -1 Object Id Null
* 
*   Question: if fifoState semaphore needs to flushed when 
*         fifo starts ?
*/
int cntrlFifoStartAndSync()
{
   if (pFifo == NULL)
      return -1;

   /* if FIFO already running do not restart */
   if ( (*pFifo->pFifoStatus & 0x1) == 0)
   {
      DPRINT(2,"Starting FIFO\n");
      *pFifo->pFifoControl = 0;
      *pFifo->pFifoControl = 1;
      *pFifo->pFifoControl = 0;   /* not sure if I need this. */
      *pFifo->pFifoControl = 2;
      DPRINT2(2,"FIFO Started [FIFO Cntrl @0x%x, FIFO Status 0x%x]\n",pFifo->pFifoControl,*pFifo->pFifoStatus);
   }
   return OK;
}

/**************************************************************
*
*  cntrlFifoClearStartMode - Clears the FIFO Start Register
*		
*     This prevents false starting of the FIFO, if for
*      example the mode has been left to start on Sync then 
*      any glitch one the sync line like a controller rebooting or
*      the FPGA being reprogrammed can result in the FIFo being started
*      Not Good...
*
*      Should only be called with the system is idle
*
* RETURNS:
*   Nothing 
* 
*/
void cntrlFifoClearStartMode()
{
   *pFifo->pFifoControl = 0;
}

/**************************************************************
*
*  cntrlFifoReset - Resets combinations of FIFO functions 
*
*  Functions resetable - FIFO, High Speed Lines
*
* RETURNS:
* 
*/
void cntrlFifoReset()
{
   /* DPRINT1(-1,"cntrlFifoReset(): pFifo = 0x%lx\n",pFifo); */
   if (pFifo != NULL)
   {
      /* DPRINT1(-1,"cntrlFifoReset(): pFifo->pFifoControl = 0x%lx\n",pFifo->pFifoControl); */
      /* *pFifo->pFifoControl = 0; */
      *pFifo->pFifoControl = 4;
      *pFifo->pFifoControl = 0;
   }
   /* Now Attempt to take Stop semaphore, when it would block that
        is the state we want it in.
     */  
    if (pSemFifoStop != NULL)
       while (semTake(pSemFifoStop,NO_WAIT) != ERROR);

     panelLedOff(FIFO_RUNNING_LED);

#ifdef INSTRUMENT
     	wvEvent(EVENT_FIFORESET,NULL,NULL);
#endif

}

/**************************************************************
*
*  cntrlFifoPut - Puts A Code into FIFO 
*
*   WARNING: Does not pend for FF Almost Full, BEWARE !!
*
* RETURNS:
* 
*/
void cntrlFifoPut(long codes)
/* code - fifo code to be stuffed into FIFO */
{
  if (pFifo == NULL)
      return;

  *pFifo->pFifoWrite = codes;

  return;
}

/**************************************************************
*
*  cntrlFifoPIO - Puts Code into FIFO 
*
*   WARNING: Does not pend for FF Almost Full, BEWARE !!
*   Should only be used in phandler's reset2SafeState()
*
* RETURNS:
* 
*/
void cntrlFifoPIO(long *pCodes, int num)
/* code - fifo code to be stuffed into FIFO */
/* num - Number of fifo codes to be stuffed directly into FIFO */
{
  int numInst, nroom, i;

  if (pFifo == NULL)
      return;

  while(num > 0)
  {
     /* how many in the instruction FIFO */
     numInst =  fifoInstrSize - *pFifo->pFifoInstructionFIFOCount;
     nroom = (num > numInst) ? numInst : num;

     for(i=0; i < nroom; i++)
     {
	  /* DPRINT2(-1,"cntrlFifoPIO(): instruct[%d] = 0x%lx\n",i,*pCodes); */
          *pFifo->pFifoWrite = *pCodes++;
     }
     num -= nroom;
  }

  return;
}

/**************************************************************
*
*  cntrlFifoStatReg - Gets FIFO status register value
*
*
* RETURNS:
*  16-bit FIFO/STM Status Register Value
*/
unsigned int  cntrlFifoStatReg()
{
    if (pFifo == NULL)
      return(-1);

    return (*pFifo->pFifoIntStatus);
}


/**************************************************************
*
*  cntrlFifoIntrpMask - Gets FIFO Interrupt Register mask
*
*
* RETURNS:
*  16-bit FIFO Interrupt Mask Value
*/
unsigned int cntrlFifoIntrpMask()
{
    if (pFifo == NULL)
      return(-1);

    return ( *pFifo->pFifoIntEnable );
}

/**************************************************************
*
*  cntrlFifoIntrpSetMask -Sets FIFO Interrupt Register mask bits
*
*
* RETURNS:
*  0
*/
int cntrlFifoIntrpSetMask(unsigned int maskbits)
{
    if (pFifo == NULL)
      return(-1);

    *pFifo->pFifoIntEnable = *pFifo->pFifoIntEnable | maskbits;
    return (0);
}

/**************************************************************
*
*  cntrlFifoIntrpClearMask - Clears FIFO Interrupt Register mask bits
*
*
* RETURNS:
*  0 
*/
int cntrlFifoIntrpClearMask(unsigned int maskbits)
{
    if (pFifo == NULL)
      return(-1);

    *pFifo->pFifoIntEnable = *pFifo->pFifoIntEnable & (~maskbits);
    return (0);
}

/**************************************************************
*
*  cntrlFifoRunning - Returns True if FIFO is running 
*
*
* RETURNS:
*  TRUE or FALSE
*/
int cntrlFifoRunning()
{
   if (pFifo == NULL)
      return(-1);

   return *pFifo->pFifoStatus & 0x1;
}

/**************************************************************
*
*  cntrlFifoEmpty - Returns True if FIFO is empty 
*
* RETURNS:
*  TRUE or FALSE
*/
int cntrlFifoEmpty()
{
   if (pFifo == NULL)
      return(-1);

   return ( (*pFifo->pFifoDataFIFOCount == 0) );
}

/**************************************************************
*
*  cntrlInstrFifoCount - Returns number of fifo words in Instruction FIFO
*
* RETURNS:
*  TRUE or FALSE
*/
int cntrlInstrFifoCount()
{
   if (pFifo == NULL)
      return(-1);

   return ( *pFifo->pFifoInstructionFIFOCount );
}

/**************************************************************
*
*  cntrlDataFifoCount - Returns number of fifo words in Data FIFO
*
*
* RETURNS:
*  TRUE or FALSE
*/
int cntrlDataFifoCount()
{
   if (pFifo == NULL)
      return(-1);

   return ( *pFifo->pFifoDataFIFOCount );
}

/**************************************************************
*
*  cntrlInvalidOpCode - Returns the invalid opcode caught, 
*                       0xFFFF if no invalid opcode  received.
*
*
* RETURNS:
*  bad opcode or 0xFFFF
*/
int cntrlInvalidOpCode()
{
   if (pFifo == NULL)
      return(-1);

   return ( *pFifo->pFifoInvalidOpCode );
}

/**************************************************************
*
*  cntrlInstrCountTotal - Returns number of instruction fifo words 
*    received.
*
*
* RETURNS:
*  TRUE or FALSE
*/
long cntrlInstrCountTotal()
{
   if (pFifo == NULL)
      return(-1);

   return ( *pFifo->pFifoInstrFIFOCountTotal );
}

/**************************************************************
*
*  cntrlClearInstrCountTotal - clears number of instruction fifo 
*  word register.
*
*
* RETURNS:
*  NONE
*/
int cntrlClearInstrCountTotal()
{
   if (pFifo == NULL)
      return(-1);

     *pFifo->pFifoClearInstrFIFOCountTotal = 1;
     *pFifo->pFifoClearInstrFIFOCountTotal = 0;

     return 0;
}

/**************************************************************
*
*  cntrlFifoWait4Stop - Returns when FIFO is not running
*
*
* RETURNS:
* 
*/
void cntrlFifoWait4Stop()
{
   int dummy,running;

   if (pFifo == NULL)
      return;


#ifdef INSTRUMENT
     wvEvent(EVENT_FIFO_WT4STOP,NULL,NULL);
#endif

  while ( (*pFifo->pFifoIntStatus == 0) && (*pFifo->pFifoDataFIFOCount > 0) )
  {
      taskDelay(calcSysClkTicks(17)); /* 17 ms, a better way of doing nothing */
  }
  return;
}

/**************************************************************
*
*  cntrlFifoBusyWait4Stop - Returns when FIFO is not running
*
*   This routine does not use taskDelay() so that no context switch
*   will occur on the calling task.  Use by phandler.c so it is not 
*   swapped out for another task.
*
* RETURNS:
* 
*/
void cntrlFifoBusyWait4Stop()
{
   if (pFifo == NULL)
      return;

#ifdef INSTRUMENT
     wvEvent(EVENT_FIFO_WT4STOP,NULL,NULL);
#endif

  while ( (*pFifo->pFifoIntStatus == 0) && (*pFifo->pFifoDataFIFOCount > 0) );
  return;

}

/**************************************************************
*
*  fifoWait4StopItrp - Returns when FIFO Stop Interrupt Occurs 
*
*
* RETURNS:
* 
*/
void cntrlFifoWait4StopItrp()
{
   /* wait via interrupt scheme */
#ifdef INSTRUMENT
    wvEvent(EVENT_FIFO_WT4STOPITR,NULL,NULL);
#endif
    if (semTake(pSemFifoStop,(sysClkRateGet() * 1200)) != OK) 
    {
         errLogRet(LOGIT,debugInfo,"fifoWait4Stop: timed out after 20 min");
         return;
    }
}

/*
 *  cntrlFifoCumulativeDurationClear(0 - clear the cumulative duration registers
 */
void cntrlFifoCumulativeDurationClear()
{
      *pFifo->pFifoClrCumDuration = 0;
      *pFifo->pFifoClrCumDuration = 1;
      *pFifo->pFifoClrCumDuration = 0;
}
/*
 * getCntrlFifoCumulativeDuration - return the Cumulative Duration
 */
void cntrlFifoCumulativeDurationGet(long long *dat)
{
  union ltw TEMP;
  TEMP.idur[1] = *pFifo->pFifoCumDurationLow;
  TEMP.idur[0] = *pFifo->pFifoCumDurationHi;
  *dat = (TEMP.ldur);
}

static void Fifo_BasicIsr(int int_status, int val) 
{
  int error;
  int RFI_status = int_status;
  
  if (RFI_status & startedMask)
  {
     panelLedOn(FIFO_RUNNING_LED);
     #ifdef INSTRUMENT
        wvEvent(EVENT_FIFOSTART,NULL,NULL);
     #endif
     if (DebugLevel >= CNTRL_FIFO_LEVEL)
     {
        logMsg(" ===========  Cntrl FIFO - Started  Duration: %lx | %lx\n",
	   *pFifo->pFifoCumDurationHi, *pFifo->pFifoCumDurationLow,3,4,5,6);
        logMsg(" =========  Levels  Instr: %ld,  Data: %ld\n", *pFifo->pFifoInstructionFIFOCount, 
                       *pFifo->pFifoDataFIFOCount);
     }
  }

  if (RFI_status & failMask)
  {
     failAsserted = 1;
     #ifdef INSTRUMENT
       wvEvent(EVENT_FIFO_FAILURE,NULL,NULL);
     #endif
     /* DPRINT(-1,"Fifo_BasicIsr: Failure line\n"); */

     ledOn();

     /* switch to SW controlled Outputs */
     /* this needs to be removed once the new FPGAs are in the build */
     /* setFifoOutputSelect(SELECT_SW_CONTROLLED_OUTPUT); removing 9/9/05 GMB */

     /* if aborted during the MRI read user byte, priorties maybe still high, so reset them. */
     resetParserPriority();   /* use this order to retain priority relationship */
     resetShandlerPriority();

     /* serialize out safe states */
     /* this needs to be removed once the new FPGAs are in the build */
     /* setAbortSerializedStates();  removing 9/9/05 GMB */
     serializeSafeVals();

     if (DebugLevel>= CNTRL_FIFO_LEVEL)
        logMsg(" ===========  Cntrl FIFO - System Fail Line Asserted  Duration: %lx | %lx\n",
	    *pFifo->pFifoCumDurationHi, *pFifo->pFifoCumDurationLow,3,4,5,6);
  }

  if ( (RFI_status & warnMask) && !failAsserted )
  {
     #ifdef INSTRUMENT
        wvEvent(EVENT_FIFO_UNDERFLOW,NULL,NULL);
     #endif
     /* DPRINT(-1,"Fifo_BasicIsr: underflow \n"); */
     error = HDWAREERROR + FIFOERROR;
     msgQSend(pMsgs2FifoIST,(char*) &error, sizeof(int), NO_WAIT, MSG_PRI_NORMAL);
     /* buffer = " Underflow";  */
     if (DebugLevel>= CNTRL_FIFO_LEVEL)
         logMsg(" ===========  Cntrl FIFO - Underflow  Duration: %lx | %lx\n",
         *pFifo->pFifoCumDurationHi, *pFifo->pFifoCumDurationLow,3,4,5,6);
  }

  if ( (RFI_status & overflowMask) && !failAsserted )
  {
   #ifdef LPFG_DEBUG
     fifoOverflowCnt++;
     fifoOverflowLastStat = RFI_status;
     fifoOverflowDataFifoCount = get_field(PFG,data_fifo_count);
     fifoOverflowInstrFifoCount = get_field(PFG,instruction_fifo_count);
     fifoOverflowDmaReqCount = get_field(PFG,dmareq_count);
     fifoOverflowDmaReqValue = get_field(PFG,dmareq_value);
   #endif
     #ifdef INSTRUMENT
       wvEvent(EVENT_FIFO_OVERFLOW,NULL,NULL);
     #endif
     /* DPRINT(-1,"Fifo_BasicIsr: overflow \n"); */
     error = HDWAREERROR + FIFO_OVRFLOW;
     msgQSend(pMsgs2FifoIST,(char*) &error, sizeof(int), NO_WAIT, MSG_PRI_NORMAL);

     if (DebugLevel >= CNTRL_FIFO_LEVEL)
        logMsg(" ===========  Cntrl FIFO - Overflow  Duration: %lx | %lx\n",
        *pFifo->pFifoCumDurationHi, *pFifo->pFifoCumDurationLow,3,4,5,6);
  }

  if ( (RFI_status & invalidOpcodeMask) && !failAsserted )
  {
     int badopcode = *pFifo->pFifoInvalidOpCode;
     #ifdef INSTRUMENT
       wvEvent(EVENT_FIFO_OVERFLOW,NULL,NULL);
     #endif
     /* DPRINT(-1,"Fifo_BasicIsr: overflow \n"); */
     error = HDWAREERROR + FIFO_INVALID_OPCODE;

     /* for gradient controller badopcode is also used to detect durations of less then 4 usec */
     if ((BrdType == GRAD_BRD_TYPE_ID) && ((badopcode == 30) || (badopcode == 31)) )
     { 
       /* case 30  gradient duration count > 0 but < 320 (4 usec) */ 
       /* case 31  gradient repeat duration count > 0 but < 320 (4 usec) */
       error = HDWAREERROR + GRADIENT_AMP_OVRRUN;
     }

     msgQSend(pMsgs2FifoIST,(char*) &error, sizeof(int), NO_WAIT, MSG_PRI_NORMAL);
     if (DebugLevel >= CNTRL_FIFO_LEVEL)
        logMsg(" ===========  Cntrl FIFO - Invalid OpCode: %d  Duration: %lx | %lx\n",
        badopcode, *pFifo->pFifoCumDurationHi, *pFifo->pFifoCumDurationLow,4,5,6);
  }


  if (RFI_status & dataAmmtMask)  
  {
     #ifdef INSTRUMENT
        wvEvent(EVENT_FIFO_AMMT,NULL,NULL);
     #endif
     /* This is level interrupts thus as long as the FIFO is AMFULL it will */
     /* keep interrupting , disable interrupt or it might just keep interrupting */
     *(pFifo->pFifoIntEnable) = ( *(pFifo->pFifoIntEnable) & (~(dataAmmtMask)) );
     if (DebugLevel > CNTRL_FIFO_LEVEL)
     {
         logMsg(" ===========  Cntrl FIFO - AMMT  Duration: %lx | %lx\n",
                *pFifo->pFifoCumDurationHi, *pFifo->pFifoCumDurationLow,3,4,5,6);
          logMsg(" =========  Levels  Instr: %ld,  Data: %ld\n", *pFifo->pFifoInstructionFIFOCount, 
                       *pFifo->pFifoDataFIFOCount);
     }
  }

  if (RFI_status & instrAmfMask)  
  {
     #ifdef INSTRUMENT
        wvEvent(EVENT_FIFO_AMFULL,NULL,NULL);
     #endif
     /* This is level interrupts thus as long as the FIFO is Almost Empty it will */
     /* keep interrupting , disable interrupt or it might just keep interrupting */
     /* and hang the system */
     *(pFifo->pFifoIntEnable) = ( *(pFifo->pFifoIntEnable) & (~(instrAmfMask)) );
     if (DebugLevel > CNTRL_FIFO_LEVEL)
     {
         logMsg(" ===========  Cntrl FIFO - AMFULL  Duration: %lx | %lx\n",
                *pFifo->pFifoCumDurationHi, *pFifo->pFifoCumDurationLow,3,4,5,6);
         logMsg(" =========  Levels  Instr: %ld,  Data: %ld\n", *pFifo->pFifoInstructionFIFOCount, 
                       *pFifo->pFifoDataFIFOCount);
     }
  }

  if (RFI_status & finishedMask)   
  { 
     #ifdef INSTRUMENT
        wvEvent(EVENT_FIFO_STOP,NULL,NULL);
     #endif
     if (DebugLevel >= CNTRL_FIFO_LEVEL)
         logMsg(" ===========  Cntrl FIFO - Stopped  Duration: %lx | %lx\n",
        *pFifo->pFifoCumDurationHi, *pFifo->pFifoCumDurationLow,3,4,5,6);

  }

  if (RFI_status & warnMask)
  {
     #ifdef INSTRUMENT
        wvEvent(EVENT_FIFO_WARNING,NULL,NULL);
     #endif

     warningAsserted = 1;

     if (DebugLevel > CNTRL_FIFO_LEVEL)
         logMsg(" ===========  Cntrl FIFO - System Warning Line Asserted, No Action Taken. Duration: %lx | %lx\n",
          *pFifo->pFifoCumDurationHi, *pFifo->pFifoCumDurationLow,3,4,5,6);
  }

  if ( (RFI_status & failMask) || (RFI_status & underflowMask) || 
       (RFI_status & overflowMask) || (RFI_status & finishedMask) || 
       (RFI_status & invalidOpcodeMask ))
  {
     panelLedOff(FIFO_RUNNING_LED);
     giveCntrlFifoStopped();   /* failure, stop fifo but the finish interrupt is not asserted */
  }

}

fifoRegShow()
{

   printf(" ------------  Fifo Register Addresses  ---------  \n");
   printf(" FIFO Write   = 0x%lx\n",pFifo->pFifoWrite);
   printf(" FIFO Control = 0x%lx, Present Value = 0x%lx\n",pFifo->pFifoControl,*pFifo->pFifoControl);
   printf(" FIFO Status  = 0x%lx, Present Value = 0x%lx\n",pFifo->pFifoStatus,*pFifo->pFifoStatus);
   printf(" FIFO Instruction Count = 0x%lx, Present Value = 0x%lx\n\n",
	    pFifo->pFifoInstructionFIFOCount,*pFifo->pFifoInstructionFIFOCount);
   printf(" FIFO Cum High= 0x%lx, Present Value = 0x%lx\n",pFifo->pFifoCumDurationHi,*pFifo->pFifoCumDurationHi);
   printf(" FIFO Cum Low = 0x%lx, Present Value = 0x%lx\n",pFifo->pFifoCumDurationLow,*pFifo->pFifoCumDurationLow);
   printf(" FIFO Clear Cum Count   = 0x%lx\n\n",pFifo->pFifoClrCumDuration);
   printf(" Interrupt Status = 0x%lx, Present Value = 0x%lx\n",pFifo->pFifoIntStatus,*pFifo->pFifoIntStatus);
   printf(" Interrupt Enable = 0x%lx, Present Value = 0x%lx\n",pFifo->pFifoIntEnable,*pFifo->pFifoIntEnable);
   printf(" Interrupt Clear = 0x%lx, Present Value = 0x%lx\n",pFifo->pFifoIntClear,*pFifo->pFifoIntClear);
   printf("\n\n");
   return 0;
}

prtfifostat(int level)
{
   int dmaReqsQd,free2Q;
   int dmaChannel;
   long long duration;
   char *strptr;

    int freeSGList,freeTxDesc;

   dmaChannel = cntrlFifoDmaChanGet();

   dmaReqsQd = dmaReqsInQueue(dmaChannel);

   free2Q = dmaReqsFreeToQueue(dmaChannel);

   freeTxDesc = dmaNumFreeTxDescs();
   freeSGList = dmaNumFreeSGListNodes();
   
   if (*pFifo->pFifoControl == 0)
      strptr = "Off";
   else if (*pFifo->pFifoControl == 1)
      strptr = "On";
   else if (*pFifo->pFifoControl == 2)
      strptr = "On with ISync";
   else if (*pFifo->pFifoControl == 3)
      strptr = "On & On with ISync";
   else
      strptr = "Undefined";

   printf("FIFO StartMode: '%s', %d\n",strptr,*pFifo->pFifoControl);
   printf("FIFO Running: '%s', %d\n",((cntrlFifoRunning() == 1) ? "YES" : "NO"),cntrlFifoRunning());
   printf("FIFO: instruct count: %d, data count: %ld\n",
		cntrlInstrFifoCount(),cntrlDataFifoCount());
   cntrlFifoCumulativeDurationGet(&duration);
   printf("FIFO: Duration: %llu (ticks) or %lf us \n",duration, (((double) duration) * .0125));
   printf("FIFO: DMA device paced pended: %d\n",dmaGetDevicePacingStatus(dmaChannel));
   printf("FIFO: DMA Request Queued: %d, Remaining Queue Space: %d\n",dmaReqsQd, free2Q);
   printf("FIFO: DMA Transfer Desc Free: %d\n",freeTxDesc);
   printf("FIFO: DMA Scatter Gather Nodes Free: %d\n",freeSGList);
   return(0);
}
