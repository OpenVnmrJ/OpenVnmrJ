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
/*
#include <sys/types.h>
#include <signal.h>
#include <sys/socket.h>
*/

#include <fcntl.h>


#include <errno.h>

#define  DELIMITER_2      '\n'


#include "errLogLib.h"

#include "ndds/ndds_c.h"
#include "NDDS_Obj.h"
#include "NDDS_SubFuncs.h"
#include "NDDS_PubFuncs.h"
#include "Codes_Downld.h"
#ifndef RTI_NDDS_4x
#include "App_HB.h"
#endif  /* RTI_NDDS_4x */

#ifdef RTI_NDDS_4x
#include "Codes_DownldPlugin.h"
#include "Codes_DownldSupport.h"
// #include "App_HBPlugin.h"
// #include "App_HBSupport.h"
#endif /* RTI_NDDS_4x */

#include "crc32.h"
#include "threadfuncs.h"
#include "nddsbufmngr.h"

#ifndef DEBUG_HEARTBEAT
#define DEBUG_HEARTBEAT	(9)
#endif

#define TRUE 1
#define FALSE 0
#define FOR_EVER 1
#define HEARTBEAT_TIMEOUT_INTERVAL	(2.8)

#define NDDS_DOMAIN_NUMBER 0
#define  MULTICAST_ENABLE  1  /* enable multicasting for NDDS */
#define  MULTICAST_DISABLE 0  /* disable multicasting for NDDS */
#define  NDDS_DBUG_LEVEL 3
static char  *ConsoleHostName = "wormhole";
int createCodeDownldPublication(cntlr_t *pCntlrThr,char *pubName);

extern char *getHostIP(char* hname, char *localIP);
extern int nddsSubscriptionDestroy(NDDS_ID pNDDS_Obj);

extern cntlr_crew_t TheSendCrew;

NDDS_ID NDDS_Domain, pPubObj, pSubObj, pStatusObj;

#ifndef RTI_NDDS_4x
NDDSSubscriber CntlrSubscriber;
#endif /* RTI_NDDS_4x */

#ifndef RTI_NDDS_4x
/*---------------------------------------------------------------------------------- */

static void safeWrite(int fd, char *buf, size_t len)
{
   size_t nleft;
   size_t bytes;
 
   nleft = len;
   while (nleft > 0)
   {
      bytes = write(fd, buf, nleft);
      if (bytes > 0)
      {
         nleft -= bytes;
         buf += bytes;
      }
   }
}
 



/*     NDDS additions */
/*---------------------------------------------------------------------------------- */

RTIBool Codes_DownldCallback(const NDDSRecvInfo *issue, NDDSInstance *instance,
                             void *callBackRtnParam)
{

   Codes_Downld *recvIssue;
   NDDSBUFMNGR_ID pBufMngr;
   int *pipeFd;
   char *bufferAddr;
   /* cntlr_t *RtnParam = (cntlr_t *) callBackRtnParam; */
   /* pipeFd = (int *) callBackRtnParam; */
   pBufMngr = (NDDSBUFMNGR_ID) callBackRtnParam;

   /* DPRINT2(+1,"Codes_DownldCallback: pipe[0]: %d, pipe[1]: %d\n",pipeFd[0],pipeFd[1]); */
   /*    possible status values:
     NDDS_FRESH_DATA, NDDS_DESERIALIZATION_ERROR, NDDS_UPDATE_OF_OLD_DATA,
     NDDS_NO_NEW_DATA, NDDS_NEVER_RECEIVED_DATA
   */
   if (issue->status == NDDS_FRESH_DATA)
   {
     recvIssue = (Codes_Downld *) instance;
     /* DPRINT4(+1, "wrtPipFd: %d - Codes_DownldCallback: received NDDS msge cmdtype: %d, strtnumber: %d, number: %d\n",  
	pipeFd[1],recvIssue->cmdtype, recvIssue->strtnumber, recvIssue->number); */
     /* DPRINT2(+1, "wrtPipFd: %d - Codes_DownldCallback: received NDDS msge cmdtype: %d\n", 
	pipeFd[1],recvIssue->cmdtype); */
     /* printf("Msge len: %d, data: '%s'\n",recvIssue->len,recvIssue->data); */

     /* write(pipeFd[1],recvIssue,sizeof(Codes_Downld)); went to safeWrite incase of signals */
     /* safeWrite(pipeFd[1],recvIssue,sizeof(Codes_Downld)); */
     /*   buffer usage  ... */
       bufferAddr = msgeBufGet(pBufMngr);
       if (bufferAddr == NULL)
          return RTI_FALSE;
       memcpy(bufferAddr,recvIssue,sizeof(Codes_Downld));
       msgePost(pBufMngr,bufferAddr);
   }  
   return RTI_TRUE;
}

/*---------------------------------------------------------------------------------- */

