/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifdef LINUX
#include <sys/ioctl.h>
#else
#include <sys/stat.h>
#endif
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdarg.h>
#ifndef LINUX
#include <thread.h>
#endif
#include <pthread.h>

#include "errLogLib.h"
#include "Monitor_Cmd.h"

#ifdef RTI_NDDS_4x
  #include "Monitor_CmdPlugin.h"
  #include "Monitor_CmdSupport.h"
#endif  /* RTI_NDDS_4x */

#define  DELIMITER_2      '\n'

#ifndef SENDTEST
#include "Console_Stat.h"
// #include "App_HB.h"

#ifdef RTI_NDDS_4x
  #include "Console_StatPlugin.h"
  #include "Console_StatSupport.h"
//  #include "App_HBPlugin.h"
//  #include "App_HBSupport.h"
#endif  /* RTI_NDDS_4x */

#ifndef RTI_NDDS_4x
#include "HBListener.h"
#endif /* RTI_NDDS_4x */
#include "NDDS_Obj.h"
#include "NDDS_PubFuncs.h"
#include "NDDS_SubFuncs.h"

#include "sockets.h"
#include "msgQLib.h"
#include "nddsbufmngr.h"
extern char *getHostIP(char *hostname, char *localIP);
extern int nddsSubscriptionDestroy(NDDS_ID pNDDS_Obj);
extern int areNodesActive();
extern void processChanMsge(int cmd, int arg1, int arg2, int arg3, char *msgstr);

#ifndef DEBUG_HEARTBEAT
#define DEBUG_HEARTBEAT	(9)
#endif

#define TRUE 1
#define FALSE 0
#define FOR_EVER 1
#define HEARTBEAT_TIMEOUT_INTERVAL	(2.8)

extern pthread_t main_threadId;

/* hostname (i.e. NIC) attached to console */
extern char ConsoleNicHostname[];

extern MSG_Q_ID pRecvMsgQ;

#define NDDS_DBUG_LEVEL 1
#define MULTICAST_ENABLE 1
#define MULTICAST_DISABLE 0

int dataSubPipeFd[2];

NDDS_ID pMonitorPub, pMonitorSub;

int MonCmdPipe[2];

#endif

NDDS_ID pMonitorPub, pMonitorSub;
#ifndef RTI_NDDS_4x
static int numMonCmdSubcriber4Pub = 0;
#endif

static NDDSBUFMNGR_ID  pMonitor_CmdBufMngr = NULL;

#ifdef SENDTEST
extern NDDS_ID  NDDS_Domain;
#else
NDDS_ID NDDS_Domain;
#endif

#ifndef SENDTEST

#define WALLNUM 2
static char *wallpaths[] = { "/usr/sbin/wall", "/usr/bin/wall" };
static char wallpath[25] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
static int shpid;

static struct timeval  hbtime;		/* time that the last heartbeat */
					/* message sent to the console  */
#endif

#ifndef SENDTEST

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
      


/*---------------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------------- */

RTIBool Monitor_CmdCallback(const NDDSRecvInfo *issue, NDDSInstance *instance,
                             void *callBackRtnParam)
{
   Monitor_Cmd *recvIssue;
   NDDSBUFMNGR_ID pBufMngr;
   char *bufferAddr;
   char *bufptr;
   pBufMngr = (NDDSBUFMNGR_ID) callBackRtnParam;

   /* DPRINT2(-1,"Monitor_CmdCallback: pipe[0]: %d, pipe[1]: %d\n",pipeFd[0],pipeFd[1]); */
   /*    possible status values:
     NDDS_FRESH_DATA, NDDS_DESERIALIZATION_ERROR, NDDS_UPDATE_OF_OLD_DATA,
     NDDS_NO_NEW_DATA, NDDS_NEVER_RECEIVED_DATA
   */
   if (issue->status == NDDS_FRESH_DATA)
   {
     recvIssue = (Monitor_Cmd *) instance;
     DPRINT6(1,"Monitor_CmdCallback() - Cmds: %ld, arg1: %ld, arg2: %ld, arg3: %ld, crc: 0x%lx, msgstr.len: %lu\n",
           recvIssue->cmd, recvIssue->arg1, recvIssue->arg2, recvIssue->arg3, recvIssue->crc32chksum, recvIssue->msgstr.len);

     bufferAddr = msgeBufGet(pBufMngr);
     if (bufferAddr == NULL)
	 return RTI_FALSE;

     memcpy(bufferAddr,recvIssue,sizeof(Monitor_Cmd));

     bufptr = bufferAddr + sizeof(Monitor_Cmd);
     /* DPRINT3(3,"bufferAddr: 0x%lx, offset: 0x%lx, msgptr: 0x%lx\n", bufferAddr, sizeof(Monitor_Cmd), bufptr); */
     if (recvIssue->msgstr.len > 0)
     {
        DPRINT1(1,"Msge: '%s'\n",recvIssue->msgstr.val);
        memcpy(bufptr,recvIssue->msgstr.val,recvIssue->msgstr.len);
     }
     else
     {
        bufptr = (char) 0;
     }
     msgePost(pBufMngr,bufferAddr);

     pthread_kill(main_threadId,SIGUSR2); /* signal console msg arrival to main thread */
   }

   return RTI_TRUE;
}

