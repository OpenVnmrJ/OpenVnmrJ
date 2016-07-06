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
modification history
--------------------
5-05-04,gmb  created 
*/

/*
DESCRIPTION

    Lock communication functions

*/
#ifndef ALLREADY_POSIX
 #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#endif
#include <string.h>
#include <vxWorks.h>
#include <stdioLib.h>
#include <sysLib.h>
#include <semLib.h>
#include <rngLib.h>
#include <msgQLib.h>

#include "errorcodes.h"
#include "instrWvDefines.h"

#include "logMsgLib.h"

#include "NDDS_Obj.h"
#include "NDDS_PubFuncs.h"
#include "NDDS_SubFuncs.h"

#include "Lock_Cmd.h"
#include "Lock_FID.h"
#include "Lock_Stat.h"

#ifdef RTI_NDDS_4x
#include "Lock_CmdPlugin.h"
#include "Lock_FIDPlugin.h"
#include "Lock_StatPlugin.h"
#include "Lock_CmdSupport.h"
#include "Lock_FIDSupport.h"
#include "Lock_StatSupport.h"
#endif

extern NDDS_ID NDDS_Domain;

/* extern NDDS_ID pMonitorPub, pMonitorSub; */

/* subscriber to create subscriptions to contoller publications dynamicly */
/* haven't decide who will actually send lock commands host or master, but
   by using a subscriber and pattern match we can change on the fly       */

#ifndef RTI_NDDS_4x
static NDDSSubscriber LockCmdSubscriber;
#endif

/* subscriptions to controller publications */
NDDS_ID pLockCmdSubs[5];

/* Lock FID Pub */
NDDS_ID pLockFIDPub = NULL;
Lock_FID *pLkFIDIssue = NULL;
/* Lock status Pub */
NDDS_ID pLockStatPub = NULL;
Lock_FID *pLkStatIssue = NULL;

int numLockCmdSubs = 0;


extern int DebugLevel;

MSG_Q_ID pMsgesToLockParser = NULL;

static int callbackParam = 0;

/*
 *   The NDDS callback routine, the routine is call when an issue of the subscribed topic
 *   is delivered.
 *   called with the context of the NDDS task n_rtu7400
 *
 */
