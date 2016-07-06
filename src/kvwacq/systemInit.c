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
/*   12-01-1994    pMsgesToAParser and pMsgesToXParser each modified to hold
		   50 messages of CONSOLE_MSGE_SIZE, since they receive stuff
		   directly from the monitor channel.				*/

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
#include "errorcodes.h"
#include "tuneObj.h"
#include "sibTask.h"
 

/*
   System Initialization
   1. Make Message Qs required by the various Tasks
   2. Instanciate hardware Objects
   3. Open the needed Channels
   4. Start the needed tasks
*/

extern void getConfig();

extern FIFO_ID pTheFifoObject;	/* FIFO Object */
extern STMOBJ_ID pTheStmObject;	/* STM Object */
extern ADC_ID    pTheAdcObject; /* ADC Object */
extern ACODE_ID  pTheAcodeObject; /* Acode object */
extern AUTO_ID   pTheAutoObject; /* Automation Object */
extern TUNE_ID   pTheTuneObject; /* Tune Object */
extern SIB_ID    pTheSibObject;	/* Safety Interface Board */

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

extern int shimSet;

/* Named Buffer configuration Values */
#define DYN_BUF_NUM 20
#define DYN_BUF_SIZ 0
#define FIX_BUF_NUM 128		/* total acodes of 256K */
#define FIX_BUF_SIZ 2048

static int	startupSysConf;


long
getStartupSysConf()
{
	return( startupSysConf );
}