/* ---------------------------------------------------------------------------------- */

int getNextMonitorCmd(NDDSBUFMNGR_ID pBufMngr, Monitor_Cmd* issue, char* msgeBuffer)
{
     int bytes;
     int msgeSize;
     char *pBufPtr;
     char *pMsgPtr;

     /* If msgeGet() would block just return */
      if (msgeGetWillPend(pBufMngr) == 1)
      {  
         /* normal to keep call this function until it return 0 */
	 return 0;
      }

      /* should not pend on this blocking call, due to the test above */
      pBufPtr = (char  *) msgeGet(pBufMngr);
      memcpy(issue, pBufPtr, sizeof(Monitor_Cmd));
     if (issue->msgstr.len > 0)
     {
        pMsgPtr = pBufPtr + sizeof(Monitor_Cmd);
        memcpy(msgeBuffer, pMsgPtr, issue->msgstr.len);
     }
     else
     {
        msgeBuffer[0] = 0;   /* null terminate the string just in case someone tries to blindly print it */
     }

     /* return this buffer to the free list for NDDS callback */
     msgeBufReturn(pBufMngr,(char*) pBufPtr);

     return(sizeof(Monitor_Cmd));
}

/* function assigned as the call back for the NDDS subscription pipe
   as a result of the SIGUSR2 being post by the NDDS call back after writing into the pipe
*/
void processMonitorCmd(int *pipeFd)
{
   Monitor_Cmd  cmdIssue;
   char msgeStr[CMD_MAX_STR_SIZE];
    int bytes;

   while ((bytes = getNextMonitorCmd(pMonitor_CmdBufMngr, &cmdIssue, msgeStr)) != 0)
   {
      DPRINT2(1,"processMonitorCmd():  bytes: %d, cmd: %d\n",bytes,cmdIssue.cmd);
      processChanMsge(cmdIssue.cmd, cmdIssue.arg1, cmdIssue.arg2, cmdIssue.arg3, 
			cmdIssue.msgstr.val);
   }

}

#else /* RTI_NDDS_4x */

/* NDDS 4x callback */
void Monitor_CmdCallback(void* listener_data, DDS_DataReader* reader)
{
   Monitor_Cmd *recvIssue;
   NDDSBUFMNGR_ID pBufMngr;
   char *bufferAddr;
   char *bufptr;

   DDS_ReturnCode_t retcode;
   struct DDS_SampleInfoSeq info_seq = DDS_SEQUENCE_INITIALIZER;
   struct Monitor_CmdSeq data_seq = DDS_SEQUENCE_INITIALIZER;
   struct DDS_SampleInfo* info = NULL;
   int i,numIssues,len;
   Monitor_CmdDataReader *MonitorCmd_reader = NULL;

   pBufMngr = (NDDSBUFMNGR_ID) listener_data;

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
          recvIssue = Monitor_CmdSeq_get_reference(&data_seq,i);

          DPRINT6(1,"Monitor_CmdCallback() - Cmds: %d, arg1: %d, arg2: %d, arg3: %d, crc: 0x%x, msgstr: '%s'\n",
            recvIssue->cmd, recvIssue->arg1, recvIssue->arg2, recvIssue->arg3, recvIssue->crc32chksum, recvIssue->msgstr);

          bufferAddr = msgeBufGet(pBufMngr);
          if (bufferAddr == NULL)
	      return;

          memcpy(bufferAddr,recvIssue,sizeof(Monitor_Cmd));

          len = strlen(recvIssue->msgstr);
          bufptr = bufferAddr + sizeof(Monitor_Cmd);
          /* DPRINT3(3,"bufferAddr: 0x%lx, offset: 0x%lx, msgptr: 0x%lx\n", bufferAddr, sizeof(Monitor_Cmd), bufptr); */
          if (len > 0)
          {
             DPRINT1(1,"Msge: '%s'\n",recvIssue->msgstr);
             strncpy(bufptr,recvIssue->msgstr,len+1);
          }
          else
          {
             bufptr = (char) 0;
          }
          msgePost(pBufMngr,bufferAddr);

          pthread_kill(main_threadId,SIGUSR2); /* signal console msg arrival to main thread */
      }
   }
   retcode = Monitor_CmdDataReader_return_loan( MonitorCmd_reader,
                  &data_seq, &info_seq);

   return;
}

 
/* ---------------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------------- */
/* NDDS 4x version since Monitor_Cmd issue structure changes with 4x */

