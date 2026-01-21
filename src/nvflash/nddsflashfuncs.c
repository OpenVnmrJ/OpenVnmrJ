/* 
 * Varian,Inc. All Rights Reserved.
 * This software contains proprietary and confidential
 * information of Varian, Inc. and its contributors.
 * Use, disclosure and reproduction is prohibited without

 * prior consent.
 */
#include <stdio.h>
#include <stdlib.h>
#ifndef VNMRS_WIN32
#include <unistd.h>
#else  //VNMRS_WIN32
#include <io.h>
#include <fcntl.h>
#endif //VNMRS_WIN32
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#ifndef VNMRS_WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#endif //VNMRS_WIN32
#include <errno.h>
#include <stdarg.h>
#ifndef LINUX 
#ifndef VNMRS_WIN32
#include <thread.h>
#endif
#endif
#ifndef VNMRS_WIN32
#include <pthread.h>
#endif //VNMRS_WIN32

#include "errLogLib.h"
#include "Monitor_Cmd.h"
// #include "NDDS_Obj.h"

#define  DELIMITER_2      '\n'

#ifndef RTI_NDDS_4x
#include "App_HB.h"
#endif  /* RTI_NDDS_4x */

#ifdef RTI_NDDS_4x
#include "Monitor_CmdPlugin.h"
#include "Monitor_CmdSupport.h"
#endif /* RTI_NDDS_4x */


/* hostname (i.e. NIC) attached to console */
/* char ConsoleNicHostname = "wormhole"; */

#define NDDS_DBUG_LEVEL 1
#define MULTICAST_ENABLE 1

extern int initSubscription(NDDS_ID pNDDS_Obj);
extern void attachOnDataAvailableCallback(NDDS_ID pNDDS_Obj,
           DDS_DataReaderListener_DataAvailableCallback callback,
           void *pUserData);
extern int createSubscription(NDDS_ID pNDDS_Obj);
extern int initPublication(NDDS_ID pNDDS_Obj);

char *get_console_hostname(void);
char *getHostIP(char *, char *);
int nddsWait4Subscribers(NDDS_ID, int, int);
int createSubscription(NDDS_ID);
int createPublication(NDDS_ID);
void cntlrNodeHB_PatternSub(void);
RTIBool nddsPublicationDestroy(NDDS_ID);
RTIBool nddsSubscriptionDestroy(NDDS_ID);
int nddsPublishData(NDDS_ID);
int nddsPublicationIssuesWait(NDDS_ID, int, int);
int createBESubscription(NDDS_ID);

int dataSubPipeFd[2];

static int MonCmdPipe[2];

NDDS_ID pMonitorPub, pMonitorSub;

#ifndef RTI_NDDS_4x
static NDDSSubscriber CntlrSubscriber = NULL;
#endif  /* RTI_NDDS_4x */

char ConsoleHostname[80];

NDDS_ID	NDDS_Domain;

#ifndef RTI_NDDS_4x
/*
     Reliable Publication Status call back routine.
     At present we use this to indicate if a subscriber has come or gone
*/
void Monitor_CmdsPubStatusRtn(NDDSPublicationReliableStatus *status,
                                    void *callBackRtnParam)
{
      /* cntlr_t *mine = (cntlr_t*) callBackRtnParam; */
      switch(status->event)
      {
      case NDDS_QUEUE_EMPTY:
        DPRINT1(1,"'%s': Queue empty\n",status->nddsTopic);
        break;
      case NDDS_LOW_WATER_MARK:
        DPRINT1(1,"'%s': Below low water mark - ",status->nddsTopic);
        DPRINT2(1,"Topic: '%s', UnAck Issues: %d\n",status->nddsTopic, status->unacknowledgedIssues);
        break;
      case NDDS_HIGH_WATER_MARK:
        DPRINT1(1,"'%s': Above high water mark - ",status->nddsTopic);
        DPRINT2(1,"Topic: '%s', UnAck Issues: %d\n",status->nddsTopic, status->unacknowledgedIssues);
        break;
      case NDDS_QUEUE_FULL:
        DPRINT1(1,"'%s': Queue full - ",status->nddsTopic);
        DPRINT2(1,"Topic: '%s', UnAck Issues: %d\n",status->nddsTopic, status->unacknowledgedIssues);
        break;
      case NDDS_SUBSCRIPTION_NEW:
        DPRINT1(1,"'%s': A new reliable subscription Appeared.\n",status->nddsTopic);
        numMonCmdSubcriber4Pub++;
        break;
      case NDDS_SUBSCRIPTION_DELETE:
        DPRINT1(1,"'%s': A reliable subscription Disappeared.\n",status->nddsTopic);
        numMonCmdSubcriber4Pub--;
        break;
      default:

                /* NDDS_BEFORERTN_VETOED
                   NDDS_RELIABLE_STATUS
                */
        break;
      }
}      
      
