/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef LINT
#endif

/* 
 */
#ifdef VNMRS_WIN32
#include <Windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#ifndef VNMRS_WIN32
#include <unistd.h>
#endif //VNMRS_WIN32
#include <string.h>
#ifndef VXWORKS
#ifdef VNMRS_WIN32
#include <time.h>
#else
#include <sys/time.h>
#endif
#include "errLogLib.h"
#else
#include "logMsgLib.h"
#endif
#include "ndds/ndds_c.h"
#include "NDDS_Obj.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef VXWORKS
#define PUB_DEFAULT_QUEUE_SIZE 10
#define PUB_DEFAULT_HWATER_MARK 7
#define PUB_DEFAULT_LWATER_MARK 3
#define PUB_DEFAULT_ACKS_PER_SENDQ 2
#else 
#define PUB_DEFAULT_QUEUE_SIZE 10
#define PUB_DEFAULT_HWATER_MARK 7
#define PUB_DEFAULT_LWATER_MARK 3
#define PUB_DEFAULT_ACKS_PER_SENDQ 2
#endif

#ifndef RTI_NDDS_4x 

typedef struct _pubevents_ {  int event;  char *desc; } PUBEVENT_DESC;

/* Events:
 * NDDS_BEFORERTN_VETOED: The sendBeforeRtn vetoed publication 
 * NDDS_HIGH_WATER_MARK: The send queue level rose to the high water mark 
 * NDDS_LOW_WATER_MARK; The send queue level fell to the low water mark 
 * NDDS_QUEUE_EMPTY: The send queue is empty 
 * NDDS_QUEUE_FULL: The send queue is full 
 * NDDS_RELIABLE_STATUS: You only see this value if you check the publication status with the NddsPublicationReliableStatusGet() 
 * NDDS_SUBSCRIPTION_DELETE: A reliable subscription was disappeared 
 * NDDS_SUBSCRIPTION_NEW: A new reliable subscription has appeared 
 */

static PUBEVENT_DESC pubEventDesc[8] = {
	{ NDDS_BEFORERTN_VETOED, "The sendBeforeRtn() vetoed publication" },
        { NDDS_HIGH_WATER_MARK, "The send queue level rose to the high water mark" },
	{ NDDS_LOW_WATER_MARK, "The send queue level fell to the low water mark" },
	{ NDDS_QUEUE_EMPTY, "The send queue is empty" },
	{ NDDS_QUEUE_FULL, "The send queue is full" },
        { NDDS_RELIABLE_STATUS, "You used NddsPublicationReliableStatusGet() to get this status" },
	{ NDDS_SUBSCRIPTION_DELETE, "A reliable subscription was disappeared" },
	{ NDDS_SUBSCRIPTION_NEW, "A reliable subscription was disappeared" }
};
	


/*
     Default Reliable Publication Status call back routine.
*/
void MyPublicationReliableStatusRtn(NDDSPublicationReliableStatus *status,
                                    void *callBackRtnParam)
{
      switch(status->event) 
      {
      case NDDS_QUEUE_EMPTY:
        /* printf("Queue empty\n"); */
        break;
      case NDDS_LOW_WATER_MARK:
        /*
        printf("Below low water mark - ");
        printf("Topic: '%s', UnAck Issues: %d\n",status->nddsTopic, status->unacknowledgedIssues);
        */
        break;
      case NDDS_HIGH_WATER_MARK:
        /*
        printf("Above high water mark - ");
        printf("Topic: '%s', UnAck Issues: %d\n",status->nddsTopic, status->unacknowledgedIssues);
        */
        break;
      case NDDS_QUEUE_FULL:
        /* printf("Queue Full for my Publication: '%s', UnAck Issues: %d\n",status->nddsTopic, status->unacknowledgedIssues); */
        break;
      case NDDS_SUBSCRIPTION_NEW:
	printf("A New Reliable Subscription Appeared for my Publication: '%s'.\n", status->nddsTopic);
	break;
      case NDDS_SUBSCRIPTION_DELETE:
	printf("A Reliable Subscription Disappeared for my Publication: '%s'.\n", status->nddsTopic);
	break;
      default:
		/* NDDS_BEFORERTN_VETOED
		   NDDS_RELIABLE_STATUS
	        */
        break;
      }
}