int getNextMonitorCmd(NDDSBUFMNGR_ID pBufMngr, Monitor_Cmd* issue, char* msgeBuffer)
{
     char *pBufPtr;
     char *pMsgPtr;
     int len;

     /* If msgeGet() would block just return */
      if (msgeGetWillPend(pBufMngr) == 1)
      {  
         /* normal to keep call this function until it return 0 */
	 return 0;
      }

      /* should not pend on this blocking call, due to the test above */
      pBufPtr = (char  *) msgeGet(pBufMngr);
      memcpy(issue, pBufPtr, sizeof(Monitor_Cmd));
      pMsgPtr = pBufPtr + sizeof(Monitor_Cmd);
      len = strlen(pMsgPtr);
     if (len > 0)
     {
        strncpy(msgeBuffer, pMsgPtr, len+1);
     }
     else
     {
        msgeBuffer[0] = 0;   /* null terminate the string just in case someone tries to blindly print it */
     }

     /* return this buffer to the free list for NDDS callback */
     msgeBufReturn(pBufMngr,(char*) pBufPtr);

     return(sizeof(Monitor_Cmd));
}

/* NDDS 4x version since Monitor_Cmd issue structure changes with 4x */
/* function assigned as the call back for the NDDS subscription pipe
   as a result of the SIGUSR2 being post by the NDDS call back after writing into the pipe
*/
void processMonitorCmd(int *pipeFd)
{
   Monitor_Cmd  cmdIssue;
   char msgeStr[CMD_MAX_STR_SIZE];
    int bytes;

   while ((bytes = getNextMonitorCmd(pMonitor_CmdBufMngr, &cmdIssue, msgeStr)) != 0)
   {
      DPRINT2(1,"processMonitorCmd():  bytes: %d, cmd: %d\n",bytes,cmdIssue.cmd);
      processChanMsge(cmdIssue.cmd, cmdIssue.arg1, cmdIssue.arg2, cmdIssue.arg3, 
			msgeStr);
   }

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
#ifndef NO_MULTICAST
    NDDS_Domain = nddsCreate(0,debuglevel,MULTICAST_ENABLE,(char*)getHostIP(ConsoleNicHostname,localIP));
#else
    NDDS_Domain = nddsCreate(0,debuglevel,MULTICAST_DISABLE,(char*)getHostIP(ConsoleNicHostname,localIP));
#endif
    if ( NDDS_Domain == NULL)
       errLogQuit(ErrLogOp,debugInfo,"Expproc: initiateNDDS(): NDDS domain failed to initialize!!!\n" );
}
 
 
NDDS_ID  createMonitorCmdsSub(char *subName)
{
    NDDS_ID pSubObj;
 
    /* Build Data type Object for both publication and subscription to Expproc */
    /* ------- malloc space for data type object --------- */
    if ( (pSubObj = (NDDS_ID) malloc( sizeof(NDDS_OBJ)) ) == NULL )
    {
        return(NULL);
    }
 
    /* create pipe for messages from NDDS signal handler Console to Expproc */
    if ( pipe(MonCmdPipe) != 0)
    {
        errLogSysQuit(ErrLogOp,debugInfo,"Expproc: createMonitorCmdsSub(): could not create Message Pipe\n" );
    }

    pMonitor_CmdBufMngr = nddsBufMngrCreate(50,(sizeof(Monitor_Cmd) + CMD_MAX_STR_SIZE + 4));

    /* set the callback routine to processMonitorCmd() and mark as direct
       this avoid the ssocket related setting which are not whated for the pipe
       set for the reading fd of the pipe
    */

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
    pSubObj->callBkRtnParam = pMonitor_CmdBufMngr; // MonCmdPipe; /* pCntlrThr->SubPipeFd;    write end of pipe */
#endif /* RTI_NDDS_4x */

    pSubObj->MulticastSubIP[0] = 0;   /* use UNICAST */

#ifdef RTI_NDDS_4x
    initSubscription(pSubObj);
    attachOnDataAvailableCallback(pSubObj, Monitor_CmdCallback, pMonitor_CmdBufMngr);
#endif /* RTI_NDDS_4x */

    createSubscription(pSubObj);
 
    return(pSubObj);
 
}