/* ---------------------------------------------------------------------------------- */

RTIBool Monitor_CmdCallback(const NDDSRecvInfo *issue, NDDSInstance *instance,
                             void *callBackRtnParam)
{


   Monitor_Cmd *recvIssue;
   int *pipeFd;
   pipeFd = (int *) callBackRtnParam;

   /* DPRINT2(-1,"Monitor_CmdCallback: pipe[0]: %d, pipe[1]: %d\n",pipeFd[0],pipeFd[1]); */
   /*    possible status values:
     NDDS_FRESH_DATA, NDDS_DESERIALIZATION_ERROR, NDDS_UPDATE_OF_OLD_DATA,
     NDDS_NO_NEW_DATA, NDDS_NEVER_RECEIVED_DATA
   */
   if (issue->status == NDDS_FRESH_DATA)
   {
     recvIssue = (Monitor_Cmd *) instance;
     DPRINT6(+1,"Monitor_CmdCallback() - Cmds: %ld, arg1: %ld, arg2: %ld, arg3: %ld, crc: 0x%lx, msgstr.len: %lu\n",
           recvIssue->cmd, recvIssue->arg1, recvIssue->arg2, recvIssue->arg3, recvIssue->crc32chksum, recvIssue->msgstr.len);

#ifdef VNMRS_WIN32
	 _write(pipeFd[1],recvIssue,sizeof(Monitor_Cmd));
#else // VNMRS_WIN32
     write(pipeFd[1],recvIssue,sizeof(Monitor_Cmd));
#endif

     if (recvIssue->msgstr.len > 0)
     {
        DPRINT1(+1,"Msge: '%s'\n",recvIssue->msgstr.val);
#ifdef VNMRS_WIN32
	_write(pipeFd[1],recvIssue->msgstr.val,recvIssue->msgstr.len);
#else
        write(pipeFd[1],recvIssue->msgstr.val,recvIssue->msgstr.len);
#endif
     }

   }

   return RTI_TRUE;
}

/* ---------------------------------------------------------------------------------- */
 
/* int readNonBlkingMsgePipe(int *pipeFd, Monitor_Cmd* issue, char* msgeBuffer) */
int readNonBlkingMsgePipe(int *pipeFd, Monitor_Cmd* issue, char* msgeBuffer)
{
     int bytes;
#ifdef WANT_NONBLOCKING_CALL
     struct stat pipeStat;
#endif // WANT_NONBLOCKING_CALL

     /* DPRINT2(-1,"readBlkingMsgePipe: pipeFd[0]: %d, pipeFd[1]: %d\n",pipeFd[0],pipeFd[1]); */
     /* the size (st_size) returned by a call to
        fstat(2) with argument  fildes[0] or fildes[1] of a pipe is the number
        of  bytes  available for reading from fildes[0] or fildes[1]
        respectively. */
#ifdef WANT_NONBLOCKING_CALL
     if ( fstat(pipeFd[0], &pipeStat) == -1)
     {
        errLogSysRet(ErrLogOp,debugInfo,"Expproc: readNonBlkingMsgePipe(): fstat error on pipe\n" );
        return(0);
     }
     else
     {
        DPRINT1(1,"readNonBlkingMsgePipe: bytes in Pipe: %ld\n",pipeStat.st_size);
        if (pipeStat.st_size == 0)    /* off_t    st_size */
	   return(0);
     }
#endif
     
     bytes = read(pipeFd[0],issue,sizeof(Monitor_Cmd));
     if (bytes != sizeof(Monitor_Cmd))
        errLogRet(ErrLogOp,debugInfo,"Expproc: readNonBlkingMsgePipe(): bytes read not equal to Monitor_Cmd issue \n" );
       
     if (issue->msgstr.len > 0)
     {
        bytes = read(pipeFd[0], msgeBuffer, issue->msgstr.len);
        if (bytes != issue->msgstr.len)
           errLogRet(ErrLogOp,debugInfo,"Expproc: readNonBlkingMsgePipe(): bytes read not equal to given string length\n" );
     }
     else
     {
        msgeBuffer[0] = 0;   /* null terminate the string just in case someone tries to blindly print it */
     }

     return(sizeof(Monitor_Cmd));
}