/*
    createPublication:

    Creates a Reliable NDDS Publication

     The following NDDS_Obj fields must be set:
     pNDDS_Obj->callBkRtn;
     pNNDS_Obj->TypeRegisterFunc
     pNNDS_Obj->TypeAllocFunc
     pNNDS_Obj->TypeSizeFunc
     pNNDS_Obj->dataTypeSize
     pNNDS_Obj->multicastFlg
     pNNDS_Obj->topicName[]
     pNDDS_Obj->dataTypeName[]
     pNDDS_Obj->MulticastSubIP[]

    Publication with the name: pNDDS_Obj->topicName is created.
    Register with the NDDS Domain, via the TypeRegisterFunc()
    Space for actually publication is allocatd via TypeAllocFunc()

*/
int createPublication(NDDS_ID pNDDS_Obj)
{
    NDDSPublication publication;
    NDDSPublicationProperties  *pProperties;
    NDDSPublicationListener   myPublicationListener;
    double requestedNAck;

    /* get default publication properties */
    NddsPublicationPropertiesDefaultGet(pNDDS_Obj->domain,&(pNDDS_Obj->pubProperties)); 
    pProperties = &(pNDDS_Obj->pubProperties);

    RtiNtpTimePackFromNanosec(pProperties->persistence, 0 , 16); /* 16 milliseconds */
    RtiNtpTimePackFromNanosec(pProperties->timeToKeepPeriod , 0, 0);	/* default  */
    if (pNDDS_Obj->publisherStrength < 1)
    {
        pProperties->strength        = 1;
    }
    else
    {
        pProperties->strength        = pNDDS_Obj->publisherStrength;
    }

    RtiNtpTimePackFromMillisec(pProperties->heartBeatTimeout, 1, 0); /*  no-acode bug change  */
    /* changes from NDDS benchmark results */
    RtiNtpTimePackFromMillisec(pProperties->heartBeatFastTimeout, 0, 250); /* 100ms,default 62.5 ms */

    /* 250 ms * 10000 = 2,500,000 ms or 2,500 seconds or ~ 41 minutes before sub is considered gone */
    pProperties->heartBeatRetries = 10000;

    /* block the Send call for up to sendMaxWait seconds when the queue is full */
    RtiNtpTimePackFromNanosec(pProperties->sendMaxWait,86400,0);

    DPRINT4(1,"----> QueueSize: %d, low: %d, high: %d  WaterMarks, HeartBeatsPerSendQ %d\n", 
         pNDDS_Obj->queueSize, pNDDS_Obj->lowWaterMark,pNDDS_Obj->highWaterMark,pNDDS_Obj->AckRequestsPerSendQueue);

    /* if Q Size  is zero then use all default values */
    if (pNDDS_Obj->queueSize <= 0)
    {
         pProperties->sendQueueSize = PUB_DEFAULT_QUEUE_SIZE;
         pProperties->highWaterMark = PUB_DEFAULT_HWATER_MARK; 
         pProperties->lowWaterMark  = PUB_DEFAULT_LWATER_MARK;
         pProperties->heartBeatsPerSendQueue = PUB_DEFAULT_ACKS_PER_SENDQ;
    }
    else
    {
         pProperties->sendQueueSize = pNDDS_Obj->queueSize;

         /* if highWaterMark is set then assume all paraemters have been specidied and use them */
         if (pNDDS_Obj->highWaterMark > 0 )
         {
             pProperties->highWaterMark   = pNDDS_Obj->highWaterMark ;
             pProperties->lowWaterMark = pNDDS_Obj->lowWaterMark;
             if (pNDDS_Obj->AckRequestsPerSendQueue > 0)
                pProperties->heartBeatsPerSendQueue = pNDDS_Obj->AckRequestsPerSendQueue ;
         }
         else
         {
             /* if highWaterMark NoT set then calculate all paraemters */
             int calcHi;
             calcHi = (3*pProperties->sendQueueSize)/4;
             pProperties->highWaterMark = (calcHi == 0) ? pProperties->sendQueueSize : calcHi;
             pProperties->lowWaterMark = pProperties->sendQueueSize/4;
             /* use default heartBeatsPerSendQueue */
         }
    }

    if (pNDDS_Obj->pubThreadId > DEFAULT_PUB_THREADID)
    {
       pProperties->threadId = pNDDS_Obj->pubThreadId;
    }

    DPRINT4(1,"----> QueueSize: %d, low: %d, high: %d  WaterMarks, HeartBeatsPerSendQ %d\n", 
         pProperties->sendQueueSize, pProperties->lowWaterMark,pProperties->highWaterMark,pProperties->heartBeatsPerSendQueue);

    DPRINT(1,"register: nddsRegisterFunc\n");
    /* NMR_DataWordsNddsRegister(); */

    (*pNDDS_Obj->TypeRegisterFunc)();

    DPRINT1(1,"Allocate NDDS Data Type: '%s'.\n",pNDDS_Obj->dataTypeName);
    /* instance = NMR_DataWordsAllocate(); */
    /* pNDDS_Obj->instance = (*nddsTypeAlloc)(); */
    pNDDS_Obj->instance = (*pNDDS_Obj->TypeAllocFunc)();

    pNDDS_Obj->dataTypeSize = (*pNDDS_Obj->TypeSizeFunc)(0);

    NddsPublicationListenerDefaultGet(&myPublicationListener);
    if (pNDDS_Obj->pubRelStatRtn == NULL)
    {
        myPublicationListener.reliableStatusRtn = MyPublicationReliableStatusRtn;
    }
    else
    {
        myPublicationListener.reliableStatusRtn = pNDDS_Obj->pubRelStatRtn;
        myPublicationListener.reliableStatusRtnParam = pNDDS_Obj->pubRelStatParam;
    }

    DPRINT1(1,"create publication: Topic Name: '%s'\n",pNDDS_Obj->topicName);
    publication = NddsPublicationCreateAtomic(pNDDS_Obj->domain, pNDDS_Obj->topicName, 
				        	pNDDS_Obj->dataTypeName, pNDDS_Obj->instance, 
						pProperties, &myPublicationListener);

   pNDDS_Obj->publication = publication;
   if ( pNDDS_Obj->publisher != NULL)
   {
      DPRINT(3,"add to publisher: NddsPublisherPublicationAdd\n");
       NddsPublisherPublicationAdd(pNDDS_Obj->publisher, publication);
   }
   else
   {
      DPRINT(3,"No publisher for domain.\n");
   }

    DPRINT(1,"\n\n");
   return(0);
}

int createBEPublication(NDDS_ID pNDDS_Obj)
{
    NDDSPublication publication;
    /* NDDSPublicationProperties  properties; */
    NDDSPublicationProperties  *pProperties;
    NDDSPublicationListener   myPublicationListener;

    /* get default publication properties */
    /* NddsPublicationPropertiesDefaultGet(pNDDS_Obj->domain,&properties);  */
    NddsPublicationPropertiesDefaultGet(pNDDS_Obj->domain,&(pNDDS_Obj->pubProperties)); 
    pProperties = &(pNDDS_Obj->pubProperties);

    RtiNtpTimePackFromNanosec(pProperties->persistence, 30, 0);	/* 0 hours  */
    RtiNtpTimePackFromNanosec(pProperties->timeToKeepPeriod , 5, 0);	/* 5 seconds  */
    pProperties->heartBeatRetries = 30;

    if (pNDDS_Obj->publisherStrength < 1)
        pProperties->strength        = 1;
    else
        pProperties->strength        = pNDDS_Obj->publisherStrength;


    if (pNDDS_Obj->pubThreadId > DEFAULT_PUB_THREADID)
       pProperties->threadId = pNDDS_Obj->pubThreadId;

    DPRINT(1,"register: nddsRegisterFunc\n");
    (*pNDDS_Obj->TypeRegisterFunc)();

    DPRINT1(+3,"Allocate NDDS Data Type: '%s'.\n",pNDDS_Obj->dataTypeName);
    pNDDS_Obj->instance = (*pNDDS_Obj->TypeAllocFunc)();

    pNDDS_Obj->dataTypeSize = (*pNDDS_Obj->TypeSizeFunc)(0);

    NddsPublicationListenerDefaultGet(&myPublicationListener);
    myPublicationListener.reliableStatusRtn = MyPublicationReliableStatusRtn;

    DPRINT1(1,"create publication: Topic Name: '%s'\n",pNDDS_Obj->topicName);
    publication = NddsPublicationCreateAtomic(pNDDS_Obj->domain, pNDDS_Obj->topicName, 
				        	pNDDS_Obj->dataTypeName, pNDDS_Obj->instance, 
						pProperties, &myPublicationListener);


   pNDDS_Obj->publication = publication;
   if ( pNDDS_Obj->publisher != NULL)
   {
      DPRINT(3,"add to publisher: NddsPublisherPublicationAdd\n");
       NddsPublisherPublicationAdd(pNDDS_Obj->publisher, publication);
   }
   else
   {
      DPRINT(3,"No publisher for domain.\n");
   }

    DPRINT(1,"\n\n");
   return(0);
}


RTIBool nddsPublicationDestroy(NDDS_ID pNDDS_Obj)
{
    void *instance = NULL;

    if (!pNDDS_Obj->publication) {
      return RTI_FALSE;
    }

    instance = NddsPublicationInstanceGet(pNDDS_Obj->publication);

    if(NddsPublicationDestroy(pNDDS_Obj->domain, pNDDS_Obj->publication)) {
        free(instance);
        free(pNDDS_Obj);
        return RTI_TRUE;
    }

    return RTI_FALSE;
}


