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
4-20-04,gmb  created 
*/

/*
DESCRIPTION


*/
// #define TIMING_DIAG_ON    /* compile in timing diagnostics */

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

/*
#include "taskPrior.h"
#include "hostAcqStructs.h"
#include "consoleStat.h"
#include "hostMsgChannels.h"
#include "expDoneCodes.h"
#include "errorcodes.h"
#include "acqcmds.h"
#include "logMsgLib.h"
*/
#include "taskPriority.h"
#include "expDoneCodes.h"
#include "errorcodes.h"
#include "instrWvDefines.h"

#include "logMsgLib.h"

#include "Monitor_Cmd.h"
#include "Cntlr_Comm.h"
#include "Lock_Cmd.h"

#include "vTypes.h"

#ifdef RTI_NDDS_4x
#include "Monitor_CmdPlugin.h"
#include "Cntlr_CommPlugin.h"
#include "Lock_CmdPlugin.h"
#include "Monitor_CmdSupport.h"
#include "Cntlr_CommSupport.h"
#include "Lock_CmdSupport.h"
#endif

#include "AParser.h"
#include "cntlrStates.h"
#include "flashUpdate.h"

#include "sysUtils.h"

#include "monitor.h"

#define HOSTNAME_SIZE 80
extern char hostName[HOSTNAME_SIZE];

extern MSG_Q_ID pMsgesToAParser;   /* MsgQ used for Msges to Acode Parser */
extern MSG_Q_ID pMsgesToXParser;   /* MsgQ used for Msges to X Parser */


extern NDDS_ID NDDS_Domain;

extern int DebugLevel;
extern int host_abort;

extern int SA_Criteria; /* criteria for SA, EXP_FID_CMPLT, BS_CMPLT, IL_CMPLT */
extern unsigned long SA_Mod; /* modulo criteria for SA, ie which fid to stop at 'il'*/
extern unsigned long SA_CTs;  /* Completed Transients for SA */


extern RING_ID  pSyncActionArgs;
extern ACODE_ID pTheAcodeObject;
extern SEM_ID pRoboAckSem;
extern MSG_Q_ID pRoboAckMsgQ;

MSG_Q_ID  pMsgesToMonitor;   /* MsgQ used for Msges to X Parser */
NDDS_ID   pMonitorPub, pMonitorSub;
NDDS_ID   pLockPub;
NDDS_ID   pGradPub;
SEM_ID    pSemLockCmdPub;
SEM_ID    pSemGradCmdPub;
MSG_Q_ID  pMsgesToFFSUpdate = NULL;   /* MsgQ used for Msges to X Parser */
static    int FFSUpdateCnt = 0;
static    char cntlrActiveList[CMD_MAX_STR_SIZE+2];
static    int ncntlrs = 0;
static    char nddsVerStr[80];

// if file exists on controller flash then this system has iCAT RF transmitters
// used to set the console status struct consoleID member so that host knows the RF type
#define RF_ICAT_FILE "rficat.conf"

/*
 *   The NDDS callback routine, the routine is call when an issue of the subscribed topic
 *   is delivered.
 *   called with the context of the NDDS task n_rtu7400
 *
 *  catches issues from Expproc
 */
#ifndef RTI_NDDS_4x
/* 3x callback */
RTIBool Monitor_CmdCallback(const NDDSRecvInfo *issue, NDDSInstance *instance,
                             void *callBackRtnParam)
{
    Monitor_Cmd *recvIssue;
    MONITOR_MSG monIssue;
    void decode(MONITOR_MSG *issue);
 
    if (issue->status == NDDS_FRESH_DATA)
    {
        recvIssue = (Monitor_Cmd *) instance;
        monIssue.cmd  = recvIssue->cmd;
        monIssue.arg1 = recvIssue->arg1;
        monIssue.arg2 = recvIssue->arg2;
        monIssue.arg3 = recvIssue->arg3;
        monIssue.arg4 = recvIssue->arg4;
        monIssue.arg5 = recvIssue->arg5;
        monIssue.crc32chksum = recvIssue->crc32chksum;
        monIssue.msgstr.len = recvIssue->msgstr.len;
        strncpy(monIssue.msgstr.val,recvIssue->msgstr.val,MAX_MONITOR_MSG_STR);
        DPRINT6(+1,"Monitor_Cmd CallBack:  cmd: %d, arg1: %d, arg2: %d, arg3: %d, crc: 0x%lx, msgstr: '%s'\n",
        recvIssue->cmd,recvIssue->arg1, recvIssue->arg2, recvIssue->arg3, recvIssue->crc32chksum, recvIssue->msgstr.val);
     	msgQSend(pMsgesToMonitor,(char*) &monIssue, sizeof(MONITOR_MSG), NO_WAIT, MSG_PRI_NORMAL);
        /* decode(recvIssue); */
    }
   return RTI_TRUE;
}

#else  /* RTI_NDDS_4x */


/* 4x callback */
void Monitor_CmdCallback(void* listener_data, DDS_DataReader* reader)
{
   Monitor_Cmd *recvIssue;
   MONITOR_MSG monIssue;
   DDS_ReturnCode_t retcode;
   DDS_Boolean result;
   struct DDS_SampleInfoSeq info_seq = DDS_SEQUENCE_INITIALIZER;
   struct Monitor_CmdSeq data_seq = DDS_SEQUENCE_INITIALIZER;
   struct DDS_SampleInfo* info = NULL;
   long i,numIssues;
   Monitor_CmdDataReader *MonitorCmd_reader = NULL;
   void decode(MONITOR_MSG *issue);


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
          monIssue.cmd  = recvIssue->cmd;
          monIssue.arg1 = recvIssue->arg1;
          monIssue.arg2 = recvIssue->arg2;
          monIssue.arg3 = recvIssue->arg3;
          monIssue.arg4 = recvIssue->arg4;
          monIssue.arg5 = recvIssue->arg5;
          monIssue.crc32chksum = recvIssue->crc32chksum;
          monIssue.msgstr.len = strlen(recvIssue->msgstr)+1;
          strncpy(monIssue.msgstr.val,recvIssue->msgstr,MAX_MONITOR_MSG_STR);
          DPRINT6(+1,"Monitor_Cmd CallBack:  cmd: %d, arg1: %d, arg2: %d, arg3: %d, crc: 0x%lx, msgstr: '%s'\n",
            recvIssue->cmd,recvIssue->arg1, recvIssue->arg2, recvIssue->arg3, recvIssue->crc32chksum, recvIssue->msgstr);
     	  msgQSend(pMsgesToMonitor,(char*) &monIssue, sizeof(MONITOR_MSG), NO_WAIT, MSG_PRI_NORMAL);
      }
   }
   retcode = Monitor_CmdDataReader_return_loan( MonitorCmd_reader,
                  &data_seq, &info_seq);

   return;
}

