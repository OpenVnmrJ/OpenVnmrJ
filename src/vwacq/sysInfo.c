/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* sysInfo.c 07/09/07  - System Information Resources */
/* 
 */


#include <vxWorks.h>
#include <taskLib.h>
#include <stdioLib.h>
#include <semLib.h>
#include <rngLib.h>
#include <msgQLib.h>
#include "hostAcqStructs.h"
#include "expDoneCodes.h"
#include "logMsgLib.h"
#include "namebufs.h"
#include "hardware.h"
#include "taskPrior.h"
#include "vmeIntrp.h"
#include "stmObj.h"
#include "fifoObj.h"
#include "adcObj.h"
#include "autoObj.h"
#include "AParser.h"
#include "expDoneCodes.h"
#include "errorcodes.h"
#include "tuneObj.h"
 

extern FIFO_ID pTheFifoObject;	/* STM Object */
extern STMOBJ_ID pTheStmObject;	/* STM Object */
extern ADC_ID    pTheAdcObject; /* ADC Object */
extern ACODE_ID  pTheAcodeObject; /* Acode object */
extern AUTO_ID   pTheAutoObject; /* Automation Object */
extern TUNE_ID   pTheTuneObject; /* Tune Object */

extern MSG_Q_ID pUpLinkMsgQ;	/* MsgQ used between UpLinker and STM Object */
extern MSG_Q_ID pMsgesToHost;	/* MsgQ used for Msges to routed upto Expproc */
extern MSG_Q_ID pMsgesToAParser;/* MsgQ used for Msges to Acode Parser */
extern MSG_Q_ID pMsgesToAupdt;	/* MsgQ used for Msges to Acode Updater */
extern MSG_Q_ID pMsgesToXParser;/* MsgQ used for Msges to Xcode Parser */
extern MSG_Q_ID pMsgesToPHandlr;/* MsgQ for Msges to Problem Handler */
extern MSG_Q_ID pTagFifoMsgQ;	/* MsgQ for Tag Fifo */
extern MSG_Q_ID pApFifoMsgQ;     /* MsgQ for AP Fifo ReadBack */
extern MSG_Q_ID pUpLinkIMsgQ;	/* MsgQ used between UpLinker and STM Object */
extern MSG_Q_ID pUpLnkHookMsgQ; /* MsgQ used between a Data process & STM Obj */
extern MSG_Q_ID pIntrpQ;	/* for testing purposes */

extern RING_ID  pSyncActionArgs;  /* Buffer for 'Sync Action' (e.g. SETVT) function args */

/* console status block sent to host */
extern STATUS_BLOCK currentStatBlock;		/* Acqstat-like status block */

/* Exception Msges to Phandler, e.g. FOO, etc. */
extern EXCEPTION_MSGE HardErrorException;
extern EXCEPTION_MSGE GenericException;

/* Fixed & Dynamic Named Buffers */
extern DLB_ID  pDlbDynBufs;
extern DLB_ID  pDlbFixBufs;

/* SA related globals */
extern SEM_ID  pSemSAStop; /* Binary  Semaphore used to Stop upLinker for SA */
extern int     SA_Criteria;/* SA, EXP_FID_CMPLT, BS_CMPLT, IL_CMPLT */
extern unsigned long SA_Mod; /* modulo for SA, ie which fid to stop at 'il'*/
extern unsigned long SA_CTs;  /* Completed Transients for SA */

/* Revision Number for Interpreter & System */
extern int SystemRevId; 
extern int InterpRevId;

/* Named Buffer configuration Values */
#define DYN_BUF_NUM 20
#define DYN_BUF_SIZ 0
#define FIX_BUF_NUM 128		/* total acodes of 256K */
#define FIX_BUF_SIZ 2048

static MSG_Q_INFO MsgQinfo;
static char *msgList[4];
static int   msgLenList[4];
static ITR_MSG *stmitrmsge;

typedef struct __dcodestruc {
		     int code;
		     char *codestr;
                  } dcodeentry;

static dcodeentry donecode2msg[14] = {	{ EXP_NULL_CASE , "NO-OP" },
				{ EXP_FID_CMPLT, "Exp/FID Complete" },
				{ BS_CMPLT, "BS Complete" },
				{ SOFT_ERROR, "Soft Error" },
				{ WARNING_MSG, "Warning" },
				{ HARD_ERROR, "Hard Error" },
				{ EXP_ABORTED, "Exp ABorted" },
				{ SETUP_CMPLT, "Setup Complete" },
				{ STOP_CMPLT, "Stop(SA) Complete" },
				{ FIX_ACQSTAT, "FIX_ACQSTAT" },
				{ SYSTEM_READY, "System Ready" },
				{ LOST_CONN, "Connection to Host Lost" },
				{ ALLOC_ERROR, "Memory Allocation Error" },
				{ WATCHDOG, "Watchdog" } };
