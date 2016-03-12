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

  This Task  handles the FID Data UpLoad to the Host computer.
  It Waits on a msgQ which is written into via STM interrupt routine
and other functions. Then writes that FID Statblock & Data up to the
host via channel interface.

*/

/* #define NOT_THREADED */

#define THREADED_RECVPROC

#ifndef RTI_NDDS_4x

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
#include "expDoneCodes.h"
#include "errorcodes.h"
#include "taskPriority.h"


/* NDDS addition */
#include "ndds/ndds_c.h"
#include "NDDS_Obj.h"

#include "Cntlr_Comm.h"

/* NDDS Primary Domain */
extern NDDS_ID NDDS_Domain;

extern char hostName[80];

static NDDSSubscriber  DDRBarrierSubscriber;

static NDDS_ID pUploadSyncPub = NULL;
static NDDS_ID pUploadSyncSub[128];
static int numSubs = 0;

/*
 * this is how this works.
 * 1. This contruct and pub/sub is a mechanism to Sychronize DDRs upload Data.
 *    e.g. it is hoped to prevent a single DDR from getting more bandwidth than the others
 *          resulting in  many FID being transfer from one and few from another.
 *   This construct is similar to a standard thread 'barrier' however a normal thread barrier
 *   stops thread in the same process on the same node.
 *   This is different since each VxWorks task in on a separate controller board
 *   And thus the NDDS pub/sub for inter-controller commun.
 *
 *                                      Author Greg Brissey  8/18/05
 *
 */


int uploadSyncCount = 10;
int uploadSyncCntDown = 10;
static SEM_ID   pBarrierWaitSem;
static int      numActiveDDRs;
static int      numAtBarrier;


/*
typedef struct _DDRSynInfo_ {
	SEM_ID   pBarrierWaitSem;
        int      numActiveDDRs;
        int      numAtBarrier;
        int      ddrRdyCntDwn;
} DDRSynInfo;
*/

/*
 * Initialize Barrier semaphore and interval value
 *
 * Author Greg Brissey  8/18/05
 *
 */
initBarrier(int uploadcnt)
{
   if (pBarrierWaitSem == NULL)
       pBarrierWaitSem = semBCreate(SEM_Q_FIFO,SEM_EMPTY);

   uploadSyncCount = uploadSyncCntDown = uploadcnt;
}

/*
 *  Reset the counters and seamphore to their initial state
 *
 *                                      Author Greg Brissey  8/18/05
 *
 */
resetBarrier()
{
   numAtBarrier = 0;
   uploadSyncCntDown = uploadSyncCount;
   while (semTake(pBarrierWaitSem,NO_WAIT) != ERROR);
}


/*
 * barrierWait() 
 * This is call by the task that is to be stopped.
 * It is setup so that every uploadSyncCount (which is settable)
 * the Task wii be pended (via a semaphore) until all other 
 * participating tasks also reach this point.
 *
 * No tracking of a particular task is down, only the total number
 * of participating tasks is stored.
 * So if 4 task are participating then when four task have call 
 * barrierWait() the uploadSyncCount time. the semaphore will be
 * released so pended task may proceed.
 * 
 *
 *                                      Author Greg Brissey  8/18/05
 *
 */
int barrierWait()
{
   int status;
   uploadSyncCntDown--;
   /* DPRINT1(-1,"barrierWait: uploadSyncCntDown = %d \n",uploadSyncCntDown); */

   status = 0;
   if( uploadSyncCntDown <= 0)
   {
      /* DPRINT2(-1,"barrierWait: At Barrier: %d, NumActiveDDRs: %d \n",numAtBarrier,numActiveDDRs); */
      if(numAtBarrier < numActiveDDRs)
      {
          /* DPRINT(-1,"barrierWait: Send At Barrier, take semaphore & wait\n"); */
          sendDDRSync(CNTLR_CMD_DDR_AT_BARRIER, 0,0,0,NULL);  /* to all other DDRs that this is at the barrier */
          semTake(pBarrierWaitSem,WAIT_FOREVER);
          /* DPRINT(-1,"barrierWait: Got Semaphore \n"); */
          status = 42;
      }
      uploadSyncCntDown = uploadSyncCount;
   }
    return(status);
}