/* function assigned as the call back for the NDDS subscription pipe
   as a result of the SIGUSR2 being post by the NDDS call back after writing into the pipe
*/
void MonitorReply(int *cmd,int *arg1, int *arg2,int *arg3, char *msge)
{
   Monitor_Cmd  cmdIssue;
   char msgeStr[CMD_MAX_STR_SIZE];
    int bytes;

   bytes = readNonBlkingMsgePipe(MonCmdPipe, &cmdIssue, msgeStr);
   DPRINT2(1,"processMonitorCmd():  bytes: %d, cmd: %d\n",bytes,cmdIssue.cmd);
   *cmd = cmdIssue.cmd;
   *arg1 = cmdIssue.arg1;
   *arg2 = cmdIssue.arg2;
   *arg3 = cmdIssue.arg3;
    msge[0] = 0;
    if (cmdIssue.msgstr.len > 0)
        strncpy(msge,msgeStr,CMD_MAX_STR_SIZE+1);
			/* cmdIssue.msgstr.val, cmdIssue.msgstr.len); */
}

#else /* RTI_NDDS_4x */

/* this is a hack since 4x structure does not have the .val and .len sub fields for the string member
 * so the structure is defined to be able to use the pipe as it is used in NDDS 3x
 */
typedef struct _monitor_update {
    int cmd;
    int arg1;
    int arg2;
    int arg3;
    unsigned int len;
} MONITOR_MSG;

/* 4x callback */
void Monitor_CmdCallback(void* listener_data, DDS_DataReader* reader)
{
   Monitor_Cmd *recvIssue;
   MONITOR_MSG xfrIssue;
   DDS_ReturnCode_t retcode;
   int *pipeFd;
   struct DDS_SampleInfoSeq info_seq = DDS_SEQUENCE_INITIALIZER;
   struct Monitor_CmdSeq data_seq = DDS_SEQUENCE_INITIALIZER;
   struct DDS_SampleInfo* info = NULL;
   int i,numIssues,len;
   Monitor_CmdDataReader *MonitorCmd_reader = NULL;
   void decode(MONITOR_MSG *issue);

   pipeFd = (int *) listener_data;

   MonitorCmd_reader = Monitor_CmdDataReader_narrow(pMonitorSub->pDReader);
   if ( MonitorCmd_reader == NULL)
   {
        errLogRet(LOGIT,debugInfo, "Monitor_CmdCallback: DataReader narrow error.\n");
        return;
   }


   retcode = Monitor_CmdDataReader_take(MonitorCmd_reader,
                              &data_seq, &info_seq,
                              DDS_LENGTH_UNLIMITED, DDS_ANY_SAMPLE_STATE,
                              DDS_ANY_VIEW_STATE, DDS_ANY_INSTANCE_STATE);
   if (retcode == DDS_RETCODE_NO_DATA) {
            return;
   } else if (retcode != DDS_RETCODE_OK) {
            errLogRet(LOGIT,debugInfo, "Monitor_CmdCallback: next instance error %d\n",retcode);
            return;
   }

   numIssues = Monitor_CmdSeq_get_length(&data_seq);

   for (i=0; i < numIssues; i++)
   {

      info = DDS_SampleInfoSeq_get_reference(&info_seq, i);
      if ( info->valid_data)
      {
          int ret __attribute__((unused));
          recvIssue = Monitor_CmdSeq_get_reference(&data_seq,i);

          DPRINT6(+1,"Monitor_Cmd CallBack:  cmd: %d, arg1: %d, arg2: %d, arg3: %d, crc: 0x%x, msgstr: '%s'\n",
            recvIssue->cmd,recvIssue->arg1, recvIssue->arg2, recvIssue->arg3, recvIssue->crc32chksum, recvIssue->msgstr);

         len = strlen(recvIssue->msgstr);
         xfrIssue.cmd = recvIssue->cmd;
         xfrIssue.arg1 = recvIssue->arg1;
         xfrIssue.arg2 = recvIssue->arg2;
         xfrIssue.arg3 = recvIssue->arg3;
         xfrIssue.len = len;

#ifdef VNMRS_WIN32
	 _write(pipeFd[1],&xfrIssue,sizeof(MONITOR_MSG));
#else // VNMRS_WIN32
         ret = write(pipeFd[1],&xfrIssue,sizeof(MONITOR_MSG));
#endif

         if (len > 0)
         {
#ifdef VNMRS_WIN32
	     _write(pipeFd[1],recvIssue->msgstr,len+1);
#else
             ret = write(pipeFd[1],recvIssue->msgstr,len+1);
#endif
	 }
      }
   }
   retcode = Monitor_CmdDataReader_return_loan( MonitorCmd_reader,
                  &data_seq, &info_seq);

   return;
}

