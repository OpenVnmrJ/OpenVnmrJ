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
#define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#include <vxWorks.h>
#include <stdlib.h>
#include <msgQLib.h>
#include <semLib.h>
#include <vme.h>
#include <iv.h>

#include "instrWvDefines.h"
#include "logMsgLib.h"
#include "vmeIntrp.h"
#include "hardware.h"
#include "timeconst.h"
#include "commondefs.h"
#include "fifoObj.h"
#include "autoObj.h"
#include "consoleStat.h"
#include "hostAcqStructs.h"
#include "mboxcmds.h"
#include "errorcodes.h"


/*
modification history
--------------------
1-28-95,gmb  created 
*/

#ifndef DEBUG_AUTO_SHIMS
#define DEBUG_AUTO_SHIMS 9
#endif

/*

Automation Board Interface Object Routines

   Interrupt Service Routines (ISR)

*/
#ifdef DEBUG
typedef struct __dcodestruc {
		     int code;
		     char *codestr;
                  } dcodeentry;

static dcodeentry donecode2msg[15] = {	{ 0 , "NO-OP" },
				{ GET_LOCK_VALUE, "Get Lock Value" },
				{ GET_SHIMS_PRESENT, "Get Shim Set Type" },
				{ GET_SERIAL_SHIMS_PRESENT, "Are Serial Shims Present" },
				{ GET_SPIN_VALUE, "Get Spinner Speed" },
				{ EJECT_SAMPLE, "Eject Spinner" },
				{ INSERT_SAMPLE, "Insert Spinner" },
				{ SET_SPIN_RATE, "Set Spinner Flow Rate" },
				{ SET_SPIN_SPEED, "Set Spinner Speed" },
				{ SET_SPIN_MAS_THRESHOLD, "Set Liq/MAS Switch Over Speed" },
				{ SET_SPIN_REG_DELTA, "Set Spinner Regulation Delta" },
				{ BEARING_ON, "Turn Bearing Air On" },
				{ BEARING_OFF, "Turn Bearing Air Off" },
				{ SAMPLE_DETECT, "Is Sample Detected in Probe" },
				{ SET_DEBUGLEVEL, "Set Debugging Level" } };
#define MBOXCMDSIZE 15

#endif

extern FIFO_ID	pTheFifoObject;
extern MSG_Q_ID pMsgesToPHandlr;

/* Exception Msges to Phandler, e.g. FOO, etc. */
extern EXCEPTION_MSGE HardErrorException;
extern EXCEPTION_MSGE GenericException;

static AUTO_OBJ *autoObj;
static char *IDStr ="Automation Object";
static int  IdCnt = 0;

/* VME Interrupt posted by the automation 332 for Exception conditions */
static SEM_ID pSemAutoExcpt = NULL;


/* VME Blocking Semaphore release when Acknowledge received from 332 */
static SEM_ID pSemGenMBox = NULL;
static SEM_ID pSemShimMBox = NULL;
static SEM_ID pSemSpinMBox = NULL;
static SEM_ID pSemVTMBox = NULL;

/* Mutex Semaphores so only one task may access a mail box at a time */
static SEM_ID pMutexGenMBox = NULL;
static SEM_ID pMutexShimMBox = NULL;
static SEM_ID pMutexSpinMBox = NULL;
static SEM_ID pMutexVTMBox = NULL;

static  int   sysClkRate = 60;
static int autoEnquiry(AUTO_ID pAutoId, int cmd, char* retval,int size,double timeout);
void printAutoEr(char* str, int cmd, int arg, int error);
/* void printAutoEr(char* str, int error); */

#define AUTO_ACK_IST_PRIORITY 60
#define AUTO_EXCPT_IST_PRIORITY 60
#define AUTO_IST_STACK_SIZE 2048
#define AUTO_IST_TASK_OPTIONS 0

#define MAX_TRYS 20

#ifdef DEBUG
char *getmboxcmd(int dcode);
#endif

/*-----------------------------------------------------------
|
|  Internal Functions
|
+---------------------------------------------------------*/
/*-------------------------------------------------------------
| Interrupt Service Routines (ISR) 
+--------------------------------------------------------------*/

/***********************************************************
*
* 332 VME Cmd Acknowledge - Interrupt Service Tasks 
*
* Actions to be taken:
*/
static VOID genMsgAck(AUTO_ID pAutoId)
{
#ifdef INSTRUMENT
  wvEvent(EVENT_AUTO_GENMSG_ACK,NULL,NULL);
#endif
  semGive(pSemGenMBox);
  /* DPRINT(0,"Gen. Cmd Ack"); */
}
static VOID spinMsgAck(AUTO_ID pAutoId)
{
#ifdef INSTRUMENT
  wvEvent(EVENT_AUTO_SPINMSG_ACK,NULL,NULL);
#endif
  semGive(pSemSpinMBox);
  /* DPRINT(1,"Spin. Cmd Ack"); */
}
static VOID shimMsgAck(AUTO_ID pAutoId)
{
#ifdef INSTRUMENT
  wvEvent(EVENT_AUTO_SHIMMSG_ACK,NULL,NULL);
#endif
  semGive(pSemShimMBox);
  /* DPRINT(0,"Shim. Cmd Ack"); */
}
static VOID vtMsgAck(AUTO_ID pAutoId)
{
#ifdef INSTRUMENT
  wvEvent(EVENT_AUTO_VTMSG_ACK,NULL,NULL);
#endif
  semGive(pSemVTMBox);
  /* DPRINT(0,"VT. Cmd Ack"); */
}

/***********************************************************
*
* 332 VME Exception - Interrupt Service Task 
*
* Actions to be taken:
*/
static VOID autoException(AUTO_ID pAutoId)
{
   short status;

   FOREVER
   {
      semTake(pSemAutoExcpt,WAIT_FOREVER); /* wait here for interrupt */

      status = *AUTO_STATR(pAutoId->autoBaseAddr);
      if (status & SPIN_NOT_REG)
      {
#ifdef INSTRUMENT
         wvEvent(EVENT_AUTO_SPINREG_ERR,NULL,NULL);
#endif
         /* errLogRet(LOGIT,debugInfo,"Spinner went Out of Regulation"); */
	 DPRINT(0,"Spinner went Out of Regulation");
      }
      else if (status & NOT_LOCKED)
      {
#ifdef INSTRUMENT
         wvEvent(EVENT_AUTO_LOCKED_ERR,NULL,NULL);
#endif
	 DPRINT(0,"Sample Lock Lost");
      }
      else if (status & VT_ATTEN )
      {
#ifdef INSTRUMENT
         wvEvent(EVENT_AUTO_VTREG_ERR,NULL,NULL);
#endif
	 DPRINT(0,"VT Went out of Regulation");
      }
      else
      {
#ifdef INSTRUMENT
       wvEvent(EVENT_AUTO_UNKNOWN_ERR,NULL,NULL);
#endif
	DPRINT1(0,"Illegal Status: 0x%x",status);
      }
   }
}


