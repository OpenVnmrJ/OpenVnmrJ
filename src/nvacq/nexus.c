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
4-27-04,gmb  created 
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

#include "expDoneCodes.h"
#include "errorcodes.h"
#include "instrWvDefines.h"
#include "taskPriority.h"
#include "nvhardware.h"

#include "logMsgLib.h"

#include "NDDS_Obj.h"
#include "NDDS_PubFuncs.h"
#include "NDDS_SubFuncs.h"

#include "Monitor_Cmd.h"
#include "Cntlr_Comm.h"

#ifdef RTI_NDDS_4x
#include "Monitor_CmdPlugin.h"
#include "Cntlr_CommPlugin.h"
#include "Monitor_CmdSupport.h"
#include "Cntlr_CommSupport.h"
#endif 


#include "AParser.h"
#include "cntlrStates.h"
#include "flashUpdate.h"
#include "tune.h"
#include "sysUtils.h"

extern int  BrdType;    /* Type of Board, RF, Master, PFG, DDR, Gradient, Etc. */
extern int  BrdNum;     /* The Board types Ordinal number, i.e. rf1 or rf2 */

#define HOSTNAME_SIZE 80
extern char hostName[HOSTNAME_SIZE];

extern NDDS_ID NDDS_Domain;

extern ACODE_ID pTheAcodeObject;

extern MSG_Q_ID pMsgesToAParser;   /* MsgQ used for Msges to Acode Parser */

NDDS_ID pCntlrPub, pCntlrSub;   /* multicast pub/sun to all controllers */

static SEM_ID pCntlrPubMutex = NULL;

/* subscriber to create subscriptions to contoller publications dynamicly */
#ifndef RTI_NDDS_4x
NDDSSubscriber CntlrSubscriber;
#endif

/* subscriptions to controller publications */
#ifndef RTI_NDDS_4x
#define MAX_NEXUS_SUBS 128
NDDS_ID pCntrlSubs[MAX_NEXUS_SUBS];

int numCntrlSubs = 0;
#endif

static SEM_ID pExceptionPubMutex = NULL;
NDDS_ID pExceptionPub, pExceptionSub;   /* multicast pub/sun to all controllers includeing master for errors/warnings */

extern int DebugLevel;

extern MSG_Q_ID  pTuneMsgQ;  /* Message Q to Tune Task */
static int callbackParam = 0;

#ifdef RTI_NDDS_4x
static int masterCallbackParam = 0;
static int cntlrCallbackParam = 1;
#endif

static MSG_Q_ID  pMsgesToFFSUpdate = NULL;   /* MsgQ used for Msges to Flash Update */

int patternType = 0;

union lltw {
     unsigned long long lldur;
     unsigned int  idur[2];
};

/*
 *   The NDDS callback routine, the routine is call when an issue of the subscribed topic
 *   is delivered.
 *   called with the context of the NDDS task n_rtu7400
 *
 */
#ifndef RTI_NDDS_4x

RTIBool Cntlr_CommCallback(const NDDSRecvInfo *issue, NDDSInstance *instance,
                             void *callBackRtnParam)
{
    Cntlr_Comm *recvIssue;
    void MasterNexus(Cntlr_Comm *issue);
    void CntlrPlexus(Cntlr_Comm *issue);
    int *param;
 
    if (issue->status == NDDS_FRESH_DATA)
    {
        recvIssue = (Cntlr_Comm *) instance;
        param = (int *) callBackRtnParam;
        DPRINT5(3,"Cntlr_Comm CallBack:  cmd: %d, arg1: %d, arg2: %d, arg3: %d, crc: 0x%lx\n",
          recvIssue->cmd,recvIssue->arg1, recvIssue->arg2, recvIssue->arg3, recvIssue->crc32chksum);
        if (*param == 0)
        {
	  /* DPRINT(-1,"calling MasterNexus()\n"); */
          MasterNexus(recvIssue);
        }
        else
        {
	  /* DPRINT(-1,"calling CntlrPlexus()\n"); */
          CntlrPlexus(recvIssue);
        }
    }
   return RTI_TRUE;
}

#else /* RTI_NDDS_4x */
void Cntlr_CommCallback(void* listener_data, DDS_DataReader* reader)
{
   Cntlr_Comm *recvIssue;
   struct DDS_SampleInfo* info = NULL;
   struct DDS_SampleInfoSeq info_seq = DDS_SEQUENCE_INITIALIZER;
   DDS_ReturnCode_t retcode;
   DDS_Boolean result;
   long i,numIssues;
   void MasterNexus(Cntlr_Comm *issue);
   void CntlrPlexus(Cntlr_Comm *issue);
   int *param;
   DDS_TopicDescription *topicDesc;

   struct Cntlr_CommSeq data_seq = DDS_SEQUENCE_INITIALIZER;
   Cntlr_CommDataReader *CntlrComm_reader = NULL;

   CntlrComm_reader = Cntlr_CommDataReader_narrow(pCntlrSub->pDReader);
   if ( CntlrComm_reader == NULL)
   {
        errLogRet(LOGIT,debugInfo,"DataReader narrow error\n");
        return;
   }

   topicDesc = DDS_DataReader_get_topicdescription(reader);
   DPRINT2(3,"Cntlr_CommCallback: Type: '%s', Name: '%s'\n",
      DDS_TopicDescription_get_type_name(topicDesc), DDS_TopicDescription_get_name(topicDesc));

   while(1)
   {
        // Given DDS_HANDLE_NIL as a parameter, take_next_instance returns
        // a sequence containing samples from only the next (in a well-determined
        // but unspecified order) un-taken instance.
        retcode =  Cntlr_CommDataReader_take_next_instance(
            CntlrComm_reader,
            &data_seq, &info_seq, DDS_LENGTH_UNLIMITED,
            &DDS_HANDLE_NIL,
            DDS_ANY_SAMPLE_STATE, DDS_ANY_VIEW_STATE, DDS_ANY_INSTANCE_STATE);


        // retcode = Cntlr_CommDataReader_take(CntlrComm_reader,
        //                      &data_seq, &info_seq,
        //                      DDS_LENGTH_UNLIMITED, DDS_ANY_SAMPLE_STATE,
        //                      DDS_ANY_VIEW_STATE, DDS_ANY_INSTANCE_STATE);

        if (retcode == DDS_RETCODE_NO_DATA) {
                 break; // return;
        } else if (retcode != DDS_RETCODE_OK) {
                 errLogRet(LOGIT,debugInfo,"next instance error %d\n",retcode);
                 break; // return;
        }

        numIssues = Cntlr_CommSeq_get_length(&data_seq);
        DPRINT1(+3,"Cntlr_Comm CallBack: numIssues: %d\n",numIssues);

        for (i=0; i < numIssues; i++)
        {
           info = DDS_SampleInfoSeq_get_reference(&info_seq, i);
           if (info->valid_data)
           {

              recvIssue = (Cntlr_Comm *) Cntlr_CommSeq_get_reference(&data_seq,i);

              param = (int *) listener_data;
              // DPRINT6(-5,"-- parm: 0x%lx, %d, master: 0x%lx, 0x%lx, cntlr: 0x%lx, 0x%lx\n",
              //   param, *param, &masterCallbackParam,masterCallbackParam, &cntlrCallbackParam,cntlrCallbackParam);
              DPRINT2(3,"Cntlr_Comm CallBack: Key: %d, Cntlr: '%s'\n",recvIssue->key,recvIssue->cntlrId);
              DPRINT5(3,"Cntlr_Comm CallBack:  cmd: %d, arg1: %d, arg2: %d, arg3: %d, crc: 0x%lx\n",
                recvIssue->cmd,recvIssue->arg1, recvIssue->arg2, recvIssue->arg3, recvIssue->crc32chksum);
             if (*param == 0)
             {
               /* DPRINT(-1,"calling MasterNexus()\n"); */
               MasterNexus(recvIssue);
             }
             else
             {
               /* DPRINT(-1,"calling CntlrPlexus()\n"); */
               CntlrPlexus(recvIssue);
             }
           }
        }
        retcode = Cntlr_CommDataReader_return_loan( CntlrComm_reader,
                  &data_seq, &info_seq);
        DDS_SampleInfoSeq_set_maximum(&info_seq, 0);
   } // while
   return;
}