int readNonBlkingMsgePipe(int *pipeFd, MONITOR_MSG* issue, char* msgeBuffer)
{
     int bytes;
#ifdef WANT_NONBLOCKING_CALL
     struct stat pipeStat;
#endif // WANT_NONBLOCKING_CALL

     /* DPRINT2(-1,"readBlkingMsgePipe: pipeFd[0]: %d, pipeFd[1]: %d\n",pipeFd[0],pipeFd[1]); */
     /* the size (st_size) returned by a call to
        fstat(2) with argument  fildes[0] or fildes[1] of a pipe is the number
        of  bytes  available for reading from fildes[0] or fildes[1]
        respectively. */
#ifdef WANT_NONBLOCKING_CALL
     if ( fstat(pipeFd[0], &pipeStat) == -1)
     {
        errLogSysRet(ErrLogOp,debugInfo,"Expproc: readNonBlkingMsgePipe(): fstat error on pipe\n" );
        return(0);
     }
     else
     {
        DPRINT1(1,"readNonBlkingMsgePipe: bytes in Pipe: %ld\n",pipeStat.st_size);
        if (pipeStat.st_size == 0)    /* off_t    st_size */
	   return(0);
     }
#endif
     
     bytes = read(pipeFd[0],issue,sizeof(MONITOR_MSG));
     if (bytes != sizeof(MONITOR_MSG))
        errLogRet(ErrLogOp,debugInfo,"Expproc: readNonBlkingMsgePipe(): bytes read not equal to MONITOR_MSG issue \n" );
       
     if (issue->len > 0)
     {
        bytes = read(pipeFd[0], msgeBuffer, issue->len+1);
        if (bytes != issue->len + 1)
           errLogRet(ErrLogOp,debugInfo,"Expproc: readNonBlkingMsgePipe(): bytes read not equal to given string length\n" );
     }
     else
     {
        msgeBuffer[0] = 0;   /* null terminate the string just in case someone tries to blindly print it */
     }

     return(sizeof(MONITOR_MSG));
}

