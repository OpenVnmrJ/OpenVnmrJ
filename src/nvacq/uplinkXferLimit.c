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

  This Task handles the FID Data UpLoad to the Host computer.

*/

/* #define NOT_THREADED */

#define THREADED_RECVPROC

#ifndef ALLREADY_POSIX
 #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#endif
#include <vxWorks.h>
#include <stdioLib.h>
#include <semLib.h>
#include <memLib.h>
#include <msgQLib.h>

#include "nvhardware.h"
#include "logMsgLib.h"

extern char hostName[80];

typedef struct _auxtimerep_ {
 union
  {
    long long lltime;
    int  topnbot[2];
  }  auxtime;
} AuxTimeTicks;

typedef struct _xferobj_ {
    int xferDownCount;
    int maxXferCount;
    unsigned long xferCount;   /* total number of transfers */
    unsigned long rcvrAtCount;  /* where recvproc is at */
    AuxTimeTicks startTime;
    double durMax;
    double durMin;
    double durAvg;
    int numStops;
    SEM_ID pFidXferSem;
} TransferObj;

TransferObj TheTransLimitObj;
static TransferObj *pXferObj = &TheTransLimitObj;

/*
 * this is how this works.
 * 1. This contruct is to schedule (i.e. limit) the number of FIDs published prior to a message 
 *     from Recvproc to continue
 *  
 * This construct is to prevent one DDR in a multi-DDR system from getting far ahead of the others.
 * The observed symptom is the one DDR gets far ahead of the others, for recvproc to process 
 * the FID and return the resources.
 * All data for FIDs 'X' must have been received from all the DDRs.   
 * However in the case where one starves out the others
 * then all resources can be used, putting the system into a dead lock.  
 *
 * this technique was tried on in NDDSThroughputTestPacket_Schedpublisher.c test code.
 *
 *                                      Author Greg Brissey  1/08/07
 *
 */

/*
 * Create Transfer Limit, make semaphore and initial values
 *
 * Author Greg Brissey  1/08/07
 *
 *    called only once....
 */
initTransferLimit(int uploadcnt)
{
   pXferObj = &TheTransLimitObj;
   pXferObj->xferDownCount = uploadcnt;
   pXferObj->xferCount = 0L;   /* total number of transfers */
   pXferObj->rcvrAtCount = 0L;   /* where recvproc is at */
   pXferObj->startTime.auxtime.lltime = 0LL;
   pXferObj->durMax = 0.0;
   pXferObj->durMin = 999999999.0;
   pXferObj->durAvg = 0.0;
   pXferObj->numStops = 0;
   /* if (pXferObj->pFidXferSem == NULL) */
   pXferObj->pFidXferSem = semBCreate(SEM_Q_FIFO,SEM_EMPTY);
}

/*
 *  Reset the counters and seamphore to their initial state
 *
 *                                      Author Greg Brissey  1/08/07 
 *
 */
resetTransferLimit()
{
   pXferObj->startTime.auxtime.lltime = 0LL;
   pXferObj->durMax = 0.0;
   pXferObj->durMin = 999999999.0;
   pXferObj->durAvg = 0.0;
   pXferObj->numStops = 0;
   pXferObj->xferCount = 0L;   /* total number of transfers */
   pXferObj->rcvrAtCount = 0L;   /* where recvproc is at */
   while (semTake( pXferObj->pFidXferSem,NO_WAIT) != ERROR);
}

/*
 *  set the transfer limit, and give the semaphore in case it's being waited one 
 *
 */
/*
* setTransferLimit(int uploadcnt)
* {
   * pXferObj->xferDownCount = uploadcnt;
   * semGive(pXferObj->pFidXferSem);
* }
*/

/*
 * set the number of transfers (FIDs) maximum
 *
 */
setMaxTransferLimit(int maxXferCnt)
{
    pXferObj->maxXferCount = maxXferCnt;
    pXferObj->xferDownCount = maxXferCnt;
    DPRINT1(-1,"setMaxTransferLimit: %d\n", maxXferCnt);
}

/*
 * Recvproc, sends the transfer interval it is at.
 * If everybody keeps up, then the uplink of data never Stops
 *
 */
setTransferIntervalAt(long intervalCnt)
{
   int delta;
   pXferObj->rcvrAtCount = intervalCnt;   /* where Recvproc is with data */
   /* Based on where Recvproc is and where the DDR is, then calc how many
    * transfer are possbile without exceeded the total maximum 
    */
   /* since these are unsigned check if a negative would result from the math */
    if (pXferObj->rcvrAtCount <= pXferObj->xferCount)
    {
        delta = pXferObj->xferCount - pXferObj->rcvrAtCount;
        pXferObj->xferDownCount = pXferObj->maxXferCount - delta;
    }
    else
        pXferObj->xferDownCount = pXferObj->maxXferCount;

   DPRINT4(-1,"increXferIntvl- AtCnt: %lu , XferCnt: %lu, delta: %d, XferCntDwn: %d\n", 
      pXferObj->rcvrAtCount, pXferObj->xferCount, delta, pXferObj->xferDownCount);
   semGive(pXferObj->pFidXferSem);
}