#define DCODESIZE 14

static dcodeentry casecode[5] = {
				  { PARSE, "Parse Instruction" },
				  { CASE, "CASE Instruction" },
				  { STATBLK, "StatBlock Data" },
				  { TUNE_UPDATE, "Tune Sync Info" },
				  { CONF_INFO, "System Configure Data" } };

#define CCODESIZE 5

static dcodeentry casetypes[8] = {
				{ PANIC, "Panic" },
				{ EXP_CMPLT, "Exp. Complete" },
				{ IL_CMPLT, "Interleave Complete" },
				{ CT_CMPLT, "CT Complete" },
				{ EXP_HALTED, "Exp Halted" },
				{ EXP_STOPPED, "Exp Stopped" },
				{ GET_SAMPLE, "Get Sample from Magnet" },
				{ LOAD_SAMPLE, "Load Sample into Magnet" } };

#define CTYPESIZE 8

sysDump(int level)
{ 
  printf("\n");
  sysSemInfo();
  printf("\nSystem Message Queues:\n");
  msgQInfo(level);
  printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
  printf("\nSystem Name Buffers:\n");
  sysNamebufs(level);
  printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
}

sysNamebufs(int level)
{
   printf("\n\nFixed Name Buffer Status;\n");
   dlbShow(pDlbFixBufs,level);
   printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
   printf("\n\nDynamic Name Buffer Status;\n");
   dlbShow(pDlbDynBufs,level);
   printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
}
sysSemInfo()
{
    printf("Uplinker SA Semaphore: ID: 0x%lx\n",pSemSAStop);
    printSemInfo(pSemSAStop,"Uplinker SA Semaphore",1);
}

msgQInfo(int level)
{
  MSG_Q_INFO *pMsgQinfo;

  printf("\nSTM --> Batch UpLinker MsgQ(0x%lx):\n",pUpLinkMsgQ);
  msgQInfoPrint(pUpLinkMsgQ);
  if ((level > 0) && (MsgQinfo.numMsgs > 0))
  {
    printQStmMsg();
  }

  printf("\nSTM --> Interactive UpLinker MsgQ(0x%lx):\n",pUpLinkIMsgQ);
  msgQInfoPrint(pUpLinkIMsgQ);
  if ((level > 0) && (MsgQinfo.numMsgs > 0))
  {
    printQStmMsg();
  }

  printf("\nAcode Parser MsgQ(0x%lx):\n",pMsgesToAParser);
  msgQInfoPrint(pMsgesToAParser);

  printf("\nXcode Parser MsgQ(0x%lx):\n",pMsgesToXParser);
  msgQInfoPrint(pMsgesToXParser);

  printf("\nHost MsgQ(0x%lx):\n",pMsgesToHost);
  msgQInfoPrint(pMsgesToHost);
  if ((level > 0) && (MsgQinfo.numMsgs > 0))
  {
    printQHostMsg();
  }
  
  printf("\nAcode Updater MsgQ(0x%lx):\n",pMsgesToAupdt);
  msgQInfoPrint(pMsgesToAupdt);
  
  printf("\nPhandler MsgQ(0x%lx):\n",pMsgesToPHandlr);
  msgQInfoPrint(pMsgesToPHandlr);
  if ((level > 0) && (MsgQinfo.numMsgs > 0))
  {
    printQPhdlr();
  }
  
  printf("\nFIFO Tag MsgQ(0x%lx):\n",pTagFifoMsgQ);
  msgQInfoPrint(pTagFifoMsgQ);
  
/*
  printf("\nFIFO AP Readback MsgQ(0x%lx):\n",pApFifoMsgQ);
  msgQInfoPrint(pApFifoMsgQ);
*/
  
}