RTIBool App_HBCallback(const NDDSRecvInfo *issue, NDDSInstance *instance,
                             void *callBackRtnParam)
{

   App_HB *recvIssue;
   cntlr_t *mine = (cntlr_t*) callBackRtnParam;

   /*    possible status values:
     NDDS_FRESH_DATA, NDDS_DESERIALIZATION_ERROR, NDDS_UPDATE_OF_OLD_DATA,
     NDDS_NO_NEW_DATA, NDDS_NEVER_RECEIVED_DATA
   */
   if (issue->status == NDDS_FRESH_DATA)
   {
     recvIssue = (App_HB *) instance;
     DPRINT5(+3, "'Sendproc': App_HBCallback: '%s': received AppStr: '%s', HB cnt: %lu, ThreadId: %d, AppID: %d\n",  
	issue->nddsTopic,recvIssue->AppIdStr, recvIssue->HBcnt, recvIssue->ThreadId,recvIssue->AppId); 
     if (mine->numSubcriber4Pub < 1)
     {
        mine->numSubcriber4Pub = 1;
        DPRINT1(+1,"'Sendproc': App_HBCallback: Issue: '%s', Is Back.\n", issue->nddsTopic);
     }
   }  
   else if (issue->status == NDDS_NO_NEW_DATA)
   {
       DPRINT1(+1,"'Sendproc': App_HBCallback: Issue: '%s', Missed Deadline App/Node must be gone.\n", 
                    issue->nddsTopic);
       mine->numSubcriber4Pub = 0;
   }
   return RTI_TRUE;
}

/* ---------------------------------------------------------------------------------- */
 
/*
If a read() is interrupted by a signal before it  reads  any
     data, it will return -1 with errno set to EINTR.

     If a read() is interrupted by a signal after it has success-
     fully  read  some  data,  it will return the number of bytes
     read.
*/
static void safeRead(int fd, char *buf, size_t len)
{
   size_t nleft;
   size_t bytes;
 
   nleft = len;
   while (nleft > 0)
   {
      bytes = read(fd, buf, nleft);
      if (bytes > 0)
      {
         nleft -= bytes;
         buf += bytes;
      }
   }
}
 
/* ---------------------------------------------------------------------------------- */

int readBlkingMsgePipe(int *pipeFd, char* msgeBuffer)
{
     size_t msgSize;

     /* DPRINT2(+1,"readBlkingMsgePipe: pipeFd[0]: %d, pipeFd[1]: %d\n",pipeFd[0],pipeFd[1]); */

     msgSize = sizeof(Codes_Downld);
     safeRead(pipeFd[0], msgeBuffer, msgSize);
     return(msgSize);
}

/* ---------------------------------------------------------------------------------- */
#ifdef XXXX
int readBlkingMsgePipe(int *pipeFd, char* msgeBuffer)
{
     int bytes;
     int nleft;
     int msgeSize;
     int totalMsgeSize;

     /* DPRINT2(+1,"readBlkingMsgePipe: pipeFd[0]: %d, pipeFd[1]: %d\n",pipeFd[0],pipeFd[1]); */
     msgeSize = 0;
     /* bytes = read(codeMsgePipeFd[0], &totalMsgeSize, sizeof(totalMsgeSize)); */
     /* bytes = read(codeMsgePipeFd[0], &msgeSize, sizeof(msgeSize)); */
     /* DPRINT2(+1, "Sendproc: readBlkingMsgePipe(): read length of msge: totalSize %d bytes, msge len = %d\n",totalMsgeSize,msgeSize); */
     nleft = sizeof(Codes_Downld);
     while( nleft > 0 )
     {
	  /* bytes = read(pipeFd[0], msgeBuffer, nleft); */
	  bytes = read(pipeFd[0], msgeBuffer, nleft);
          /* DPRINT2(+1, "readMsgePipe(): rdPipeFd: %d, read msge: read %d bytes\n",pipeFd[0],bytes); */
          if (bytes < 0) 
          {
            bytes = 0;

#ifdef XXXX_CAUSES_CORE_DUMP
            /* This caused a core dump.  in Recvproc usage so beware */
            if ( (errno != EAGAIN) && (errno != EINTR))
               errLogSysRet(LOGOPT,debugInfo,"Sendproc: readBlkingMsgePipe(): pipe read error %d",errno);
#endif
          }
          nleft -= bytes;
          msgeBuffer += bytes;
          msgeSize += bytes;
     }
     return(msgeSize);
}

#endif

#else  /* RTI_NDDS_4x */

/* 4x callback */

#define MAX_BUFMNGRS 300
static NDDSBUFMNGR_ID      NddsBufMngrs[MAX_BUFMNGRS];
static int numOfNddsBufMngrs = 0;


