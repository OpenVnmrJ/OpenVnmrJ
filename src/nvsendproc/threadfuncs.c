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

/* #include <sys/types.h>
#include <sys/stat.h>
*/
#include <signal.h>
#ifndef LINUX
#include <thread.h>
#endif
#include <sys/errno.h>
#include <pthread.h>
#include <time.h>


/* #include "errors.h" */

#include <string.h>

#include "ndds/ndds_c.h"
#include "NDDS_Obj.h"
#include "Codes_Downld.h"

#ifndef RTI_NDDS_4x
#include "App_HB.h"
#endif /* RTI_NDDS_4x */

#include "errLogLib.h"

#include "nddsfuncs.h"
#include "threadfuncs.h"
#include "barrier.h"

extern int createCodeDownldPublication(cntlr_t *pCntlrThr,char *pubName);
extern int createCodeDownldSubscription(cntlr_t *pCntlrThr,char *subName);
extern int createAppHB_BESubscription(cntlr_t *pCntlrThr,char *subName);
extern int downLoadExpData(cntlr_t *pWrker,  void *expinfo, char *bufRootName);
extern void unMapDownLoadExpData(cntlr_t *pWrker,  void *expinfo, char *bufRootName);
void pThreadBlockAllSigs(void);

extern barrier_t TheBarrier;

#define CREW_SIZE 1

// #define MAX_IPv4_UDP_SIZE_BYTES 65535   /* IPv4 UDP max Packet size */
 
/* extern char databuf[MAX_IPv4_UDP_SIZE_BYTES]; */

#ifndef RTI_NDDS_4x

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
        mine->numSubcriber4Pub++;
	break;
      case NDDS_SUBSCRIPTION_DELETE:
	DPRINT2(+1,"'%s': A reliable subscription disappeared for '%s'.\n",mine->cntlrId, status->nddsTopic);
        mine->numSubcriber4Pub--;
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
#endif /* RTI_NDDS_4x */

/*
 *  Initialize the conditional and mutex for the download work crew 
 *
 *   Author: Greg Brissey   3/20/2004
 */
int initCrew(cntlr_crew_t *pCrew)
{
   int stat __attribute__((unused));

//   DPRINT1(+1,"Crew Struct Addr: %p\n",pCrew);

   /* clear complete structure */
   memset(pCrew,0,sizeof(cntlr_crew_t));

   pCrew->crew_size = 0;
   stat = pthread_mutex_init(&pCrew->mutex,NULL);  /* assign defaults to mutex */
//   DPRINT2(+1,"stat: %d, Mutex: 0x%lx\n",stat,pCrew->mutex);
   stat = pthread_cond_init(&pCrew->cmdgo,NULL);
//   DPRINT2(+1,"stat: %d, Cmd cond: 0x%lx\n",stat,pCrew->cmdgo);
   stat = pthread_cond_init(&pCrew->done,NULL);
//   DPRINT2(+1,"stat: %d, done cond: 0x%lx\n",stat,pCrew->done);
   return 0;
}
 
/*
 *  find the thread assign to a controller name, i.e. 'rf1'
 *  and return it's index in the work crew struct
 *
 *   Author: Greg Brissey   3/20/2004
 */
int findCntlr(cntlr_crew_t *pCrew, char *cntlrName)
{
   int crew_index;
   int index = -1;
   for (crew_index = 0; crew_index < pCrew->crew_size; crew_index++) {
        if ( strcmp(pCrew->crew[crew_index].cntlrId,cntlrName) == 0)
        {
             index = pCrew->crew[crew_index].index;
             break;
        }
   }
   return(index);
}   
 
#ifndef RTI_NDDS_4x
/*
 *  Add a download thread to handle a controller
 *
 *   Author: Greg Brissey   3/20/2004
 */