#endif  /* RTI_NDS_4x */
 

/*
 * routine to send commands to the lock controller 
 *
 */
#ifndef RTI_NDDS_4x

void send2Lock(int cmd,int arg1,int arg2,double arg3, double arg4)
{
   Lock_Cmd *issue;
   semTake(pSemLockCmdPub,WAIT_FOREVER);  /* binary one */
   issue = pLockPub->instance;
   issue->cmd  = cmd;
   issue->arg1 = arg1;
   issue->arg2 = arg2;
   issue->arg3 = arg3;
   issue->arg4 = arg4;
   nddsPublishData(pLockPub);
   semGive(pSemLockCmdPub);
}

#else /* RTI_NDDS_4x */

/* NDDS_4x Version */
int send2Lock(int cmd,int arg1,int arg2,double arg3, double arg4)
{
   DDS_ReturnCode_t result;
   DDS_InstanceHandle_t instance_handle = DDS_HANDLE_NIL;
   Lock_CmdDataWriter *LockCmd_writer = NULL;
   Lock_Cmd *issue;

   // DPRINT2(-12,"-----> send2Lock: arg3: %f, arg4: %f\n", arg3, arg4);

   LockCmd_writer = Lock_CmdDataWriter_narrow(pLockPub->pDWriter);
   if (LockCmd_writer == NULL) {
        errLogRet(LOGIT,debugInfo, "send2Lock: DataReader narrow error.\n");
        return -1;
    }
   semTake(pSemLockCmdPub,WAIT_FOREVER);  /* binary one */
   issue = (Lock_Cmd *) pLockPub->instance;
   issue->cmd  = cmd;
   issue->arg1 = arg1;
   issue->arg2 = arg2;
   issue->arg3 = arg3;
   issue->arg4 = arg4;
   result = Lock_CmdDataWriter_write(LockCmd_writer,
                issue,&instance_handle);
   if (result != DDS_RETCODE_OK) {
            errLogRet(LOGIT,debugInfo, "send2Lock: write error %d\n",result);
   }
   semGive(pSemLockCmdPub);
   return 0;
}

// version that execFunc can used, only integers can be passed NOT doubles
//  so double are encoded as two integers.
send2LockEncoded(int cmd,int arg1,int arg2,int arg3, int arg4, int arg5, int arg6)
{
   union {
      long long lword;
      double dword;
      int  nword[2];
     }  encoder1,encoder2;

     // double arg3
     encoder1.nword[0] = arg3; 
     encoder1.nword[1] = arg4;
     // DPRINT3(-1,"-----> send2LockEncoded: arg3: 0x%lx, 0x%lx, %f \n", encoder1.nword[0], encoder1.nword[1], encoder1.dword);

     // double arg4
     encoder2.nword[0] = arg3; 
     encoder2.nword[1] = arg4;
     send2Lock(cmd,arg1,arg2,encoder1.dword, encoder2.dword);
     return 0;
}

#endif /* RTI_NDDS_4x */

tstLkCmd()
{
   send2Lock(LK_SET_RATE,0,0,1.0, 20.0);
}


#ifdef XXX
restartComLnk()
{
   int tid,stat,priority,stacksize,taskoptions;
   /* if tComLink still around then Connection was never made */
   DPRINT(1,"Time to Restart Exppproc Link.");
    if ((tid = taskNameToId("tMonitor")) != ERROR)
    {
       stat = taskDelete(tid);
    }
    if ((tid = taskNameToId("tStatAgent")) != ERROR)
    {
       stat = taskDelete(tid);
    }
    if ((tid = taskNameToId("tStatMon")) != ERROR)
    {
       stat = taskDelete(tid);
    }
   }
}

killComLnk()
{
   int tid;

   if ((tid = taskNameToId("tMonitor")) != ERROR)
       taskDelete(tid);
   if ((tid = taskNameToId("tStatAgent")) != ERROR)
       taskDelete(tid);
   if ((tid = taskNameToId("tStatMon")) != ERROR)
       taskDelete(tid);
}
#endif 


#define  MIN_INTERVAL	10
#define  MAX_INTERVAL	5000
#define  DEFAULT_INTERVAL  MAX_INTERVAL
#define  UNITS_PER_SEC	1000

/*
extern STATUS_BLOCK	currentStatBlock;

static struct {
	int	currentTicks;
	SEM_ID	newValueTicks;
} statblock_coordinator;
*/

#ifdef XXX
startStatMonitor( int taskpriority, int taskoptions, int stacksize )
{
   int statmonitor();

   statblock_coordinator.currentTicks = calc_new_interval( DEFAULT_INTERVAL );
   if (taskNameToId("tStatMon") == ERROR)
     taskSpawn("tStatMon",taskpriority,taskoptions,stacksize,statmonitor,
		1,2,3,4,5,6,7,8,9,10);
}
#endif 

