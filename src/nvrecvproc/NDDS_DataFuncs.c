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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>	/* nanosleep() */

#include "errLogLib.h"

#include "ndds/ndds_c.h"
#include "NDDS_Obj.h"
#include "NDDS_SubFuncs.h"
#include "NDDS_PubFuncs.h"
#ifndef RTI_NDDS_4x
#include "Data_UploadCustom3x.h"
#else /* RTI_NDDS_4x */
#include "Data_Upload.h"
#endif  /* RTI_NDDS_4x */
#include "sysUtils.h"

#ifdef RTI_NDDS_4x
#include "Data_UploadPlugin.h"
#include "Data_UploadSupport.h"
#endif  /* RTI_NDDS_4x */

#ifdef THREADED
// #include "App_HB.h"
#include "recvthrdfuncs.h"
#include "expDoneCodes.h"
#include "workQObj.h"
#include "rcvrDesc.h"
#include "memorybarrier.h"
#endif

/* #define DIAG_TIMESTAMP */


#define NDDS_DBUG_LEVEL 1
#define MULTICAST_ENABLE 1
#define MULTICAST_DISABLE 0

#ifdef THREADED
extern cntlr_crew_t TheRecvCrew;
extern membarrier_t TheMemBarrier;
#endif

extern RCVR_DESC ProcessThread;
extern void *createFidBlockHeader();
extern NDDS_ID createHBListener(char *subtopic, void *pParam );

/*
* NDDS_ISSUE_DESC_ID  pNddsIssueList[128];
* int totalIssues = 0;
*/

int dataSubPipeFd[2];

NDDS_ID NDDS_Domain;
NDDS_ID pDataSub,pDataReplyPub;

#define NDDS_DOMAIN_NUMBER 0

char *ConsoleHostName = "wormhole";

#ifndef RTI_NDDS_4x
NDDSSubscriber CntlrSubscriber;
#endif  /* RTI_NDDS_4x */

extern PIPE_STAGE_ID pProcThreadPipeStage;
extern int AbortFlag;

/*  int discardIssues = 0;   USE shared data struct */

RCVR_DESC_ID ddrSubList[64];
int numSubs = 0;

static NDDS_ID pDataSubList[64];
static int numDataSubs = 0;

static int numDataUploadReplySubcribers = 0;

#ifndef RTI_NDDS_4x
/*
     Reliable Publication Status call back routine.
     At present we use this to indicate if a subscriber has come or gone
*/
void Data_UploadPubStatusRtn(NDDSPublicationReliableStatus *status,
                                    void *callBackRtnParam)
{
      /* cntlr_t *mine = (cntlr_t*) callBackRtnParam; */
      RCVR_DESC_ID pRcvrDesc = (RCVR_DESC_ID) callBackRtnParam;
      switch(status->event)
      {  
      case NDDS_QUEUE_EMPTY:
        DPRINT1(0,"'%s': Queue empty\n",status->nddsTopic);
        break;
      case NDDS_LOW_WATER_MARK:
        DPRINT1(0,"'%s': Below low water mark - ",status->nddsTopic);
        DPRINT2(0,"Topic: '%s', UnAck Issues: %d\n",status->nddsTopic, status->unacknowledgedIssues);
        break;
      case NDDS_HIGH_WATER_MARK:
        DPRINT1(0,"'%s': Above high water mark - ",status->nddsTopic);
        DPRINT2(0,"Topic: '%s', UnAck Issues: %d\n",status->nddsTopic, status->unacknowledgedIssues);
        break;
      case NDDS_QUEUE_FULL:
        DPRINT1(0,"'%s': Queue full - ",status->nddsTopic);
        DPRINT2(0,"Topic: '%s', UnAck Issues: %d\n",status->nddsTopic, status->unacknowledgedIssues);
        break;
      case NDDS_SUBSCRIPTION_NEW:
        DPRINT2(0,"'%s': A new reliable subscription for Pub: '%s' has Appeared.\n",
			pRcvrDesc->cntlrId,status->nddsTopic);
        /* DPRINT1(1,"'%s': A new reliable subscription Appeared.\n",status->nddsTopic); */
        numDataUploadReplySubcribers++;
        break;
      case NDDS_SUBSCRIPTION_DELETE:
        DPRINT2(1,"'%s': A reliable subscription for Pub: '%s' has Disappeared.\n",
			pRcvrDesc->cntlrId,status->nddsTopic);
        /* DPRINT1(1,"'%s': A reliable subscription Disappeared.\n",status->nddsTopic); */
        numDataUploadReplySubcribers--;
        break;
      default:
 
                /* NDDS_BEFORERTN_VETOED
                   NDDS_RELIABLE_STATUS
                */
        break;
      }  
}


#ifndef THREADED

#else    /* THREADED */

/* fail-safe dummy buffer for NDDS issue */
/* char  failsafebuf[MAX_FIXCODE_SIZE+10]; */

#define CUSTOM_DESERIALIZER
#ifndef CUSTOM_DESERIALIZER 
#else  /* CUSTOM_DESERIALIZER */

/* ---------------------------------------------------------------------------------- */