int addCntrlThread(cntlr_crew_t *pCrew, char *cntlrName)
{
   int index,status;
   char pubtopic[128];
   char subtopic[128];
   char subHBtopic[128];
   cntlr_t   *me;
   void *worker_routine (void *arg);

    index = findCntlr(pCrew, cntlrName);
    if (index == -1)
    {
        status = pthread_mutex_lock (&pCrew->mutex);
        if (status != 0)
            errLogSysQuit(LOGOPT,debugInfo,"Could not lock mutex controller '%s'",cntlrName);
 
        index = pCrew->crew_size;
        me = &(pCrew->crew[index]);
        pCrew->crew_size++;
        pCrew->crew[index].index = index;
        pCrew->crew[index].crew = pCrew;
        strcpy(pCrew->crew[index].cntlrId,cntlrName);

        barrierSetCount(&TheBarrier,pCrew->crew_size); /* update number of threads in the barrier membership */
                 
        /* HOST_PUB_TOPIC_FORMAT_STR "h/%s/dwnld/strm" */
        /* HOST_SUB_TOPIC_FORMAT_STR "%s/h/dwnld/reply" */
        sprintf(pubtopic,HOST_PUB_TOPIC_FORMAT_STR,cntlrName);
        sprintf(subtopic,HOST_SUB_TOPIC_FORMAT_STR,cntlrName);
#ifndef RTI_NDDS_4x
        sprintf(subHBtopic,SUB_NodeHB_TOPIC_FORMAT_STR,cntlrName);
#endif /* RTI_NDDS_4x */
        DPRINT3(+2,"addCntlrThread(): '%s': build PS: '%s', '%s'\n", me->cntlrId,pubtopic,subtopic);
        /* Publications used with multi thread need a thread ID which can be just an ordianl number */
        createCodeDownldPublication(me,pubtopic);
        createCodeDownldSubscription(me,subtopic);
#ifndef RTI_NDDS_4x
        createAppHB_BESubscription(me,subHBtopic);
#endif /* RTI_NDDS_4x */

        status = pthread_mutex_unlock (&pCrew->mutex);
        if (status != 0)
            errLogSysQuit(LOGOPT,debugInfo,"Could not unlock mutex controller '%s'",me->cntlrId);
        /* start thread */
        status = pthread_create (&pCrew->crew[index].threadId,
            NULL, worker_routine, (void*)&pCrew->crew[index]);
        if (status != 0)
           errLogSysQuit(LOGOPT,debugInfo,"Could not create controller; '%s', thread",me->cntlrId);
                 
    }            
    else         
      index = -1;
                 
    return(index);
}         

#else  /* RTI_NDDS_4x */

/* -------------------    for NDDS 4x ----------------------*/
int initCntrlThread(cntlr_crew_t *pCrew, char *cntlrName)
{
   int index,status;
   cntlr_t   *me;
   void *worker_routine (void *arg);

    index = findCntlr(pCrew, cntlrName);
    if (index == -1) // not alread present then buld it
    {
        status = pthread_mutex_lock (&pCrew->mutex);
        if (status != 0)
            errLogSysQuit(LOGOPT,debugInfo,"Could not lock mutex controller '%s'",cntlrName);
 
        index = pCrew->crew_size;
        me = &(pCrew->crew[index]);
        pCrew->crew_size++;
        pCrew->crew[index].index = index;
        pCrew->crew[index].crew = pCrew;
        strcpy(pCrew->crew[index].cntlrId,cntlrName);

        barrierSetCount(&TheBarrier,pCrew->crew_size); /* update number of threads in the barrier membership */
        status = pthread_mutex_unlock (&pCrew->mutex);
        if (status != 0)
            errLogSysQuit(LOGOPT,debugInfo,"Could not unlock mutex controller '%s'",me->cntlrId);
    }            
    else
       index = -1;
    return (index);
}

