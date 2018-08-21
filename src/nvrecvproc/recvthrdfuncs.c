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

#include <signal.h>
#ifndef LINUX
#include <thread.h>
#endif
#include <sys/errno.h>
#include <pthread.h>
#include <time.h>


#include <string.h>
#include "ndds/ndds_c.h"
#include "NDDS_Obj.h"
#ifndef RTI_NDDS_4x
#include "Data_UploadCustom3x.h"
#else /* RTI_NDDS_4x */
#include "Data_Upload.h"
#endif  /* RTI_NDDS_4x */
// #include "App_HB.h"

#include "errLogLib.h"

#include "hostAcqStructs.h"
#include "mfileObj.h"
#include "rngBlkLib.h"
#include "workQObj.h"
#include "recvthrdfuncs.h"
#include "barrier.h"

extern int createDataUploadPub(cntlr_t *pCntlrThr,char *pubName);
extern int createDataUploadSub(cntlr_t *pCntlrThr,char *subName);
extern int createAppHB_BESub(cntlr_t *pCntlrThr,char *subName);
void pThreadBlockAllSigs(void);

extern barrier_t TheBarrier;

/* PIPE_STAGE_ID pProcThreadPipeStage = NULL; */

RCVR_DESC ProcessThread;

#define MAX_DDR_SUBS 64
extern RCVR_DESC_ID ddrSubList[MAX_DDR_SUBS];
extern int numSubs;

cntlr_status_t CntlrStatus;


#define CREW_SIZE 1

#define MAX_IPv4_UDP_SIZE_BYTES 65535   /* IPv4 UDP max Packet size */
 
#ifndef RTI_NDDS_4x
/* extern char databuf[MAX_IPv4_UDP_SIZE_BYTES]; 8/

/*
     Reliable Publication Status call back routine.
     At present we use this to indicate if a subscriber has come or gone
*/
void MyThreadPubStatusRtn(NDDSPublicationReliableStatus *status,
                                    void *callBackRtnParam)
{
      cntlr_t *mine = (cntlr_t*) callBackRtnParam;
      switch(status->event) 
      {
      case NDDS_QUEUE_EMPTY:
        DPRINT1(1,"'%s': Queue empty\n",mine->cntlrId);
        break;
      case NDDS_LOW_WATER_MARK:
        DPRINT1(1,"'%s': Below low water mark - ",mine->cntlrId);
        DPRINT2(1,"Topic: '%s', UnAck Issues: %d\n",status->nddsTopic, status->unacknowledgedIssues);
        break;
      case NDDS_HIGH_WATER_MARK:
        DPRINT1(1,"'%s': Above high water mark - ",mine->cntlrId);
        DPRINT2(1,"Topic: '%s', UnAck Issues: %d\n",status->nddsTopic, status->unacknowledgedIssues);
        break;
      case NDDS_QUEUE_FULL:
        DPRINT1(1,"'%s': Queue full - ",mine->cntlrId);
        DPRINT2(1,"Topic: '%s', UnAck Issues: %d\n",status->nddsTopic, status->unacknowledgedIssues);
        break;
      case NDDS_SUBSCRIPTION_NEW:
	DPRINT2(+1,"'%s': A new reliable subscription Appeared for '%s'.\n",mine->cntlrId, status->nddsTopic);
        /* mine->numSubcriber4Pub++; */
	break;
      case NDDS_SUBSCRIPTION_DELETE:
	DPRINT2(+1,"'%s': A reliable subscription disappeared for '%s'.\n",mine->cntlrId, status->nddsTopic);
        /* mine->numSubcriber4Pub--; */
#ifdef  TEST_THREAD_DELETION   /* not used, not safe!! */
        if (mine->numSubcriber4Pub == 0)
        {
	    mine->numSubcriber4Pub = -1; /* -1 remoe thread */
            /* rmCntrlThread(mine->crew, mine->cntlrId); */
        }
#endif
	break;
      default:
		/* NDDS_BEFORERTN_VETOED
		   NDDS_RELIABLE_STATUS
	        */
        break;
      }
}
#endif  /* RTI_NDDS_4x */