/**************************************************************
*
*  autoCreate - create the Automation Object Data Structure & Semaphore
*
*
* RETURNS:
* OK - if no error, NULL - if mallocing or semaphore creation failed
*
*/ 
AUTO_ID  autoCreate(unsigned long baseAddr, unsigned long memAddr, unsigned long sharMemOffset, int apBusAddr, int vector, int level, char* idstr)
/* unsigned long baseAddr - base address of FIFO */
/* unsigned long memAddr - base address of Automation DRAM */
/* unsigned long sharMemOffset - offset to Automation DRAM used as Shared Memory */
/* int apBusAddr  - Ap Bus address*/
/* int   vector  - VME Interrupt vector number */
/* int   level   - VME Interrupt level */
/* char* idstr - user indentifier string */
{
   static VOID genMsgAck(AUTO_ID pAutoId);
   static VOID spinMsgAck(AUTO_ID pAutoId);
   static VOID shimMsgAck(AUTO_ID pAutoId);
   static VOID vtMsgAck(AUTO_ID pAutoId);
   static VOID autoException(AUTO_ID pAutoId);

   char tmpstr[80];
   register AUTO_OBJ *pAutoObj;
   short sr;
   int i,vec1,vec2,vec3,vec4,vec5;
   int tExcptPid;
   unsigned long prevalue,value;

   acqerrno = 0;

  /* ------- malloc space for FIFO Object --------- */
  if ( (pAutoObj = (AUTO_OBJ *) malloc( sizeof(AUTO_OBJ)) ) == NULL )
  {
    errLogSysRet(LOGIT,debugInfo,"autoCreate: Could not Allocate Space:");
    return(NULL);
  }

  /* zero out structure so we don't free something by mistake */
  memset(pAutoObj,0,sizeof(AUTO_OBJ));

  /* ------ Translate Bus address to CPU Board local address ----- */
  if (sysBusToLocalAdrs(AUTO_VME_ACCESS_TYPE,
              ((long)baseAddr & 0xffffff),&(pAutoObj->autoBaseAddr)) == -1)
  {
    errLogRet(LOGIT,debugInfo,
       "autoCreate: Can't Obtain Bus(0x%lx) to Local Address.",
	  baseAddr);
    autoDelete(pAutoObj);
    return(NULL);
  }

  /* ------ Translate Bus address to CPU Board local address ----- */
  if (sysBusToLocalAdrs(AUTO_MEM_ACCESS_TYPE,
                         memAddr,&(pAutoObj->autoMemAddr)) == -1)
  {
    errLogRet(LOGIT,debugInfo,
       "autoCreate: Can't Obtain Memory Bus(0x%lx) to Local Address.",
	  memAddr);
    autoDelete(pAutoObj);
    return(NULL);
  }

  /* ------ Translate Bus address to CPU Board local address ----- */
  if (sysBusToLocalAdrs(AUTO_MEM_ACCESS_TYPE,
                 (memAddr+sharMemOffset),&(pAutoObj->autoSharMemAddr)) == -1)
  {
    errLogRet(LOGIT,debugInfo,
       "autoCreate: Can't Obtain Memory Bus(0x%lx) to Local Address.",
	  (memAddr+sharMemOffset));
    autoDelete(pAutoObj);
    return(NULL);
  }

  /* Check Base Vector for validity */
/* --------- Automation interupt vector numbers ------------ */
/* Each Addition Automation board, add 1 to vector number */

  if ( (vector == AUTO_ITRP_VEC) )
  {
     pAutoObj->vmeItrVector = vector;
  }
  else
  {
        errLogRet(LOGIT,debugInfo,
	  "autoCreate: Invalid Base Vector: 0x%x (Valid: 0x%x)\n",
	   vector,AUTO_ITRP_VEC);
        autoDelete(pAutoObj);
        return(NULL);
  }

  pAutoObj->vmeItrLevel = 2;
  pAutoObj->ApBusAddr = apBusAddr;

  /* ------ Create Id String ---------- */
  IdCnt++;
  if (idstr == NULL) 
  {
     sprintf(tmpstr,"%s %d\n",IDStr,IdCnt);
     pAutoObj->pIdStr = (char *) malloc(strlen(tmpstr)+2);
  }
  else
  {
     pAutoObj->pIdStr = (char *) malloc(strlen(idstr)+2);
  }

  if (pAutoObj->pIdStr == NULL)
  {
     autoDelete(pAutoObj);
     errLogSysRet(LOGIT,debugInfo,
	"autoCreate: IdStr - Could not Allocate Space:");
     return(NULL);
  }

  if (idstr == NULL) 
  {
     strcpy(pAutoObj->pIdStr,tmpstr);
  }
  else
  {
     strcpy(pAutoObj->pIdStr,idstr);
  }

  pSemAutoExcpt = semBCreate(SEM_Q_FIFO,SEM_EMPTY);
  pSemGenMBox = semBCreate(SEM_Q_FIFO,SEM_EMPTY);
  pSemShimMBox = semBCreate(SEM_Q_FIFO,SEM_EMPTY);
  pSemSpinMBox = semBCreate(SEM_Q_FIFO,SEM_EMPTY);
  pSemVTMBox = semBCreate(SEM_Q_FIFO,SEM_EMPTY);
  
  if ( (pSemAutoExcpt == NULL)  ||
       (pSemGenMBox == NULL) || (pSemShimMBox == NULL)  ||
       (pSemSpinMBox == NULL) || (pSemVTMBox == NULL)
     )
  {
     autoDelete(pAutoObj);
     errLogSysRet(LOGIT,debugInfo,
	"autoCreate: IdStr - Could not Allocate Semaphore Space:");
     return(NULL);
  }

  /* create the MailBox Mutual Exclusion semaphores */
  pMutexGenMBox = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE |
                                  SEM_DELETE_SAFE);
  pMutexShimMBox = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE |
                                  SEM_DELETE_SAFE);
  pMutexSpinMBox = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE |
                                  SEM_DELETE_SAFE);
  pMutexVTMBox = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE |
                                  SEM_DELETE_SAFE);

  if ( (pMutexGenMBox == NULL) || (pMutexShimMBox == NULL)  ||
       (pMutexSpinMBox == NULL) || (pMutexVTMBox == NULL) 
     )
  {
     autoDelete(pAutoObj);
     errLogSysRet(LOGIT,debugInfo,
	"autoCreate: IdStr - Could not Allocate Semaphore Space:");
     return(NULL);
  }

  sysClkRate = sysClkRateGet();

  /* ------ Test for Boards Presents ---------- */
  if ( vxMemProbe((char*) (pAutoObj->autoBaseAddr + AUTO_SR), 
		     VX_READ, 2, &sr) == ERROR)
  { 
    errLogRet(LOGIT,debugInfo,
       "autoCreate: Could not read Automation's Status register(0x%lx), Board 0x%lx NOT Present\n",
		(pAutoObj->autoBaseAddr + AUTO_SR), pAutoObj->autoBaseAddr);

    /* set errno to AUTO CARD Misssing */
    acqerrno = WARNINGS + AUTOMISSING; /* errorcode */

    if (IdCnt > 1)
    {
      autoDelete(pAutoObj);
      return(NULL);
    }
    else
    {
      pAutoObj->autoBaseAddr = 0xFFFFFFFF;
      pAutoObj->autoMemAddr = 0xFFFFFFFF;
      pAutoObj->autoSharMemAddr = 0xFFFFFFFF;
      pAutoObj->autoBrdVersion = 0xee;
      return(pAutoObj);
    }
  }
  else
  {
     /* read from Diagnostic PROM */
#ifdef PROM_INSTALLED
     pAutoObj->autoBrdVersion = FF_REG(pAutoObj->autoBaseAddr,0);
     DPRINT2(1,"autoCreate: Automation Board Version %d (0x%x) present.\n",
		pAutoObj->autoBrdVersion,pAutoObj->autoBrdVersion);
#else
    pAutoObj->autoBrdVersion = 0xff;
     DPRINT2(1,"autoCreate: Skipped PROM, made Automation Board Version %d (0x%x)\n",
		pAutoObj->autoBrdVersion,pAutoObj->autoBrdVersion);
#endif
  }


  /* ----- reset board and get status register */
  /* autoReset(pAutoObj,AUTO_RESET_ALL); /* reset board */

  /*------ Disable all mbox interrupts on board, leave QUART & MCU332 interrupts alone  -----------*/
  autoItrpDisable(pAutoObj,GNRL_MAIL_I | SPIN_MAIL_I | VT_MAIL_I | SHIM_MAIL_I);

  /* ------- Connect VME interrupt vector to proper Semaphore to Give ----- */
#ifdef INSTRUMENT

   vec1 = intConnect( INUM_TO_IVEC(pAutoObj->vmeItrVector + GEN_ACK_VEC ), 
			genMsgAck, pAutoObj);
   vec4 = intConnect( INUM_TO_IVEC(pAutoObj->vmeItrVector + SHIM_ACK_VEC ),
			shimMsgAck, pAutoObj);
   vec5 = intConnect( INUM_TO_IVEC(pAutoObj->vmeItrVector + AUTO_EXCPT_VEC ),
			autoException, pAutoObj);

#else

   vec1 = intConnect( INUM_TO_IVEC(pAutoObj->vmeItrVector + GEN_ACK_VEC ), 
			semGive, pSemGenMBox);

   vec4 = intConnect( INUM_TO_IVEC(pAutoObj->vmeItrVector + SHIM_ACK_VEC ), 
			semGive, pSemShimMBox);

   vec5 = intConnect( INUM_TO_IVEC(pAutoObj->vmeItrVector + AUTO_EXCPT_VEC ),
			semGive, pSemAutoExcpt);
#endif

/*
   vec2 = intConnect( INUM_TO_IVEC(pAutoObj->vmeItrVector + SPIN_ACK_VEC ), semGive, pSemSpinMBox);
   vec3 = intConnect( INUM_TO_IVEC(pAutoObj->vmeItrVector + VT_ACK_VEC ), semGive, pSemVTMBox);
*/