/*
  nddsWait4Subscribers - used to wait for a specified number of subscribers to
  appear for this publication. If timeout is exceeded it returns an error of -1
*/
int nddsWait4Subscribers(NDDS_ID pNDDS_Obj, int timeout, int numberSubscribers)
{
   RTINtpTime waitTime         = {0,0};
   int retrys = 1;

   RtiNtpTimePackFromNanosec(waitTime, timeout, 0);

   if (NddsPublicationSubscriptionWait(pNDDS_Obj->publication, waitTime, retrys, numberSubscribers) !=
                                                            NDDS_WAIT_SUCCESS) {
        /* printf("There is no subscription to the topic. Might as well exit.\n"); */
        return -1;
    }

   return(0);
}

/*
   nddsPublicationIssuesWait - will cause calling task to pend until the Publication send Q has
   dropped below or equal to the specified Q level
   e.g. nddsPublicationIssuesWait( xx, 60, 0 );
	would wait for a maximum of 60 sec for the send Q to drop to zero.
	This means all issues where sent and acknowledged by all subscribers.
*/
#ifdef VXWORKS

int nddsPublicationIssuesWait(NDDS_ID pNDDS_Obj, int timeOut, int Qlevel)
{
    int status;
    RTINtpTime maxWaitSec;
    RtiNtpTimePackFromMillisec(maxWaitSec, timeOut, 0);
    /* NddsPublicationWait (NDDSPublication publication, RTINtpTime maxWaitSec, int sendQueueLevel)  */
    status = NddsPublicationWait(pNDDS_Obj->publication, maxWaitSec, Qlevel) ;
    if (status != NDDS_PUBLICATION_SUCCESS)
    {
       status = -1;
    }
    else
       status = 0;
    return(status);
}

#else /* UNIX */

int nddsPublicationIssuesWait(NDDS_ID pNDDS_Obj, int timeOut, int Qlevel)
{
   int status, nddsstatus, leftsecs;
   RTINtpTime maxWaitSec;
    RtiNtpTimePackFromMillisec(maxWaitSec, 0, 0);
   leftsecs = timeOut;
   status = -1;
   while(leftsecs >= 0)
   {
      nddsstatus = NddsPublicationWait(pNDDS_Obj->publication, maxWaitSec, Qlevel) ;
      if (nddsstatus == NDDS_PUBLICATION_SUCCESS)
      {
       status = 0;
       break;
      }
      if (leftsecs > 0)
#ifdef VNMRS_WIN32
		Sleep(1000);  /* wait one second intervals */
#else //VNMRS_WIN32
        sleep(1);  /* wait one second intervals */
#endif //VNMRS_WIN32
      leftsecs--;
   }
   return status;
}
#endif

/*
   nddsPublishData - send publication issue immediately with th econtect of the call task
*/
int nddsPublishData(NDDS_ID pNDDS_Obj)
{
   int status = 0;
   /* printf("Send Issue\n"); */
   if ( pNDDS_Obj->publisher != NULL)
   {
       NddsPublisherSend(pNDDS_Obj->publisher);
   }
   else
   {
       if (NddsPublicationSend(pNDDS_Obj->publication) != NDDS_PUBLICATION_SUCCESS)
         status = -1;
       else
         status = 0;
   }
   return(status);
}

/*
 * NDDSPublicationReliableEvent event: The latest event on the publication's reliable stream 
 * const char* nddsTopic: The NDDSTopic of the publication 
 * int subscriptionReliable: The number of reliable subscriptions subscribed to this publication 
 * int subscriptionUnreliable:  The number of unreliable subscriptions subscribed to this publication 
 * int unacknowledgedIssues: The number of unacknowledges issues 
 */
char *getPubEventDesc(int event)
{
    int i;
    for (i=0; i < 8; i++)
    {
       if (event == pubEventDesc[i].event)
           return(pubEventDesc[i].desc);
    }
    return("Undefined");
}

printReliablePubStatus( NDDSPublicationReliableStatus *status)
{
   DPRINT4(-1,"'%s': # ReliableSubs: %d, # BESubs: %d, unack Issues: %d\n",
	status->nddsTopic,status->subscriptionReliable,status->subscriptionUnreliable, 
        status->unacknowledgedIssues);
   DPRINT2(-1,"'%s': Event: %s\n", status->nddsTopic,getPubEventDesc(status->event));
}

/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/

#else /* RTI_NDDS_4x */

/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/

/*----------------------------- Data Writer Call Backs ---------------------------------*/
// 	Handles the DDS_OFFERED_DEADLINE_MISSED_STATUS status.
void Default_OfferedDeadlineMissed(void* listener_data,
                                         DDS_DataWriter* writer,
                                         const struct   DDS_OfferedDeadlineMissedStatus *status)
{
        DPRINT(0,"OfferedDeadlineMissed\n");
        // ThroughputTestPacket_Stats *pStats = (ThroughputTestPacket_Stats*) listener_data;
        // ++(pStats->_deadlineCount);
/*
     DDS_Long 	total_count
 	Total cumulative count of the number of times the DDS_DataWriter failed 
        to write within its offered deadline.
     DDS_Long 	total_count_change
 	The incremental changes in total_count since the last time the listener 
        was called or the status was read.
     DDS_InstanceHandle_t 	last_instance_handle
 	Handle to the last instance in the DDS_DataWriter for which an offered 
        deadline was missed. 
*/
}
//	Handles the DDS_OFFERED_INCOMPATIBLE_QOS_STATUS status.
void Default_OfferedIncompatibleQos(void* listener_data,
                                         DDS_DataWriter* writer,
                                         const struct DDS_OfferedIncompatibleQosStatus *status)
{
#ifdef DEBUG
    DDS_TopicDescription *topicDesc;
    DDS_Topic *topic;

    topic = DDS_DataWriter_get_topic(writer);
    topicDesc = DDS_Topic_as_topicdescription(topic);
    DPRINT2(-4," ------ OfferedIncompatibleQos:  Type: '%s', Name: '%s' Matched -----------\n",
            DDS_TopicDescription_get_type_name(topicDesc),DDS_TopicDescription_get_name(topicDesc));
//    DPRINT2(-4,"Reason: %d, '%s'\n", status->last_policy_id, getIncompatQosDesc(status->last_policy_id));
/*
        DDS_Long 	total_count
 	   Total cumulative number of times the concerned DDS_DataWriter discovered a 
           DDS_DataReader for the same DDS_Topic, common partition with a requested QoS 
           that is incompatible with that offered by the DDS_DataWriter.
        DDS_Long 	total_count_change
 	   The incremental changes in total_count since the last time the listener was 
           called or the status was read.
        DDS_QosPolicyId_t 	last_policy_id
 	   The DDS_QosPolicyId_t of one of the policies that was found to be incompatible 
           the last time an incompatibility was detected.
        DDS_QosPolicyCountSeq 	policies
 	   A list containing for each policy the total number of times that the concerned 
           DDS_DataWriter discovered a DDS_DataReader for the same DDS_Topic and common 
           partition with a requested QoS that is incompatible with that offered by 
           the DDS_DataWriter. 
*/
#endif
}
// 	Handles the DDS_LIVELINESS_LOST_STATUS status.
void Default_LivelinessLost(void* listener_data,
                                         DDS_DataWriter* writer,
                                         const struct DDS_LivelinessLostStatus *status)
{
#ifdef DEBUG
    DDS_TopicDescription *topicDesc;
    DDS_Topic *topic;

    topic = DDS_DataWriter_get_topic(writer);
    topicDesc = DDS_Topic_as_topicdescription(topic);
    DPRINT2(-4," ------ LivelinessLost:  Type: '%s', Name: '%s' Matched -----------\n",
            DDS_TopicDescription_get_type_name(topicDesc),DDS_TopicDescription_get_name(topicDesc));
    DPRINT1(-4,"cumulative number of times DDS_DataWriter failure to to actively signal its liveliness: %d\n",
		status->total_count);
    DPRINT1(-4,"Delta number of times DDS_DataWriter failure to to actively signal its liveliness: %d\n",
		status->total_count_change);
/*
    DDS_Long 	total_count
 	Total cumulative number of times that a previously-alive DDS_DataWriter 
        became not alive due to a failure to to actively signal its liveliness 
        within the offered liveliness period.
    DDS_Long 	total_count_change
 	The incremental changees in total_count since the last time the listener 
        was called or the status was read. 
*/
#endif

}
// 	Handles the DDS_PUBLICATION_MATCHED_STATUS status.
void Default_PublicationMatched(void* listener_data,
                                         DDS_DataWriter* writer,
                                         const struct DDS_PublicationMatchedStatus *status)
{
#ifdef DEBUG
    DDS_TopicDescription *topicDesc;
    DDS_Topic *topic;

    topic = DDS_DataWriter_get_topic(writer);
    topicDesc = DDS_Topic_as_topicdescription(topic);
    DPRINT2(+1," ------ Publication Type: '%s', Name: '%s' Matched -----------\n",
            DDS_TopicDescription_get_type_name(topicDesc),DDS_TopicDescription_get_name(topicDesc));
    DPRINT1(+1,"  The total cumulative number of times the concerned DDS_DataWriter discovered a 'match' with a DDS_DataReader: %d\n",
              status->total_count);
    DPRINT1(+1,"  The incremental changes in total_count since the last time the listener was called or the status was read: %d\n",
              status->total_count_change);
    DPRINT1(+1,"  The current number of readers with which the DDS_DataWriter is matched: %d\n",
              status->current_count);
    DPRINT1(+1,"  The change in current_count since the last time the listener was called or the status was read: %d\n",
              status->current_count_change);
//    DPRINT1(+1,"  A handle to the last DDS_DataReader that caused the the DDS_DataWriter's status to change: %p\n",
//              status->last_subscription_handle);
    DPRINT(+1," ----------------------------------------\n");
#endif
}