systemInit()
{
   int    iter;

   extern uplink();
   extern startParser();
   extern startDl();
   extern fakeItr(MSG_Q_ID, int);
   extern uplink(MSG_Q_ID,STMOBJ_ID);
   extern strtHostComLk(MSG_Q_ID);
   extern pHandler(MSG_Q_ID);
   extern downlink(MSG_Q_ID ,DLB_ID ,DLB_ID );
   extern void APparser();
   extern int connBroker();
   extern void fifoSW1aupdt(ACODE_ID pAcodeId);
   extern void fifoSW2ISR(void);

   DebugLevel = 0;
   startupSysConf = 0;
   memset( &currentStatBlock.stb, 0, sizeof( currentStatBlock.stb ) );

   tickSet(0L);  /* set tick count to Zero, for Ldelay() & Tcheck() */

   /* initializes System Task Ready Flags */
   InitSystemFlags();

   /* Pre-set Hardware Error so we can be quick about it */
   HardErrorException.exceptionType = HARD_ERROR;
   
 /* Add our System Delete Hook Routine */
   addSystemDelHook();

 /* Build the Stop Acquisition Semaphore */
  pSemSAStop = semBCreate(SEM_Q_FIFO,SEM_EMPTY);
  SA_Criteria = 0;  /* reset to no SA */
  SA_Mod = SA_CTs = 0L;

 /* 1.   -------------  Message Qs */
   pUpLinkMsgQ = msgQCreate(512,sizeof(ITR_MSG),MSG_Q_FIFO);
   pMsgesToHost = msgQCreate(512,sizeof(currentStatBlock)+10,MSG_Q_PRIORITY);
   pMsgesToAParser = msgQCreate(50,CONSOLE_MSGE_SIZE,MSG_Q_PRIORITY);
   pMsgesToAupdt = msgQCreate(50,CONSOLE_MSGE_SIZE,MSG_Q_PRIORITY);
   pMsgesToXParser = msgQCreate(50,CONSOLE_MSGE_SIZE,MSG_Q_PRIORITY);
   pMsgesToPHandlr = msgQCreate(25,128,MSG_Q_PRIORITY);
   pTagFifoMsgQ = msgQCreate(64,sizeof(long),MSG_Q_PRIORITY);
   /* pApFifoMsgQ = msgQCreate(128,sizeof(ITR_MSG_FIFO),MSG_Q_FIFO); */
   pUpLinkIMsgQ = msgQCreate(512,sizeof(ITR_MSG),MSG_Q_FIFO);
   pUpLnkHookMsgQ = msgQCreate(10,sizeof(ITR_MSG),MSG_Q_FIFO);

   pSyncActionArgs = rngCreate(50 * sizeof(long));  

 /* 2.   -------------  Channels */
  
 /* 3.   -------------  Hardware Objects */

   pTheFifoObject = fifoCreate(FIFO_BASE_ADR,
		BASE_INTRP_VEC,4,"FIFO Object for 1st Board");
   if (pTheFifoObject->fifoBaseAddr == 0xFFFFFFFF) 
   {
     GenericException.exceptionType = WARNING_MSG;  	/* donecode for Vnmr */
     GenericException.reportEvent = WARNINGS + FIFOMISSING; /* errorcode */
     msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
		sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
   }
   else
   {
     startupSysConf |= FIFO_PRESENT;
   }

   instantiateSTMs();  /* create STM Objects for all STM's present in system */

   /*
   pTheStmObject = stmCreate(STM_BASE_ADR,
		(unsigned long)STM_MEM_BASE_ADR, 
		STM_AP_ADR, STM_BASE_INTRP_VEC, 3,
		"STM Object for 1st Board");
*/
   if (pTheStmObject->stmBaseAddr == 0xFFFFFFFF) 
   {
     GenericException.exceptionType = WARNING_MSG;  	/* donecode for Vnmr */
     GenericException.reportEvent = WARNINGS + STMMISSING; /* errorcode */
     msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
		sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
   }
   else
   {
     for (iter = 0; iter < MAX_STM_OBJECTS; iter++)
       if (stmGetStmObjByIndex( iter ) != NULL)
        startupSysConf |= STM_PRESENT( iter );
   }
    

   pTheAdcObject = adcCreate(ADC_BASE_ADR,
		ADC_AP_ADR1A,ADC_ITRP_VEC,3,"ADC Object for 1st Board");
   if (pTheAdcObject->adcBaseAddr == 0xFFFFFFFF) 
   {
     GenericException.exceptionType = WARNING_MSG;  	/* donecode for Vnmr */
     GenericException.reportEvent = WARNINGS + ADCMISSING; /* errorcode */
     msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
		sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
   }
   else {
     startupSysConf |= ADC_PRESENT(0);
   }

   pTheAcodeObject = acodeCreate("");
  
/* Not for MURCURY */
/*   /* Register Setup ISR for Fifo SW intrp 1, interactive usage (acqi) */
/*   fifoSwIsrReg(pTheFifoObject,FIFO_SW_INTRP1, fifoSW1aupdt, pTheAcodeObject);
/*   /* Register Setup ISR for Fifo SW intrp 2, stop acquisition (sa) */
/*   fifoSwIsrReg(pTheFifoObject,FIFO_SW_INTRP2, fifoSW2ISR, NULL);
/* Not for MURCURY */

   pTheTuneObject = tuneCreate();

  /* enable VME interrupts */
  sysIntEnable(4);
  sysIntEnable(3);
  sysIntEnable(2);

  pDlbDynBufs = dlbCreate(DYNAMIC_BUF, DYN_BUF_NUM, DYN_BUF_SIZ);
  pDlbFixBufs = dlbCreate(FIXED_FAST_BUF, FIX_BUF_NUM, FIX_BUF_SIZ);
  if ((pDlbDynBufs == NULL) || (pDlbFixBufs == NULL))
  {
     exit(1);
  }

   printf("Waiting for MSR: ");
   pTheAutoObject = autoCreate(AUTO_BASE_ADR, AUTO_MEM_BASE_ADR, 
		AUTO_SHARED_MEM_OFFSET,
		AUTO_AP_ADR, AUTO_ITRP_VEC, 3, "Automation Object");
   if (pTheAutoObject->autoBaseAddr != 0xFFFFFFFF)
   {
     startupSysConf |= MSR_PRESENT;
   }

   if (acqerrno != 0) 
   {
     printf("MSR not present on Mercury-vx\n");
/*     GenericException.exceptionType = WARNING_MSG;  	/* donecode for Vnmr */
/*     GenericException.reportEvent = acqerrno; /* errorcode */
/*     msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
/*		sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
/* */
   }
   else
     printf("Ready\n");

   autoItrpEnable(pTheAutoObject, AUTO_ALLITRPS);
   
   /* If no FIFO then don't config  */
   if (pTheFifoObject->fifoBaseAddr != 0xFFFFFFFF) 
   {
      int calltaskid, calltaskprior;
      calltaskid = taskIdSelf();
      taskPriorityGet(calltaskid,&calltaskprior);
      taskPrioritySet(calltaskid,APARSER_TASK_PRIORITY);
      fifoItrpEnable(pTheFifoObject,FSTOPPED_I);
      /* moved determineShimType() to here, because it now
      /* uses the fifo to read APbus for MERCURY, need priorities
      /* otherwhise "attemp to start empty fifo" msg */
      determineShimType();
      currentStatBlock.stb.AcqShimSet = shimSet;
      getConfig();
      taskPrioritySet(calltaskid,calltaskprior);
   }

   /* Create the SIB object */
   pTheSibObject = sibCreate(SIB_ITRP_VEC, SIB_ITRP_LEVEL);

  dmaInit();
  storeConsoleDebug( SYSTEM_STARTUP );
  startTasks();
  printf("System Version: %d\n", SystemRevId);
  printf("Interpreter Version: %d\n", InterpRevId);
  printf("Bootup Complete.\n");
}