#ifndef RTI_NDDS_4x
send2Expproc(int cmd,int arg1,int arg2,int arg3,char *msgstr,long len) /* NO_WAIT, MSG_PRI_NORMAL); */
{
   Monitor_Cmd *issue;
   issue = pMonitorPub->instance;
   issue->cmd  = cmd;  /* atoi( token ); */
   issue->arg1 = arg1;
   issue->arg2 = arg2;
   issue->arg3 = arg3;
   issue->msgstr.len = len;
   memcpy(issue->msgstr.val,msgstr,len);
   nddsPublishData(pMonitorPub);
}

#else

/* NDDS_4x Version */
// int send2Monitor(int cmd, int arg1, int arg2, int arg3, char *msgstr, long len)
int send2Expproc(int cmd, int arg1, int arg2, int arg3, char *msgstr, long len)
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

#endif /* RTI_NDDS_4x */

/*************************************************************
*
*  monitor - Wait for Message from Host 
*   Receives Messages from Host and routes them to the proper
*   task for handling. 
*
*			Author Greg Brissey 10-12-04
*/
monitor()
{
   void decode();
   MONITOR_MSG Cmd;
   int bytes;

   DPRINT(-1,"Monitor 1:Server LOOP Ready & Waiting.\n");
   FOREVER
   {
     /* if connection lost this routine send msge to phandler LOST_CONN */
     bytes = msgQReceive(pMsgesToMonitor, (char*) &Cmd,sizeof(MONITOR_MSG), WAIT_FOREVER);
     DPRINT1(2,"Monitor: got %d bytes\n",bytes);

     decode( &Cmd );
   } 
}

startMonitor(int taskpriority,int taskoptions,int stacksize)
{
    pMsgesToMonitor = msgQCreate(20, sizeof(MONITOR_MSG), MSG_Q_FIFO);
    if (taskNameToId("tMonitor") == ERROR)
     taskSpawn("tMonitor",taskpriority,taskoptions,
                   stacksize,monitor, 1,2,3,4,5,6,7,8,9,10);
}