//  	<<eXtension>> A change has occurred in the writer's cache of unacknowledged samples.
void Default_ReliableWriterCacheChanged(void* listener_data,DDS_DataWriter* writer, 
					const struct DDS_ReliableWriterCacheChangedStatus *status)
{
    /*
    DDS_TopicDescription *topicDesc;
    DDS_Topic *topic;
    * topic = DDS_DataWriter_get_topic(writer);
    * topicDesc = DDS_Topic_as_topicdescription(topic);
    */

    /* ======================================================================= */
    /*  Used in diagnostics for readMRIUserByte    GMB    3/1/08               */
    /* ======================================================================= */
    /*
    *if ( status->unacknowledged_sample_count > 0)
    *{
    *   DPRINT2(-4," ------ ReaderActivityChanged for: Type: '%s', Name: '%s'  -----------\n",
    *        DDS_TopicDescription_get_type_name(topicDesc), DDS_TopicDescription_get_name(topicDesc));
    *   DPRINT1(-4,"Current Unacknowledged samples in the writer's cache: %d\n",
    *               status->unacknowledged_sample_count);
    *}
    */
    // printf("Match Found for Publication: '%s'\n",DDS_TopicDescription_get_name(topicDesc));
/*
    printf(" ------ CacheChanged -----------\n");
    printf("  The number of times the reliable writer's cache of unacknowledged samples has become empty: %d\n",
              status->empty_reliable_writer_cache);
    printf("  The number of times the reliable writer's cache of unacknowledged samples has become full: %d\n",
              status->full_reliable_writer_cache);
    printf("  The number of times the reliable writer's cache of unacknowledged samples has fallen to the low watermark: %d\n",
              status->low_watermark_reliable_writer_cache);
    printf("  The number of times the reliable writer's cache of unacknowledged samples has risen to the high watermark: %d\n",
              status->high_watermark_reliable_writer_cache);
    printf("  The current number of unacknowledged samples in the writer's cache: %d\n",
              status->unacknowledged_sample_count);
    printf(" ----------------------------------------\n");
*/
}

//  	<<eXtension>> A matched reliable reader has become active or become inactive. 
void Default_ReliableReaderActivityChanged(void* listener_data,DDS_DataWriter* writer,
					const struct DDS_ReliableReaderActivityChangedStatus *status)
{
#ifdef DEBUG
    DDS_TopicDescription *topicDesc;
    DDS_Topic *topic;

    topic = DDS_DataWriter_get_topic(writer);
    topicDesc = DDS_Topic_as_topicdescription(topic);
    DPRINT2(+1," ------ ReaderActivityChanged for: Type: '%s', Name: '%s'  -----------\n",
        DDS_TopicDescription_get_type_name(topicDesc), DDS_TopicDescription_get_name(topicDesc));
    // printf(" ------ ReaderActivityChanged -----------\n");
    DPRINT1(+1,"  The current number of reliable readers currently matched with this reliable writer: %d\n",
              status->active_count);
    DPRINT1(+1,"  The number of reliable readers that have been dropped by this reliable writer because they failed to send acknowledgements in a timely fashion: %d\n",
              status->inactive_count);
    DPRINT1(+1,"  The most recent change in the number of active remote reliable readers: %d\n",
              status->active_count_change);
    DPRINT1(+1,"  The most recent change in the number of inactive remote reliable readers: %d\n",
              status->inactive_count_change);
//    DPRINT1(+1,"  The instance handle of the last reliable remote reader to be determined inactive: %p\n",
//              status->last_instance_handle);
    DPRINT(+1," ----------------------------------------\n");
#endif
}

/*******************************************************/
/* Attach Routines for callbacks of dataWriterListener */
/*******************************************************/

void attachPublicationMatchedCallback(NDDS_ID pNDDS_Obj, 
              DDS_DataWriterListener_PublicationMatchedCallback callback)
{
   pNDDS_Obj->pDWriterListener->on_publication_matched = callback;
}

void attachOfferedDeadlineMissedCallback(NDDS_ID pNDDS_Obj, 
              DDS_DataWriterListener_OfferedDeadlineMissedCallback callback)
{
   pNDDS_Obj->pDWriterListener->on_offered_deadline_missed = callback;
}

void attachOfferedIncompatibleQosCallback(NDDS_ID pNDDS_Obj, 
              DDS_DataWriterListener_OfferedIncompatibleQosCallback callback)
{
   pNDDS_Obj->pDWriterListener->on_offered_incompatible_qos = callback;
}


void attachLivelinessLostCallback(NDDS_ID pNDDS_Obj, 
              DDS_DataWriterListener_LivelinessLostCallback callback)
{
   pNDDS_Obj->pDWriterListener->on_liveliness_lost = callback;
}