#ifndef RTI_NDDS_4x
RTIBool Lock_CmdCallback(const NDDSRecvInfo *issue, NDDSInstance *instance,
                             void *callBackRtnParam)
{
    Lock_Cmd *recvIssue;
    void LockCmdParser(Lock_Cmd *issue);
    int *param;
 
    if (issue->status == NDDS_FRESH_DATA)
    {
        recvIssue = (Lock_Cmd *) instance;
        DPRINT5(-1,"Lock_Cmd CallBack:  cmd: %d, arg1: %d, arg2: %d, arg3: %lf, arg4: %lf crc: 0x%lx\n",
        recvIssue->cmd,recvIssue->arg1, recvIssue->arg2, recvIssue->arg3, recvIssue->arg4);
        msgQSend(pMsgesToLockParser, (char*) recvIssue, sizeof(Lock_Cmd), NO_WAIT, MSG_PRI_NORMAL);
        /* LockCmdParser(recvIssue); */
    }
   return RTI_TRUE;
}
#else /* RTI_NDDS_4x */
/* 4x callback */
void Lock_CmdCallback(void* listener_data, DDS_DataReader* reader)
{
   Lock_Cmd *recvIssue;
   DDS_ReturnCode_t retcode;
   DDS_Boolean result;
   struct DDS_SampleInfoSeq info_seq = DDS_SEQUENCE_INITIALIZER;
   struct Lock_CmdSeq data_seq = DDS_SEQUENCE_INITIALIZER;
   struct DDS_SampleInfo* info = NULL;
   long i,numIssues;
   Lock_CmdDataReader *LockCmd_reader = NULL;
   DDS_TopicDescription *topicDesc;


   LockCmd_reader = Lock_CmdDataReader_narrow(pLockCmdSubs[0]->pDReader);
   if ( LockCmd_reader == NULL)
   {
        errLogRet(LOGIT,debugInfo, "Lock_CmdCallback: DataReader narrow error.\n");
        return;
   }
   topicDesc = DDS_DataReader_get_topicdescription(reader); 
   DPRINT2(-1,"Lock_CmdCallback; Type: '%s', Name: '%s'\n",
       DDS_TopicDescription_get_type_name(topicDesc), DDS_TopicDescription_get_name(topicDesc));
   while(1)
   {
      // Given DDS_HANDLE_NIL as a parameter, take_next_instance returns
      // a sequence containing samples from only the next (in a well-determined
      // but unspecified order) un-taken instance.
      retcode =  Lock_CmdDataReader_take_next_instance(
            LockCmd_reader,
            &data_seq, &info_seq, DDS_LENGTH_UNLIMITED,
            &DDS_HANDLE_NIL,
            DDS_ANY_SAMPLE_STATE, DDS_ANY_VIEW_STATE, DDS_ANY_INSTANCE_STATE);

        // retcode = Lock_CmdDataReader_take(LockCmd_reader,
        //                       &data_seq, &info_seq,
        //                       DDS_LENGTH_UNLIMITED, DDS_ANY_SAMPLE_STATE,
        //                       DDS_ANY_VIEW_STATE, DDS_ANY_INSTANCE_STATE);

      if (retcode == DDS_RETCODE_NO_DATA) {
            break ; // return;
      } else if (retcode != DDS_RETCODE_OK) {
            errLogRet(LOGIT,debugInfo, "Lock_CmdCallback: next instance error %d\n",retcode);
            break ; // return;
      }


      numIssues = Lock_CmdSeq_get_length(&data_seq);

      for (i=0; i < numIssues; i++)
      {

         info = DDS_SampleInfoSeq_get_reference(&info_seq, i);
         if ( info->valid_data)
         {
             recvIssue = Lock_CmdSeq_get_reference(&data_seq,i);
             DPRINT5(-1,"Lock_Cmd CallBack:  cmd: %d, arg1: %d, arg2: %d, arg3: %lf, arg4: %lf crc: 0x%lx\n",
             recvIssue->cmd,recvIssue->arg1, recvIssue->arg2, recvIssue->arg3, recvIssue->arg4);
             msgQSend(pMsgesToLockParser, (char*) recvIssue, sizeof(Lock_Cmd), NO_WAIT, MSG_PRI_NORMAL);
         }
      }
      retcode = Lock_CmdDataReader_return_loan( LockCmd_reader,
                  &data_seq, &info_seq);
      DDS_SampleInfoSeq_set_maximum(&info_seq, 0);
   }  // while

   return;
}
 
#endif /* RTI_NDDS_4x */

#ifdef XXX
/*
 * just initial routine to send cmds, not complete yet
 * not all structure member filled in
 *
 */
*LockCmdPub(int cmd,int arg1,int arg2,double arg3, double arg3)
*{
*   Lock_Cmd *issue;
*   issue = pLockCmdPub->instance;
*   issue->cmd  = cmd; 
*   issue->arg1 = arg1;
*   issue->arg2 = arg2;
*   issue->arg3 = arg3;
*   issue->arg4 = arg4;
*   nddsPublishData(pLockCmdPub);
*}
#endif

tstLkMsgQ()
{
   Lock_Cmd tstIssue;
   tstIssue.cmd = LK_SET_PHASE;
   tstIssue.arg1 = 180;
   tstIssue.arg2 = 0;
   tstIssue.arg3 = 0.0;
   tstIssue.arg4 = 0.0;
  
   msgQSend(pMsgesToLockParser, (char*) &tstIssue, sizeof(Lock_Cmd), NO_WAIT, MSG_PRI_NORMAL);
}

void LkParser()
{
   Lock_Cmd  cmdIssue;
   int	ival;
   void LockCmdParser(Lock_Cmd *issue);

   FOREVER 
   {
      ival = msgQReceive(pMsgesToLockParser,(char*) &cmdIssue,sizeof(Lock_Cmd),WAIT_FOREVER);
      DPRINT1( 2, "Lkparse:  msgQReceive returned %d\n", ival );
      if (ival == ERROR)
      {
         printf("LOCK PARSER Q ERROR\n");
         return;
      }
      else
      {
         LockCmdParser(&cmdIssue);
      }
   }
}


/*
 * The LockCmdParser, this is the command interpreter for the lock controller
 * Any publication to star/lock/cmd pattern will be processed here.
 * Typical commands are to set the power,gain, phase and frequency.
 *
 *   Author: Greg Brissey 5/05/04
 */

