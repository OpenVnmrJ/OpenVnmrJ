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

#include <fcntl.h>


#include <errno.h>
#include <signal.h>

#include "errLogLib.h"

#include "ndds/ndds_c.h"
#include "NDDS_Obj.h"
#include "NDDS_SubFuncs.h"
#include "NDDS_PubFuncs.h"
#include "Console_Stat.h"
// #include "App_HB.h"
#include "hostAcqStructs.h"
#include "msgQLib.h"

#ifdef RTI_NDDS_4x
#include "Console_StatPlugin.h"
#include "Console_StatSupport.h"
#endif  /* RTI_NDDS_4x */

#define TRUE 1
#define FALSE 0
#define FOR_EVER 1

#define NDDS_DOMAIN_NUMBER 0
#define  MULTICAST_ENABLE  1     /* enable multicasting for NDDS */
#define  MULTICAST_DISABLE 0     /* disable multicasting for NDDS */
#define  NDDS_DBUG_LEVEL 3
static char  *ConsoleHostName = "wormhole";

NDDS_ID NDDS_Domain, pPubObj, pStatSub, pStatusObj;

#ifndef RTI_NDDS_4x
static NDDSSubscriber CntlrSubscriber = NULL;
#endif  /* RTI_NDDS_4x */

/* required HB for operation */
static int Master_HB = -1;   /* OK without the master the status is never to Inforproc, so for consistency */
static int DDR_HB = -1;
static int Expproc_HB = -1;  /* OK Expproc forks Infoproc, so for consistency */

static int totalHB_Subscriptions = 0;
static int currentHB_Subscriptions = 0;

extern char ProcName[256];

extern pthread_t main_threadId;    /* main thread Id, so we can signal just this thread */

extern long encodedAcqSample; /* encoded AcqSample as comes from console, used in  update_statinfo (infoqueu.c) */

/*     NDDS additions */
/*---------------------------------------------------------------------------------- */

#ifndef RTI_NDDS_4x

RTIBool Console_StatCallback(const NDDSRecvInfo *issue, NDDSInstance *instance,
                             void *callBackRtnParam)
{

   Console_Stat *recvIssue;
   void consolestatAction(Console_Stat *data);

   /*    possible status values:
     NDDS_FRESH_DATA, NDDS_DESERIALIZATION_ERROR, NDDS_UPDATE_OF_OLD_DATA,
     NDDS_NO_NEW_DATA, NDDS_NEVER_RECEIVED_DATA
   */
   if (issue->status == NDDS_FRESH_DATA)
   {
     recvIssue = (Console_Stat *) instance;
     /* DPRINT(+1, "Console Status Issue Received\n"); */
     consolestatAction(recvIssue);

   }  
   return RTI_TRUE;
}

#else /* RTI_NDDS_4x */

void Console_StatCallback(void* listener_data, DDS_DataReader* reader)
{
   Console_Stat *recvIssue;
   void consolestatAction(Console_Stat *data);
   struct DDS_SampleInfo* info = NULL;
   struct DDS_SampleInfoSeq info_seq = DDS_SEQUENCE_INITIALIZER;
   DDS_ReturnCode_t retcode;
   DDS_Boolean result;
   long i,numIssues;
   DDS_TopicDescription *topicDesc;


   struct Console_StatSeq data_seq = DDS_SEQUENCE_INITIALIZER;
   Console_StatDataReader *Console_Stat_reader = NULL;

   Console_Stat_reader = Console_StatDataReader_narrow(pStatSub->pDReader);
   if ( Console_Stat_reader == NULL)
   {
        errLogRet(LOGIT,debugInfo,"DataReader narrow error\n");
        return;
   }

   topicDesc = DDS_DataReader_get_topicdescription(reader);
   DPRINT2(1,"Console_StatCallback: Type: '%s', Name: '%s'\n",
      DDS_TopicDescription_get_type_name(topicDesc), DDS_TopicDescription_get_name(topicDesc));
        retcode = Console_StatDataReader_take(Console_Stat_reader,
                              &data_seq, &info_seq,
                              DDS_LENGTH_UNLIMITED, DDS_ANY_SAMPLE_STATE,
                              DDS_ANY_VIEW_STATE, DDS_ANY_INSTANCE_STATE);

        if (retcode == DDS_RETCODE_NO_DATA) {
                 return; // break; // return;
        } else if (retcode != DDS_RETCODE_OK) {
                 errLogRet(LOGIT,debugInfo,"next instance error %d\n",retcode);
                 return; // break; // return;
        }

        numIssues = Console_StatSeq_get_length(&data_seq);
        DPRINT1(1,"Console_StatCallback: numIssues: %d\n",numIssues);

        for (i=0; i < numIssues; i++)
        {
           info = DDS_SampleInfoSeq_get_reference(&info_seq, i);
           if (info->valid_data)
           {
              recvIssue = (Console_Stat *) Console_StatSeq_get_reference(&data_seq,i);
              consolestatAction(recvIssue);
           }
        }
        retcode = Console_StatDataReader_return_loan( Console_Stat_reader,
                  &data_seq, &info_seq);
   return;
}