#endif  /* SENDTEST */

NDDS_ID  createMonitorCmdsPub(char *pubName)
{
     NDDS_ID pPubObj;

    /* Build Data type Object for both publication and subscription to Expproc */
    /* ------- malloc space for data type object --------- */
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
    pPubObj->highWaterMark = 1;
    pPubObj->lowWaterMark = 0;
    pPubObj->AckRequestsPerSendQueue = 10;
    pPubObj->pubThreadId = 1;   /* for mulit threaded apps */
#ifndef RTI_NDDS_4x
    pPubObj->pubRelStatRtn = Monitor_CmdsPubStatusRtn;
    /* pPubObj->pubRelStatParam =  (void*) pCntlrThr; */
    pPubObj->pubRelStatParam =  (void*) NULL;
#endif /* RTI_NDDS_4x */
#ifdef RTI_NDDS_4x
    initPublication(pPubObj);
#endif /* RTI_NDDS_4x */
    createPublication(pPubObj);
    return(pPubObj);
}
 
#ifndef RTI_NDDS_4x
int startHB_Monitor()
{
   /* HB_PATTERN_ARG patArgs;
    * patArgs.callbackRoutine = 
    * patArgs.callbackParams = 
    */
   startHBThread("Expproc");
   HeartBeatPatternSub();
   /* cntlrAppHB_PatternSub(NULL); */
   /* cntrlRequiredNodesSub(); */
   /* cntlrNodeHB_PatternSub(&patArgs); */
   return 0;
}
#endif  /* RTI_NDDS_4x */

#ifdef SENDTEST

void initiatePub()
{
    pMonitorPub = createMonitorCmdsPub(HOST_PUB_CMDS_TOPIC_FORMAT_STR);
}

void DestroyPubSub()
{
    if (pMonitorPub != NULL)
       nddsPublicationDestroy(pMonitorPub);
}
#else 

void initiatePubSub()
{
    pMonitorPub = createMonitorCmdsPub(HOST_PUB_CMDS_TOPIC_FORMAT_STR);
    pMonitorSub =   createMonitorCmdsSub(HOST_SUB_CMDS_TOPIC_FORMAT_STR);
}

void DestroyPubSub()
{
    DPRINT(1,">>>>>>>>>> Destroy Monitor Pub/Sub\n");
    if (pMonitorPub != NULL)
       nddsPublicationDestroy(pMonitorPub);
    if (pMonitorSub != NULL)
       nddsSubscriptionDestroy(pMonitorSub);
}
#endif

void DestroyDomain()
{
#ifndef RTI_NDDS_4x
   if (NDDS_Domain->domain != NULL)
   {
      DPRINT1(+1,"Expproc: Destroy Domain: 0x%lx\n",NDDS_Domain->domain);
      NddsDestroy(NDDS_Domain->domain); /* NddsDomainHandleGet(0) */
   }
#else /* RTI_NDDS_4x */
   NDDS_Shutdown(NDDS_Domain);
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
     issue = (Monitor_Cmd *) pMonitorPub->instance;
     issue->cmd = cmd;
     issue->arg1 = arg1;
     issue->arg2 = arg2;
     issue->arg3 = arg3;
     issue->msgstr.len = len;
     memcpy(issue->msgstr.val,msgstr,len);
     issue->crc32chksum = 0;
     nddsPublishData(pMonitorPub);
}
#else /* RTI_NDDS_4x */

int send2Monitor(int cmd, int arg1, int arg2, int arg3, char *msgstr, size_t len)
{
   DDS_ReturnCode_t result;
   DDS_InstanceHandle_t instance_handle = DDS_HANDLE_NIL;
   Monitor_CmdDataWriter *MonitorCmd_writer = NULL;
   Monitor_Cmd *issue;

   MonitorCmd_writer = Monitor_CmdDataWriter_narrow(pMonitorPub->pDWriter);
   if (MonitorCmd_writer == NULL) {
        errLogRet(LOGIT,debugInfo, "send2Monitor: DataReader narrow error.\n");
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
            errLogRet(LOGIT,debugInfo, "send2Monitor: write error %d\n",result);
            return -1;
   }
   return 0;
}