/* static int cmdledtog = 0; */

void LockCmdParser(Lock_Cmd *issue)
{
   int len;
   int cmd;
   DPRINT5(-2,"LockCmdParser cmd: %d, arg1: %d, arg2: %d, arg3: %lf, arg4: %lf\n", issue->cmd,
		issue->arg1,issue->arg2,issue->arg3,issue->arg4);

#ifdef NEED_SOME_TOJUST_PULSE_THELED
   if (cmdledtog)
     {
        panelLedOn(2); cmdledtog = 0;
     }
     else
     {
        panelLedOff(2); cmdledtog = 1;
     }
#endif

   switch( issue->cmd )
   {
     case LK_SET_GAIN:
	  {
   	        DPRINT(-1,"LK_SET_GAIN\n");
                setLockGain(issue->arg1);
	  }
	  break;
     case LK_SET_POWER:
	  {
   	        DPRINT(-1,"LK_SET_POWER\n");
                setLockPower(issue->arg1);
	  }
	  break;
     case LK_SET_PHASE:
	  {
   	        DPRINT(-1,"LK_SET_PHASE\n");
                setLockPhase(issue->arg1);
	  }
	  break;
     case LK_ON:
	  {
   	        DPRINT(-1,"LK_ON\n");
                pulser_on();
	  }
	  break;
     case LK_OFF:
	  {
   	        DPRINT(-1,"LK_OFF\n");
                pulser_off();
	  }
	  break;
     case LKRATE:
          {    
                DPRINT1(-1,"LKRATE @ %f",issue->arg3);
                /* -12 is used as a key, depending on the value of lockpower */
                /* If lockpower is 0, use 0, else use 12.0                   */
                lockRate(issue->arg3,-12.0);
          }
          break;
     case LK_SET_RATE:
	  {
   	        DPRINT(-1,"LK_SET_MODE\n");
                /* lockRate(double hz, double duty) */
                lockRate(issue->arg3, issue->arg4);
	  }
	  break;
     case LK_SET_FREQ:
	  {
   	        DPRINT1(-1,"LK_SET_FREQ %f\n",issue->arg3);
                /* void setLockDDS(double freq) */
                setLockDDS(issue->arg3*1e6);
	  }
	  break;

/* was going to put autolock on lock controller, but many things are controller by master
   and it just makes it simplier at this point to put it on the master

        GMB    1/6/05
*/
#ifdef AUTOLOCK_ON_LOCKCNTLR

     case LK_AUTOLOCK:
	  {
   	        DPRINT3(-1,"LK_AUTOLOCK: Mode: %d, Max. Power: %d, Max. Gain: %d\n",issue->arg1, issue->arg2, (int) issue->arg3);
                 /* do_autolock(mode,maxpower,maxgain); */
                 /* msgQSend( ); */
	  }
	  break;
#endif


     default:
	    DPRINT1(-1,"Unknown command: %d\n",issue->cmd);
            break;

   }
   return;
}


#ifdef NOT_USED_HERE_BUT_IS_IN_MONITOR_C
/*
 * Create a Publication Topic to communicate with the Lock
 *
 *					Author Greg Brissey 5-06-04
 */
NDDS_ID createLockCmdPub(NDDS_ID nddsId, char *topic, char *cntlrName)
{
     int result;
     NDDS_ID pPubObj;
     char pubtopic[128];
     Lock_Cmd  *issue;

    /* Build Data type Object for both publication and subscription to Expproc */
    /* ------- malloc space for data type object --------- */
    if ( (pPubObj = (NDDS_ID) malloc( sizeof(NDDS_OBJ)) ) == NULL )
      {  
        return(NULL);
      }  

    /* zero out structure */
    memset(pPubObj,0,sizeof(NDDS_OBJ));
    memcpy(pPubObj,nddsId,sizeof(NDDS_OBJ));

    strcpy(pPubObj->topicName,topic);
    pPubObj->pubThreadId = 89;    /* DEFAULT_PUB_THREADID; */
         
    /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */
    getLock_CmdInfo(pPubObj);
         
    DPRINT2(-1,"Create Pub topic: '%s' for Cntlr: '%s'\n",pPubObj->topicName,cntlrName);
    createPublication(pPubObj);
    issue = (Lock_Cmd  *) pPubObj->instance;
    return(pPubObj);
}        
#endif