#endif  /* RTI_NDDS_4x */
/**************************************************************
*
*  initiateNDDS - Initialize a NDDS Domain for  communications 
*   
***************************************************************/
void initiateNDDS(int debuglevel)
{
    char localIP[80];
    /* NDDS_ID nddsCreate(int domain, int debuglevel, int multicast, char *nicIP) */

#ifndef NO_MULTICAST
    NDDS_Domain = nddsCreate(NDDS_DOMAIN_NUMBER,debuglevel,MULTICAST_ENABLE,(char*) getHostIP(ConsoleHostName,localIP));
#else
    NDDS_Domain = nddsCreate(NDDS_DOMAIN_NUMBER,debuglevel,MULTICAST_DISABLE,(char*) getHostIP(ConsoleHostName,localIP));
#endif

   if (NDDS_Domain == NULL)
      errLogQuit(LOGOPT,debugInfo,"Sendproc: initiateNDDS(): NDDS domain failed to initialized\n");
}


void DestroyDomain()
{
#ifndef RTI_NDDS_4x
   if (NDDS_Domain->domain != NULL)
   {
      DPRINT1(1,"Infoproc: Destroy Domain: 0x%lx\n",NDDS_Domain->domain);
      NddsDestroy(NDDS_Domain->domain); /* NddsDomainHandleGet(0) */
      usleep(400000);  /* 400 millisec sleep, give time for msge to be sent to NddsManager */
   }
#else /* RTI_NDDS_4x */
   if (NDDS_Domain != NULL)
   {
      DPRINT1(1,"Infoproc: Destroy Domain: 0x%lx\n",NDDS_Domain);
      NDDS_Shutdown(NDDS_Domain); /* NddsDomainHandleGet(0) */
      usleep(400000);  /* 400 millisec sleep, give time for msge to be sent to NddsManager */
   }
#endif  /* RTI_NDDS_4x */
}


NDDS_ID  createConsoleStatSub(char *subName)
{
    NDDS_ID pSubObj;
 
    /* Build Data type Object for both publication and subscription to Expproc */
    /* ------- malloc space for data type object --------- */
    if ( (pSubObj = (NDDS_ID) malloc( sizeof(NDDS_OBJ)) ) == NULL )
    {
        return(NULL);
    }

    /* DPRINT2(-1,"---> pipe fd[0]: %d, fd[1]: %d\n",lockSubPipeFd[0],lockSubPipeFd[1]); */
    /* zero out structure */
    memset(pSubObj,0,sizeof(NDDS_OBJ));
    memcpy(pSubObj,NDDS_Domain,sizeof(NDDS_OBJ));

    strcpy(pSubObj->topicName,subName);           

    /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */
    getConsole_StatInfo(pSubObj);
 
    /* NDDS issue callback routine */
#ifndef RTI_NDDS_4x
    pSubObj->callBkRtn = Console_StatCallback;
    pSubObj->callBkRtnParam = NULL;
#endif  /* RTI_NDDS_4x */
    pSubObj->MulticastSubIP[0] = 0;   /* use UNICAST */
    /* i.e. 5 times per second max */
    pSubObj->BE_UpdateMinDeltaMillisec = 250; 
#ifdef RTI_NDDS_4x
    initBESubscription(pSubObj);
    attachOnDataAvailableCallback(pSubObj, Console_StatCallback, NULL);
    createSubscription(pSubObj);
#else /* RTI_NDDS_4x */
    createBESubscription(pSubObj);
#endif  /* RTI_NDDS_4x */
 
    return(pSubObj);
}



#ifdef XXXX

*int createAcqStatdPublication(char *pubName)
*{
*     int result;
* 
*    /* Build Data type Object for both publication and subscription to Expproc */
*    /* ------- malloc space for data type object --------- */
*    if ( (pPubObj = (NDDS_ID) malloc( sizeof(NDDS_OBJ)) ) == NULL )
*      {
*        return(-1);
*      }  
*
* 
*    /* zero out structure */
*    memset(pPubObj,0,sizeof(NDDS_OBJ));
*    memcpy(pPubObj,NDDS_Domain,sizeof(NDDS_OBJ));
* 
*    strcpy(pPubObj->topicName,pubName);
* 
*    /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */
*    getAcq_StatInfo(pPubObj);
*    createBEPublication(pPubObj);
*    return(0);
*}
* 
*killOffSub()
*{
*   int stat;
*    stat = nddsPublicationDestroy(pPubObj);
*    printf("Pub destroy: %d\n",stat);
*    stat = nddsSubscriptionDestroy(pSubObj);
*    printf("Sub destroy: %d\n",stat);
*}
*
#endif

