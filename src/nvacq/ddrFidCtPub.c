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
#include "taskPriority.h"

#include "logMsgLib.h"

#include "NDDS_Obj.h"
#include "NDDS_PubFuncs.h"
#include "NDDS_SubFuncs.h"

#include "FidCt_Stat.h"

#ifdef RTI_NDDS_4x
#include "FidCt_StatPlugin.h"
#include "FidCt_StatSupport.h"
#endif  /* RTI_NDDS_4x */

#define HOSTNAME_SIZE 80
extern char hostName[HOSTNAME_SIZE];

extern int DebugLevel;
extern NDDS_ID NDDS_Domain;

extern int scan_count;
extern int fid_count;
extern int fid_ct;

/* Board Type, rf,master,ddr,etc., and board number, rf1,rf2,rf3, etc. */
extern int BrdType;
extern int BrdNum;


/* Console  status Pub */
NDDS_ID pFidCtStatusPub = NULL;

FidCt_Stat *pFidCtStat = NULL;

/* SEM_ID pFidCtPubSem = NULL;  Bugzilla bug# 150 */
MSG_Q_ID pFidCtPubMsgQ = NULL;

#ifdef RTI_NDDS_4x
publishFidCtStat(NDDS_ID pNDDS_Obj)
{
   DDS_ReturnCode_t result;
   DDS_InstanceHandle_t instance_handle = DDS_HANDLE_NIL;
   FidCt_StatDataWriter *FidCt_Stat_writer = NULL;

   FidCt_Stat_writer = FidCt_StatDataWriter_narrow(pNDDS_Obj->pDWriter);
   if (FidCt_Stat_writer == NULL) {
        errLogRet(LOGIT,debugInfo,"publishFidCtStat: DataWriter narrow error\n");
        return -1;
   }

   result = FidCt_StatDataWriter_write(FidCt_Stat_writer,
                pNDDS_Obj->instance,&instance_handle);
   if (result != DDS_RETCODE_OK) {
            errLogRet(LOGIT,debugInfo,"publishFidCtStat: DataWriter write error: %d\n",result);
   }
   return 0;
}
#endif  /* RTI_NDDS_4x */
/*
 *   Task waits for the semaphore to be given then publishes the Status
 *   This is done to avoid doing the publishing from within a ISR routines.
 *
 *     Author:  Greg Brissey 9/29/04
 */
pubFidCtStatus()
{
   int arg,bytes;
   FidCt_Stat fstat;
   int timeoutnticks;
   timeoutnticks = calcSysClkTicks(5000);  /* 5 sec timeout */
   FOREVER
   {
       /* semTake(pFidCtPubSem, WAIT_FOREVER); see bugzilla bug #150 WSR TSR# 421424 */
       /* bytes = msgQReceive(pFidCtPubMsgQ, (char*)&fstat,sizeof(FidCt_Stat), WAIT_FOREVER); */
       bytes = msgQReceive(pFidCtPubMsgQ, (char*)&fstat,sizeof(FidCt_Stat), timeoutnticks);
       
       /* DPRINT1(-1,"pubFidCtStatus: CT: %d\n",arg); */
       if (pFidCtStatusPub != NULL)
       {
          if (bytes != ERROR)
          {
             pFidCtStat->FidCt=fstat.FidCt;
             pFidCtStat->Ct=fstat.Ct;
             DPRINT2(1,"pubFidCtStatus: QRECV: fid:%d ct:%d\n", (int)fstat.FidCt,(int)fstat.Ct); 
          }
          else   /* timed out, allows console to go idle if time Procs running before comlink is set up */
          {
             pFidCtStat->FidCt=fid_count;
             pFidCtStat->Ct=fid_ct;
             DPRINT2(5,"pubFidCtStatus: Timed out: fid:%d ct:%d\n", (int)fid_count,(int)fid_ct); 
          }
#ifndef RTI_NDDS_4x
          nddsPublishData(pFidCtStatusPub);
#else  /* RTI_NDDS_4x */
          publishFidCtStat(pFidCtStatusPub);
#endif  /* RTI_NDDS_4x */
       }
   }
}

/*
 * publishe the FidCt issue in the call task context.
 * possible usage is in upLink to avoid issue pub starvation, though it does take
 *  1/2 to several milliseconds to publish, so for now not ultilized
 *    GMB 1/29/07
 */
void publishFidCt()
{
   if (pFidCtStatusPub != NULL)
#ifndef RTI_NDDS_4x
       nddsPublishData(pFidCtStatusPub);
#else  /* RTI_NDDS_4x */
       publishFidCtStat(pFidCtStatusPub);
#endif  /* RTI_NDDS_4x */
   return;
}