int findCntlr( char *cntlrName)
{
   int crew_index;
   int index = -1;
   for (crew_index = 0; crew_index < numSubs; crew_index++) {
        if ( strcmp(ddrSubList[crew_index]->cntlrId,cntlrName) == 0)
        {
             index = crew_index;
             break;
        }
   }
   return(index);
}   



/* 
 *  create the one processing thread, all receiver (ddr) thread feed into this one processing thread
 * 
 *     Author:  Greg Brissey      2005
 */
void buildProcPipeStage(void *pParam)
{
     int index,status;
     RCVR_DESC_ID pRcvrDesc;
     void *worker_routine (void *arg);
     int  processFid(void *);

     pRcvrDesc = &ProcessThread;
     memset(pRcvrDesc,0,sizeof(RCVR_DESC));
     strncpy(pRcvrDesc->cntlrId,"ProcessStage",32);
     /* pRcvrDesc->pInputQ = rngBlkCreate(QUEUE_LENGTH,"ProcInputQ", 1); /* me->pPipeStages[1]->pOutputQ; */
     pRcvrDesc->pInputQ = rngBlkCreate(1024,"ProcInputQ", 1); /* me->pPipeStages[1]->pOutputQ; */
     pRcvrDesc->pOutputQ = NULL;
     pRcvrDesc->pParam = pParam;
     pRcvrDesc->pCallbackFunc = processFid;

 
     /* start thread */
     status = pthread_create (&(pRcvrDesc->threadID),
                                  NULL, worker_routine, (void*) pRcvrDesc);
  if (status != 0)
     errLogSysQuit(LOGOPT,debugInfo,"Could not creat controller; '%s', thread",pRcvrDesc->cntlrId);
  return;
}


/*
 *  Add a data upload thread to handle a DDR Receiver
 *
 *   Author: Greg Brissey   4/10/2005
 */
int addCntrlThread(RCVR_DESC_ID pRcvrDesc, char *cntlrName)
{
   int index,status;
   void *worker_routine (void *arg);
   int  recvFid(void *);

   pRcvrDesc->pCallbackFunc = recvFid;
 
  /* start thread */
  status = pthread_create (&(pRcvrDesc->threadID),
                                  NULL, worker_routine, (void*) pRcvrDesc);
  if (status != 0)
     errLogSysQuit(LOGOPT,debugInfo,"Could not creat controller; '%s', thread",pRcvrDesc->cntlrId);
  return pRcvrDesc->threadID;
}
   
/*
 * The following routines are to allow the processFID() thread wait for the recvFid() threads to
 * come to a non active state prior to preceeding to closing the Experiment file, etc.
 * Thus prevent various race conditions and recvFid() threads hung in barrierWait()
 *
 *   Author: Greg Brissey  10/18/2005
 */
/*
 *   initialize the thread status mutex,conditaion variable, etc.
 */
initCntlrStatus()
{
   int status,i;
   cntlr_status_t *pCntlrStatus;

   pCntlrStatus = &CntlrStatus;
   
   pCntlrStatus->numCntlrs = 0;
   pCntlrStatus->cntlrsActive = 0;
   pCntlrStatus->waiting4Done = 0;
   pCntlrStatus->wrkdone = 0;
   status = pthread_mutex_init( &pCntlrStatus->mutex, NULL );
   if (status != 0)
       return status;

   status = pthread_cond_init( &pCntlrStatus->done, NULL);
   if (status != 0)
   {
      pthread_mutex_destroy( &pCntlrStatus->mutex );
      return status;
   }

   for(i=0; i < MAX_DDR_SUBS; i++)
      memset(&(ddrSubList[i]),0,sizeof(RCVR_DESC));

   return 0;
}

/*
 *  reset predicates of conditional variable
 */
resetCntlrStatus()
{
   int status;
   cntlr_status_t *pCntlrStatus;

   pCntlrStatus = &CntlrStatus;
   
   status = pthread_mutex_lock( &pCntlrStatus->mutex );
    if (status != 0)
        return status;

    pCntlrStatus->cntlrsActive = 0;
    pCntlrStatus->waiting4Done = 0;

    pthread_mutex_unlock( &pCntlrStatus->mutex);
    return 0;     /* error, -1 for waker, or 0 */
}

/*
 * As each thread recvFid() start to work it increments the number threads active  
 */