void Codes_DownldCallback(void* listener_data, DDS_DataReader* reader)
{
   Codes_Downld *recvIssue;
   // NDDSBUFMNGR_ID pBufMngr[];
   char *bufferAddr;
   struct DDS_SampleInfo* info = NULL;
   struct DDS_SampleInfoSeq info_seq = DDS_SEQUENCE_INITIALIZER;
   DDS_ReturnCode_t retcode;
   int i,numIssues;
   DDS_TopicDescription *topicDesc;

   // pBufMngr = (NDDSBUFMNGR_ID *) listener_data;

   struct Codes_DownldSeq data_seq = DDS_SEQUENCE_INITIALIZER;
   Codes_DownldDataReader *CodesDownld_reader = NULL;

   CodesDownld_reader = Codes_DownldDataReader_narrow(pSubObj->pDReader);
   if ( CodesDownld_reader == NULL)
   {
        errLogRet(LOGIT,debugInfo,"DataReader narrow error\n");
        return;
   }

   topicDesc = DDS_DataReader_get_topicdescription(reader);
   DPRINT2(2,"Codes_DownldCallback: Type: '%s', Name: '%s'\n",
      DDS_TopicDescription_get_type_name(topicDesc), DDS_TopicDescription_get_name(topicDesc));
   while(1)
   {
        // Given DDS_HANDLE_NIL as a parameter, take_next_instance returns
        // a sequence containing samples from only the next (in a well-determined
        // but unspecified order) un-taken instance.
        retcode =  Codes_DownldDataReader_take_next_instance(
            CodesDownld_reader,
            &data_seq, &info_seq, DDS_LENGTH_UNLIMITED,
            &DDS_HANDLE_NIL,
            DDS_ANY_SAMPLE_STATE, DDS_ANY_VIEW_STATE, DDS_ANY_INSTANCE_STATE);

        if (retcode == DDS_RETCODE_NO_DATA) {
            DPRINT(+2,"Codes_DownldCallback: Take Instance gave DDS_RETCODE_NO_DATA, break out of callback while\n");
            break; // return;
        } else if (retcode != DDS_RETCODE_OK) {
            errLogRet(LOGIT,debugInfo,"Codes_DownldCallback: take next instance error %d\n",retcode);
        }
        numIssues = Codes_DownldSeq_get_length(&data_seq);
        DPRINT1(2,"Codes_DownldCallback: numIssues: %d\n",numIssues);

        for (i=0; i < numIssues; i++)
        {
           info = DDS_SampleInfoSeq_get_reference(&info_seq, i);
           if (info->valid_data)
           {
              int index;
              recvIssue = (Codes_Downld *) Codes_DownldSeq_get_reference(&data_seq,i);
              index = findCntlr(&TheSendCrew, recvIssue->nodeId);
              bufferAddr = msgeBufGet(NddsBufMngrs[index]);
              if (bufferAddr == NULL)
                 return;
              memcpy(bufferAddr,recvIssue,sizeof(Codes_Downld));
              msgePost(NddsBufMngrs[index],bufferAddr);
           }
        }
        retcode = Codes_DownldDataReader_return_loan( CodesDownld_reader,
                  &data_seq, &info_seq);
        DDS_SampleInfoSeq_set_maximum(&info_seq, 0);
   } // while
   return;
}

#endif /* RTI_NDDS_4x */

/**************************************************************
*
*  initiateNDDS - Initialize a NDDS Domain for  communications 
*   
***************************************************************/
void initiateNDDS(int debuglevel)
{
    char localIP[80];
    /* NDDS_ID nddsCreate(int domain, int debuglevel, int multicast, char *nicIP) */
    /* NDDS_Domain = nddsCreate(NDDS_DEFAULT_DOMAIN,NDDS_DBUG_LEVEL,MULTICAST,getHostIP(ConsoleHostName,localIP)); */
#ifndef NO_MULTICAST
    NDDS_Domain = nddsCreate(NDDS_DOMAIN_NUMBER,debuglevel,MULTICAST_ENABLE,(char*) getHostIP(ConsoleHostName,localIP));
#else
    NDDS_Domain = nddsCreate(NDDS_DOMAIN_NUMBER,debuglevel,MULTICAST_DISABLE,(char*) getHostIP(ConsoleHostName,localIP));
#endif
   if (NDDS_Domain == NULL)
      errLogQuit(LOGOPT,debugInfo,"Sendproc: initiateNDDS(): NDDS domain failed to initialized\n");
}



#ifndef RTI_NDDS_4x
int createAppHB_BESubscription(cntlr_t *pCntlrThr,char *subName)
{
    /* Build Data type Object for both publication and subscription to Expproc */
    /* ------- malloc space for data type object --------- */
    if ( (pSubObj = (NDDS_ID) malloc( sizeof(NDDS_OBJ)) ) == NULL )
    {
        return(-1);
    }
    pCntlrThr->SubHBId = pSubObj;

    /* zero out structure */
    memset(pSubObj,0,sizeof(NDDS_OBJ));
    memcpy(pSubObj,NDDS_Domain,sizeof(NDDS_OBJ));

    strcpy(pSubObj->topicName,subName);
 
    /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */
    getApp_HBInfo(pSubObj);
 
    pSubObj->callBkRtn = App_HBCallback;
    pSubObj->callBkRtnParam = (void*) pCntlrThr;
#ifndef NO_MULTICAST
    strcpy(pSubObj->MulticastSubIP,APP_HB_MULTICAST_IP);
#else
    pSubObj->MulticastSubIP[0] = 0;   /* use UNICAST */
#endif
    pSubObj->BE_UpdateMinDeltaMillisec = 1000;   /* max rate once a second */
    pSubObj->BE_DeadlineMillisec = 6000; /* no HB in 6 sec then it's gone.. */
    createBESubscription(pSubObj);
    DPRINT1(+2,"createAppHBSubscription(): subscription: 0x%lx\n",pSubObj->subscription);
}
#endif /* RTI_NDDS_4x */

