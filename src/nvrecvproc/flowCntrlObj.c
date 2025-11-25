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
#ifndef LINUX
#include <thread.h>
#endif
#include <pthread.h>

// #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
  
#include "errLogLib.h"
#ifndef RTI_NDDS_4x
#include "Data_UploadCustom3x.h"
#else /* RTI_NDDS_4x */
#include "Data_Upload.h"
#endif  /* RTI_NDDS_4x */
/* #include "ndds/ndds_cpp.h" */
#include "flowCntrlObj.h"

extern int send2DDR(NDDS_ID pPub, int cmd, int arg1, int arg2, int arg3);

/*
modification history
--------------------
1-08-07,gmb  created
*/

/*
DESCRIPTION

 flow control for FID uplink Object for recvproc

*/

FlowContrlObj *flowCntrlCreate()
{
  int status __attribute__((unused));
  FlowContrlObj *pFlowCntrl;

  pFlowCntrl = (FlowContrlObj *) malloc(sizeof(FlowContrlObj)); /* create structure */
  if (pFlowCntrl == NULL) 
     return (NULL);

  memset(pFlowCntrl,0,sizeof(FlowContrlObj));

  status = pthread_mutex_init(&pFlowCntrl->mutex,NULL);

  return(pFlowCntrl);
}

void resetFlowCntrl(FlowContrlObj *pFlowCntrl)
{
    int i;
    pFlowCntrl->numPubs = 0;
    pFlowCntrl->matchNum = 0;
    pFlowCntrl->numPubsAtHiH2oMark = 0;
    pFlowCntrl->AtIncreNum = 0L;
    for(i=0; i < MAX_SUBSCRIPTIONS; i++)
    {
        pFlowCntrl->incrementVals[i] = 0;
    }   
    RTINtpTime_setZero(&pFlowCntrl->_timeStarted);
    RTINtpTime_setZero(&pFlowCntrl->_timeDuration);
    pFlowCntrl->timesReplySent = 0;
    return;
}

void initFlowCntrl(FlowContrlObj *pFlowCntrl, int Id, char *cntlrId, NDDS_ID PubId, int maxLimit, int xferHiH2OLimit)
{
    /* Id will range 1-n, thus subtract one for 0-n */
    Id--;
//    DPRINT5(+2,"initFlowCntrl: Id: %d, Idstr: '%s', Pub: 0x%lx, max & hih2o: %d, %d\n",Id,cntlrId,PubId,maxLimit,xferHiH2OLimit);
    pFlowCntrl->cntlrId[Id] = cntlrId;
    pFlowCntrl->idIndex[pFlowCntrl->numPubs] = Id;
    pFlowCntrl->pubs[pFlowCntrl->numPubs] = PubId;
    pFlowCntrl->maxXferLimit = maxLimit;
    pFlowCntrl->HighH2OMark = xferHiH2OLimit;
    pFlowCntrl->numPubs++;
    pFlowCntrl->matchNum = pFlowCntrl->numPubs;
    return;
}

void publishIncremFlowMsg(FlowContrlObj *pFlowCntrl)
{
    int i;
    int status __attribute__((unused));
    for (i = 0; i < pFlowCntrl->numPubs; i++ )
    {
       status = send2DDR(pFlowCntrl->pubs[i], C_RECVPROC_CONTINUE_UPLINK, i, pFlowCntrl->AtIncreNum, 0);
    }
    return;
}


/*
 * The problem with increment and sending within the same function is that recvFid() is executed with in multiple threads,
 * so each thread must increment the count, but only one thread (the one coming back from the marrier wait should actually
 * send the message, so one function will not work, must break this up into two, one to inclrement and another to test and send
 *      Greg Brissey 1/25/07
 */
void IncrementTransferLoc(FlowContrlObj *pFlow, int Id)
{
    /* Id will range 1-n, thus subtract one for 0-n */
    Id--;

   /* increment number of issues received by this publisher */
   pFlow->incrementVals[Id]++;

   /* if the # of issues is == high water mark, increment the number of publishers
    * at the high water mark
    */
   DPRINT5(2,"'%s': At: %d, High H2O Mark: %d, # At Mark: %d, matchNum: %d\n",pFlow->cntlrId[Id],
                pFlow->incrementVals[Id],  pFlow->HighH2OMark, pFlow->numPubsAtHiH2oMark, pFlow->matchNum);
      if (pFlow->incrementVals[Id] == pFlow->HighH2OMark)
      {      
         pthread_mutex_lock (&(pFlow->mutex));
           pFlow->numPubsAtHiH2oMark++;
         pthread_mutex_unlock (&(pFlow->mutex));
          /* DPRINT6(-5,"'%s': AtH2OMark incrCnt[%d]: %d, High H2O Mark: %d, # At Mark: %d, matchNum: %d\n",pFlow->cntlrId[Id],Id,
                pFlow->incrementVals[Id],  pFlow->HighH2OMark, pFlow->numPubsAtHiH2oMark, pFlow->matchNum); */
          DPRINT5(2,"'%s': At: %d, High H2O Mark: %d, # At Mark: %d, matchNum: %d\n",pFlow->cntlrId[Id],
                pFlow->incrementVals[Id],  pFlow->HighH2OMark, pFlow->numPubsAtHiH2oMark, pFlow->matchNum);
      }
   return;
}