#endif /* RTI_NDDS_4x */

#ifndef SENDTEST

static double
diff_hbtime( struct timeval *newtime )
{
	int	diffsec, diffusec;
	double	dval;

	diffsec = newtime->tv_sec - hbtime.tv_sec;
	diffusec = newtime->tv_usec - hbtime.tv_usec;
	if (diffusec < 0) {
		diffsec--;
		diffusec += 1000000;
	}
	dval = (double) (diffsec) + ((double) (diffusec)) / 1.0e6;

	return( dval );
}

/*  print_hbtime_diff should only be called if DebugLevel is high
    enough that the debug output is to be printed.  Therefore we
    can use -1 as the "debug level" in the DPRINTn macro	*/

int print_hbtime_diff( char *msg, struct timeval *newtime )
{
	DPRINT2( -1, "%s: heartbeat time difference: %g\n",
		      msg, diff_hbtime( newtime )  );
        return 0;
}


/**************************************************************
*
*  shutdownComm - Close the Message Q , DataBase & Channel 
*
* This routine closes the message Q , DataBase and Channel
*  
* RETURNS:
* MSG_Q_ID , or NULL on error. 
*
*       Author Greg Brissey 8/4/94
*/
void shutdownComm(void)
{
#ifndef RTI_NDDS_4x
     HBExit();
#endif  /* RTI_NDDS_4x */
     deleteMsgQ(pRecvMsgQ);
     /* DestroyPubSub(); */
     DestroyDomain();
     usleep(500000);  /* 500 millisec sleep */
}



int consoleConn()
{
   /*
    if (numMonCmdSubcriber4Pub > 0)
       return(1);
    else
       return(0);
   */
   /* return(1); */
   /* return( areMasterAndDdrActive() ); */
   return( areNodesActive() );
}

#ifdef XXXX
/*-------------------------------------------------------------------------
|
|   Setup the timer interval alarm
|
+--------------------------------------------------------------------------*/
int setRtimer(double timsec, double interval)
{
    long sec,frac;
    struct itimerval timeval,oldtime;

    sec = (long) timsec;
    frac = (long) ( (timsec - (double)sec) * 1.0e6 ); /* usecs */
    DPRINT2(3,"setRtimer(): sec = %ld, frac = %ld\n",sec,frac);
    timeval.it_value.tv_sec = sec;
    timeval.it_value.tv_usec = frac;
    sec = (long) interval;
    frac = (long) ( (interval - (double)sec) * 1.0e6 ); /* usecs */
    DPRINT2(3,"setRtimer(): sec = %ld, frac = %ld\n",sec,frac);
    timeval.it_interval.tv_sec = sec;
    timeval.it_interval.tv_usec = frac;
    if (setitimer(ITIMER_REAL,&timeval,&oldtime) == -1)
    {
         perror("setitimer error");
         return(-1);
    }
    return(0);
}
#endif

/**************************************************************
*
*   deliverMessage - send message to a named message queue or socket
*
*   Use this program to send a message to a message queue
*   using the name of the message queue.  It opens the
*   message queue, sends the message and then closes the
*   message queue.  The return value is the result from
*   sending the message, unless it cannot access the message
*   queue, in which case it returns -1.  Its two arguments
*   are the name of the message queue and the message, both
*   the address of character strings.
*   
*   
*   March 1998:  deliverMessage was extended to send the message
*   to either a message queue or a socket.  It selects the system
*   interface based on the format of the interface argument.
*
*   A message queue will have an interface either like "Expproc"
*   (single word) or "Vnmr 1234" (word, space, number - note
*   this latter form is now obsolete).
*
*   A socket will have an interface like "inova400 34567 1234"
*   (word, space, number, space, number - the word is the hostname,
*   the first number is the port address of the socket to connect
*   to, the second is the process ID).
*
*   Using a format specifier of "%s %d %d" (string of non-space
*   characters, space, number, space, number) it scans this
*   interface argument.  If it can convert three fields, then
*   the interface is a socket; otherwise it is a message queue.
*/