#ifdef RTI_NDDS_4x
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
    char pubtopic[128*2];
    struct DDS_PublicationBuiltinTopicDataSeq data_seq = DDS_SEQUENCE_INITIALIZER;
    struct DDS_PublicationBuiltinTopicData *regular_data;
    struct DDS_SampleInfoSeq info_seq = DDS_SEQUENCE_INITIALIZER;
    DDS_ReturnCode_t retcode;
    int i;
    struct DDS_PublicationBuiltinTopicDataDataReader* builtin_reader = 
	(struct DDS_PublicationBuiltinTopicDataDataReader*) reader;
    unsigned char *user_data = NULL; 
    
    /* take the latest publication found */
    retcode = DDS_PublicationBuiltinTopicDataDataReader_take(
        builtin_reader, &data_seq, &info_seq, DDS_LENGTH_UNLIMITED,
        DDS_ANY_SAMPLE_STATE, DDS_ANY_VIEW_STATE, DDS_ANY_INSTANCE_STATE);
    
    DPRINT(+1,"**************  Builtin Discovery Callback  ************************** \n");
    if (retcode != DDS_RETCODE_OK) {
        errLogRet(LOGIT,debugInfo,"Error: failed to access data from the built-in reader\n");
	     return;
    }

    for (i = 0; i < DDS_PublicationBuiltinTopicDataSeq_get_length(&data_seq); ++i) 
    {
        if (DDS_SampleInfoSeq_get_reference(&info_seq, i)->valid_data) 
        {
	    regular_data = DDS_PublicationBuiltinTopicDataSeq_get_reference(&data_seq, i);
	    DPRINT2(1,"Built-in Reader: found publisher of topic \"%s\" and type "
		   "\"%s\".\n", regular_data->topic_name, regular_data->type_name);
	    
            DPRINT2(1,"Looking for type: '%s', topic: '%s'\n", Codes_DownldTYPENAME,CNTLR_CODES_DOWNLD_PUB_M21_STR);
            if ( (strcmp(Codes_DownldTYPENAME, regular_data->type_name) == 0) &&
                 (strcmp(CNTLR_CODES_DOWNLD_PUB_M21_STR, regular_data->topic_name) == 0)  )
            {
              int len, threadIndex;
              DDS_Octet cntlrName[128];
	      /* see if there is user data */
              len = DDS_OctetSeq_get_length(&(regular_data->user_data.value));
	      if (len == 0)
              {
                 DPRINT(1,"--->  No user_data\n");
                 continue;
              }

              DDS_OctetSeq_to_array(&(regular_data->user_data.value),cntlrName,len);
	           user_data = DDS_OctetSeq_get_reference(&(regular_data->user_data.value), 0);
	           DPRINT2(-1,"Built-in Reader: found \"%s\" and type " 
                         "\"%s\".\n", regular_data->topic_name, regular_data->type_name);
              DPRINT1(-1,"User_data: '%s'\n",user_data);
              DPRINT1(-1,"CntrlName: '%s'\n",cntlrName);
              threadIndex = initCntrlThread(&TheSendCrew, (char *)cntlrName);
              DPRINT1(-1,"thread Index: %d\n",threadIndex);
              if (threadIndex != -1)
              { 
                NddsBufMngrs[threadIndex] = nddsBufMngrCreate(100,sizeof(Codes_Downld));
                DPRINT1(-1,"nddsBufMngrCreate: %p\n", NddsBufMngrs[threadIndex]);
                sprintf(pubtopic,HOST_PUB_TOPIC_FORMAT_STR,cntlrName);
                DPRINT1(-1,"Topic name: '%s'\n",pubtopic);
                createCodeDownldPublication(&(TheSendCrew.crew[threadIndex]),pubtopic);
                createCntrlThread(&TheSendCrew, threadIndex, NddsBufMngrs[threadIndex]);
                numOfNddsBufMngrs++;
              }
              else
              {
                DPRINT1(-1,"Thread & Pub already present for cntlr: '%s'\n",cntlrName);
              }
           }
       }
    }

    retcode = DDS_PublicationBuiltinTopicDataDataReader_return_loan(builtin_reader, &data_seq, &info_seq);
    if (retcode != DDS_RETCODE_OK) {
        errLogRet(LOGIT,debugInfo,"return loan error %d\n", retcode);
    }
}