int  AllAtIncrementMark(FlowContrlObj *pFlow, int Id)
{
   RTINtpTime endTime;
   RTINtpTime duration;
   int i,msgSent,index;

   msgSent = 0;
    /* Id will range 1-n, thus subtract one for 0-n */
    Id--;

   DPRINT5(+2,"'%s': TAS; Increment: %u, High H2O Mark: %d, # At Mark: %d, matchNum: %d\n",pFlow->cntlrId[Id],
                 (pFlow->AtIncreNum + pFlow->HighH2OMark), pFlow->HighH2OMark, pFlow->numPubsAtHiH2oMark, pFlow->matchNum);

   /* have all the publishers reach the high water mark */
   pthread_mutex_lock (&(pFlow->mutex));
   if ( pFlow->numPubsAtHiH2oMark >= pFlow->matchNum ) 
   {
      pFlow->AtIncreNum += pFlow->HighH2OMark;

      /* Now reset counters and test values */
      pFlow->numPubsAtHiH2oMark = 0;
      for( i = 0; i < pFlow->numPubs; i++)
      {  
          index = pFlow->idIndex[i];
          pFlow->incrementVals[index] = 0;
      }  
      pthread_mutex_unlock (&(pFlow->mutex));

      /* send to all publishers to continue with publishing */
      DPRINT(+2,"Send Flow Msg to Continue\n");
      publishIncremFlowMsg(pFlow);
 
      msgSent = 1; 
      /* 1st time though start time is going to be zero so skip the calc */
#ifndef RTI_NDDS_4x
      if (RtiNtpTimeIsZero(&(pFlow->_timeStarted)) == RTI_FALSE)
      {  
         NddsUtilityTimeGet(&endTime);
         /* this duration should be the time that is took all the publishers to
            reach the high Water Mark */
         RtiNtpTimeSubtract(duration, endTime, pFlow->_timeStarted);
         /* add up the duration, an average will be calucated at the end */
         RtiNtpTimeIncrement(pFlow->_timeDuration, duration);
         pFlow->timesReplySent++;
      }  
      NddsUtilityTimeGet(&(pFlow->_timeStarted));
#else /* RTI_NDDS_4x */
      if (RTINtpTime_compareToZero(&(pFlow->_timeStarted)) == 1)
      {  
         get_time(&endTime);
         /* this duration should be the time that is took all the publishers to
            reach the high Water Mark */
         RTINtpTime_subtract(duration, endTime, pFlow->_timeStarted);
         /* add up the duration, an average will be calucated at the end */
         RTINtpTime_increment(pFlow->_timeDuration, duration);
         pFlow->timesReplySent++;
      }  
      get_time(&(pFlow->_timeStarted));
#endif  /* RTI_NDDS_4x */
   }
   else
   {
      pthread_mutex_unlock (&(pFlow->mutex));
   }
   return ( msgSent );
}       

void MarkFlowCntrPubDone(FlowContrlObj *pFlow, int Id)
{
    /* Id will range 1-n, thus subtract one for 0-n */
    Id--;

   DPRINT3(+2,"'%s':  High H2O Mark: %d, # At Mark: %d, Is DONE!\n",pFlow->cntlrId[Id],
                pFlow->HighH2OMark, pFlow->numPubsAtHiH2oMark);
   pthread_mutex_lock (&(pFlow->mutex));
     pFlow->matchNum--;
   pthread_mutex_unlock (&(pFlow->mutex));
   return; 
}
 
void ReportFlow(FlowContrlObj *pFlow)
{
    int durationSec;
    int durationNanosec;
    double durationInDouble;
    double durationAvg;

 
#ifndef RTI_NDDS_4x
    RtiNtpTimeUnpackToNanosec(durationSec, durationNanosec, pFlow->_timeDuration);
#else /* RTI_NDDS_4x */
    RTINtpTime_unpackToNanosec(durationSec, durationNanosec, pFlow->_timeDuration);
#endif  /* RTI_NDDS_4x */
    durationInDouble = durationSec + (double)durationNanosec/1000000000.0f;
    durationAvg = durationInDouble / (double) pFlow->timesReplySent;
    durationAvg = durationAvg * 1000.0; /* milliseconds now */
 
    DPRINT2(+2,"Times Flow msg Sent: %d, Avg Time For Publishers to Reach HiWaterMark: %10.3f milliseconds\n",
        pFlow->timesReplySent, durationAvg);
 
    return;
}