RTIBool Data_UploadCallback(const NDDSRecvInfo *issue, NDDSInstance *instance,
                             void *callBackRtnParam)
{
 
   Data_Upload *recvIssue;
   char *CntlrName;
   RCVR_DESC_ID pRcvrDesc;
   WORKQ_ID   pWorkQId;
   WORKQ_ENTRY_ID pWorkQEntry;

   /* dereference pointers for ease */
   pRcvrDesc = (RCVR_DESC_ID) callBackRtnParam;
   CntlrName = pRcvrDesc->cntlrId;
  
   if (issue->status != NDDS_FRESH_DATA) 
   {
      switch(issue->status)
      {
	case NDDS_DESERIALIZATION_ERROR:
             errLogRet(ErrLogOp,debugInfo,"'%s': Data_UpldCallBack: NDDS NDDS_DESERIALIZATION_ERROR!!\n",
			CntlrName);
             break;
#ifdef XXX
	case NDDS_NEVER_RECEIVED_DATA:
             errLogRet(ErrLogOp,debugInfo,"'%s': Data_UpldCallBack: NDDS NDDS_NEVER_RECIEVED_DATA\n",
			CntlrName);
             break;
	case NDDS_NO_NEW_DATA:
             errLogRet(ErrLogOp,debugInfo,"'%s': Data_UpldCallBack: NDDS NDDS_NO_NEW_DATA\n",
			CntlrName);
             break;
	case NDDS_UPDATE_OF_OLD_DATA:
             errLogRet(ErrLogOp,debugInfo,"'%s': Data_UpldCallBack: NDDS NDDS_UPDATE_OF_OLD_DATA\n",
			CntlrName);
             break;
#endif
        default:
	     break;
      }
      return RTI_TRUE;
   }

/*
   if (discardIssues == 1)
   {
       DPRINT(+1,"Data_UpldCallBack: >>>>>>>>>>  Discard Issue <<<<<<<<<<<\n");
      return RTI_TRUE;
   }
*/

   /* dereference pointers for ease */
   pWorkQId = pRcvrDesc->pWorkQObj;
   pWorkQEntry = pRcvrDesc->activeWrkQEntry;

   recvIssue = (Data_Upload *) instance;

/*
   DPRINT6(-1,"'%s': Data_UpldCallBack() - elemId: %lu, totalBytes: %lu, dataOffset: 0x%lx, crc: 0x%lx, data.len: %lu\n",
		CntlrName,recvIssue->elemId, recvIssue->totalBytes, recvIssue->dataOffset, 
	        recvIssue->crc32chksum, recvIssue->data.len);
*/
   /* If No work Q Entry then just return */
   if (pWorkQEntry == NULL)
   {
      DPRINT(-1,"pWorkQEntry is NULL\n");
      return RTI_TRUE;
   }
     
   /* if flag == 1 then fid statblock and FID data have been serialized and completed */
   switch (recvIssue->deserializerFlag)
   {
        case NO_DATA: 
                  DPRINT1(+3,"'%s': Data_UpldCallBack() -  No Data \n",CntlrName);
               break;

        case ERROR_BLK:
                  DPRINT1(-1,"'%s': Data_UpldCallBack() - VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV\n",CntlrName);
                  DPRINT1(-1,"'%s': Data_UpldCallBack() - Exception FidStatBlk send to directly to processing stage\n",CntlrName);
                  if (rngBlkIsFull(ProcessThread.pInputQ))
                  {
                     errLogRet(ErrLogOp,debugInfo,"'%s': Data_UpldCallBack() - Going to block putting entry into ProcessThread pipe stage\n",CntlrName);
                  }
                  rngBlkPut(ProcessThread.pInputQ, &pWorkQEntry,1);
                  DPRINT1(-1,"'%s': Data_UpldCallBack() - ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n",CntlrName);
	       break;

        case COMPLETION_BLK:
                DPRINT1(-1,"'%s': Data_UpldCallBack() - VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV\n",CntlrName);
                DPRINT1(-1,"'%s': Data_UpldCallBack() - Completion FidStatBlk send to directly to processing stage\n",CntlrName);
                if (rngBlkIsFull(pRcvrDesc->pInputQ))
                {
                   errLogRet(ErrLogOp,debugInfo,"'%s': Data_UpldCallBack() - Going to block putting entry into pipe stage\n",CntlrName);
                }
                rngBlkPut(pRcvrDesc->pInputQ, &pWorkQEntry,1);
                DPRINT1(-1,"'%s': Data_UpldCallBack() - ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n",CntlrName);
	       break;

        case DATA_BLK:
             DPRINT1(-1,"'%s': Data_UpldCallBack() - VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV\n",CntlrName);
             DPRINT4(-1,"'%s': Data_UpldCallBack() - FID elemId: %lu, trueElemId: %lu, prev ElemId: %lu \n",
			CntlrName,pWorkQEntry->pFidStatBlk->elemId,pWorkQEntry->trueElemId,pRcvrDesc->prevElemId);
             DPRINT2(-1,"'%s': Data_UpldCallBack() - Fid Xfer Cmplt, Send workQEntry (0x%lx) to next stage\n",
			CntlrName,pWorkQEntry);
             /* send this workQ on to the next stage of the pipe line */
             /* this doesn't work for nfmod=0. need diff>nfmod not 1*/
             /*
             if ( pWorkQEntry->pFidStatBlk->elemId - pRcvrDesc->prevElemId > 1 )
             {
                    errLogRet(ErrLogOp,debugInfo,"'%s': Data_UpldCallBack() -!!!!!!!!!   prev ElemId: %lu, present ElemId: %lu, LOST: %lu !!!!!!!!!!! \n",
                       CntlrName,pRcvrDesc->prevElemId, pWorkQEntry->pFidStatBlk->elemId, 
                       ((pWorkQEntry->pFidStatBlk->elemId) - (pRcvrDesc->prevElemId)));
             }
             */
             pRcvrDesc->prevElemId = pWorkQEntry->pFidStatBlk->elemId;
             if (rngBlkIsFull(pRcvrDesc->pInputQ))
             {
                   errLogRet(ErrLogOp,debugInfo,"'%s': Data_UpldCallBack() - Going to block putting entry into pipe stage\n",CntlrName);
             }
             rngBlkPut(pRcvrDesc->pInputQ, &pWorkQEntry,1);

              /*         testing       */
/*
              if ((pRcvrDesc->prevElemId % 20) == 0)
              {
                 DPRINT2(-4,"'%s': Data_UpldCallBack() - FID elemId: %lu, mAsync called\n",
			CntlrName,pWorkQEntry->pFidStatBlk->elemId);
                 mAsync(pWorkQEntry->pInvar->fiddatafile->pMapFile);
              }
*/
             break;

   }


#ifdef DIAG_TIMESTAMP
   printTimeStamp(pRcvrDesc->cntlrId, "Data_UpldCallBack", 
                   &(pRcvrDesc->p4Diagnostics->tp),80000); 
#endif

   return RTI_TRUE;
}

#endif
#endif    /* THREADED */

#else /* RTI_NDDS_4x */

// #define NO_DATA 0
// #define ERROR_BLK 11
// #define COMPLETION_BLK 22
// #define DATA_BLK 42


/********************* 4x Discovery callbacks Added for user data: start *****************************/
void PublicationListener_on_requested_deadline_missed(
    void* listener_data,
    DDS_DataReader* reader,
    const struct DDS_RequestedDeadlineMissedStatus *status)
{
}

void PublicationListener_on_requested_incompatible_qos(
    void* listener_data,
    DDS_DataReader* reader,
    const struct DDS_RequestedIncompatibleQosStatus *status)
{
}

void PublicationListener_on_sample_rejected(
    void* listener_data,
    DDS_DataReader* reader,
    const struct DDS_SampleRejectedStatus *status)
{
}

void PublicationListener_on_liveliness_changed(
    void* listener_data,
    DDS_DataReader* reader,
    const struct DDS_LivelinessChangedStatus *status)
{
}

void PublicationListener_on_sample_lost(
    void* listener_data,
    DDS_DataReader* reader,
    const struct DDS_SampleLostStatus *status)
{
}

void PublicationListener_on_subscription_matched(
    void* listener_data,
    DDS_DataReader* reader,
    const struct DDS_SubscriptionMatchedStatus *status)
{
}

void PublicationListener_on_data_available( void* listener_data,
    DDS_DataReader* reader)
{
    char pubtopic[128];
    struct DDS_PublicationBuiltinTopicDataSeq data_seq = DDS_SEQUENCE_INITIALIZER;
    struct DDS_PublicationBuiltinTopicData *regular_data;
    struct DDS_SampleInfoSeq info_seq = DDS_SEQUENCE_INITIALIZER;
    DDS_ReturnCode_t retcode;
    int i,cntlrNum;
    struct DDS_PublicationBuiltinTopicDataDataReader* builtin_reader =
        (struct DDS_PublicationBuiltinTopicDataDataReader*) reader;
    struct DDS_SampleInfo *info_data = NULL;
    unsigned char *user_data = NULL;

    /* take the latest publication found */
    retcode = DDS_PublicationBuiltinTopicDataDataReader_take(
        builtin_reader, &data_seq, &info_seq, DDS_LENGTH_UNLIMITED,
        DDS_ANY_SAMPLE_STATE, DDS_ANY_VIEW_STATE, DDS_ANY_INSTANCE_STATE);

    DPRINT(3,"**************  Builtin Discovery Callback  ************************** \n");
    if (retcode != DDS_RETCODE_OK) {
        errLogRet(ErrLogOp,debugInfo,"*** Discvry Callback Error: failed to access data from the built-in reader\n");
        return;
    }

    for (i = 0; i < DDS_PublicationBuiltinTopicDataSeq_get_length(&data_seq); ++i)
    {
        if (DDS_SampleInfoSeq_get_reference(&info_seq, i)->valid_data)
        {
            regular_data = DDS_PublicationBuiltinTopicDataSeq_get_reference(&data_seq, i);
            DPRINT2(+3,"Built-in Reader: found publisher of topic \"%s\" and type "
                   "\"%s\".\n", regular_data->topic_name, regular_data->type_name);

            DPRINT2(+3,"Looking for type: '%s', topic: '%s'\n", Data_UploadTYPENAME,DATA_UPLOAD_M21TOPIC_STR);
            if ( (strcmp(Data_UploadTYPENAME, regular_data->type_name) == 0) &&
                 (strcmp(DATA_UPLOAD_M21TOPIC_STR, regular_data->topic_name) == 0)  )
            {
              int len, threadIndex,threadid;
              char cntlrName[128];
              char subtopic[128];
              RCVR_DESC_ID pRcvrDesc;
              extern RCVR_DESC_ID pTheRcvrDesc;
              /* see if there is user data */
              len = DDS_OctetSeq_get_length(&(regular_data->user_data.value));
              if (len == 0)
              {
                 DPRINT(+3,"--->  No user_data\n");
                 continue;
              }

              DDS_OctetSeq_to_array(&(regular_data->user_data.value),cntlrName,len);
              user_data = DDS_OctetSeq_get_reference(&(regular_data->user_data.value), 0);
              DPRINT3(+1,"'%s': Discvry Found type: '%s', topic: '%s'\n", cntlrName, Data_UploadTYPENAME,DATA_UPLOAD_M21TOPIC_STR);
              DPRINT2(+1,"'%s': Discvry User_data: '%s'\n",cntlrName,user_data);
              DPRINT2(+1,"'%s': Discvry CntrlName: '%s'\n",cntlrName,cntlrName);
              /* ------------------------------------------------ */
              cntlrNum = getCntlrNum(cntlrName);
              DPRINT2(+1,"'%s': Discvry Cntlr number: %d\n",cntlrName,cntlrNum);

              pRcvrDesc = (RCVR_DESC_ID) malloc(sizeof(RCVR_DESC));
              if (pRcvrDesc == NULL)
                 errLogSysQuit(LOGOPT,debugInfo,"'%s': Discvry Could not create Work Desc for: '%s'",cntlrName,cntlrName);

              memset(pRcvrDesc,0,sizeof(RCVR_DESC));

              // ddrSubList[numSubs++] = pRcvrDesc;
              ddrSubList[cntlrNum-1] = pRcvrDesc;    /* 1-64 */
              numSubs++;

              strncpy(pRcvrDesc->cntlrId,cntlrName,32);

              /* create a workQ manager object for the data work */
              pRcvrDesc->pWorkQObj = workQCreate((void*)pRcvrDesc, 40);  /* max queue depth */
              if (pRcvrDesc->pWorkQObj == NULL)
                       errLogSysQuit(LOGOPT,debugInfo,"'%s': Discvry Could not create Work Q Object; '%s'",cntlrName,pRcvrDesc->cntlrId);

              /* create a unique fid block header for this thread to use. */
              pRcvrDesc->pFidBlockHeader = createFidBlockHeader();
              if (pRcvrDesc->pFidBlockHeader == NULL)
                 errLogSysQuit(LOGOPT,debugInfo,"'%s': Discvry Could not create Fid Block Header: '%s'\n",cntlrName,pRcvrDesc->cntlrId);

              /* pRcvrDesc->pInputQ = rngBlkCreate(QUEUE_LENGTH,"inputQ", 1); */
              pRcvrDesc->pInputQ = rngBlkCreate(2048,"inputQ", 1);
              pRcvrDesc->pOutputQ = ProcessThread.pInputQ;
         
              threadid = addCntrlThread(pRcvrDesc, cntlrName);
              sprintf(pubtopic,HOST_PUB_UPLOAD_TOPIC_FORMAT_STR,cntlrName);
              /* in 4x there is only one subscription, a many 2 one type */
              pRcvrDesc->SubId = pTheRcvrDesc->SubId;
              createDataUploadPub(pRcvrDesc,pubtopic);
              // pDataSubList[cntlrNum] = pRcvrDesc->SubId;  doesn't appear to be used
              // numDataSubs++;
              /* internal deserializer pointer to RcvrDesc */
              // issue = pRcvrDesc->SubId->instance;
              // issue->pPrivateIssueData = (unsigned long) pRcvrDesc;
           }
       }
    }

    retcode = DDS_PublicationBuiltinTopicDataDataReader_return_loan
        (builtin_reader, &data_seq, &info_seq);
    if (retcode != DDS_RETCODE_OK) {
        printf("return loan error %d\n", retcode);
    }
}