int
deliverMessage( char *interface, char *message )
{
	char		tmpstring[ 256 ];
	int		ival1, ival2;
	int		ival, mlen;
	MSG_Q_ID	tmpMsgQ;

	if (interface == NULL)
	  return( -1 );
	if (message == NULL)
	  return( -1 );
	mlen = strlen( message );
	if (mlen < 1)
	  return( -1 );

    	ival = sscanf( interface, "%s %d %d\n", &tmpstring[ 0 ], &ival1, &ival2 );

    /*
     *       diagPrint(debugInfo,"Expproc deliverMessage  ----> host: '%s', port: %d,(%d,%d)  pid: %d\n", 
     *           tmpstring,ival1,0xffff & ntohs(ival1), 0xffff & htons(ival1), ival2);
     */
	if (ival >= 3) {
		int	 replyPortAddr;
		char	*hostname;
		Socket	*pReplySocket;

		replyPortAddr = ival1;
		hostname = &tmpstring[ 0 ];

		pReplySocket = createSocket( SOCK_STREAM );
		if (pReplySocket == NULL)
		  return( -1 );
		ival = openSocket( pReplySocket );
		if (ival != 0)
		  return( -1 );
                /* replyPortAddr is already in network order, 
                 * so switch back so sockets.c can switch back
                 */
		ival = connectSocket( pReplySocket, hostname, 0xFFFF & htons(replyPortAddr) );
		if (ival != 0)
		  return( -2 );

		writeSocket( pReplySocket, message, mlen );
		closeSocket( pReplySocket );
		free( pReplySocket );

		return( 0 );
	}

	tmpMsgQ = openMsgQ( interface );
	if (tmpMsgQ == NULL)
	  return( -1 );

        ival = sendMsgQ( tmpMsgQ, message, mlen, MSGQ_NORMAL, NO_WAIT );

        closeMsgQ( tmpMsgQ );
	return( ival );
}

/*  See deliverMessage for an explanation of how this works  */

int
verifyInterface( char *interface )
{
	char		tmpstring[ 256 ];
	int		ival1, ival2;
	int		ival;
	MSG_Q_ID	tmpMsgQ;

	if (interface == NULL)
	  return( 0 );

    	ival = sscanf( interface, "%s %d %d\n", &tmpstring[ 0 ], &ival1, &ival2 );

	if (ival >= 3) {
		int	 peerProcId;

		peerProcId = ival2;
		ival = kill( peerProcId, 0 );
		if (ival != 0) {
			if (errno != ESRCH)
			  DPRINT1( -1, "Error accessing PID %d\n", peerProcId );
			return( 0 );
		}
		else
		  return( 1 );
	}
	else {
		tmpMsgQ = openMsgQ( interface );
		if (tmpMsgQ == NULL)
		  return( 0 );

	        closeMsgQ( tmpMsgQ );
		return( 1 );
	}
}


/*-----------------------------------------------------------------------
|
| findwall - search several pathes to find the wall command
|
+------------------------------------------------------------------------*/
static int findwall(path)
char *path;
{
    int i;
 
    for (i=0;i<WALLNUM;i++)
    {
        if (access(wallpaths[i],X_OK) != -1)
        {
            strcpy(path,wallpaths[i]);
            return(0);
        }   
    }    
    strcpy(path,"");
    return(-1);
}
 

/***********************************************************************
* wallMsge
*   Execute the UNIX wall command with the given message
*
* WARNING: 
*   Processes using this function must catch the SIGCHLD of this forked shell
*/
int wallMsge(char *fmt, ...)
{
   va_list  vargs;
   char msge[512];
   char cmd[1024];

   va_start(vargs, fmt);
   vsprintf(msge, fmt, vargs);
   va_end(vargs);

   if ((int)strlen(wallpath) < 2)
   {
      findwall(wallpath);
      DPRINT1(3,"wallMsge: find path = '%s'\n",wallpath);
      if ((int)strlen(wallpath) < 2)
         return(0);
   }
#ifdef LINUX
   sprintf(cmd,"echo '%s' | %s",msge,wallpath); /* /usr/bin/wall "echo 'msge'" */
#else
   sprintf(cmd,"echo '%s' | %s -a",msge,wallpath); /* /usr/sbin/wall -a "echo 'msge'" */
#endif

   DPRINT1(1,"wallMsge: cmd = '%s'\n",cmd);
   shpid = vfork();
   if (shpid == 0)   /* if we are the child then exec the sh */
   {
       execl("/bin/sh","sh","-c",cmd,NULL);
       exit(1);
   }
   return 0;
}
#endif