/*
   vec1 = intConnect( INUM_TO_IVEC(pAutoObj->vmeItrVector + GEN_ACK_VEC ), 
			genMsgAck, pAutoObj);
   vec4 = intConnect( INUM_TO_IVEC(pAutoObj->vmeItrVector + SHIM_ACK_VEC ),
			shimMsgAck, pAutoObj);
   vec5 = intConnect( INUM_TO_IVEC(pAutoObj->vmeItrVector + AUTO_EXCEPT_VEC ),
			autoException, pAutoObj);
*/


   vec2 = intConnect( INUM_TO_IVEC(pAutoObj->vmeItrVector + SPIN_ACK_VEC ), 
			spinMsgAck, pAutoObj);

   vec3 = intConnect( INUM_TO_IVEC(pAutoObj->vmeItrVector + VT_ACK_VEC ), 
			vtMsgAck, pAutoObj);

   if ( (vec1 == ERROR) || (vec2 == ERROR) || (vec3 == ERROR) || (vec4 == ERROR) || 
	(vec5 == ERROR) )
   {
     errLogSysRet(LOGIT,debugInfo,
	"autoCreate: Could not connect Automation interrupt vector: ");
     autoDelete(pAutoObj);
     return(NULL);
   }

   /* ------- Spawn the Interrupt Service Tasks -------- */

     tExcptPid = taskSpawn("tAutoExcpt", AUTO_EXCPT_IST_PRIORITY, AUTO_IST_TASK_OPTIONS,
		AUTO_IST_STACK_SIZE, autoException, pAutoObj,ARG2,
		ARG3,ARG4,ARG5,ARG6,ARG7,ARG8,ARG9,ARG10);

     if ( tExcptPid == ERROR)
     {
        errLogSysRet(LOGIT,debugInfo,
           "autoCreate: could not spawn Automation Exception IST");
        autoDelete(pAutoObj);
        return(NULL);
     }
    

    DPRINT1(0,"AUTOMATION Card 0x%lx PRESENT.\n", pAutoObj->autoBaseAddr);

   /* Now Wait for Automation Card Bootup HeartBeat */

   DPRINT1(1,"AUTO_HEARTBEAT at: 0x%lx \n", AUTO_HEARTBEAT(pAutoObj->autoMemAddr));

   /* set errno to AUTO CARD Misssing */
   acqerrno = WARNINGS + AUTO_NOTBOOTED; /* errorcode */

   for (i=0; i < MAX_TRYS; i++)
   {
     taskDelay(sysClkRate * 3);
     value = *AUTO_HEARTBEAT(pAutoObj->autoMemAddr);
     /* DPRINT1(1,"HeartBeat: %lu\n",value); */
     if (i == 0)
       prevalue = value;
     else
     {
        if (value != prevalue)
        {
	   DPRINT(0,"HeartBeat OK\n");
	   acqerrno = 0;	/* reset errno to no error */
	   break;
	}
	else
	{
	   DPRINT1(0,"HeartBeat = %lu\n",value);
	}
     }
   }
   return( pAutoObj );
}


/**************************************************************
*
*  autoDelete - Deletes Automation Object and  all resources
*
*
* RETURNS:
*  OK or ERROR
*
*	Author Greg Brissey 10/1/93
*/
int autoDelete(AUTO_ID pAutoId)
/* AUTO_ID 	pAutoId - Automation Object identifier */
{
   int tid;

   if (pAutoId != NULL)
   {

      if ((tid = taskNameToId("tAutoExcpt")) != ERROR)
         taskDelete(tid);
      if (pSemAutoExcpt != NULL)
	semDelete(pSemAutoExcpt);
      if (pSemGenMBox != NULL)
	semDelete(pSemGenMBox);
      if (pSemShimMBox != NULL)
	semDelete(pSemShimMBox);
      if (pSemSpinMBox != NULL)
	semDelete(pSemSpinMBox);
      if (pSemVTMBox != NULL)
	semDelete(pSemVTMBox);
      if (pMutexGenMBox != NULL)
	semDelete(pMutexGenMBox);
      if (pMutexShimMBox != NULL)
	semDelete(pMutexShimMBox);
      if (pMutexSpinMBox != NULL)
	semDelete(pMutexSpinMBox);
      if (pMutexVTMBox != NULL)
	semDelete(pMutexVTMBox);

      if (pAutoId->pIdStr != NULL)
	 free(pAutoId->pIdStr);
      free(pAutoId);
   }
}


/**************************************************************
*
*  autoItrpEnable - Set the Automation Interrupt Mask
*
*  This routines set the VME interrupt mask of the Automation. 
*
* RETURNS:
* void 
*
*/ 
void autoItrpEnable(AUTO_ID pAutoId, int mask)
/* AUTO_ID 	pAutoId - Automation Object identifier */
/* int mask;	 mask of interrupts to enable */
{
   if ( (pAutoId == NULL) || (pAutoId->autoBaseAddr == 0xFFFFFFFF))
      return;

   *AUTO_IMASK(pAutoId->autoBaseAddr) =  
	    ((*AUTO_IMASK(pAutoId->autoBaseAddr)) & ~mask);
}

/**************************************************************
*
*  autoItrpDisable - Set the Automation Interrupt Mask
*
*  This routines set the VME interrupt mask of the Automation. 
*
* RETURNS:
* void 
*
*/ 
void autoItrpDisable(AUTO_ID pAutoId, int mask)
/* AUTO_ID 	pAutoId - Automation Object identifier */
/* int mask;	 mask of interrupts to disable */
{
   if ( (pAutoId == NULL) || (pAutoId->autoBaseAddr == 0xFFFFFFFF))
      return;

   *AUTO_IMASK(pAutoId->autoBaseAddr) = 
		((*AUTO_IMASK(pAutoId->autoBaseAddr)) | mask);
}

/**************************************************************
*
*  autoReset - Resets combinations of Automation Board functions 
*
*  Functions resetable - state machine, FIFO, APbus & High
*  			 Speed Lines
*
* RETURNS:
* 
*/
void autoReset(AUTO_ID pAutoId, int options)
/* pAutoId - auto Object identifier */
/* options - those FIFO functions to be reset */
{
   int state;

   if ( (pAutoId == NULL) || (pAutoId->autoBaseAddr == 0xFFFFFFFF))
      return;

#ifdef INSTRUMENT
   wvEvent(EVENT_AUTO_RESET,NULL,NULL);
#endif

   *AUTO_CNTRL(pAutoId->autoBaseAddr) = 0xff & options;
}

/**************************************************************
*
*  autoStatReg - Gets Automation status register value
*
*
* RETURNS:
*  16-bit Automation Status Register Value
*/
short autoStatReg(AUTO_ID pAutoId)
/* pAutoId - auto Object identifier */
{
   if ( (pAutoId == NULL) || (pAutoId->autoBaseAddr == 0xFFFFFFFF))
      return(1);

    return (*AUTO_STATR(pAutoId->autoBaseAddr));
}

/**************************************************************
*
*  autoCntrlReg - Gets Automation control register value
*
*
* RETURNS:
*  16-bit Automation Control Register Value
*/
short autoCntrlReg(AUTO_ID pAutoId)
/* pAutoId - auto Object identifier */
{
   if ( (pAutoId == NULL) || (pAutoId->autoBaseAddr == 0xFFFFFFFF))
      return(1);

    return (*AUTO_CNTRL(pAutoId->autoBaseAddr));
}



/**************************************************************
*
*  autoIntrpMask - Gets Automation Interrupt Register mask
*
*
* RETURNS:
*  16-bit FIFO Interrupt Mask Value
*/
short autoIntrpMask(AUTO_ID pAutoId)
/* pAutoId - auto Object identifier */
{
   if ( (pAutoId == NULL) || (pAutoId->autoBaseAddr == 0xFFFFFFFF))
      return(1);

    return ((*AUTO_IMASK(pAutoId->autoBaseAddr)) & AUTO_ALLITRPS);
}

/**************************************************************
*
*  autoSetDebugLevel - set debug on automation Card 
*
*
* RETURNS:
*  status  OK or Error code
*/
autoSetDebugLevel(AUTO_ID pAutoId,int dlevel)
{
   int stat;
   MBOX_CMD_SIZE cmds[2];

   if ( (pAutoId == NULL) || (pAutoId->autoBaseAddr == 0xFFFFFFFF))
      return(-1);

   cmds[0] = SET_DEBUGLEVEL;
   cmds[1] = (DEBUGLEVEL_VALUE_SIZE) dlevel;
   /* send message to automation to eject sample */
   stat = autoSpinMsgSend(pAutoId,(char*) &cmds, sizeof(cmds), QCMD_TIMEOUT);
   if (stat != OK)
   {
       printAutoEr("autoSetDebugLevel", SET_DEBUGLEVEL, dlevel, stat);
   }
   return(stat);
}

ulong_t autoGetHeartBeat(AUTO_ID pAutoId)
{
   if ( (pAutoId == NULL) || (pAutoId->autoBaseAddr == 0xFFFFFFFF))
      return(0L);

   return(*AUTO_HEARTBEAT(pAutoId->autoMemAddr));
}

/**************************************************************
*
*  autoSampleEject - Eject Sample via the Automation Card 
*
*
* RETURNS:
*  status  OK or Error code
*/
int autoSampleEject(AUTO_ID pAutoId)
/* pAutoId - auto Object identifier */
{
   int stat;
   MBOX_CMD_SIZE spincmd;
   short spinstat;

   if ( (pAutoId == NULL) || (pAutoId->autoBaseAddr == 0xFFFFFFFF))
      return(-1);


   stat = OK;
   spinstat = *AUTO_STATR(pAutoId->autoBaseAddr);
   if ( spinstat & EJECT_AIR_ON)
   {
      DPRINT1(1,"autoSampleEject: Eject already On. (0x%x)",spinstat);
   }
   else
   {
       DPRINT(1,"autoSampleEject: Eject.\n");
       spincmd = EJECT_SAMPLE;

       /* send message to automation to eject sample */
       stat = autoSpinMsgSend(pAutoId,(char*) &spincmd, sizeof(spincmd),LCMD_TIMEOUT);
       if (stat != OK)
       {
           printAutoEr("autoSampleEject", EJECT_SAMPLE, 0, stat);
       }
   }
   return(stat);
}

