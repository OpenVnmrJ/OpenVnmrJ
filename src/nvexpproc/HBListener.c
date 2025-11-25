/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
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

#ifndef RTI_NDDS_4x
#include "errLogLib.h"
#include "ndds/ndds_c.h"
#include "NDDS_Obj.h"
#include "App_HB.h"
#include "HBListener.h"
#endif /* RTI_NDDS_4x */

#ifdef RTI_NDDS_4x
/* HB are not used any more even for 3X so for 4X except for this function everything is ifdef'd out */
/* are nodes HB being recieved, Note if HB are not subscribe too then return true */
int areNodesActive()
{
     return ( 1 );
}

#else /* RTI_NDDS_4x */

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

static int Master_HB = -1;
static int DDR_HB = -1;
static int totalHB_Subscriptions = 0;
static int currentHB_Subscriptions = 0;

/* --------------------------  Heart Beat Listener ---------------- */

typedef struct node_tag {
   int type;
   int number;
   int recvSub;
   char name[32];
 } node_t;
 
#define MAX_BRD_TYPE 10
static char *brdTypeStr[11] = { "master", "rf", "pfg", "gradient" , "lock", "ddr", "reserved1", "reserved2", 
				"Expproc", "Sendproc", "Recvproc" };
 
 
static int cntlrName2TypeNNum(char *id, int *type, int *num)
{
    char name[16], numstr[16];
    int i,j,k,len;
    len = strlen(id);
    j = k = 0;
    for(i=0; i< len; i++)
    {
       if (!isdigit(id[i]))
       { 
         name[j++] = id[i];
       } 
       else
       {
        numstr[k++] = id[i];
       } 
    }
    name[j] = numstr[k] = 0;
    if (k != 0)
      *num = atoi(numstr);
    else
      *num = 0;
        
    for (i=0; i < MAX_BRD_TYPE; i++)
    {
      if (strcmp(name,brdTypeStr[i]) == 0)
        break;
    }
    *type = i;
 
    return(0);
}



/*---------------------------------------------------------------------------------- */

RTIBool Node_HBCallback(const NDDSRecvInfo *issue, NDDSInstance *instance,
                             void *callBackRtnParam)
{

   App_HB *recvIssue;
   node_t *nodeId = (node_t*) callBackRtnParam;

   /*    possible status values:
     NDDS_FRESH_DATA, NDDS_DESERIALIZATION_ERROR, NDDS_UPDATE_OF_OLD_DATA,
     NDDS_NO_NEW_DATA, NDDS_NEVER_RECEIVED_DATA
   */
   if (issue->status == NDDS_FRESH_DATA)
   {
     recvIssue = (App_HB *) instance;
     DPRINT6(+5, "'%s': Node_HBCallback: '%s': received AppStr: '%s', HB cnt: %lu, ThreadId: %d, AppID: %d\n",
        ProcName,issue->nddsTopic,recvIssue->AppIdStr, recvIssue->HBcnt, recvIssue->ThreadId,recvIssue->AppId);

     DPRINT5(+4,"'%s': node: '%s', type: %d, number: %d, recvSub: %d\n",ProcName,
                nodeId->name,nodeId->type,nodeId->number,nodeId->recvSub);
     if (nodeId->recvSub == 0)
     {   
        nodeId->recvSub = 1;
        currentHB_Subscriptions++;
     }   
     
   }
   else if (issue->status == NDDS_NO_NEW_DATA)
   {
       DPRINT2(+5,"'%s': Node_HBCallback: Issue: '%s', Missed Deadline App/Node must be gone.\n", ProcName,issue->nddsTopic);
     DPRINT5(+4,"'%s': node: '%s', type: %d, number: %d, recvSub: %d\n",ProcName,
             nodeId->name,nodeId->type,nodeId->number,nodeId->recvSub);
     if (nodeId->recvSub == 1)
     {
        nodeId->recvSub = 0;
        currentHB_Subscriptions--;
     }
   }
   return RTI_TRUE;
}
 


#ifdef NOT_USED_ANYMORE
/*---------------------------------------------------------------------------------- */
 