void createCntrlThread(cntlr_crew_t *pCrew, int theadIndex, NDDSBUFMNGR_ID pNddsBufMngr)
{
   cntlr_t   *pCntlrThr;
   int status;
   void *worker_routine (void *arg);

   pCntlrThr = &(pCrew->crew[theadIndex]);
   status = pthread_mutex_lock (&pCrew->mutex);
   if (status != 0)
      errLogSysQuit(LOGOPT,debugInfo,"Could not lock mutex controller '%s'",pCntlrThr->cntlrId);

    pCntlrThr->numSubcriber4Pub = 1;   // hack for now GMB
    pCntlrThr->pNddsBufMngr = pNddsBufMngr;
    // pCntlrThr->SubId = pSubObj;
    // pCntlrThr->PubId = pPubObj;
    status = pthread_mutex_unlock (&pCrew->mutex);
    if (status != 0)
       errLogSysQuit(LOGOPT,debugInfo,"Could not unlock mutex controller '%s'",pCntlrThr->cntlrId);
    /* start thread */
    status = pthread_create (&(pCntlrThr->threadId),
            NULL, worker_routine, (void*) pCntlrThr);
    if (status != 0)
       errLogSysQuit(LOGOPT,debugInfo,"Could not create controller; '%s', thread",pCntlrThr->cntlrId);
                 
}

#endif /* RTI_NDDS_4x */
   
/*
 * The thread start routine for crew threads. Waits until "go"
 * command, processes work items until requested to shut down.
 */