/* function assigned as the call back for the NDDS subscription pipe
   as a result of the SIGUSR2 being post by the NDDS call back after writing into the pipe
*/
void MonitorReply(int *cmd,int *arg1, int *arg2,int *arg3, char *msge)
{
   MONITOR_MSG  cmdIssue;
   char msgeStr[CMD_MAX_STR_SIZE];
    int bytes;

   bytes = readNonBlkingMsgePipe(MonCmdPipe, &cmdIssue, msgeStr);
   DPRINT2(1,"processMonitorCmd():  bytes: %d, cmd: %d\n",bytes,cmdIssue.cmd);
   *cmd = cmdIssue.cmd;
   *arg1 = cmdIssue.arg1;
   *arg2 = cmdIssue.arg2;
   *arg3 = cmdIssue.arg3;
    msge[0] = 0;
    if (cmdIssue.len > 0)
        strncpy(msge,msgeStr,CMD_MAX_STR_SIZE+1);
			/* cmdIssue.msgstr.val, cmdIssue.msgstr.len); */
}
#endif  /* RTI_NDDS_4x */
/**************************************************************
*
*  initiateNDDS - Initialize a NDDS Domain for  communications
*
***************************************************************/
NDDS_ID initiateNDDS(int debuglevel)
{
    char localIP[80];
    /* NDDS_ID nddsCreate(int domain, int debuglevel, int multicast, char *nicIP) */
    /* NDDS_Domain = nddsCreate(NDDS_DEFAULT_DOMAIN,NDDS_DBUG_LEVEL,MULTICAST_ENABLE,getHostIP(ConsoleHostname,localIP)); */

    strcpy(ConsoleHostname, (char*) get_console_hostname());

    if (getHostIP(ConsoleHostname, localIP) != NULL)
       NDDS_Domain = nddsCreate((int)0,(int)debuglevel,(int)MULTICAST_ENABLE,(char *)localIP);
	else
	    NDDS_Domain = NULL;
    if ( NDDS_Domain == NULL) {
       if (debuglevel)
          errLogRet(ErrLogOp,debugInfo,"Expproc: initiateNDDS(): NDDS domain failed to initialize!!!\n" );
       else
          errLogQuit(ErrLogOp,debugInfo,"Expproc: initiateNDDS(): NDDS domain failed to initialize!!!\n" );
    }
    return (NDDS_Domain);
}
 
int wait4Master2Connect(int timeout)
{
   int numberSubscribers;
   numberSubscribers = 1;
   if (timeout == 0)
        timeout = 10; /* 10 seconds */
   return( nddsWait4Subscribers(pMonitorPub, timeout, numberSubscribers) );
}

/* int nddsPublicationIssuesWait(NDDS_ID pNDDS_Obj, int timeOut, int Qlevel) */
 
NDDS_ID  createMonitorCmdsSub(char *subName)
{
    int setFdAsync( int fd, void *clientData, void (*callback)() );
    NDDS_ID pSubObj;
 
    /* Build Data type Object for both publication and subscription to Expproc */
    /* ------- malloc space for data type object --------- */
    if (NDDS_Domain == NULL)
        return(NULL);
	
    if ( (pSubObj = (NDDS_ID) malloc( sizeof(NDDS_OBJ)) ) == NULL )
    {
        return(NULL);
    }
 
    /* create pipe for messages from NDDS signal handler Console to Expproc */
#ifdef VNMRS_WIN32
    if ( _pipe(MonCmdPipe,1024,_O_BINARY) != 0)
#else //VNMRS_WIN32
    if ( pipe(MonCmdPipe) != 0)
#endif
    {
        errLogSysQuit(ErrLogOp,debugInfo,"Expproc: createMonitorCmdsSub(): could not create Message Pipe\n" );
    }

    DPRINT2(1,"---> pipe fd[0]: %d, fd[1]: %d\n",MonCmdPipe[0],MonCmdPipe[1]);
    /* zero out structure */
    memset(pSubObj,0,sizeof(NDDS_OBJ));
    memcpy(pSubObj,NDDS_Domain,sizeof(NDDS_OBJ));
 
    strcpy(pSubObj->topicName,subName);
 
    /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */
    getMonitor_CmdInfo(pSubObj);
 
    /* NDDS issue callback routine */
#ifndef RTI_NDDS_4x
    pSubObj->callBkRtn = Monitor_CmdCallback;
    pSubObj->callBkRtnParam = MonCmdPipe; /* pCntlrThr->SubPipeFd;    write end of pipe */
#endif  /* RTI_NDDS_4x */
    pSubObj->MulticastSubIP[0] = 0;   /* use UNICAST */
#ifdef RTI_NDDS_4x
    initSubscription(pSubObj);
    attachOnDataAvailableCallback(pSubObj,Monitor_CmdCallback, MonCmdPipe);
#endif  /* RTI_NDDS_4x */
    createSubscription(pSubObj);
 
    return(pSubObj);
 
}