*RTIBool App_HBCallback(const NDDSRecvInfo *issue, NDDSInstance *instance,
*                             void *callBackRtnParam)
*{
* 
*   App_HB *recvIssue;
*   /* cntlr_t *mine = (cntlr_t*) callBackRtnParam; */
* 
*   /*    possible status values:
*     NDDS_FRESH_DATA, NDDS_DESERIALIZATION_ERROR, NDDS_UPDATE_OF_OLD_DATA,
*     NDDS_NO_NEW_DATA, NDDS_NEVER_RECEIVED_DATA
*   */ 
*   if (issue->status == NDDS_FRESH_DATA)
*   {         
*     recvIssue = (App_HB *) instance;
*     DPRINT6(+3, "'%s': App_HBCallback: '%s': received AppStr: '%s', HB cnt: %lu, ThreadId: %d, AppID: %d\n",
*        ProcName,issue->nddsTopic,recvIssue->AppIdStr, recvIssue->HBcnt, recvIssue->ThreadId,recvIssue->AppId);
*   }
*   else if (issue->status == NDDS_NO_NEW_DATA)
*   {
*       DPRINT2(+1,"'%s': App_HBCallback: Issue: '%s', Missed Deadline App/Node must be gone.\n", ProcName,issue->nddsTopic);
*   }
*   return RTI_TRUE;
*}
*
*/*---------------------------------------------------------------------------------- */
*
*
*static int Master_HB = -1;
*static int DDR_HB = -1;
*static int masterCntlrType = 1;
*static int ddrCntlrType = 2;
* 
*RTIBool App_NodeCallback(const NDDSRecvInfo *issue, NDDSInstance *instance,
*                             void *callBackRtnParam)
*{
* 
*   App_HB *recvIssue;
*   int cntlrType;
* 
*   /*    possible status values:
*     NDDS_FRESH_DATA, NDDS_DESERIALIZATION_ERROR, NDDS_UPDATE_OF_OLD_DATA,
*     NDDS_NO_NEW_DATA, NDDS_NEVER_RECEIVED_DATA
*   */ 
*   cntlrType = *((int*)callBackRtnParam);
*   if (issue->status == NDDS_FRESH_DATA)
*   {         
*     recvIssue = (App_HB *) instance;
*     DPRINT6(+3, "'%s': App_NodeCallback: '%s': received AppStr: '%s', HB cnt: %lu, ThreadId: %d, AppID: %d\n",
*        ProcName,issue->nddsTopic,recvIssue->AppIdStr, recvIssue->HBcnt, recvIssue->ThreadId,recvIssue->AppId);
*     
*     if (cntlrType == 1)  /* 1 = master, 2 = ddr */
*     {
*        if (Master_HB < 1)
*        {
*            Master_HB = 1;
*            DPRINT2(+1,"'%s': App_NodeCallback: Issue: '%s', Is Back.\n", ProcName, issue->nddsTopic);
*        }
*     }
*     else if (cntlrType == 2)
*     {
*        if (DDR_HB < 1)
*        {
*            DDR_HB = 1;
*            DPRINT2(+1,"'%s': App_NodeCallback: Issue: '%s', Is Back.\n", ProcName, issue->nddsTopic);
*        }
*     }
*     else
*     {
*       DPRINT2(+1,"'%s': App_NodeCallback: Valid controller types 1 or 2, received: %d\n",ProcName, cntlrType);
*     }
*   }
*   else if (issue->status == NDDS_NO_NEW_DATA)
*   {
*      DPRINT2(+1,"'%s': App_NodeCallback: Issue: '%s', Missed Deadline, Node must be gone.\n", ProcName, issue->nddsTopic);
*     if (cntlrType == 1)  /* 1 = master, 2 = ddr */
*     {
*        Master_HB = -1;
*     }
*     else if (cntlrType == 2)
*     {
*        DDR_HB = -11;
*     }
*     else
*     {
*       DPRINT2(+1,"'%s': App_NodeCallback: Valid controller types 1 or 2, received: %d\n",ProcName, cntlrType);
*     }
*   }
*   return RTI_TRUE;
*}
*
#endif /* NOT_USED_ANYMORE */
      
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

    // pSubObj->MulticastSubIP[0] = 0;   /* use UNICAST */
    pSubObj->BE_UpdateMinDeltaMillisec = 1000;   /* max rate once a second */
    pSubObj->BE_DeadlineMillisec = 6000; /* no HB in 6 sec then it's gone.. */
    createBESubscription(pSubObj);
    DPRINT1(+1,"createAppHBSubscription(): subscription: 0x%lx\n",pSubObj->subscription);
  
    return( pSubObj );
}   