startTasks()
{
   extern uplink();
   extern startParser();
   extern startDl();
   extern fakeItr(MSG_Q_ID, int);
   extern uplink(MSG_Q_ID,STMOBJ_ID);
   extern strtHostComLk(MSG_Q_ID);
   extern pHandler(MSG_Q_ID);
   extern downlink(MSG_Q_ID ,DLB_ID ,DLB_ID );
   extern void APparser();
   extern int connBroker();

   int priority, stacksize, bigstacksize, medstacksize, taskoptions, 
       taskDelayTicks;
 /* 4.   -------------  Start Tasks */
   /* Start communication link to Host */
   /* STD_STACKSIZE 3000, MED_STACKSIZE 10000, BIG_STACKSIZE 20000 */
   /* STD_TASKOPTIONS 0 */

   initConnBroker();

   /* indicate acq. idle to acqstatus */
   currentStatBlock.stb.Acqstate = ACQ_IDLE;   

   /* Initiate comm link with Host */
   startComLnk(MONITOR_TASK_PRIORITY,STD_TASKOPTIONS, STD_STACKSIZE);

   /* Start UpLinker */
   startUpLnk(UPLINKER_TASK_PRIORITY, STD_TASKOPTIONS, STD_STACKSIZE);

   /* Start interactive UpLinker */
   startUpLnkI(UPLINKERI_TASK_PRIORITY, STD_TASKOPTIONS, STD_STACKSIZE);

   /* Start Problem Handler */
   startPhandler(PHANDLER_PRIORITY, STD_TASKOPTIONS, STD_STACKSIZE);

   /* Start Exp. Signal Handler */
   startShandler(SHANDLER_PRIORITY, STD_TASKOPTIONS | VX_FP_TASK, XSTD_STACKSIZE);

   /* Start Fake STM interrupt Task used by parser */
   taskDelayTicks = 20;
/*
   if (taskNameToId("tIntrp") == ERROR)
     taskSpawn("tIntrp",priority-1,0,STD_STACKSIZE,fakeItr,pUpLinkMsgQ,
		taskDelayTicks, 3,4,5,6,7,8,9,10);
*/

   /* Start downLinker */
   startDwnLnk(DOWNLINKER_TASK_PRIORITY,STD_TASKOPTIONS,CON_STACKSIZE);

   /* Start A Parser */
   startParser(APARSER_TASK_PRIORITY, STD_TASKOPTIONS, MED_STACKSIZE);

   /* Start X Parser */
   startXParser(XPARSER_TASK_PRIORITY, STD_TASKOPTIONS, STD_STACKSIZE);

   /* Start A Updt */
   startAupdt(AUPDT_TASK_PRIORITY, STD_TASKOPTIONS, MED_STACKSIZE);

   /*  Create a Canned set of A-codes for Interactive Lock  */
   /*  moved to monitor.c, where it is called each time you start
       interactive lock.  The A_codes and the RT variables are
       deleted when you stop the lock display. */
   /*setup_for_lock();*/
   if (taskNameToId("tConBrker") == ERROR)
   {
     taskDelay(sysClkRateGet() * 1);
     taskSpawn("tConBrker",CONBROKER_TASK_PRIORITY,STD_TASKOPTIONS,
		CON_STACKSIZE, connBroker,1,2,3,4,5,6,7,8,9,10);
     taskDelay(sysClkRateGet() * 1);
   }

   sysEnable();

/*   /* If no FIFO then don't start tune task */
/*   if (pTheFifoObject->fifoBaseAddr != 0xFFFFFFFF) 
/*   {
/*      startTuneMonitor( 210, STD_TASKOPTIONS, STD_STACKSIZE );
/*   }
/*
/*   /* Start the Safety System Monitor */
/*   startSibTask(SIB_IST_PRIORITY, STD_TASKOPTIONS, STD_STACKSIZE);
/* */
}