NDDS_ID  createMonitorCmdsPub(char *pubName)
{
//      int result;
     NDDS_ID pPubObj;

    /* Build Data type Object for both publication and subscription to Expproc */
    /* ------- malloc space for data type object --------- */
    if (NDDS_Domain == NULL)
        return(NULL);
    if ( (pPubObj = (NDDS_ID) malloc( sizeof(NDDS_OBJ)) ) == NULL )
      {
        return(NULL);
      }
 
    /* zero out structure */    
    memset(pPubObj,0,sizeof(NDDS_OBJ));
    memcpy(pPubObj,NDDS_Domain,sizeof(NDDS_OBJ));

    strcpy(pPubObj->topicName,pubName);           

    /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */
    getMonitor_CmdInfo(pPubObj);
    pPubObj->queueSize = 10;
    pPubObj->highWaterMark = 2;
    pPubObj->lowWaterMark = 0;
    pPubObj->AckRequestsPerSendQueue = 10;
    pPubObj->pubThreadId = 1;   /* for mulit threaded apps */
#ifndef RTI_NDDS_4x
    pPubObj->pubRelStatRtn = Monitor_CmdsPubStatusRtn;
#endif  /* RTI_NDDS_4x */
    /* pPubObj->pubRelStatParam =  (void*) pCntlrThr; */
    pPubObj->pubRelStatParam =  (void*) NULL;
#ifdef RTI_NDDS_4x
    initPublication(pPubObj);
#endif  /* RTI_NDDS_4x */
    createPublication(pPubObj);
    return(pPubObj);
}
 
void startHB_Monitor()
{
   /* HB_PATTERN_ARG patArgs;
    * patArgs.callbackRoutine = 
    * patArgs.callbackParams = 
    */
   /* startHBThread("Expproc"); */
   /* cntlrAppHB_PatternSub(NULL); */
   /* cntrlRequiredNodesSub(); */
   /* cntlrNodeHB_PatternSub(&patArgs); */
}

void initiatePubSub()
{
    pMonitorPub = createMonitorCmdsPub(HOST_PUB_CMDS_TOPIC_FORMAT_STR);
    pMonitorSub =   createMonitorCmdsSub(HOST_SUB_CMDS_TOPIC_FORMAT_STR);
#ifndef RTI_NDDS_4x
    cntlrNodeHB_PatternSub();
#endif  /* RTI_NDDS_4x */
}

void DestroyPubSub()
{
    DPRINT(1,">>>>>>>>>> Destroy Monitor Pub/Sub\n");
    if (pMonitorPub != NULL)
       nddsPublicationDestroy(pMonitorPub);
    if (pMonitorSub != NULL)
       nddsSubscriptionDestroy(pMonitorSub);
}

void DestroyDomain()
{
    if (NDDS_Domain == NULL)
        return;
#ifndef RTI_NDDS_4x
   if (NDDS_Domain->domain != NULL)
      NddsDestroy(NDDS_Domain->domain); /* NddsDomainHandleGet(0) */
#else /* RTI_NDDS_4x */
      NDDS_Shutdown(NDDS_Domain); /* NddsDomainHandleGet(0) */
#endif  /* RTI_NDDS_4x */
}

/*  
   send a monitor cmd issue to the Console (master controller)
*/

#ifndef RTI_NDDS_4x
int send2Monitor(int cmd, int arg1, int arg2, int arg3, char *msgstr, size_t len)
{
    Monitor_Cmd *issue;
     RTINtpTime                maxWait    = {10,0};

     if (pMonitorPub == NULL)
	return (-1);
	
     issue = (Monitor_Cmd *) pMonitorPub->instance;
     issue->cmd = cmd;
     issue->arg1 = arg1;
     issue->arg2 = arg2;
     issue->arg3 = arg3;
     issue->msgstr.len = len;
     memcpy(issue->msgstr.val,msgstr,len);
     issue->crc32chksum = 0;
     nddsPublishData(pMonitorPub);
     /* wait for message to be acknowledged */
     nddsPublicationIssuesWait( pMonitorPub, 60, 0 );
     return(0);
}
#else /* RTI_NDDS_4x */