/*
 * NDDS will call this callback function to create Subscriptions for the Publication Pattern registered
 * below.
 * Downloader Threads to the Controller's Pub/Sub aimed at Sendproc
 *
 *                                      Author Greg Brissey 4-26-04
 */
NDDSSubscription App_HBPatternSubCreate( const char *nddsTopic, const char *nddsType,
                  void *callBackRtnParam)
{
     char cntrlName[128];
     char *chrptr;
     int threadIndex;
     NDDSSubscription pSub;
     NDDS_ID pHB_SubId;
     node_t *pNodeTag;
 
     DPRINT3(+1,"App_HBPatternSubCreate(): Topic: '%s', Type: '%s', arg: 0x%lx\n",
                nddsTopic, nddsType, callBackRtnParam);
     strncpy(cntrlName,nddsTopic,127);
     chrptr = strchr(cntrlName,'/');
     *chrptr = 0;
     if (strcmp(cntrlName,"Expproc") == 0)
     {
          DPRINT1(+1,"Not subscribing to HB for: '%s'\n",cntrlName);
          return NULL;
     }

     pNodeTag = (node_t *) malloc(sizeof(node_t));
     strcpy(pNodeTag->name,cntrlName);
     pNodeTag->recvSub = 0;
     cntlrName2TypeNNum(pNodeTag->name, &(pNodeTag->type), &(pNodeTag->number));

     /* must have a master for proper operation */
     if (pNodeTag->type == 0) 
          Master_HB = 1;

     /* must have a ddr for proper operation */
     if (pNodeTag->type == 5)
          DDR_HB = 1;

     pHB_SubId  = createAppHB_BESubscription((char*) nddsTopic,(void*) Node_HBCallback, pNodeTag);

     pSub = pHB_SubId->subscription;                                                               
     DPRINT2(+1,"App_HBPatternSubCreate(): Cntlr: '%s', subscription: 0x%lx\n",
                cntrlName,pHB_SubId->subscription);
 
    totalHB_Subscriptions++;
     return pSub;
}
 
/*
 *  Create a the Code DowndLoad pattern subscriber, to dynamicly allow subscription creation
 *  as controllers come on-line and publication to Sendproc download topic
 *
 *                                      Author Greg Brissey 2-24-05
 */
int HeartBeatPatternSub()
{
    /* MASTER_SUB_COMM_PATTERN_TOPIC_STR */
    if (CntlrSubscriber == NULL)
        CntlrSubscriber = NddsSubscriberCreate(NDDS_DOMAIN_NUMBER);

    /* All VxWorks nodes */                                     
    NddsSubscriberPatternAdd(CntlrSubscriber,
           nodeHB_PATTERN_FORMAT_STR, App_HBNDDSType , App_HBPatternSubCreate, NULL);
 
    /* the procs */
    NddsSubscriberPatternAdd(CntlrSubscriber,
           AppHB_PATTERN_FORMAT_STR, App_HBNDDSType , App_HBPatternSubCreate, NULL);

    return 0;
}


/* are nodes HB being recieved, Note if HB are not subscribe too then return true */
areNodesActive()
{
     int OK;
     if (CntlrSubscriber != NULL)
     {
        DPRINT5(+3,"'%s': areNodeActive(), Master_HB: %d, DDR_HB: %d,  totalHB_Subscriptions = %d, currentHB_Subscriptions = %d\n",
	    ProcName, Master_HB, DDR_HB, totalHB_Subscriptions, currentHB_Subscriptions);
        /* must have a master & a DDR controller */
         OK = ( (Master_HB == 1) && (DDR_HB == 1) && (totalHB_Subscriptions == currentHB_Subscriptions ) );
     }
     else
     {
        /* DPRINT1(+5,"'%s': areNodeActive(), HB PatternSub never initialized, return All Present as default\n", ProcName); */
        OK = 1;
     }
     return ( OK );
}