/*
 * Create a Subscription Topic to communicate with the Controllers/Master
 *
 *					Author Greg Brissey 4-26-04
 */
NDDS_ID createLockCmdSub(NDDS_ID nddsId, char *topic, void *callbackArg)
{

   NDDS_ID  pSubObj;
   char subtopic[128];

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
    getLock_CmdInfo(pSubObj);
 
#ifndef RTI_NDDS_4x
    pSubObj->callBkRtn = Lock_CmdCallback;
    pSubObj->callBkRtnParam = callbackArg;
#endif  /* RTI_NDDS_4x */
    pSubObj->MulticastSubIP[0] = 0;   /* use UNICAST */

#ifdef RTI_NDDS_4x
    initSubscription(pSubObj);
    attachOnDataAvailableCallback(pSubObj,Lock_CmdCallback,&callbackArg);
#endif  /* RTI_NDDS_4x */
    createSubscription(pSubObj);
    return ( pSubObj );
}

#ifndef RTI_NDDS_4x
/*
 * The Lock via NDDS uses this callback function to create Subscriptions to the
 * Host/MAster Lock Command Publications aimed at the Lock Controller 
 *
 *					Author Greg Brissey 5-05-04
 */
NDDSSubscription Lock_CmdPatternSubCreate( const char *nddsTopic, const char *nddsType, 
                  void *callBackRtnParam) 
{ 
     NDDSSubscription pSub;
     DPRINT3(-1,"Lock_CmdPatternSubCreate(): Topic: '%s', Type: '%s', arg: 0x%lx\n",
		nddsTopic, nddsType, callBackRtnParam);
     DPRINT2(-1,"callbackParam: 0x%lx, callBackRtnParam: 0x%lx\n",&callbackParam,callBackRtnParam);
     pLockCmdSubs[numLockCmdSubs]  = createLockCmdSub(NDDS_Domain, (char*) nddsTopic, (void *) &callbackParam );
     pSub = pLockCmdSubs[numLockCmdSubs++]->subscription;
     return pSub;
}

/*
 *  Lock creates a pattern subscriber, to dynamicly allow subscription creation
 *  as host/master come on-line and publish to the Lock Parser 
 *
 *					Author Greg Brissey 5-06-04
 */
lockCmdPubPatternSub()
{
    LockCmdSubscriber = NddsSubscriberCreate(0);

    /* Lock subscribe to any publications from controllers */
    /* star/lock/cmds */
    NddsSubscriberPatternAdd(LockCmdSubscriber,  
           LOCK_SUB_CMDS_PATTERN_TOPIC_STR, Lock_CmdNDDSType , Lock_CmdPatternSubCreate, (void *)callbackParam); 
}
#endif  /* RTI_NDDS_4x */

startLockParser(int priority, int taskoptions, int stacksize)
{
   DPRINT1(-1,"sizeof(Lock_Cmd) = %d\n", sizeof(Lock_Cmd));
   if (pMsgesToLockParser == NULL)
      pMsgesToLockParser = msgQCreate(300,sizeof(Lock_Cmd),MSG_Q_PRIORITY);
   if (pMsgesToLockParser == NULL)
   {
      errLogSysRet(LOGIT,debugInfo,"could not create Lock Parser MsgQ, ");
      return;
   }
   
   if (taskNameToId("tLkParser") == ERROR)
      taskSpawn("tLkParser",priority,0,stacksize,LkParser,pMsgesToLockParser,
						2,3,4,5,6,7,8,9,10);
}

killLkParser()
{
   int tid;
   if ((tid = taskNameToId("tLkParser")) != ERROR)
      taskDelete(tid);
}