/**************************************************************
*
*  autoSampleBump - Bump Sample up & down to help get it spinning 
*
*
* RETURNS:
*  status  OK or Error code
*/
int autoSampleBump(AUTO_ID pAutoId)
/* pAutoId - auto Object identifier */
{
   volatile int zero = 0;  /* so not optimized away */
   int stat,cnt;
   DPRINT(1,"autoSampleBump: \n");
   /* Eject */
   *AUTO_SPINCNTRL(pAutoId->autoBaseAddr) = 
			(TURN_EJECT_BIT | TURN_BEAR_BIT | TURN_SDROP_BIT);
   taskDelay(6);
   /* Insert */
   *AUTO_SPINCNTRL(pAutoId->autoBaseAddr) = zero;/*don't what clr instruction*/
   taskDelay(150);  /* wait 2.5 sec */

   /* check for 2 more sec at most to detect sample in probe */
   for (cnt=0; cnt < 8; cnt++)
   {
       stat = autoSampleDetect(pAutoId);
       if (stat == 1)
       {
           DPRINT(2,"autoSampleBump: Sample Detected\n");
           break;
       }
       taskDelay(30);  /* check each .5 sec */
   } 
   /* Bearing On */
   *AUTO_SPINCNTRL(pAutoId->autoBaseAddr) = TURN_BEAR_BIT; 
   
   return(OK);
}



/**************************************************************
*
*  autoSpinRateSet - Set Spin Rate of Sample via the Automation Card 
*
*
* RETURNS:
*  status  OK or Error code
*/
int autoSpinRateSet(AUTO_ID pAutoId,int rate)
/* pAutoId - auto Object identifier */
/* rate -    spin rate in Hertz */
{
   int stat;
   struct {
	    MBOX_CMD_SIZE spincmd;
	    SET_SPIN_ARG_SIZE spinrate;
          } cmdstream;

   if ( (pAutoId == NULL) || (pAutoId->autoBaseAddr == 0xFFFFFFFF))
      return(-1);

   DPRINT1(0,"autoSpinRateSet: rate %d\n",rate);

   stat = OK;
   cmdstream.spincmd = SET_SPIN_RATE;
   cmdstream.spinrate = rate;

   /* send message to automation to eject sample */
   stat = autoSpinMsgSend(pAutoId,(char*) &cmdstream, sizeof(cmdstream),LCMD_TIMEOUT);
   if (stat != OK)
   {
       printAutoEr("autoSpinRateSet", SET_SPIN_RATE, rate, stat);
   }
   return(stat);
}

/**************************************************************
*
*  autoSpinSpeedSet - Set Spin Speed of Sample via the Automation Card 
*
*
* RETURNS:
*  status  OK or Error code
*/
int autoSpinSpeedSet(AUTO_ID pAutoId,int speed)
/* pAutoId - auto Object identifier */
/* speed -    spin speed in Hertz */
{
   int stat;
   struct {
	    MBOX_CMD_SIZE spincmd;
	    SET_SPIN_ARG_SIZE spinspeed;
          } cmdstream;

   if ( (pAutoId == NULL) || (pAutoId->autoBaseAddr == 0xFFFFFFFF))
      return(-1);


   DPRINT1(0,"autoSpinSpeedSet: speed %d\n",speed);

   stat = OK;
   cmdstream.spincmd = SET_SPIN_SPEED;
   cmdstream.spinspeed = speed;

   /* send message to automation to eject sample */
   stat = autoSpinMsgSend(pAutoId,(char*) &cmdstream, sizeof(cmdstream),LCMD_TIMEOUT);
   /* DPRINT1(0,"autoSpinSpeedSet: returned stat: %d\n",stat); */
   if (stat != OK)
   {
       printAutoEr("autoSpinSpeedSet", SET_SPIN_SPEED, speed, stat);
   }
   return(stat);
}

/**************************************************************
*
*  autoSpinSetMASThres  - Set Spin Speed Threshold of MAS (Magic Angle Spinner) 
*			  switch over.
*
*
* RETURNS:
*  status  OK or Error code
*/
int autoSpinSetMASThres(AUTO_ID pAutoId,int threshold)
/* pAutoId - auto Object identifier */
{
   int stat;
   struct {
	    MBOX_CMD_SIZE spincmd;
	    SET_SPIN_ARG_SIZE MASthres;
          } cmdstream;

   if ( (pAutoId == NULL) || (pAutoId->autoBaseAddr == 0xFFFFFFFF))
      return(-1);

   stat = OK;
   cmdstream.spincmd = SET_SPIN_MAS_THRESHOLD;
   cmdstream.MASthres = threshold;

   /* send message to automation to eject sample */
   stat = autoSpinMsgSend(pAutoId,(char*) &cmdstream, sizeof(cmdstream),LCMD_TIMEOUT);
   /* DPRINT1(0,"autoSpinSetMASThres: returned stat: %d\n",stat); */
   if (stat != OK)
   {
       printAutoEr("autoSpinSetMASThres", SET_SPIN_MAS_THRESHOLD, threshold, stat);
   }
   return(stat);
}
/**************************************************************
*
*  autoSpinSetRegDelta  - Set Spin Regulation Delta
*
*
* RETURNS:
*  status  OK or Error code
*/
int autoSpinSetRegDelta(AUTO_ID pAutoId,int hz)
/* pAutoId - auto Object identifier */
/* hz - delta speed(Hz) */
{
   int stat;
   struct {
	    MBOX_CMD_SIZE spincmd;
	    SET_SPIN_ARG_SIZE MASthres;
          } cmdstream;

   if ( (pAutoId == NULL) || (pAutoId->autoBaseAddr == 0xFFFFFFFF))
      return(-1);

   stat = OK;
   cmdstream.spincmd = SET_SPIN_REG_DELTA;
   cmdstream.MASthres = hz;

   /* send message to automation to eject sample */
   stat = autoSpinMsgSend(pAutoId,(char*) &cmdstream, sizeof(cmdstream),LCMD_TIMEOUT);
   if (stat != OK)
   {
       printAutoEr("autoSpinSetMASThres", SET_SPIN_REG_DELTA, hz, stat);
   }
   return(stat);
}


/**************************************************************
*
*  autoSampleInsert - Insert Sample via the Automation Card 
*
*
* RETURNS:
*  status  OK or Error code
*/
int autoSampleInsert(AUTO_ID pAutoId)
/* pAutoId - auto Object identifier */
{
   int stat;
   MBOX_CMD_SIZE spincmd;
   short spinstat;
   char  spincntrl;

   if ( (pAutoId == NULL) || (pAutoId->autoBaseAddr == 0xFFFFFFFF))
      return(-1);

   DPRINT(0,"autoSampleInsert: \n");
   stat = OK;
   spinstat = *AUTO_STATR(pAutoId->autoBaseAddr);
   if ( spinstat & EJECT_AIR_ON)
   {
       /* send message to automation to insert sample */
       spincmd = INSERT_SAMPLE;

       /* send message to automation to eject sample */
       stat = autoSpinMsgSend(pAutoId,(char*) &spincmd, sizeof(spincmd),LCMD_TIMEOUT);
       if (stat != OK)
       {
           printAutoEr("autoSampleInsert", INSERT_SAMPLE, 0, stat);
       }
      /* turn off Eject * Bearing Air */
/*
      *AUTO_SPINCNTRL(pAutoId->autoBaseAddr) = TURN_SDROP_BIT;
      taskDelay(sysClkRate * 5);
      *AUTO_SPINCNTRL(pAutoId->autoBaseAddr) = 0;
      taskDelay(sysClkRate * 1);
      *AUTO_SPINCNTRL(pAutoId->autoBaseAddr) = TURN_BEAR_BIT;
*/
      spinstat = *AUTO_STATR(pAutoId->autoBaseAddr);
      if ( spinstat & SMPLE_AT_BOT) /* Sample at Bottom of Magnet Bore  */
      {
         DPRINT1(0,"autoSampleInsert: Spinner in Probe Confirmed. (0x%x)",
			spinstat);
      }
      else
      { 
         DPRINT1(0,"autoSampleInsert: No Spinner in Probe. (0x%x)",
			spinstat);
      }
   }
   else
   {
      DPRINT1(0,"autoSampleInsert: Eject already Off. (0x%x)",spinstat);
   }
   return(stat);
}