#ifdef NOT_USED_ANYMORE
/*
 * NDDS will call this callback function to create Subscriptions for the Publication Pattern registered
 * below.
 * Downloader Threads to the Controller's Pub/Sub aimed at Sendproc
 *
 *                                      Author Greg Brissey 4-26-04
 */
*NDDSSubscription App_HBPatternSubCreate( const char *nddsTopic, const char *nddsType,
*                  void *callBackRtnParam)
*{
*     char cntrlName[128];
*     char *chrptr;
*     int threadIndex;
*     NDDSSubscription pSub;
*     NDDS_ID pHB_SubId;
*     HB_PATTERN_ARG *pPatternArg;
*  
*     pPatternArg = (HB_PATTERN_ARG *) callBackRtnParam;
*     DPRINT3(+1,"App_HBPatternSubCreate(): Topic: '%s', Type: '%s', arg: 0x%lx\n",
*                nddsTopic, nddsType, callBackRtnParam);
*     strncpy(cntrlName,nddsTopic,127);
*     chrptr = strchr(cntrlName,'/');
*     *chrptr = 0;
*     DPRINT1(+1,"CntrlName: '%s'\n",cntrlName);
*     if (strcmp(cntrlName,"Expproc") == 0)
*     {
*          DPRINT1(+1,"CntrlName: '%s'\n",cntrlName);
*          return NULL;
*     }
*
*     if (pPatternArg == NULL)
*         pHB_SubId  = createAppHB_BESubscription((char*) nddsTopic,(void*)App_HBCallback, NULL);
*     else
*         pHB_SubId  = createAppHB_BESubscription((char*) nddsTopic,pPatternArg->callbackRoutine, pPatternArg->callbackParams);
*
*     pSub = pHB_SubId->subscription;
*     DPRINT2(+1,"App_HBPatternSubCreate(): Cntlr: '%s', subscription: 0x%lx\n",
*                cntrlName,pHB_SubId->subscription);
*  
*     return pSub;
*} 
*
*
*/*
* *  Create a the Code DowndLoad pattern subscriber, to dynamicly allow subscription creation
* *  as controllers come on-line and publication to Sendproc download topic
* *
* *                                      Author Greg Brissey 4-26-04
* */
*cntlrAppHB_PatternSub(HB_PATTERN_ARG *pPatternArg)
*{
*    if (CntlrSubscriber == NULL)
*        CntlrSubscriber = NddsSubscriberCreate(NDDS_DOMAIN_NUMBER);
*
*    NddsSubscriberPatternAdd(CntlrSubscriber,
*           AppHB_PATTERN_FORMAT_STR, App_HBNDDSType , App_HBPatternSubCreate, (void *)pPatternArg);
*}
*
*cntlrNodeHB_PatternSub(HB_PATTERN_ARG *pPatternArg)
*{
*    if (CntlrSubscriber == NULL)
*        CntlrSubscriber = NddsSubscriberCreate(NDDS_DOMAIN_NUMBER);
*
*    NddsSubscriberPatternAdd(CntlrSubscriber,
*           nodeHB_PATTERN_FORMAT_STR, App_HBNDDSType , App_HBPatternSubCreate, (void *)pPatternArg);
*}
* 
*int cntrlRequiredNodesSub()
*{
*   NDDS_ID pDDRId;
*   NDDS_ID pMasterId;
*   char subtopic[80];
*   sprintf(subtopic,SUB_NodeHB_TOPIC_FORMAT_STR,"master1");
*   pMasterId = createAppHB_BESubscription(subtopic, (void*) App_NodeCallback, (void *) &masterCntlrType);
*   sprintf(subtopic,SUB_NodeHB_TOPIC_FORMAT_STR,"ddr1");
*   pDDRId = createAppHB_BESubscription(subtopic, (void*) App_NodeCallback, (void *) &ddrCntlrType );
*   return 0;
*}
*
*areMasterAndDdrActive()
*{
*   return ( ( Master_HB > -1) && (DDR_HB > -1) );
*}
#endif

#endif  /* RTI_NDDS_4x */