createDiscoveryCallback(NDDS_ID pNDDS_Obj)
{
    DDS_Subscriber                             *builtinSubscriber;
    DDS_PublicationBuiltinTopicDataDataReader *builtinDataReader;
    struct DDS_DataReaderListener builtin_listener = DDS_DataReaderListener_INITIALIZER;

    /* get the built-in subscriber and data reader */
    builtinSubscriber = DDS_DomainParticipant_get_builtin_subscriber(pNDDS_Obj->pParticipant);
    DPRINT(0,"createDiscoveryCallback: get builtin_subscriber\n");
    if (builtinSubscriber == NULL) {
        printf("***Error: failed to create built-in subscriber\n");
        return 0;
    }
    DPRINT(0,"createDiscoveryCallback: get builtin_datareader \n");
    builtinDataReader = (DDS_PublicationBuiltinTopicDataDataReader *)DDS_Subscriber_lookup_datareader(builtinSubscriber, DDS_PUBLICATION_TOPIC_NAME);
    if (builtinDataReader == NULL) {
         DPRINT(-5,"***Error: failed to create built-in subscription data reader\n");
        return 0;
    }

    /* Setup data reader listener */
    builtin_listener.on_requested_deadline_missed  =
        PublicationListener_on_requested_deadline_missed;
    builtin_listener.on_requested_incompatible_qos =
        PublicationListener_on_requested_incompatible_qos;
    builtin_listener.on_sample_rejected =
        PublicationListener_on_sample_rejected;
    builtin_listener.on_liveliness_changed =
        PublicationListener_on_liveliness_changed;
    builtin_listener.on_sample_lost =
        PublicationListener_on_sample_lost;
    builtin_listener.on_subscription_matched =
        PublicationListener_on_subscription_matched;
    builtin_listener.on_data_available =
        PublicationListener_on_data_available;

     DPRINT(0,"set builtin_datareader listener\n");
    DDS_DataReader_set_listener((DDS_DataReader *)builtinDataReader, &builtin_listener, DDS_DATA_AVAILABLE_STATUS);
}
/********************* Add for user data: finish *****************************/


/* NDDS 4x Callback  */
#define MAX_RCVRDESC 128

static RCVR_DESC_ID  NddsRecvDescs[MAX_RCVRDESC];
static int numOfNddsNddsRecvDescs = 0;