/*
 * show routine for this contruct
 *
 *                                      Author Greg Brissey  8/18/05
 *
 */
showBarrier()
{
   printf("DDR barrier: \n");
   printf("uploadSyncCount: %d, uploadSyncCntDown: %d\n",uploadSyncCount,uploadSyncCntDown);
   printf("numActiveDDRs: %d, numAtBarrier; %d\n",numActiveDDRs,numAtBarrier);
   semShow(pBarrierWaitSem,0);
   printf("\n\n");
}

/* =================================================================================== */
/* =================================================================================== */
/* ++++++++++++++++++++++++++  DDR Sync Pub/Sun Routines +++++++++++++++++++++++++++++ */
/* =================================================================================== */
/* =================================================================================== */

static SEM_ID pDDRSyncPubMutex = NULL;

/*
 *   The NDDS callback routine, the routine is call when an issue of the subscribed topic
 *   is delivered.
 *   called with the context of the NDDS task n_rtu7400
 *
 *
 *                                      Author Greg Brissey  8/18/05
 *
 */
RTIBool DDRSync_CommCallback(const NDDSRecvInfo *issue, NDDSInstance *instance,
                             void *callBackRtnParam)
{
    Cntlr_Comm *recvIssue;
 
    if (issue->status == NDDS_FRESH_DATA)
    {
        recvIssue = (Cntlr_Comm *) instance;
        DPRINT6(+1," DDRSync_Comm CallBack:  nddsTopic: '%s', cmd: %d, arg1: %d, arg2: %d, arg3: %d, crc: 0x%lx\n",
        issue->nddsTopic,recvIssue->cmd,recvIssue->arg1, recvIssue->arg2, recvIssue->arg3, recvIssue->crc32chksum);
        /* (*callbackFunc)(recvIssue); */
        switch(recvIssue->cmd)
        {
          case CNTLR_CMD_DDR_AT_BARRIER:
                numAtBarrier++;
                DPRINT3(-1," '%s: SyncBarrierParser:  numAtBarrier: %d , numActiveDDRs: %d\n",recvIssue->cntlrId,numAtBarrier,numActiveDDRs);
                if (numAtBarrier >= numActiveDDRs)
                {
                   numAtBarrier = 0;
                   DPRINT1(-1," '%s': SyncBarrierParser:  give Semaphore\n",recvIssue->cntlrId);
                   semGive(pBarrierWaitSem);
                }
                break;

           case CNTLR_CMD_SET_NUM_ACTIVE_DDRS:
                numActiveDDRs = recvIssue->errorcode;
                numAtBarrier = 0;
                uploadSyncCntDown = uploadSyncCount;
                while (semTake(pBarrierWaitSem,NO_WAIT) != ERROR);
                DPRINT3(-1,"'%s': SyncBarrierParser: setActive ddrs to: %d, clear numAtBarrier: %d\n",
                     recvIssue->cntlrId,numActiveDDRs,numAtBarrier);
                break;

           case CNTLR_CMD_SET_UPLINK_CNTDWN:
                uploadSyncCntDown = uploadSyncCount = recvIssue->errorcode;;
                DPRINT2(-1,"'%s': SyncBarrierParser: set uploadSyncCount to: %d\n", recvIssue->cntlrId,uploadSyncCount);
                break;
        }

    }
   return RTI_TRUE;
}

/*
 * Create a Exception Publication to communicate with the Cntrollers/Master
 *
 *
 *                                      Author Greg Brissey  8/18/05
 *
 */
NDDS_ID createDDRSyncCommPub(NDDS_ID nddsId, char *topic, char *cntlrName)
{
     int result;
     NDDS_ID pPubObj;
     char pubtopic[128];
     Cntlr_Comm  *issue;

    /* Build Data type Object for both publication and subscription to Expproc */
    /* ------- malloc space for data type object --------- */
    if ( (pPubObj = (NDDS_ID) malloc( sizeof(NDDS_OBJ)) ) == NULL )
      {  
        return(NULL);
      }  

    /* create the pub issue  Mutual Exclusion semaphore */
    pDDRSyncPubMutex = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE |
                                  SEM_DELETE_SAFE);


    /* zero out structure */
    memset(pPubObj,0,sizeof(NDDS_OBJ));
    memcpy(pPubObj,nddsId,sizeof(NDDS_OBJ));

    strcpy(pPubObj->topicName,topic);
    pPubObj->pubThreadId = 0xbadc0de;  /* DEFAULT_PUB_THREADID; taskIdSelf(); */
         
    /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */
    getCntlr_CommInfo(pPubObj);
         
    DPRINT2(+1,"Create Pub topic: '%s' for Cntlr: '%s'\n",pPubObj->topicName,cntlrName);
    createPublication(pPubObj);
    issue = (Cntlr_Comm  *) pPubObj->instance;
    strcpy(issue->cntlrId,cntlrName);   /* fill in the constant cntlrId string */
    return(pPubObj);
}        