#endif /* RTI_NDDS_4x */
 
/*
 * Used by Master, to make it usage clear
 * just initial routine to send cmds, not complete yet
 * not all structure member filled in
 *
 */
#ifndef RTI_NDDS_4x
cntlrCommPub(int cmd,int arg1,int arg2,int arg3,char *strmsg)
{
   Cntlr_Comm *issue;
   issue = pCntlrPub->instance;
   semTake(pCntlrPubMutex, WAIT_FOREVER);
   DPRINT(+3,"cntlrCommPub: got Mutex\n");
   issue->cmd  = cmd;  /* atoi( token ); */
   issue->arg1 = arg1;
   issue->arg2 = arg2;
   issue->arg3 = arg3;
   /* issue->errorcode */
   /* issue->warningcode */
   /* issue-> crc32chksum */

   if (strmsg != NULL)
   {
     int strsize = strlen(strmsg);
     if (strsize <= COMM_MAX_STR_SIZE)
       strncpy(issue->msgstr,strmsg,COMM_MAX_STR_SIZE);
     else
     {
       DPRINT2(-1,"msg to long: %d, max: %d\n",strsize,COMM_MAX_STR_SIZE);
     }
   }
   nddsPublishData(pCntlrPub);
   DPRINT(+3,"cntlrCommPub: give Mutex\n");
   semGive(pCntlrPubMutex);
   return 0;
}

#else /* RTI_NDDS_4x */
/* the RTI NDDS 4x Varient */
cntlrCommPub(int cmd,int arg1,int arg2,int arg3,char *strmsg)
{
   DDS_ReturnCode_t result;
   DDS_InstanceHandle_t instance_handle = DDS_HANDLE_NIL;
   Cntlr_CommDataWriter *CntlrCmd_writer = NULL;
   Cntlr_Comm *issue;

   issue = pCntlrPub->instance;
   semTake(pCntlrPubMutex, WAIT_FOREVER);
   DPRINT(+3,"cntlrCommPub: got Mutex\n");
   issue->cmd  = cmd;  /* atoi( token ); */
   issue->arg1 = arg1;
   issue->arg2 = arg2;
   issue->arg3 = arg3;
   /* issue->errorcode */
   /* issue->warningcode */
   /* issue-> crc32chksum */

   if (strmsg != NULL)
     {
       int strsize = strlen(strmsg);
       if (strsize <= COMM_MAX_STR_SIZE)
	 strncpy(issue->msgstr,strmsg,COMM_MAX_STR_SIZE);
       else
         {
           errLogRet(LOGIT,debugInfo,"msg to long: %d, max: %d \n",strsize,COMM_MAX_STR_SIZE);
         }
     }
   CntlrCmd_writer = Cntlr_CommDataWriter_narrow(pCntlrPub->pDWriter);
   if (CntlrCmd_writer == NULL) {
     errLogRet(LOGIT,debugInfo,"DataWriter narrow error\n");
     semGive(pCntlrPubMutex);
     return -1;
   }

   // instance_handle = pCntlrPub->issueRegHandle;
   result = Cntlr_CommDataWriter_write(CntlrCmd_writer,
				       issue,&instance_handle);
   if (result != DDS_RETCODE_OK) {
     errLogRet(LOGIT,debugInfo,"DataWriter write error: %d\n",result);
   }
   DPRINT(+3,"cntlrCommPub: give Mutex\n");
   semGive(pCntlrPubMutex);
   return 0;
}

#endif /* RTI_NDDS_4x */
 
wait4Pub2BSent()
{
   nddsPublicationIssuesWait(pCntlrPub, 10, 0);
}

/*
 * In reality these call the same routine, the difference being that
 * the publications built have different 'topic' names between the master
 * and all other controllers.
 * These functions are just provided so that the intent is clear.
 *
 *    Author: Greg Brissey   9/20/04
 */
send2Master(int cmd,int arg1,int arg2,int arg3,char *strmsg)
{
    DPRINT(+3,"send2Master\n");
    cntlrCommPub(cmd,arg1,arg2,arg3,strmsg);
    return 0;
}

send2MasterTicks(long long ticks)
{
    union lltw TEMP;
    TEMP.lldur = ticks;
    DPRINT(+3,"send2MasterTicks\n");
    cntlrCommPub(CNTLR_CMD_SET_FIFOTICK,TEMP.idur[0],TEMP.idur[1],0,NULL);
    return 0;
}

send2AllCntlrs(int cmd, int arg1,int arg2,int arg3,char *strmsg)
{
    DPRINT(+3,"send2AllCntlrs\n");
    cntlrCommPub(cmd,arg1,arg2,arg3,strmsg);
    return 0;
}

/*
 * The Master Nexus, this is where all commnunication from the 'other'
 * controllers come. 
 * Typical usage will be for controllers reporting in on status, errors or warnings,
 * or possible command for the master hardware (serialports [VT,Shims,etc.])
 *
 *   Author: Greg Brissey 4/27/04
 */