handleData(Data_Upload *recvIssue, RCVR_DESC_ID pRcvrDesc)
{
   char *CntlrName;
   WORKQ_ID   pWorkQId;
   WORKQ_ENTRY_ID pWorkQEntry;
   int eventFlag;
   long dataLength;
   SHARED_DATA_ID pSharedData;
   int copyDataIntoWorkQBufs(RCVR_DESC_ID pRcvrDesc, Data_Upload *pIssue);
   int fillInWorkQ(Data_Upload *pIssue, WORKQ_ENTRY_ID pWorkQEntry);

   /* dereference pointers for ease */
   CntlrName = pRcvrDesc->cntlrId;

   /* dereference pointers for ease */
   pWorkQId = pRcvrDesc->pWorkQObj;
   pWorkQEntry = pRcvrDesc->activeWrkQEntry;

   copyDataIntoWorkQBufs(pRcvrDesc, recvIssue);

   pWorkQId = pRcvrDesc->pWorkQObj;
   pWorkQEntry = pRcvrDesc->activeWrkQEntry;

   eventFlag = fillInWorkQ(recvIssue, pWorkQEntry);

    dataLength = DDS_OctetSeq_get_length(&recvIssue->data);
    DPRINT4(+3,"'%s': handleData() - type: %d (DATAUPLOAD_FID=%d), dataLength: %ld\n",
              pRcvrDesc->cntlrId,recvIssue->type,DATAUPLOAD_FID,dataLength);
    if ( (recvIssue->type == DATAUPLOAD_FID) && ((recvIssue->dataOffset+dataLength) == recvIssue->totalBytes) )
    {
       eventFlag = DATA_BLK;
#ifdef DESERIALIZER_DEBUG
       DPRINT2(-2,"'%s': Data_Deserializer() - Fid Upload Complete: flag = %d \n",
		pRcvrDesc->cntlrId,recvIssue->deserializerFlag);
#endif
    }

    /* remove this lock and for aborts/errors you will coredump !! */
    pSharedData = (SHARED_DATA_ID) lockSharedData(&TheMemBarrier);
    if (pSharedData == NULL)
        errLogSysQuit(LOGOPT,debugInfo,"Deserializer: Could not lock memory barrier mutex");

      if ( pSharedData->discardIssues == 1)    /* error, start to discard any pubs receivered */
          eventFlag = NO_DATA;

    unlockSharedData(&TheMemBarrier);
 
    if ( eventFlag == ERROR_BLK)
    {
        /* this may not be neccessary (locking), however it's know to work, so for now we keep it 1/29/07 GMB */
        /* the only reason to not use lock is to improve performance, but not worth the chance at this point */
        pSharedData = (SHARED_DATA_ID) lockSharedData(&TheMemBarrier);
        if (pSharedData == NULL)
           errLogSysQuit(LOGOPT,debugInfo,"Deserializer: Could not lock memory barrier mutex");

         pSharedData->AbortFlag = pSharedData->discardIssues = 1;   /* error, start to discard any pubs receivered */

         unlockSharedData(&TheMemBarrier);

    }

   /* If No work Q Entry then just return */
   if (pWorkQEntry == NULL)
   {
      DPRINT(+2,"pWorkQEntry is NULL\n");
      return RTI_TRUE;
   }
     
   /* if flag == 1 then fid statblock and FID data have been serialized and completed */
   switch (eventFlag)
   {
        case NO_DATA: 
                  DPRINT1(+3,"'%s': Data_UpldCallBack() -  No Data \n",CntlrName);
               break;

        case ERROR_BLK:
                  DPRINT1(+2,"'%s': Data_UpldCallBack() - VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV\n",CntlrName);
                  DPRINT1(+2,"'%s': Data_UpldCallBack() - Exception FidStatBlk send to directly to processing stage\n",CntlrName);
                  if (rngBlkIsFull(ProcessThread.pInputQ))
                  {
                     errLogRet(ErrLogOp,debugInfo,"'%s': Data_UpldCallBack() - Going to block putting entry into ProcessThread pipe stage\n",CntlrName);
                  }
                  rngBlkPut(ProcessThread.pInputQ, &pWorkQEntry,1);
                  DPRINT1(+2,"'%s': Data_UpldCallBack() - ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n",CntlrName);
	       break;

        case COMPLETION_BLK:
                DPRINT1(+2,"'%s': Data_UpldCallBack() - VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV\n",CntlrName);
                DPRINT1(+2,"'%s': Data_UpldCallBack() - Completion FidStatBlk send to directly to processing stage\n",CntlrName);
                if (rngBlkIsFull(pRcvrDesc->pInputQ))
                {
                   errLogRet(ErrLogOp,debugInfo,"'%s': Data_UpldCallBack() - Going to block putting entry into pipe stage\n",CntlrName);
                }
                rngBlkPut(pRcvrDesc->pInputQ, &pWorkQEntry,1);
                DPRINT1(+2,"'%s': Data_UpldCallBack() - ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n",CntlrName);
	       break;

        case DATA_BLK:
             DPRINT1(+2,"'%s': Data_UpldCallBack() - VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV\n",CntlrName);
             DPRINT4(+2,"'%s': Data_UpldCallBack() - FID elemId: %lu, trueElemId: %lu, prev ElemId: %lu \n",
			CntlrName,(unsigned long) pWorkQEntry->pFidStatBlk->elemId,(unsigned long) pWorkQEntry->trueElemId,(unsigned long) pRcvrDesc->prevElemId);
             DPRINT2(+2,"'%s': Data_UpldCallBack() - Fid Xfer Cmplt, Send workQEntry (0x%lx) to next stage\n",
			CntlrName,(unsigned long) pWorkQEntry);
             /* send this workQ on to the next stage of the pipe line */
             /* this doesn't work for nfmod=0. need diff>nfmod not 1*/
             /*
             if ( pWorkQEntry->pFidStatBlk->elemId - pRcvrDesc->prevElemId > 1 )
             {
                    errLogRet(ErrLogOp,debugInfo,"'%s': Data_UpldCallBack() -!!!!!!!!!   prev ElemId: %lu, present ElemId: %lu, LOST: %lu !!!!!!!!!!! \n",
                       CntlrName,pRcvrDesc->prevElemId, pWorkQEntry->pFidStatBlk->elemId, 
                       ((pWorkQEntry->pFidStatBlk->elemId) - (pRcvrDesc->prevElemId)));
             }
             */
             pRcvrDesc->prevElemId = pWorkQEntry->pFidStatBlk->elemId;
             if (rngBlkIsFull(pRcvrDesc->pInputQ))
             {
                   errLogRet(ErrLogOp,debugInfo,"'%s': Data_UpldCallBack() - Going to block putting entry into pipe stage\n",CntlrName);
             }
             rngBlkPut(pRcvrDesc->pInputQ, &pWorkQEntry,1);

              /*         testing       */
/*
              if ((pRcvrDesc->prevElemId % 20) == 0)
              {
                 DPRINT2(-4,"'%s': Data_UpldCallBack() - FID elemId: %lu, mAsync called\n",
			CntlrName,pWorkQEntry->pFidStatBlk->elemId);
                 mAsync(pWorkQEntry->pInvar->fiddatafile->pMapFile);
              }
*/
             break;

   }
  
}


 /* 'NDDS_DataFuncs.c' line: 228, 'ddr1': Data_UpldCallBack() - VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV
 * 'ddr1': Data_UpldCallBack() - Reg. FidStatBlock: elemId: 1, trueElemId: 1, FID transfer @ addr: 0xfe81003c
 * 'ddr1': Data_UpldCallBack() - FID transfer: new addr: 0xfe81003c, bytes recv: 64000
 * 'ddr1': Data_UpldCallBack() - offset*len: 64000, totalBytes: 409600
 * 'ddr1': Data_UpldCallBack() - FID transfer: new addr: 0xfe81fa3c, bytes recv: 64000
 * 'ddr1': Data_UpldCallBack() - offset*len: 128000, totalBytes: 409600
 * 'ddr1': Data_UpldCallBack() - FID transfer: new addr: 0xfe82f43c, bytes recv: 64000
 * 'ddr1': Data_UpldCallBack() - offset*len: 192000, totalBytes: 409600
 * 'ddr1': Data_UpldCallBack() - FID transfer: new addr: 0xfe83ee3c, bytes recv: 64000
 * 'ddr1': Data_UpldCallBack() - offset*len: 256000, totalBytes: 409600
 * 'ddr1': Data_UpldCallBack() - FID transfer: new addr: 0xfe84e83c, bytes recv: 64000
 * 'ddr1': Data_UpldCallBack() - offset*len: 320000, totalBytes: 409600
 * 'ddr1': Data_UpldCallBack() - FID transfer: new addr: 0xfe85e23c, bytes recv: 64000
 * 'ddr1': Data_UpldCallBack() - offset*len: 384000, totalBytes: 409600
 * 'ddr1': Data_UpldCallBack() - FID transfer: new addr: 0xfe86dc3c, bytes recv: 25600
 * 'ddr1': Data_UpldCallBack() - offset*len: 409600, totalBytes: 409600
 * 'ddr1': Data_UpldCallBack() - Fid Xfer Cmplt, Send workQEntry (0x130e1c0) to next stage
 * 'ddr1': Data_UpldCallBack() - ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 */

