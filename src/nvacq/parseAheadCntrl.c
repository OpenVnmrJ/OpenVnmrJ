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
#include <semLib.h>
#include <stdlib.h>
#include "logMsgLib.h"
#include "instrWvDefines.h"


/* number of scans (CTs) to parse ahead prior to blocking */
static parseAheadCount = 10;
static parseAheadCntDown = 10;


/* postion within the parseAheadCntDown that an itr should placed into the FIFO */
static parseAheadItrLoc  = 5;
static parseAheadItrCntDwn = 5;

static SEM_ID parserStopSem = NULL;

/**************************************************************
*
*  parseAheadInit - create the A interpeter Parse Ahead Control
*
* RETURNS:
* OK - if no error, NULL - if mallocing or semaphore creation failed
*
*/ 

int  parseAheadInit(int parseCnt, int itrCnt)
{

  if (parserStopSem == NULL)
  {
     /* parserStopSem = semCCreate(SEM_Q_FIFO,SEM_EMPTY); */
     parserStopSem = semBCreate(SEM_Q_FIFO,SEM_EMPTY);
     if ( (parserStopSem == NULL) )
     {
        errLogSysRet(LOGIT,debugInfo,
	   "parseAheadInit: Failed to allocate parserStopSem Semaphore:");
        return(ERROR);
      }
  }

   parseAheadCntDown = parseAheadCount = parseCnt;
   parseAheadItrCntDwn = parseAheadItrLoc = itrCnt;
   
   return(OK);
}

/**************************************************************
*
*  cntrlStatesDelete - Deletes Controller States Object and  all resources
*
*
* RETURNS:
*  OK or ERROR
*
*	Author Greg Brissey 8/9/2004
*/
int parseAheadDelete()
{

   if (parserStopSem != NULL)
   {
         semDelete(parserStopSem);
   }
}


parseAheadSemReset()
{

   if (parserStopSem != NULL)
   {
       #ifdef INSTRUMENT
              wvEvent(EVENT_PARSEAHEAD_SEMRESET,NULL,NULL);
       #endif
      /* give semi thus unblocking stuffing */
      semGive(parserStopSem);

      /* Now Attempt to take it when, when it would block that     */
      /*   is the state we want it in.                             */
      while (semTake(parserStopSem,NO_WAIT) != ERROR);
   }
   parseAheadCntDown = parseAheadCount;

}




/*************************************************************
*  parseCntDownReset - reset count down and give semaphore to relases parser
*    call from an ISR
*
*			Author Greg Brissey
*
*/
int  parseCntDownReset()
{
   int errorcode,type,num;

   if (parserStopSem == NULL)
     return(-1);

   #ifdef INSTRUMENT
     wvEvent(EVENT_PARSEAHEAD_CNTRESET,NULL,NULL);
   #endif

 /* --    DPRINT1(-1,"parseCntDownReset: reset count to: %d\n",parseAheadCount); */
 /* --    parseAheadCntDown = parseAheadCount; */

    DPRINT2(-1,"parseCntDownReset: add parseAheadItrLoc (%d) to count: %d\n",parseAheadItrLoc,parseAheadCount);

    parseAheadCntDown += parseAheadItrLoc;
    
    #ifdef INSTRUMENT
        wvEvent(EVENT_PARSEAHEAD_SEMGIVE2FREE,NULL,NULL);
    #endif
    semGive(parserStopSem);

    /* semFlush(parserStopSem);  right now only one task pends */

   return(OK);
}


int parseAheadDecrement(int timeout)
{
  int instrwords[20];
  int len,total;
  int state;

  if (parserStopSem == NULL)
     return(-1);

/*
 * #ifdef INSTRUMENT
 *    wvEvent(7000 + parseAheadCntDown,NULL,NULL);
 * #endif
*/
   parseAheadCntDown--;
   parseAheadItrCntDwn--;
   DPRINT2(-1,"parseAheadCntDown: %d, parseAheadItrLoc: %d\n",parseAheadCntDown,parseAheadItrLoc);

   /* if at the interrupt location in the parse count down then put an SW 3 Itr in FIFO */
   /* if ( parseAheadCntDown == parseAheadItrLoc) */
   DPRINT1(-1,"parseAheadItrCntDwn: %d\n",parseAheadItrCntDwn);
   if ( parseAheadItrCntDwn == 0)
   {
       #ifdef INSTRUMENT
             wvEvent(EVENT_PARSEAHEAD_ITR_INSERT,NULL,NULL);
       #endif
      DPRINT(-1,"Time to put SW 3 ITR into FIFO\n");
      /* place SW 3 Itr into FIFO */
      len = fifoEncodeGates(0,0x400,0x400,instrwords);   /* SW Itr 3 */

      /* fifoEncodeSWItr place 100 nsec delay when in the FIFO, this was observed to cause the
         DDR and RF channels to occasional skew 100 nsec from each other, jumping back and forth
         so now we include the interrupt into the next fifo word (what ever that is) thus no
         addition delay in introduced.  This seems to fix this jitter,  3/9/05  GMB */
      /* len = fifoEncodeSWItr(3, instrwords); */
      writeCntrlFifoBuf(instrwords,len);
      parseAheadItrCntDwn = parseAheadItrLoc;
   }
   else
   {
      /* put in a dummy 100nsec so all timing are constent */
      /* len = fifoEncodeDuration(1, 8,instrwords); */
      /* see above, just making sure at some point we cleat this SW itr gate */
      len = fifoEncodeGates(0,0x400,0x000,instrwords);   /* unset SW Itr 3 */
      writeCntrlFifoBuf(instrwords,len);
   }

   if( parseAheadCntDown <= 0)
   {
       #ifdef INSTRUMENT
              wvEvent(EVENT_PARSEAHEAD_WAIT4ITR,NULL,NULL);
       #endif
       flushCntrlFifoRemainingWords();  /* make sure fifo words get into FIFO */
       if (! cntrlFifoRunning())
       {
	   DPRINT(-1,"---->>> parseAheadDecrement: Start FIFO\n");
           taskDelay(calcSysClkTicks(166));  /* 166 ms, taskDelay(10); */
           startFifo4Exp();  /* startCntrlFifo(); */
       }
       DPRINT(-1,"Time to stop parser\n");
       switch(timeout)
       {
        case NO_WAIT:
	     state = semTake(parserStopSem, NO_WAIT);  
	     break;

        case WAIT_FOREVER: /* block if state has not changed */
             while( parseAheadCntDown <= 0  )
             {
                #ifdef INSTRUMENT
                    wvEvent(EVENT_PARSEAHEAD_SEMTAKE2SUSPEND,NULL,NULL);
                #endif
	       state = semTake(parserStopSem, WAIT_FOREVER);  
             }
	     break;

        default:     /* block if state has not changed, until timeout */
             state = 0;
             while( parseAheadCntDown <= 0 )
             {
               #ifdef INSTRUMENT
                    wvEvent(EVENT_PARSEAHEAD_SEMTAKE2SUSPEND,NULL,NULL);
               #endif
               if ( (state = semTake(parserStopSem, (sysClkRateGet() * timeout) ) ) != OK )
	       {
	            state = ERROR;		/* timed out */
	            break;
	       }
             }
             if (state != ERROR)
	        state = 1;
             break;
       }
   }
   return(state);
}