/**************************************************************
*
*  autoBearingOn - Turn on Bearing Air
*
*
* RETURNS:
*  status  OK or Error code
*/
int autoBearingOn(AUTO_ID pAutoId)
/* pAutoId - auto Object identifier */
{
   int stat;
   MBOX_CMD_SIZE spincmd;
   short spinstat;

   if ( (pAutoId == NULL) || (pAutoId->autoBaseAddr == 0xFFFFFFFF))
      return(-1);


   DPRINT(0,"autoBearingOn: \n");
   stat = OK;
   spinstat = *AUTO_STATR(pAutoId->autoBaseAddr);
   if ( spinstat & BEAR_AIR_ON)
   {
      DPRINT1(0,"autoBearingOn: Bearin Air already On. (0x%x)",spinstat);
   }
   else
   {
       spincmd = BEARING_ON;

       /* send message to automation to eject sample */
       stat = autoSpinMsgSend(pAutoId,(char*) &spincmd, sizeof(spincmd),LCMD_TIMEOUT);
       if (stat != OK)
       {
           printAutoEr("autoBearingOn", BEARING_ON, 0, stat);
       }
   }
   return(stat);
}


/**************************************************************
*
*  autoBearingOff - Turn off Bearing Air
*
*
* RETURNS:
*  status  OK or Error code
*/
int autoBearingOff(AUTO_ID pAutoId)
/* pAutoId - auto Object identifier */
{
   int stat;
   MBOX_CMD_SIZE spincmd;
   short spinstat;

   if ( (pAutoId == NULL) || (pAutoId->autoBaseAddr == 0xFFFFFFFF))
      return(-1);


   DPRINT(0,"autoBearingOff: \n");
   stat = OK;
   spinstat = *AUTO_STATR(pAutoId->autoBaseAddr);
   if ( spinstat & BEAR_AIR_ON)
   {
       spincmd = BEARING_OFF;

       /* send message to automation to eject sample */
       stat = autoSpinMsgSend(pAutoId,(char*) &spincmd, sizeof(spincmd),LCMD_TIMEOUT);
       if (stat != OK)
       {
           printAutoEr("autoBearingOff", BEARING_OFF, 0, stat);
       }
   }
   else
   {
      DPRINT1(0,"autoBearingOff: Bearin Air already Off. (0x%x)",spinstat);
   }
   return(stat);
}


/**************************************************************
*
*  autoSpinZero - Return state of Spinner Zero
*
*
* RETURNS:
*  1 = Spinner at Zero, 0 = NOT Zero,  -1 = Error
*/
int autoSpinZero(AUTO_ID pAutoId)
/* pAutoId - auto Object identifier */
{
   if ( (pAutoId == NULL) || (pAutoId->autoBaseAddr == 0xFFFFFFFF))
      return(-1);

    return( ( (*AUTO_STATR(pAutoId->autoBaseAddr) & SPIN_SPD_ZERO) ? 1 : 0 ) );
}

/**************************************************************
*
*  autoSampleDetect - Return detected state of Sample 
*
*
* RETURNS:
*  1 = Sample in Probe, 0 = Sample NOT in Probe,  -1 = Error
*/
int autoSampleDetect(AUTO_ID pAutoId)
/* pAutoId - auto Object identifier */
{
   int stat;
   MBOX_CMD_SIZE spincmd;
   short spinstat;
   char  spincntrl;

   if ( (pAutoId == NULL) || (pAutoId->autoBaseAddr == 0xFFFFFFFF))
      return(-1);

   /* send message to automation to insert sample */
   spincmd = SAMPLE_DETECT;

   /* send message to automation to eject sample */
   stat = autoSpinMsgSend(pAutoId,(char*) &spincmd, sizeof(spincmd),LCMD_TIMEOUT);
   DPRINT1(1,"autoSampleDetect: sample detected: '%s'\n", ((stat == 1) ? "YES" : "NO"));
   /* DPRINT1(1,"autoSampleDetect: VME samp detected: %d\n",
	( (*AUTO_STATR(pAutoId->autoBaseAddr) & SMPLE_AT_BOT) ? 1 : 0)); */
   /* return( ( (*AUTO_STATR(pAutoId->autoBaseAddr) & SMPLE_AT_BOT) ? 1 : 0 ) ); */
   return(stat);
}

/**************************************************************
*
*  autoLockSense - Return state of Lock Sense
*
*
* RETURNS:
*  1 = Sample is Locked, 0 = Sample is Not Locked, -1 = Error
*/
int autoLockSense(AUTO_ID pAutoId)
/* pAutoId - auto Object identifier */
{
   if (pTheFifoObject == NULL)
      return(-1);

    return( ( (*FF_STATR(pTheFifoObject->fifoBaseAddr) & LOCK_SENSE) ? 1 : 0 ) );
}


int autoGenrlMsgSend(AUTO_ID pAutoId, char *msgbuffer, int size)
/* pAutoId - auto Object identifier */
/* msgbuffer - pointer to message buffer */
/* size - message size */
{
   int state;

   if ( (pAutoId == NULL) || (pAutoId->autoBaseAddr == 0xFFFFFFFF))
      return(-1);

   if (size > AUTO_MBOX_MAXSIZE)
   {
     errLogSysRet(LOGIT,debugInfo,
	"autoGenrlMsgSend: Message Size %d, larger than maximum %d\n",size, AUTO_MBOX_MAXSIZE );
     return(-1);
   }

   if (semTake(pMutexGenMBox, (sysClkRate * TIMEOUT_SECS )) != OK)
   {
      state = MUTEX_TIME_OUT;
      DPRINT(0,"autoGenrlMsgSend: Mutex Timeout");
   }
   else
   {
      memcpy((char*)(AUTO_GEN_MBOX(pAutoId->autoSharMemAddr)),
			msgbuffer,size);
     *AUTO_GEN_MB(pAutoId->autoBaseAddr) = (short) size;   /* trig intrp */
      /* Wait till cmd completes or times out */
      if (semTake(pSemGenMBox, (sysClkRate * TIMEOUT_SECS )) != OK)   
      {
        state = CMD_TIME_OUT;
        DPRINT(0,"autoGenrlMsgSend: Command Timeout");
      }
      else
      {
        state = *(VOL_INT_PTR AUTO_GEN_MBOX_ACK(pAutoId->autoSharMemAddr));
      }

      semGive(pMutexGenMBox);
   }
   return(state);
}

int autoLkValueGet(AUTO_ID pAutoId)
{
volatile short	*thirdADC;
volatile char	*ADCcntl;
int	ivar;
   ADCcntl  = (char  *)0xffff0601;
   thirdADC = (short *)0xffff0602;
   *thirdADC = 0;					/* CTC for 3rd ADC */
   ivar = 0;
   while( (*ADCcntl & 2) && (ivar < 1000000)) ivar++;	/* wait for EOC down */
   ivar = 0;
   while( !(*ADCcntl & 2) && (ivar < 1000000)) ivar++;	/* wait for EOC up */
   ivar = -(*thirdADC);
   return ( ivar );
}

/*int autoLkValueGet(AUTO_ID pAutoId)
/* pAutoId - auto Object identifier */
/*{
/*   int status;
/*   LOCK_VALUE_SIZE value;
/*   static int autoEnquiry(AUTO_ID pAutoId, int cmd, char* retval,int size,double timeout);
/*
/*   if ( (pAutoId == NULL) || (pAutoId->autoBaseAddr == 0xFFFFFFFF))
/*      return(-1);
/*
/*   status = autoEnquiry(pAutoId, GET_LOCK_VALUE,(char*) &value, 
				 sizeof(LOCK_VALUE_SIZE),QCMD_TIMEOUT);
/*
/*   if (status != OK)
/*   {
/*      printAutoEr("autoLkValueGet", GET_LOCK_VALUE, 0, status);
/*      value = -1;
/*   }
/*
/*   return((int)value);
/*}*/

int autoSpinValueGet(AUTO_ID pAutoId)
/* pAutoId - auto Object identifier */
{
   int status;
   SPIN_VALUE_SIZE value;

   if ( (pAutoId == NULL) || (pAutoId->autoBaseAddr == 0xFFFFFFFF))
      return(-1);

   status = autoEnquiry(pAutoId, GET_SPIN_VALUE,(char*) &value, 
				 sizeof(SPIN_VALUE_SIZE),QCMD_TIMEOUT);

   if (status != OK)
   {
      printAutoEr("autoSpinValueGet", GET_SPIN_VALUE, 0, status);
      value = -1;
   }

   return((int)value);
}

int autoShimsPresentGet( AUTO_ID pAutoId )
{
   int status;
   SHIMS_PRESENT_SIZE value;

   if ( (pAutoId == NULL) || (pAutoId->autoBaseAddr == 0xFFFFFFFF))
      return(-1);

   status = autoEnquiry(pAutoId, GET_SHIMS_PRESENT,(char*) &value, 
				 sizeof(SHIMS_PRESENT_SIZE),MCMD_TIMEOUT);
   DPRINT1(DEBUG_AUTO_SHIMS,"autoEnquiry status is %d in autoShimsPresentGet\n", status );

   if (status != OK)
   {
      printAutoEr("autoShimsPresentGet", GET_SHIMS_PRESENT, 0, status);
      value = -1;
   }

   DPRINT1(DEBUG_AUTO_SHIMS,"autoEnquiry value is %d in autoShimsPresentGet\n", value );

   return((int)value);
}