incrActiveCntlrStatus(char *cntlrId)
{
    int status;
   cntlr_status_t *pCntlrStatus;
   pCntlrStatus = &CntlrStatus;

    status = pthread_mutex_lock( &pCntlrStatus->mutex );
    if (status != 0)
        return status;

    pCntlrStatus->cntlrsActive++;
    pCntlrStatus->wrkdone = 0;
    DPRINT2(+2,"'%s: incrActiveCntlrStatus: active: %d\n", cntlrId,pCntlrStatus->cntlrsActive);

    pthread_mutex_unlock( &pCntlrStatus->mutex);
    return 0;     /* error, -1 for waker, or 0 */

}
   
/*
 * As each thread recvFid() finishs it's work it decrements the number threads active  
 */
decActiveCntlrStatus(char *cntlrId)
{
    int status;
   cntlr_status_t *pCntlrStatus;

   pCntlrStatus = &CntlrStatus;

    status = pthread_mutex_lock( &pCntlrStatus->mutex );
    if (status != 0)
        return status;

    pCntlrStatus->cntlrsActive--;

    DPRINT2(+2,"'%s': decActiveCntlrStatus: active: %d\n", cntlrId,pCntlrStatus->cntlrsActive);
    if (pCntlrStatus->cntlrsActive == 0)
    {
       DPRINT1(+2,"decActiveCntlrStatus: wait4done: %d\n",pCntlrStatus->waiting4Done);
       /* the usage of waiting4Done never worked properly occassional ending up never
        * broadcasting the conditiona variable */
       /* if ( pCntlrStatus->waiting4Done == 1)		/* thread waiting for done */
       /* {  */
         DPRINT(+2,"decActiveCntlrStatus: count Zero, and someone waiting broadcast to show all done\n");
         pCntlrStatus->wrkdone = 1;
         status = pthread_cond_broadcast( &pCntlrStatus->done );
       /*  } */
    }

    pthread_mutex_unlock( &pCntlrStatus->mutex);
    return 0;     /* error, -1 for waker, or 0 */

}
   
/*
 * Used by processFID() thread, to wait for the recvFID() threads to reach the non-active state
 * then it can safely proceed to close up experimenr files, etc.
 * I tried setting time out less than 1 second via getimeofday()
 * gettimeofday(&tp,NULL);
 *  timeout.tv_sec = tp.tv_sec;    This never worked properly
 *  timeout.tv_nsec =  (tp.tv_usec * 1000) + 500000000;   .50 sec
 * But this never worked.
 */
wait4DoneCntlrStatus()
{
    int status, cancel, temp;
    cntlr_status_t *pCntlrStatus;
    struct timespec timeout;

   pCntlrStatus = &CntlrStatus;
    status = pthread_mutex_lock( &pCntlrStatus->mutex );
    if (status != 0)
        return status;

    pCntlrStatus->waiting4Done = 1;		/* thread waiting for done */
    if (pCntlrStatus->cntlrsActive == 0)
    {
       DPRINT(+2,"wait4DoneCntlrStatus: count is Zero, Just return\n");
       status = 0;
    }
    else
    {
       /* not a cancellation point, disable it */
       pthread_setcancelstate( PTHREAD_CANCEL_DISABLE, &cancel);

       /*
        * Wait until wrkdone == 1 which means that it
        * has been boradcast to continue
        */
       DPRINT2(+2,"wait4DoneCntlrStatus: active: %d, workdone: %d, wait on conditional\n",
		pCntlrStatus->cntlrsActive,pCntlrStatus->wrkdone);
       while( pCntlrStatus->wrkdone != 1 )
       {
          barrierWaitAbort(&TheBarrier);  /* free any struck threads */
          time(&timeout.tv_sec);
          timeout.tv_sec += 1;    /* one second timeout */
          timeout.tv_nsec = 0;
          status = pthread_cond_timedwait( &pCntlrStatus->done, &pCntlrStatus->mutex,&timeout);
          /* status = pthread_cond_wait( &pCntlrStatus->done, &pCntlrStatus->mutex); */
          DPRINT2(+2,"pthread_cond_timedwait: status return: %d, ETIMEDOUT: %d\n",status,ETIMEDOUT);
          if (status == ETIMEDOUT )
          {
		DPRINT(+2,"wait4DoneCntlrStatus: timeout, retry\n");
          }
/*
          if (status != ETIMEDOUT ) 
          {
	      DPRINT(-4,"wait4DoneCntlrStatus: Error return\n");
              break;
          }
*/
       }
       pthread_setcancelstate( cancel, &temp);
    }
    pCntlrStatus->waiting4Done = 0;		/* thread waiting for done */
    pthread_mutex_unlock( &pCntlrStatus->mutex);
    return status;     /* error, -1 for waker, or 0 */
}