/* NDDS_4x Version */
int send2Monitor(int cmd, int arg1, int arg2, int arg3, char *msgstr, size_t len)
{
   DDS_ReturnCode_t result;
   DDS_InstanceHandle_t instance_handle = DDS_HANDLE_NIL;
   Monitor_CmdDataWriter *MonitorCmd_writer = NULL;
   Monitor_Cmd *issue;

   MonitorCmd_writer = Monitor_CmdDataWriter_narrow(pMonitorPub->pDWriter);
   if (MonitorCmd_writer == NULL) {
        errLogRet(LOGIT,debugInfo, "send2Expproc: DataReader narrow error.\n");
        return -1;
    }
   issue = (Monitor_Cmd *) pMonitorPub->instance;
   issue->cmd = cmd;
   issue->arg1 = arg1;
   issue->arg2 = arg2;
   issue->arg3 = arg3;
   strncpy(issue->msgstr,msgstr,len);
   issue->crc32chksum = 0;

   result = Monitor_CmdDataWriter_write(MonitorCmd_writer,
                issue,&instance_handle);
   if (result != DDS_RETCODE_OK) {
            errLogRet(LOGIT,debugInfo, "send2Expproc: write error %d\n",result);
            return -1;
   }
   return 0;
}



#endif  /* RTI_NDDS_4x */


/* +++++++++++++++++++++ Heart Beat ++++++++++++++++++++++++++++ */

#ifndef RTI_NDDS_4x
/*---------------------------------------------------------------------------------- */
 
RTIBool App_NodeCallback(const NDDSRecvInfo *issue, NDDSInstance *instance,
                             void *callBackRtnParam)
{

   App_HB *recvIssue;
//    int cntlrType;

   /*    possible status values:
     NDDS_FRESH_DATA, NDDS_DESERIALIZATION_ERROR, NDDS_UPDATE_OF_OLD_DATA,
     NDDS_NO_NEW_DATA, NDDS_NEVER_RECEIVED_DATA
   */ 
   if (issue->status == NDDS_FRESH_DATA)
   {
     recvIssue = (App_HB *) instance;
     DPRINT5(+3, "App_NodeCallback: '%s': received AppStr: '%s', HB cnt: %lu, ThreadId: %d, AppID: %d\n",
        issue->nddsTopic,recvIssue->AppIdStr, recvIssue->HBcnt, recvIssue->ThreadId,recvIssue->AppId);

     DPRINT1(+1,"App_NodeCallback: Issue: '%s', Is Here.\n", issue->nddsTopic);
   }
   else if (issue->status == NDDS_NO_NEW_DATA)
   {
      DPRINT1(+1,"App_NodeCallback: Issue: '%s', Missed Deadline, Node must be gone.\n", issue->nddsTopic);
   }
   return RTI_TRUE;
}

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
    strcpy(pSubObj->MulticastSubIP,APP_HB_MULTICAST_IP);
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
//      int threadIndex;
     NDDSSubscription pSub;
     NDDS_ID pHB_SubId;

     DPRINT3(+1,"App_HBPatternSubCreate(): Topic: '%s', Type: '%s', arg: 0x%lx\n",
                nddsTopic, nddsType, callBackRtnParam);

     strncpy(cntrlName,nddsTopic,127);
     chrptr = strchr(cntrlName,'/');
     *chrptr = 0;
     DPRINT2(-1,"App_HBPatternSubCreate(): Cntlr: '%s', subscription: '%s'\n",
                cntrlName,nddsTopic);
 
     pHB_SubId  = createAppHB_BESubscription((char*) nddsTopic,(void*)App_NodeCallback, NULL);
 
     pSub = pHB_SubId->subscription;
 
     return pSub;
}
 


void cntlrNodeHB_PatternSub()
{
    if (CntlrSubscriber == NULL)
        CntlrSubscriber = NddsSubscriberCreate(0);
 
    NddsSubscriberPatternAdd(CntlrSubscriber,
           nodeHB_PATTERN_FORMAT_STR, App_HBNDDSType , App_HBPatternSubCreate, (void *)NULL);
}
#endif  /* RTI_NDDS_4x */