autoSerialShimsPresent(AUTO_ID pAutoId )
{
   int status;
   SHIMS_PRESENT_SIZE value;

   if ( (pAutoId == NULL) || (pAutoId->autoBaseAddr == 0xFFFFFFFF))
      return(-1);

   status = autoEnquiry(pAutoId, GET_SERIAL_SHIMS_PRESENT,(char*) &value, 
				 sizeof(SHIMS_PRESENT_SIZE),MCMD_TIMEOUT);
   DPRINT1(0,"autoSerialShimsPresent: status is %d \n", status );

   if (status != OK)
   {
      printAutoEr("autoShimsPresentGet", GET_SERIAL_SHIMS_PRESENT, 0, status);
      value = -1;
   }

   DPRINT1(0,"autoSerialShimsPresent value is %d\n", value );

   return((int)value);
}

static int autoEnquiry(AUTO_ID pAutoId, int cmd, char* retval,int size, double timeout)
{
   int status;
   short cmdtype;
   unsigned long offset;
   int itimeout;

#ifdef INSTRUMENT
   wvEvent(EVENT_AUTO_GENMSG_SEND,NULL,NULL);
#endif
   if (semTake(pMutexGenMBox, (sysClkRate * TIMEOUT_SECS )) != OK)
   {
      status = MUTEX_TIME_OUT;
#ifdef INSTRUMENT
      wvEvent(EVENT_AUTO_GENMUX_TIMEOUT,NULL,NULL);
#endif
   }
   else
   {
      cmdtype = (short) cmd;
      memcpy((char*)(AUTO_GEN_MBOX(pAutoId->autoSharMemAddr)),
			&cmdtype,sizeof(cmdtype));

     *AUTO_GEN_MB(pAutoId->autoBaseAddr) = (short) sizeof(cmd); /* trig intrp */

      /* Wait till cmd completes or times out */
      itimeout = sysClkRate * timeout;
      if (semTake(pSemGenMBox, itimeout ) != OK)   
      {
        status = CMD_TIME_OUT;
#ifdef INSTRUMENT
        wvEvent(EVENT_AUTO_GENCMD_TIMEOUT,NULL,NULL);
#endif
      }
      else
      {
        status = *(VOL_INT_PTR AUTO_GEN_MBOX_ACK(pAutoId->autoSharMemAddr));
      }

      if (status == OK)
      {
         offset = pAutoId->autoSharMemAddr + sizeof(int);
         memcpy((char*) retval, (AUTO_GEN_MBOX(offset)), size);
      }

      semGive(pMutexGenMBox);
   }
   return(status);
}

int autoSpinMsgSend(AUTO_ID pAutoId, char *msgbuffer, int size, double timeout)
/* pAutoId - auto Object identifier */
/* msgbuffer - pointer to message buffer */
/* size - message size */
{
   int state;
   int itimeout;

   if ( (pAutoId == NULL) || (pAutoId->autoBaseAddr == 0xFFFFFFFF))
      return(-1);

   if (size > AUTO_MBOX_MAXSIZE)
   {
     errLogSysRet(LOGIT,debugInfo,
	"autoSpinMsgSend: Message Size %d, larger than maximum %d\n",size, AUTO_MBOX_MAXSIZE );
     return(-1);
   }

#ifdef INSTRUMENT
   wvEvent(EVENT_AUTO_SPINMSG_SEND,NULL,NULL);
#endif

    DPRINT1(1,"autoSpinMsgSend: '%s'\n",
	   getmboxcmd(*((MBOX_CMD_SIZE *)msgbuffer)));

   if (semTake(pMutexSpinMBox, (sysClkRate * TIMEOUT_SECS)) != OK)
   {
#ifdef INSTRUMENT
      wvEvent(EVENT_AUTO_SPINMUX_TIMEOUT,NULL,NULL);
#endif
      state = MUTEX_TIME_OUT;
      DPRINT(-1,"autoSpinMsgSend: Mutex Timeout");
   }
   else
   {
      memcpy((char*)(AUTO_SPIN_MBOX(pAutoId->autoSharMemAddr)),
                     msgbuffer,size);
      *AUTO_SPIN_MB(pAutoId->autoBaseAddr) = (short) size;   /* trig intrp */

       /* calc int timeout value for frac of sec passed */
       itimeout = (int) (sysClkRate * timeout);

      /* Wait till cmd completes or times out */
      if (semTake(pSemSpinMBox, itimeout ) != OK)   
      {
        state = CMD_TIME_OUT;
        DPRINT1(-1,"autoSpinMsgSend: '%s',  Command Timeout",
	   getmboxcmd(*((MBOX_CMD_SIZE *)msgbuffer)));
#ifdef INSTRUMENT
        wvEvent(EVENT_AUTO_SPINCMD_TIMEOUT,NULL,NULL);
#endif
      }
      else
      {
        state = *(VOL_INT_PTR AUTO_SPIN_MBOX_ACK(pAutoId->autoSharMemAddr));
      }

      semGive(pMutexSpinMBox);
   }
   return(state);
}

int autoVTMsgSend(AUTO_ID pAutoId, char *msgbuffer, int size)
/* pAutoId - auto Object identifier */
/* msgbuffer - pointer to message buffer */
/* size - message size */
{
   int state;

   if ( (pAutoId == NULL) || (pAutoId->autoBaseAddr == 0xFFFFFFFF))
      return(-1);

   if (size > AUTO_MBOX_MAXSIZE)
   {
     errLogSysRet(LOGIT,debugInfo,
	"autoVTMsgSend: Message Size %d, larger than maximum %d\n",size, 
		AUTO_MBOX_MAXSIZE );
     return(-1);
   }

#ifdef INSTRUMENT
   wvEvent(EVENT_AUTO_VTMSG_SEND,NULL,NULL);
#endif
   if (semTake(pMutexVTMBox, (sysClkRate * TIMEOUT_SECS)) != OK)
   {
#ifdef INSTRUMENT
      wvEvent(EVENT_AUTO_VTMUX_TIMEOUT,NULL,NULL);
#endif
      state = MUTEX_TIME_OUT;
      DPRINT(-1,"autoVTMsgSend: Mutex Timeout");
   }
   else
   {
      memcpy((char*)AUTO_VT_MBOX(pAutoId->autoSharMemAddr),
                        msgbuffer,size);
      *AUTO_VT_MB(pAutoId->autoBaseAddr) = (short) size;   /* trig intrp */

      /* Wait till cmd completes or times out */
      if (semTake(pSemVTMBox, (sysClkRate * TIMEOUT_SECS )) != OK)   
      {
        state = CMD_TIME_OUT;
        DPRINT(-1,"autoVTMsgSend: Command Timeout");
#ifdef INSTRUMENT
        wvEvent(EVENT_AUTO_VTCMD_TIMEOUT,NULL,NULL);
#endif
      }
      else
      {
        state = *(VOL_INT_PTR AUTO_VT_MBOX_ACK(pAutoId->autoSharMemAddr));
      }

      semGive(pMutexVTMBox);
   }
   return(state);
}

int autoShimMsgSend(AUTO_ID pAutoId, char *msgbuffer, int size)
/* pAutoId - auto Object identifier */
/* msgbuffer - pointer to message buffer */
/* size - message size */
{
   int state;

   if ( (pAutoId == NULL) || (pAutoId->autoBaseAddr == 0xFFFFFFFF))
      return(-1);

   if (size > AUTO_MBOX_MAXSIZE)
   {
     errLogSysRet(LOGIT,debugInfo,
	"autoShimMsgSend: Message Size %d, larger than maximum %d\n",size, 
		AUTO_MBOX_MAXSIZE );
     return(-1);
   }

#ifdef INSTRUMENT
   wvEvent(EVENT_AUTO_SHIMMSG_SEND,NULL,NULL);
#endif
   if (semTake(pMutexShimMBox, (sysClkRate * TIMEOUT_SECS)) != OK)
   {
      state = MUTEX_TIME_OUT;
      DPRINT(-1,"autoShimMsgSend: Mutex Timeout");
#ifdef INSTRUMENT
      wvEvent(EVENT_AUTO_SHIMMUX_TIMEOUT,NULL,NULL);
#endif
   }
   else
   {
      memcpy((char*)(AUTO_SHIM_MBOX(pAutoId->autoSharMemAddr)),
                  msgbuffer,size);
      *AUTO_SHIM_MB(pAutoId->autoBaseAddr) = (short) size;   /* trig intrp */

      /* Wait till cmd completes or times out */
      if (semTake(pSemShimMBox, (sysClkRate * TIMEOUT_SECS )) != OK)   
      {
        state = CMD_TIME_OUT;
        DPRINT(-1,"autoShimMsgSend: Command Timeout");
#ifdef INSTRUMENT
        wvEvent(EVENT_AUTO_SHIMCMD_TIMEOUT,NULL,NULL);
#endif
      }
      else
      {
        state = *(VOL_INT_PTR AUTO_SHIM_MBOX_ACK(pAutoId->autoSharMemAddr));
      }

      semGive(pMutexShimMBox);
   }
   return(state);
}

/* to be determined later */
int autoSpinApMsgSend(AUTO_ID pAutoId, char *msgbuffer, int size)
{
   printf("autoSpinApMsgSend: Not Implemented.\n");
}