// <<eXtension>> A change has occurred in the writer's cache of unacknowledged samples.
void attachReliableWriterCacheChangedCallback(NDDS_ID pNDDS_Obj, 
              DDS_DataWriterListener_ReliableWriterCacheChangedCallback callback)
{
   pNDDS_Obj->pDWriterListener->on_reliable_writer_cache_changed = callback;
}

void attachReliableReaderActivityChangedCallback(NDDS_ID pNDDS_Obj, 
              DDS_DataWriterListener_ReliableReaderActivityChangedCallback callback)
{
   pNDDS_Obj->pDWriterListener->on_reliable_reader_activity_changed = callback;
}

void attachDWriterUserData(NDDS_ID pNDDS_Obj, void *pUserdata)
{
   pNDDS_Obj->pDWriterListener->as_listener.listener_data = pUserdata;
}

/* ------------------------------------------------------------------------------------*/

void initDataWriterListener(NDDS_ID pNDDS_Obj)
{
    struct DDS_DataWriterListener *pWriter_listener = 
              (struct DDS_DataWriterListener*) malloc(sizeof(struct DDS_DataWriterListener));
    // DDS_DataWriterListener_initialize(pWriter_listener);

    pNDDS_Obj->pDWriterListener = pWriter_listener;

    pWriter_listener->as_listener.listener_data = (void*) NULL;

    /* setup the default callbacks, these can be overridden prior to the enablePublisher call */
    pWriter_listener->on_offered_deadline_missed = Default_OfferedDeadlineMissed;
    pWriter_listener->on_offered_incompatible_qos = Default_OfferedIncompatibleQos;
    pWriter_listener->on_liveliness_lost = Default_LivelinessLost;
    pWriter_listener->on_publication_matched = Default_PublicationMatched;
    pWriter_listener->on_reliable_writer_cache_changed = Default_ReliableWriterCacheChanged;
    pWriter_listener->on_reliable_reader_activity_changed = Default_ReliableReaderActivityChanged;

}

void attachDWDiscvryUserData(NDDS_ID pNDDS_Obj, void *pUserdata, long length)
{
    DDS_OctetSeq_ensure_length(&(pNDDS_Obj->pDWriterQos->user_data.value), length, length);
    if (!DDS_OctetSeq_from_array(&(pNDDS_Obj->pDWriterQos->user_data.value), pUserdata, length)) 
    {
        errLogRet(LOGIT,debugInfo,"attachDWDiscvryUserData: failed setting discovery user data \n"); 
    }
}

