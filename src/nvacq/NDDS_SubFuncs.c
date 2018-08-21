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
#include <stdio.h>
#include <stdlib.h>
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

#define SUB_DEFAULT_QUEUE_SIZE 10

#ifdef __cplusplus
extern "C" {
#endif

#ifndef RTI_NDDS_4x

typedef struct _status_desc_ {  int status; char* desc; } STATUS_DESC;

static STATUS_DESC  statusDesc[5] = { 
       { NDDS_NEVER_RECEIVED_DATA, "Never received an issue, but a deadline occurred" },
       { NDDS_NO_NEW_DATA, "A deadline occurred since the last issue received" },
       { NDDS_UPDATE_OF_OLD_DATA, "Received a new issue, whose time stamp is the same or older than the last fresh issue" },
       { NDDS_FRESH_DATA, "A new issue received" },
       { NDDS_DESERIALIZATION_ERROR, "The deserialization method for the NDDSType returned an error" }
};

#define SUB_DEFAULT_Q_SIZE 20

/*
 *    A default reliable subscripion status call back routine 
*/
void MySubscriptionReliableStatusRtn(NDDSSubscriptionReliableStatus *status,
                                     void *userParam)
{
    switch (status->event) {
      case NDDS_ISSUES_DROPPED:
         DPRINT2(0,"'%s': %d Issues Dropped\n",status->nddsTopic,status->issuesDropped);
         break;
      case NDDS_PUBLICATION_NEW:
        DPRINT2(0,"A new publication took over my Subscription: '%s', # of dropped issues: %d.\n",
               status->nddsTopic,status->issuesDropped);
        break;
      default:  /* Do nothing */
        DPRINT2(0,"Unknown Reliable Sub. Status Event Occured for: '%s', # of dropped issues: %d.\n",
             status->nddsTopic,status->issuesDropped);
        break;
    }
}
 

/*
   create a subscription
*/
int createSubscription(NDDS_ID pNDDS_Obj)
{
    int millisec, seconds;
    NDDSSubscription subscription;
    NDDSSubscriptionProperties  *pProperties;
    /* NDDSIssueListener issueListener; */
    NDDSSubscriptionReliableListener  reliableListener;
    /* unsigned int multicastIP = NddsStringToAddress("225.0.0.1"); */
    unsigned int interfaceIP = NDDS_USE_UNICAST;

    DPRINT1(+1,"createSubscription: Topic name: '%s'\n",pNDDS_Obj->topicName);

    /* Obtain the default Subscription properties of System */
    NddsSubscriptionPropertiesDefaultGet(pNDDS_Obj->domain,&(pNDDS_Obj->subProperties));
    pProperties = &(pNDDS_Obj->subProperties);

    /* from optimized large packet */
    pProperties->receiveQueueSize =  ( pNDDS_Obj->queueSize > 0) ?
                 pNDDS_Obj->queueSize : SUB_DEFAULT_Q_SIZE;

    /* deadline pasts then subscription is nolonger present */
    if (pNDDS_Obj->BE_DeadlineMillisec == 0)  /* zero use default 20 seconds */
    {
       RtiNtpTimePackFromNanosec(pProperties->deadline, 20, 0);
    }
    else
    {
      seconds = pNDDS_Obj->BE_DeadlineMillisec  / 1000;
      millisec = pNDDS_Obj->BE_DeadlineMillisec % 1000;
      DPRINT2(+1,"BE: Deadline: secs: %d, millisec: %d\n",seconds,millisec);
      RtiNtpTimePackFromMillisec(pProperties->deadline, seconds, millisec);
    }

    /* Modify defaults to our required properties */
    pProperties->mode = NDDS_SUBSCRIPTION_IMMEDIATE;
    /* properties.mode = NDDS_SUBSCRIPTION_POLLED; */

    NddsSubscriptionIssueListenerDefaultGet(&(pNDDS_Obj->issueListener));
    pNDDS_Obj->issueListener.recvCallBackRtn = pNDDS_Obj->callBkRtn;
    pNDDS_Obj->issueListener.recvCallBackRtnParam = pNDDS_Obj->callBkRtnParam;

    reliableListener.ReliableStatusRtn =  MySubscriptionReliableStatusRtn;
    reliableListener.ReliableStatusRtnParam = NULL;

    DPRINT1(+3,"createSubscription: Register NDDS Data Type: '%s'.\n",pNDDS_Obj->dataTypeName);
    (*pNDDS_Obj->TypeRegisterFunc)();
    DPRINT1(+3,"createSubscription: Allocate NDDS Data Type: '%s'.\n",pNDDS_Obj->dataTypeName);
    pNDDS_Obj->instance = (*pNDDS_Obj->TypeAllocFunc)();

    if ( strlen(pNDDS_Obj->MulticastSubIP) > 5)
    {
      interfaceIP = NddsStringToAddress(pNDDS_Obj->MulticastSubIP);
      DPRINT1(+1,"createSubscription: MultiCast IP: '%s'\n",pNDDS_Obj->MulticastSubIP);
    }
    else
    {
      DPRINT(+3,"createSubscription: UNICAST  IP\n");
      interfaceIP = NDDS_USE_UNICAST;
     }

    subscription = NddsSubscriptionReliableCreateAtomic(pNDDS_Obj->domain, 
					  pNDDS_Obj->topicName,  /* "Reliable HelloMsg", */
                                          pNDDS_Obj->dataTypeName, pNDDS_Obj->instance,
                                          pProperties, &(pNDDS_Obj->issueListener),
                                          &reliableListener, interfaceIP);

    pNDDS_Obj->subscription = subscription;

    /* add subsciption to subscriber if there is one */
    if ( pNDDS_Obj->subscriber != NULL )
    {
      DPRINT(+3,"add to subscriber: NddsSubscriberSubscriptionAdd\n");
      NddsSubscriberSubscriptionAdd(pNDDS_Obj->subscriber,subscription);
    }
    else
    {
    DPRINT(+3,"createSubscription: No subscriber\n");
    }
    return(0);
}

int createBESubscription(NDDS_ID pNDDS_Obj)
{
    int millisec, seconds;
    NDDSSubscription subscription;
    NDDSSubscriptionProperties  *pProperties;
    /* NDDSIssueListener issueListener; */
    unsigned int interfaceIP = NDDS_USE_UNICAST;

    DPRINT1(+1,"createBESubscription: Topic name: '%s'\n",pNDDS_Obj->topicName);

    /* Obtain the default Subscription properties of System */
    NddsSubscriptionPropertiesDefaultGet(pNDDS_Obj->domain,&(pNDDS_Obj->subProperties));
    pProperties = &(pNDDS_Obj->subProperties);

    /* Modify defaults to our required properties */
    pProperties->mode = NDDS_SUBSCRIPTION_IMMEDIATE;

    /* subscriber update rate,  20 milliseconds */
    seconds = pNDDS_Obj->BE_UpdateMinDeltaMillisec  / 1000;
    millisec = pNDDS_Obj->BE_UpdateMinDeltaMillisec % 1000;
    DPRINT2(+1,"BE: minimumSeparation: secs: %d, millisec: %d\n",seconds,millisec);
    if ( (seconds == 0) && (millisec <= 0) )
      millisec = 1;
    RtiNtpTimePackFromMillisec(pProperties->minimumSeparation, seconds, millisec);

    /* deadline pasts then subscription is nolonger present */
    if (pNDDS_Obj->BE_DeadlineMillisec == 0)  /* zero use default 20 seconds */
    {
       RtiNtpTimePackFromNanosec(pProperties->deadline, 20, 0);
    }
    else
    {
      seconds = pNDDS_Obj->BE_DeadlineMillisec  / 1000;
      millisec = pNDDS_Obj->BE_DeadlineMillisec % 1000;
      DPRINT2(+1,"BE: Deadline: secs: %d, millisec: %d\n",seconds,millisec);
      RtiNtpTimePackFromMillisec(pProperties->deadline, seconds, millisec);
    }

    NddsSubscriptionIssueListenerDefaultGet(&(pNDDS_Obj->issueListener));
    pNDDS_Obj->issueListener.recvCallBackRtn = pNDDS_Obj->callBkRtn;
    pNDDS_Obj->issueListener.recvCallBackRtnParam = pNDDS_Obj->callBkRtnParam;

    DPRINT1(+3,"createBESubscription: Register NDDS Data Type: '%s'.\n",pNDDS_Obj->dataTypeName);
    (*pNDDS_Obj->TypeRegisterFunc)();
    DPRINT1(+3,"createBESubscription: Allocate NDDS Data Type: '%s'.\n",pNDDS_Obj->dataTypeName);
    pNDDS_Obj->instance = (*pNDDS_Obj->TypeAllocFunc)();

    if ( strlen(pNDDS_Obj->MulticastSubIP) > 5)
    {
      interfaceIP = NddsStringToAddress(pNDDS_Obj->MulticastSubIP);
      DPRINT1(+1,"createBESubscription: MultiCast IP: '%s'\n",pNDDS_Obj->MulticastSubIP);
    }
    else
    {
      DPRINT(+3,"createBESubscription: UNICAST  IP\n");
      interfaceIP = NDDS_USE_UNICAST;
     }

    subscription = NddsSubscriptionCreateAtomic(pNDDS_Obj->domain,
					  pNDDS_Obj->topicName,  /* "Reliable HelloMsg", */
                                          pNDDS_Obj->dataTypeName, pNDDS_Obj->instance,
                                          pProperties, &(pNDDS_Obj->issueListener),
                                          interfaceIP);

    pNDDS_Obj->subscription = subscription;

    /* add subsciption to subscriber if there is one */
    if ( pNDDS_Obj->subscriber != NULL )
    {
      DPRINT(+3,"add to subscriber: NddsSubscriberSubscriptionAdd\n");
      NddsSubscriberSubscriptionAdd(pNDDS_Obj->subscriber,subscription);
    }
    else
    {
       DPRINT(3,"createSubscription: No subscriber\n");
    }
    return(0);
}

/*
 *  this sets the subscription property 'enabled' to false
 * resulting in no issues are received, however this does not
 * remove the subscription, i.e. issues will still be sent
 *
 *     Author: Greg Brissey   5/19/04
 */
RTIBool disableSubscription(NDDS_ID pNDDS_Obj)
{
    NDDSSubscriptionProperties  properties;
    NddsSubscriptionPropertiesGet(pNDDS_Obj->subscription,&properties);
    properties.enabled = RTI_FALSE;
    NddsSubscriptionPropertiesSet(pNDDS_Obj->subscription,&properties);
    return RTI_TRUE;
}
 
/*
 *  this sets the subscription property 'enabled' to true
 * resulting in issues being be received, This is the default state
 * Thus this call is only necceassary if disableSubscription(0 was called to
 * suspend issue receiving for a while.
 *
 *     Author: Greg Brissey   5/19/04
 */
RTIBool enableSubscription(NDDS_ID pNDDS_Obj)
{
    NDDSSubscriptionProperties  properties;
    NddsSubscriptionPropertiesGet(pNDDS_Obj->subscription,&properties);
    properties.enabled = RTI_TRUE;
    NddsSubscriptionPropertiesSet(pNDDS_Obj->subscription,&properties);
    return RTI_TRUE;
}

/*
 *  destroys the subscription  but leaves the NDDS_Obj structure intacted.
 * in theory the instance is free by this destory at some point in time
 * We should still allocate a new instance.
 *
 *     Author: Greg Brissey   5/19/04
 */
RTIBool nddsSubscriptionRemove(NDDS_ID pNDDS_Obj)
{
    if (!pNDDS_Obj->subscription) {
        return RTI_FALSE;
    }

   DPRINT1(+1,"nddsSubscriptionRemove: '%s'\n",pNDDS_Obj->topicName);
   if (NddsSubscriptionDestroy(pNDDS_Obj->domain, pNDDS_Obj->subscription) == RTI_TRUE)
   {
	pNDDS_Obj->subscription = NULL;
        return RTI_TRUE;
   }

   return RTI_FALSE;
}

RTIBool nddsSubscriptionDestroy(NDDS_ID pNDDS_Obj)
{
    void  *instance = NULL;

    if (!pNDDS_Obj->subscription) {
        return RTI_FALSE;
    }

    DPRINT1(+1,"nddsSubscriptionDestroy: '%s'\n",pNDDS_Obj->topicName);
    instance = NddsSubscriptionInstanceGet(pNDDS_Obj->subscription);

    if (NddsSubscriptionDestroy(pNDDS_Obj->domain, pNDDS_Obj->subscription) == RTI_TRUE) {
        /* Should not destroy the instance immediately.
         free(instance);
	*/
	free(pNDDS_Obj);
        return RTI_TRUE;
    }

    return RTI_FALSE;
}

void prtNDDS_Sub_SCCSid()
{
    printf("%s\n",SCCSid);
}

/* NDDSRecvInfo : 
 * RTINtpTime localTimeWhenReceived:     The local time when the issue was received 
 * const char* nddsTopic:                The topic of the subscription receiving the issue 
 * const char* nddsType:                 The type of the subscription receiving the issue 
 * int publicationId:                    The publication's unique Id 
 * NDDSSequenceNumber publSeqNumber:     Sending (publication) Sequence Number (e.g. publSeqNumber.high, publSeqNumber.low)
 * NDDSSequenceNumber recvSeqNumber:     Receiving Sequence Number 
 * RTINtpTime remoteTimeWhenPublished:   The remote time when the issue was published 
 * unsigned int senderAppId:             The sender's application Id 
 * unsigned int senderHostId:            The sender's host Id 
 * unsigned int senderNodeIP:            The sender's IP address 
 * NDDSRecvStatus status:                The status affects which fields are valid 
 * RTIBool validRemoteTimeWhenPublished: Whether or not a valid remote time was received 
 */
/* NDDSSequenceNumber: long high, unsigned long low */
/* NDDSRecvStatus status Values:
 *  NDDS_NEVER_RECEIVED_DATA:  Never received an issue, but a deadline occurred
 *  NDDS_NO_NEW_DATA:          Received at least one issue. A deadline occurred since the last issue received.
 *  NDDS_UPDATE_OF_OLD_DATA:   Received a new issue, whose time stamp is the same or older than the time stamp 
 *                               of the last fresh issue received. Most likely caused by the presence of multiple 
 *                               publications with the same publication topic running on computers with clocks 
 *                               that are not well synchronized.
 * NDDS_FRESH_DATA:            A new issue received
 * NDDS_DESERIALIZATION_ERROR: The deserialization method for the NDDSType returned an error. Most likely caused 
 *                               by an inconsistent interpretation of the NDDSType between the publishing and 
 *                               subscribing applications. This can be caused by an inconsistent specification 
 *                               of the NDDSType or of the maximum size of arrays or strings within the NDDSType. 
 */

char *getSubStatusDesc(int status)
{
    int i;
    for (i=0; i < 5; i++)
    {
       if (status == statusDesc[i].status)
           return(statusDesc[i].desc);
    }
    return("Undefined");
}

printNDDSInfo(NDDSRecvInfo *info)
{
    RTINtpTime timedif;
    char senttimestr[RTI_NTP_TIME_STRING_LEN];
    char recvtimestr[RTI_NTP_TIME_STRING_LEN];
    char diftimestr[RTI_NTP_TIME_STRING_LEN];

    RtiNtpTimeToString (&(info->localTimeWhenReceived), recvtimestr);
    RtiNtpTimeToString (&(info->remoteTimeWhenPublished), senttimestr);

    RtiNtpTimeSubtract(timedif, info->localTimeWhenReceived, info->remoteTimeWhenPublished) ;
    RtiNtpTimeToString (&timedif, diftimestr);

    DPRINT3(-1,"PubId: %d (0x%lx), topic: '%s'\n", info->publicationId,info->publicationId,  info->nddsTopic); 
    DPRINT3(-1,"Time Sent: %s, Time Recv'd: %s,  Delta Time: %s\n",senttimestr,recvtimestr,diftimestr);
    DPRINT4(-1,"Pub Seq#: %d, %lu, Recv'd Seq#: %ld, %lu\n",info->publSeqNumber.high,info->publSeqNumber.low,
		info->recvSeqNumber.high, info->recvSeqNumber.low);
    DPRINT3(-1,"senderAppId: 0x%lx, senderHostId: 0x%lx, senderNodeIP: 0x%lx\n", info->senderAppId, info->senderHostId, info->senderNodeIP);
    DPRINT1(-1,"status: '%s'\n",getSubStatusDesc(info->status));
}

/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/

#else /* RTI_NDDS_4x */

/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/

/*----------------------------- Data Reader Call Backs ---------------------------------*/

typedef struct _rejected_reasons {  int status; char* desc; } REJECTED_DESC;

static REJECTED_DESC  rejectedDesc[7] = { 
    {  DDS_NOT_REJECTED, "Samples are never rejected." },
    { DDS_REJECTED_BY_INSTANCE_LIMIT, "Resource limit on the number of instances was reached." },
    { DDS_REJECTED_BY_SAMPLES_LIMIT, "Resource limit on the number of samples was reached." },
    { DDS_REJECTED_BY_SAMPLES_PER_INSTANCE_LIMIT, "Resource limit on the number of samples per instance was reached." },
    { DDS_REJECTED_BY_REMOTE_WRITERS_LIMIT, "Resource limit on the number of remote writers from which a DDS_DataReader may read was reached." },
    { DDS_REJECTED_BY_REMOTE_WRITERS_PER_INSTANCE_LIMIT, "Resource limit on the number of remote writers for a single instance from which a DDS_DataReader may read was reached." },
    { DDS_REJECTED_BY_SAMPLES_PER_REMOTE_WRITER_LIMIT, "Resource limit on the number of samples from a given remote writer that a DDS_DataReader may store was reached." },
};

char *getRejectedDesc(int status)
{
    int i;
    for (i=0; i < 7; i++)
    {
       if (status == rejectedDesc[i].status)
           return(rejectedDesc[i].desc);
    }
    return("Undefined");
}

void Default_RequestedDeadlineMissed(void* listener_data, DDS_DataReader* reader,
                                         const struct DDS_RequestedDeadlineMissedStatus *status)
{
   DDS_TopicDescription *topicDesc;
   topicDesc = DDS_DataReader_get_topicdescription(reader);
/*
   DDS_Long 	total_count
 	Total cumulative count of the deadlines detected for any 
        instance read by the DDS_DataReader.
   DDS_Long 	total_count_change
 	The incremental number of deadlines detected since the 
        last time the listener was called or the status was read.
   DDS_InstanceHandle_t 	last_instance_handle
 	Handle to the last instance in the DDS_DataReader for 
        which a deadline was detected. 
*/
   DPRINT2(-1,"Default_RequestedDeadlineMissed for: Type: '%s', Name: '%s'\n",
           DDS_TopicDescription_get_type_name(topicDesc), DDS_TopicDescription_get_name(topicDesc));
}
void Default_RequestedIncompatibleQos(void* listener_data, DDS_DataReader* reader,
                      const struct DDS_RequestedIncompatibleQosStatus *status) 
{
   DDS_TopicDescription *topicDesc;
   topicDesc = DDS_DataReader_get_topicdescription(reader);
/*
   DDS_Long 	total_count
 	Total cumulative count of how many times the concerned 
        DDS_DataReader discovered a DDS_DataWriter for the same DDS_Topic 
        with an offered QoS that is incompatible with that requested by 
        the DDS_DataReader.
   DDS_Long 	total_count_change
 	The change in total_count since the last time the listener 
        was called or the status was read.
   DDS_QosPolicyId_t 	last_policy_id
 	The PolicyId_t of one of the policies that was found to be incompatible 
        the last time an incompatibility was detected.
   DDS_QosPolicyCountSeq 	policies
 	A list containing, for each policy, the total number of times that 
        the concerned DDS_DataReader discovered a DDS_DataWriter for the 
        same DDS_Topic with an offered QoS that is incompatible with that 
        requested by the DDS_DataReader. 
*/
   DPRINT2(-5,"Default_RequestedIncompatibleQos for: Type: '%s', Name: '%s'\n",
           DDS_TopicDescription_get_type_name(topicDesc), DDS_TopicDescription_get_name(topicDesc));
   DPRINT2(-5,"  Incompatible Qos: %d, '%s'\n",status->last_policy_id, getIncompatQosDesc(status->last_policy_id));
}

void Default_RequestedSampleRejected(void* listener_data, DDS_DataReader* reader,
                        const struct DDS_SampleRejectedStatus *status)
{
    DDS_TopicDescription *topicDesc;

/*
    DDS_Long 	total_count
 	Total cumulative count of samples rejected by the DDS_DataReader.
    DDS_Long 	total_count_change
 	The incremental number of samples rejected since the last time the 
        listener was called or the status was read.
    DDS_SampleRejectedStatusKind 	last_reason
 	Reason for rejecting the last sample rejected.
    DDS_InstanceHandle_t 	last_instance_handle
 	Handle to the instance being updated by the last sample that was rejected.

*/
   topicDesc = DDS_DataReader_get_topicdescription(reader);
   DPRINT2(-5,"Default_RequestedSampleRejected for: Type: '%s', Name: '%s'\n",
           DDS_TopicDescription_get_type_name(topicDesc), DDS_TopicDescription_get_name(topicDesc));
   DPRINT1(-5,"Issue/Sample Rejected for: '%s'\n",DDS_TopicDescription_get_name(topicDesc));
   DPRINT2(-5,"Total: %d, Delta: %d\n", status->total_count, status->total_count_change);
   DPRINT2(-5,"Reason: %d, '%s'\n", status->last_reason, getRejectedDesc(status->last_reason));

}
void Default_RequestedLivelinessChanged(void* listener_data, DDS_DataReader* reader,
                      const struct DDS_LivelinessChangedStatus *status) 
{
    DDS_TopicDescription *topicDesc;
/*
    DDS_Long 	alive_count
 	The total count of currently alive DDS_DataWriter entities that 
        write the DDS_Topic the DDS_DataReader reads.
    DDS_Long 	not_alive_count
 	The total count of currently not_alive DDS_DataWriter entities 
        that write the DDS_Topic the DDS_DataReader reads.
    DDS_Long 	alive_count_change
 	The change in the alive_count since the last time the listener 
        was called or the status was read.
    DDS_Long 	not_alive_count_change
 	The change in the not_alive_count since the last time the listener 
        was called or the status was read.
    DDS_InstanceHandle_t 	last_publication_handle
 	An instance handle to the last remote writer to change its liveliness. 
*/
   topicDesc = DDS_DataReader_get_topicdescription(reader);
   DPRINT2(+1,"Default_RequestedLivelinessChanged for: Type: '%s', Name: '%s'\n",
           DDS_TopicDescription_get_type_name(topicDesc), DDS_TopicDescription_get_name(topicDesc));
   DPRINT1(+1,"The total count of currently alive DDS_DataWriter entities: %d\n",status->alive_count);
   DPRINT1(+1,"The total count of currently not_alive DDS_DataWriter entities: %d\n",status->not_alive_count);
   DPRINT1(+1,"The change in the alive_count: %d\n",status->alive_count_change);
   DPRINT1(+1,"The change in the not_alive_count: %d\n", status->not_alive_count_change);
   DPRINT1(+1,"handle to the last remote writer to change its liveliness: 0x%lx\n",status->last_publication_handle);
}
void Default_RequestedSampleLost(void* listener_data, DDS_DataReader* reader,
                        const struct DDS_SampleLostStatus *status)
{
    DDS_TopicDescription *topicDesc;
/*
    DDS_Long 	total_count
 	Total cumulative count of all samples lost across all instances 
        of data published under the DDS_Topic.
    DDS_Long 	total_count_change
 	The incremental number of samples lost since the last time the 
        listener was called or the status was read. 
*/
   topicDesc = DDS_DataReader_get_topicdescription(reader);
   DPRINT2(+1,"Default_RequestedSampleLost for: Type: '%s', Name: '%s'\n",
           DDS_TopicDescription_get_type_name(topicDesc), DDS_TopicDescription_get_name(topicDesc));
   DPRINT(+1,"Issue/Sample Lost!\n");
   DPRINT2(+1,"Total: %d, Delta: %d\n", status->total_count, status->total_count_change);
}
void Default_SubscriptionMatched(void* listener_data, DDS_DataReader* reader,
                        const struct DDS_SubscriptionMatchedStatus *status)
{
    DDS_TopicDescription *topicDesc;
/*
    DDS_Long 	total_count
 	The total cumulative number of times the concerned DDS_DataReader 
        discovered a "match" with a DDS_DataWriter.
    DDS_Long 	total_count_change
 	The change in total_count since the last time the listener was 
        called or the status was read.
    DDS_Long 	current_count
 	The current number of writers with which the DDS_DataReader is matched.
    DDS_Long 	current_count_change
 	The change in current_count since the last time the listener was 
        called or the status was read.
    DDS_InstanceHandle_t 	last_publication_handle
 	A handle to the last DDS_DataWriter that caused the status to change. 
*/
   topicDesc = DDS_DataReader_get_topicdescription(reader);
   DPRINT4(+1,"Subscription Type: '%s', Name: '%s' Matched, Matched: %d, Delta: %d\n",
           DDS_TopicDescription_get_type_name(topicDesc), DDS_TopicDescription_get_name(topicDesc),
           status->current_count,status->current_count_change);
}
void Default_OnDataAvailable( void* listener_data, DDS_DataReader* reader)
{
    DPRINT(-5,"Default_OnDataAvailable\n");
}

/*******************************/
/*    attach callback Routines */
/*******************************/

void attachOnDataAvailableCallback(NDDS_ID pNDDS_Obj, 
           DDS_DataReaderListener_DataAvailableCallback callback, void *pUserData)
{
   DPRINT3(+2,"attachOnDataAvailable: obj: 0x%lx, callback: 0x%lx, Userdata: 0x%lx\n", pNDDS_Obj,callback,pUserData);
   pNDDS_Obj->pDReaderListener->on_data_available = callback;
   pNDDS_Obj->pDReaderListener->as_listener.listener_data = pUserData;
}

void attachSampleLostCallback(NDDS_ID pNDDS_Obj, DDS_DataReaderListener_SampleLostCallback callback)
{
   pNDDS_Obj->pDReaderListener->on_sample_lost = callback;
}

void attachDeadlineMissedCallback(NDDS_ID pNDDS_Obj, 
           DDS_DataReaderListener_RequestedDeadlineMissedCallback callback)
{
   pNDDS_Obj->pDReaderListener->on_requested_deadline_missed = callback;
}

void attachLivelinessChangedCallback(NDDS_ID pNDDS_Obj, 
           DDS_DataReaderListener_LivelinessChangedCallback callback)
{
   pNDDS_Obj->pDReaderListener->on_liveliness_changed = callback;
}

void attachSampleRejectedChangedCallback(NDDS_ID pNDDS_Obj, 
           DDS_DataReaderListener_SampleRejectedCallback callback)
{
   pNDDS_Obj->pDReaderListener->on_sample_rejected = callback;
}

void attachSubscriptionMatchedCallback(NDDS_ID pNDDS_Obj, 
           DDS_DataReaderListener_SubscriptionMatchedCallback callback)
{
   pNDDS_Obj->pDReaderListener->on_subscription_matched = callback;
}

void attachRequestedIncompatibleQosCallback(NDDS_ID pNDDS_Obj, 
           DDS_DataReaderListener_RequestedIncompatibleQosCallback callback)
{
   pNDDS_Obj->pDReaderListener->on_requested_incompatible_qos = callback;
}

void attachUserData(NDDS_ID pNDDS_Obj, void * pUserData)
{
   pNDDS_Obj->pDReaderListener->as_listener.listener_data = pUserData;
}


/* ------------------------------------------------------------------------------------*/

initDataReaderListener(NDDS_ID pNDDS_Obj)
{
    struct DDS_DataReaderListener *pReader_listener = 
              (struct DDS_DataReaderListener*) malloc(sizeof(struct DDS_DataReaderListener));
    // DDS_DataReaderListener_initialize(pReader_listener);

    pNDDS_Obj->pDReaderListener = pReader_listener;

    pReader_listener->as_listener.listener_data = (void*) NULL;
    /* setup the default callbacks, these can be overridden prior to the enablePublisher call */
    pReader_listener->on_requested_incompatible_qos = Default_RequestedIncompatibleQos;
    pReader_listener->on_liveliness_changed = Default_RequestedLivelinessChanged;

    pReader_listener->on_sample_rejected = Default_RequestedSampleRejected;
    pReader_listener->on_sample_lost = Default_RequestedSampleLost;
    pReader_listener->on_subscription_matched = Default_SubscriptionMatched;
    pReader_listener->on_requested_deadline_missed = Default_RequestedDeadlineMissed;
    pReader_listener->on_data_available = Default_OnDataAvailable; 


    // pReader_listener->as_listener.listener_data = UserDataPtr;

}

void initSubMulticastAddr(NDDS_ID pNDDS_Obj)
{
    struct DDS_TransportMulticastSettings_t* multicast_locator = NULL;
    DDS_TransportMulticastSettingsSeq_ensure_length(&(pNDDS_Obj->pDReaderQos->multicast.value), 1, 1);
    multicast_locator = DDS_TransportMulticastSettingsSeq_get_reference(&(pNDDS_Obj->pDReaderQos->multicast.value),0);

    /* replace multicast with requested address */
    DDS_String_replace(&multicast_locator->receive_address, pNDDS_Obj->MulticastSubIP); // mcast_user_data_address);
    DPRINT1(+1,"initMulticastAddr: MultiCast IP: '%s'\n",pNDDS_Obj->MulticastSubIP);

}

int initSubscription(NDDS_ID pNDDS_Obj)
{
    DDS_ReturnCode_t retcode;
    double requestedNAck;

    struct DDS_DataReaderQos *pReader_qos = (struct DDS_DataReaderQos*) malloc(sizeof(struct DDS_DataReaderQos));
    DDS_DataReaderQos_initialize(pReader_qos);

    pNDDS_Obj->pDReaderQos = pReader_qos;

    /* Need only one Publisher per App so create the Publisher only if required */
    if (pNDDS_Obj->pSubscriber == NULL)
        pNDDS_Obj->pSubscriber = SubscriberCreate(pNDDS_Obj->pParticipant);

    DPRINT(+3,"RegisterAndCreateTopic\n");
    pNDDS_Obj->pTopic = RegisterAndCreateTopic(pNDDS_Obj);

    DPRINT(+3,"DDS_Subscriber_get_default_datareader_qos\n");
    /* get default data writer properties */
    retcode = DDS_Subscriber_get_default_datareader_qos(pNDDS_Obj->pSubscriber, pReader_qos);
    if (retcode != DDS_RETCODE_OK) {
      errLogRet(LOGIT,debugInfo,"initSubscription: failed to get default datareader qos\n"); 
      NDDS_Shutdown(pNDDS_Obj);
      return -1;
    }

    /* set datawriters QOS for FULL reliability */
    /* DDS_BEST_EFFORT_RELIABILITY_QOS */
    pReader_qos->reliability.kind = DDS_RELIABLE_RELIABILITY_QOS;
    pReader_qos->history.kind = DDS_KEEP_ALL_HISTORY_QOS;
    // pWriter_qos->history.depth ignored when kind == DDS_KEEP_ALL_HISTORY_QOS, spec by resource settings

    /* The above Qos will insure that a publisher will pend indefinitely to send */
    /* block the Send call for up to sendMaxWait seconds when the queue is full */
    /* RtiNtpTimePackFromNanosec(pProperties->sendMaxWait,86400,0); */

    /* Set for Exclusive NOT Shared ownership;  for keyed topic */
    pReader_qos->ownership.kind = DDS_EXCLUSIVE_OWNERSHIP_QOS;

    /* RtiNtpTimePackFromNanosec(pProperties->persistence, 0 , 16); /* 16 milliseconds */
    /* ================================================================================= */
    /* The time before a subscriber will switch to a new publisher after last pub from
       a previous publisher */
    /* ================================================================================= */
    //pReader_qos->liveliness.lease_duration.sec = 10;
    // pReader_qos->liveliness.lease_duration.sec = 2;   // for readMRIUserByte testing, hammer it
    // pReader_qos->liveliness.lease_duration.nanosec = 0; 
    /* ================================================================================= */
    /* For readMRIUserByte, we need to turn off the liveness HB that are recieved and    */
    /* handle by the evt task that has been interfering with the UserByte data receive   */
    /* task.  To achieve this, set lease duration to infinite (which is its default)     */
    /* in addition the kind is set from automatic to manual by participant, thus as long as */
    /* any pub has published on this participant then all publisher on this participant are */
    /* considered alive.                GMB  3/5/2008                                    */ 
    /* ================================================================================= */

    pReader_qos->liveliness.kind = DDS_MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
    pReader_qos->liveliness.lease_duration = DDS_DURATION_INFINITE;   // for readMRIUserByte testing

    /* ================================================================================= */
    /* ================================================================================= */

    /* I believe for 4x this should be left to the default of Infinity */
    // RtiNtpTimePackFromNanosec(pProperties->timeToKeepPeriod , 0, 0);	/* default  */
    // pWriter_qos->lifespan.duration.sec = 0;
    // pWriter_qos->lifespan.duration.nanosec = 0;   /* 16 milliseconds */


    /* The assumption for a reader is that most time the reader will only be 
       receiving one instance of a publication so we generate only one instance 
      at first however for some pubs each controller will publish it's one instance thus we need
      a macimum of the total number of controller allowed in a system */
    pReader_qos->resource_limits.initial_instances = 1;
    pReader_qos->resource_limits.max_instances = 50;  

    DPRINT1(+3,"----> QueueSize: %d \n", pNDDS_Obj->queueSize);

    /* ================================================================================= */
    /* For the FID Data Upload subscriber this makes all the difference, at default values
    *  the acks might be delayed resulting in extreme variable data transfer rates!
    *  With the follow change the transfer of data smoothed out, and was on a par with NDDS 3x
    */
    /* ================================================================================= */
    pReader_qos->protocol.rtps_reliable_reader.min_heartbeat_response_delay.sec = 0;
    pReader_qos->protocol.rtps_reliable_reader.min_heartbeat_response_delay.nanosec = 0;
    pReader_qos->protocol.rtps_reliable_reader.max_heartbeat_response_delay.sec = 0;
    pReader_qos->protocol.rtps_reliable_reader.max_heartbeat_response_delay.nanosec = 0;
    /* ================================================================================= */
    /* ================================================================================= */

  // DDS_DataReaderQos_is_consistentI:inconsistent QoS policies: 
  // reader_resource_limits.max_samples_per_remote_writer and 
  // resource_limits.max_samples


    /* if Q Size  is zero then use all default values */
    if (pNDDS_Obj->queueSize <= 0)
    {
         /* pProperties->sendQueueSize = PUB_DEFAULT_QUEUE_SIZE; */
         /* The initial number of sample resources allocated */
         pReader_qos->resource_limits.initial_samples = SUB_DEFAULT_QUEUE_SIZE;

         /* the total samples for an instance that the DataWriter can queue/track */
         pReader_qos->resource_limits.max_samples_per_instance = SUB_DEFAULT_QUEUE_SIZE;
         pReader_qos->reader_resource_limits.max_samples_per_remote_writer = SUB_DEFAULT_QUEUE_SIZE;

         /* the total samples for all instances a DataWriter can queue/track */
         pReader_qos->resource_limits.max_samples = /* max samples per instance * max instances */
              pReader_qos->resource_limits.max_samples_per_instance * 
                  pReader_qos->resource_limits.max_instances;
    }
    else
    {
         /* pProperties->sendQueueSize = pNDDS_Obj->queueSize; */
         pReader_qos->resource_limits.initial_samples = pNDDS_Obj->queueSize;
         pReader_qos->resource_limits.max_samples_per_instance = pNDDS_Obj->queueSize;
         pReader_qos->reader_resource_limits.max_samples_per_remote_writer = pNDDS_Obj->queueSize;
         pReader_qos->resource_limits.max_samples = /* max samples per instance * max instances */
              pReader_qos->resource_limits.max_samples_per_instance * 
                  pReader_qos->resource_limits.max_instances;
    }

    /*  not useful in 4x */
    /*if (pNDDS_Obj->pubThreadId > DEFAULT_PUB_THREADID)
     * {
     *   pProperties->threadId = pNDDS_Obj->pubThreadId;
     * }
     */

    if ( strlen(pNDDS_Obj->MulticastSubIP) > 5)
    {
       initSubMulticastAddr(pNDDS_Obj);
    }

    DPRINT1(+3,"----> QueueSize: %d\n", pReader_qos->resource_limits.max_samples_per_instance);

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
    initDataReaderListener(pNDDS_Obj);
    DPRINT(+3,"initSubscription complete\n");

   return(0);
}

int initBESubscription(NDDS_ID pNDDS_Obj)
{
    DDS_ReturnCode_t retcode;
    double requestedNAck;
    long seconds,nanosec;

    struct DDS_DataReaderQos *pReader_qos = (struct DDS_DataReaderQos*) malloc(sizeof(struct DDS_DataReaderQos));
    DDS_DataReaderQos_initialize(pReader_qos);

    pNDDS_Obj->pDReaderQos = pReader_qos;

    /* Need only one Publisher per App so create the Publisher only if required */
    if (pNDDS_Obj->pSubscriber == NULL)
        pNDDS_Obj->pSubscriber = SubscriberCreate(pNDDS_Obj->pParticipant);

    pNDDS_Obj->pTopic = RegisterAndCreateTopic(pNDDS_Obj);

    /* get default data writer properties */
    retcode = DDS_Subscriber_get_default_datareader_qos(pNDDS_Obj->pSubscriber, pReader_qos);
    if (retcode != DDS_RETCODE_OK) {
      errLogRet(LOGIT,debugInfo,"initBESubscription: failed to get default datareader qos\n"); 
      NDDS_Shutdown(pNDDS_Obj);
      return -1;
    }

    /* set datareader QOS for Best Effort (default) */
    pReader_qos->reliability.kind = DDS_BEST_EFFORT_RELIABILITY_QOS;
    pReader_qos->history.kind = DDS_KEEP_LAST_HISTORY_QOS;
    // DDS_KEEP_LAST_HISTORY_QOS and depth to 1 (defaults)
    pReader_qos->history.depth = 1;

    /* The above Qos will insure that a publisher will pend indefinitely to send */
    /* block the Send call for up to sendMaxWait seconds when the queue is full */
    /* RtiNtpTimePackFromNanosec(pProperties->sendMaxWait,86400,0); */

    /* Set for Exclusive NOT Shared ownership;  for keyed topic */
    pReader_qos->ownership.kind = DDS_EXCLUSIVE_OWNERSHIP_QOS;

    /* after 10 sec liveliness loss detected */
    // pReader_qos->liveliness.lease_duration.sec = 2;    // for readMRIUserByte testing
    //pReader_qos->liveliness.lease_duration.sec = 10;
    // pReader_qos->liveliness.lease_duration.nanosec = 0;
    /* ================================================================================= */
    /* For readMRIUserByte, we need to turn off the liveness HB that are recieved and    */
    /* handle by the evt task that has been interfering with the UserByte data receive   */
    /* task.  To achieve this, set lease duration to infinite (which is its default)     */
    /* in addition the kind is set from automatic to manual by participant, thus as long as */
    /* any pub has published on this participant then all publisher on this participant are */
    /* considered alive.                GMB  3/5/2008                                    */ 
    /* ================================================================================= */
    pReader_qos->liveliness.kind = DDS_MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
    pReader_qos->liveliness.lease_duration = DDS_DURATION_INFINITE;   // for readMRIUserByte testing
    /* ================================================================================= */
    /* ================================================================================= */

    /* I believe for 4x this should be left to the default of Infinity */
    // RtiNtpTimePackFromNanosec(pProperties->timeToKeepPeriod , 0, 0);	/* default  */
    // pReader_qos->lifespan.duration.sec = 5;
    // pReader_qos->lifespan.duration.nanosec = 0;   /* 16 milliseconds */

    /* subscriber update rate,  20 milliseconds */
    seconds = pNDDS_Obj->BE_UpdateMinDeltaMillisec  / 1000;
    nanosec = (pNDDS_Obj->BE_UpdateMinDeltaMillisec % 1000) * 1000000;
    DPRINT2(+1,"BE: minimumSeparation: secs: %d, millisec: %d\n",seconds,nanosec/1000000);
    if ( (seconds == 0) && (nanosec <= 0) )
      nanosec = 10000;
    pReader_qos->time_based_filter.minimum_separation.sec = seconds;
    pReader_qos->time_based_filter.minimum_separation.nanosec = nanosec;

    /* deadline does map to what is was used for in 3x */
    /* deadline pasts then subscription is nolonger present */
    //if (pNDDS_Obj->BE_DeadlineMillisec == 0)  /* zero use default 20 seconds */
    // {
       // pReader_qos->deadline.period.sec = 20;
       // pReader_qos->deadline.period.nanosec = 0;
    // }
    // else
    // {
      // seconds = pNDDS_Obj->BE_DeadlineMillisec  / 1000;
      // nanosec = (pNDDS_Obj->BE_DeadlineMillisec % 1000) * 1000;
      // DPRINT2(+1,"BE: Deadline: secs: %d, millisec: %d\n",seconds,millisec);
      // pReader_qos->deadline.period.sec = 20;
      // pReader_qos->deadline.period.nanosec = 0;
    // }

    if ( strlen(pNDDS_Obj->MulticastSubIP) > 5)
    {
       initSubMulticastAddr(pNDDS_Obj);
    }

    pNDDS_Obj->instance = (*pNDDS_Obj->TypeAllocFunc)(DDS_BOOLEAN_TRUE);

    /* setup the default callbacks, these can be overridden prior to the enablePublisher call */
    initDataReaderListener(pNDDS_Obj);

    return 0;
}

// enablePublication(NDDS_ID pNDDS_Obj)
int createSubscription(NDDS_ID pNDDS_Obj)
{

    DPRINT(+3,"Creating Subscription ---- \n");
    pNDDS_Obj->pDReader = DDS_Subscriber_create_datareader(pNDDS_Obj->pSubscriber,
                DDS_Topic_as_topicdescription(pNDDS_Obj->pTopic),
                pNDDS_Obj->pDReaderQos, /* &DDS_DATAREADER_QOS_DEFAULT */
                pNDDS_Obj->pDReaderListener /* or NULL */,
                DDS_STATUS_MASK_ALL);

                // DDS_DATA_AVAILABLE_STATUS |
                // DDS_REQUESTED_DEADLINE_MISSED_STATUS |
                // DDS_SAMPLE_LOST_STATUS |
                // DDS_SAMPLE_REJECTED_STATUS |
                // DDS_SUBSCRIPTION_MATCHED_STATUS );


   if (pNDDS_Obj->pDReader == NULL) {
        errLogRet(LOGIT,debugInfo,"create_datareader error\n"); 
        NDDS_Shutdown(pNDDS_Obj);
        return (-1);
    }
    DPRINT(+3,"created ---- \n");
  
     return 0;
}

int nddsSubscriptionRemove(NDDS_ID pNDDS_Obj)
{
}
int nddsSubscriptionDestroy(NDDS_ID pNDDS_Obj)
{
}
#endif  /* RTI_NDDS_4x */

#ifdef __cplusplus
}
#endif