int msgQInfoPrint(MSG_Q_ID pMsgQ)
{
  int i,pPendTasks[4];
  char *pStr;

  memset(&MsgQinfo,0,sizeof(MsgQinfo));
  MsgQinfo.taskIdListMax = 4;
  MsgQinfo.taskIdList = &pPendTasks[0];
  MsgQinfo.msgListMax = 4;
  MsgQinfo.msgPtrList = &msgList[0];
  MsgQinfo.msgLenList = &msgLenList[0];

  msgQInfoGet(pMsgQ,&MsgQinfo);
  printf("%d Msge Queued, %d Limit, %d (bytes) Msge Size Limit\n",
	  MsgQinfo.numMsgs,MsgQinfo.maxMsgs,MsgQinfo.maxMsgLength);

  if (MsgQinfo.numMsgs == 0)
      pStr = "Pending to get from MsgQ";
  else if (MsgQinfo.numMsgs == MsgQinfo.maxMsgs) 
      pStr = "Pending to put into MsgQ";

  if (MsgQinfo.numTasks > 0)
  {
    for(i=0;i < MsgQinfo.numTasks; i++)
    {
      printf("Task: '%s' - 0x%lx is %s \n",taskName(pPendTasks[i]),pPendTasks[i],pStr);
    }
  }
  else
  {
      printf("No Task(s) Pending on this msgQ\n");
  }
  
  return(0);
}

systemInfo()
{

}

printSemInfo(SEM_ID pSemId,char *msge,int level)
{
   int npend,pAry[6],i;

   npend = semInfo(pSemId,pAry,6);
   if ( npend > 0 )
   {
     for(i=0;i < npend; i++)
     {
       printf("Task: '%s' - 0x%lx Pending on %s\n",taskName(pAry[i]),pAry[i],msge);
     }
     semShow(pSemId,0);
   }
   else
   {
      printf("No Tasks Pending on %s\n",msge);
   }
}

printQStmMsg()
{
   int i;
   char *dtype;
   char *getdonecodetype(int dcode);

   for(i=0;i < MsgQinfo.numMsgs; i++)
   {
      stmitrmsge = (ITR_MSG*) (MsgQinfo.msgPtrList[i]);
      dtype = getdonecodetype(stmitrmsge->donecode);
      printf("Msge %d - STM Tag: %d, Msge Type: %d, DoneCode: %s (%d), ErrorCode: %d, Count: %lu\n",
		i+1,stmitrmsge->tag,stmitrmsge->msgType,dtype,stmitrmsge->donecode,
		stmitrmsge->errorcode,stmitrmsge->count);
   }
}
printQPhdlr()
{
   int i;
   EXCEPTION_MSGE *msge;
   char *getdonecodetype(int dcode);
   char *dtype;
   for(i=0;i < MsgQinfo.numMsgs; i++)
   {
      msge = (EXCEPTION_MSGE*) (MsgQinfo.msgPtrList[i]);
      dtype = getdonecodetype(msge->exceptionType);
      printf("Msge %d - DoneCode: %s (%d), ErrorCode: %d\n",
	i+1,dtype,msge->exceptionType,msge->reportEvent);
   }
}
printQHostMsg()
{
   int i;
   long  *msge;
   char *getdonecodetype(int dcode);
   char *getcasecodetype(int dcode);
   char *getcasetype(int dcode);
   char *dtype,*ctype;;
   for(i=0;i < MsgQinfo.numMsgs; i++)
   {
      msge = (long*) (MsgQinfo.msgPtrList[i]);
      ctype = getcasecodetype(msge[0]);
      if (msge[0] == CASE)
      {
        dtype = getdonecodetype(msge[1]);
        if (dtype == NULL)
           dtype = getcasetype(msge[1]);
        printf("Msge %d - Type: %s(%d), Instruction: %s (%d), Args: %ld, %ld, %ld\n",
	i+1,ctype,msge[0],dtype,msge[1], msge[2], msge[3], msge[4]);
      }
      else
      {
        printf("Msge %d - Type: %s (%d), Args/Data: %ld, %ld, %ld, %ld\n",
	i+1,ctype,msge[0],msge[1], msge[2], msge[3], msge[4]);
      }
   }
}
char *getdonecodetype(int dcode)
{
   int i;
   for (i=0;i < DCODESIZE; i++)
   {
      if ( donecode2msg[i].code == dcode )
      {
         return(donecode2msg[i].codestr);
      }
   }
   return(NULL);
}
char *getcasecodetype(int dcode)
{
   int i;
   for (i=0;i < CCODESIZE; i++)
   {
      if ( casecode[i].code == dcode )
      {
         return(casecode[i].codestr);
      }
   }
   return(NULL);
}
char *getcasetype(int dcode)
{
   int i;
   for (i=0;i < CTYPESIZE; i++)
   {
      if ( casetypes[i].code == dcode )
      {
         return(casetypes[i].codestr);
      }
   }
   return(NULL);
}