/*
   initialize the publication Qos for Reliably publication
*/
int initPublication(NDDS_ID pNDDS_Obj)
{
    DDS_ReturnCode_t retcode;

    struct DDS_DataWriterQos *pWriter_qos = (struct DDS_DataWriterQos*) malloc(sizeof(struct DDS_DataWriterQos));
    DDS_DataWriterQos_initialize(pWriter_qos);

    pNDDS_Obj->pDWriterQos = pWriter_qos;

    /* Need only one Publisher per App so create the Publisher only if required */
    if (pNDDS_Obj->pPublisher == NULL)
        pNDDS_Obj->pPublisher = PublisherCreate(pNDDS_Obj->pParticipant);

    pNDDS_Obj->pTopic = RegisterAndCreateTopic(pNDDS_Obj);

    /* get default data writer properties */
    retcode = DDS_Publisher_get_default_datawriter_qos(pNDDS_Obj->pPublisher, pWriter_qos);
    if (retcode != DDS_RETCODE_OK) {
      errLogRet(LOGIT,debugInfo,"initPublication: failed to get default datawriter qos\n"); 
      NDDS_Shutdown(pNDDS_Obj);
      return -1;
    }

    /* set datawriters QOS for FULL reliability */
    pWriter_qos->reliability.kind = DDS_RELIABLE_RELIABILITY_QOS;
    pWriter_qos->reliability.max_blocking_time.sec = 86400;  /* 24 hours */
    pWriter_qos->reliability.max_blocking_time.nanosec = 0;

    pWriter_qos->history.kind = DDS_KEEP_ALL_HISTORY_QOS;
    // pWriter_qos->history.depth ignored when kind == DDS_KEEP_ALL_HISTORY_QOS, spec by resource settings
    pWriter_qos->protocol.push_on_write = DDS_BOOLEAN_TRUE;
    /* The above Qos will insure that a publisher will pend indefinitely to send */
    /* block the Send call for up to sendMaxWait seconds when the queue is full */
    /* RtiNtpTimePackFromNanosec(pProperties->sendMaxWait,86400,0); */

    /* Set for Exclusive NOT Shared ownership;  for keyed topic */
    pWriter_qos->ownership.kind = DDS_EXCLUSIVE_OWNERSHIP_QOS;
    if (pNDDS_Obj->publisherStrength < 1)
    {
        pWriter_qos->ownership_strength.value = 1;
    }
    else
    {
        pWriter_qos->ownership_strength.value = pNDDS_Obj->publisherStrength;
    }

    /* the time before a subscriber will switch to a new publisher after last pub from */
    /* a previous publisher */
    /* after 10 sec liveliness loss detected */
    //pWriter_qos->liveliness.lease_duration.sec = 10;
    //pWriter_qos->liveliness.lease_duration.sec = 2;   // for readMRIUserByte testing
    // pWriter_qos->liveliness.lease_duration.nanosec = 0;
    /* ================================================================================= */
    /* For readMRIUserByte, we need to turn off the liveness HB that are recieved and    */
    /* handle by the evt task that has been interfering with the UserByte data receive   */
    /* task.  To achieve this, set lease duration to infinite (which is its default)     */
    /* in addition the kind is set from automatic to manual by participant, thus as long as */
    /* any pub has published on this participant then all publisher on this participant are */
    /* considered alive.                GMB  3/5/2008                                    */ 
    /* ================================================================================= */

    pWriter_qos->liveliness.kind = DDS_MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
    pWriter_qos->liveliness.lease_duration = DDS_DURATION_INFINITE;   // for readMRIUserByte testing

    /* ================================================================================= */
    /* ================================================================================= */

    /* I believe for 4x this should be left to the default of Infinity */
    // RtiNtpTimePackFromNanosec(pProperties->timeToKeepPeriod , 0, 0);	/* default  */
    // pWriter_qos->lifespan.duration.sec = 0;
    // pWriter_qos->lifespan.duration.nanosec = 0;   /* 16 milliseconds */

    pWriter_qos->protocol.push_on_write = DDS_BOOLEAN_TRUE;

    // pWriter_qos->protocol.rtps_reliable_writer.heartbeat_period.sec = 24 * 3600;  // readMRIUserByte test
    pWriter_qos->protocol.rtps_reliable_writer.heartbeat_period.sec = 1; /* no-acode bug change  */
    pWriter_qos->protocol.rtps_reliable_writer.heartbeat_period.nanosec = 0;

    // only in 4.2x
#ifndef RTI_NDDS41
    // pWriter_qos->protocol.rtps_reliable_writer.late_joiner_heartbeat_period.sec = 24 * 3600; // readMRIUserByte test
    pWriter_qos->protocol.rtps_reliable_writer.late_joiner_heartbeat_period.sec = 1;
    pWriter_qos->protocol.rtps_reliable_writer.late_joiner_heartbeat_period.nanosec = 0;
#endif

    /* ================================================================================= */
    /* readMIRUserByte test (RTI)  2/15/08 */
    // pWriter_qos->protocol.rtps_reliable_writer.min_nack_response_delay.sec = 0;
    // pWriter_qos->protocol.rtps_reliable_writer.min_nack_response_delay.nanosec = 5000;  // 5usec
    /* ================================================================================= */
    /* make sure nack response delay is zero so repairs are done within the receive task */
    /* rather than the evt task                                                          */                      
    /* not an issue for readMIRUserByte, return to default values, min =0, max = 0.2 sec */
    /* ================================================================================= */
    // pWriter_qos->protocol.rtps_reliable_writer.min_nack_response_delay.sec = 0;
    // pWriter_qos->protocol.rtps_reliable_writer.min_nack_response_delay.nanosec = 0; 
    // pWriter_qos->protocol.rtps_reliable_writer.max_nack_response_delay.sec = 0;
    // pWriter_qos->protocol.rtps_reliable_writer.max_nack_response_delay.nanosec = 0;
    /* ================================================================================= */

    /* changes from NDDS benchmark results */
    //   readMRIUserByte testing   GMB 3/4/2008
    // pWriter_qos->protocol.rtps_reliable_writer.fast_heartbeat_period.sec = 24 * 3600;  // readMRIUserByte test
    // pWriter_qos->protocol.rtps_reliable_writer.fast_heartbeat_period.nanosec = 0;

    pWriter_qos->protocol.rtps_reliable_writer.fast_heartbeat_period.sec = 0;
    pWriter_qos->protocol.rtps_reliable_writer.fast_heartbeat_period.nanosec = 250000000;

    /* 250 ms * 10000 = 2,500,000 ms or 2,500 seconds or ~ 41 minutes before sub is considered gone */
    /* pProperties->heartBeatRetries = 10000; */
    pWriter_qos->protocol.rtps_reliable_writer.max_heartbeat_retries = 1000;


    /*  readMIRUserByte test (RTI)  2/29/08 */
    /* pWriter_qos->protocol.rtps_reliable_writer.max_bytes_per_nack_response = 128; */

    /* The assumption for a writer is that it deals in only one instance of an issue 
       so we generate only one instance and allow a maximum of two just in case */
    pWriter_qos->resource_limits.initial_instances = 1;
    pWriter_qos->resource_limits.max_instances = 2;

    DPRINT4(1,"----> QueueSize: %d, low: %d, high: %d  WaterMarks, HeartBeatsPerSendQ %d\n", 
         pNDDS_Obj->queueSize, pNDDS_Obj->lowWaterMark,pNDDS_Obj->highWaterMark,pNDDS_Obj->AckRequestsPerSendQueue);

    /* if Q Size  is zero then use all default values */
    if (pNDDS_Obj->queueSize <= 0)
    {
         /* pProperties->sendQueueSize = PUB_DEFAULT_QUEUE_SIZE; */
         /* The initial number of sample resources allocated */
         pWriter_qos->resource_limits.initial_samples = PUB_DEFAULT_QUEUE_SIZE;
         /* the total samples for an instance that the DataWriter can queue/track */
         pWriter_qos->resource_limits.max_samples_per_instance = PUB_DEFAULT_QUEUE_SIZE;
         /* the total samples for all instances a DataWriter can queue/track */
         pWriter_qos->resource_limits.max_samples =            /* max samples per instance * max instances */
                      pWriter_qos->resource_limits.max_samples_per_instance * 
                                          pWriter_qos->resource_limits.max_instances;

         pWriter_qos->protocol.rtps_reliable_writer.high_watermark = PUB_DEFAULT_HWATER_MARK;
         pWriter_qos->protocol.rtps_reliable_writer.low_watermark = PUB_DEFAULT_LWATER_MARK;
         //  to obtains the expected number of piggyback acks as in 3x 
         // piggybacks are base on resource_limits.max_samples NOT resource_limits.max_samples_per_instance
         // Thus we must be consistant with the calculation for max_sample above with request Ack per sendQ
         pWriter_qos->protocol.rtps_reliable_writer.heartbeats_per_max_samples = PUB_DEFAULT_ACKS_PER_SENDQ *
                                    pWriter_qos->resource_limits.max_instances;
    }
    else
    {
         /* pProperties->sendQueueSize = pNDDS_Obj->queueSize; */
         pWriter_qos->resource_limits.initial_samples = pNDDS_Obj->queueSize;
         pWriter_qos->resource_limits.max_samples_per_instance = pNDDS_Obj->queueSize;
         pWriter_qos->resource_limits.max_samples =  pNDDS_Obj->queueSize *  
                                       pWriter_qos->resource_limits.max_instances;

         /* if highWaterMark is set then assume all paraemters have been specidied and use them */
         if (pNDDS_Obj->highWaterMark > 0 )
         {
             /* pProperties->highWaterMark   = pNDDS_Obj->highWaterMark ; */
             /* pProperties->lowWaterMark = pNDDS_Obj->lowWaterMark; */
             pWriter_qos->protocol.rtps_reliable_writer.high_watermark = pNDDS_Obj->highWaterMark;
             pWriter_qos->protocol.rtps_reliable_writer.low_watermark = pNDDS_Obj->lowWaterMark;
             if (pNDDS_Obj->AckRequestsPerSendQueue > 0)
             {
                /* Note: for 4x, heartbeats_per_max_samples<=::DDS_DataWriterQos::resource_limits.max_samples
                 * If set to the same as maximum, one heartbeat will be sent along with each sample. 
                 * If set to zero, no piggyback heartbeat will be sent. 
                 * NOTE: number of piggy backs is based on resource_limits.max_samples NOT
                 *       initial_samples.
                 */
                pWriter_qos->protocol.rtps_reliable_writer.heartbeats_per_max_samples = 
                      pNDDS_Obj->AckRequestsPerSendQueue *  pWriter_qos->resource_limits.max_instances;
             }
             else
             {
                int acks;
                acks =  pWriter_qos->resource_limits.max_samples / 2;
                pWriter_qos->protocol.rtps_reliable_writer.heartbeats_per_max_samples = 
                           (acks > 0) ? acks : 1;  // at least once minimum
             }
         }
         else
         {
             /* if highWaterMark NoT set then calculate all paraemters */
             int calcHi, acks;
             /* calcHi = (3*pProperties->sendQueueSize)/4; */
             calcHi = (3*pWriter_qos->resource_limits.max_samples_per_instance)/4;
             /* pProperties->highWaterMark = (calcHi == 0) ? pProperties->sendQueueSize : calcHi; */
             pWriter_qos->protocol.rtps_reliable_writer.high_watermark = 
                  (calcHi == 0) ? pWriter_qos->resource_limits.max_samples_per_instance : calcHi;
             /* pProperties->lowWaterMark = pProperties->sendQueueSize/4; */
             pWriter_qos->protocol.rtps_reliable_writer.low_watermark = 
                  pWriter_qos->resource_limits.max_samples_per_instance / 4;
             /* calc default heartBeatsPerSendQueue, i.e. every other issue */
             acks =  pWriter_qos->resource_limits.max_samples / 2;
             pWriter_qos->protocol.rtps_reliable_writer.heartbeats_per_max_samples = 
                        (acks > 0) ? acks : 1;  // at least once minimum
         }
    }

    /*  not useful in 4x */
    /*if (pNDDS_Obj->pubThreadId > DEFAULT_PUB_THREADID)
     * {
     *   pProperties->threadId = pNDDS_Obj->pubThreadId;
     * }
     */

    DPRINT4(1,"----> QueueSize: %d, low: %d, high: %d  WaterMarks, HeartBeatsPerSendQ %d\n", 
         pWriter_qos->resource_limits.max_samples_per_instance,
         pWriter_qos->protocol.rtps_reliable_writer.low_watermark,
         pWriter_qos->protocol.rtps_reliable_writer.high_watermark,
         pWriter_qos->protocol.rtps_reliable_writer.heartbeats_per_max_samples);

    /* (*pNDDS_Obj->TypeRegisterFunc)(); alsready done in RegisterAndCreateTopic() */

    // DPRINT1(1,"Allocate NDDS Data Type: '%s'.\n",pNDDS_Obj->dataTypeName);
    /* instance = NMR_DataWordsAllocate(); */
    /* pNDDS_Obj->instance = (*nddsTypeAlloc)(); */
    pNDDS_Obj->instance = (*pNDDS_Obj->TypeAllocFunc)(DDS_BOOLEAN_TRUE);

    /* can't find a 4x equivilent yet...  */
    /* pNDDS_Obj->dataTypeSize = (*pNDDS_Obj->TypeSizeFunc)(0); */

    /* For NDDS 4x there is not a single callback for status but one for each type
     * of status available thus setting up callback must be left up to the App
     * via direct call to functions to install callback before actually creating the DataWriter
     */ 

    /* setup the default callbacks, these can be overridden prior to the enablePublisher call */
    initDataWriterListener(pNDDS_Obj);

   return(0);
}