void MasterNexus(Cntlr_Comm *issue)
{
   int len,errorcode;
   int cmd;
   Cntlr_Comm  cmdIssue;
   DPRINT4(+2,"MasterPlexus cmd: %d, arg1: %d, arg2: %d, arg3: %d\n", issue->cmd,
		issue->arg1,issue->arg2,issue->arg3);

   switch( issue->cmd )
   {
     case CNTLR_CMD_HERE:
	  {
   	        /* DPRINT(-1,"HERE\n"); */
#ifdef INSTRUMENT
	        wvEvent(EVENT_NEXUS_RCALLREPLY,NULL,NULL);
#endif
		DPRINT2(-1,"Cntlr: '%s' Acknowledged ROLLCALL, with: '%s'.\n",
			issue->cntlrId,getCntlrStateStr(issue->arg1));
		DPRINT2(-1,"Cntlr: '%s', ConfigInfo: %d\n",issue->cntlrId,issue->arg2);
		errorcode = cntlrStatesAdd(issue->cntlrId,issue->arg1,issue->arg2);
		/* -1 == controller already in list, 0 = controller added */
		DPRINT2(+1,"Cntlr: '%s': %s in Cntlr List\n", issue->cntlrId, 
			((errorcode == -1) ? "Already Present" : "Added"));
#ifdef INSTRUMENT
		wvEvent(EVENT_NEXUS_RCALLREPLY_CMPLT,NULL,NULL);
#endif
	  }
	  break;

     case CNTLR_CMD_READY4SYNC:
	  {
#ifdef INSTRUMENT
	wvEvent(EVENT_NEXUS_READY4SYNC,NULL,NULL);
#endif
   	        /* DPRINT1(-1,"Cntlr: '%s', reporting READY FOR SYNC\n",issue->cntlrId); */
   	        DPRINT2(-1,"Cntlr: '%s', Reported: '%s'.\n",
				issue->cntlrId,getCntlrStateStr(issue->arg1));
#ifdef TIMING_DIAG_ON
   {
      double delta,duration;
      double getTimeStampDurations(double *delta,double *duration);
      updateTimeStamp(0);
      getTimeStampDurations(&delta,&duration);
      DPRINT3(-19,"'%s': reporting READY4SYNC: delta: %lf usec, duration: %lf usec\n",issue->cntlrId,delta,duration);
      updateTimeStamp(0); /* remove time use to perform this printing for next TSPRINT */ \
   }
#endif 
                errorcode = cntlrSetState(issue->cntlrId,issue->arg1, 0 /* errorcode */ );
#ifdef INSTRUMENT
	wvEvent(EVENT_NEXUS_READY4SYNC_CMPLT,NULL,NULL);
#endif
	  }
	  break;

     case CNTLR_CMD_STATE_UPDATE:
	  {
#ifdef INSTRUMENT
	wvEvent(EVENT_NEXUS_STATEUPDATE,NULL,NULL);
#endif
        DPRINT3(-1,"Cntlr: '%s', reported Updated State: '%s', errorcode: %d.\n",
                           issue->cntlrId,getCntlrStateStr(issue->arg1),issue->arg2);
#ifdef TIMING_DIAG_ON
   {
      double delta,duration;
      double getTimeStampDurations(double *delta,double *duration);
      updateTimeStamp(0);
      getTimeStampDurations(&delta,&duration);
      DPRINT4(-19,"'%s': StateUpdate2: '%s', delta: %lf usec, duration: %lf usec\n",
            issue->cntlrId,getCntlrStateStr(issue->arg1),delta,duration);
      updateTimeStamp(0); /* remove time use to perform this printing for next TSPRINT */ \
   }
#endif 

       errorcode = cntlrSetState(issue->cntlrId,issue->arg1, issue->arg2 /* errorcode */ );

       /* cntlrStateShow(); */
#ifdef INSTRUMENT
	    wvEvent(EVENT_NEXUS_STATEUPDATE_CMPLT,NULL,NULL);
#endif
	  }
	  break;

          
     case CNTLR_CMD_SET_ACQSTATE:
	  {
#ifdef INSTRUMENT
	wvEvent(EVENT_NEXUS_ACQSTATE,NULL,NULL);
#endif
   	        DPRINT2(-1,"Cntlr: '%s', set Acqstate: %d\n",
                           issue->cntlrId,issue->arg1);
                     /* issue->cntlrId,getAcqStateStr(issue->arg1)); */
                setAcqState(issue->arg1);
#ifdef INSTRUMENT
	wvEvent(EVENT_NEXUS_ACQSTATE_CMPLT,NULL,NULL);
#endif
	  }
	  break;

     case CNTLR_CMD_SET_FIFOTICK:
	  {
            union lltw TEMP;
            TEMP.idur[0] = issue->arg1;  /* FifoCumDurationHi; */
            TEMP.idur[1] = issue->arg2;   /* pFifoCumDurationLow; */
#ifdef INSTRUMENT
	wvEvent(EVENT_NEXUS_STATEUPDATE,NULL,NULL);
#endif
   	        DPRINT3(+1,"Cntlr: '%s', reported Fifo Ticks: 0x%llx, %lld\n",
                           issue->cntlrId,TEMP.lldur,TEMP.lldur);
                errorcode = cntlrSetFifoTicks(issue->cntlrId,TEMP.lldur);
                /* cntlrStateShow(); */
#ifdef INSTRUMENT
	wvEvent(EVENT_NEXUS_STATEUPDATE_CMPLT,NULL,NULL);
#endif
	  }
	  break;

          
/* was going to put autolock on lock controller, but many things are controlled
   by master and it just makes it simpler at this point to put it on the master

        GMB    1/6/05
*/
#ifdef AUTOLOCK_ON_LOCKCNTLR

     case CNTLR_CMD_SET_SHIMDAC:
	  {
                int zero;
                int oneDac[4];
   	        DPRINT3(-1,"Cntlr: '%s', set Shim DAC#: %d, Value: %d\n",
                           issue->cntlrId,issue->arg1,issue->arg2);
		zero=0;
	        oneDac[0] = 13;
	        oneDac[1] = issue->arg1;    /* Z0 = 1 */
	        oneDac[2] = issue->arg2;
                /* if put in then all controller need a dummy shimHandler() func */
		// shimHandler(oneDac,&zero,3,0);   // count=1, fifoFlag=false
	  }
	  break;

#endif

     default:
	    DPRINT1(-5,"MasterNexus: Unknown command: %d\n",issue->cmd);
            break;

   }
   return;
}