void *worker_routine (void *arg)
{
   cntlr_t   *mine = (cntlr_t *) arg;
   cntlr_crew_t *crew = mine->crew;
   int status;
   int state;

// DPRINT3(+2,"'%s': Crew %d, thrdId: %d,  starting\n", mine->cntlrId,mine->threadId,mine->index);

    /* DPRINT1(+1,"crew mutex: addr to: 0x%lx, actual addr: 0x%lx\n",&crew->mutex,crew->mutex); */
    pThreadBlockAllSigs();   /* block most signals here */

    pthread_detach(mine->threadId);   /* if thread terminates no need to join it to recover resources */
    
    /*
     * Now, as long as there's work, keep doing it.
     */
    while (1) {

        /*
         * Lock mutex prior to checking and/or waiting on conditional 
         */
        status = pthread_mutex_lock (&crew->mutex);
        if (status != 0)
            errLogSysQuit(LOGOPT,debugInfo,"Could not lock mutex for controller '%s'",mine->cntlrId);
 
        /*
	 * if the is no work (crew->work_count == 0) or this thread is done (crew->cmd[mine->index] < 1)
         * then wait on the conditional, when work is ready we will be awakened
         */
        while ((crew->work_count == 0) || ( crew->cmd[mine->index] < 1) ) {
            status = pthread_cond_wait (&crew->cmdgo, &crew->mutex);
            if (status != 0)
                errLogSysQuit(LOGOPT,debugInfo,"COnditaion wait error for controller '%s'",mine->cntlrId);
        }

	/* store our command into the threads work struct */
        mine->cmd = crew->cmd[mine->index];

        /* mark command as taken, and being worked on */
        crew->cmd[mine->index] = 0;
        

        /* have a private copy no need to keep mutex locked, 
         * need to let the other threads run to.
         */ 
        status = pthread_mutex_unlock (&crew->mutex);
        if (status != 0)
            errLogSysQuit(LOGOPT,debugInfo,"Could not unlock mutex for controller '%s'",mine->cntlrId);

//      DPRINT5(+2,"'%s': Crew %d woke,a Cmd: %d, work cnt: %d, subcriber: %d\n",
//                mine->cntlrId, mine->index, mine->cmd, crew->work_count,mine->numSubcriber4Pub);
 

        /*
         * We have work to do. Process it, which may involve
         */

        /* if we have no subscriber then don't bother trying to download to that controller */
        if (mine->numSubcriber4Pub > 0)
        {
            /* prevent cancellation of thread while it is transfering files */
            status = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &state);
            if (status != 0)
               errLogSysQuit(LOGOPT,debugInfo,"Could not disable cancel for controller '%s'",mine->cntlrId);

           /* *************************************************************************** */
           /* *************************************************************************** */
           /* downLoadExpData(cntlr_t *pWrker,  void *expinfo, char *bufRootName) */

           status = downLoadExpData(mine,  mine->pParam, mine->label);

           /* *************************************************************************** */
           /* *************************************************************************** */

            /* re-enable deferred cancelation */
            status = pthread_setcancelstate(state, &state);
            if (status != 0)
               errLogSysQuit(LOGOPT,debugInfo,"Could not re-enable cancel for controller '%s'",mine->cntlrId);

            /* Once a thread is done it's work, remove it from the barrier membership */
            /*
             *  Without this action, the problem arises for interleave where some controllers can
             *  hold all the acodes and some not. The scenario is, for cntlr threads that can hold all
             *  the Acodes, the thread completes, however the others must continue to send acodes,
             *  when they enter the barrierWait(), the threads stop waiting for the completed threads 
             *  to send more acodes which they never will, thus dead-locked .
             */
            DPRINT2(2,"'%s': Crew %d done it's work, decrement barrier membership\n", mine->cntlrId, mine->index);
            barrierDecThreshold(&TheBarrier);

        }
        else
        {
           barrierDecThreshold(&TheBarrier);
           DPRINT1(+1,"'%s': No  Action Taken,  No subcribers!!!\n", mine->cntlrId );
        }
        /*
         * It's important that the count be decremented AFTER
         * processing the current controller . That ensures the
         * count won't go to 0 until we're really done.
         */

        /* always lock the mutex before change a value in the crew struct */
        status = pthread_mutex_lock (&crew->mutex);
        if (status != 0)
            errLogSysQuit(LOGOPT,debugInfo,"Could not lock mutex for controller '%s'",mine->cntlrId);
 
        crew->work_count--;

//      DPRINT3(+2,"'%s': Crew %d decremented work to %d\n", mine->cntlrId, mine->index,
//                crew->work_count);

        /* if this is the last thread to finish, then a few items must be done */
        /* 1. Unmap any files that were used */
        /* 2. Signal anyone waiting on the completion conditional that we are done. */
        if (crew->work_count <= 0) 
        {
            barrierResetThreshold(&TheBarrier);
            DPRINT(+2,"Crew all threads done, unmap files.\n");
            unMapDownLoadExpData(mine,  mine->pParam, mine->label);

            DPRINT2(+2,"All threads done, '%s':  %d finished last\n", mine->cntlrId,mine->index);
            status = pthread_cond_broadcast (&crew->done);
            if (status != 0)
               errLogSysQuit(LOGOPT,debugInfo,"Could not broadcast on conditional for controller '%s'",mine->cntlrId);
        }


        status = pthread_mutex_unlock (&crew->mutex);
        if (status != 0)
            errLogSysQuit(LOGOPT,debugInfo,"Could not unlock mutex for controller '%s'",mine->cntlrId);

    }

    return NULL;
}


/*
 * method that the main thread calls to be sure all controller threads
 * have reached their idle state
 */