/*********************************************************************
*
* decode base on the cmd code decides what msgeQ to put the message into
*  then it returns
*
*					Author Greg Brissey 10-6-94
*/
void decode(MONITOR_MSG *issue)
{
   char *token;
   int len;
   int cmd;
   MONITOR_MSG  cmdIssue;

   DPRINT4(+2,"decode cmd: %d, arg1: %d, arg2: %d, arg3: %d\n", issue->cmd,
		issue->arg1,issue->arg2,issue->arg3);
   DPRINT1(+2,"decode Msge Str: len: %lu\n",issue->msgstr.len);
   if (issue->msgstr.len > 0)
   {
       issue->msgstr.val[issue->msgstr.len] = '\0';
       DPRINT1(+2,"decode Msge Str: '%s'\n",issue->msgstr.val);
   }

   switch( issue->cmd )
   {
     case ECHO:
	  {
		long	ival;
   	        DPRINT(1,"ECHO\n");
	    
                /* 1st arg is the PARSE command being return to expproc */
	        /*
		cmdIssue.cmd  = issue->arg1; 
                cmdIssue.msgstr.len = issue->msgstr.len;
                cmdIssue.msgstr.val = issue->msgstr.val;
	        */

#ifdef INSTRUMENT
     		wvEvent(EVENT_MONITOR_CMD_ECHO,NULL,NULL);
#endif
		send2Expproc(issue->arg1,0,0,0,issue->msgstr.val,issue->msgstr.len); /* NO_WAIT, MSG_PRI_NORMAL); */
	  }
	  break;

     case XPARSER:
   	   DPRINT(1,"XPARSER\n");
	  msgQSend(pMsgesToXParser, issue->msgstr.val, issue->msgstr.len+1, NO_WAIT, MSG_PRI_NORMAL);
	  break;

     case APARSER:
          {
	          PARSER_MSG pCmd;
   	       DPRINT(1,"APARSER\n");
             pCmd.parser_cmd = issue->cmd;
	          pCmd.NumAcodes = issue->arg1;
	          pCmd.NumTables = issue->arg2;
	          pCmd.startFID = issue->arg3;
	          strncpy(pCmd.AcqBaseBufName,issue->msgstr.val ,32);
     
#ifdef TIMING_DIAG_ON    /* compile in timing diagnostics */
             TSPRINT("Monitor APARSER: ");
#endif

             /* start my own parser */
     	       msgQSend(pMsgesToAParser,(char*) &pCmd, sizeof(PARSER_MSG), NO_WAIT, MSG_PRI_NORMAL);
             /* Now the master will tell the other contoller to start when it is ready */
             /* tell the other controller to start theirs */
             /* send2AllCntlrs(CNTLR_CMD_APARSER, pCmd.NumAcodes,pCmd.NumTables,pCmd.startFID,pCmd.AcqBaseBufName); */
          }
	  break;

     case STARTINTERACT:
	   DPRINT1( 1, "STARTINTERACT will send %s to the A-Parser\n", token );
     	   /* msgQSend(pMsgesToAParser, token, len+1, NO_WAIT, MSG_PRI_NORMAL); */
	  break;

     case AUPDT:
   	   DPRINT(1,"AUPDT\n");
     	   /* msgQSend(pMsgesToAupdt, token, len+1, NO_WAIT, MSG_PRI_NORMAL); */
	  break;

     case HALTACQ:    /* equivilent of AA but has wexp processing instead of werr processing */
   	   DPRINT(-1,"HALTACQ\n");
           host_abort = 1;
           assertFailureLine();  /* hardline to stop and reset all controllers FIFOs, reset to safe states */
           sendException(EXP_HALTED, EXP_HALTED, 0,0,NULL);
#ifdef INSTRUMENT
           wvEvent(EVENT_MONITOR_CMD_HALTACQ,NULL,NULL);
#endif
          break;

     case FLASH_UPDATE:
          {
            int cmd,result;
            int fileSize;
            int waitTimeout;
            // int nRfIcats;
            char msgstr[CMD_MAX_STR_SIZE+2];
            char cntlrList[CMD_MAX_STR_SIZE+2];
            // char cntlrConfList[256];
            char filename[256];
            char md5sig[64];
            char newname[256];
            unsigned long srcCRC;
            char *pCList;
            char *strptr;
            FLASH_UPDATE_MSG ffsMsge;
            char CntrlMsgeStr[CMD_MAX_STR_SIZE];

            pCList = msgstr;

            FFSUpdateCnt++;

            if (pMsgesToFFSUpdate == NULL)
            {
		          DPRINT(-1,"Start FlashFS Update Task.\n");
                pMsgesToFFSUpdate =  startFlashUpdate(FFSUPDATE_TASK_PRIORITY,STD_TASKOPTIONS,256*1024);
            }

            DPRINT(-1,"======>>> Start FLASH_UPDATE");
            /* remove all controllers list in the state obkect */
            if (FFSUpdateCnt == 1)  /* 1st time through rollcall everbody then keep it */
            {
               cntlrStatesRemoveAll();
               cntlrStatesAdd("master1",CNTLR_READYnIDLE,MASTER_CONFIG_STD);
               /* populate the controllers in the state object via rollcall */
               rollcall();
               taskDelay(calcSysClkTicks(500));  /* 1/2 sec  taskDelay(30); */
               rollcall();
               taskDelay(calcSysClkTicks(500));  /* 1/2 sec  taskDelay(30); */
               ncntlrs =  cntlrPresentGet(cntlrActiveList);
               cntlrSetInUseAll(CNTLR_INUSE_STATE);
               cntlrSetStateAll(CNTLR_NOT_READY);
               /*   moved test to master.c 
               // scans all rf configs even though given rf1
               nRfIcats = cntlrConfigGet("rf1",RF_CONFIG_ICAT,cntlrConfList);
               DPRINT2(-9,"iCAT RFs: %d, list: '%s'\n",nRfIcats,cntlrConfList);
               if (nRfIcats > 0)
               {
                  char *msg = "RF: iCAT";
                  cpBuf2FF(RF_ICAT_FILE,msg,(strlen(msg) + 1));
               }
               else
              */
               ffdel(RF_ICAT_FILE);  // remove in about 10 builds
            }
            else
            {
               cntlrSetStateAll(CNTLR_NOT_READY);
            }
            DPRINT2(-1,"=======>>> FLASH_UPDATE: %d - cntrls: '%s'\n",ncntlrs,cntlrActiveList);
	         send2Expproc(39,ncntlrs,0,0,cntlrActiveList,strlen(cntlrActiveList)+1); /* NO_WAIT, MSG_PRI_NORMAL); */

            ffsMsge.cmd = CNTLR_CMD_FFS_UPDATE;
            ffsMsge.filesize = issue->arg2;   /* file size */
            ffsMsge.arg2 = issue->arg3;	      /* defered update flag */
            ffsMsge.arg3 = 0;
            ffsMsge.crc32chksum = issue->crc32chksum;
            strncpy(ffsMsge.msgstr,issue->msgstr.val,MAX_FLASH_UPDATE_MSG_STR);

            /* timeout value in seconds */
            waitTimeout =  (issue->arg1 > 0)  ? issue->arg1 : 60;  /* default to 60 seconds */
            DPRINT1(-1,"Time to Wait: %d sec.\n",waitTimeout);

            DPRINT3(-1,"cmd: %d, filesize: %ld, msge: '%s'\n",ffsMsge.cmd,ffsMsge.filesize,ffsMsge.msgstr);

            /* if (pMsgesToFFSUpdate == NULL)
            {
		          DPRINT(-1,"Start FlashFS Update Task.\n");
                pMsgesToFFSUpdate =  startFlashUpdate(FFSUPDATE_TASK_PRIORITY,STD_TASKOPTIONS,256*1024);
            } */
            DPRINT(-1,"\n\n========>> FLASH_UPDATE: Initiate FLASH Update to all Controllers\n");

            /* tell the master directly */
     	      msgQSend(pMsgesToFFSUpdate,(char*) &ffsMsge, sizeof(FLASH_UPDATE_MSG), WAIT_FOREVER, MSG_PRI_NORMAL);

            /* tell other controllers to update flash */
            send2AllCntlrs(ffsMsge.cmd, issue->arg2,issue->arg2,issue->arg3,ffsMsge.msgstr);

            /* Wait for all to complete.... */
            /* result = cntlrStatesCmpList(CNTLR_FLASH_UPDATE_CMPLT, waitTimeout, cntlrList);  /* one minute to complete */
            result = cntlrStatesCmpListUntilAllCtlrsRespond(CNTLR_FLASH_UPDATE_CMPLT, waitTimeout, cntlrList);
            if (result > 0)
            {
	             send2Expproc(39,ncntlrs,0,0,cntlrList,strlen(cntlrList)+1); /* NO_WAIT, MSG_PRI_NORMAL); */
                DPRINT(-1,"========>> FLASH_UPDATE: FAILURE \n");
                DPRINT1(-1,"           Controllers: '%s' FAILED  to Complete within the timeout\n",
			                    cntlrList);
            }
            else
            {
	             send2Expproc(40,0,0,0,cntlrList,0); /* NO_WAIT, MSG_PRI_NORMAL); */
                DPRINT(-1,"========>> FLASH_UPDATE: Successful Completion\n");
            }
          }

          break;

     case FLASH_COMMIT:
          {
            int result,ncntlrs;
            int waitTimeout;
            char *ErrorMsg = "Update Task is not running\n";
            FLASH_UPDATE_MSG ffsMsge;
            char cntlrList[CMD_MAX_STR_SIZE+2];

            cntlrSetInUseAll(CNTLR_INUSE_STATE);
            cntlrSetStateAll(CNTLR_NOT_READY);

            if (pMsgesToFFSUpdate == NULL)
            {
		/* this is an error */
	        send2Expproc(39,1,0,0,ErrorMsg,strlen(ErrorMsg)+1); /* NO_WAIT, MSG_PRI_NORMAL); */
	        break;
            }
            ffsMsge.cmd = CNTLR_CMD_FFS_COMMIT;
            ffsMsge.filesize = issue->arg2;   /* file size */
            ffsMsge.arg2 = issue->arg3;	      /* defered update flag */
            ffsMsge.arg3 = 0;
            ffsMsge.crc32chksum = issue->crc32chksum;
            strncpy(ffsMsge.msgstr,issue->msgstr.val,MAX_FLASH_UPDATE_MSG_STR);

            DPRINT(-1,"========>> FLASH_COMMIT: Initiate FLASH Commit to all Controllers\n");
            /* timeout value in seconds */
            waitTimeout =  (issue->arg1 > 0)  ? issue->arg1 : 120;  /* default to 120 seconds */
            /* DPRINT2(+1,"passed timeout: %d, Timeout: %d\n",issue->arg1,waitTimeout); */
            DPRINT1(-1,"Time to Wait: %d sec.\n",waitTimeout);

            /* tell other controllers to commit update flash files */
            send2AllCntlrs(ffsMsge.cmd, issue->arg2,issue->arg2,issue->arg3,ffsMsge.msgstr);
            /* tell the master directly */
     	    msgQSend(pMsgesToFFSUpdate,(char*) &ffsMsge, sizeof(FLASH_UPDATE_MSG), WAIT_FOREVER, MSG_PRI_NORMAL);

            /* Wait for all to complete.... */
            /* result = cntlrStatesCmpList(CNTLR_FLASH_COMMIT_CMPLT, waitTimeout, cntlrList);  /* one minute to complete */
            result = cntlrStatesCmpListUntilAllCtlrsRespond(CNTLR_FLASH_COMMIT_CMPLT, waitTimeout, cntlrList);
            if (result > 0)
            {
	        send2Expproc(39,ncntlrs,0,0,cntlrList,strlen(cntlrList)+1); /* NO_WAIT, MSG_PRI_NORMAL); */
                DPRINT(-1,"========>> FLASH_COMMIT: FAILURE \n");
                DPRINT1(-1,"           Controllers: '%s' FAILED  to Complete within the timeout\n",
			cntlrList);
            }
            else
            {
	        send2Expproc(40,0,0,0,cntlrList,0); /* NO_WAIT, MSG_PRI_NORMAL); */
                DPRINT(-1,"========>> FLASH_COMMIT: Successful Completion\n");
            }

          }

          break;

     case FLASH_DELETE:
          {
            int cmd,result;
            int fileSize,ncntlrs;
            int waitTimeout;
            char msgstr[CMD_MAX_STR_SIZE+2];
            char cntlrList[CMD_MAX_STR_SIZE+2];
            char filename[256];
            char *pCList;
            char *strptr;
            FLASH_UPDATE_MSG ffsMsge;
            char CntrlMsgeStr[CMD_MAX_STR_SIZE];

            ncntlrs =  cntlrPresentGet(cntlrList);
            cntlrSetInUseAll(CNTLR_INUSE_STATE);
            cntlrSetStateAll(CNTLR_NOT_READY);
            DPRINT2(-1,"=======>>> FLASH_DELETE: %d - cntrls: '%s'\n",ncntlrs,cntlrList);
	    send2Expproc(39,ncntlrs,0,0,cntlrList,strlen(cntlrList)+1); /* NO_WAIT, MSG_PRI_NORMAL); */

            ffsMsge.cmd = CNTLR_CMD_FFS_DELETE;
            ffsMsge.filesize = 0;   /* file size */
            ffsMsge.arg2 = 0;
            ffsMsge.arg3 = 0;
            ffsMsge.crc32chksum = issue->crc32chksum;
            strncpy(ffsMsge.msgstr,issue->msgstr.val,MAX_FLASH_UPDATE_MSG_STR);

            /* timeout value in seconds */
            waitTimeout =  (issue->arg1 > 0)  ? issue->arg1 : 60;  /* default to 60 seconds */
            DPRINT2(-1,"passed timeout: %d, Timeout: %d\n",issue->arg1,waitTimeout);

            DPRINT3(-1,"cmd: %d, filesize: %ld, msge: '%s'\n",ffsMsge.cmd,ffsMsge.filesize,ffsMsge.msgstr);
            if (pMsgesToFFSUpdate == NULL)
            {
		DPRINT(-1,"Start FlashFS Update Task.\n");
                pMsgesToFFSUpdate = startFlashUpdate( 70, STD_TASKOPTIONS, 256*1024);
            }
            DPRINT(-1,"========>> FLASH_DELETE: Initiate FLASH Delete to all Controlles\n");
            /* tell other controllers to update flash */
            send2AllCntlrs(ffsMsge.cmd, issue->arg2,issue->arg2,0,ffsMsge.msgstr);
            /* tell the master directly */
     	    msgQSend(pMsgesToFFSUpdate,(char*) &ffsMsge, sizeof(FLASH_UPDATE_MSG), WAIT_FOREVER, MSG_PRI_NORMAL);

            /* Wait for all to complete.... */
            result = cntlrStatesCmpListUntilAllCtlrsRespond(CNTLR_FLASH_UPDATE_CMPLT, waitTimeout, cntlrList);  /* one minute to complete */
            if (result > 0)
            {
	        send2Expproc(39,ncntlrs,0,0,cntlrList,strlen(cntlrList)+1); /* NO_WAIT, MSG_PRI_NORMAL); */
                DPRINT(-1,"========>> FLASH_DELETE: FAILURE \n");
                DPRINT1(-1,"           Controllers: '%s' FAILED  to Complete within the timeout\n",
			cntlrList);
            }
            else
            {
	        send2Expproc(40,0,0,0,cntlrList,0); /* NO_WAIT, MSG_PRI_NORMAL); */
                DPRINT(-1,"========>> FLASH_DELETE: Successful Completion\n");
            }
          }
          break;

     case ABORTACQ:
   	    DPRINT(-1,"==========>> ABORTACQ <<===============\n");
           host_abort = 1;
           assertFailureLine();  /* hardline to stop and reset all controllers FIFOs, reset to safe states */
           sendException(EXP_ABORTED, EXP_ABORTED, 0,0,NULL);
#ifdef INSTRUMENT
           wvEvent(EVENT_MONITOR_CMD_ABORTACQ,NULL,NULL);
#endif
		break;
     case NDDS_VER_QUERY:
          {
            extern char *nddsverstr(char *str);
            nddsverstr(nddsVerStr);
	         send2Expproc(NDDS_VER_QUERY,1,0,0,nddsVerStr,strlen(nddsVerStr)+1);
          }
          break;

     case STATINTERV:
	  break;

     case STARTLOCK:
	  break;

     case GETINTERACT:
	  break;

     case STOPINTERACT:
	  break;

     case STOP_ACQ:
          {
             SA_Criteria = issue->arg1;
             SA_Mod = issue->arg2;
           
#ifdef INOVA
#ifdef INSTRUMENT
           wvEvent(EVENT_MONITOR_CMD_STOPACQ,NULL,NULL);
#endif     
             /* if EOS SA then enable End-of-Transient SW2 interrupt */
             if (SA_Criteria == CT_CMPLT)
             {
                  fifoItrpEnable(pTheFifoObject, SW2_I );
             }
#endif
             DPRINT2(1,"STOP_ACQ, SA Criteria: %d, Mod: %lu\n",
                SA_Criteria,SA_Mod);
	  }
	  break;

     case ACQDEBUG:
#ifdef INSTRUMENT
             wvEvent(EVENT_MONITOR_CMD_ACQDEBUG,NULL,NULL);
#endif
             DebugLevel = issue->arg1;
             send2AllCntlrs(CNTLR_CMD_SET_DEBUGLEVEL, DebugLevel,0,0,NULL );
             DPRINT1(-1,"ACQDEBUG: DebugLevel = %d\n",DebugLevel);
	  break;

     case HEARTBEAT:
	  break;

     case GETSTATBLOCK:
          sendConsoleStatus();
          /* The stat block publisher has a minimun time between statblocks of
             250 msec. ( see createConsoleStatSub() in nddsinfofuncs.c nvinfoproc).
             If two or more readhw() commands are executed in quick succession,
             the stat block for the second one may not be sent because of this
             minimum time between publications. We don't want to decrease the
             minimum time to something like 1 msec., because then, during a hign-speed
             acquisition where ct may be changing quite rapidly, the system could
             be flooded with stat block transfers. Our solution is to slow down the
             readhw sethw functions. 
           */
          taskDelay(calcSysClkTicks(300));
	  break;

     case ABORTALLACQS:
          {
	      DPRINT(-1,"ABORTALLACQS: reboot all controllers\n");
              /*  BOOT_CLEAR provides for an automatic restart with memory cleared.
                  The system attempts to reload the VxWorks kernel and execute any
                  startup script, as provided for in the PROM parameters.

#ifdef INSTRUMENT
               wvEvent(EVENT_MONITOR_CMD_ABORTALLACQS,NULL,NULL);
#endif
             /* resetFPGA(); /* reset FPGA , also diables all interrupts */
             host_abort = 1;

             /* since this is equivalent to AA and more, assert the failine to all controllers go to
                safe states prior to rebooting */
             assertFailureLine();
             taskDelay(calcSysClkTicks(2000));  /*  2 sec, wait tobe sure serialization has been completed */
             reboot(1);	         /* reboot causing the master to assert System-Reset which goes to 
				    out on the back-plane to all other controllers */

/*    The old way for older controllers that didn't allow for the system reset to be asserted */
       /* reliance on commnuncation to reboot could be a problem. */
/*
             send2AllCntlrs(CNTLR_CMD_REBOOT, 0,0,0,NULL );
             wait4Pub2BSent();
             taskDelay(calcSysClkTicks(5000));
             reboot();
*/
 
          }

	  break;

     case OK2TUNE:
           break;

     case ROBO_CMD_ACK:
          {
             int cmdAck;
             cmdAck = issue->arg1;
             if (cmdAck == -1)
             {
                   errLogRet( LOGIT, debugInfo,
                      "ROBO_CMD_ACK: status  of -1, report Timeout\n" );
                   cmdAck = SMPERROR + SMPTIMEOUT;  /* default to timeout */
              }      
            DPRINT1(-1,"ROBO_CMD_ACK: status value: %d \n",cmdAck);
#ifdef INSTRUMENT
           wvEvent(EVENT_MONITOR_CMD_ROBO_ACK,NULL,NULL);
#endif
            /* DPRINT2(-1,"ROBO_CMD_ACK: status: %d into ringbuffer pSyncActionArgs: 0x%lx\n",
		cmdAck,pSyncActionArgs); */

            msgQSend(pRoboAckMsgQ,(char*) &cmdAck,sizeof(int), WAIT_FOREVER, MSG_PRI_NORMAL);

#ifdef NOT_USING_SEM
            rngBufPut(pSyncActionArgs,(char*) &cmdAck,sizeof(cmdAck));
            DPRINT1(+2,"ROBO_CMD_ACK: semGive: pRoboAckSem: 0x%lx\n", pRoboAckSem);
            semGive(pRoboAckSem);
#endif
           }

	 break;

     case CONSOLEINFO:
         break;

     case DOWNLOAD:
         break;

     case STARTFIDSCOPE:
         break;

     case STOPFIDSCOPE:
         break;

     case GETCONSOLEDEBUG:
         break;

     default:
         errLogRet(LOGIT,debugInfo, "Host Cmd: %d Unknown.\n",cmd);
         break;
   }
   return;
}