/*********************************************************************
*
*  The Controller Communication Plexus with the Master controller 
*  The 'Other' (not master)  controllers get their message decoded here.
*  Typically the master send commands here, however other controllers
*  might send commands to one another, thus not going through the master.
*
*					Author Greg Brissey 4-26-04
*/
void CntlrPlexus(Cntlr_Comm *issue)
{
   int len;
   int cmd;
   Cntlr_Comm  cmdIssue;

   DPRINT4(+2,"CntlrPlexus cmd: %d, arg1: %d, arg2: %d, arg3: %d\n", issue->cmd,
		issue->arg1,issue->arg2,issue->arg3);

   switch( issue->cmd )
   {
     case CNTLR_CMD_ROLLCALL:
	  {
          int result;
          // RF_CONFIG_STD = 0 - default for all controllers config
          int ConfigInfo = RF_CONFIG_STD; 
           
	  DPRINT(-1,"CntlrPlexus(): ROLLCALL\n");
          #ifdef SILKWORM
          if (BrdType == RF_BRD_TYPE_ID)
          {
             result = execFunc("getRfType",NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
   	       DPRINT1(-1,"CntlrPlexus(): ROLLCALL, getRfTpe result: %d\n",result);
             if ( result == 0 )
                ConfigInfo = RF_CONFIG_STD;
             else if( result == 1 )
                ConfigInfo = RF_CONFIG_ICAT;
             else
                ConfigInfo =  RF_CONFIG_UNSET;
          }
          #endif
	  if (BrdType == LPFG_BRD_TYPE_ID)
	    ConfigInfo = PFG_CONFIG_LOCK;

	  send2Master(CNTLR_CMD_HERE,CNTLR_READYnIDLE,ConfigInfo,0,NULL); /* NO_WAIT, MSG_PRI_NORMAL); */
	  }
	  break;

     case CNTLR_CMD_SET_DEBUGLEVEL:
          DebugLevel = issue->arg1;
          DPRINT1(-1,"CntlrPlexus(): SET_DEBUGLEVEL: DebugLevel = %d\n",DebugLevel);
	  break;

     case CNTLR_CMD_REBOOT:
          DPRINT(-1,"CntlrPlexus(): ReBooting\n");
          reboot();
	  break;

     case CNTLR_CMD_APARSER:
          {
	    PARSER_MSG pCmd;
            if (BrdType != LOCK_BRD_TYPE_ID)
            {

#ifdef INSTRUMENT
	       wvEvent(EVENT_NEXUS_APARSE,NULL,NULL);
#endif
   	       DPRINT5(-1,"CntlrPlexus(): APARSER: cmd: %d, Acodes: %d, Tables: %d, start FId: %d, Basename: '%s'\n",
		       issue->cmd,issue->arg1,issue->arg2,issue->arg3,issue->msgstr);
               pCmd.parser_cmd = issue->cmd;
	       pCmd.NumAcodes = issue->arg1;
	       pCmd.NumTables = issue->arg2;
	       pCmd.startFID = issue->arg3;
	       strncpy(pCmd.AcqBaseBufName,issue->msgstr, EXP_BASE_NAME_SIZE);
     	       msgQSend(pMsgesToAParser,(char*) &pCmd, sizeof(PARSER_MSG), NO_WAIT, MSG_PRI_NORMAL);
#ifdef INSTRUMENT
	       wvEvent(EVENT_NEXUS_APARSE_CMPLT,NULL,NULL);
#endif
            }
          }
	  break;

     case CNTLR_CMD_FFS_UPDATE:
          {
            FLASH_UPDATE_MSG  ffsMsge;
            MSG_Q_ID startFlashUpdate(int taskpriority,int taskoptions,int stacksize);
#ifdef INSTRUMENT
	wvEvent(EVENT_NEXUS_FFSUPDATE_CMPLT,NULL,NULL);
#endif
            if (pMsgesToFFSUpdate == NULL)
            {
		DPRINT(-1,"Start FlashFS Update Task.\n");
                pMsgesToFFSUpdate = startFlashUpdate(FFSUPDATE_TASK_PRIORITY,STD_TASKOPTIONS,256*1024);
            }
            DPRINT(-1,"========>> FLASH_UPDATE: Initiate FLASH Update\n");
            DPRINT3(-1,"cmd: %d, filesize: %ld, msge: '%s'\n",issue->cmd,issue->arg1,issue->msgstr);
            ffsMsge.cmd = issue->cmd;
            ffsMsge.filesize = issue->arg1;   /* file size */
            ffsMsge.arg2 = issue->arg3;	      /* defered update flag */
            ffsMsge.arg3 = 0;
            ffsMsge.crc32chksum = issue->crc32chksum;
            strncpy(ffsMsge.msgstr,issue->msgstr,MAX_FLASH_UPDATE_MSG_STR);

            DPRINT3(-1,"cmd: %d, filesize: %ld, msge: '%s'\n",ffsMsge.cmd,ffsMsge.filesize,ffsMsge.msgstr);

     	    msgQSend(pMsgesToFFSUpdate,(char*) &ffsMsge, sizeof(FLASH_UPDATE_MSG), WAIT_FOREVER, MSG_PRI_NORMAL);
#ifdef INSTRUMENT
	wvEvent(EVENT_NEXUS_FFSUPDATE_CMPLT,NULL,NULL);
#endif
          }
	  break;

     case CNTLR_CMD_FFS_COMMIT:
          {
            FLASH_UPDATE_MSG  ffsMsge;
            MSG_Q_ID startFlashUpdate(int taskpriority,int taskoptions,int stacksize);
#ifdef INSTRUMENT
	wvEvent(EVENT_NEXUS_FFSCOMMIT,NULL,NULL);
#endif
            if (pMsgesToFFSUpdate == NULL)
            {
		DPRINT(-1,"Start FlashFS Update Task.\n");
                pMsgesToFFSUpdate =  startFlashUpdate(70,STD_TASKOPTIONS,256*1024);
            }
            DPRINT(-1,"========>> FLASH_UPDATE: Initiate FLASH Commit\n");
            ffsMsge.cmd = issue->cmd;
            ffsMsge.filesize = issue->arg1;   /* file size */
            ffsMsge.arg2 = issue->arg3;	      /* defered update flag */
            ffsMsge.arg3 = 0;
            ffsMsge.crc32chksum = issue->crc32chksum;
            strncpy(ffsMsge.msgstr,issue->msgstr,MAX_FLASH_UPDATE_MSG_STR);

            DPRINT3(-1,"cmd: %d, filesize: %ld, msge: '%s'\n",ffsMsge.cmd,ffsMsge.filesize,ffsMsge.msgstr);
     	    msgQSend(pMsgesToFFSUpdate,(char*) &ffsMsge, sizeof(FLASH_UPDATE_MSG), WAIT_FOREVER, MSG_PRI_NORMAL);
#ifdef INSTRUMENT
	wvEvent(EVENT_NEXUS_FFSCOMMIT_CMPLT,NULL,NULL);
#endif
          }
	  break;


     case CNTLR_CMD_FFS_DELETE:
          {
            FLASH_UPDATE_MSG  ffsMsge;
            MSG_Q_ID startFlashUpdate(int taskpriority,int taskoptions,int stacksize);
            if (pMsgesToFFSUpdate == NULL)
            {
		DPRINT(-1,"Start FlashFS Update Task.\n");
                pMsgesToFFSUpdate =  startFlashUpdate(70,STD_TASKOPTIONS,256*1024);
            }
            DPRINT(-1,"========>> FLASH_DELETE: Initiate FLASH Delete\n");
            ffsMsge.cmd = issue->cmd;
            ffsMsge.filesize = 0;   /* file size */
            ffsMsge.arg2 = 0;
            ffsMsge.arg3 = 0;
            ffsMsge.crc32chksum = issue->crc32chksum;
            strncpy(ffsMsge.msgstr,issue->msgstr,MAX_FLASH_UPDATE_MSG_STR);

            DPRINT3(-1,"cmd: %d, filesize: %ld, msge: '%s'\n",ffsMsge.cmd,ffsMsge.filesize,ffsMsge.msgstr);
     	    msgQSend(pMsgesToFFSUpdate,(char*) &ffsMsge, sizeof(FLASH_UPDATE_MSG), WAIT_FOREVER, MSG_PRI_NORMAL);
          }
          break;

     case CNTLR_CMD_TUNE_QUIET:
     case CNTLR_CMD_TUNE_ENABLE:
     case CNTLR_CMD_TUNE_FINI:
          {
            TUNE_MSG  tuneMsge;
            DPRINT( 1,"========>> TUNE CMD  <<========== \n");
            tuneMsge.cmd = issue->cmd;
            tuneMsge.channel = issue->arg1;   /* file size */
            tuneMsge.arg2 = issue->arg2;
            tuneMsge.arg3 = issue->arg3;
            tuneMsge.crc32chksum = issue->crc32chksum;
            strncpy(tuneMsge.msgstr,issue->msgstr,MAX_FLASH_UPDATE_MSG_STR);
            DPRINT4( 1,"cmd: %d, channel: %ld, atten: %d, msge: '%s'\n",
                       tuneMsge.cmd,tuneMsge.channel,tuneMsge.arg2,tuneMsge.msgstr);
            if (pTuneMsgQ != NULL)
     	          msgQSend(pTuneMsgQ,(char*) &tuneMsge, sizeof(TUNE_MSG), WAIT_FOREVER, MSG_PRI_NORMAL);
          }
          break;
#ifdef NO_DIFF_CASES
     case CNTLR_CMD_TUNE_ENABLE:
          {
            TUNE_MSG  tuneMsge;
          }
          break;
     case CNTLR_CMD_TUNE_FINI:
          {
            TUNE_MSG  tuneMsge;
          }
          break;
#endif
     case CNTLR_RTVAR_UPDATE:
          { 
            if (BrdType != LOCK_BRD_TYPE_ID)
            {
               int *rtVar =  (int *)pTheAcodeObject->pLcStruct;
#ifdef INSTRUMENT
	       wvEvent(EVENT_NEXUS_RTVARUPDATE,NULL,NULL);
#endif
	       DPRINT2( 1,"CNTLR_RTVAR_UPDATE: index=%d value=%d\n",
				issue->arg1,issue->arg2);
               rtVar[issue->arg1] = issue->arg2;
               semGive(pTheAcodeObject->pSemParseSuspend);
#ifdef INSTRUMENT
	       wvEvent(EVENT_NEXUS_RTVARUPDATE_CMPLT,NULL,NULL);
#endif
            }
          }
          break;

     default:
	    DPRINT1(-5,"CntlrPlexus: Unknown command: %d\n",issue->cmd);
            break;

   }
   return;
}

/*
 * Create a Publication Topic to communicate with the Cntrollers/Master
 *
 *					Author Greg Brissey 4-26-04
 */
NDDS_ID createCntlrCommPub(NDDS_ID nddsId, char *topic, const char *cntlrName)
{
    int result;
    NDDS_ID pPubObj;
    char pubtopic[128];
    Cntlr_Comm  *issue;

    /* Build Data type Object for both publication and subscription to Expproc */
    /* ------- malloc space for data type object --------- */
    if ( (pPubObj = (NDDS_ID) malloc( sizeof(NDDS_OBJ)) ) == NULL )
        return(NULL);

    /* create the pub issue  Mutual Exclusion semaphore */
    if (pCntlrPubMutex == NULL)
	pCntlrPubMutex = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE |
				    SEM_DELETE_SAFE);

    /* zero out structure */
    memset(pPubObj,0,sizeof(NDDS_OBJ));
    memcpy(pPubObj,nddsId,sizeof(NDDS_OBJ));

    strcpy(pPubObj->topicName,topic);
    pPubObj->pubThreadId = 42; /* DEFAULT_PUB_THREADID; taskIdSelf(); */
         
    /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */
    getCntlr_CommInfo(pPubObj);
         
    DPRINT2(+1,"Create Pub topic: '%s' for Cntlr: '%s'\n",pPubObj->topicName,cntlrName);

#ifdef RTI_NDDS_4x
  /*
  #ifdef PFG_LOCK_COMBO_CNTLR // has more readers/writers than single-function controllers
    pPubObj->queueSize  = 4;
    pPubObj->AckRequestsPerSendQueue = 4;
    pPubObj->highWaterMark = 2;
  #else
  */
    pPubObj->queueSize  = 2;
    pPubObj->AckRequestsPerSendQueue = 2;
    pPubObj->highWaterMark = 1;
  //#endif
    pPubObj->lowWaterMark =  0;
    initPublication(pPubObj);
#endif /* RTI_NDDS_4x */
    createPublication(pPubObj);
    issue = (Cntlr_Comm  *) pPubObj->instance;
    strcpy(issue->cntlrId,cntlrName);   /* fill in the constant cntlrId string */
    return pPubObj;
}        