int wait4DoneCntlrStatus(cntlr_crew_t *crew)
{
    int status, cancel, temp;
    struct timespec timeout;

    status = pthread_mutex_lock (&crew->mutex);
    if (status != 0)
       return -1;

//    DPRINT1(+2,"wait4DoneCntlrStatus: work  is %d\n",crew->work_count);
    if (crew->work_count == 0)
    {
       DPRINT(+2,"wait4DoneCntlrStatus: count is Zero, Just return\n");
       status = 0;
    }
    else 
    {
       char *nullvalue = NULL;

       /* not a cancellation point, disable it */
       pthread_setcancelstate( PTHREAD_CANCEL_DISABLE, &cancel);
 
       /*
        * Wait until workcount == 0 which means that all
        * sending threads have return to there idle state 
        */
//     DPRINT1(+2,"wait4DoneCntlrStatus: active: %d, wait on conditional\n",
//              crew->work_count);

       while(crew->work_count > 0)
       {
          int i;

          barrierWaitAbort(&TheBarrier);  /* free any struck threads */
          time(&timeout.tv_sec);
          timeout.tv_sec += 1;    /* one second timeout */
          timeout.tv_nsec = 0;
          for( i = 0; i < crew->crew_size; i++)
          {
	      /* check each thread, if block on msgGet() place a NULL
               * within the msgQ, to release the thread, NULL values
               * will be interpreted as an aborted msg
               */
              if (msgeGetIsPended(crew->crew[i].pNddsBufMngr) == 1)
              {
                DPRINT1(+2,"'%s': is blocked on msgeGet()\n", crew->crew[i].cntlrId);
                msgePost(crew->crew[i].pNddsBufMngr,nullvalue);
              }
          }
          status = pthread_cond_timedwait( &crew->done, &crew->mutex, &timeout);
          DPRINT2(+2,"pthread_cond_timedwait: status return: %d, ETIMEDOUT: %d\n",status,ETIMEDOUT);
          if (status == ETIMEDOUT )
          {
                DPRINT(+2,"wait4DoneCntlrStatus: timeout, retry\n");
          }
      }
      pthread_setcancelstate( cancel, &temp);
    }
    pthread_mutex_unlock (&crew->mutex);
    return status;     /* error, -1 for waker, or 0 */
}

   

void pThreadBlockAllSigs()
{
   sigset_t   blockmask,oldmask;
   sigemptyset( &blockmask );
   sigaddset( &blockmask, SIGALRM );
   sigaddset( &blockmask, SIGIO );
   sigaddset( &blockmask, SIGCHLD );
   sigaddset( &blockmask, SIGQUIT );
   sigaddset( &blockmask, SIGPIPE );
   sigaddset( &blockmask, SIGALRM );
   sigaddset( &blockmask, SIGTERM );
   sigaddset( &blockmask, SIGUSR1);
   sigaddset( &blockmask, SIGUSR2);
   pthread_sigmask(SIG_BLOCK,&blockmask,&oldmask);
}
#ifdef XXXX
/*
 * rm thread via pthread_cancel
 * this tended to hang all the threads,
 * thus a differnet technique was used, to allow
 * the thread itselef to determine self termination.
 *
 *   Author: Greg Brissey   3/20/2004
 */
rmCntrlThread(cntlr_crew_t *pCrew, char *cntlrName)
{
   int index,status,state;
   cntlr_t   *me;
   void *result;

   index = findCntlr(pCrew, cntlrName);
   if (index >= 0)
   {
     DPRINTF( ("removing Thread: '%s', index: %d\n",cntlrName,index) );
       me = &(pCrew->crew[index]);
      status = pthread_cancel(me->threadId);
      if (status != 0)
        err_abort (status,"Cancel Thread");
      status = pthread_join (me->threadId, &result);
      if (status != 0)
        err_abort (status,"Join Thread");
     if (result == PTHREAD_CANCELED)
        printf("thread canceled.\n");
     else
        printf("thread NOT canceled.\n");
      pCrew->crew_size--;
      if (me->SubId != NULL)
      {

          status = nddsSubscriptionDestroy(me->SubId);
          me->SubId = NULL;
          if (me->SubPipeFd[0] > 0)
          {
              close(me->SubPipeFd[0]);
              close(me->SubPipeFd[1]);
	      me->SubPipeFd[0] = me->SubPipeFd[1] = -1;
          }
      }
      if (me->PubId != NULL)
      {
        status = nddsPublicationDestroy(me->PubId);
        me->PubId = NULL;
      } 
   }
   else
   {
     DPRINT1(-4,"Cntlr: '%s', not present in crew\n",cntlrName);
   }
}
#endif