resetBufs()
{
   /* reset STM */
   stmInitial(pTheStmObject, 5, 1024, pUpLinkMsgQ, 0);
   /* reset bufs */
   dlbFreeAll(pDlbDynBufs);
   dlbFreeAll(pDlbFixBufs);
}

killTasks()
{
   int tid;
   int taskSrchDelete(char *basename);

   killParser();
   killAupdt();
   killXParser();
   killDwnLnk();
   killComLnk();
   killUpLnk();
   killUpLnkI();
   killPhandler();
   killShandler();
   killTune();

/*
   if ((tid = taskNameToId("tConBrker")) != ERROR)
	taskDelete(tid);
*/

   if ((tid = taskNameToId("tIntrp")) != ERROR)
	taskDelete(tid);

   taskSrchDelete("tAcptS");  /* delete those socket Accepting tasks */

   return(0);
}

/* delete All Tasks and buffers and restart */
resetAll()
{
   killTasks();

   msgQDelete(pUpLinkMsgQ);
   msgQDelete(pMsgesToHost);
   msgQDelete(pMsgesToAParser);
   msgQDelete(pMsgesToAupdt);
   msgQDelete(pMsgesToXParser); 
   msgQDelete(pMsgesToPHandlr);
   msgQDelete(pTagFifoMsgQ);
   msgQDelete(pApFifoMsgQ);
   msgQDelete(pUpLinkIMsgQ);
   msgQDelete(pUpLnkHookMsgQ);
   msgQDelete(pIntrpQ);

   fifoDelete(pTheFifoObject);
   stmDelete(pTheStmObject);
   adcDelete(pTheAdcObject);
   autoDelete(pTheAutoObject);
   acodeDelete(pTheAcodeObject);
   tuneDelete(pTheTuneObject);

   dlbDelete(pDlbDynBufs);
   dlbDelete(pDlbFixBufs);

   semDelete(pSemSAStop);
}

/*-------------------------------------------------
* systemRestart() - clean the system and restart 
*	all applications
*/
systemConRestart()
{
/*
  killTasks();
  sysReset();
  resetBufs();
  sysEnable();
  startTasks();
*/
  DPRINT(1,"systemConRestart: - \n");
  restartComLnk(); 
  restartDwnLnk();
  restartUpLnk();
   return(0);
}

/*-------------------------------------------------
* systemRestart() - clean the system and restart 
*	all applications
*/
systemRestart()
{
   resetAll();
   systemInit();
   return(0);
}