/*
 * Create a Subscription Topic to communicate with the Controllers/Master
 *
 *					Author Greg Brissey 4-26-04
 */
NDDS_ID createCntlrCommSub(NDDS_ID nddsId, char *topic, char *multicastIP, void *callbackArg)
{

   NDDS_ID  pSubObj;
   char subtopic[128];

    /* Build Data type Object for both publication and subscription to Expproc */
    /* ------- malloc space for data type object --------- */
    if ( (pSubObj = (NDDS_ID) malloc( sizeof(NDDS_OBJ)) ) == NULL )
        return NULL;
 
    /* zero out structure */
    memset(pSubObj,0,sizeof(NDDS_OBJ));
    memcpy(pSubObj,nddsId,sizeof(NDDS_OBJ));
 
    strcpy(pSubObj->topicName,topic);
 
    /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */
    getCntlr_CommInfo(pSubObj);
 
#ifndef RTI_NDDS_4x
    pSubObj->callBkRtn = Cntlr_CommCallback;
    pSubObj->callBkRtnParam = callbackArg;
#endif /* RTI_NDDS_4x */
 
    if (multicastIP == NULL)
       pSubObj->MulticastSubIP[0] = 0;   /* use UNICAST */
    else
       strcpy(pSubObj->MulticastSubIP,multicastIP);

#ifdef RTI_NDDS_4x
    initSubscription(pSubObj);
    attachOnDataAvailableCallback(pSubObj,Cntlr_CommCallback,callbackArg /* user data */ );
#endif /* RTI_NDDS_4x */
    createSubscription(pSubObj);
    return pSubObj;
}


#ifdef  RTI_NDDS_4x
/* Register an instance based on key given */
int Register_CntlrCommkeyed_instance(NDDS_ID pNDDS_Obj,int key)
{
    // DDS_InstanceHandle_t regHandle;
    Cntlr_Comm *pIssue = NULL;
    Cntlr_CommDataWriter *CntlrCommWriter = NULL;

    CntlrCommWriter = Cntlr_CommDataWriter_narrow(pNDDS_Obj->pDWriter);
    if (CntlrCommWriter == NULL) {
        errLogRet(LOGIT,debugInfo, "Register_CntlrCommkeyed_instance: DataReader narrow error.\n");
        return -1;
    }

    pIssue = pNDDS_Obj->instance;

    pIssue->key = key;

    // for Keyed Topics must register this keyed topic
    pNDDS_Obj->issueRegHandle = Cntlr_CommDataWriter_register_instance(CntlrCommWriter, pIssue);

    return 0;
}
#endif  /*  RTI_NDDS_4x */


/*
 * The Master via NDDS uses this callback function to create Subscriptions to the
 * Controller's Publications aimed at the Master Nexus
 *
 *					Author Greg Brissey 4-26-04
 */
#ifndef RTI_NDDS_4x
NDDSSubscription Cntlr_CommPatternSubCreate( const char *nddsTopic, const char *nddsType, 
                  void *callBackRtnParam) 
{ 
     NDDSSubscription pSub;

     DPRINT4(+1,"Cntlr_CommPatternSubCreate(): pCntrlSubs[%d]: Topic: '%s', Type: '%s', arg: 0x%lx\n",numCntrlSubs,
		nddsTopic, nddsType, callBackRtnParam);
     DPRINT2(+1,"callbackParam: 0x%lx, callBackRtnParam: 0x%lx\n",&callbackParam,callBackRtnParam);
     if (numCntrlSubs >= MAX_NEXUS_SUBS)
     {
        NDDS_ID pCntrlSub;
        errLogRet(LOGIT,debugInfo,"Number of controller subscriptions to master nexus has exceeded max: %d\n",MAX_NEXUS_SUBS);
	/* go ahead and make subscription, at this point we don't actually use the pCntrlSubs[] array for anything */
        pCntrlSub = createCntlrCommSub(NDDS_Domain, (char*) nddsTopic, NULL, (void *) &callbackParam );
        pSub = pCntrlSub->subscription;
        return pSub;
     }
     pCntrlSubs[numCntrlSubs]  = createCntlrCommSub(NDDS_Domain, (char*) nddsTopic, NULL, (void *) &callbackParam );
     pSub = pCntrlSubs[numCntrlSubs++]->subscription;
     return pSub;
}

/*
 *  Master creates a pattern subscriber, to dynamicly allow subscription creation
 *  as controllers come on-line and publusion to the Masters Nexus 
 *
 *					Author Greg Brissey 4-26-04
 */