void initialFidCtStatComm()
{
   char topic[60];
   int startFidCtStatPub(int priority, int taskoptions, int stacksize);
   NDDS_ID createFidCtStatusPub(NDDS_ID nddsId, char *topic);
#ifdef RTI_NDDS_4x
   long key;
   int Register_FidCt_Statkeyed_instance(NDDS_ID pNDDS_Obj,int key);
#endif  /* RTI_NDDS_4x */

#ifndef RTI_NDDS_4x
   sprintf(topic,FIDCT_PUB_STAT_TOPIC_FORMAT_STR,hostName);
   pFidCtStatusPub = createFidCtStatusPub(NDDS_Domain,(char*) topic);
#else  /* RTI_NDDS_4x */
   pFidCtStatusPub = createFidCtStatusPub(NDDS_Domain,(char*) FIDCT_STAT_M21TOPIC_STR);
   key = (BrdType << 8) | BrdNum;   /* key needs to be unique per board send to master */
   Register_FidCt_Statkeyed_instance(pFidCtStatusPub,key);
#endif  /* RTI_NDDS_4x */
   pFidCtStat = pFidCtStatusPub->instance;

   pFidCtStat->FidCt = 0;
   pFidCtStat->Ct = 0;
   startFidCtStatPub(STATMON_TASK_PRIORITY+10, STD_TASKOPTIONS, STD_STACKSIZE);
}

startFidCtStatPub(int priority, int taskoptions, int stacksize)
{
   /* if (pFidCtPubSem == NULL) */
   if (pFidCtPubMsgQ == NULL)
   {
      /* char *tmpbuf = (char*) malloc(1024); see Bugzilla bug# 150 */
      /* pFidCtPubSem = semBCreate(SEM_Q_FIFO,SEM_EMPTY); */
      /* pFidCtPubSem = semBCreate(SEM_Q_PRIORITY,SEM_EMPTY); */

      pFidCtPubMsgQ = msgQCreate(10,sizeof(FidCt_Stat),MSG_Q_PRIORITY);
      /* if ( (pFidCtPubSem == NULL) ) */
      if ( (pFidCtPubMsgQ == NULL) )
      {
        errLogSysRet(LOGIT,debugInfo,
	   "startConsoleStatPub: Failed to allocate pFidCtPubSem Semaphore:");
        return(ERROR);
      }
   }
   
   /* DPRINT1(-1,"pFidCtPubSem: 0x%lx\n",pFidCtPubSem); */
   if (taskNameToId("tFidCtPub") == ERROR)
      taskSpawn("tFidCtPub",priority,0,stacksize,pubFidCtStatus,1,
						2,3,4,5,6,7,8,9,10);
}

killConsolePub()
{
   int tid;
   if ((tid = taskNameToId("tFidCtPub")) != ERROR)
      taskDelete(tid);
}

#ifdef  RTI_NDDS_4x
/* Register an instance based on key given */
int Register_FidCt_Statkeyed_instance(NDDS_ID pNDDS_Obj,int key)
{
    FidCt_Stat *pIssue = NULL;
    FidCt_StatDataWriter *FidCt_StatWriter = NULL;

    FidCt_StatWriter = FidCt_StatDataWriter_narrow(pNDDS_Obj->pDWriter);
    if (FidCt_StatWriter == NULL) {
        errLogRet(LOGIT,debugInfo, "Register_FidCt_Statkeyed_instance: DataReader narrow error.\n");
        return -1;
    }

    pIssue = pNDDS_Obj->instance;

    pIssue->key = key;
    // pIssue->key = 1;

    // for Keyed Topics must register this keyed topic
    FidCt_StatDataWriter_register_instance(FidCt_StatWriter, pIssue);

    return 0;
}
#endif  /*  RTI_NDDS_4x */

/*
 * Create a Best Effort Publication Topic to communicate the Lock Status
 * Information
 *
 *					Author Greg Brissey 9-29-04
 */
NDDS_ID createFidCtStatusPub(NDDS_ID nddsId, char *topic)
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
    pPubObj->pubThreadId = STATMON_TASK_PRIORITY; /* DEFAULT_PUB_THREADID; taskIdSelf(); */
         
    /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */
    getFidCt_StatInfo(pPubObj);
         
    /* added this so for muliple DDRs the strength is different for all DDRs
     * DDR1 =1, DDR2 = 2, etc. 
     */
    pPubObj->publisherStrength = 1024 - BrdNum;

    DPRINT2(1,"createFidCtStatusPub: Create Pub topic: '%s', strenght: %d \n",
                pPubObj->topicName,pPubObj->publisherStrength);
#ifndef RTI_NDDS_4x
    createBEPublication(pPubObj);
#else  /* RTI_NDDS_4x */
    initBEPublication(pPubObj);
    createPublication(pPubObj);
#endif  /* RTI_NDDS_4x */
    return(pPubObj);
}        

zeroFidCtState()
{
   fid_count = fid_ct = 0;
   DPRINT(1,"zeroFidCtState done\n");
}

sendFidCtStatus()
{
   FidCt_Stat fstat;

    if (pFidCtStatusPub != NULL)
    {
        fstat.FidCt = fid_count;
        fstat.Ct = fid_ct;
       
/*
        pFidCtStat->FidCt = fidnum;
        pFidCtStat->Ct = ct;
*/
    }
/*
    if (pFidCtPubSem != NULL)
      semGive(pFidCtPubSem);
*/
    if (pFidCtPubMsgQ == NULL)
        return;
        
    // If the msg Q is full msgQSend will not post the new fid status - this just means
    // we are running too fast to keep up with the (real time) fid/ct update rate
    // and some fid/ct status values will not show up in vj's exp bar.
    
     msgQSend(pFidCtPubMsgQ, (char *)&fstat, sizeof(FidCt_Stat), NO_WAIT, MSG_PRI_NORMAL);
     
    /* DPRINT1(-1,"sendFidCtStatus: MsqSend: 0x%lx\n",pFidCtPubMsgQ); */
}