#ifdef NOT_USED

hasIcatRF()
{
   HANDLE newFile;
   if ((newFile = vfOpenFile(RF_ICAT_FILE)) == 0)
   {
        /* file not present */
        // printf("File: '%s' does not exist on FFS.\n",filename);
        return(0);
   }
   vfCloseFile(newFile);
   return 1;

}
#endif

/* NDDS */
NDDS_ID createMonitorCmdPub(NDDS_ID nddsId)
{
     int result;
     NDDS_ID pPubObj;
     char pubtopic[128];

    /* BUild Data type Object for both publication and subscription to Expproc */
    /* ------- malloc space for data type object --------- */
    if ( (pPubObj = (NDDS_ID) malloc( sizeof(NDDS_OBJ)) ) == NULL )
      {  
        return(NULL);
      }  

    /* zero out structure */
    memset(pPubObj,0,sizeof(NDDS_OBJ));
    memcpy(pPubObj,nddsId,sizeof(NDDS_OBJ));

    /* sprintf(pubtopic,"%s/h/dwnld/reply",hostName); */
    strcpy(pPubObj->topicName,CNTLR_PUB_CMDS_TOPIC_FORMAT_STR);
    pPubObj->pubThreadId = 10; /* DEFAULT_PUB_THREADID; taskIdSelf(); */

    DPRINT1(1,"createMonitorCmdPub: topic: '%s'\n",pPubObj->topicName);
         
    /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */
    getMonitor_CmdInfo(pPubObj);
         
#ifdef RTI_NDDS_4x
     pPubObj->queueSize  = 2;
     pPubObj->highWaterMark = 1;
     pPubObj->lowWaterMark =  0;
     pPubObj->AckRequestsPerSendQueue = 2;
     initPublication(pPubObj);
#endif
    createPublication(pPubObj);
    return(pPubObj);
}        

