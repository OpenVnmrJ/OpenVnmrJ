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
#ifndef RTI_NDDS_4x

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdarg.h>
#ifndef LINUX
#include <thread.h>
#endif
#include <pthread.h>

#include "errLogLib.h"
#include "ndds/ndds_c.h"
#include "NDDS_Obj.h"
#include "App_HB.h"

#include "sockets.h"
#include "msgQLib.h"

#define TRUE 1
#define FALSE 0
#define FOR_EVER 1

/* hostname (i.e. NIC) attached to console */
extern char ConsoleNicHostname[];

extern char ProcName[256];

#define NDDS_DBUG_LEVEL 1
#define MULTICAST_ENABLE 1
#define NDDS_DOMAIN_NUMBER 0

NDDS_ID pMonitorPub, pMonitorSub;

static NDDSSubscriber CntlrSubscriber = NULL;

extern NDDS_ID NDDS_Domain;

/*---------------------------------------------------------------------------------- */
/*---------------------------------------------------------------------------------- */

static int DDR_HB = -1;
static int ddrCntlrType = 1;
 
RTIBool App_NodeCallback(const NDDSRecvInfo *issue, NDDSInstance *instance,
                             void *callBackRtnParam)
{
 
   App_HB *recvIssue;
   int cntlrType;
 
   /*    possible status values:
     NDDS_FRESH_DATA, NDDS_DESERIALIZATION_ERROR, NDDS_UPDATE_OF_OLD_DATA,
     NDDS_NO_NEW_DATA, NDDS_NEVER_RECEIVED_DATA
   */ 
   /* cntlrType = *((int*)callBackRtnParam); */
   if (issue->status == NDDS_FRESH_DATA)
   {         
     recvIssue = (App_HB *) instance;
     DPRINT6(+3, "'%s': App_NodeCallback: '%s': received AppStr: '%s', HB cnt: %lu, ThreadId: %d, AppID: %d\n",
        ProcName,issue->nddsTopic,recvIssue->AppIdStr, recvIssue->HBcnt, recvIssue->ThreadId,recvIssue->AppId);
     
      if (DDR_HB < 1)
      {
          DDR_HB = 1;
          DPRINT2(+1,"'%s': App_NodeCallback: Issue: '%s', Is Back.\n", ProcName, issue->nddsTopic);
      }
   }
   else if (issue->status == NDDS_NO_NEW_DATA)
   {
      DPRINT2(+1,"'%s': App_NodeCallback: Issue: '%s', Missed Deadline, Node must be gone.\n", ProcName, issue->nddsTopic);
      DDR_HB = -1;
   }
   return RTI_TRUE;
}
/*---------------------------------------------------------------------------------- */
      

/* future call? int createAppHB_BESubscription(cntlr_t *pCntlrThr,char *subName) */
NDDS_ID createAppHB_BESubscription(char *subName, void *callbackRoutine, void *callBackRtnParam)
{
    NDDS_ID pSubObj;

    /* Build Data type Object for both publication and subscription to Expproc */
    /* ------- malloc space for data type object --------- */
    if ( (pSubObj = (NDDS_ID) malloc( sizeof(NDDS_OBJ)) ) == NULL )
    {
        return(NULL);
    }

    /* zero out structure */
    memset(pSubObj,0,sizeof(NDDS_OBJ));
    memcpy(pSubObj,NDDS_Domain,sizeof(NDDS_OBJ));

    strcpy(pSubObj->topicName,subName);

    /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */
    getApp_HBInfo(pSubObj);

    pSubObj->callBkRtn = (NddsCallBkRtn) callbackRoutine;
    pSubObj->callBkRtnParam = (void*) callBackRtnParam;

#ifndef NO_MULTICAST
     strcpy(pSubObj->MulticastSubIP,APP_HB_MULTICAST_IP);
#else
     pSubObj->MulticastSubIP[0] = 0;   /* use UNICAST */
#endif

    pSubObj->BE_UpdateMinDeltaMillisec = 1000;   /* max rate once a second */
    pSubObj->BE_DeadlineMillisec = 6000; /* no HB in 6 sec then it's gone.. */
    createBESubscription(pSubObj);
    DPRINT1(+1,"createAppHBSubscription(): subscription: 0x%lx\n",pSubObj->subscription);
  
    return( pSubObj );
}   


/*
 *  Create a the Code DowndLoad pattern subscriber, to dynamicly allow subscription creation
 *  as controllers come on-line and publication to Sendproc download topic
 *
 *                                      Author Greg Brissey 4-26-04
 */
/*cntlrNodeHB_PatternSub()
 *{
 *   if (CntlrSubscriber == NULL)
 *      CntlrSubscriber = NddsSubscriberCreate(NDDS_DOMAIN_NUMBER);
 *
 *    NddsSubscriberPatternAdd(CntlrSubscriber,
 *           nodeHB_PATTERN_FORMAT_STR, App_HBNDDSType , App_HBPatternSubCreate, (void *)NULL);
 *}
 */

NDDS_ID createHBListener(char *subtopic, void *pParam )
{
   NDDS_ID pDDRId;
   pDDRId = createAppHB_BESubscription(subtopic, (void*) App_NodeCallback, (void *) pParam );
   return (pDDRId);
}
 
startDDR_HBListener()
{
   NDDS_ID pDDRId;
   char subtopic[80];
   sprintf(subtopic,SUB_NodeHB_TOPIC_FORMAT_STR,"ddr1");
   pDDRId = createAppHB_BESubscription(subtopic, (void*) App_NodeCallback, (void *) &ddrCntlrType );
}

isDDRActive()
{
   return ( (DDR_HB > -1) );
}
#endif  /* RTI_NDDS_4x */