int copyDataIntoWorkQBufs(RCVR_DESC_ID pRcvrDesc, Data_Upload *pIssue)
{
   // RCVR_DESC_ID pRcvrDesc;
   WORKQ_ID   pWorkQId;
   WORKQ_ENTRY_ID pWorkQEntry;
   SHARED_DATA_ID pSharedData;
   int retval,workQsAv;
   char *pXferData;
   long dataLength;


   // pRcvrDesc = (RCVR_DESC_ID) objp->pPrivateIssueData;

   pWorkQId = pRcvrDesc->pWorkQObj;
   pWorkQEntry = pRcvrDesc->activeWrkQEntry;

   retval = 0;

   if (pIssue->type == DATAUPLOAD_FIDSTATBLK)
   {
        long doneCode,fidBytes,elemID;
#ifdef DESERIALIZER_DEBUG
         DPRINT1(+2,"'%s': getNewDataValPtr() - Statblock Transfer\n",pRcvrDesc->cntlrId);
#endif
        /* get new workQ entry and set up the issue so NDDS load data into fidstatblock buffer */
        if ( workQGetWillPend(pWorkQId) )
        {
           errLogRet(ErrLogOp,debugInfo,"'%s': getNewDataValPtr() - Going to block getting workQ\n",pRcvrDesc->cntlrId);
        }
        pRcvrDesc->activeWrkQEntry = workQGet(pWorkQId);

        /* Copy into FidStatBlk WorkQ buffer */
        pXferData = DDS_OctetSeq_get_contiguous_bufferI(&pIssue->data);
        dataLength = DDS_OctetSeq_get_length(&pIssue->data);
        memcpy(pRcvrDesc->activeWrkQEntry->pFidStatBlk, pXferData, dataLength);

        // need to check for the EXP_FIDZERO_CMPLT doneCode within the StatBlk
        doneCode = ntohl(pRcvrDesc->activeWrkQEntry->pFidStatBlk->doneCode);
        // DPRINT2(-5,"copyDataIntoWorkQBufs:  doneCode: %ld, (0x%ld)\n", doneCode,doneCode);   //SEND_ZERO_FID DIAG
        if ( (doneCode & 0xFFFF) == EXP_FIDZERO_CMPLT)
        {
            elemID = ntohl(pRcvrDesc->activeWrkQEntry->pFidStatBlk->elemId);
            fidBytes = ntohl(pRcvrDesc->activeWrkQEntry->pFidStatBlk->dataSize);
            DPRINT2(+2,"EXP_FIDZERO_CMPLT:  ElemID: %ld, memset ZERO fidBytes: %ld\n", elemID, fidBytes);
            // Zero FID actually sends no data, thus fill the data with zeros
            memset(pRcvrDesc->activeWrkQEntry->pFidData, 0, fidBytes);
        }

#define INSTRUMENT
#ifdef INSTRUMENT

        workQsAv = numAvailWorkQs(pWorkQId);
     
        if (workQsAv < pRcvrDesc->p4Diagnostics->workQLowWaterMark)
           pRcvrDesc->p4Diagnostics->workQLowWaterMark = workQsAv;

#endif

    }
    else if (pIssue->type == DATAUPLOAD_FID)
    {
        if (pIssue->dataOffset == 0)
        {
           /* objp->data.val = getWorkQNewFidBufferPtr(pWorkQId,pRcvrDesc->activeWrkQEntry); */
           pWorkQEntry->FidStrtAddr = pRcvrDesc->activeWrkQEntry->pFidData;

           /* Copy beginning data into FidData WorkQ buffer */
           pXferData = DDS_OctetSeq_get_contiguous_bufferI(&pIssue->data);
           dataLength = DDS_OctetSeq_get_length(&pIssue->data);
           memcpy(pRcvrDesc->activeWrkQEntry->pFidData, pXferData, dataLength);
           // DPRINT1(-7,"DATAUPLOAD_FID:  copy fidBytes: %ld\n", dataLength);  // SEND_ZERO_FID DIAG

           // memcpy(pRcvrDesc->activeWrkQEntry->pFidData, 
           //        DDS_OctetSeq_get_contiguous_bufferI(&pIssue->data),
           //         DDS_OctetSeq_get_length(&pIssue->data));

           // pIssue->data.val = pRcvrDesc->activeWrkQEntry->pFidData;
           // pWorkQEntry->FidStrtAddr = pIssue->data.val;

#ifdef DESERIALIZER_DEBUG
           DPRINT2(+2,"'%s': getNewDataValPtr() - Start of FID Transfer: start addr: 0x%lx\n",
			pRcvrDesc->cntlrId,pIssue->data.val);
#endif
        }
        else
        {
#ifdef DESERIALIZER_DEBUG
           DPRINT3(+2,"'%s': getNewDataValPtr() - FID: offset: %ld, totalBytes: %ld\n",
			pRcvrDesc->cntlrId,pIssue->dataOffset,pIssue->totalBytes);
#endif
           // pIssue->data.val = (pWorkQEntry->FidStrtAddr + pIssue->dataOffset);

           /* Copy sucessive pieces of data into FidData WorkQ buffer */
           pXferData = DDS_OctetSeq_get_contiguous_bufferI(&pIssue->data);
           dataLength = DDS_OctetSeq_get_length(&pIssue->data);
           memcpy((pWorkQEntry->FidStrtAddr + pIssue->dataOffset), pXferData, dataLength);

           // memcpy((pWorkQEntry->FidStrtAddr + pIssue->dataOffset),
           //        DDS_OctetSeq_get_contiguous_bufferI(&pIssue->data),
           //        DDS_OctetSeq_get_length(&pIssue->data));
        }
   }

#ifdef XXXX
/*
*   pSharedData = (SHARED_DATA_ID) lockSharedData(&TheMemBarrier);
*   if (pSharedData == NULL)
*       errLogSysQuit(LOGOPT,debugInfo,"getNewDataValPtr: Could not lock memory barrier mutex");
*
*     /* just incase data file not open and NULL was returned */
*     if ( ( pIssue->data.val == NULL) || (pSharedData->discardIssues == 1) )
*        pIssue->data.val = failsafeAddr;
*
*    unlockSharedData(&TheMemBarrier);
*/
#endif 

   return retval;
}


int fillInWorkQ(Data_Upload *pIssue, WORKQ_ENTRY_ID pWorkQEntry)
{
      int retcode = 0;
      if (pIssue->type == DATAUPLOAD_FIDSTATBLK)
      {
         /*This is a Fid Stat Block */

        pWorkQEntry->statBlkCRC = pIssue->crc32chksum;

#ifdef LINUX
        FSB_CONVERT_NTOH( pWorkQEntry->pFidStatBlk );
#endif
        switch((pWorkQEntry->pFidStatBlk->doneCode & 0xFFFF))
         {
          /* any for the following case means there is no data following this statblock 
	   *	   and it should be pass on for processing 
           */
          case EXP_HALTED:
          case EXP_ABORTED:
          case HARD_ERROR:
                  pWorkQEntry->statBlkType = ERRSTATBLK;
#ifdef DESERIALIZER_DEBUG
                  DPRINT(+2,"fillInWorkQ: ERRSTATBLK\n");
#endif
                  retcode = ERROR_BLK;
                  break;

          case STOP_CMPLT:
          case SETUP_CMPLT:
          case WARNING_MSG:
                if ((pWorkQEntry->pFidStatBlk->doneCode & 0xFFFF) == WARNING_MSG)
                {
                   pWorkQEntry->statBlkType = WRNSTATBLK;
#ifdef DESERIALIZER_DEBUG
                  DPRINT(+2,"fillInWorkQ: WRNSTATBLK\n");
#endif
                }
                else
                {
                   pWorkQEntry->statBlkType = SU_STOPSTATBLK;
#ifdef DESERIALIZER_DEBUG
                  DPRINT(+2,"fillInWorkQ: SU_STOPSTATBLK\n");
#endif
                }
                 retcode = COMPLETION_BLK;
                 break;

         case EXP_FIDZERO_CMPLT:
                pWorkQEntry->statBlkType = FIDSTATBLK;
                pWorkQEntry->dataCRC = 0;
                retcode = DATA_BLK;   // set this to DATA_BLK since no data will actually be sent, just the stat blk.
                DPRINT(+2,"fillInWorkQ: EXP_FIDZERO_CMPLT\n");
                break;

            default:
                pWorkQEntry->statBlkType = FIDSTATBLK;
#ifdef DESERIALIZER_DEBUG
                 DPRINT(+2,"fillInWorkQ: FIDSTATBLK\n");
#endif
                retcode = NO_DATA;
                break;
         }
    }
    else if (pIssue->type == DATAUPLOAD_FID)
    {
        pWorkQEntry->dataCRC = pIssue->crc32chksum;
        retcode = NO_DATA;
    }
    return (retcode);
}


void Data_UploadCallback(void* listener_data, DDS_DataReader* reader)
{
   Data_Upload *recvIssue;
   char *CntlrName;
   RCVR_DESC_ID pRcvrDesc;

   struct DDS_SampleInfo* info = NULL;
   struct DDS_SampleInfoSeq info_seq = DDS_SEQUENCE_INITIALIZER;
   DDS_ReturnCode_t retcode;
   DDS_Boolean result;
   long i,numIssues;
   DDS_TopicDescription *topicDesc;


   struct Data_UploadSeq data_seq = DDS_SEQUENCE_INITIALIZER;
   Data_UploadDataReader *Data_Upload_reader = NULL;

   Data_Upload_reader = Data_UploadDataReader_narrow(pDataSub->pDReader);
   if ( Data_Upload_reader == NULL)
   {
        errLogRet(LOGIT,debugInfo,"DataReader narrow error\n");
        return;
   }

   topicDesc = DDS_DataReader_get_topicdescription(reader);
   DPRINT2(+2,"Data_UploadCallback: Type: '%s', Name: '%s'\n",
      DDS_TopicDescription_get_type_name(topicDesc), DDS_TopicDescription_get_name(topicDesc));
   while(1)
   {
        // Given DDS_HANDLE_NIL as a parameter, take_next_instance returns
        // a sequence containing samples from only the next (in a well-determined
        // but unspecified order) un-taken instance.
        retcode =  Data_UploadDataReader_take_next_instance(
            Data_Upload_reader,
            &data_seq, &info_seq, DDS_LENGTH_UNLIMITED,
            &DDS_HANDLE_NIL,
            DDS_ANY_SAMPLE_STATE, DDS_ANY_VIEW_STATE, DDS_ANY_INSTANCE_STATE);

        if (retcode == DDS_RETCODE_NO_DATA) {
//            errLogRet(LOGIT,debugInfo,"Data_UploadCallback: Take Instance gives DDS_RETCODE_NO_DATA, break out of callback while\n");
            break; // return;
        } else if (retcode != DDS_RETCODE_OK) {
                 errLogRet(LOGIT,debugInfo,"Data_UploadCallback: take next instance error %d\n",retcode);
        }
        numIssues =  Data_UploadSeq_get_length(&data_seq);
        DPRINT1(+2,"Data_UploadCallback: numIssues: %ld\n",numIssues);

        for (i=0; i < numIssues; i++)
        {
           info = DDS_SampleInfoSeq_get_reference(&info_seq, i);
           if (info->valid_data)
           {
              int index,ddrNum;
              recvIssue = (Data_Upload *) Data_UploadSeq_get_reference(&data_seq,i);
              ddrNum = recvIssue->key;

              // index = findCntlr(xxx,xxx);
              // pRcvrDesc = (RCVR_DESC_ID) listener_data;
              index = ddrNum;
              handleData(recvIssue,ddrSubList[index]);

           }
        }
        retcode = Data_UploadDataReader_return_loan( Data_Upload_reader,
                  &data_seq, &info_seq);
        DDS_SampleInfoSeq_set_maximum(&info_seq, 0);
   } // while
   return;
}

