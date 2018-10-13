/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* HB are not used any more even for 3X so for 4X everything is ifdef'd out */
#ifndef RTI_NDDS_4x

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "ndds/ndds_c.h"
#include "NDDS_Obj.h"
#include "NDDS_PubFuncs.h"
#include "App_HB.h"

#include "errLogLib.h"

extern int DebugLevel;
extern NDDS_ID NDDS_Domain;


static pthread_t HBpThreadId;

/*
 * Create a Best Effort App Heart Beat Publication 
 *
 *   Author Greg Brissey 11-15-04
 */
NDDS_ID createAppHBPub(NDDS_ID nddsId, char *topic)
{
     int result;
     NDDS_ID pPubObj;
     char pubtopic[128];

    /* Build Data type Object for both publication and subscription to Expproc *
/
    /* ------- malloc space for data type object --------- */
    if ( (pPubObj = (NDDS_ID) malloc( sizeof(NDDS_OBJ)) ) == NULL )
      {
        return(NULL);
      }

    /* zero out structure */
    memset(pPubObj,0,sizeof(NDDS_OBJ));
    memcpy(pPubObj,nddsId,sizeof(NDDS_OBJ));
    strcpy(pPubObj->topicName,topic);
    pPubObj->pubThreadId = 424242;

    /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */
    getApp_HBInfo(pPubObj);

    DPRINT1(+1,"Create Pub topic: '%s' \n",pPubObj->topicName);
    createBEPublication(pPubObj);
    return(pPubObj);
}


/*
 *  Start the HeartBeat Thread 
 *
 *   Author: Greg Brissey   11/15/2004
 */
int startHBThread(char *AppName)
{
   int index,status;
   char pubHBtopic[128];
   NDDS_ID pAppHBPub;
   App_HB  *pAppHBIssue;
   void *HB_Pub_Routine (void *arg);
   NDDS_ID createAppHBPub(NDDS_ID nddsId, char *topic);

   sprintf(pubHBtopic,PUB_AppHB_TOPIC_FORMAT_STR,AppName);
   /* sprintf(subHBtopic,"%s/AppHB",cAppName); */
   DPRINT1(+1,"startHBThread(): '%s' \n", pubHBtopic);
   /* Publications used with multi thread need a thread ID 
        which can be just an ordinal number */

   pAppHBPub = createAppHBPub(NDDS_Domain,(char*) pubHBtopic);
   pAppHBIssue = pAppHBPub->instance;

   strncpy(pAppHBIssue->AppIdStr,AppName,16);
   pAppHBIssue->HBcnt = 0L;
   pAppHBIssue->ThreadId = 200;
   pAppHBIssue->AppId = 0;


   /* start thread */
   status = pthread_create ( (pthread_t *) &(pAppHBIssue->ThreadId),
            NULL, HB_Pub_Routine, (void*)pAppHBPub);
   if (status != 0)
       errLogSysQuit(LOGOPT,debugInfo,"Could not App HB thread\n");

   HBpThreadId = (pthread_t) pAppHBIssue->ThreadId;

   return(index);
}         
   
/*
 * The thread start routine for crew threads. Waits until "go"
 * command, processes work items until requested to shut down.
 */
void *HB_Pub_Routine (void *arg)
{
   unsigned long *pNodeHBcnt;
   NDDS_ID pAppHBPub;
   App_HB  *pAppHBIssue;
   RTINtpTime sleepTime = { 0, 0 };

   pAppHBPub = (NDDS_ID) arg;
   pAppHBIssue = pAppHBPub->instance;

   pNodeHBcnt = &(pAppHBIssue->HBcnt);

    pthread_detach((pthread_t) pAppHBIssue->ThreadId);   /* if thread terminates no need to join it to recover resources */
    
    RtiNtpTimePackFromMillisec(sleepTime, 0, 500);   /* 1/2 sec sleep */
    /*
     * Now, as long as there's work, keep doing it.
     */
    while (1) 
    {
        /* usec = .000001 sec */
	/* usleep(500000);  1/2 sec sleep, replaced with OS independent NDDS utility NddsUtilitySleep  */
#ifndef __INTERIX
        NddsUtilitySleep(sleepTime);   /* equiv to nanosleep() */
#else
        usleep(500000);
#endif
        (*pNodeHBcnt)++;
        nddsPublishData(pAppHBPub);
    }

    return NULL;
}


/* for termination of Expproc, cancell this thread so it doesn't core dump */
void HBExit(void)
{
   pthread_cancel(HBpThreadId);
}
#endif  /* RTI_NDDS_4x */