cntlrPubPatternSub()
{
    /* MASTER_SUB_COMM_PATTERN_TOPIC_STR */
    CntlrSubscriber = NddsSubscriberCreate(0);

    /* master subscribe to any publications from controllers */
    NddsSubscriberPatternAdd(CntlrSubscriber,  
           "*/master/cmd",  Cntlr_CommNDDSType , Cntlr_CommPatternSubCreate, (void *)callbackParam); 
}

#endif /* RTI_NDDS_4x */
 

initialMasterComm()
{
    InitCntlrStates();
    cntlrStatesAdd(hostName,CNTLR_READYnIDLE,MASTER_CONFIG_STD);
#ifndef RTI_NDDS_4x
    pCntlrPub = createCntlrCommPub(NDDS_Domain,MASTER_PUB_COMM_TOPIC_FORMAT_STR, "master1" );
    cntlrPubPatternSub();
#else
    long key;
    pCntlrPub = createCntlrCommPub(NDDS_Domain,MASTER_PUB_COMM_TOPIC_FORMAT_STR, "master1" );
    key = (BrdType << 8) | BrdNum;   /* key needs to be unique per board send to master */
    Register_CntlrCommkeyed_instance(pCntlrPub,key);
    /* in 4x all cntlrs publish on one topic to master (m21 - many two one) */
    pCntlrSub = createCntlrCommSub(NDDS_Domain,MASTER_SUB_COMM_M21TOPIC_FORMAT_STR,NULL, (void *) &masterCallbackParam );
#endif  /* RTI_NDDS_4x */
}

initialCntlrComm(const char* cntlrId, long brdType, long brdNum)
{
#ifndef RTI_NDDS_4x
    char topicStr[128];
    sprintf(topicStr,CNTLR_PUB_COMM_TOPIC_FORMAT_STR,cntlrId);
    pCntlrPub = createCntlrCommPub(NDDS_Domain,topicStr, cntlrId);
    callbackParam = 1;
    pCntlrSub = createCntlrCommSub(NDDS_Domain,CNTLR_SUB_COMM_TOPIC_FORMAT_STR, CNTLR_COMM_MULTICAST_IP, 
				   (void *) &callbackParam);
#else
    long key;
    /* in 4x all the cntlr use the same topic to publish to master, an instance key is used */
    pCntlrPub = createCntlrCommPub(NDDS_Domain,CNTLR_PUB_COMM_M21TOPIC_FORMAT_STR, cntlrId );
    key = (brdType << 8) | brdNum;   /* key needs to be unique per board send to master */
    Register_CntlrCommkeyed_instance(pCntlrPub,key);
    pCntlrSub = createCntlrCommSub(NDDS_Domain,CNTLR_SUB_COMM_TOPIC_FORMAT_STR, CNTLR_COMM_MULTICAST_IP, 
				   (void *) &cntlrCallbackParam);
    callbackParam = 1;
#endif  /* RTI_NDDS_4x */
}

/* =================================================================================== */
/* =================================================================================== */
/* ++++++++++++++++++++++++++  Exception Pub/Sun Routines +++++++++++++++++++++++++++++ */
/* =================================================================================== */
/* =================================================================================== */


typedef void (*PVF_MSG) (Cntlr_Comm *issue);

/*
 *   The NDDS callback routine, the routine is call when an issue of the subscribed topic
 *   is delivered.
 *   called with the context of the NDDS task n_rtu7400
 *
 */
#ifndef RTI_NDDS_4x
RTIBool Exception_CommCallback(const NDDSRecvInfo *issue, NDDSInstance *instance,
                             void *callBackRtnParam)
{
    Cntlr_Comm *recvIssue;
    PVF_MSG callbackFunc;
 
    if (issue->status == NDDS_FRESH_DATA)
    {
#ifdef INSTRUMENT
	wvEvent(EVENT_NEXUS_RECVEXCPT,NULL,NULL);
#endif
        recvIssue = (Cntlr_Comm *) instance;
        callbackFunc = (PVF_MSG) callBackRtnParam;
        DPRINT5(+1,"Exception_Comm CallBack:  cmd: %d, arg1: %d, arg2: %d, arg3: %d, crc: 0x%lx\n",
        recvIssue->cmd,recvIssue->arg1, recvIssue->arg2, recvIssue->arg3, recvIssue->crc32chksum);
        (*callbackFunc)(recvIssue);
#ifdef INSTRUMENT
	wvEvent(EVENT_NEXUS_RECVEXCPT_CMPLT,NULL,NULL);
#endif
    }
   return RTI_TRUE;
}

#else /* RTI_NDDS_4x */

void Exception_CommCallback(void* listener_data, DDS_DataReader* reader)
{
   Cntlr_Comm *recvIssue;
   struct DDS_SampleInfo* info = NULL;
   struct DDS_SampleInfoSeq info_seq = DDS_SEQUENCE_INITIALIZER;
   DDS_ReturnCode_t retcode;
   DDS_Boolean result;
   long i,numIssues;
   void MasterNexus(Cntlr_Comm *issue);
   void CntlrPlexus(Cntlr_Comm *issue);
   int *param;
   PVF_MSG callbackFunc;
   DDS_TopicDescription *topicDesc;

   struct Cntlr_CommSeq data_seq = DDS_SEQUENCE_INITIALIZER;
   Cntlr_CommDataReader *CntlrComm_reader = NULL;

   CntlrComm_reader = Cntlr_CommDataReader_narrow(pExceptionSub->pDReader);
   if ( CntlrComm_reader == NULL)
   {
        errLogRet(LOGIT,debugInfo,"DataReader narrow error\n");
        return;
   }

   topicDesc = DDS_DataReader_get_topicdescription(reader);
   DPRINT2(+1,"Exception_CommCallback: Type: '%s', Name: '%s'\n",
      DDS_TopicDescription_get_type_name(topicDesc), DDS_TopicDescription_get_name(topicDesc));

   while(1)
   {

        // Given DDS_HANDLE_NIL as a parameter, take_next_instance returns
        // a sequence containing samples from only the next (in a well-determined
        // but unspecified order) un-taken instance.
        retcode =  Cntlr_CommDataReader_take_next_instance(
             CntlrComm_reader,
             &data_seq, &info_seq, DDS_LENGTH_UNLIMITED,
             &DDS_HANDLE_NIL,
             DDS_ANY_SAMPLE_STATE, DDS_ANY_VIEW_STATE, DDS_ANY_INSTANCE_STATE);

        // retcode = Cntlr_CommDataReader_take(CntlrComm_reader,
        //                       &data_seq, &info_seq,
        //                       DDS_LENGTH_UNLIMITED, DDS_ANY_SAMPLE_STATE,
        //                       DDS_ANY_VIEW_STATE, DDS_ANY_INSTANCE_STATE);

        if (retcode == DDS_RETCODE_NO_DATA) {
                 return; // break; // return;
        } else if (retcode != DDS_RETCODE_OK) {
                 errLogRet(LOGIT,debugInfo,"next instance error %d\n",retcode);
                 return; // break; // return;
        }

        numIssues = Cntlr_CommSeq_get_length(&data_seq);
        DPRINT1(+1,"Exception Cntlr_Comm CallBack: numIssues: %d\n",numIssues);

        for (i=0; i < numIssues; i++)
        {
           info = DDS_SampleInfoSeq_get_reference(&info_seq, i);
           if (info->valid_data)
           {
#ifdef INSTRUMENT
	      wvEvent(EVENT_NEXUS_RECVEXCPT,NULL,NULL);
#endif

              recvIssue = (Cntlr_Comm *) Cntlr_CommSeq_get_reference(&data_seq,i);
              callbackFunc = (PVF_MSG) listener_data;
              DPRINT2(+1,"Exception_Comm CallBack: Key: %d, Cntlr: '%s'\n",recvIssue->key,recvIssue->cntlrId);
              DPRINT5(+1,"Exception_Comm CallBack:  cmd: %d, arg1: %d, arg2: %d, arg3: %d, crc: 0x%lx\n",
                    recvIssue->cmd,recvIssue->arg1, recvIssue->arg2, recvIssue->arg3, recvIssue->crc32chksum);
              (*callbackFunc)(recvIssue);   // call function

#ifdef INSTRUMENT
	     wvEvent(EVENT_NEXUS_RECVEXCPT_CMPLT,NULL,NULL);
#endif
           }
        }
        retcode = Cntlr_CommDataReader_return_loan( CntlrComm_reader,
                  &data_seq, &info_seq);
        DDS_SampleInfoSeq_set_maximum(&info_seq, 0);
   } // while
   return;
}
#endif /* RTI_NDDS_4x */
 