int autoVTApMsgSend(AUTO_ID pAutoId, char *msgbuffer, int size)
{
   printf("autoVTApMsgSend: Not Implemented.\n");
}
int autoShimApMsgSend(AUTO_ID pAutoId, char *msgbuffer, int size)
{
   printf("autoShimApMsgSend: Not Implemented.\n");
}

/***********************************************************************
* autoAirSet - Turn Various Spinner Air ON 
*
* Valid cmds:
* TURN_EJECT_BIT, TURN_BEAR_BIT, TURN_SDROP_BIT
*
* RETURNS:
*  Status bits  or -1 if Object identifier NULL 
*/
autoAirSet(AUTO_ID pAutoId, int cmd)
/* pAutoId - auto Object identifier */
/* cmd - What Air to turn on of off */
{
   if ( (pAutoId == NULL) || (pAutoId->autoBaseAddr == 0xFFFFFFFF))
      return(-1);

    if ( (cmd == TURN_EJECT_BIT) || (cmd == TURN_BEAR_BIT) ||
	 (cmd == TURN_SDROP_BIT)
       )
    {
	*AUTO_SPINCNTRL(pAutoId->autoBaseAddr) = 
		*AUTO_SPINCNTRL(pAutoId->autoBaseAddr) |  cmd;
    }
    return(OK);
}

/***********************************************************************
* autoAirUnset - Turn Off Various SPinner Air 
*
* Valid cmds:
* TURN_EJECT_BIT, TURN_BEAR_BIT, TURN_SDROP_BIT
*
* RETURNS:
*  Status bits  or -1 if Object identifier NULL 
*/
autoAirUnset(AUTO_ID pAutoId, int cmd)
/* pAutoId - auto Object identifier */
/* cmd - What Air to turn on of off */
{
   if ( (pAutoId == NULL) || (pAutoId->autoBaseAddr == 0xFFFFFFFF))
      return(-1);

    if ( (cmd == TURN_EJECT_BIT) || (cmd == TURN_BEAR_BIT) ||
	 (cmd == TURN_SDROP_BIT)
       )
    {
	*AUTO_SPINCNTRL(pAutoId->autoBaseAddr) = 
		*AUTO_SPINCNTRL(pAutoId->autoBaseAddr) &  
		~(cmd & (TURN_EJECT_BIT | TURN_BEAR_BIT | TURN_SDROP_BIT));
    }
    return(OK);
}

/***********************************************************************
* autoIntrpTest - Test Interrupts 
*
*  Valid interrupts: 
* POST_GMAIL_I    Post a General Mail Box Interrupt
* POST_SPNMAIL_I  Post a Spinner Mail Box Interrupt
* POST_VTMAIL_I	  Post a VT Mail Box Interrupt 
* POST_SHMMAIL_I  Post a Shim Mail Box Interrupt
* POST_332MPU_I   Post a 332 MCU VME Interrupt 
*
* RETURNS:
*  OK or -1 if Object identifier NULL 
*/
int autoIntrpTest(AUTO_ID pAutoId, int interrupt)
/* pAutoId - auto Object identifier */
/* interrupt  - Which interrupt to test */
{
   if ( (pAutoId == NULL) || (pAutoId->autoBaseAddr == 0xFFFFFFFF))
      return(-1);
    
    *AUTO_INTRP_TST(pAutoId->autoBaseAddr)  = interrupt;
    return(OK);
}

/***********************************************************************
* autoHSnRStat - return HS & R Status Bits 
*
* RETURNS:
*  Status bits  or -1 if Object identifier NULL 
*/
autoHSnRStat(AUTO_ID pAutoId)
/* pAutoId - auto Object identifier */
{
   if ( (pAutoId == NULL) || (pAutoId->autoBaseAddr == 0xFFFFFFFF))
      return(-1);
    
    return ((int) *AUTO_HSR_SR(pAutoId->autoBaseAddr));
}


/***********************************************************************
* autoLSDVget - compute value for LSDV word based on other automation
*               card functions and current value of LSDV word
*
* RETURNS:
*  New value or current value if Object identifier is NULL
*/
/* 
/* #define OFF		0
/* #define REGULATED	1
/* #define NOTREG		2
/* #define NOTPRESENT	3
/* 
/*  Distinguish lock mode from lock status  */
/* 
/* #define  LSDV_LKMODE		0x02
/* #define  LSDV_MASK4LOCKSTAT	0x0c
/* #define  LSDV_SHIFT4LOCKSTAT	2
/* #define  LSDV_MASK4SPIN		0x30
/* #define  LSDV_SHIFT4SPIN	4
/* #define  LSDV_MASK4EJECT	0x40
/* #define  LSDV_SHIFT4EJECT	6
/* 
/* short autoLSDVget(AUTO_ID pAutoId)
/* /* pAutoId - auto Object identifier */
/* {
/* short	currentLSDV;
/* int	sysIsLocked, lockStat, ejectStat, spinStat;
/* 
/*    currentLSDV = getLSDVword();
/*	if ( (pAutoId == NULL) || (pAutoId->autoBaseAddr == 0xFFFFFFFF))
/*	  return( currentLSDV );
/* */
/* /*  The LSDV_LKMODE bit is set or cleared when lock mode is turned on or off.
/* /*    See lkapio.c.  Although a race condition exists between running this
/* /*    function and setting/clearing the lock mode, the consequence would only
/* /*    be inconsistancy between the lock mode and the lock status fields.		*/
/* 
/*    if (currentLSDV & LSDV_LKMODE)
/*    {
/*       sysIsLocked = *FF_STATR(pTheFifoObject->fifoBaseAddr) & LOCK_SENSE;
/*       if (sysIsLocked)
/*          lockStat = REGULATED;
/*       else
/*          lockStat = NOTREG;
/*    }
/*    else
/*    {  lockStat = OFF;
/*    }
/* 
/*    lockStat <<= LSDV_SHIFT4LOCKSTAT;
/*    currentLSDV &= ~(LSDV_MASK4LOCKSTAT);
/*    currentLSDV |= lockStat;
/* 
/* /*  Spinner is either off, regulated or not regulated.  */
/* 
/*    if (*FF_STATR(pTheFifoObject->fifoBaseAddr) & SPIN_SENSE)
/*       spinStat = NOTREG;
/*    else
/*       spinStat = OFF;
/* 
/*	if (autoSpinZero( pAutoId ))
/*	  spinStat = OFF;
/*	else if (autoSpinReg( pAutoId ))
/*	  spinStat = REGULATED;
/*	else
/*	  spinStat = NOTREG;
/* 
/* 	spinStat <<= LSDV_SHIFT4SPIN;
/* 	currentLSDV &= ~(LSDV_MASK4SPIN);
/* 	currentLSDV |= spinStat;
/* 
/* /*  Set the eject status bit if no sample is in the probe.  */
/* 
/*	ejectStat=((*AUTO_STATR(pAutoId->autoBaseAddr)&SMPLE_AT_BOT)?1:0); 
/*	/* ejectStat = (autoSampleDetect( pAutoId ) == 0); */
/*	ejectStat <<= LSDV_SHIFT4EJECT;
/*	currentLSDV &= ~(LSDV_MASK4EJECT);
/*	currentLSDV |= ejectStat;
/* 
/* 	return( currentLSDV );
/* }
/*  */

/**************************************************************
*
*  autoGenSpinMBoxCodes - Generate Fifo words to send value to Spinner MBox.
*   
*  Generates the appropriate Fifo Words to send a 16-bit value to the
* Spinner MBox on the Automation Board via APbus. High Speed Line setting are not included
* call application will need to OR them in.
*
* RETURNS:
*     Number of Fifo Words place in array
*/
int autoGenSpinMBoxCodes(AUTO_ID pAutoId, unsigned int value, unsigned long *pCodes)
/* pAutoId - Automation Object identifier */
/* value - value to be sent */
/* pCodes - Array to put genetated codes. (fifo words) */
{
   if ( (pAutoId == NULL) )
     return(-1);

  return(
    autoGenApRegCmds(pAutoId,value,AUTO_AP_DEST_SPIN,pCodes)
        );
}
/**************************************************************
*
*  autoGenVTMBoxCodes - Generate Fifo words to send value to VT MBox.
*   
*  Generates the appropriate Fifo Words to send a 16-bit value to the
* VT MBox on the Automation Board via APbus. High Speed Line setting are not included
* call application will need to OR them in.
*
* RETURNS:
*     Number of Fifo Words place in array
*/
int autoGenVTMBoxCodes(AUTO_ID pAutoId, unsigned int value, unsigned long *pCodes)
/* pAutoId - Automation Object identifier */
/* value - value to be sent */
/* pCodes - Array to put genetated codes. (fifo words) */
{
   if ( (pAutoId == NULL) )
     return(-1);

  return(
    autoGenApRegCmds(pAutoId,value,AUTO_AP_DEST_VT,pCodes)
        );
}
/**************************************************************
*
*  autoGenShimMBoxCodes - Generate Fifo words to send value to Shim MBox.
*   
*  Generates the appropriate Fifo Words to send a 16-bit value to the
* Shim MBox on the Automation Board via APbus. High Speed Line setting are not included
* call application will need to OR them in.
*
* RETURNS:
*     Number of Fifo Words place in array
*/
int autoGenShimMBoxCodes(AUTO_ID pAutoId, unsigned int value, unsigned long *pCodes)
/* pAutoId - Automation Object identifier */
/* value - value to be sent */
/* pCodes - Array to put genetated codes. (fifo words) */
{
   if (pAutoId == NULL)
     return(-1);

  return(
    autoGenApRegCmds(pAutoId,value,AUTO_AP_DEST_SHIM,pCodes)
        );
}