/* STM fake interrupts generator used by parser to kick the UpLinker into Action */
fakeItr(MSG_Q_ID pUpLnkMsgQ,int delayTicks)
{
   ITR_MSG itrmsge;

   /* first make msgQ */
   pIntrpQ = msgQCreate(1024,sizeof(ITR_MSG),MSG_Q_FIFO);
   DPRINT(1,"Fake Intrp:Server LOOP Ready & Waiting.\n");
   FOREVER
   {
      msgQReceive(pIntrpQ,(char*) &itrmsge,sizeof(ITR_MSG), WAIT_FOREVER);
      DPRINT1(3,"Fake Intrp: Got Tag: %ld\n",itrmsge.tag);
      msgQSend(pTheStmObject->pIntrpMsgs, (char *)(&itrmsge),sizeof(ITR_MSG),
                WAIT_FOREVER,MSG_PRI_NORMAL);
      taskDelay(delayTicks); 
   }
}
sysEnable()
{

   stmItrpEnable(pTheFifoObject, (RTZ_ITRP_MASK | RPNZ_ITRP_MASK | 
			  MAX_SUM_ITRP_MASK | APBUS_ITRP_MASK));

/*
   fifoItrpEnable(pTheFifoObject, FSTOPPED_I | FSTRTEMPTY_I | FSTRTHALT_I |
			   NETBL_I | FORP_I | NETBAP_TOUT_I | TAGFNOTEMPTY_I |
			   SW1_I | SW2_I | SW3_I | SW4_I );
*/

   fifoItrpEnable(pTheFifoObject, FSTOPPED_I | FSTRTEMPTY_I | FSTRTHALT_I |
			   NETBL_I | FORP_I | PFAMFULL_I | TAGFNOTEMPTY_I |
			   SW1_I | SW2_I | SW3_I | SW4_I );

   autoItrpEnable(pTheAutoObject, AUTO_ALLITRPS);
   return(0);
}
sysDisable()
{
  stmItrpDisable(pTheFifoObject, 0);
  fifoItrpDisable(pTheFifoObject, FF_ALLITRPS);
  autoItrpDisable(pTheAutoObject, AUTO_ALLITRPS);
   return(0);
}

sysReset()
{
   stmReset(pTheStmObject);
   fifoReset(pTheFifoObject, RESETFIFOBRD);
   fifoResetStuffing(pTheFifoObject);
   fifoFlushBuf(pTheFifoObject);
   adcReset(pTheAdcObject);
   resetSA();
   return(0);
}

sysStat()
{
  printf("\n");
  stmShow(pTheStmObject,0);
  fifoShow(pTheFifoObject,0);
  adcShow(pTheAdcObject,0);
  tuneShow(pTheTuneObject,0);
  sysSemInfo();
  printf("\n");
  sysShow();
  printf("System Version: %d\n", SystemRevId);
  printf("Interpreter Version: %d\n", InterpRevId);
   return(0);
}

syshelp()
{
   printf("Status and Stat Information commands:\n");
   printf("sysfifo(level)   - Print the State of the FIFO Object\n");
   printf("sysfiforesrc()   - Print the System Resources Used\n");
   printf("sysstm(level)    - Print the State of the STM  Object\n");
   printf("sysstmresrc()    - Print the System Resources Used\n");
   printf("sysadc(level)    - Print the State of the ADC  Object\n");
   printf("systune(level)   - Print the State of the Tune Object\n");
   printf("systuneresrc()   - Print the System Resources Used\n");
   printf("sysacode(level)  - Print the State of the Acode Object\n");
   printf("sysacodesrc()    - Print the System Resources Used\n");
   printf("sysmsgqs(level)  - Print the State of the Various Acquisition Message Queues\n");
   printf("syssems(level)   - Print the State of the Various Acquisition Semaphore\n");
   printf("sysdlbs(level)   - Print the State of the Various Name Buffers\n");
   printf("sysdlbsresrc()   - Print the System Resources Used\n");
   printf("sysChanDump(level)   - Print the bytes in the channel ports.\n");
   printf("sysresrc()       - Print the Global System Resources Used\n");
   printf("sysresrcdump()   - Print All System Resources Used\n");
}
sysfifo(level)
{
  fifoShow(pTheFifoObject,level);
}
sysfiforesrc()
{
  fifoShwResrc(pTheFifoObject,1);
}
sysstm(level)
{
  stmShowEm(level);
}
sysstmresrc()
{
  stmShwResrcEm(1);
}
sysadc(level)
{
  adcShow(pTheAdcObject,level);
}
sysacode(int level)
{
  acodeShow(pTheAcodeObject, level);
}
sysacoderesrc()
{
  acodeShwResrc(pTheAcodeObject,1);
}