/*
 * Create a Exception Publication to communicate with the Cntrollers/Master
 *
 *					Author Greg Brissey 4-26-04
 */
NDDS_ID createExceptionCommPub(NDDS_ID nddsId, char *topic, char *cntlrName)
{
    int result;
    NDDS_ID pPubObj;
    char pubtopic[128];
    Cntlr_Comm  *issue;

    /* Build Data type Object for both publication and subscription to Expproc */
    /* ------- malloc space for data type object --------- */
    if ( (pPubObj = (NDDS_ID) malloc( sizeof(NDDS_OBJ)) ) == NULL )
        return NULL;

    /* create the pub issue  Mutual Exclusion semaphore */
    pExceptionPubMutex = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE |
				    SEM_DELETE_SAFE);

    /* zero out structure */
    memset(pPubObj,0,sizeof(NDDS_OBJ));
    memcpy(pPubObj,nddsId,sizeof(NDDS_OBJ));

    strcpy(pPubObj->topicName,topic);
    pPubObj->pubThreadId = 5;  /* DEFAULT_PUB_THREADID; taskIdSelf(); */
         
    /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */
    getCntlr_CommInfo(pPubObj);
         
    DPRINT2(-1,"createExceptionCommPub: '%s' for Cntlr: '%s'\n",pPubObj->topicName,cntlrName);

#ifdef RTI_NDDS_4x
  #ifdef PFG_LOCK_COMBO_CNTLR // has more readers/writers than single-function controllers
     pPubObj->queueSize  = 4;
     pPubObj->highWaterMark = 2;
     pPubObj->AckRequestsPerSendQueue = 4;
  #else
     pPubObj->queueSize  = 2;
     pPubObj->highWaterMark = 1;
     pPubObj->AckRequestsPerSendQueue = 2;
  #endif
     pPubObj->lowWaterMark =  0;
     initPublication(pPubObj);
#endif /* RTI_NDDS_4x */
 
    createPublication(pPubObj);
    issue = (Cntlr_Comm  *) pPubObj->instance;
    strcpy(issue->cntlrId,cntlrName);   /* fill in the constant cntlrId string */
    return(pPubObj);
}        

/*
 * Create a Exception Subscription to communicate with the Controllers/Master
 *
 *					Author Greg Brissey 4-26-04
 */
NDDS_ID createExceptionCommSub(NDDS_ID nddsId, char *topic, char *multicastIP, void *callbackArg)
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
    getCntlr_CommInfo(pSubObj);

 
#ifndef RTI_NDDS_4x
    pSubObj->callBkRtn = Exception_CommCallback;
    pSubObj->callBkRtnParam = callbackArg;  /* this is actually a callback routine */
#endif /* RTI_NDDS_4x */
 
    if (multicastIP == NULL)
       pSubObj->MulticastSubIP[0] = 0;   /* use UNICAST */
    else
       strcpy(pSubObj->MulticastSubIP,multicastIP);

#ifdef RTI_NDDS_4x

    initSubscription(pSubObj);
    attachOnDataAvailableCallback(pSubObj,Exception_CommCallback,callbackArg /* user callback func */ );
#endif /* RTI_NDDS_4x */
 
    createSubscription(pSubObj);
    return ( pSubObj );
}

void initialExceptionComm(void *callbackFunc)
{
    long key;

    startLedPatShow();  // as soon as possible 

    // DPRINT(-5,"createExceptionCommPub\n");
    pExceptionPub = createExceptionCommPub(NDDS_Domain,EXCEPTION_PUB_COMM_TOPIC_FORMAT_STR, hostName );
#ifdef RTI_NDDS_4x
    key = (BrdType << 8) | BrdNum;   /* key needs to be unique per board send to master */
    Register_CntlrCommkeyed_instance(pExceptionPub,key);
#endif /* RTI_NDDS_4x */
    // DPRINT(-5,"createExceptionCommSub\n");
    pExceptionSub = createExceptionCommSub(NDDS_Domain,EXCEPTION_SUB_COMM_TOPIC_FORMAT_STR, EXCEPTION_COMM_MULTICAST_IP, 
				   (void *) callbackFunc);
}

/*
 * Used by All , to publish warnings or errors 
 * A mulitcast publication thus all controllers will get this pub
 * even the one sending it
 *
 */
int sendException(int cmd, int arg1,int arg2,int arg3,char *strmsg)
{
#ifdef RTI_NDDS_4x
   DDS_ReturnCode_t result;
   DDS_InstanceHandle_t instance_handle = DDS_HANDLE_NIL;
   Cntlr_CommDataWriter *CntlrCmd_writer = NULL;
#endif /* RTI_NDDS_4x */
   Cntlr_Comm *issue;
 
#ifdef INSTRUMENT
	wvEvent(EVENT_NEXUS_SENDEXCPT,NULL,NULL);
#endif
   /* if it's a warning then don't bother with updating the cntlrState
    * since could prevent proper operation, observed failure was master wait for controllers to
    * report ready for sync, however after the grad1 reported ready for sync but others had not, the grad sent
    * a warning which changed its state thus the master never stated the FIFO, since the grad state had changed 
    *       Greg Brissey   10/26/05
    */
   DPRINT1(+1,"sendException: callbackParam = %d\n",callbackParam);
   if ( arg1 > 200)   /* if errocde is a warning do NOT update cntlr state */
   {

     if (callbackParam == 0)   /* Master */
     {
      cntlrSetState("master1",CNTLR_EXCPT_INITIATED,arg1 /* errorcode */);
      DPRINT(+2,"sendException: master1 CNTLR_EXCPT_INITIATED\n");
     }
     else
     {
      send2Master(CNTLR_CMD_STATE_UPDATE,CNTLR_EXCPT_INITIATED, arg1 /* errorcode */, 0,NULL);
      DPRINT(+2,"sendException: Send master CNTLR_EXCPT_INITIATED\n");
     }
   }

   issue = pExceptionPub->instance;
   semTake(pExceptionPubMutex, WAIT_FOREVER);
   DPRINT(+1,"sendException: got Mutex\n");
   issue->cmd  = cmd;  /* atoi( token ); */
   issue->errorcode = arg1;
   issue->warningcode = arg2;
   issue->arg1 = arg3;
   
   if (strmsg != NULL)
     {
       int strsize = strlen(strmsg);
       if (strsize <= COMM_MAX_STR_SIZE)
	 strncpy(issue->msgstr,strmsg,COMM_MAX_STR_SIZE);
       else
	 errLogRet(LOGIT,debugInfo,"sendException: msg to long: %d, max: %d\n",strsize,COMM_MAX_STR_SIZE);
     }
   DPRINT(+2,"sendException: sendExceptionDirectly\n");
   sendExceptionDirectly(issue);  /* send the exception directly to the phandler of the controller */
   
#ifndef RTI_NDDS_4x
   nddsPublishData(pExceptionPub); /* send exception to other controllers via NDDS publication */
#else  /* RTI_NDDS_4x */
   CntlrCmd_writer = Cntlr_CommDataWriter_narrow(pExceptionPub->pDWriter);
   if (CntlrCmd_writer == NULL) {
     errLogRet(LOGIT,debugInfo,"DataWriter narrow error\n");
     semGive(pExceptionPubMutex);
     return -1;
   }
   
   DPRINT(+2,"sendException: pub 4x exception\n");
   result = Cntlr_CommDataWriter_write(CntlrCmd_writer,
				       issue,&instance_handle);
   if (result != DDS_RETCODE_OK) {
     errLogRet(LOGIT,debugInfo,"DataWriter write error: %d\n",result);
   }
#endif /* RTI_NDDS_4x */
 
   DPRINT(+1,"sendExcpetion: give Mutex\n");
   semGive(pExceptionPubMutex);
   
   if (cmd != WARNING_MSG)
     patternType = 1;   /* show execption pattern on panel LEDs */
   
#ifdef INSTRUMENT
   wvEvent(EVENT_NEXUS_SENDEXCPT_CMPLT,NULL,NULL);
#endif
   return 0;
}

