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

#ifndef NDDS_Obj_h
#define NDDS_Obj_h

#ifndef RTI_NDDS_4x
#ifndef ndds_cdr_h
#include "ndds/ndds_cdr.h"
#endif
#endif /* RTI_NDDS_4x */

#ifndef ndds_c_h
#include "ndds/ndds_c.h"
#endif

#define MAX_UDP_SIZE_BYTES (64*1024)
#define MAX_SUBSCRIPTION_SIZE_BYTES (63*1024)

#define DEFAULT_PUB_THREADID -1
#define MIN_MULTICAST_IP "224.0.1.0"
#define MAX_MULTICAST_IP "239.255.255.255"

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif


#ifndef RTI_NDDS_4x

typedef RTIBool (*NddsCallBkRtn) (const NDDSRecvInfo *issue, NDDSInstance *instance, void *callBackRtnParam);
typedef RTIBool (*DataTypeRegister) (void);
typedef NDDSInstance* (*DataTypeAllocate) (void);
typedef int (*DataTypeSize)(int size);

typedef struct {
                 NDDSDomain 		domain;
                 NDDSDomainProperties    domainProperties;
                 NDDSPublisher		publisher;
                 NDDSSubscriber		subscriber;
                 NDDSPublication 	publication;
                 NDDSPublicationProperties  pubProperties;
                 int			queueSize;   /* SendQueue for Pub, receiveQueue for Sub */
                 int			highWaterMark;   /* Pub use */
                 int			lowWaterMark;   /* Pub use */
                 int			AckRequestsPerSendQueue;   /* Pub use */
                 int			pubThreadId;   /* used in multi threaded apps */
                 NDDSPublicationReliableStatusRtn pubRelStatRtn;
                 void*			pubRelStatParam;
                 NDDSSubscription	subscription;
                 NDDSSubscriptionProperties  subProperties;
                 NDDSIssueListener 	issueListener;
                 NddsCallBkRtn		callBkRtn;
                 void*			callBkRtnParam;
                 DataTypeRegister	TypeRegisterFunc;
                 DataTypeAllocate	TypeAllocFunc;
                 DataTypeSize		TypeSizeFunc;
                 NDDSInstance*		instance;  /* pub/sub data Type  instance */
                 int			dataTypeSize;
                 int     		multicastFlg;
                 int     		BE_UpdateMinDeltaMillisec;
                 int     		BE_DeadlineMillisec;
                 int			publisherStrength;
                 char			topicName[128];   /* The Unique name of publication */
                 char		        dataTypeName[128]; /* The NDDS data type name */
                 char			MulticastSubIP[20];   /* Sub MultiCast IP string, e.g. "225.0.0.1" */
} NDDS_OBJ;

#else /* RTI_NDDS_4x */

// typedef RTIBool (*NddsCallBkRtn) (const NDDSRecvInfo *issue, NDDSInstance *instance, void *callBackRtnParam);
typedef char* (*DataTypeNameGet) (void);
typedef DDS_ReturnCode_t (*DataTypeRegister) (DDS_DomainParticipant*, char*);
typedef void* (*DataTypeAllocate) (int);
typedef int (*DataTypeSize)(int size);

typedef struct {
                DDS_DomainParticipant    *pParticipant;
                struct DDS_DomainParticipantQos *pParticipantQos;
                DDS_Publisher            *pPublisher;
                DDS_DataWriter           *pDWriter;
                struct DDS_DataWriterQos *pDWriterQos;
                struct DDS_DataWriterListener *pDWriterListener;
                DDS_InstanceHandle_t     issueRegHandle;
                DDS_Subscriber           *pSubscriber;
                DDS_DataReader           *pDReader;
                struct DDS_DataReaderQos *pDReaderQos;
                struct DDS_DataReaderListener *pDReaderListener;
                DDS_Topic                *pTopic;
	             int			queueSize;   /* SendQueue for Pub, receiveQueue for Sub */
	             int			highWaterMark;   /* Pub use */
	             int			lowWaterMark;   /* Pub use */
	             int			AckRequestsPerSendQueue;   /* Pub use */
                int			pubThreadId;   /* used in multi threaded apps */
                void*			pubRelStatParam;
		          void*			callBkRtnParam;
		          DataTypeRegister	TypeRegisterFunc;
		          DataTypeAllocate	TypeAllocFunc;
                DataTypeSize		TypeSizeFunc;
		          void*		instance;  /* pub/sub data Type  instance */
		          int			dataTypeSize;
                int     		multicastFlg;
                int     		BE_UpdateMinDeltaMillisec;
                int     		BE_DeadlineMillisec;
                int			publisherStrength;
		          char			topicName[128];   /* The Unique name of publication */
		          char		        dataTypeName[128]; /* The NDDS data type name */
		          char			MulticastSubIP[20];   /* Sub MultiCast IP string, e.g. "225.0.0.1" */
} NDDS_OBJ;

#endif /* RTI_NDDS_4x */
 
typedef NDDS_OBJ *NDDS_ID;
 
/* --------- ANSI/C++ compliant function prototypes --------------- */
 
#if defined(__STDC__) || defined(__cplusplus)
 
extern NDDS_ID nddsCreate(int domain, int debuglevel, int multicast, char* nicIP );
// extern NDDSDomain initDomain(int nddsDomain, int nddsVerbosity, int multicast, char *nicIP, NDDS_OBJ *pNddsObj);
extern int createPublisher(NDDS_ID pNDDS_Obj);
extern int createSubscriber(NDDS_ID pNDDS_Obj);

#ifdef RTI_NDDS_4x
extern void NDDS_Shutdown( NDDS_OBJ *pNddsObj );
extern DDS_Publisher *PublisherCreate(DDS_DomainParticipant *participant);
extern DDS_Subscriber *SubscriberCreate(DDS_DomainParticipant *participant);
extern DDS_Topic *RegisterAndCreateTopic(NDDS_ID pNDDS_Obj);
extern DDS_Boolean initHRTimer();
extern DDS_Boolean get_time(struct RTINtpTime *_time );
extern double get_delta_time(struct RTINtpTime finishTime, struct RTINtpTime startTime);
#endif
/* extern NDDS_ID nddsCreate(int domain, int debuglevel, int multicast, char *nicIP, NDDSDomainListener *pDomainListener); */
/* extern NDDSDomain initDomain(int nddsDomain, int nddsVerbosity, int multicast, char *nicIP, NDDSDomainListener *pDomainListener); */

#else
/* --------- NON-ANSI/C++ prototypes ------------  */
 
// extern NDDS_ID nddsCreate();
// extern NDDSDomain initDomain();

#endif
 
#ifdef __cplusplus
}
#endif
 
#endif