/* NDDS */
NDDS_ID createMonitorCmdSubscription(NDDS_ID nddsId)
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
 
    strcpy(pSubObj->topicName,CNTLR_SUB_CMDS_TOPIC_FORMAT_STR);
 
    DPRINT1(1,"createMonitorCmdSubscription: Topic: '%s'\n",pSubObj->topicName);
    /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */
    getMonitor_CmdInfo(pSubObj);
 
    pSubObj->MulticastSubIP[0] = 0;   /* use UNICAST */

#ifndef RTI_NDDS_4x
    pSubObj->callBkRtn = Monitor_CmdCallback;
    pSubObj->callBkRtnParam = NULL;
#else  /* RTI_NDDS_4x */
    initSubscription(pSubObj);
    attachOnDataAvailableCallback(pSubObj,Monitor_CmdCallback,NULL/* user data */ );
#endif
    createSubscription(pSubObj);
    return ( pSubObj );
}



/*
 * Create a Publication Topic to communicate with the Lock
 *
 *					Author Greg Brissey 5-06-04
 */
NDDS_ID createLockCmdPub(NDDS_ID nddsId)
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

    pSemLockCmdPub = semBCreate(SEM_Q_FIFO, SEM_FULL);
    if (pSemLockCmdPub == NULL)
    {
       free(pPubObj);
       return(NULL);
    }

    /* zero out structure */
    memset(pPubObj,0,sizeof(NDDS_OBJ));
    memcpy(pPubObj,nddsId,sizeof(NDDS_OBJ));

    /* sprintf(pPubObj->topicName,"%s/lock/cmd",hostName); */