/*
 * The thread start routine for crew threads. Waits in the rngBlkGet()
 * call  where address of workQEntries are received, then processes work items until requested to shut down.
 *
 * For none active Receivers/DDRs their cmd will set to -1 , thus they will
 *  pend immediately.
 *
 *     Author:  Greg Brissey
 */
void *worker_routine (void *arg)
{
   int status;
   WORKQ_ENTRY_ID  pWrkQentry;
   RCVR_DESC_ID pRcvrDesc;
   WORKQINVARIENT_ID pWrkqInvar;
   int inQueue;
   int processThreadFlag;
   char *CntlrId;

   RCVR_DESC_ID pAccessWrkDesc = (RCVR_DESC_ID) arg; 
   if (pAccessWrkDesc == &ProcessThread)
      processThreadFlag = 1;
   else
      processThreadFlag = 0;

   CntlrId = pAccessWrkDesc->cntlrId;
   DPRINT3(+2,"'%s':  threadId: 0x%lx, RCVR_DESC_ID: 0x%lx,  starting\n", 
         CntlrId,pAccessWrkDesc->threadID,pAccessWrkDesc);

    pThreadBlockAllSigs();   /* block most signals here */

    pthread_detach(pAccessWrkDesc->threadID);   /* if thread terminates no need to join it to recover resources */

    while (1) {

	/* obtain work from the pipe line Q, if non the thread blocks */
        rngBlkGet(pAccessWrkDesc->pInputQ, &pWrkQentry,1);

        if (!processThreadFlag)
           incrActiveCntlrStatus(CntlrId);
             
        pRcvrDesc = (RCVR_DESC_ID) pWrkQentry->pInvar->pRcvrDesc;


        DPRINT2(+2,"'%s': Got workQ: 0x%lx\n", pRcvrDesc->cntlrId, pWrkQentry);
        inQueue = rngBlkNElem(pRcvrDesc->pInputQ);
        DPRINT2(+2,"'%s': work still in Q: %d\n", pRcvrDesc->cntlrId, inQueue);

#define INSTRUMENT
#ifdef INSTRUMENT
        if (inQueue >  pRcvrDesc->p4Diagnostics->pipeHighWaterMark)
             pRcvrDesc->p4Diagnostics->pipeHighWaterMark = inQueue;
#endif

        DPRINT2(+2,"'%s': Got workQ: 0x%lx\n", pAccessWrkDesc->cntlrId, pAccessWrkDesc);
        DPRINT2(+2,"'%s': work still in Q: %d\n", pAccessWrkDesc->cntlrId, rngBlkNElem(pAccessWrkDesc->pInputQ));

        status = (pAccessWrkDesc->pCallbackFunc)(pWrkQentry);

        /* if status is not < 0 then send workQ on to next stage
         * if status < 0 then there is an error or the function call already
         * handled the passing to the next stage and this routine does not need to
         */
        DPRINT2(+2,"'%s': CallBack Returned: %d\n", pRcvrDesc->cntlrId, status);
        if (status  >= 0)
        {
           DPRINT2(+2,"'%s': Done,  Send WorkQ onto Processing Thread: 0x%lx\n", 
		pRcvrDesc->cntlrId, pRcvrDesc);
           rngBlkPut(pAccessWrkDesc->pOutputQ, &pWrkQentry,1);
        }
        if (!processThreadFlag)
           decActiveCntlrStatus(CntlrId);
    }
}

void pThreadBlockAllSigs()
{
   sigset_t   blockmask;
   sigfillset( &blockmask );
#ifdef PROFILING  /* enable these signals to allow profiling of threads */
   sigdelset(&blockmask, SIGPROF);
   sigdelset(&blockmask, SIGEMT);
#endif
   pthread_sigmask(SIG_BLOCK,&blockmask,NULL);
}