/*
 * Create a Exception Subscription to communicate with the Controllers/Master
 *
 *
 *                                      Author Greg Brissey  8/18/05
 *
 */
NDDS_ID createDDRSyncCommSub(NDDS_ID nddsId, char *topic, char *multicastIP, void *callbackArg)
{

   NDDS_ID  pSubObj;

    /* Build Data type Object for both publication and subscription to Expproc */
    /* ------- malloc space for data type object --------- */
    if ( (pSubObj = (NDDS_ID) malloc( sizeof(NDDS_OBJ)) ) == NULL )
    {
        return(NULL);
    }
 
    /* zero out structure */
    memset(pSubObj,0,sizeof(NDDS_OBJ));
    memcpy(pSubObj,nddsId,sizeof(NDDS_OBJ));
 
    strcpy(pSubObj->topicName,topic);
 
    /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */
    getCntlr_CommInfo(pSubObj);
 
    pSubObj->callBkRtn = DDRSync_CommCallback;
    pSubObj->callBkRtnParam = NULL;
    if (multicastIP == NULL)
       pSubObj->MulticastSubIP[0] = 0;   /* use UNICAST */
    else
       strcpy(pSubObj->MulticastSubIP,multicastIP);

    DPRINT1(-1,"createDDRSyncCommSub: create subscript for topic: '%s'\n",pSubObj->topicName);
    createSubscription(pSubObj);
    return ( pSubObj );
}


/*
 * The Barrier involved nodes via NDDS uses this callback function to create Subscriptions to the
 * DDRs/Master *
 *
 *                                      Author Greg Brissey  8/18/05
 *
 */
NDDSSubscription DDRBarrier_CmdPatternSubCreate( const char *nddsTopic, const char *nddsType, 
                  void *callBackRtnParam) 
{ 
     NDDSSubscription pSub;
     DPRINT3(-1,"DDRBarrier_CmdPatternSubCreate(): Topic: '%s', Type: '%s', arg: 0x%lx\n",
		nddsTopic, nddsType, callBackRtnParam);
     pUploadSyncSub[numSubs] =  createDDRSyncCommSub(NDDS_Domain, (char*) nddsTopic, 
			DDRSYNC_COMM_MULTICAST_IP, (void *) callBackRtnParam );
     pSub = pUploadSyncSub[numSubs++]->subscription;
     return pSub;
}

/*
 *  creates a pattern subscriber, to dynamicly allow subscription creation
 *  as master/ddrs come on-line and publish to the DDR Barrier
 *
 *                                      Author Greg Brissey  8/18/05
 *
 */
DDRBarrierPubPatternSub(void *callback)
{
    DDRBarrierSubscriber = NddsSubscriberCreate(0);

    /* subscribe to any barrier publications from controllers */
    NddsSubscriberPatternAdd(DDRBarrierSubscriber,  
           DDRBARRIER_SUB_PATTERN_TOPIC_FORMAT_STR,  Cntlr_CommNDDSType , DDRBarrier_CmdPatternSubCreate, (void *)callback); 
}


/*
 * called by the master, it only need to publish, to inform how many participating controllers (DDRs)
 * there are for the barrier.
 *
 *                                      Author Greg Brissey  8/18/05
 *
 */
void initialMasterDDRSyncComm()
{
    pUploadSyncPub = createDDRSyncCommPub(NDDS_Domain,MASTERBARRIER_PUB_COMM_TOPIC_FORMAT_STR, hostName );
}

/*
 * DDRs will call this so they can participate in the barrier
 *
 *                                      Author Greg Brissey  8/18/05
 *
 */