// int createBEPublication(NDDS_ID pNDDS_Obj)
int initBEPublication(NDDS_ID pNDDS_Obj)
{
    DDS_ReturnCode_t retcode;

    struct DDS_DataWriterQos *pWriter_qos = (struct DDS_DataWriterQos*) malloc(sizeof(struct DDS_DataWriterQos));
    DDS_DataWriterQos_initialize(pWriter_qos);

    pNDDS_Obj->pDWriterQos = pWriter_qos;

    /* Need only one Publisher per App so create the Publisher only if required */
    if (pNDDS_Obj->pPublisher == NULL)
        pNDDS_Obj->pPublisher = PublisherCreate(pNDDS_Obj->pParticipant);

    pNDDS_Obj->pTopic = RegisterAndCreateTopic(pNDDS_Obj);

    /* get default data writer properties */
    retcode = DDS_Publisher_get_default_datawriter_qos(pNDDS_Obj->pPublisher, pWriter_qos);
    if (retcode != DDS_RETCODE_OK) {
      errLogRet(LOGIT,debugInfo,"initBEPublication: failed to get default datawriter qos\n"); 
      NDDS_Shutdown(pNDDS_Obj);
      return -1;
    }

    /* set datawriters QOS for Best Effort (default) */
    pWriter_qos->reliability.kind = DDS_BEST_EFFORT_RELIABILITY_QOS;
    pWriter_qos->history.kind = DDS_KEEP_LAST_HISTORY_QOS;
    /* DDS_KEEP_LAST_HISTORY_QOS and depth to 1 (defaults) */
    pWriter_qos->history.depth = 1;

    pWriter_qos->protocol.push_on_write = DDS_BOOLEAN_TRUE;

    /* Set for Exclusive NOT Shared ownership;  for keyed topic */
    pWriter_qos->ownership.kind = DDS_EXCLUSIVE_OWNERSHIP_QOS;
    if (pNDDS_Obj->publisherStrength < 1)
    {
        pWriter_qos->ownership_strength.value = 1;
    }
    else
    {
        pWriter_qos->ownership_strength.value = pNDDS_Obj->publisherStrength;
    }
    /* after 10 sec liveliness loss detected */
    // pWriter_qos->liveliness.lease_duration.sec = 2;     // for readMRIUserByte testing
    //pWriter_qos->liveliness.lease_duration.sec = 10;
    // pWriter_qos->liveliness.lease_duration.nanosec = 0;
    /* ================================================================================= */
    /* For readMRIUserByte, we need to turn off the liveness HB that are recieved and    */
    /* handle by the evt task that has been interfering with the UserByte data receive   */
    /* task.  To achieve this, set lease duration to infinite (which is its default)     */
    /* in addition the kind is set from automatic to manual by participant, thus as long as */
    /* any pub has published on this participant then all publisher on this participant are */
    /* considered alive.                GMB  3/5/2008                                    */ 
    /* ================================================================================= */

    pWriter_qos->liveliness.kind = DDS_MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
    pWriter_qos->liveliness.lease_duration = DDS_DURATION_INFINITE;   // for readMRIUserByte testing

    /* ================================================================================= */
    /* ================================================================================= */

    // RtiNtpTimePackFromNanosec(pProperties->timeToKeepPeriod , 0, 0);	/* default  */
    pWriter_qos->lifespan.duration.sec = 5;
    pWriter_qos->lifespan.duration.nanosec = 0; 

    pNDDS_Obj->instance = (*pNDDS_Obj->TypeAllocFunc)(DDS_BOOLEAN_TRUE);

    return 0;
}

// enablePublication(NDDS_ID pNDDS_Obj)
int createPublication(NDDS_ID pNDDS_Obj)
{

   DPRINT4(+1,"publisher: %p, tope: %p, Qos: %p, Listener: %p\n",
                   pNDDS_Obj->pPublisher, pNDDS_Obj->pTopic,pNDDS_Obj->pDWriterQos,pNDDS_Obj->pDWriterListener);
   pNDDS_Obj->pDWriter = DDS_Publisher_create_datawriter(pNDDS_Obj->pPublisher, pNDDS_Obj->pTopic,
                                             pNDDS_Obj->pDWriterQos, /*  &DDS_DATAWRITER_QOS_DEFAULT */
                                             pNDDS_Obj->pDWriterListener /* or NULL */,
                                             DDS_STATUS_MASK_ALL);

                                             // DDS_PUBLICATION_MATCHED_STATUS);
                                             // DDS_STATUS_MASK_NONE);
                                             // DDS_STATUS_MASK_ALL);


   DPRINT(+1,"created...\n");
   if (pNDDS_Obj->pDWriter == NULL) {
        errLogRet(LOGIT,debugInfo,"createPublication: create_datawriter error\n"); 
        NDDS_Shutdown(pNDDS_Obj);
        return (-1);
    }
    return 0;
  
}