systune(level)
{
  tuneShow(pTheTuneObject,level);
}
systuneresrc()
{
  tuneShwResrc(pTheTuneObject,1);
}
sysmsgqs(int level)
{
   msgQInfo(level);
}
sysresrc()
{
   printf("\n System Message Queues:\n");
   printf("    pUpLinkMsgQ ---- 0x%lx\n",pUpLinkMsgQ);
   printf("    pMsgesToHost --- 0x%lx\n",pMsgesToHost);
   printf("    pMsgesToAParser  0x%lx\n",pMsgesToAParser);
   printf("    pMsgesToAupdt -- 0x%lx\n",pMsgesToAupdt);
   printf("    pMsgesToXParser  0x%lx\n",pMsgesToXParser);
   printf("    pMsgesToPHandlr  0x%lx\n",pMsgesToPHandlr);
   printf("    pTagFifoMsgQ --- 0x%lx\n",pTagFifoMsgQ);
   printf("    pUpLinkIMsgQ --- 0x%lx\n",pUpLinkIMsgQ);
   printf("\n  System Ring Buffer:\n");
   printf("    pSyncActionArgs: 0x%lx\n",pSyncActionArgs);
   printf("\n  System Semaphore:\n");
   printf("    pSemSAStop:      0x%lx\n",pSemSAStop);
}

sysdlbs(int level)
{
  sysNamebufs(level);
}
sysdlbsresrc()
{
   printf("\n\nFixed Name Buffer\n");
   dlbShwResrc(pDlbFixBufs,1);
   printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
   printf("\n\nDynamic Name Buffer Status;\n");
   dlbShwResrc(pDlbDynBufs,1);
   printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
}
syssems(level)
{
    sysSemInfo();
    
    printf("Acode Update Semaphore: \n");
    printSemInfo(pTheAcodeObject->pSemParseUpdt,"Acode Update Semaphore",level);

    printf("Acode Control Semaphore: \n");
    printSemInfo(pTheAcodeObject->pAcodeControl,"Acode Control Semaphore",level);

    printf("Parser Suspension Semaphore: \n");
    printSemInfo(pTheAcodeObject->pSemParseSuspend,"Acode Suspension Semaphore",level);

    printf("Fifo Start Watchdog Timer: \n");
    wdShow(pTheAcodeObject->wdFifoStart);
}

sysresrcdump()
{
   sysfiforesrc();
   sysstmresrc();
   sysacoderesrc();
   systuneresrc();
   sysdlbsresrc();
   sysresrc();
   sysflagsresrc();
}

sysChanDump()
{
   int i,rbytes,wbytes;
   for(i=0; i < 5; i++)
   {
     rbytes = bytesInChannel(i);
     wbytes = bytesInChannelW(i);
     DPRINT3(-1,"Channel: %d, Bytes in: %d - Readport, %d - Writeport\n",i,rbytes,wbytes);
   }
}


/*---------------------------------------------------------
*  taskSrchDelete - delete all task with the basename passed
*
*/
taskSrchDelete(char *basename)
{
    FAST int    nTasks;                 /* number of task */
    FAST int    ix;                     /* index */
    int         idList [1024]; /* list of active IDs */
    int		baselen;
    char 	*tName;
    char 	tcname[40];
    char	*taskName(int tid);
  
    baselen = strlen(basename);
    /* printf("basename='%s', len=%d\n",basename,baselen); */

    nTasks = taskIdListGet (idList, 1024);
    taskIdListSort (idList, nTasks);
 
    for (ix = 0; ix < nTasks; ++ix)
    {
      tName = taskName(idList[ix]);
      if ( tName != NULL )
      {
	memcpy(tcname,tName,baselen);  /* just compare the basename char */
        tcname[baselen] = '\0';
        /*printf("base:'%s', task:'%s', taskf:'%s'\n",basename,tcname,tName);*/
        if (strcmp(basename,tcname) == 0)
	   taskDelete(idList [ix]);
      }
    }
   return(0);
}