int initStatusSub()
{
   /* topic names form: sub: master/h/constat, h/acqstat */
   pStatSub = createConsoleStatSub(HOST_SUB_STAT_TOPIC_FORMAT_STR);
#ifndef RTI_NDDS_4x
   CntlrSubscriber = NULL;
#endif  /* RTI_NDDS_4x */
   return 0;
}

#ifndef RTI_NDDS_4x
int initHBSubs()
{

   /* topic names form: sub: master/h/constat, h/acqstat */
   pStatSub = createConsoleStatSub(HOST_SUB_STAT_TOPIC_FORMAT_STR);
   nodeHeartBeatPatternSub();
   intExpprocHBSub();
   /* createCodeDownldPublication(pCntlr,"h/rf1/dwnld/strm"); */
   return 0;

}
#endif  /* RTI_NDDS_4x */


static void
locateCurrentShims( char *currentShimsFile )
{
        strcpy( currentShimsFile, (char*) getenv("vnmrsystem") );
        if (strlen( currentShimsFile ) < (size_t) 1)
          strcpy( currentShimsFile, "/vnmr" );
        strcat( currentShimsFile, "/acqqueue/currentShimValues" );
}


#define MAXPATHL	128

static void
updateCurrentShims()
{
        char    file[MAXPATHL];

        locateCurrentShims( &file[ 0 ] );
        writeConsolseStatusBlock( &file[ 0 ] );
}

void consolestatAction(Console_Stat *data)
{
   CONSOLE_STATUS *csbPtr;
   int stat, shimsChanged;

   /* need address past the dataTypeID member to be equivilent to CONSOLE_STATUS */
   csbPtr = (CONSOLE_STATUS *) &(data->AcqCtCnt);

   /*
    *  DPRINT1(-1,"Acqstate: %d\n",csbPtr->Acqstate);
    *  DPRINT1(-1,"AcqCtCnt: %d\n",csbPtr->AcqCtCnt);
    *  DPRINT1(-1,"AcqFidCnt: %d\n",csbPtr->AcqFidCnt);
    *  DPRINT1(-1,"AcqSample: %d\n",csbPtr->AcqSample);
    *  DPRINT1(-1,"AcqLockFreqAP: %d\n",csbPtr->AcqLockFreqAP);
  */

/*
#ifdef LINUX
   CSB_CONVERT(csbPtr);
#endif
*/

#ifndef RTI_NDDS_4x   /* not used in 4x */
   if (CntlrSubscriber != NULL)   /* if HB subscription never start then skip this test */
   {
      DPRINT6(+4,"'%s': }}}}}}}}}}++++=====>> Expproc_HB: %d, Master_HB: %d, DDR_HB: %d, currentHB: %d, totalHB: %d\n",
          ProcName,Expproc_HB, Master_HB, DDR_HB, currentHB_Subscriptions, totalHB_Subscriptions);

      if ( (Expproc_HB != 1) || (Master_HB != 1) || (DDR_HB != 1) || (totalHB_Subscriptions != currentHB_Subscriptions) )
      {
          DPRINT3(+3,"'%s': -------->>  Lost a Node, current: %d, total: %d, Force status inactive\n",
		ProcName,currentHB_Subscriptions,totalHB_Subscriptions);
          csbPtr->Acqstate = ACQ_INACTIVE;
      }
   }
#endif  /* RTI_NDDS_4x */
 

  DPRINT4(1, "Channel Bits: %d  %d  %d  %d\n",(int)(csbPtr->AcqChannelBitsConfig1), \
     (int)(csbPtr->AcqChannelBitsConfig2),(int)(csbPtr->AcqChannelBitsActive1),     \
     (int)(csbPtr->AcqChannelBitsActive2) );
 /*
   shimsChanged = compareShimsConsoleStatusBlock( (CONSOLE_STATUS *) csbPtr );
 */
   if (csbPtr->Acqstate == ACQ_REBOOT)
   {
      MSG_Q_ID pExpMsgQ;
      pExpMsgQ = openMsgQ("Expproc");
      if (pExpMsgQ)
      {
         setStatAcqState(ACQ_REBOOT);
         pthread_kill(main_threadId,SIGUSR2); 
         sendMsgQ(pExpMsgQ, "rebooted", strlen("rebooted"), MSGQ_NORMAL, WAIT_FOREVER);
         closeMsgQ(pExpMsgQ);
         usleep(2000000);  /* 200 sec sleep, give time for msge to be sent and shims to download */
      }
      return;
   }
   stat = receiveConsoleStatusBlock( csbPtr );
   /* DPRINT1(-1, "case STATBLK: statblock memcmp = %d\n",stat); */
   /* if (stat != 0 && ActiveExpInfo.ShrExpInfo != NULL &&
    *        ACQ_ACQUIRE == getStatAcqState()) { */

   /* this shows Fid & Ct coming correct even Vnmrj show bizzare values */
   /* DPRINT2(+6,"Rcvd: Fid: %lu, Ct: %lu\n",(unsigned long)csbPtr->AcqFidCnt,(unsigned long)csbPtr->AcqCtCnt); */
   if (stat != 0 && ACQ_ACQUIRE == getStatAcqState()) 
   {
       long  ctCnt;
                         
       /*  The CT counter is now kept by the console;
           Expproc just transfers the value so Infoproc
           and Acqstat can get at it.  July 1997     */
         
       ctCnt = getStatAcqCtCnt();
       setStatCT((unsigned long) ctCnt);
       setStatElem((unsigned long)csbPtr->AcqFidCnt);
  }
 
  if (stat != 0) 
  {
      int sample;
        
      /* The Gilson sample preparation system can cause
         the sample value to take on large values.  Keep
         this value in the range [0, 999].    July 1997   */
        
      encodedAcqSample = data->AcqSample;
      sample = getStatAcqSample();
      sample %= 1000;
      setStatAcqSample( sample );
        
      /* sigInfoproc();  /* signal Infoproc to check status */
/*
      if (shimsChanged)
      {
         updateCurrentShims();
         DPRINT( 1, "shims changed\n" );
      }
 */
      /* signal Console Status has change to the main thread */
      pthread_kill(main_threadId,SIGUSR2); 
  }
}

