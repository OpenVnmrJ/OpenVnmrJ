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
#ifndef LINT
#endif


#ifdef VNMRS_WIN32
#include <Windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef VXWORKS
#include "errLogLib.h"
#else
#include "logMsgLib.h"
#include "taskPriority.h"
#endif
#include "NDDS_Obj.h"
//#include "ndds/ndds_c.h"

#ifndef RTI_NDDS_4x
#include "ndds/wavesurf.h"
#endif

#ifdef __cplusplus
extern "C" { */
#endif

/* #define DOMAIN_LISTENER_ON */

#ifndef RTI_NDDS_4x
#ifdef DOMAIN_LISTENER_ON
   /* Domain Listener Routines */
static RTIBool subRemoteNewHook(const struct NDDSApplicationInfo *appInfo,
                                 const struct NDDSSubscriptionInfo *subInfo,
                                 void *userHookParam);
static RTIBool pubRemoteNewHook(const struct NDDSApplicationInfo *appInfo,
                                   const struct NDDSPublicationInfo *pubInfo,
                                   void *userHookParam);
static void subRemoteDeleteHook(const struct NDDSApplicationInfo *appInfo,
                                      const struct NDDSSubscriptionInfo *subInfo,
                                      void *userHookParam);
static  void pubRemoteDeleteHook( const struct NDDSApplicationInfo *appInfo,
                                      const struct NDDSPublicationInfo *pubInfo, 
                                      void *userHookParam);
static int DomainNumber = 0;

#endif  /* DOMAIN_LISTENER_ON */
#endif /* RTI_NDDS_4x */

/* **************************************************************************** */
/*     nddsCreate - instantiates the NDDS domain
 *		  you need at least one for NDDS to function.
 *		  creates and returns the NDDS_Obj structure.
 *	
 *    This structure is partially fill with domain information
 *   The user fills in specific fields to create publications
 *  or subscription.
 *
 *    Prior to this the user must create a NDDS data Type via a .x file
 *   and generated the required functions via nddsgen on the .x file.
 *
 */
/* **************************************************************************** */

/* --------------------------------------------------------------------------- */
/*    nddsCreate:
 *       domain - the domain ordinal number 0 - N 
 *		pubication & subscriptions  must be on the same domain to commincate
 *	        typical only one domain is used. domain Zero.
 *             Each domain is handled by a single task, thus some that pends this
 *            tasks stops all communictions on that domain.
 *             Multiple Domains can be used to avoid this for criticial publications
 *             or opubications that are know to pend the domain task alot.
 *
 *      debuglevel - 0 no printing, 1-? bigger the number the more output
 *
 *      multicast - enable the domain for multicasting
 *
 */
/* --------------------------------------------------------------------------- */
NDDS_ID nddsCreate(int domain, int debuglevel, int multicast, char *nicIP)
{
   NDDS_OBJ *pNddsObj;
#ifndef RTI_NDDS_4x
   NDDSDomain initDomain(int nddsDomain, int nddsVerbosity, int multicast, char *nicIP, NDDS_OBJ *pNddsObj);
#else /* RTI_NDDS_4x */
   DDS_Publisher *PublisherCreate(DDS_DomainParticipant *participant);
   DDS_Subscriber *SubscriberCreate(DDS_DomainParticipant *participant);
   DDS_DomainParticipant *initDomain(int domainId, int nddsVerbosity, int multicast, char *nicIP, NDDS_OBJ *pNddsObj);
#endif  /* RTI_NDDS_4x */

    /* ------- malloc space for FIFO Object --------- */
      if ( (pNddsObj = (NDDS_OBJ *) malloc( sizeof(NDDS_OBJ)) ) == NULL )
      {
        /* errLogSysRet(LOGIT,debugInfo,"fifoCreate: Could not Allocate Space:"); */
        return(NULL);
      }
 
    /* zero out structure so we don't free something by mistake */
    memset(pNddsObj,0,sizeof(NDDS_OBJ));

#ifndef RTI_NDDS_4x
    pNddsObj->domain = initDomain(domain, debuglevel, multicast, nicIP, pNddsObj);
    /* if we fail to obtain a domain pointer then return NULL */
    if (pNddsObj->domain == NULL)

#else
    initDomain(domain, debuglevel, multicast, nicIP, pNddsObj);
    /* if we fail to obtain a domain pointer then return NULL */
    if (pNddsObj->pParticipant == NULL)

#endif
    {
       free(pNddsObj);
       pNddsObj = NULL;
    }

#ifdef RTI_NDDS_4x
      /* for 4x create the publisher and subscriber up front since most will use just one */
      pNddsObj->pPublisher = PublisherCreate(pNddsObj->pParticipant);
      pNddsObj->pSubscriber = SubscriberCreate(pNddsObj->pParticipant);
#endif

    return(pNddsObj);
}

/*
   called internally by nddsCreate()
*/

#ifndef RTI_NDDS_4x
NDDSDomain initDomain(int nddsDomain, int nddsVerbosity, int multicast, char *nicIP, NDDS_OBJ *pNddsObj)
{
    NDDSDomain   domain;
    /* NDDSDomainProperties domainProperties; */
    NDDSDomainProperties *pDomainProperties;
    NDDSDomainListener *pDomainListener;
    NDDSDomainListener domainListener;
    int i,nddsInitTries;
    char IPString[256];
    unsigned int loopBackIP  = NddsStringToAddress("127.0.0.1");
    
    /* get local IP, e.g. NddsStringToAddress("172.16.0.2"); */
    unsigned int interfaceIP = NddsStringToAddress(nicIP);


#ifndef LINUX
#ifdef DEBUG
    WaveSurfProperties waveSurfProperties;
 
   /* Initialize wavesurf */
    WaveSurfPropertiesDefaultGet(&waveSurfProperties);
    waveSurfProperties.appName = "NirvanaConsole";
    WaveSurfInit(&waveSurfProperties);
#endif
#endif
 
   pDomainListener = NULL;

#ifdef DOMAIN_LISTENER_ON
   DomainNumber = nddsDomain;
   if ( NddsDomainListenerDefaultGet(&domainListener) == RTI_TRUE) 
   {
	/* attach Sub new hook, pub new hook, Sub remote delete, Pub remote delete */
         domainListener.onSubscriptionRemoteNew = subRemoteNewHook;
         domainListener.onSubscriptionRemoteNewParam  = &DomainNumber;
         domainListener.onPublicationRemoteNew = pubRemoteNewHook;
         domainListener.onPublicationRemoteNewParam = &DomainNumber;
	 domainListener.onSubscriptionRemoteDelete = subRemoteDeleteHook;
	 domainListener.onSubscriptionRemoteDeleteParam = &DomainNumber;
         domainListener.onPublicationRemoteDelete  = pubRemoteDeleteHook;
         domainListener.onPublicationRemoteDeleteParam  = &DomainNumber;
   }
   pDomainListener = &domainListener;
#endif

    /* fill in the NDDSDomainProperties structure with the defaults of the Domain */
    /* NddsDomainPropertiesDefaultGet(&domainProperties);  */
    NddsDomainPropertiesDefaultGet(&(pNddsObj->domainProperties)); 
    pDomainProperties = &(pNddsObj->domainProperties);

    DPRINT(+1,"\n\n");
    DPRINT3(+1,"Defaults: MaxSerialSize: %d, recvBuf: %d, sendBuf: %d\n",
	pDomainProperties->maxSizeSerialize,pDomainProperties->dgram.recvBufferSize,pDomainProperties->dgram.sendBufferSize);

    /* Set max serialization to NDDS's Maximum of 63K or 64512 bytes */
    pDomainProperties->maxSizeSerialize = 64512; /* max of 63K */

#ifndef VXWORKS

    /* on Solaris IPC V shared memory and Semaphores keep increasing
     * as the procs are started and stopped. If we turn off share memory 
     * usage for NDDS this is no longer a problem.  
     * A temp fix, but effective for the short term
     * Since we do not use NDDS between processes on the SUN this should not impact performance at all.
     * Greg Brissey     1/11/05
    */
    pDomainProperties->sharedMemoryCommunicationEnabled = RTI_FALSE;

#else    /* VxWorks */

    /* on the same node use shared memory to communicate rather than ethernet loopBack */
    pDomainProperties->sharedMemoryCommunicationEnabled = RTI_TRUE;

#endif

    /* these may need to be tuned, for best operation */
#ifndef VXWORKS
    pDomainProperties->dgram.recvBufferSize  = 262144; /* 30 * 1048576; 30 MB,  128*1024;  65535;  2*32274;  65535; */
    pDomainProperties->dgram.sendBufferSize  = 262144; /* was 66 * 1024; 65535;  32274;  65535; */
#else
    pDomainProperties->dgram.recvBufferSize  = 131072; /*  maximum allow by VxWorks */
    pDomainProperties->dgram.sendBufferSize  = 131072; /*  maximum allow by VxWorks */
#endif

    /* this kills the NDDS manager if the task spawning it is terminated */
    pDomainProperties->domainBase.spawnedManager.terminate = RTI_TRUE; 

    DPRINT3(+1,"New Values: MaxSerialSize: %d, recvBuf: %d, sendBuf: %d\n",
	pDomainProperties->maxSizeSerialize,pDomainProperties->dgram.recvBufferSize,pDomainProperties->dgram.sendBufferSize);

    // Make the heartbeating more aggressive
    RtiNtpTimePackFromMillisec(pDomainProperties->domainBase.fastPeriod, 0, 100);
    RtiNtpTimePackFromMillisec(pDomainProperties->domainBase.rtt.rtoInitial, 0, 100);

  /* speed up update of manager's data base, every 10 sec rather than the default of 72 sec */
     RtiNtpTimePackFromMillisec(pDomainProperties->domainBase.spawnedManager.purgePeriod,10,0);


#ifdef VXWORKS
    pDomainProperties->tasks.atPriority = NDDS_ALARM_TASK_PRIORITY;  /* 51 */
    pDomainProperties->tasks.rtPriority = NDDS_RECEIVER_TASK_PRIORITY;  /* 52 */
    pDomainProperties->tasks.stPriority = NDDS_SENDER_TASK_PRIORITY; /* 53 */
    pDomainProperties->tasks.dtPriority = NDDS_DBM_TASK_PRIORITY;     /* 54 */
    pDomainProperties->domainBase.spawnedManager.priority = NDDS_MANAGER_TASK_PRIORITY;
#endif

    /* Remove all NICs other than interfaceIP, but DON'T remove loopback.
       This is a problem with a Sun host with multiple ethernet cards present
       We only want to publish/subscribe subscriptions on the console subnet 
       If NDDS see 3 NIC it will attempt to send a subscription to all three subnets
       as you can image this can really slow things down.
    */
    for (i=0;  i < pDomainProperties->nicIPAddressCount;  i++) {
        NddsAddressToString(pDomainProperties->nicProperties[i].ipAddress, IPString);
        /* DPRINT2(-1,"nic[%d] = '%s'\n",i,IPString); */
        if ((pDomainProperties->nicProperties[i].ipAddress != loopBackIP) &&
            (pDomainProperties->nicProperties[i].ipAddress != interfaceIP) )
        {
            /* Just clear ifFlags, don't modify nicIPAddressCount! */
            pDomainProperties->nicProperties[i].ifFlags = 0;
            DPRINT2(1,"nic[%d] = '%s', removed..\n",i,IPString);
        }
        else
        {
            DPRINT2(1,"nic[%d] = '%s', kept..\n",i,IPString);
        }
    }
 
    if (multicast)
    {
        /*  enable multicasting publishing */
        pDomainProperties->multicast.enabled = RTI_TRUE;
        pDomainProperties->multicast.ttl = NDDSTTLSameSubnet;
        pDomainProperties->multicast.loopBackEnabled = RTI_TRUE;  /* default */
        pDomainProperties->domainBase.spawnedManager.multicast.enabled = RTI_TRUE;
        pDomainProperties->domainBase.spawnedManager.wire.metaMulticastIP =  NddsStringToAddress("225.0.0.1");
        pDomainProperties->domainBase.spawnedManager.hostList.managers =  "225.0.0.1";


        /* suggested by Howard at RTI to avoid multicast storms */
       pDomainProperties->domainBase.spawnedManager.wire.multicastThreshold = 0;
       pDomainProperties->wire.multicastThreshold = 0;
    }


#ifdef VXWORKS
    /* specify a unique App Identifier, to handle quick reboots */
    pDomainProperties->wire.appId = sysTimestampLock();
    pDomainProperties->domainBase.spawnedManager.wire.appId = sysTimestampLock();
    DPRINT2(0," >>>>>>  domain AppId : %d, Manager AppId: %d <<<<<<<<< \n",pDomainProperties->wire.appId,
                pDomainProperties->domainBase.spawnedManager.wire.appId);
#endif

    NddsVerbositySet(nddsVerbosity);  /* calling this prior to NddsInit is OK */
#ifdef NO_SUPPORT_IN_31B
#ifdef VXWORKS
    if (nddsVerbosity < 1)
       NddsTaskIdVerbositySet(-1);       /* stop warnings about multiple task publishing same issue */
#endif
#endif

    DPRINT1(+1,"initialize domain: NddsInit maxSizeSerialize: %d\n",pDomainProperties->maxSizeSerialize);

#ifndef VXWORKS
    for(nddsInitTries = 4; nddsInitTries > 0; nddsInitTries--)
    {
       domain = NddsInit(nddsDomain, pDomainProperties, pDomainListener);
       if (domain != NULL)
	  break;
       DPRINT1(+1,">>>>>>>  NddsInit() return NULL, Retrying, retrys left: %d\n",nddsInitTries);
#ifdef VNMRS_WIN32
	   Sleep(1000);
#else
       sleep(1);    /* wait one second prior to retrying */
#endif
    }
#else
    domain = NddsInit(nddsDomain, pDomainProperties, pDomainListener);
#endif
    /* DPRINT(-1,"\n\n"); */
    return(domain);
}

/*
    Creates A publisher for the domain. 
    A publisher is a separate context/thread than the calling routine. 
*/
int createPublisher(NDDS_ID pNDDS_Obj)
{
    NDDSPublisher publisher = NddsPublisherCreate(pNDDS_Obj->domain,
                                              NDDS_PUBLISHER_SIGNALLED);
   pNDDS_Obj->publisher = publisher;
   return(0); 
}
/*
    Create a subscriber for the Domain
*/
int createSubscriber(NDDS_ID pNDDS_Obj)
{
   NDDSSubscriber subscriber = NddsSubscriberCreate(pNDDS_Obj->domain);
   pNDDS_Obj->subscriber = subscriber;
   return(0); 
}

void prtNDDScw_SCCSid()
{
    printf("%s\n",SCCSid);
}




/* ============================================================================ */
/*    DOMAIN LISTENER ROUTINES  */
/* ============================================================================ */

#ifdef DOMAIN_LISTENER_ON

/* Domain Listener Routines */

/*
*  Subscription Remote New Hook routine 
*/
static RTIBool subRemoteNewHook(const struct NDDSApplicationInfo *appInfo,
                                 const struct NDDSSubscriptionInfo *subInfo,
                                 void *userHookParam)
{
    int domain;
    domain = *((int*) userHookParam);
#ifdef VXWORKS
    if (DebugLevel > -1)
    {
#endif
    diagPrint(NULL,"Domain %d: subRemoteNewHook: AppInfo: AppId: 0x%lx, hostId : 0x%lx\n",domain,appInfo->appId, appInfo->hostId);
    diagPrint(NULL,"Domain %d: subRemoteNewHook: Sub - topic: '%s', type: '%s', ObjectId: 0x%lx\n",domain,
		subInfo->nddsTopic,subInfo->nddsType,subInfo->objectId);
#ifdef VXWORKS
    }
#endif
    return(RTI_TRUE);
}

/*
*  Publication Remote New Hook routine 
*/
static RTIBool pubRemoteNewHook(const struct NDDSApplicationInfo *appInfo,
                                   const struct NDDSPublicationInfo *pubInfo,
                                   void *userHookParam)
{
    int domain;
    domain = *((int*) userHookParam);
#ifdef VXWORKS
    if (DebugLevel > -1)
    {
#endif
      diagPrint(NULL,"Domain %d: pubRemoteNewHook: AppInfo: AppId: 0x%lx, hostId : 0x%lx\n",domain,appInfo->appId, appInfo->hostId);
      diagPrint(NULL,"Domain %d: pubRemoteNewHook: Pub - topic: '%s', type: '%s', ObjectId: 0x%lx\n",domain,
		pubInfo->nddsTopic,pubInfo->nddsType, pubInfo->objectId);
#ifdef VXWORKS
    }
#endif
    return(RTI_TRUE);
}

/*
*  Subscription Remote Delete Hook routine 
*/
static void subRemoteDeleteHook(const struct NDDSApplicationInfo *appInfo,
                                      const struct NDDSSubscriptionInfo *subInfo,
                                      void *userHookParam)
{
    int domain;
    domain = *((int*) userHookParam);
#ifdef VXWORKS
    if (DebugLevel > -1)
    {
#endif
      diagPrint(NULL,"Domain %d: subRemoteDeleteHook: AppInfo: AppId: 0x%lx, hostId : 0x%lx\n",domain,appInfo->appId, appInfo->hostId);
      diagPrint(NULL,"Domain %d: subRemoteDeleteHook: Sub - topic: '%s', type: '%s', ObjectId: 0x%lx\n",domain,
		subInfo->nddsTopic,subInfo->nddsType,subInfo->objectId);
#ifdef VXWORKS
    }
#endif
}
  
/*
*  Publication Remote Delete Hook routine 
*/
static  void pubRemoteDeleteHook(
         const struct NDDSApplicationInfo *appInfo,
         const struct NDDSPublicationInfo *pubInfo, void *userHookParam)
{
    int domain;
    domain = *((int*) userHookParam);
#ifdef VXWORKS
    if (DebugLevel > -1)
    {
#endif
      diagPrint(NULL,"Domain %d: pubRemoteDeleteHook: AppInfo: AppId: 0x%lx, hostId : 0x%lx\n",domain,appInfo->appId, appInfo->hostId);
      diagPrint(NULL,"Domain %d: pubRemoteDeleteHook: Pub - topic: '%s', type: '%s', ObjectId: 0x%lx\n",domain,
		pubInfo->nddsTopic,pubInfo->nddsType,pubInfo->objectId);
#ifdef VXWORKS
    }
#endif
}

#endif    /* DOMAIN_LISTENER_ON */

/*******************************************************************************************/
/*******************************************************************************************/
/*******************************************************************************************/

#else  /* RTI_NDDS_4x NDDS 4x */

/*******************************************************************************************/
/*******************************************************************************************/
/*******************************************************************************************/

/* NDDS 4.2E  Port ---------------------------------------------------------------------- */

/* What Qos was incompatible */
typedef struct _status_desc_ {  int status; char* desc; } STATUS_DESC;

static STATUS_DESC  incompatQosDesc[42] = { 
    { DDS_INVALID_QOS_POLICY_ID, "Invalid Qos Policy." },
    { DDS_USERDATA_QOS_POLICY_ID, "UserData Qos Policy." },
    { DDS_DURABILITY_QOS_POLICY_ID, "Durability Qos Policy." },
    { DDS_PRESENTATION_QOS_POLICY_ID, "Presentation Qos Policy." }, 	
    { DDS_DEADLINE_QOS_POLICY_ID, "Deadline Qos Policy." }, 	
    { DDS_LATENCYBUDGET_QOS_POLICY_ID, "LatencyBudget Qos Policy." },
    { DDS_OWNERSHIP_QOS_POLICY_ID, "Ownership Qos Policy." }, 	
    { DDS_OWNERSHIPSTRENGTH_QOS_POLICY_ID, "OwnershipStrenght Qos Policy." }, 	
    { DDS_LIVELINESS_QOS_POLICY_ID, "Liveliness Qos Policy." }, 	
    { DDS_TIMEBASEDFILTER_QOS_POLICY_ID, "TimebaseFilter Qos Policy." }, 	
    { DDS_PARTITION_QOS_POLICY_ID, "Partition Qos Policy." }, 	
    { DDS_RELIABILITY_QOS_POLICY_ID, "Reliability Qos Policy." }, 	
    { DDS_DESTINATIONORDER_QOS_POLICY_ID, "DestinationOrder Qos Policy." }, 	
    { DDS_HISTORY_QOS_POLICY_ID, "Hisotry Qos Policy." }, 	
    { DDS_RESOURCELIMITS_QOS_POLICY_ID, "ResourceLimits Qos Policy." }, 	
    { DDS_ENTITYFACTORY_QOS_POLICY_ID, "EntityFactory Qos Policy." }, 
    { DDS_WRITERDATALIFECYCLE_QOS_POLICY_ID, "WriterDataLifeCycle Qos Policy." }, 
    { DDS_READERDATALIFECYCLE_QOS_POLICY_ID, "ReaderDataLifeCycle Qos Policy." }, 	
    { DDS_TOPICDATA_QOS_POLICY_ID, "TopciData Qos Policy." }, 	
    { DDS_GROUPDATA_QOS_POLICY_ID, "GroupData Qos Policy." }, 	
    { DDS_TRANSPORTPRIORITY_QOS_POLICY_ID, "TransportPriority Qos Policy." }, 	
    { DDS_LIFESPAN_QOS_POLICY_ID, "LifeSpan Qos Policy." }, 	
    { DDS_DURABILITYSERVICE_QOS_POLICY_ID, "DurabilityService Qos Policy." }, 	
    { DDS_WIREPROTOCOL_QOS_POLICY_ID, "WireProtocol Qos Policy." }, 
    { DDS_DISCOVERY_QOS_POLICY_ID, "Discovery Qos Policy." }, 
    { DDS_DATAREADERRESOURCELIMITS_QOS_POLICY_ID, "DataReaderResourceLimits Qos Policy." }, 
    { DDS_DATAWRITERRESOURCELIMITS_QOS_POLICY_ID, "DataWriterResourceLimits Qos Policy." },
    { DDS_DATAREADERPROTOCOL_QOS_POLICY_ID, "DataReaderProtocol Qos Policy." }, 
    { DDS_DATAWRITERPROTOCOL_QOS_POLICY_ID, "DataWriterProtocol Qos Policy." },
    { DDS_DOMAINPARTICIPANTRESOURCELIMITS_QOS_POLICY_ID, "DOmainParticipantResourceLimits Qos Policy." },
    { DDS_EVENT_QOS_POLICY_ID, "Event Qos Policy." }, 
    { DDS_DATABASE_QOS_POLICY_ID, "Database Qos Policy." },
    { DDS_RECEIVERPOOL_QOS_POLICY_ID, "ReceiverPool Qos Policy." },
    { DDS_DISCOVERYCONFIG_QOS_POLICY_ID, "DiscoveryConfig Qos Policy." },
    { DDS_EXCLUSIVEAREA_QOS_POLICY_ID, "ExclusiveArea Qos Policy." },
    { DDS_TRANSPORTSELECTION_QOS_POLICY_ID, "TransportSelection Qos Policy." },
    { DDS_TRANSPORTUNICAST_QOS_POLICY_ID, "TransportUnicast Qos Policy." },
    { DDS_TRANSPORTMULTICAST_QOS_POLICY_ID, "TransportMulticast Qos Policy." },
    { DDS_TRANSPORTBUILTIN_QOS_POLICY_ID, "TransportBuiltin Qos Policy." },
    { DDS_TYPESUPPORT_QOS_POLICY_ID, "TypeSupport Qos Policy." },
    { DDS_PUBLISHMODE_QOS_POLICY_ID, "PublishMode Qos Policy." },
    { DDS_ASYNCHRONOUSPUBLISHER_QOS_POLICY_ID, "AsynchronousPublisher Qos Policy." },
};

char *getIncompatQosDesc(int status)
{
    int i;
    for (i=0; i < 42; i++)
    {
       if (status == incompatQosDesc[i].status)
           return(incompatQosDesc[i].desc);
    }
    return("Undefined");
}


/* Delete all entities */
static int participant_shutdown( DDS_DomainParticipant *participant)
{
    DDS_ReturnCode_t retcode;
    int status = 0;

    if (participant != NULL) {
        retcode = DDS_DomainParticipant_delete_contained_entities(participant);
        if (retcode != DDS_RETCODE_OK) {
            errLogRet(LOGIT,debugInfo,"participant_shutdown: delete_contained_entities error %d\n",retcode); 
            status = -1;
        }

        retcode = DDS_DomainParticipantFactory_delete_participant(
            DDS_TheParticipantFactory, participant);
        if (retcode != DDS_RETCODE_OK) {
            errLogRet(LOGIT,debugInfo,"participant_shutdown: delete_participant error %d\n",retcode); 
            status = -1;
        }
    }

    /* RTI Data Distribution Service provides finalize_instance() method on
       domain participant factory and finalize() method on type support for
       people who want to release memory used by the participant factory and
       type support singletons. Uncomment the following block of code for
       clean destruction of the singletons. */
/*
    HelloWorldTypeSupport_finalize();

    retcode = DDS_DomainParticipantFactory_finalize_instance();
    if (retcode != DDS_RETCODE_OK) {
        errLogRet(LOGIT,debugInfo,"finalize_instance error %d\n",retcode); 
        status = -1;
    }
*/

    return status;
}

void NDDS_Shutdown( NDDS_OBJ *pNddsObj )
{
   participant_shutdown(pNddsObj->pParticipant);
}

/* turn on verbose NDDS logggin */
NDDSLogOn()
{
    NDDS_Config_Logger_set_verbosity( NDDS_Config_Logger_get_instance(), NDDS_CONFIG_LOG_VERBOSITY_STATUS_ALL);
}

DDS_DomainParticipant *initDomain(int domainId, int nddsVerbosity, int multicast, char *nicIP, NDDS_OBJ *pNddsObj)
{

   DDS_DomainParticipantFactory* factory = NULL;
   // DDS_DomainParticipant *participant = NULL;

   struct DDS_DomainParticipantFactoryQos factory_qos = DDS_DomainParticipantFactoryQos_INITIALIZER;
   struct NDDS_Transport_UDPv4_Property_t udpv4_property = NDDS_TRANSPORT_UDPV4_PROPERTY_DEFAULT;
   DDS_ReturnCode_t retcode;
   char *locator = NULL;

/* if not vxworks then create & initial the high resolution clock */
#ifndef RTI_VXWORKS
    initHRTimer();
#endif
   // struct DDS_DomainParticipantQos participant_qos = DDS_DomainParticipantQos_INITIALIZER;
   struct DDS_DomainParticipantQos *pParticipant_qos = malloc(sizeof(struct DDS_DomainParticipantQos));
   DDS_DomainParticipantQos_initialize(pParticipant_qos);


  // DDS_DomainParticipantFactory_get_default_participant_qos(myFactory, myQos);
  // DDS_DomainParticipant_set_qos(myParticipant, myQos);
  // DDS_DomainParticipantQos_finalize(myQos);

#define MAX_PEER_LOCATOR_STR_LEN 64
  locator = DDS_String_alloc(8 + MAX_PEER_LOCATOR_STR_LEN);

  factory = DDS_DomainParticipantFactory_get_instance();
  if (factory == NULL) {
        errLogRet(LOGIT,debugInfo,"initDomain: failed to get domain participant factory\n"); 
        return 0;
  }

  /* the participant must be created Unenable to allow the transport qos changes */

   DDS_DomainParticipantFactory_get_qos(factory, &factory_qos);
   factory_qos.entity_factory.autoenable_created_entities = DDS_BOOLEAN_FALSE;
   DDS_DomainParticipantFactory_set_qos(factory, &factory_qos);
   DDS_DomainParticipantFactoryQos_finalize (&factory_qos);

    /* Set the initial peers. These list all the computers the application
       may communicate with along with the maximum number of RTI Data
       Distribution Service participants that can concurrently run on that
       computer. This list only needs to be a superset of the actual list of
       computers and participants that will be running at any time.
    */


    const char* NDDS_DISCOVERY_INITIAL_PEERS[] = {
        "225.0.0.1",      /* multicast discovery IP Addr */
        "builtin.udpv4://127.0.0.1",
        "builtin.shmem://"
    };
    const long NDDS_DISCOVERY_INITIAL_PEERS_LENGTH =
                 sizeof(NDDS_DISCOVERY_INITIAL_PEERS)/sizeof(const char*);



    /* initialize participant_qos with default values */
    retcode = DDS_DomainParticipantFactory_get_default_participant_qos(factory,
                                                             pParticipant_qos);
    if (retcode != DDS_RETCODE_OK) {
        errLogRet(LOGIT,debugInfo,"initDomain: failed to get domain default participant qos\n"); 
        return 0;
    }

    /* ----------------- Modify participant_qos ------------------------ */

    if (!DDS_StringSeq_from_array(&(pParticipant_qos->discovery.initial_peers),
                             NDDS_DISCOVERY_INITIAL_PEERS,
                             NDDS_DISCOVERY_INITIAL_PEERS_LENGTH)) {
        errLogRet(LOGIT,debugInfo,"initDomain: failed to set discovery.initial_peers qos\n"); 
        return 0;
    }

    /* Regarding discovery, there's actually another parameter you'll need to */
    /* set, instructing the participant to listen on a particular multicast */
    /* address.  This address is automatically set to the first multicast */
    /* address found in the initial peers, but this occurs at application */
    /* initialization, so only file or environment variable */
    /* NDDS_DISCOVERY_PEERS changes will influence the setting.  Since we're */
    /* changing the initial peers programatically, we'll also need to set */
    /* this parameter: */
    /* Note that although the QoS is a sequence, currently (4.1e) only the */
    /*  first address is used. */

    sprintf(*DDS_StringSeq_get_reference(&(pParticipant_qos->discovery.multicast_receive_addresses), 0),
             "225.0.0.1");

    /* ========================================================================== */
    /* Change the participant liveliness settings, for mriUserByte, GMB  2/12/08  */
    /* ========================================================================== */
    // default is 100 sec lease duration, 30 sec assert period
    /* Leave this setting to the default, below code was for testing purposes.. GMB  3/5/2008
    // pParticipant_qos->discovery_config.participant_liveliness_lease_duration.sec = 2; //1 * 60; // 1 minutes
    // pParticipant_qos->discovery_config.participant_liveliness_lease_duration.nanosec = 0;
    // pParticipant_qos->discovery_config.participant_liveliness_assert_period.sec = 1;  // 1 * 60;
    // pParticipant_qos->discovery_config.participant_liveliness_assert_period.nanosec = 0;
    /* ========================================================================== */
    /* ========================================================================== */

    // DPRINT(-2,"-- set discover options \n");

  /* speed up update of manager's data base, every 10 sec rather than the default of 61 sec */
    pParticipant_qos->database.cleanup_period.sec = 10;
    pParticipant_qos->database.cleanup_period.nanosec = 0;

#ifndef RTI_VXWORKS
    pParticipant_qos->receiver_pool.buffer_size = 256*1024; //65535;
    pParticipant_qos->event.thread.priority = RTI_OSAPI_THREAD_PRIORITY_HIGH;
    pParticipant_qos->receiver_pool.thread.priority =  RTI_OSAPI_THREAD_PRIORITY_HIGH;
    pParticipant_qos->event.max_count = 512; // 16 * 1024;  /* what is this?? */
#else
    pParticipant_qos->receiver_pool.buffer_size = 128*1024; //65535;
    // event thread priority in VxWorks 110 by default
    // pParticipant_qos->event.thread.priority = NDDS_ALARM_TASK_PRIORITY; // 75; // 110;
    // pParticipant_qos->receiver_pool.thread.priority =  NDDS_RECEIVER_TASK_PRIORITY; // 71 default;

    /* ========================================================================== */
    /* for ReadMRIUserByte lower NDDS Evt task to below that of the receive tasks    2/12/08  GMB. */
    /* ========================================================================== */
    pParticipant_qos->event.thread.priority = NDDS_RECEIVER_TASK_PRIORITY;
    pParticipant_qos->receiver_pool.thread.priority =  NDDS_ALARM_TASK_PRIORITY; // 71 default;
    pParticipant_qos->database.thread.priority =   NDDS_DBM_TASK_PRIORITY; // 120 default;
    // pParticipant_qos->event.max_count = 512; // 16 * 1024;  /* what is this?? */
    // unique id for vxworks
    pParticipant_qos->wire_protocol.rtps_app_id = sysTimestampLock();
#endif

    /* ========================================================================== */
    /* for ReadMRIUserByte follow code was to test with the discovery heat beats off. 3/04/08  GMB. */
    /* ========================================================================== */
    // pParticipant_qos->discovery_config.subscription_writer.heartbeat_period.sec = 24 * 3600;
    // pParticipant_qos->discovery_config.subscription_writer.fast_heartbeat_period.sec = 24 * 3600;
    // pParticipant_qos->discovery_config.subscription_writer.late_joiner_heartbeat_period.sec = 24 * 3600;
    // pParticipant_qos->discovery_config.publication_writer.heartbeat_period.sec = 24 * 3600;
    // pParticipant_qos->discovery_config.publication_writer.fast_heartbeat_period.sec = 24 * 3600;
    // pParticipant_qos->discovery_config.publication_writer.late_joiner_heartbeat_period.sec = 24 * 3600;
    /* ========================================================================== */

    /* Create the participant */
    // DPRINT(-2,"Creating Participant ---- \n");
     pNddsObj->pParticipant =
        DDS_DomainParticipantFactory_create_participant(factory,
                                                        domainId,
                                                        pParticipant_qos,
                                                        /* &participant_listener */ NULL ,
                                                        DDS_STATUS_MASK_NONE);

     if ( pNddsObj->pParticipant == NULL) {
        errLogRet(LOGIT,debugInfo,"initDomain: failed to create domain participant\n"); 
        NDDS_Shutdown(pNddsObj);
        return NULL;
    }
    pNddsObj->pParticipantQos = pParticipant_qos;

    if (NDDS_Transport_Support_get_builtin_transport_property(pNddsObj->pParticipant,
                                          DDS_TRANSPORTBUILTIN_UDPv4,
                                          (struct NDDS_Transport_Property_t*)&udpv4_property)
           != DDS_RETCODE_OK)
    {
        errLogRet(LOGIT,debugInfo,"initDomain: failed to get builtin transport property\n"); 
    }
    /* -------------------------------------------------------------------------------*/
    /* Increase the UDPv4 maximum message size to 64K (large messages). */
    udpv4_property.parent.message_size_max =  65535;
#ifndef RTI_VXWORKS
    udpv4_property.recv_socket_buffer_size =  256*1024; // 65535;
    udpv4_property.send_socket_buffer_size =  256*1024; //65535;
#else
    udpv4_property.recv_socket_buffer_size =  128*1024; // 65535;
    udpv4_property.send_socket_buffer_size =  128*1024; //65535;

    /* ========================================================================== */
    /* Using the zero copy network buffers, resulted in a lock/mutex contention between
    /* The NDDS Event & Receive tasks in VxWorks.  This resulted in the Evt Task blocking
    /* the Receive tasks for significant amounts of time (2+ ms) resulting in one mode of
    /* readMRIUserByte  failure.. Thus we no turn this OFF.   GMB   2/5/2008   */
    /* ========================================================================== */
    udpv4_property.no_zero_copy = 1; // turn off network stack zero copy for readMRIUserByte

#endif


    /* ONLY one interface permitted for test */

    sprintf(locator, "%s", nicIP /* "172.16.0.1" */);
    DPRINT1(2,"Limit NIC to: '%s'\n",nicIP);
    // sprintf(locator, "%s", "172.16.0.*" );

    udpv4_property.parent.allow_interfaces_list_length = 1;
    udpv4_property.parent.allow_interfaces_list = &locator;


    // DPRINT(-2,"Set Transport properties ---- \n");
    if (NDDS_Transport_Support_set_builtin_transport_property(pNddsObj->pParticipant,
                                          DDS_TRANSPORTBUILTIN_UDPv4,
                                          (struct NDDS_Transport_Property_t*)&udpv4_property)
           != DDS_RETCODE_OK)
    {
        errLogRet(LOGIT,debugInfo,"initDomain: failed to set builtin transport property\n"); 
    }
    /* -------------------------------------------------------------------------------*/

    // C++ way:  participant.enable();
     // DPRINT(-2,"Enable Participant ---- \n");
    // taskDelay(60);
    DDS_Entity * entity = DDS_DomainParticipant_as_entity(pNddsObj->pParticipant);
    DDS_Entity_enable(entity);

    return pNddsObj->pParticipant;

}

DDS_Publisher *PublisherCreate(DDS_DomainParticipant *participant)
{
  DDS_Publisher *publisher;
  DDS_ReturnCode_t retcode;
  struct DDS_PublisherQos publisher_qos = DDS_PublisherQos_INITIALIZER;

  retcode = DDS_DomainParticipant_get_default_publisher_qos(participant,
                                                           &publisher_qos);
  if (retcode != DDS_RETCODE_OK) 
  {
     errLogRet(LOGIT,debugInfo,"PublisherCreate: failed to get default publisher qos\n"); 
     participant_shutdown(participant);
     return NULL;
   }

   publisher_qos.asynchronous_publisher.thread.priority = RTI_OSAPI_THREAD_PRIORITY_HIGH;


    // DPRINT(-2,"Creating Publisher ---- \n");
   publisher =  DDS_DomainParticipant_create_publisher(participant,
                                                      &publisher_qos, /* &DDS_PUBLISHER_QOS_DEFAULT */
                                                   /* &subscriber_listener or */ NULL,
                                                      DDS_STATUS_MASK_NONE);

   if (publisher == NULL) {
       errLogRet(LOGIT,debugInfo,"PublisherCreate: failed to  create publisher\n"); 
       participant_shutdown(participant);
       return NULL;
   }

   return publisher;
}

DDS_Subscriber *SubscriberCreate(DDS_DomainParticipant *participant)
{
  DDS_Subscriber *subscriber;

    /* To customize subscriber QoS, use
       DDS_DomainParticipant_get_default_subscriber_qos() */
     // DPRINT(-2,"Creating Subscriber ---- \n");
    subscriber = DDS_DomainParticipant_create_subscriber(
        participant, &DDS_SUBSCRIBER_QOS_DEFAULT, NULL /* listener */,
        DDS_STATUS_MASK_NONE);
    if (subscriber == NULL) {
        errLogRet(LOGIT,debugInfo,"PublisherCreate: failed to  create subscriber\n"); 
        participant_shutdown(participant);
        return NULL;
    }

    return subscriber;
}

DDS_Topic *RegisterAndCreateTopic(NDDS_ID pNDDS_Obj)
{
    DDS_Topic *topic = NULL;
    DDS_ReturnCode_t retcode;
    const char *type_name = NULL;
    struct DDS_TopicQos topic_qos = DDS_TopicQos_INITIALIZER;
    DDS_TopicDescription *TopicDesc;

    /* -------------- Unique for each Issue type -------------- */
    // vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

    /* Register type before creating topic */
    // type_name = NDDSThroughputTestPacketTypeSupport_get_type_name();
    type_name = pNDDS_Obj->dataTypeName;  /* (*pNDDS_Obj->TypeNameFunc)(); */
    DPRINT1(2,"Registering: type_name: '%s'\n",type_name);

    // (*pNDDS_Obj->TypeRegisterFunc)();
    // retcode = NDDSThroughputTestPacketTypeSupport_register_type(participant, type_name);
    retcode = (*pNDDS_Obj->TypeRegisterFunc)(pNDDS_Obj->pParticipant, type_name);
     if (retcode != DDS_RETCODE_OK) {
        errLogRet(LOGIT,debugInfo,"RegisterAndCreateTopic: failed to register type, err: %d\n",retcode); 
        NDDS_Shutdown(pNDDS_Obj);
        return NULL;
    }

    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    //* If topic is not unique then trying to create one will fail, thus 1st lookup the topic to
    //  if it is already present if so then obtain the topic and return it.
    // DDS_Topic *DDS_DomainParticipant_find_topic(DDS_DomainParticipant *self, const char *topic_name, 
    //                                              const struct DDS_Duration_t *timeout)
    // 	Finds an existing (or ready to exist) DDS_Topic, based on its name.
    // 	Lookup an existing locally-created DDS_TopicDescription, based on its name. 
    DPRINT2(2,"Check for Topic: Type: '%s', Name: '%s'\n",type_name, pNDDS_Obj->topicName);
    TopicDesc =  DDS_DomainParticipant_lookup_topicdescription(pNDDS_Obj->pParticipant, pNDDS_Obj->topicName);
    if (TopicDesc != NULL)
    {
         // Narrow the given DDS_TopicDescription pointer to a DDS_Topic pointer.
         topic =  DDS_Topic_narrow(TopicDesc);
          DPRINT2(2,"Topic already present, desc: 0x%lx, topic: 0x%lx\n",TopicDesc, topic);
         return topic;
    }
    
    /* -----------------------------------------------------------------------------*/

    /* setup QOS for Topic and create Topic */
    /* -----------------------------------------------------------------------------*/
    DPRINT(2,"topic not present, create it... \n");

    // retcode = DDS_DomainParticipant_get_default_topic_qos(participant,
    retcode = DDS_DomainParticipant_get_default_topic_qos(pNDDS_Obj->pParticipant,
                                                      &topic_qos);

    if (retcode != DDS_RETCODE_OK) {
        errLogRet(LOGIT,debugInfo,"RegisterAndCreateTopic: failed to get default topic qos\n"); 
        NDDS_Shutdown(pNDDS_Obj);
        return NULL;
    }
/*
    DDS_Topic *DDS_DomainParticipant_find_topic(DDS_DomainParticipant *self, const char *topic_name, 
                                                 const struct DDS_Duration_t *timeout)
 	Finds an existing (or ready to exist) DDS_Topic, based on its name.
    DDS_TopicDescription *DDS_DomainParticipant_lookup_topicdescription (DDS_DomainParticipant *self, const char *topic_name)
 	Lookup an existing locally-created DDS_TopicDescription, based on its name. 
    DDS_Topic * 	DDS_Topic_narrow (DDS_TopicDescription *self)
 	Narrow the given DDS_TopicDescription pointer to a DDS_Topic pointer. 
*/

    // DPRINT1(2,"Creating topic for: '%s'\n", pNDDS_Obj->topicName);
    topic = DDS_DomainParticipant_create_topic(pNDDS_Obj->pParticipant, pNDDS_Obj->topicName, /* "Example HelloWorld"*/
                                               type_name,
                                              &topic_qos,  /*  &DDS_TOPIC_QOS_DEFAULT */
                                           /* &topic_listener */ NULL ,
                                               DDS_STATUS_MASK_NONE);

    if (topic == NULL) {
        errLogRet(LOGIT,debugInfo,"RegisterAndCreateTopic: failed to create topic\n"); 
        NDDS_Shutdown(pNDDS_Obj);
        return NULL;
    }
    DPRINT1(2,"Creating topic for: '%s' Complete.\n", pNDDS_Obj->topicName);
    return topic;
}


/* --------------------  VxWorks Priority Receive Task Change routines -------- */
#ifdef VXWORKS

// the purpose of this routine to to make the meta receive tasks (rR0000, rR0100) priority 
// below that of the user data receive tasks (rR0200,rR0300), which is not possible from 
// the NDDS API directly.
//  Greg Brissey   2/11/08

/*  "rDtb00", "rEvt00", "rR0000", "rR0100", "rR0200", "rR0300", NULL }; */
char *nddsRecvTaskList[] = { "rR0200", "rR0300", NULL };
char *nddsMetaRecvTaskList[] = { "rR0000", "rR0100", NULL };

/*
   The problem is that for mriUserByte the meta traffic can delay the multicast
   user data receive task from completing it's job in a timely manner, thus we
   manually changing the priorities of these receiving task around just a bit
   MultiCast Recv highest, Unicast Recv next, then both Meta Recv next 
   Note: right after NDDS is Initialized and domain created. Only the Meta Receive
         tasks (R00,R01) are present). Not until pub/sub for unicast & multicast
         are created will the R02 & R03 task be present..     GMB 2/12/08

   This solution has not been completely tested, as of 2/12/08   GMB.

   Update 3/3/2008 - this will not work since all the receive tasks are in a pool
                     which means any of the receive task could handle the multicast
                     user data.  Thus changing the priority does not accomplish the above
                     aim reliably or predictable!
                     GMB
*/
#ifdef NOT_USED_SEE_ABOVE_COMMENTS_GMB

*NDDS_ChngMetaRecvTasksPriority()
*{
*   int numTasks,i,j,stat;
*   int taskIdList[256];
*   char *tName;
*  int recvTaskPrior,taskPri,newPri;
*
*   /* obtain all running tasks */
*   numTasks = taskIdListGet(taskIdList,256);
*
*   /* the assumption for this routine to work properly is that all the
*      ndds receive task start with equal priority
*      and rR03 is multicast user data rec., rR02 unicast user data receive
*   */
*   for (i=0; i < numTasks; i++)
*   {
*      /* name of task to check */
*      tName = (char*) taskName(taskIdList[i]);
*      DPRINT1(+4,"Task: '%s'  \n", tName);
*      /* Unicast receive then set below multicast rR03 task */
*/*
* *      if (strncmp(tName,"rR0200",strlen("rR0200")) == 0)
* *      {
* *         taskPriorityGet(taskIdList[i],&taskPri);
* *         newPri = taskPri + 1;
* *         DPRINT3(-5,"task: '%s' (0x%lx), Pri: %d\n", tName, taskIdList[i], recvTaskPrior);
* *         taskPrioritySet(taskIdList[i],newPri);
* *      }
* */
*      for (j=0; nddsMetaRecvTaskList[j] != NULL; j++)
*      {  
*         // DPRINT2(-4,"Task: '%s' , MatchTask: '%s' \n", tName,nddsMetaRecvTaskList[j]);
*         /* compare the names with in the 1st 6 char for a match */
*         if (strncmp(tName,nddsMetaRecvTaskList[j],strlen(nddsMetaRecvTaskList[j])) == 0)
*         {
*            taskPriorityGet(taskIdList[i],&taskPri);
*            newPri = taskPri + 5;
*            DPRINT4(+4,"Chng Pri of; '%s', ID; 0x%lx, From: %d, To: %d\n", 
*                        tName,taskIdList[i],taskPri,newPri);
*            taskPrioritySet(taskIdList[i],newPri);
*            break;
*         }
*      }   
*  } 
*}
*
/* must be called after unicast & multicast pub/sub have been created
   otherwise these task (rR02,rR03) will not be present..
   May only need to call this is MRIUserByte is being used.

  Still requires testing   2/12/08  GMB.


  See Update 3/3/2008 comment above...  GMB

*/
*NDDS_ChngRecvTasksPriority()
*{
*   int numTasks,i,j,stat;
*   int taskIdList[256];
*   char *tName;
*   int taskPri,newPri;
*
*   /* obtain all running tasks */
*   numTasks = taskIdListGet(taskIdList,256);
*
*   for (i=0; i < numTasks; i++)
*   {
*      tName = (char*) taskName(taskIdList[i]);
*      // DPRINT1(+4,"Task: '%s'  \n", tName);
*
*      /* Find the Unicast User Data Receive Task (rR02)  Priority */
*      /* NDDS 4.x tasks, these they are dynamicly named thus we look for 
*         a match in the first 6 charaters
*      */
*      if (strncmp(tName,"rR0200",strlen("rR0200")) == 0)
*      {
*         taskPriorityGet(taskIdList[i],&taskPri);
*         newPri = taskPri + 1;
*         DPRINT4(+4,"Chng Pri of; '%s', ID; 0x%lx, From: %d, To: %d\n", 
*                        tName,taskIdList[i],taskPri,newPri);
*         taskPrioritySet(taskIdList[i],newPri);
*      }
*   }
*}
#endif  /* NOT_USED_SEE_ABOVE_COMMENTS_GMB */
#endif  /* VXWORKS */

/* --------------------------  Timer routines ----------------------------- */

static struct RTIClock *_clock; /* A pointer to a high resolution clock */
static double _HRclock_overhead;

static DDS_Boolean calculate_clock_overhead()
{
    int i = 0;
    RTIBool ok = RTI_FALSE;

    struct RTINtpTime begin_time = RTI_NTP_TIME_ZERO,
                      clock_traversal_time = RTI_NTP_TIME_ZERO;

    _clock->reset(_clock);

    if (!_clock->getTime(_clock, &begin_time)) {
        printf("***Error: failed to get time\n");
        goto finally;
    }

#define TIME_MANAGER_CALCULATION_LOOP_COUNT_MAX 100
    for (i = 0; i < TIME_MANAGER_CALCULATION_LOOP_COUNT_MAX; ++i) {
        if (!_clock->getTime(_clock, &clock_traversal_time)) {
            printf("***Error: failed to get time\n");
            goto finally;
        }
    }

    RTINtpTime_decrement(clock_traversal_time, begin_time);

    _HRclock_overhead = RTINtpTime_toDouble(&clock_traversal_time)/
        (double)TIME_MANAGER_CALCULATION_LOOP_COUNT_MAX;
    ok = RTI_TRUE;

finally:

    if (ok == RTI_TRUE) {
        return DDS_BOOLEAN_TRUE;
    } else {
        return DDS_BOOLEAN_FALSE;
    }
}

DDS_Boolean initHRTimer()
{
    _clock = RTIHighResolutionClock_new();

    if (_clock != NULL) {
       calculate_clock_overhead();
        return DDS_BOOLEAN_TRUE;
    } else {
        return DDS_BOOLEAN_FALSE;
    }
}

DDS_Boolean get_time(struct RTINtpTime *_time )
{
    RTIBool return_value;

    if (_clock != NULL) 
    {
       return_value = _clock->getTime(_clock, _time);
       if (return_value == RTI_TRUE) {
           return DDS_BOOLEAN_TRUE;
       } else {
           return DDS_BOOLEAN_FALSE;
       }
    }
    else
      return DDS_BOOLEAN_FALSE;
}

double get_delta_time(struct RTINtpTime finishTime, struct RTINtpTime startTime)
{
    struct RTINtpTime delta_time;
    double return_value;

    RTINtpTime_subtract(delta_time, finishTime, startTime);
    return_value = RTINtpTime_toDouble(&delta_time);
    return_value -= _HRclock_overhead;
    return return_value;
}
/* --------------------------  Timer routines ----------------------------- */


#endif  /* RTI_NDDS_4x */

#ifdef __cplusplus
}
#endif