void initialLockComm()
{
   NDDS_ID createLockFIDPub(NDDS_ID nddsId, char *topic);
   NDDS_ID createLockStatPub(NDDS_ID nddsId, char *topic);
#ifndef RTI_NDDS_4x
   lockCmdPubPatternSub();
#else  /* RTI_NDDS_4x */
   pLockCmdSubs[0]  = createLockCmdSub(NDDS_Domain,  LOCK_CMDS_TOPIC_STR, (void *) &callbackParam );
   numLockCmdSubs = 1;
#endif  /* RTI_NDDS_4x */
   pLockFIDPub = createLockFIDPub(NDDS_Domain,(char*) LOCK_PUB_FID_TOPIC_FORMAT_STR);
   pLkFIDIssue = pLockFIDPub->instance;
   pLockStatPub = createLockStatPub(NDDS_Domain,(char*) LOCK_PUB_STAT_TOPIC_FORMAT_STR);
   /* StatFilter();  test of ndds filtering */
   pLkStatIssue = pLockStatPub->instance;
}

/*
 * Create a Best Effort Publication Topic to communicate the Lock FID
 * Information
 *
 *					Author Greg Brissey 5-06-04
 */
NDDS_ID createLockFIDPub(NDDS_ID nddsId, char *topic)
{
     int result;
     NDDS_ID pPubObj;
     char pubtopic[128];
     Lock_FID  *issue;

    /* Build Data type Object for both publication and subscription to Expproc */
    /* ------- malloc space for data type object --------- */
    if ( (pPubObj = (NDDS_ID) malloc( sizeof(NDDS_OBJ)) ) == NULL )
      {  
        return(NULL);
      }  

    /* zero out structure */
    memset(pPubObj,0,sizeof(NDDS_OBJ));
    memcpy(pPubObj,nddsId,sizeof(NDDS_OBJ));

    strcpy(pPubObj->topicName,topic);
    pPubObj->pubThreadId = 83;	/* DEFAULT_PUB_THREADID; */
         
    /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */
    getLock_FIDInfo(pPubObj);
         
    DPRINT1(-1,"createLockFIDPub: topic: '%s' \n",pPubObj->topicName);
#ifndef RTI_NDDS_4x
    createBEPublication(pPubObj);
    /* issue = (Lock_FID  *) pPubObj->instance; */
#else  /* RTI_NDDS_4x */
    initBEPublication(pPubObj);
    createPublication(pPubObj);
#endif  /* RTI_NDDS_4x */
    return(pPubObj);
}        

#ifndef RTI_NDDS_4x
void pubLkFID()
{
   if (pLockFIDPub != NULL)
       nddsPublishData(pLockFIDPub);
}

#else  /* RTI_NDDS_4x */

int pubLkFID(short *pData, int sizeInShorts)
{
   DDS_ReturnCode_t result;
   DDS_InstanceHandle_t instance_handle = DDS_HANDLE_NIL;
   Lock_FIDDataWriter *LockFID_writer = NULL;
   Lock_FID *issue;

   if (pLockFIDPub != NULL)
   {
     LockFID_writer = Lock_FIDDataWriter_narrow(pLockFIDPub->pDWriter);
     if (LockFID_writer == NULL) {
        errLogRet(LOGIT,debugInfo, "pubLkFID: DataReader narrow error.\n");
        return -1;
      }

      issue = (Lock_FID*) pLockFIDPub->instance;
      // DDS_ShortSeq_from_array(&(issue->lkfid), pData, sizeInShorts);
      DDS_ShortSeq_set_maximum(&(issue->lkfid),0);
      DDS_ShortSeq_loan_contiguous(&(issue->lkfid), (short*) pData, sizeInShorts, sizeInShorts );
      // DDS_ShortSeq_set_length(&(issue->lkfid), sizeInShorts);

      DPRINT(+4,"publish Lock_FID data\n");
      result = Lock_FIDDataWriter_write(LockFID_writer,
                pLockFIDPub->instance,&instance_handle);
     if (result != DDS_RETCODE_OK) {
            errLogRet(LOGIT,debugInfo, "pubLkFID: write error %d\n",result);
     }
     DDS_ShortSeq_unloan(&(issue->lkfid));
  }
  return 0;
}
tstLkFidPub()
{
   short data[512];
   int i;
   for(i=0;i<512;i++) data[i] = i;
   pubLkFID(data, 512);
}
#endif  /* RTI_NDDS_4x */

/*
 * Create a Best Effort Publication Topic to communicate the Lock Stat
 * Information
 *
 */