/* --------------------------  Heart Beat Listener ---------------- */

typedef struct node_tag {
   int type;
   int number;
   int recvSub;
   char name[32];
 } node_t;

#define MAX_BRD_TYPE 8
static char *brdTypeStr[9] = { "master", "rf", "pfg", "gradient" , "lock", "ddr", "reserved1", "reserved2", "Expproc" };

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

#ifndef RTI_NDDS_4x
/* VxWorks controller Node HB subscription callback */
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
 
/*---------------------------------------------------------------------------------- */


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

     DPRINT4(1,"'%s': App_HBPatternSubCreate(): Topic: '%s', Type: '%s', arg: 0x%lx\n",
                ProcName,nddsTopic, nddsType, callBackRtnParam);
     strncpy(cntrlName,nddsTopic,127);
     chrptr = strchr(cntrlName,'/');
     *chrptr = 0;

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
 
     /* setup expproc HB specifically via intExpprocHBSub() */
     /* so it will not be a pattrern that is ever matched */
     /* must have Expproc for proper operation */
     /* if (pNodeTag->type == 8)
      *    Expproc_HB = 1;  */
 
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
int nodeHeartBeatPatternSub()
{
    /* MASTER_SUB_COMM_PATTERN_TOPIC_STR */
    CntlrSubscriber = NddsSubscriberCreate(NDDS_DOMAIN_NUMBER);
 
    /* All VxWorks nodes */
    NddsSubscriberPatternAdd(CntlrSubscriber,
           nodeHB_PATTERN_FORMAT_STR, App_HBNDDSType , App_HBPatternSubCreate, NULL);

    return 0;
}     

intExpprocHBSub()
{
    char exptopic[32];
    node_t *pNodeTag;
    NDDS_ID pHB_SubId;

    pNodeTag = (node_t *) malloc(sizeof(node_t));
    strcpy(pNodeTag->name,"Expproc");
    pNodeTag->recvSub = 0;
    cntlrName2TypeNNum("Expproc", &(pNodeTag->type), &(pNodeTag->number));
     
   /* Expproc */
    sprintf(exptopic,SUB_AppHB_TOPIC_FORMAT_STR,"Expproc");
    DPRINT2(+1,"'%s': sub to pattern: '%s'\n",ProcName,exptopic);
    pHB_SubId  = createAppHB_BESubscription((char*) exptopic,(void*) Node_HBCallback, pNodeTag);
    totalHB_Subscriptions++;
    Expproc_HB = 1;

}
#endif  /* RTI_NDDS_4x */
 