#endif  /* RTI_NDDS_4x */

/* ---------------------------------------------------------------------------------- */

#ifndef RTI_NDDS_4x

prtPubStatus(NDDS_ID pPub)
{
  int unackIssues;
  NDDSPublicationReliableStatus status;
  if (pPub != NULL)
  {
    unackIssues = NddsPublicationReliableStatusGet(pPub->publication, &status);
    printReliablePubStatus(&status);
  }
}

#endif  /* RTI_NDDS_4x */
/* 
   send a upload cmd issue to the DDR
*/
#ifdef RTI_NDDS_4x
int ddsPublishData(NDDS_ID pNDDS_Obj)
{
   DDS_ReturnCode_t result;
   DDS_InstanceHandle_t instance_handle = DDS_HANDLE_NIL;
   Data_UploadDataWriter *Data_Upload_writer = NULL;

   Data_Upload_writer = Data_UploadDataWriter_narrow(pNDDS_Obj->pDWriter);
   if (Data_Upload_writer == NULL) {
        errLogRet(LOGIT,debugInfo,"DataWriter narrow error\n");
        return -1;
   }

   result = Data_UploadDataWriter_write(Data_Upload_writer,
                pNDDS_Obj->instance,&instance_handle);
   if (result != DDS_RETCODE_OK) {
            errLogRet(LOGIT,debugInfo,"DataWriter write error: %d\n",result);
   }
   return 0;
}
#endif  /* RTI_NDDS_4x */

int send2DDR(NDDS_ID pPub, int cmd, int arg1, int arg2, int arg3)
{
     int result;
     Data_Upload *issue;
     RTINtpTime                maxWait    = {10,0};

#ifndef RTI_NDDS_4x
     if (DebugLevel > 2)
        prtPubStatus(pPub);
#endif  /* RTI_NDDS_4x */

     issue = (Data_Upload *) pPub->instance;
     issue->sn = cmd;
     issue->elemId = arg1;
     issue->totalBytes = arg2;
     issue->dataOffset = arg3;
     issue->crc32chksum = 0;
#ifndef RTI_NDDS_4x
     issue->data.len = 0;
     result = nddsPublishData(pPub);
#else /* RTI_NDDS_4x */
     DDS_OctetSeq_set_length(&issue->data,0);
     result = ddsPublishData(pPub);
#endif  /* RTI_NDDS_4x */
     return (result);   /* 0 = OK, -1 = failed */
}

int send2AllDDRs(int cmd, int arg1, int arg2, int arg3)
{
    int i;
    int result,endresult;
    endresult = 0;
    for(i=0; i < numSubs; i++)
    {
       DPRINT2(1,"send2DDR: %d, Topic: '%s'\n", i+1, ddrSubList[i]->PubId->topicName);
       result = send2DDR(ddrSubList[i]->PubId, cmd, arg1, arg2, arg3);
/*
       safesleep(250000);
       result = send2DDR(ddrSubList[i]->PubId, cmd, arg1, arg2, arg3);
*/
       if (result == -1)
       {
          errLogSysQuit(LOGOPT,debugInfo,"send2AllDDRs: send2DDR to '%s' failed\n", ddrSubList[i]->PubId->topicName);
	  endresult = -1;
       }
    }
    return (endresult);
}

/* 0 = successful, -1 = timed out */
int wait4Send2Cmplt(int timeoutsec,int qlevel)
{
    int i;
    int result = 0;
    for(i=0; i < numSubs; i++)
    {
       result = nddsPublicationIssuesWait(ddrSubList[i]->PubId, timeoutsec, qlevel);
    }  
   return result;
}

setDiscardIssues()
{
   SHARED_DATA_ID pSharedData;
   pSharedData = (SHARED_DATA_ID) lockSharedData(&TheMemBarrier);
    if (pSharedData == NULL)
       errLogSysQuit(LOGOPT,debugInfo,"setDiscardIssues: Could not lock memory barrier mutex");

       pSharedData->discardIssues = 1;

    unlockSharedData(&TheMemBarrier);

   /* discardIssues = 1;  */
}

void clearDiscardIssues()
{
   SHARED_DATA_ID pSharedData;
   pSharedData = (SHARED_DATA_ID) lockSharedData(&TheMemBarrier);
    if (pSharedData == NULL)
       errLogSysQuit(LOGOPT,debugInfo,"clearDiscardIssues: Could not lock memory barrier mutex");

       pSharedData->discardIssues = 0;

    unlockSharedData(&TheMemBarrier);

   /* discardIssues = 0;  */
}

/**************************************************************
*
*  initiateNDDS - Initialize a NDDS Domain for  communications
*
***************************************************************/
void initiateNDDS(int debuglevel)
{
    char localIP[80];
    /* NDDSDomainListener domainListener; */
    /* buildDomainLister(NDDSDomainListener *DListener); */
    /* NDDS_ID nddsCreate(int domain, int debuglevel, int multicast, char *nicIP) */
    /* buildDomainLister(&domainListener); */

#ifndef NO_MULTICAST
    NDDS_Domain = nddsCreate(NDDS_DOMAIN_NUMBER,debuglevel,MULTICAST_ENABLE,getHostIP(ConsoleHostName,localIP));
#else
    NDDS_Domain = nddsCreate(NDDS_DOMAIN_NUMBER,debuglevel,MULTICAST_DISABLE,getHostIP(ConsoleHostName,localIP));
#endif

    if (NDDS_Domain == NULL)
       errLogSysQuit(ErrLogOp,debugInfo,"Recvproc: NDDS DOmain Failed to be initialized!!!!!!\n");
}
 
#ifndef RTI_NDDS_4x
RTIBool subRemoteNewHook(const struct NDDSApplicationInfo *appInfo,
                                 const struct NDDSSubscriptionInfo *subInfo,
                                 void *userHookParam)
{
    DPRINT2(-1,"Recvproc: subRemoteNewHook: AppInfo: AppId: 0x%lx, hostId : 0x%lx\n",appInfo->appId, appInfo->hostId);
    DPRINT3(-1,"Recvproc: subRemoteNewHook: Sub - topic: '%s', type: '%s', ObjectId: 0x%lx\n",
		subInfo->nddsTopic,subInfo->nddsType,subInfo->objectId);
    return(RTI_TRUE);
}

RTIBool pubRemoteNewHook(const struct NDDSApplicationInfo *appInfo,
                                   const struct NDDSPublicationInfo *pubInfo,
                                   void *userHookParam)
{
    DPRINT2(-1,"Recvproc: pubRemoteNewHook: AppInfo: AppId: 0x%lx, hostId : 0x%lx\n",appInfo->appId, appInfo->hostId);
    DPRINT3(-1,"Recvproc: pubRemoteNewHook: Pub - topic: '%s', type: '%s', ObjectId: 0x%lx\n",
		pubInfo->nddsTopic,pubInfo->nddsType, pubInfo->objectId);
    return(RTI_TRUE);
}

void subRemoteDeleteHook(const struct NDDSApplicationInfo *appInfo,
                                      const struct NDDSSubscriptionInfo *subInfo,
                                      void *userHookParam)
{
      DPRINT2(-1,"Recvproc: subRemoteDeleteHook: AppInfo: AppId: 0x%lx, hostId : 0x%lx\n",appInfo->appId, appInfo->hostId);
      DPRINT3(-1,"Recvproc: subRemoteDeleteHook: Sub - topic: '%s', type: '%s', ObjectId: 0x%lx\n",
		subInfo->nddsTopic,subInfo->nddsType,subInfo->objectId);
}
  
 void pubRemoteDeleteHook(
         const struct NDDSApplicationInfo *appInfo,
         const struct NDDSPublicationInfo *pubInfo, void *userHookParam)
{
    DPRINT2(-1,"Recvproc: pubRemoteDeleteHook: AppInfo: AppId: 0x%lx, hostId : 0x%lx\n",appInfo->appId, appInfo->hostId);
    DPRINT3(-1,"Recvproc: pubRemoteDeleteHook: Pub - topic: '%s', type: '%s', ObjectId: 0x%lx\n",
		pubInfo->nddsTopic,pubInfo->nddsType,pubInfo->objectId);
}