/**************************************************************
*
*  autoGenShimApCodes - Generate Fifo words to send shim value to MSR
*			via the AP Bus
*   
*  Generates the appropriate Fifo Words to send a 6-bit dac# (3f or 63 max) and
*  signed 16 bit dac value to the shims on the Automation Board via APbus. 
*
* RETURNS:
*     Number of Fifo Words place in array
*/
int autoGenShimApCodes(AUTO_ID pAutoId, int dac, int value, unsigned long *pCodes)
/* pAutoId - Automation Object identifier */
/* dac     - dac to be changed */
/* value   - value to set dac to */
/* pCodes - Array to put generated codes. (fifo words) */
{
   int cnt,ap1,ap2;
   unsigned long delay;

   if (pAutoId == NULL)
     return(-1);

   /* if dac or values out of range return -1 */
   if ( (dac > 63 /*0x3f*/) ||  (value > 32767 /*0x7fff*/ ) || (value < -32768))
      return(-1);

   /*	       encode dac value               encode hi nibble value          mark as 1st word in sequence */
   ap1  = ((((short) dac) << 8) & 0x3f00) | ((((short)value) >> 8) & 0xff) | 0x8000;

   /*    encode low nibble value ,      mark as 2nd word in sequence */
   ap2 = (((short)value) & 0xff)  | 0xC000;

   DPRINT2(1,"autoGenShimApCodes:  1: 0x%x, 2: 0x%x\n",ap1,ap2);
   cnt = autoGenApRegCmds(pAutoId,ap1,AUTO_AP_DEST_SHIM,pCodes);

   delay = AP_ITRP_MIN_DELAY_CNT;
   /* Min Delay between Apbus Interrupts */
   pCodes[cnt++] = 0L | ((delay & 0x0fffffff) >> 5);  /* hw */
   pCodes[cnt++] = (0xff & delay) << 27;   /* lw */

   cnt += autoGenApRegCmds(pAutoId,ap2,AUTO_AP_DEST_SHIM,&pCodes[cnt]);

   /* Min Delay between Apbus interrupts */
   pCodes[cnt++] = 0L | ((delay & 0x0fffffff) >> 5);  /* hw */
   pCodes[cnt++] = (0xff & delay) << 27;   /* lw */

   return(cnt);
}
/**************************************************************
*
*  autoGenApRegCmds - Generate Fifo words to send a value to a Mail Box.
*   
*  Generates the appropriate Fifo Words to send a 16-bit value to the given 
*  Mail Box.  High Speed Line setting are not included
*call application will need to OR them in.
*
* RETURNS:
*     Number of Fifo Words place in array
*/
int autoGenApRegCmds(AUTO_ID pAutoId, unsigned int data, int mbox, unsigned long *pCodes)
/* pAutoId - Automation Object identifier */
/* data - value to send to mailbox */
/* mbox - mail box to send it to */
/* pCodes - Array to put generated codes. (fifo words) */
{
   unsigned long lowAddr,highAddr,apAdrData;
   unsigned long delay;

   /* generate the 16-bit date value */
   lowAddr = pAutoId->ApBusAddr + AUTO_AP_DATA;
   apAdrData = (lowAddr << 16) | (data & 0xffff);
   *pCodes = APWRT | ((apAdrData & 0x3fffffe0) >> 5);
   *(pCodes+1) = (0x01f & apAdrData) << 27 | (0L & 0x3ffffff); 
   DPRINT3(1,"autoGenSrcAdrCodes: low apwd: 0x%lx, Fifo Wrd1: 0x%lx  Wrd2: 0x%lx\n",
		apAdrData,*pCodes,*(pCodes+1));

   delay = AP_MIN_DELAY_CNT;
   /* Min Delay between Apbus Transactions */
   *(pCodes+2)= 0L | ((delay & 0x0fffffff) >> 5);  /* hw */
   *(pCodes+3) = (0xff & delay) << 27;   /* lw */

   /* generate the mbox destination */
   highAddr = pAutoId->ApBusAddr + AUTO_AP_DEST;
   apAdrData = (highAddr << 16) | (mbox & 0xffff);
   *(pCodes+4) = APWRT | ((apAdrData & 0x3fffffe0) >> 5);
   *(pCodes+5) = (0x01f & apAdrData) << 27 | (0L & 0x3ffffff); 
   DPRINT3(1,"autoGenSrcAdrCodes: high apwd: 0x%lx, Fifo Wrd1: 0x%lx  Wrd2: 0x%lx\n",
		apAdrData,*(pCodes+2),*(pCodes+3));

   /* Min Delay between Apbus Transactions */
   *(pCodes+6)= 0L | ((delay & 0x0fffffff) >> 5);  /* hw */
   *(pCodes+7) = (0xff & delay) << 27;   /* lw */

   return(8);
}

void printAutoEr(char* str, int cmd, int arg, int error)
{
   char errbuf[256];

   sprintf(errbuf,"%s: Cmd: '%s', Arg: %d",str,getmboxcmd(cmd),arg);
   switch(error)
   {
     case MUTEX_TIME_OUT:
            errLogRet(LOGIT,debugInfo,"%s Mutex Timeout",errbuf);
	    break;

      case CMD_TIME_OUT:
            errLogRet(LOGIT,debugInfo,"%s Command Timeout",errbuf);
		break;

      default:
            errLogRet(LOGIT,debugInfo,"%s Error %d",errbuf,error);
		break;
   }
  return;
}

/********************************************************************
* autoShow - display the status information on the automation Object
*
*  This routine display the status information of the automation Object
*
*
*  RETURN
*   VOID
*
*/
void autoShow(AUTO_ID pAutoId, int level)
/* AUTO_ID pAutoId  - auto Object ID */
/* int level 	   - level of information */
{
   int i;
   char *pstr;
   unsigned short status;

   if (pAutoId == NULL)
   {
     printf("autoShow: Automation Object pointer is NULL.\n");
     return;
   }
   printf("\n\n-------------------------------------------------------------\n\n");
   if (pAutoId->autoBaseAddr == 0xFFFFFFFF)
     printf(">>>>>>>>>>  Automation Board NOT Present, Automation Object for Testing. <<<<<<<<<<<\n\n");
   printf("Automation Object: Board Addr: 0x%lx, '%s'\n",pAutoId->autoBaseAddr, pAutoId->pIdStr);

   printf("Board Ver: %d,  VME: vector 0x%x, level %d, Ap Addr: 0x%x\n",
		pAutoId->autoBrdVersion, pAutoId->vmeItrVector, 
		pAutoId->vmeItrLevel,pAutoId->ApBusAddr);


  if (pAutoId->autoBaseAddr == 0xFFFFFFFF)
     return;

   status = *AUTO_STATR(pAutoId->autoBaseAddr);
   printf("Automation Status Reg: 0x%x\n", status);

   printf("Spinner: %s Regulated, Speed is %s Zero\n",
   ((status & SPIN_NOT_REG) ? "NOT" : ""), ((status & SPIN_SPD_ZERO) ? "" : "NOT"));

   printf("Air: Bearing %s, Eject %s, Slow Drop %s\n",
     ((status & BEAR_AIR_ON) ? "On" : "Off") ,
     ((status & EJECT_AIR_ON) ? "On" : "Off") ,
     ((status & SLWDROP_AIR_ON) ? "On" : "Off") );

   printf("Sample is at %s%s of the Upper Barrel.\n",
    ((status & SMPLE_AT_BOT) ? "Bottom" : ""), 
    ((status & SMPLE_AT_TOP) ? "Top" : "") ); 

   printf("Sample is %s Locked\n", ((status & NOT_LOCKED) ? "Not" : "") );
   printf("VT is %s Regulating\n", ((status & VT_ATTEN) ? "Not" : "") );

   status = *AUTO_HSR_SR(pAutoId->autoBaseAddr);
    printf("Transmitter Configuration Switch Setting: 0x%x\n",(0xf & status));
    printf("Transmitter Gating Status : 0x%x\n",(0xf & (status >> 4)));
  
 return;
}

#ifdef DEBUG

char *getmboxcmd(int dcode)
{
   int i;
   for (i=0;i < MBOXCMDSIZE; i++)
   {
      if ( donecode2msg[i].code == dcode )
      {
         return(donecode2msg[i].codestr);
      }
   }
   return("");
}
#endif