#ifndef RTI_NDDS_4x
    sprintf(pPubObj->topicName,LOCK_CMDS_TOPIC_FORMAT_STR,hostName);
#else  /* RTI_NDDS_4x */
    strcpy(pPubObj->topicName,LOCK_CMDS_TOPIC_STR);
#endif /* RTI_NDDS_4x */
    pPubObj->pubThreadId = 20;   /* DEFAULT_PUB_THREADID; taskIdSelf(); */
         
    /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */
    getLock_CmdInfo(pPubObj);
         
    DPRINT1(1,"createLockCmdPub: topic: '%s'\n",pPubObj->topicName);
#ifdef RTI_NDDS_4x
    pPubObj->queueSize  = 2;
    pPubObj->highWaterMark = 1;
    pPubObj->lowWaterMark =  0;
    pPubObj->AckRequestsPerSendQueue = 2;
    initPublication(pPubObj);
#endif
    createPublication(pPubObj);
#ifdef RTI_NDDS_4x
//    Register_LockCmd_keyed_instance(pPubObj,1);  not needed for this app
#endif
    return(pPubObj);
}        

#ifdef  NOT_USED_XXX
#ifdef RTI_NDDS_4x
* /* Register an instance based on key given */
* int Register_LockCmd_keyed_instance(NDDS_ID pNDDS_Obj,long key)
* {
*     Lock_Cmd *pIssue = NULL;
*     Lock_CmdDataWriter *LockCmdWriter = NULL;
* 
*     LockCmdWriter = Lock_CmdDataWriter_narrow(pNDDS_Obj->pDWriter);
*     if (LockCmdWriter == NULL) {
*         errLogRet(LOGIT,debugInfo, "Register_LockCmd_keyed_instance: DataReader narrow error.\n");
*         return -1;
*     }
* 
*     pIssue = pNDDS_Obj->instance;
* 
*     pIssue->key = key;
*     // pIssue->key = 1;
* 
*     // for Keyed Topics must register this keyed topic
*     Lock_CmdDataWriter_register_instance(LockCmdWriter, pIssue);
* 
*     return 0;
* }
#endif
#endif

/*
 * Create a Publication Topic to communicate with the Grad
 *
 *					Author Greg Brissey 5-06-04
 */