buildDomainLister(NDDSDomainListener *DListener)
{
  if ( NddsDomainListenerDefaultGet(DListener) == RTI_TRUE) 
  {
	/* attach Sub new hook, pub new hook, Sub remote delete, Pub remote delete */
         DListener->onSubscriptionRemoteNew = subRemoteNewHook;
         DListener->onPublicationRemoteNew = pubRemoteNewHook;
	 DListener->onSubscriptionRemoteDelete = subRemoteDeleteHook;
         DListener->onPublicationRemoteDelete  = pubRemoteDeleteHook;
  }
}

#endif  /* RTI_NDDS_4x */

#ifndef THREADED

#else   /* THREADED */


int createDataUploadSub(RCVR_DESC_ID pRcvrDesc,char *subName)
{
    NDDS_ID  pSubObj;
    /* Build Data type Object for both publication and subscription to Expproc */
    /* ------- malloc space for data type object --------- */
    if ( (pSubObj = (NDDS_ID) malloc( sizeof(NDDS_OBJ)) ) == NULL )
    {
        return(-1);
    }
    pRcvrDesc->SubId = pSubObj;
     
    /* zero out structure */
    memset(pSubObj,0,sizeof(NDDS_OBJ));
    memcpy(pSubObj,NDDS_Domain,sizeof(NDDS_OBJ));

    strcpy(pSubObj->topicName,subName);
     
    /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */
    getData_UploadInfo(pSubObj);
 
#ifndef RTI_NDDS_4x
    pSubObj->callBkRtn = Data_UploadCallback;
    pSubObj->callBkRtnParam = pRcvrDesc;    /* thread crew struct */
#endif  /* RTI_NDDS_4x */
    pSubObj->MulticastSubIP[0] = 0;   /* use UNICAST */
    pSubObj->queueSize  = 32;
    pSubObj->BE_DeadlineMillisec = 120 * 1000;   /* 2 minutes, for subscript to be thought lost  */
#ifdef RTI_NDDS_4x
    pDataSub = pSubObj; 
    initSubscription(pSubObj);
    attachOnDataAvailableCallback(pSubObj, Data_UploadCallback, pRcvrDesc);
#endif  /* RTI_NDDS_4x */
    createSubscription(pSubObj);
#ifndef RTI_NDDS_4x
    DPRINT1(+1,"createCodeDownldSubscription(): subscription: 0x%lx\n",pSubObj->subscription);
#endif  /* RTI_NDDS_4x */
    return(0);
}

int createDataUploadPub(RCVR_DESC_ID pRcvrDesc,char *pubName)
{
     NDDS_ID pPubObj;
 
    /* Build Data type Object for both publication and subscription to Expproc */
    /* ------- malloc space for data type object --------- */
    if ( (pPubObj = (NDDS_ID) malloc( sizeof(NDDS_OBJ)) ) == NULL )
    {
        return(-1);
    }

    /* pCntlrThr->PubId = pPubObj; */
    pRcvrDesc->PubId = pPubObj;
 
    /* zero out structure */
    memset(pPubObj,0,sizeof(NDDS_OBJ));
    memcpy(pPubObj,NDDS_Domain,sizeof(NDDS_OBJ));
 
    strcpy(pPubObj->topicName,pubName);

    /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */
    getData_UploadInfo(pPubObj);
    pPubObj->pubThreadId = pRcvrDesc->threadID;   /* for mulit threaded apps */

    pPubObj->queueSize = 4;
    pPubObj->highWaterMark = 3;
    pPubObj->lowWaterMark = 1;
    pPubObj->AckRequestsPerSendQueue = 4;

#ifndef RTI_NDDS_4x
    pPubObj->pubRelStatRtn = Data_UploadPubStatusRtn;
    pPubObj->pubRelStatParam =  (void*) pRcvrDesc;
#else /* RTI_NDDS_4x */
    initPublication(pPubObj);
#endif  /* RTI_NDDS_4x */
    createPublication(pPubObj);
    return(0);
}

#ifndef RTI_NDDS_4x
/*
 * NDDS will call this callback function to create Subscriptions for the Publication Pattern registered
 * below.
 * Downloader Threads to the Controller's Pub/Sub aimed at Sendproc
 *
 *                                      Author Greg Brissey 4-26-04
 */
NDDSSubscription Data_UploadPatternSubCreate( const char *nddsTopic, const char *nddsType,
                  void *callBackRtnParam)
{
     int threadid;
     int cntrlNum;
     char cntrlName[128];
     char pubtopic[128];
     char subtopic[128];
     char subHBtopic[128];
     char *chrptr;
     NDDSSubscription pSub;
     RCVR_DESC_ID pRcvrDesc;
     Data_Upload *issue;
     
     DPRINT3(-1,"Cntlr_CodesDwnldPatternSubCreate(): Topic: '%s', Type: '%s', arg: 0x%lx\n",
                nddsTopic, nddsType, callBackRtnParam);
     strncpy(cntrlName,nddsTopic,127);
     chrptr = strchr(cntrlName,'/');
     *chrptr = 0;
     cntrlNum = getCntlrNum(cntrlName);
     DPRINT2(-1,"CntrlName: '%s', number: %d\n",cntrlName,cntrlNum);

     /* buildProcPipeStage(NULL); */


     pRcvrDesc = (RCVR_DESC_ID) malloc(sizeof(RCVR_DESC));
     if (pRcvrDesc == NULL)
        errLogSysQuit(LOGOPT,debugInfo,"Could not create Work Desc for: '%s'",cntrlName);

     memset(pRcvrDesc,0,sizeof(RCVR_DESC));

    ddrSubList[numSubs++] = pRcvrDesc;

    strncpy(pRcvrDesc->cntlrId,cntrlName,32);

    /* create a workQ manager object for the data work */
    /* pRcvrDesc->pWorkQObj = workQCreate((void*)pRcvrDesc, MAX_WORKQ_ENTRIES); */
    /* pRcvrDesc->pWorkQObj = workQCreate((void*)pRcvrDesc, 2048, 256 MB Max ); */
    /* pRcvrDesc->pWorkQObj = workQCreate((void*)pRcvrDesc, 40, 64 MB Max ); */
    pRcvrDesc->pWorkQObj = workQCreate((void*)pRcvrDesc, 40);  /* max queue depth */
    if (pRcvrDesc->pWorkQObj == NULL)
     errLogSysQuit(LOGOPT,debugInfo,"Could not creat Work Q Object; '%s'",pRcvrDesc->cntlrId);

    /* create a unique fid block header for this thread to use. */
    pRcvrDesc->pFidBlockHeader = createFidBlockHeader();
    if (pRcvrDesc->pFidBlockHeader == NULL)
       errLogSysQuit(LOGOPT,debugInfo,"Could not creat Fid Block Header: '%s'\n",pRcvrDesc->cntlrId);

     /* pRcvrDesc->pInputQ = rngBlkCreate(QUEUE_LENGTH,"inputQ", 1); */
     pRcvrDesc->pInputQ = rngBlkCreate(2048,"inputQ", 1);
     pRcvrDesc->pOutputQ = ProcessThread.pInputQ;

     threadid = addCntrlThread(pRcvrDesc, cntrlName);
     sprintf(pubtopic,HOST_PUB_UPLOAD_TOPIC_FORMAT_STR,cntrlName);
     sprintf(subtopic,HOST_SUB_UPLOAD_TOPIC_FORMAT_STR,cntrlName);
     sprintf(subHBtopic,SUB_NodeHB_TOPIC_FORMAT_STR,cntrlName);
     createDataUploadSub(pRcvrDesc,subtopic);
     createDataUploadPub(pRcvrDesc,pubtopic);
     /* pRcvrDesc->SubHBId = (NDDS_ID) createHBListener(subHBtopic,(void*) pRcvrDesc); */
     pRcvrDesc->SubHBId = createHBListener(subHBtopic, NULL);
     pDataSubList[cntrlNum] = pRcvrDesc->SubId;
     numDataSubs++;
     /* internal deserializer pointer to RcvrDesc */
     issue = pRcvrDesc->SubId->instance;
     issue->pPrivateIssueData = (unsigned long) pRcvrDesc;
     pSub = pRcvrDesc->SubId->subscription;
 
     return pSub;
}