/*
 *
 * routine to free the any routine waiting on the treansferlimit semaphore
 *
 */
releaseTransferLimitWait()
{
   pXferObj->xferDownCount = 4;  /* can't be one, so any greater than one will garrentee and pend on the limit
                                    will be freed. */
   semGive(pXferObj->pFidXferSem);
   DPRINT1(-1,"releaseTransferLimitWait: %d, & giveSem.\n", pXferObj->xferDownCount);
}


setTransferLimit(int uploadcnt)
{
   pXferObj->xferDownCount = uploadcnt;
   semGive(pXferObj->pFidXferSem);
}


/*
 * for Errors or User aborts, reset and give the semaphore incase the upLink is pending on this
 *
 */
abortTransferLimitWait()
{
   pXferObj->xferDownCount = -999;
   semGive(pXferObj->pFidXferSem);
}

/*
 * transferLimit() 
 * This is call by the FID uplink task that is to be stopped.
 * It is setup so that if the xferDownCount reaches zero then the uplink is halted
 * by acquiring the semaphore which will pend. 
 * When the setTransferIntervalAt() is invoked if the countdown is non-zero and the semaphore 
 * is given the Task will be released to continue.
 *
 *                                      Author Greg Brissey  1/08/07
 *
 */
int transferLimitWait()
{
    extern int sysTimerClkFreq;
    int result;

    pXferObj->xferDownCount--;
    pXferObj->xferCount++;
    DPRINT1(-1,"transferLimitWait() - xferDownCount:  %d     ===========================\n", pXferObj->xferDownCount);
    /* printf("Sequence#: %d, pubStopCntDwn: %d\n",packetSentCount, pubStopCntDwn); */

    while ( ( pXferObj->xferDownCount <= 0) && ( pXferObj->xferDownCount > -999 ) )
    {
        DPRINT(-1,"dataPublisher() - STOP: FID Xfer  <<<<<<< ===============================================\n");
        vxTimeBaseGet(&pXferObj->startTime.auxtime.topnbot[0], &pXferObj->startTime.auxtime.topnbot[1]);
        semTake(pXferObj->pFidXferSem,WAIT_FOREVER);
    }
    if (pXferObj->xferDownCount == -999)
       result = -1;  /* aborted */
    else
       result = 1;

    if ( (pXferObj->startTime.auxtime.lltime != 0LL) )
    {
       double durationInDouble;
       AuxTimeTicks endTime;
       AuxTimeTicks duration;
 
       vxTimeBaseGet(&endTime.auxtime.topnbot[0], &endTime.auxtime.topnbot[1]);
 
       duration.auxtime.lltime = endTime.auxtime.lltime - pXferObj->startTime.auxtime.lltime;
       pXferObj->startTime.auxtime.lltime = 0LL;
 
       durationInDouble = (double) duration.auxtime.lltime / ((double) sysTimerClkFreq / 1000.0); /* Hz/1000 -> KHz -> usec */
       pXferObj->numStops++;
       pXferObj->durAvg += durationInDouble;
       if (durationInDouble > pXferObj->durMax)
          pXferObj->durMax = durationInDouble;
       if (durationInDouble < pXferObj->durMin)
          pXferObj->durMin = durationInDouble;
          
       DPRINT1(-1,"Time Stopped:  %10.3f usec\n", durationInDouble);
     }  
}

XferLimitSummary()
{
    if ( pXferObj->numStops == 0 )
    {
       DPRINT(-6,"\n +++ Xfer Limit -- Fid Data Transfer was never limit stopped. +++\n\n");
    }
    else
    {
       DPRINT4(-6,"\n +++ Xfer Limit -- Stopped: %d, Avg:  %10.3f, Max: %10.3f, Min: %10.3f us +++\n\n", 
	    pXferObj->numStops, (pXferObj->durAvg / pXferObj->numStops), pXferObj->durMax, pXferObj->durMin);
    }

}

/*
 * show routine for this contruct
 *
 *                                      Author Greg Brissey  8/18/05
 *
 */
showTransferLimit()
{
   printf("Transfer Limit: (0x%lx)\n",pXferObj);
   printf("xferDownCount: %d, \n",pXferObj->xferDownCount);
   semShow(pXferObj->pFidXferSem,1);
   printf("\n\n");
}