NDDS_ID createGradCmdPub(NDDS_ID nddsId)
{
    NDDS_ID pPubObj;

   /* Build Data Object for both publication and subscription to Gradient */
   /* ------- malloc space for data type object --------- */
   if ( (pPubObj = (NDDS_ID) malloc( sizeof(NDDS_OBJ)) ) == NULL )
   {  
      return(NULL);
   }  

   pSemGradCmdPub = semBCreate(SEM_Q_FIFO, SEM_FULL);
   if (pSemGradCmdPub == NULL)
   {
      free(pPubObj);
      return(NULL);
   }

   /* zero out structure */
   memset(pPubObj,0,sizeof(NDDS_OBJ));
   memcpy(pPubObj,nddsId,sizeof(NDDS_OBJ));

   /* GRAD_SUB_CMDS_PATTERN_TOPIC_STR */
#ifndef RTI_NDDS_4x
   sprintf(pPubObj->topicName,"%s/gradient/comm",hostName);
#else  /* RTI_NDDS_4x */
    strcpy(pPubObj->topicName,GRAD_CMDS_TOPIC_STR);
#endif /* RTI_NDDS_4x */
   DPRINT1(-1,"createGradCmdPub: create %s publication\n", pPubObj->topicName);
   pPubObj->pubThreadId = 22;   /* V is 22nd letter in alphabet */
         
   /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */
   getLock_CmdInfo(pPubObj);
         
#ifdef RTI_NDDS_4x
    pPubObj->queueSize  = 2;
    pPubObj->highWaterMark = 1;
    pPubObj->lowWaterMark =  0;
    pPubObj->AckRequestsPerSendQueue = 2;
    initPublication(pPubObj);
#endif
   createPublication(pPubObj);
#ifdef RTI_NDDS_4x
//    Register_LockCmd_keyed_instance(pPubObj,2);
#endif
   return(pPubObj);
}        

/*
 * routine to send commands to the lock controller 
 *
 */
#ifndef RTI_NDDS_4x

void send2Grad(int cmd,int arg1,int arg2,double arg3, double arg4)
{
Lock_Cmd *issue;
// DPRINT(-1,"sending pGradPub\n");
   semTake(pSemGradCmdPub,WAIT_FOREVER);  /* binary one */
// DPRINT(-1,"got Semaphore\n");
   issue = pGradPub->instance;
   issue->cmd  = cmd;
   issue->arg1 = arg1;
   issue->arg2 = arg2;
   issue->arg3 = arg3;
   issue->arg4 = arg4;
// DPRINT(-1,"here goes pGradPub\n");
   nddsPublishData(pGradPub);
   semGive(pSemGradCmdPub);
// DPRINT(-1,"gave Semaphore\n");
}

#else /* RTI_NDDS_4x */

/* NDDS_4x Version */
int send2Grad(int cmd,int arg1,int arg2,double arg3, double arg4)
{
   DDS_ReturnCode_t result;
   DDS_InstanceHandle_t instance_handle = DDS_HANDLE_NIL;
   Lock_CmdDataWriter *GradCmd_writer = NULL;
   Lock_Cmd *issue;

   GradCmd_writer = Lock_CmdDataWriter_narrow(pGradPub->pDWriter);
   if (GradCmd_writer == NULL) {
        errLogRet(LOGIT,debugInfo, "send2Grad: DataReader narrow error.\n");
        return -1;
    }
// DPRINT(-1,"sending pGradPub\n");
   semTake(pSemGradCmdPub,WAIT_FOREVER);  /* binary one */
// DPRINT(-1,"got Semaphore\n");
   issue = (Lock_Cmd *) pGradPub->instance;
   issue->cmd  = cmd;
   issue->arg1 = arg1;
   issue->arg2 = arg2;
   issue->arg3 = arg3;
   issue->arg4 = arg4;
// DPRINT(-1,"here goes pGradPub\n");
   result = Lock_CmdDataWriter_write(GradCmd_writer,
                issue,&instance_handle);
   if (result != DDS_RETCODE_OK) {
            errLogRet(LOGIT,debugInfo, "send2Grad: write error %d\n",result);
   }
   semGive(pSemGradCmdPub);
// DPRINT(-1,"gave Semaphore\n");
}

#endif /* RTI_NDDS_4x */
tstGradCmd()
{
   send2Grad(13 /*SHIMDAC*/, 16/*X*/, -300, 0.0, 0.0);
}

/* NDDS */
initialMonitor()
{
    pMonitorPub = createMonitorCmdPub(NDDS_Domain);
    pMonitorSub = createMonitorCmdSubscription(NDDS_Domain);
    pLockPub = createLockCmdPub(NDDS_Domain);
    pGradPub = createGradCmdPub(NDDS_Domain);
    startMonitor(MONITOR_TASK_PRIORITY,STD_TASKOPTIONS,BIG_STACKSIZE);
}

abortacq()
{
   assertFailureLine();  /* hardline to stop and reset all controllers FIFOs, reset to safe states */
   sendException(EXP_ABORTED, EXP_ABORTED, 0,0,NULL);
}

tstRfConfig()
{
   int nRfIcats,nRfStd;
   char cntlrConfList[256];
   nRfStd = cntlrConfigGet("rf1",RF_CONFIG_STD,cntlrConfList);
   DPRINT2(-9,"Standard RFs: %d, list: '%s'\n",nRfStd,cntlrConfList);
   nRfIcats = cntlrConfigGet("rf1",RF_CONFIG_ICAT,cntlrConfList);
   DPRINT2(-9,"iCAT RFs: %d, list: '%s'\n",nRfIcats,cntlrConfList);
   if (nRfIcats > 0)
   {
     char *msg = "RF: iCAT";
     
     DPRINT(-9,"writing rficat.conf file to flash\n");
     cpBuf2FF(RF_ICAT_FILE,msg,(strlen(msg) + 1));
   }
   else
     ffdel(RF_ICAT_FILE);
}