/*
  nddsWait4Subscribers - used to wait for a specified number of subscribers to
  appear for this publication. If timeout is exceeded it returns an error of -1
*/
    /*
      DDS_ReturnCode_t 	
          DDS_DataWriter_get_publication_matched_status 
            (DDS_DataWriter *self, struct DDS_PublicationMatchedStatus *status)
      DDS_ReturnCode_t 	
          DDS_DataWriter_get_reliable_reader_activity_changed_status (DDS_DataWriter *self, 
            struct DDS_ReliableReaderActivityChangedStatus *status)
 	  Get the reliable reader activity changed status for this writer. 
    */
int nddsWait4Subscribers(NDDS_ID pNDDS_Obj, int timeOutInSec, int numberSubscribers)
{
   int status, leftsecs;
   DDS_ReturnCode_t retcode;
   //struct DDS_PublicationMatchedStatus PMStatus;
   struct DDS_ReliableReaderActivityChangedStatus RRACStatus;

   leftsecs = timeOutInSec;
   status = -1;
   while(leftsecs >= 0)
   {
      retcode = DDS_DataWriter_get_reliable_reader_activity_changed_status(pNDDS_Obj->pDWriter, &RRACStatus);
      if (retcode != DDS_RETCODE_OK) 
      {
         DPRINT1(-5,"nddsWait4Subscribers: get_reader_activity_changed_status failed. err: %d\n",retcode);
         continue;
      }
      DPRINT2(+1,"nddsWait4Subscribers: Current Subs: %d, wait for: %d\n", 
           RRACStatus.active_count, numberSubscribers);
      if (RRACStatus.active_count == numberSubscribers)
      {
       status = 0;
       break;
      }
      if (leftsecs > 0)
#ifndef VXWORKS
   #ifdef VNMRS_WIN32
        Sleep(1000);  /* wait one second intervals */
   #else //VNMRS_WIN32
        sleep(1);  /* wait one second intervals */
   #endif //VNMRS_WIN32
#else  // VXWORKS
        taskDelay(calcSysClkTicks(1000)); /* 1 sec delay */
#endif // VXWORKS
      leftsecs--;
   }
   return status;
}

/*
   nddsPublicationIssuesWaitAll - will cause calling task to pend until the Publication send Q has
   dropped to Zero
   Equivilent ot e.g. nddsPublicationIssuesWait( xx, 60, 0 );
	would wait for a maximum of 60 sec for the send Q to drop to zero.
	This means all issues where sent and acknowledged by all subscribers.
*/
int nddsPublicationIssuesWaitAll(NDDS_ID pNDDS_Obj, int timeOutInSec)
{
   DDS_ReturnCode_t retcode;
   struct DDS_Duration_t timeOut;

   timeOut.sec = timeOutInSec;
   timeOut.nanosec = 0;
   // Blocks the calling thread until all data written by reliable DDS_DataWriter 
   //   entity is acknowledged, or until timeout expires. 
   retcode = DDS_DataWriter_wait_for_acknowledgments (pNDDS_Obj->pDWriter, &timeOut);
   if (retcode == DDS_RETCODE_OK)
       return 0;
   else if (retcode == DDS_RETCODE_TIMEOUT)
       return -1;
   else
   {
       DPRINT1(-5,"nddsPublicationIssuesWaitAll: DDS_DataWriter_wait_for_acknowledgments error: %d\n",retcode);
       return -2;
   }
}

/*
   nddsPublicationIssuesWait - will cause calling task to pend until the Publication send Q has
   dropped below or equal to the specified Q level
   e.g. nddsPublicationIssuesWait( xx, 60, 0 );
	would wait for a maximum of 60 sec for the send Q to drop to zero.
	This means all issues where sent and acknowledged by all subscribers.
*/
/*
     DDS_ReturnCode_t 	
        DDS_DataWriter_wait_for_acknowledgments (DDS_DataWriter *self, const struct DDS_Duration_t *max_wait)
 	      Blocks the calling thread until all data written by reliable DDS_DataWriter 
              entity is acknowledged, or until timeout expires. 

    DDS_ReturnCode_t 	
        DDS_DataWriter_get_reliable_writer_cache_changed_status (DDS_DataWriter *self, 
           struct DDS_ReliableWriterCacheChangedStatus *status)
 	      Get the reliable cache status for this writer. 
*/
int nddsPublicationIssuesWait(NDDS_ID pNDDS_Obj, int timeOutInSec, int Qlevel)
{
   int status, leftsecs;
   DDS_ReturnCode_t retcode;
   struct DDS_ReliableWriterCacheChangedStatus CCStatus;

   /* if wait for all issue to be sent, call the more direct method for this purpose */
   if (Qlevel == 0)
   {
       return(nddsPublicationIssuesWaitAll(pNDDS_Obj, timeOutInSec));
   }

   leftsecs = timeOutInSec;
   status = -1;
   while(leftsecs >= 0)
   {
      retcode = DDS_DataWriter_get_reliable_writer_cache_changed_status(pNDDS_Obj->pDWriter, &CCStatus);
      if (retcode != DDS_RETCODE_OK)
      {
         DPRINT1(-4,"nddsPublicationIssuesWait: get_cache_changed_status failed. err: %d\n",retcode);
         continue;
      }
      DPRINT2(+1,"nddsWait4Subscribers: Current UnAck: %d, wait for: %d\n", 
               CCStatus.unacknowledged_sample_count, Qlevel);
      if (CCStatus.unacknowledged_sample_count == Qlevel)
      {
       status = 0;
       break;
      }
      if (leftsecs > 0)
#ifndef VXWORKS
   #ifdef VNMRS_WIN32
        Sleep(1000);  /* wait one second intervals */
   #else //VNMRS_WIN32
        sleep(1);  /* wait one second intervals */
   #endif //VNMRS_WIN32
#else  // VXWORKS
        taskDelay(calcSysClkTicks(1000)); /* 1 sec delay */
#endif // VXWORKS
      leftsecs--;
   }
   return status;
}

/*
   nddsPublishData - send publication issue immediately with th econtect of the call task
*/
int nddsPublishData(NDDS_ID pNDDS_Obj)
{
/*
*  DDS_ReturnCode_t result;
*  DDS_InstanceHandle_t instance_handle = DDS_HANDLE_NIL;
*  Monitor_CmdDataWriter *Monitor_Cmd_writer = NULL;
*
*   Monitor_Cmd_writer = Monitor_CmdDataWriter_narrow(pNDDS_Obj->pDWriter);
*   if (Monitor_Cmd_writer == NULL) {
*        printf("DataWriter narrow error\n");
*        // publisher_shutdown(participant);
*        return -1;
*    }
*
*
*   result = Monitor_CmdDataWriter_write(Monitor_Cmd_writer,
*                pIssue,&instance_handle);
*   if (result != DDS_RETCODE_OK) {
*            printf("write error %d\n", result);
*   }
*/
   return(0);

}

int nddsPublicationDestroy(NDDS_ID pNDDS_Obj)
{
   return(0);
}

#endif /* RTI_NDDS_4x */

#ifdef __cplusplus
}
#endif