void createDiscoveryCallback(NDDS_ID pNDDS_Obj)
{
    DDS_Subscriber                             *builtinSubscriber;
    DDS_PublicationBuiltinTopicDataDataReader *builtinDataReader;
    struct DDS_DataReaderListener builtin_listener = DDS_DataReaderListener_INITIALIZER;

    /* get the built-in subscriber and data reader */
    builtinSubscriber = DDS_DomainParticipant_get_builtin_subscriber(pNDDS_Obj->pParticipant);
    // printf("get builtin_subscriber\n");
    if (builtinSubscriber == NULL) {
        errLogRet(LOGIT,debugInfo," failed to create built-in subscriber\n");
	     return;
    }
    // printf("get builtin_datareader \n");
    builtinDataReader = (DDS_PublicationBuiltinTopicDataDataReader *)DDS_Subscriber_lookup_datareader(builtinSubscriber, DDS_PUBLICATION_TOPIC_NAME);
    if (builtinDataReader == NULL) {
       errLogRet(LOGIT,debugInfo," failed to create built-in subscriber data reader\n");
	    return;
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

    // printf("set builtin_datareader listener\n");
    DDS_DataReader_set_listener((DDS_DataReader *)builtinDataReader, &builtin_listener, DDS_DATA_AVAILABLE_STATUS); 
}
/********************* Add for user data: finish *****************************/


void CodeDownld_RequestedLivelinessChanged(void* listener_data, DDS_DataReader* reader,
                      const struct DDS_LivelinessChangedStatus *status)
{
   DDS_TopicDescription *topicDesc;
   topicDesc = DDS_DataReader_get_topicdescription(reader);
   DPRINT2(-1,"Default_RequestedLivelinessChanged for: Type: '%s', Name: '%s'\n",
           DDS_TopicDescription_get_type_name(topicDesc), DDS_TopicDescription_get_name(topicDesc));
   DPRINT1(-1,"The total count of currently alive DDS_DataWriter entities: %d\n",status->alive_count);
   DPRINT1(-1,"The total count of currently not_alive DDS_DataWriter entities: %d\n",status->not_alive_count);
   DPRINT1(-1,"The change in the alive_count: %d\n",status->alive_count_change);
   DPRINT1(-1,"The change in the not_alive_count: %d\n", status->not_alive_count_change);
//   DPRINT1(-1,"handle to the last remote writer to change its liveliness: %ld\n",status->last_publication_handle);
}

void CodeDownld_SubscriptionMatched(void* listener_data, DDS_DataReader* reader,
                        const struct DDS_SubscriptionMatchedStatus *status)
{
//    DDS_ReturnCode_t retcode;
    DDS_TopicDescription *topicDesc;
//    struct DDS_DataWriterQos DWQos = DDS_DataWriterQos_INITIALIZER;
/*
    DDS_Long    total_count
        The total cumulative number of times the concerned DDS_DataReader
        discovered a "match" with a DDS_DataWriter.
    DDS_Long    total_count_change
        The change in total_count since the last time the listener was
        called or the status was read.
    DDS_Long    current_count
        The current number of writers with which the DDS_DataReader is matched.
    DDS_Long    current_count_change
        The change in current_count since the last time the listener was
        called or the status was read.
    DDS_InstanceHandle_t        last_publication_handle
        A handle to the last DDS_DataWriter that caused the status to change.
*/
   topicDesc = DDS_DataReader_get_topicdescription(reader);
   DPRINT4(-1,"Subscription Type: '%s', Name: '%s' Matched, Matched: %d, Delta: %d\n",
           DDS_TopicDescription_get_type_name(topicDesc), DDS_TopicDescription_get_name(topicDesc),
           status->current_count,status->current_count_change);

   return;
}

#endif /* RTI_NDDS_4x */


int createCodeDownldSubscription(cntlr_t *pCntlrThr,char *subName)
{
    /* Build Data type Object for both publication and subscription to Expproc */
    /* ------- malloc space for data type object --------- */
    if ( (pSubObj = (NDDS_ID) malloc( sizeof(NDDS_OBJ)) ) == NULL )
    {
        return(-1);
    }
#ifndef RTI_NDDS_4x     /* used in ndds 4x */
    pCntlrThr->SubId = pSubObj;
#endif /* RTI_NDDS_4x */

    /* create pipe for messages from NDDS signal handler Console to Expproc */
    /* subscription Console2Expproc */

    /* create ring buffer for message and a blocking ring buffer for addresses of these
       buffers */
#ifndef RTI_NDDS_4x    /* created in discovery callback */
     pCntlrThr->pNddsBufMngr = nddsBufMngrCreate(100,sizeof(Codes_Downld));
#endif /* RTI_NDDS_4x */

    /*
     *if ( pipe( pCntlrThr->SubPipeFd) != 0)
     *{
     *    errLogSysQuit(LOGOPT,debugInfo,"Sendproc: createCodeDownldSubscription(): could not create Message Pipe\n");
     *}
     */
    /* DPRINT2(+1,"---> pipe fd[0]: %d, fd[1]: %d\n",pCntlrThr->SubPipeFd[0],pCntlrThr->SubPipeFd[1]);  */
    /* zero out structure */
    memset(pSubObj,0,sizeof(NDDS_OBJ));
    memcpy(pSubObj,NDDS_Domain,sizeof(NDDS_OBJ));
 
    strcpy(pSubObj->topicName,subName);
 
    /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */
    getCodes_DownldInfo(pSubObj);
 
    pSubObj->queueSize = HOST_CODES_DOWNLD_SUB_QSIZE;
#ifndef RTI_NDDS_4x
    pSubObj->callBkRtn = Codes_DownldCallback;
    pSubObj->callBkRtnParam = pCntlrThr->pNddsBufMngr;    /* write end of pipe */
#endif /* RTI_NDDS_4x */
    pSubObj->MulticastSubIP[0] = 0;   /* use UNICAST */
#ifdef RTI_NDDS_4x
    initSubscription(pSubObj);
    attachOnDataAvailableCallback(pSubObj,Codes_DownldCallback,(void*) NddsBufMngrs);
    attachLivelinessChangedCallback(pSubObj,CodeDownld_RequestedLivelinessChanged);
    attachSubscriptionMatchedCallback(pSubObj,CodeDownld_SubscriptionMatched);
#endif /* RTI_NDDS_4x */
    createSubscription(pSubObj);
#ifndef RTI_NDDS_4x
    DPRINT1(+1,"createCodeDownldSubscription(): subscription: 0x%lx\n",pSubObj->subscription);
#endif /* RTI_NDDS_4x */

    return(0);
}

int createCodeDownldPublication(cntlr_t *pCntlrThr,char *pubName)
{
#ifndef RTI_NDDS_4x
    extern  void MyThreadPubStatusRtn(NDDSPublicationReliableStatus *status,
                                    void *callBackRtnParam);
#endif /* RTI_NDDS_4x */
    /* Build Data type Object for both publication and subscription to Expproc */
    /* ------- malloc space for data type object --------- */
    if ( (pPubObj = (NDDS_ID) malloc( sizeof(NDDS_OBJ)) ) == NULL )
      {
        return(-1);
      }  

    pCntlrThr->PubId = pPubObj;
 
    /* zero out structure */
    memset(pPubObj,0,sizeof(NDDS_OBJ));
    memcpy(pPubObj,NDDS_Domain,sizeof(NDDS_OBJ));
 
    strcpy(pPubObj->topicName,pubName);
 
    /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */
    getCodes_DownldInfo(pPubObj);

    pPubObj->queueSize = HOST_CODES_DOWNLD_PUB_QSIZE;
    pPubObj->highWaterMark = HOST_CODES_DOWNLD_PUB_HIWATER;
    pPubObj->lowWaterMark = HOST_CODES_DOWNLD_PUB_LOWWATER;
    // pPubObj->AckRequestsPerSendQueue = HOST_CODES_DOWNLD_PUB_ACKSPERQ;
    pPubObj->AckRequestsPerSendQueue = HOST_CODES_DOWNLD_PUB_QSIZE;

#ifndef RTI_NDDS_4x
    pPubObj->pubThreadId = pCntlrThr->index;   /* for mulit threaded apps */
    pPubObj->pubRelStatRtn = MyThreadPubStatusRtn;  /* in threadfuncs.c */
    pPubObj->pubRelStatParam =  (void*) pCntlrThr;
#else  /* RTI_NDDS_4x */
    initPublication(pPubObj);
#endif /* RTI_NDDS_4x */
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
static int callbackParam = 1;
NDDSSubscription Cntlr_CodeDwnldPatternSubCreate( const char *nddsTopic, const char *nddsType,
                  void *callBackRtnParam)
{
     char cntrlName[128];
     char *chrptr;
     int threadIndex;
     cntlr_t   *me;
     NDDSSubscription pSub;

     DPRINT3(+1,"Cntlr_CodesDwnldPatternSubCreate(): Topic: '%s', Type: '%s', arg: 0x%lx\n",
                nddsTopic, nddsType, callBackRtnParam);
     DPRINT2(+1,"callbackParam: 0x%lx, callBackRtnParam: 0x%lx\n",&callbackParam,callBackRtnParam);
     strncpy(cntrlName,nddsTopic,127);
     chrptr = strchr(cntrlName,'/');
     *chrptr = 0;
     DPRINT1(+1,"CntrlName: '%s'\n",cntrlName);
     threadIndex = addCntrlThread(&TheSendCrew,cntrlName);
     me = &(TheSendCrew.crew[threadIndex]);
     DPRINT3(+1,"Cntlr_CodeDwnldPatternSubCreate(): threadIndex: %d, Cntlr: '%s', subscription: 0x%lx\n",
		threadIndex,me->cntlrId,me->SubId->subscription);

     pSub = me->SubId->subscription;

     /* pSub = me->SubHBId->subscription; */
    
     return pSub;
}


/*
 *  Create a the Code DowndLoad pattern subscriber, to dynamicly allow subscription creation
 *  as controllers come on-line and publication to Sendproc download topic 
 *
 *                                      Author Greg Brissey 4-26-04
 */
int cntlrCodeDwnldPatternSub()
{
    /* MASTER_SUB_COMM_PATTERN_TOPIC_STR */
    CntlrSubscriber = NddsSubscriberCreate(NDDS_DOMAIN_NUMBER);
 
    /* master subscribe to any publications from controllers */
    NddsSubscriberPatternAdd(CntlrSubscriber,
           "*/h/dwnld/reply",  Codes_DownldNDDSType , Cntlr_CodeDwnldPatternSubCreate, (void *)&callbackParam);

#ifdef XXX
    NddsSubscriberPatternAdd(CntlrSubscriber,
           nodeHB_PATTERN_FORMAT_STR, App_HBNDDSType , Cntlr_CodeDwnldPatternSubCreate, (void *)&callbackParam);
#endif
    return 0;
}

#endif /* RTI_NDDS_4x */

#ifdef RTI_NDDS_4x
void initCodeDownldSub()
{
   createDiscoveryCallback(NDDS_Domain);
   createCodeDownldSubscription(NULL,HOST_CODES_DOWNLD_SUB_M21_STR);
}

int ddsPublishData(NDDS_ID pNDDS_Obj)
{
   DDS_ReturnCode_t result;
   DDS_InstanceHandle_t instance_handle = DDS_HANDLE_NIL;
   Codes_DownldDataWriter *CodesDownld_writer = NULL;

   CodesDownld_writer = Codes_DownldDataWriter_narrow(pNDDS_Obj->pDWriter);
   if (CodesDownld_writer == NULL) {
        errLogRet(LOGIT,debugInfo,"DataWriter narrow error\n");
        return -1;
   }

   result = Codes_DownldDataWriter_write(CodesDownld_writer,
                pNDDS_Obj->instance,&instance_handle);
   if (result != DDS_RETCODE_OK) {
            errLogRet(LOGIT,debugInfo,"DataWriter write error: %d\n",result);
   }
   return 0;
}
#endif /* RTI_NDDS_4x */


int sendCmd(int cmdtype,int buftype,char *buft,char *label)
{
      Codes_Downld *issue = pPubObj->instance;
      issue->sn = 0;
      issue->cmdtype = cmdtype;
      issue->status = buftype;
#ifndef RTI_NDDS_4x
      issue->data.len = 0;
#else /* RTI_NDDS_4x */
      DDS_OctetSeq_set_length(&issue->data,0);
#endif /* RTI_NDDS_4x */
      strcpy(issue->msgstr,buft);
      strcpy(issue->label,label);
#ifndef RTI_NDDS_4x
      return( nddsPublishData(pPubObj) );  /* -1 = error, 0 = OK */
#else  /* RTI_NDDS_4x */
      return( ddsPublishData(pPubObj) );  /* -1 = error, 0 = OK */
#endif /* RTI_NDDS_4x */
}


int getXferSize(NDDS_ID pubId, NDDSBUFMNGR_ID pBufMngr)
{
     int status;
     Codes_Downld *issue = pubId->instance;
     Codes_Downld  *pReplyIssue;
     int numOfBufs, cmdtype;


     /* prepare to send command */
     issue->sn = 0;
     issue->cmdtype = C_NAMEBUF_QUERY;
     issue->status = 0;
#ifndef RTI_NDDS_4x
     issue->data.len = 0;
#else  /* RTI_NDDS_4x */
      DDS_OctetSeq_set_length(&issue->data,0);
#endif /* RTI_NDDS_4x */
     issue->msgstr[0] = 0;
     strcpy(issue->label,"Query for NBufs");
#ifndef RTI_NDDS_4x
     status = nddsPublishData(pubId);
#else  /* RTI_NDDS_4x */
     status = ddsPublishData(pubId);
#endif /* RTI_NDDS_4x */
     if (status == -1)
     {
        errLogRet(LOGOPT,debugInfo,"Sendproc: getXferSize(): publishing failed");
        return(-1);
     }

      /* wait on this blocking call until an reply issue is received */
      pReplyIssue = (Codes_Downld  *) msgeGet(pBufMngr);
      if (pReplyIssue == NULL) 
      {
	  DPRINT(+2,"getXferSize(): Aborted Read\n");
          return(-99);
      }
      cmdtype = pReplyIssue->cmdtype;
      numOfBufs = pReplyIssue->status;
      /* return this buffer to the free list for NDDS callback */
      msgeBufReturn(pBufMngr,(char*) pReplyIssue);

     if (cmdtype != C_QUERY_ACK)
        return(-1);
     else
        return(numOfBufs);
}


int sendXferStartwArgs(NDDS_ID pubId,int timeout,int arg2)
{
     Codes_Downld *issue = pubId->instance;
     issue->sn = arg2;
     issue->cmdtype = C_DWNLD_START;
     issue->status = timeout;
#ifndef RTI_NDDS_4x
     issue->data.len = 0;
#else  /* RTI_NDDS_4x */
      DDS_OctetSeq_set_length(&issue->data,0);
#endif /* RTI_NDDS_4x */
     issue->msgstr[0] = 0;
     strcpy(issue->label,"Start of Transfer");
#ifndef RTI_NDDS_4x
     return( nddsPublishData(pubId) );
#else  /* RTI_NDDS_4x */
     return( ddsPublishData(pubId) );
#endif /* RTI_NDDS_4x */
}

int sendXferStart(NDDS_ID pubId)
{
     Codes_Downld *issue = pubId->instance;
     issue->sn = 0;
     issue->cmdtype = C_DWNLD_START;
     issue->status = 0;
#ifndef RTI_NDDS_4x
     issue->data.len = 0;
#else  /* RTI_NDDS_4x */
      DDS_OctetSeq_set_length(&issue->data,0);
#endif /* RTI_NDDS_4x */
     issue->msgstr[0] = 0;
     strcpy(issue->label,"Start of Transfer");
#ifndef RTI_NDDS_4x
     return( nddsPublishData(pubId) );
#else  /* RTI_NDDS_4x */
     return( ddsPublishData(pubId) );
#endif /* RTI_NDDS_4x */
}

int sendXferCmplt(NDDS_ID pubId)
{
     Codes_Downld *issue = pubId->instance;
     issue->sn = 0;
     issue->cmdtype = C_DWNLD_CMPLT;
     issue->status = 0;
#ifndef RTI_NDDS_4x
     issue->data.len = 0;
#else  /* RTI_NDDS_4x */
      DDS_OctetSeq_set_length(&issue->data,0);
#endif /* RTI_NDDS_4x */
     issue->msgstr[0] = 0;
     strcpy(issue->label,"Transfer Cmplt");
#ifndef RTI_NDDS_4x
     return( nddsPublishData(pubId) );
#else  /* RTI_NDDS_4x */
     return( ddsPublishData(pubId) );
#endif /* RTI_NDDS_4x */
}


int wait4ConsoleSub(NDDS_ID pubId)
{
   int stat;
   int retry = 0;
   while( (stat = nddsWait4Subscribers(pubId,10,1)) == -1)
   {
       retry++;
       DPRINT1(+1,"Waiting on subscription to '%s'.\n",pubId->topicName);
       if (retry > 0)
        return -1;
   }
   return 0;
}

int writeToConsole(char *cntlrId, NDDS_ID pubId, char *name, char* bufAdr,int size, int serialNum, int ackItr )
{
  Codes_Downld *issue = pubId->instance;
  int xfrsize,tbytes;
  int bytesleft;
/*
#ifdef LINUX
  int ival;
#endif
*/
  int ival;

  DPRINT3(+2,"'%s' - writeToConsole: name: '%s',  pub xfrsize: %d\n",cntlrId, name,size);
  issue->cmdtype = C_DOWNLOAD;
  issue->status = 0;
  issue->ackInterval = ackItr;
  issue->sn = serialNum;
  issue->msgstr[0] = 0;
  strcpy(issue->label,name);
  issue->crc32chksum = addbfcrc(bufAdr,size);

  tbytes = 0;

  // i = 1;
  issue->totalBytes = size;
  issue->dataOffset = 0;
  bytesleft = size;
  /* wait for any previous transfer to complete */
  /* nddsPublicationIssuesWait(pPubObj, 1, 1); */
  while (bytesleft > 0)
  {
     xfrsize = (bytesleft < MAX_FIXCODE_SIZE)  ? bytesleft : MAX_FIXCODE_SIZE;
     /* issue->sn = i++; */
#ifndef RTI_NDDS_4x
     issue->data.len = xfrsize;
     /* printf("memcpy(0x%lx,0x%lx,%ld)\n",issue->data.val,bufAdr,xfrsize); */
     memcpy(issue->data.val,bufAdr,xfrsize);
#else  /* RTI_NDDS_4x */
     DDS_OctetSeq_from_array(&(issue->data),(const DDS_Octet *)bufAdr,xfrsize);
#endif /* RTI_NDDS_4x */
     DPRINT5(+2,"'%s' - writeToConsole: name: '%s', sn: %d, pub xfrsize: %d, crc32: %d\n",cntlrId, issue->label,issue->sn, xfrsize,issue->crc32chksum);
     /* blockAllEvents();    thread already has most signal blocked */
     /* nddsPublishData(pPubObj); */
#ifndef RTI_NDDS_4x
     nddsPublishData(pubId);
#else  /* RTI_NDDS_4x */
     ddsPublishData(pubId);
#endif /* RTI_NDDS_4x */
     /* unblockAllEvents(); thread already has most signal blocked */
     tbytes += xfrsize;   /* increment total bytes transfered */
     bufAdr += xfrsize;   /* increment address into data to xfer */
     issue->dataOffset += xfrsize;
     bytesleft -= xfrsize; /* decrement bytes left to xfer */
     /* this is to over come the VxWorks stack from dropping UDP packets, i.e. slow it down, GMB 10/25/2005 */
     /* nddsPublicationIssuesWait(pubId, 1, 0); still too fast over 3000 packets/sec or 4.5 Mbytes/sec */
     /* this is to over come the VxWorks stack from dropping UDP packets, i.e. slow it down, GMB 10/25/2005 */
#ifdef LINUX
     while ( ((ival = usleep(10000)) != 0) && (errno == EINTR))
     {
          ival = errno = 0;
     }
#else
     while ( ((ival = usleep(10000)) != 0) && (errno == EINTR))
     {
          ival = errno = 0;
     }
#endif
  }
  return(tbytes);
}


int killOffSub()
{
   int stat;
    stat = nddsPublicationDestroy(pPubObj);
    DPRINT1(-3,"Pub destroy: %d\n",stat);
    stat = nddsSubscriptionDestroy(pSubObj);
    DPRINT1(-3,"Sub destroy: %d\n",stat);
    return 0;
}

int initPS(cntlr_crew_t *crewList)
{

    cntlr_t *pCntlr;
    /* just one for now */
    /* topica names form: con/rf1/downld /strm, h/rf1/downld/reqack */
    pCntlr = &crewList->crew[0];
   createCodeDownldPublication(pCntlr,"h/rf1/dwnld/strm");
   createCodeDownldSubscription(pCntlr,"c/rf1/dwnld/reply");
   return 0;
}

int shutdownComm(void)
{
#ifdef JUST_DELETE_DOMAIN
   int i, numOfSubs;
   numOfSubs = TheSendCrew.crew_size;      /* Size of array */
   for (i=0; i < numOfSubs; i++)
   {
      DPRINT1(+1,"Destroying pub/sub for Cntlr: '%s'\n",
	    TheSendCrew.crew[i].cntlrId);

       if (TheSendCrew.crew[i].PubId != NULL)
          nddsPublicationDestroy(TheSendCrew.crew[i].PubId);
       if (TheSendCrew.crew[i].SubId != NULL)
          nddsSubscriptionDestroy(TheSendCrew.crew[i].SubId);
   }
   /* usleep(500000);   .5 sec, no effect in make the vxworks see the pub/sub disappear quicker,
      maybe its a NDDS manager setting */
#endif

   if (NDDS_Domain != NULL) 
   { 
#ifndef RTI_NDDS_4x
      DPRINT1(+1,"Sendproc: Destroy Domain: 0x%lx\n",NDDS_Domain->domain);
      NddsDestroy(NDDS_Domain->domain);
#else  /* RTI_NDDS_4x */
      NDDS_Shutdown( NDDS_Domain );
#endif /* RTI_NDDS_4x */
      usleep(400000);  /* 400 millisec sleep */
   }   
   return 0;

}