NDDS_ID createLockStatPub(NDDS_ID nddsId, char *topic)
{
     int result;
     NDDS_ID pPubObj;
     char pubtopic[128];

    /* Build Data type Object for both publication and subscription to Expproc */
    /* ------- malloc space for data type object --------- */
    if ( (pPubObj = (NDDS_ID) malloc( sizeof(NDDS_OBJ)) ) == NULL )
      {  
        return(NULL);
      }  

    /* zero out structure */
    memset(pPubObj,0,sizeof(NDDS_OBJ));
    memcpy(pPubObj,nddsId,sizeof(NDDS_OBJ));

    strcpy(pPubObj->topicName,topic);
    pPubObj->pubThreadId = 97;	/* DEFAULT_PUB_THREADID; */
         
    /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */
    getLock_StatInfo(pPubObj);
         
    DPRINT1(-1,"createLockStatPub: topic: '%s' \n",pPubObj->topicName);
#ifndef RTI_NDDS_4x
    createBEPublication(pPubObj);
#else  /* RTI_NDDS_4x */
    initBEPublication(pPubObj);
    createPublication(pPubObj);
#endif  /* RTI_NDDS_4x */
    return(pPubObj);
}        

#ifdef NDDS_SENDBEFORERTN_AFTERSENDRTN

static ledtoggle = 0;

RTIBool ledToggle(const char *nddsTopic, const char *nddsType, NDDSInstance *instance, void *beforeRtnParam)
{
   int led;
   led = *((int *) beforeRtnParam);
   if (ledtoggle)
     {
        panelLedOn(2); ledtoggle = 0;
     }
     else
     {
        panelLedOff(2); ledtoggle = 1;
     }
   return RTI_TRUE;
}

static ledtog = 0;
RTIBool led3Toggle(const char *nddsTopic, const char *nddsType, NDDSInstance *instance, void *beforeRtnParam)
{
   if (ledtog)
     {
        panelLedOn(4); ledtog = 0;
     }
     else
     {
        panelLedOff(4); ledtog = 1;
     }
   return RTI_TRUE;
}
StatFilter()
{
   NDDSPublicationListener listener;

   NddsPublicationListenerDefaultGet(&listener);
   listener.sendBeforeRtn = ledToggle;
   listener.afterSendRtn = led3Toggle;
   NddsPublicationListenerSet((pLockStatPub->publication), &listener);
}
#endif

SEM_ID	pSemLockStatPub;
void startLockStatPub(int priority, int options, int stackSize)
{
    void LockStatPublisher();
    int LSP;
    if (pSemLockStatPub == 0)
        pSemLockStatPub = semBCreate(SEM_Q_FIFO,SEM_EMPTY);
    LSP = taskSpawn("LockStatPub",priority,options,stackSize,
                (void *)LockStatPublisher,0,0,0,0,0,0,0,0,0,0);
    if (LSP == ERROR)
    {
       perror("taskSpawn LSP");
       semDelete(pSemLockStatPub);
    }
}


#ifdef RTI_NDDS_4x
// helper function for routine below 
int pubLockStat(NDDS_ID pNDDS_Obj)
{
   DDS_ReturnCode_t result;
   DDS_InstanceHandle_t instance_handle = DDS_HANDLE_NIL;
   Lock_StatDataWriter *LockStat_writer = NULL;

   if (pNDDS_Obj != NULL)
   {
     LockStat_writer = Lock_StatDataWriter_narrow(pNDDS_Obj->pDWriter);
     if (LockStat_writer == NULL) {
        errLogRet(LOGIT,debugInfo, "pubLockStat: DataReader narrow error.\n");
        return -1;
      }
     result = Lock_StatDataWriter_write(LockStat_writer,
                pNDDS_Obj->instance,&instance_handle);
     if (result != DDS_RETCODE_OK) {
            errLogRet(LOGIT,debugInfo, "pubLockStat: write error %d\n",result);
     }
  }
}
#endif  /* RTI_NDDS_4x */

void LockStatPublisher()
{
   FOREVER
   {
       semTake(pSemLockStatPub, WAIT_FOREVER);
       if (pLockStatPub != NULL)
#ifndef RTI_NDDS_4x
           nddsPublishData(pLockStatPub);
#else  /* RTI_NDDS_4x */
          pubLockStat(pLockStatPub);
#endif  /* RTI_NDDS_4x */
   }
}

void pubLkStat()
{
   semGive(pSemLockStatPub);
}