prtcallbackParm()
{
   DPRINT1(-6,"sendException: callbackParam = %d, 0 == Master\n",callbackParam);
}
/*
 * diagnostic stubs, for easy testing
 *
 *					Author Greg Brissey 4-26-04
 */
int rollcall()
{
    cntlrCommPub(CNTLR_CMD_ROLLCALL,0,0,0,NULL);
    return 0;
}

here()
{
    cntlrCommPub(CNTLR_CMD_HERE,0,0,0,NULL);
}

sndstate(int state)
{
    send2Master(CNTLR_CMD_STATE_UPDATE,state,0,0,NULL);
}

static void callbackF(Cntlr_Comm *recvIssue)
{
   DPRINT5(-1,"Exception_Comm CallBack:  cmd: %d, arg1: %d, arg2: %d, arg3: %d, crc: 0x%lx\n",
        recvIssue->cmd,recvIssue->arg1, recvIssue->arg2, recvIssue->arg3, recvIssue->crc32chksum);
}
regexcp()
{
   initialExceptionComm((void *)callbackF);
}
pubExcp(int cmd)
{
   sendException(cmd, 1, 2, 3,"testing");
}

sndErr()
{
   /* HARD_ERROR    15, HDWAREERROR     900, TOOFEWRCVRS     22 */
   /* sendException(HARD_ERROR, HDWAREERROR + TOOFEWRCVRS, 0,0,NULL); */
   sendException(15, 922, 0, 0, NULL);
}
sndWarn()
{
   /* WARING_MSG  14, WARNING     100, ADCOVR     3 */
   /* sendException(HARD_ERROR, HDWAREERROR + TOOFEWRCVRS, 0,0,NULL); */
   sendException(14, 116, 0, 0, NULL);
}

SEM_ID pTheShowSem;
SEM_ID pTheShowMutex;

static int patternStpFlag = 0;

int haltLedShow()
{
    semTake(pTheShowMutex,60);    /* Mutex */
       patternStpFlag = 1;
    semGive(pTheShowMutex);    /* give Mutex back */
    return 0;
}

int resumeLedShow()
{
    semTake(pTheShowMutex,60);    /* Mutex */
      semGive(pTheShowSem);    /* taken once exp start, etc. */
      patternStpFlag = 0;
    semGive(pTheShowMutex);    /* give Mutex back */
   return 0;
}

PatternShow()
{
   while(1)
   {
      semTake(pTheShowSem,WAIT_FOREVER);    /* taken once exp start, etc. */
      switch(patternType)
      {
          case 0 /* IDLE_PAT */:
               idlepat(calcSysClkTicks(50), calcSysClkTicks(67));  /* idlepat(3,4);    */
               break;
          case 1 /* EXCEPT_PAT */:
	       exceptpat(calcSysClkTicks(117));   /* exceptpat(7); */
               break;
           default:
               idlepat(calcSysClkTicks(50), calcSysClkTicks(67)); /* idlepat(3,4);    */
               break;
      }
      semGive(pTheShowSem);    /* taken once exp start, etc. */
      if (patternStpFlag == 1)
      {
         semTake(pTheShowSem,60);    /* taken once exp start, etc. */
         panelLedAllOff();
         patternStpFlag = 0;
      }
   }
}

idlepat(int ondelay,int offdelay)
{
    panelLedMaskOn(0x20);
    taskDelay(ondelay);
    panelLedMaskOn(0x10);
    taskDelay(ondelay);
    panelLedMaskOn(0x08);
    taskDelay(ondelay);
    panelLedMaskOn(0x04);
    taskDelay(ondelay);
    panelLedMaskOn(0x02);
    taskDelay(ondelay);
    panelLedMaskOff(0x20);
    taskDelay(offdelay);
    panelLedMaskOff(0x10);
    taskDelay(offdelay);
    panelLedMaskOff(0x08);
    taskDelay(offdelay);
    panelLedMaskOff(0x04);
    taskDelay(offdelay);
    panelLedMaskOn(0x04);
    taskDelay(ondelay);
    panelLedMaskOn(0x08);
    taskDelay(ondelay);
    panelLedMaskOn(0x10);
    taskDelay(ondelay);
    panelLedMaskOn(0x20);
    panelLedMaskOff(0x02);
    taskDelay(offdelay);
    panelLedMaskOff(0x04);
    taskDelay(offdelay);
    panelLedMaskOff(0x08);
    taskDelay(offdelay);
    panelLedMaskOff(0x10);
    taskDelay(offdelay);
}

exceptpat(int delay)
{
    panelLedMaskOn(0x22);
    taskDelay(delay);
    panelLedMaskOff(0x22);
    panelLedMaskOn(0x14);
    taskDelay(delay);
    panelLedMaskOff(0x14);
    panelLedOn(4);
    taskDelay(delay);
    panelLedMaskOn(0x14);
    taskDelay(delay);
    panelLedOff(4);
    panelLedMaskOff(0x14);
}

/* priority = 255  */
/* startLedPatShow(int taskpriority,int taskoptions,int stacksize) */
startLedPatShow()
{
     /* PANEL_LED_TASK_PRIORITY=255, STD_STACKSIZE=4096, STD_TASKOPTIONS=0 */
    pTheShowSem = semBCreate(SEM_Q_FIFO,SEM_FULL);
    pTheShowMutex = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE |
                                  SEM_DELETE_SAFE); 

    if (taskNameToId("tPanelLeds") == ERROR)
      taskSpawn("tPanelLeds",PANEL_LED_TASK_PRIORITY,STD_TASKOPTIONS,
                   STD_STACKSIZE,PatternShow, 1,2,3,4,5,6,7,8,9,10);
}