#endif  /* RTI_NDDS_4x */

 
#ifdef xxxx
NDDSSubscription Data_UploadPatternSubCreate( const char *nddsTopic, const char *nddsType,
                  void *callBackRtnParam)
{
     char cntrlName[128];
     char *chrptr;
     int threadIndex;
     cntlr_t   *me;
     NDDSSubscription pSub;
     
     DPRINT3(-1,"Cntlr_CodesDwnldPatternSubCreate(): Topic: '%s', Type: '%s', arg: 0x%lx\n",
                nddsTopic, nddsType, callBackRtnParam);
     /* DPRINT2(-1,"callbackParam: 0x%lx, callBackRtnParam: 0x%lx\n",&callbackParam,callBackRtnParam); */
     strncpy(cntrlName,nddsTopic,127);
     chrptr = strchr(cntrlName,'/');
     *chrptr = 0;
     DPRINT1(-1,"CntrlName: '%s'\n",cntrlName);
     threadIndex = addCntrlThread(&TheRecvCrew,cntrlName);
     me = &(TheRecvCrew.crew[threadIndex]);
     DPRINT3(-1,"Cntlr_CodeDwnldPatternSubCreate(): threadIndex: %d, Cntlr: '%s', subscription: 0x%lx\n",
                threadIndex,me->cntlrId,me->SubId->subscription);
 
     pSub = me->SubId->subscription;
 
     return pSub;
}
#endif

#ifdef XXXXX
NDDSSubscription Data_UploadPatternSubCreate( const char *nddsTopic, const char *nddsType,
                  void *callBackRtnParam)
{
     char pubtopic[128];
     char subtopic[128];
     char cntrlName[128];
     char *chrptr;
     int threadIndex;
     cntlr_t   *me;
     NDDSSubscription pSub;
     NDDS_ISSUE_DESC_ID pIssues;
     
     DPRINT3(-1,"Cntlr_CodesDwnldPatternSubCreate(): Topic: '%s', Type: '%s', arg: 0x%lx\n",
                nddsTopic, nddsType, callBackRtnParam);
     /* DPRINT2(-1,"callbackParam: 0x%lx, callBackRtnParam: 0x%lx\n",&callbackParam,callBackRtnParam); */
     strncpy(cntrlName,nddsTopic,127);
     chrptr = strchr(cntrlName,'/');
     *chrptr = 0;
     DPRINT1(-1,"CntrlName: '%s'\n",cntrlName);

     pIssues = (NDDS_ISSUE_DESC_ID) malloc(sizeof(NDDS_ISSUE_DESC));

     strcpy(pNddsIssuelist[totalIssue]->cntlrId,cntrlName,32);

     /* threadIndex = addCntrlThread(&TheRecvCrew,cntrlName); */
     /* me = &(TheRecvCrew.crew[threadIndex]); */
     sprintf(pubtopic,HOST_PUB_UPLOAD_TOPIC_FORMAT_STR,cntlrName);
     sprintf(subtopic,HOST_SUB_UPLOAD_TOPIC_FORMAT_STR,cntlrName);
     createDataUploadSub(pIssues,subtopic);
     createDataUploadPub(pIssues,pubtopic);
     DPRINT2(-1,"Cntlr_CodeDwnldPatternSubCreate(): Cntlr: '%s', subscription: 0x%lx\n",
                me->cntlrId,pIssues->pSubId->subscription);
 
     pSub = pIssues->pSubId->subscription;
 
     pNddsIssuelist[totalIssue++] = pIssues;
     return pSub;
}
#endif

#endif  /* THREADED */

int getCntlrNum(char *id)
{
    char numstr[16];
    int i,k,len;
    int num;
    len = strlen(id);
    k = 0;
    for(i=0; i< len; i++)
    {
       if (isdigit(id[i]))
       { 
        numstr[k++] = id[i];
       } 
    }   
    numstr[k] = 0;
    num = atoi(numstr);
    return(num);
}



int callbackParam;

#ifndef RTI_NDDS_4x
/*
 *  Create a the Code DowndLoad pattern subscriber, to dynamicly allow subscription creation
 *  as controllers come on-line and publication to Recvproc data upload topic
 *
 *                                      Author Greg Brissey 8-05-04
 */
cntlrDataUploadPatternSub()
{
    /* MASTER_SUB_COMM_PATTERN_TOPIC_STR */
    CntlrSubscriber = NddsSubscriberCreate(NDDS_DOMAIN_NUMBER);

    /* master subscribe to any publications from controllers */
    NddsSubscriberPatternAdd(CntlrSubscriber,
           "*/h/upload/strm",  Data_UploadNDDSType , Data_UploadPatternSubCreate, (void *)&callbackParam);
}
#else /* RTI_NDDS_4x */

RCVR_DESC_ID pTheRcvrDesc;

cntlrDataUploadPatternSub()
{
  createDiscoveryCallback(NDDS_Domain);
  
  pTheRcvrDesc = (RCVR_DESC_ID) malloc(sizeof(RCVR_DESC));
  if (pTheRcvrDesc == NULL)
     errLogSysQuit(LOGOPT,debugInfo,"cntlrDataUploadPatternSub: Could not create Work Desc\n");

  memset(pTheRcvrDesc,0,sizeof(RCVR_DESC));

  createDataUploadSub(pTheRcvrDesc,DATA_UPLOAD_M21TOPIC_STR);
}
 
#endif  /* RTI_NDDS_4x */

#ifndef  THREADED

void initiateDataPub()
{
    char pubTopic[30];

    sprintf(pubTopic,(const char*)HOST_PUB_UPLOAD_TOPIC_FORMAT_STR,"ddr1");

    pDataReplyPub = createDataUploadPub(pubTopic);
    /* pDataReplyPub = createDataUploadPub(HOST_PUB_UPLOAD_MCAST_TOPIC_FORMAT_STR); */
}

void initiateDataSub()
{
    char subTopic[30];

    sprintf(subTopic,(const char*)HOST_SUB_UPLOAD_TOPIC_FORMAT_STR,"ddr1");

   pDataSub = createDataUploadSub(subTopic);
   
}

#else

void initiateDataPub()
{
}

#endif  /* THREADED */
 

void shutdownComm()
{
#ifndef RTI_NDDS_4x
   RTINtpTime sleepTime = { 0, 0 };

   RtiNtpTimePackFromMillisec(sleepTime, 0, 400);   /* 400 millisec sleep */

#else /* RTI_NDDS_4x */

   struct DDS_Duration_t sleepTime = { 0, 0 };
   sleepTime.nanosec = 400000000;  /* 400 millisec sleep */
  
#endif  /* RTI_NDDS_4x */

/* destroying the domain is enough */
/*
   for (i=0; i < numDataSubs; i++)
   {
      if (pDataSubList[i] != NULL)
      {
        DPRINT1(+1,"Recvproc: Destroy Sub: 0x%lx\n",pDataSubList[i]);
        nddsSubscriptionDestroy(pDataSubList[i]);
      }
   }
   nddsPublicationDestroy(pDataReplyPub);
*/

   if (NDDS_Domain != NULL)
   {
#ifndef RTI_NDDS_4x
      DPRINT1(+1,"Recvproc: Destroy Domain: 0x%lx\n",NDDS_Domain->domain);
      NddsDestroy(NDDS_Domain->domain);
      NddsUtilitySleep(sleepTime);   /* equiv to nanosleep() */
#else /* RTI_NDDS_4x */
      NDDS_Shutdown(NDDS_Domain);
      NDDS_Utility_sleep(&sleepTime);
#endif  /* RTI_NDDS_4x */
      /* usleep(400000); */  /* 400 millisec sleep */
   }

}

int printTimeStamp(char *cntlrId, char *where, struct timeval *pPrevTS, int threshold)
{
    struct timeval tp;                                                     
    long sdif,usdif;
    
    if ((cntlrId == NULL) || (where == NULL) || (pPrevTS == NULL))
       return -1;

    gettimeofday(&tp,NULL);

    sdif = tp.tv_sec - pPrevTS->tv_sec;
    usdif = tp.tv_usec - pPrevTS->tv_usec;
    if (usdif < 0)
    {
       sdif--;
       usdif += 1000000;
    }
    usdif += (sdif * 1000000);
    if (usdif >= threshold)
    {
      DPRINT5(-4,"'%s': %s: TS: delta: %ld usec ; %lu sec, %lu usec\n",
        cntlrId, where, usdif, tp.tv_sec,tp.tv_usec);
    }
    memcpy(pPrevTS,&tp,sizeof(tp));
    return ( usdif );
}
