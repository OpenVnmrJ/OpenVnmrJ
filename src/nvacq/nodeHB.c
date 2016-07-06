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
#include <string.h>
#include <vxWorks.h>
#include <stdioLib.h>
#include <sysLib.h>

#include "instrWvDefines.h"
#include "taskPriority.h"

#include "logMsgLib.h"

#include "NDDS_Obj.h"
#include "NDDS_PubFuncs.h"
#include "NDDS_SubFuncs.h"

#ifndef RTI_NDDS_4x

#include "App_HB.h"

extern int DebugLevel;
extern NDDS_ID NDDS_Domain;


#define HOSTNAME_SIZE 80
extern char hostName[HOSTNAME_SIZE];

int HBPubRate = 60;

/* Console  status Pub */
NDDS_ID pNodeHBPub = NULL;
App_HB *pAppHBIssue = NULL;

/*
 *   Task waits for the semaphore to be given then publishes the Status
 *   This is done to avoid doing the publishing from within a NDDS task.
 *
 *     Author:  Greg Brissey 9/29/04
 */
pubHB()
{
   unsigned long *pNodeHBcnt;
   HBPubRate = calcSysClkTicks(1000); /* 1 sec update rate no matter what */
   pNodeHBcnt = &(pAppHBIssue->HBcnt);
   
   FOREVER
   {
       (*pNodeHBcnt)++;
       taskDelay(HBPubRate);  /* every 1 sec return and publish */
       if (pNodeHBPub != NULL)
          nddsPublishData(pNodeHBPub);
   }
}

 
void initialNodeHB()
{
   char topicName[80];
   int startNodeHBPub(int priority, int taskoptions, int stacksize);
   NDDS_ID createAppHBPub(NDDS_ID nddsId, char *topic);

   sprintf(topicName,PUB_NodeHB_TOPIC_FORMAT_STR,hostName);
   pNodeHBPub = createAppHBPub(NDDS_Domain,(char*) topicName);
   pAppHBIssue = pNodeHBPub->instance;

   strncpy(pAppHBIssue->AppIdStr,hostName,16);
   pAppHBIssue->HBcnt = 0L;
   pAppHBIssue->ThreadId = 200;
   pAppHBIssue->AppId = 0;
   startNodeHBPub(HEARTBEAT_TASK_PRIORITY, STD_TASKOPTIONS, STD_STACKSIZE);
}

startNodeHBPub(int priority, int taskoptions, int stacksize)
{
   
   if (taskNameToId("tAppHBPub") == ERROR)
      taskSpawn("tAppHBPub",priority,0,stacksize,pubHB,1,
						2,3,4,5,6,7,8,9,10);
}

killAppHBPub()
{
   int tid;
   if ((tid = taskNameToId("tAppHBPub")) != ERROR)
      taskDelete(tid);
}


/*
 * Create a Best Effort Publication Topic to communicate the Lock Status
 * Information
 *
 *					Author Greg Brissey 9-29-04
 */
NDDS_ID createAppHBPub(NDDS_ID nddsId, char *topic)
{
     int result;
     NDDS_ID pPubObj;
     char pubtopic[128];

    /* Build Data type Object for both publication and subscription to Expproc */
    /* ------- malloc space for data type object --------- */
    if ( (pPubObj = (NDDS_ID) malloc( sizeof(NDDS_OBJ)) ) == NULL )
      {  
        return(NULL);
      }  

    /* zero out structure */
    memset(pPubObj,0,sizeof(NDDS_OBJ));
    memcpy(pPubObj,nddsId,sizeof(NDDS_OBJ));

    strcpy(pPubObj->topicName,topic);
    pPubObj->pubThreadId = STATMON_TASK_PRIORITY+5; /* DEFAULT_PUB_THREADID; taskIdSelf(); */
         
    /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */
    getApp_HBInfo(pPubObj);
         
    DPRINT1(-1,"Create Pub topic: '%s' \n",pPubObj->topicName);
    createBEPublication(pPubObj);
    return(pPubObj);
}        

#endif  /* RTI_NDDS_4x */