void initialDDRSyncComm()
{
    char pubtopic[128];
    DDRBarrierPubPatternSub((void *) NULL);
    sprintf(pubtopic,DDRBARRIER_PUB_COMM_TOPIC_FORMAT_STR,hostName);
    DPRINT3(-1,"format: '%s', cntrlL '%s', topic: '%s'\n",DDRBARRIER_PUB_COMM_TOPIC_FORMAT_STR,hostName,pubtopic);
    pUploadSyncPub = createDDRSyncCommPub(NDDS_Domain,pubtopic, hostName );
}

/*
 * Used by All , to publish At Barrier message, etc.
 * A mulitcast publication thus all controllers will get this pub
 * even the one sending it
 *
 *
 *                                      Author Greg Brissey  8/18/05
 */
int sendDDRSync(int cmd, int arg1,int arg2,int arg3,char *strmsg)
{
   int status;
   Cntlr_Comm *issue;
/*
#ifdef INSTRUMENT
	wvEvent(EVENT_NEXUS_SENDEXCPT,NULL,NULL);
#endif
*/
   issue = pUploadSyncPub->instance;
   semTake(pDDRSyncPubMutex, WAIT_FOREVER);
      DPRINT(+1,"sendDDRSync: got Mutex\n");
      issue->cmd  = cmd;  /* atoi( token ); */
      issue->errorcode = arg1;
      issue->warningcode = arg2;
      issue->arg1 = arg3;

      if (strmsg != NULL)
      {
         int strsize = strlen(strmsg);
        if (strsize <= COMM_MAX_STR_SIZE)
          strncpy(issue->msgstr,strmsg,COMM_MAX_STR_SIZE);
        else
          DPRINT2(-1,"msg to long: %d, max: %d\n",strsize,COMM_MAX_STR_SIZE);
      }
      status = nddsPublishData(pUploadSyncPub); /* send exception to other controllers via NDDS publication */
      if ( status == -1)
        DPRINT(-1,"sendDDRSync: error in publishing\n");
/*
 *     Slows everything down alot !!
 *     status = nddsPublicationIssuesWait(pUploadSyncPub, 5, 0);
 *     if ( status == -1)
 *       DPRINT(-1,"sendDDRSync: error in waiting\n");
 */
      DPRINT(+1,"sendExcpetion: give Mutex\n");
   semGive(pDDRSyncPubMutex);

/*
#ifdef INSTRUMENT
	wvEvent(EVENT_NEXUS_SENDEXCPT_CMPLT,NULL,NULL);
#endif
*/
     return 0;
}

/* 
 *  Some simple test routines to try it out and confirm proper operation..
 *
 *                                      Author Greg Brissey  8/18/05
 *
 */

initmasterbar()
{
   initialMasterDDRSyncComm();
}

initbarcom()
{
  initBarrier(10);
  initialDDRSyncComm();
}

setcntdwn(int uploadcnt)
{
  sendDDRSync(CNTLR_CMD_SET_UPLINK_CNTDWN, uploadcnt,0,0,NULL);
}

setactddr(int nddrs)
{
    sendDDRSync(CNTLR_CMD_SET_NUM_ACTIVE_DDRS, nddrs,0,0,NULL);
}

tstbarrier()
{
    int i;
    int retval;
   int pTmpId, TmpPrior;
    int status;

   pTmpId = taskIdSelf();
   taskPriorityGet(pTmpId,&TmpPrior);
   taskPrioritySet(pTmpId,132);
   status = nddsPublicationIssuesWait(pUploadSyncPub, 5, 0);
   if ( status == -1)
        DPRINT(-1,"sendDDRSync: error in waiting\n");
    for(i=0; i < 100; i++)
    {
       /* DPRINT1(-1," %d: call barrier wait\n",i); */
       retval = barrierWait();
       if (retval == 42)
          DPRINT1(-1,">>>> %d: <<<<<<   returned from barrier wait\n",i);
    }
   taskPrioritySet(pTmpId,TmpPrior);
}

sndatbar()
{
  sendDDRSync(CNTLR_CMD_DDR_AT_BARRIER, 0,0,0,NULL);  /* to all other DDRs that this is at the barrier */
}

#endif  /* RTI_NDDS_4x */
