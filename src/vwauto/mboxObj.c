/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* mboxObj.c  - Automation Board 332 Mail Box Interface Source Modules */
#ifndef LINT
#endif
/* 
 */


#define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#include <vxWorks.h>
#include <stdlib.h>
#include <msgQLib.h>
#include <semLib.h>
#include <rngLib.h>
#include <vme.h>
#include <iv.h>
#include "logMsgLib.h"
#include "vmeIntrp.h"
#include "hardware.h"
#include "m32hdw.h"
#include "instrWvDefines.h"

/*
modification history
--------------------
2-1-95,gmb  created 
*/
/*

 MPU332 Automation Board Mail Box Interface Object Routines

   Interrupt Service Routines (ISR)

*/

/* VME Interrupt posted by the automation 332 for Exception conditions */
static SEM_ID pSemAutoExcpt = NULL;

/* VME Interrupt posted by the automation 332 for Command Complete Acknowledgement */
static SEM_ID pSemAutoAck = NULL;

/* VME Blocking Semaphore release when Ackonowledge received from 332 */
static SEM_ID pSemGenMBox = NULL;
static SEM_ID pSemShimMBox = NULL;
static SEM_ID pSemSpinMBox = NULL;
SEM_ID pSemVTMBox = NULL;
static SEM_ID pSemApShimMBox = NULL;
static SEM_ID pSemApSpinMBox = NULL;
static SEM_ID pSemApVTMBox = NULL;

/* Mutex Smeaphores so only one task may access a mail box at a time */
static SEM_ID pMutexGenMBox = NULL;
static SEM_ID pMutexShimMBox = NULL;
static SEM_ID pMutexSpinMBox = NULL;
SEM_ID pMutexVTMBox = NULL;

static RING_ID pShimAPdata = NULL;

static char WriteCntrlReg = 0;

static char *pSharedMemory = 0L;

#define AUTO_ACK_IST_PRIORITY 60
#define AUTO_EXCPT_IST_PRIORITY 60
#define AUTO_IST_STACK_SIZE 2048
#define AUTO_IST_TASK_OPTIONS 0

/*-----------------------------------------------------------
|
|  Internal Functions
|
+---------------------------------------------------------*/
/*-------------------------------------------------------------
| Interrupt Service Routines (ISR) 
+--------------------------------------------------------------*/
void genrlISR()
{
   short mbvalue;

   mbvalue = *M32_MAILBOX(MPU332_GENMAIL);
   DPRINT2(1,"genrlISR: RegVal: %d (0x%x)\n",mbvalue,mbvalue);
   /* logMsg("GenrlISR: RegVal: %d (0x%x)\n",mbvalue,mbvalue); */
   semGive(pSemGenMBox);

#ifdef INSTRUMENT
     wvEvent(EVENT_MBOX_GENISR,&mbvalue,sizeof(short));
#endif

}

void spinISR()
{
   short mbvalue;

   mbvalue = *M32_MAILBOX(MPU332_SPINMAIL);
   DPRINT2(1,"spinISR: RegVal: %d (0x%x)\n",mbvalue,mbvalue);
   /* logMsg("spinISR: RegVal: %d (0x%x)\n",mbvalue,mbvalue); */
   if (mbvalue <= AUTO_MBOX_SIZE)
   {
     semGive(pSemSpinMBox);  /* mailbox messsage */
   }
   else
   {
     semGive(pSemApSpinMBox);  /* AP Bus mailbox message */
   }
#ifdef INSTRUMENT
     wvEvent(EVENT_MBOX_SPINISR,&mbvalue,sizeof(short));
#endif
}

void VtISR()
{
   short mbvalue;

   mbvalue = *M32_MAILBOX(MPU332_VTMAIL);
   if (mbvalue <= AUTO_MBOX_SIZE)
   {
     semGive(pSemVTMBox);  /* mailbox messsage */
   }
   else
   {
     semGive(pSemApVTMBox);  /* AP Bus mailbox message */
   }
#ifdef INSTRUMENT
     wvEvent(EVENT_MBOX_VTISR,&mbvalue,sizeof(short));
#endif
}

/* static short dacvalue; */

void shimISR()
{
   short mbvalue;

   mbvalue = *M32_MAILBOX(MPU332_SHIMMAIL);
   DPRINT2(1,"shimISR: RegVal: %d (0x%x)\n",mbvalue,mbvalue);
   if ((mbvalue <= AUTO_MBOX_SIZE) && (mbvalue > 0) )
   {
     semGive(pSemShimMBox);  /* mailbox messsage */
#ifdef INSTRUMENT
     wvEvent(EVENT_MBOX_SHIMISR,&mbvalue,sizeof(short));
#endif
   }
   else
   {
     /* bit 7 set word is DAC #  plus hi nibble dac value */
     /* bit 7&6 set word is low nibble dac value */
     if ( (mbvalue & 0xC000) == 0x8000 )  
     {
        /* logMsg("shimISR: dac #: %d\n",(mbvalue & 0x7f00) >> 8); */
        rngBufPut(pShimAPdata, (char*) &mbvalue, sizeof(mbvalue));
	/* dacvalue = (mbvalue & 0x00ff) << 8; */
        /* logMsg("shimISR: dac #: %d, hnibble: %d (0x%x)\n",
		((mbvalue & 0x7f00) >> 8),dacvalue,dacvalue); */
#ifdef INSTRUMENT
        wvEvent(EVENT_MBOX_SHIMAP1ISR,&mbvalue,sizeof(short));
#endif
     }
     else	/* 2nd Word is low nibble value */
     {
       /* logMsg("shimISR: dac low nib val %d\n",(mbvalue & 0x00ff) ); */
       /* dacvalue = dacvalue  + (mbvalue & 0x00ff); */
       /* logMsg("shimISR: low nib: %d, dac val %d (0x%x)\n",
		(mbvalue & 0x00ff),dacvalue,dacvalue); */
       rngBufPut(pShimAPdata, (char*) &mbvalue, sizeof(mbvalue));
       semGive(pSemApShimMBox);  /* AP Bus mailbox message */
#ifdef INSTRUMENT
        wvEvent(EVENT_MBOX_SHIMAP2ISR,&mbvalue,sizeof(short));
#endif
     }
   }
}


/**************************************************************
*
*  mboxCreate - create the Automation Mail Box Object Data Structure & Semaphores
*
*
* RETURNS:
* OK - if no error, NULL - if mallocing or semaphore creation failed
*
*/ 
int  mboxInit(char *sharedMemAddr)
{

  pSharedMemory = sharedMemAddr;

  pSemGenMBox = semBCreate(SEM_Q_FIFO,SEM_EMPTY);
  pSemShimMBox = semBCreate(SEM_Q_FIFO,SEM_EMPTY);
  pSemSpinMBox = semBCreate(SEM_Q_FIFO,SEM_EMPTY);
  pSemVTMBox = semBCreate(SEM_Q_FIFO,SEM_EMPTY);
  pSemApShimMBox = semCCreate(SEM_Q_FIFO,SEM_EMPTY);
  pSemApSpinMBox = semBCreate(SEM_Q_FIFO,SEM_EMPTY);
  pSemApVTMBox = semBCreate(SEM_Q_FIFO,SEM_EMPTY);

  pShimAPdata = rngCreate(128);
  
  if ( 
       (pSemGenMBox == NULL) || (pSemShimMBox == NULL)  ||
       (pSemSpinMBox == NULL) || (pSemVTMBox == NULL)   ||
       (pSemApShimMBox == NULL)  ||
       (pSemApSpinMBox == NULL) || (pSemApVTMBox == NULL)
     )
  {
     mboxDelete();
     errLogSysRet(LOGIT,debugInfo,
	"mboxInit: Could not Allocate Semaphore Space:");
     return(NULL);
  }

  /* ------- Connect VME interrupt vector to proper Semaphore to Give ----- */

  /*  line + 24 */

   if ( intConnect( 
	INUM_TO_IVEC( MBOX_BASE_VEC + GEN_MBOX_ITRP_OFFSET),  
		     genrlISR, NULL) == ERROR)
   {
     errLogSysRet(LOGIT,debugInfo,
	"mboxInit: Could not connect General Mbox interrupt vector: ");
     mboxDelete();
     return;
   }

   if ( intConnect( 
	INUM_TO_IVEC( (MBOX_BASE_VEC+ SPIN_MBOX_ITRP_OFFSET) ),  
		     spinISR, NULL) == ERROR)
   {
     errLogSysRet(LOGIT,debugInfo,
	"mboxInit: Could not connect Spinner Mbox interrupt vector: ");
     mboxDelete();
     return;
   }

   if ( intConnect( 
	INUM_TO_IVEC( MBOX_BASE_VEC+ VT_MBOX_ITRP_OFFSET),  
		     VtISR, NULL) == ERROR)
   {
     errLogSysRet(LOGIT,debugInfo,
	"mboxInit: Could not connect VT Mbox interrupt vector: ");
     mboxDelete();
     return;
   }

   if ( intConnect( 
	INUM_TO_IVEC( (MBOX_BASE_VEC+ SHIM_MBOX_ITRP_OFFSET) ),  
		     shimISR, NULL) == ERROR)
   {
     errLogSysRet(LOGIT,debugInfo,
	"mboxInit: Could not connect Shim Mbox interrupt vector: ");
     mboxDelete();
     return;
   }

   /* Fill shared memory mail boxs with "ee"  */
   /*
   memset((char*) AUTO_GEN_MBOX((MPU332_RAM+AUTO_SHARED_MEM_OFFSET)),
		0xee,AUTO_MBOX_TOTAL_SIZE);
   */

   printf("Old addr: 0x%lx, new addr: 0x%lx\n",(MPU332_RAM+AUTO_SHARED_MEM_OFFSET),pSharedMemory);
   memset((char*) AUTO_GEN_MBOX(pSharedMemory), 0xee, AUTO_MBOX_TOTAL_SIZE);

   WriteCntrlReg = 0x10;

   *M32_CNTRL = WriteCntrlReg;  /* clear red light */

   return;
}


/**************************************************************
*
*  mboxDelete - Deletes Automation Object and  all resources
*
*
* RETURNS:
*  OK or ERROR
*
*	Author Greg Brissey 10/1/93
*/
int mboxDelete()
{
      if (pSemGenMBox != NULL)
	semDelete(pSemGenMBox);
      if (pSemShimMBox != NULL)
	semDelete(pSemShimMBox);
      if (pSemSpinMBox != NULL)
	semDelete(pSemSpinMBox);
      if (pSemVTMBox != NULL)
	semDelete(pSemVTMBox);
      if (pSemApShimMBox != NULL)
	semDelete(pSemApShimMBox);
      if (pSemApSpinMBox != NULL)
	semDelete(pSemApSpinMBox);
      if (pSemApVTMBox != NULL)
	semDelete(pSemApVTMBox);
}


/**************************************************************
*
*  m32StatReg - Gets Automation status register value
*
*
* RETURNS:
*  8-bit Automation Status Register Value
*/
char autoStatReg()
{
    return (*M32_CNTRL);
}

/**************************************************************
*
*  cntrlRegSet - Sets Device control register value
*
*
* RETURNS:
*  void
*/
void cntrlRegSet(unsigned char value)
{

    DPRINT1(2,"cntrlRegSet: 0x%x\n",value);
    if (value != SET_QUART_RESET)  /* don't retain quart reset */
    {
       WriteCntrlReg |= value;
       *M32_CNTRL = WriteCntrlReg;
    }
    else
    {
       *M32_CNTRL = WriteCntrlReg | SET_QUART_RESET;
    }
}

/**************************************************************
*
*  cntrlRegClear - Sets Device control register value
*
*
* RETURNS:
*  void
*/
void cntrlRegClear(unsigned char value)
{

    DPRINT1(2,"cntrlRegClear: 0x%x\n",value);
    if (value != SET_QUART_RESET)  /* don't retain quart reset */
    {
       WriteCntrlReg =  (WriteCntrlReg & ~value);
       *M32_CNTRL = WriteCntrlReg;
    }
}


#define TIMEOUT_SECS 10  /* 10 second timeout on mutex */

/******************************************************************
*  mboxGenGetMsg - Get a message from General message Mail Box
*
*  One Major assumption is that only one task will be call this 
*  routine. If more than one task try garbled messages will result.
*  Mutex exclusion will be necessary 
*
*
*/
int mboxGenGetMsg(char *msgbuffer, int size)
/* msgbuffer - pointer to message buffer */
/* size - message size */
{
   int bytes;

#ifdef INSTRUMENT
   wvEvent(EVENT_MBOX_GENMSGGET,NULL,NULL);
#endif
   semTake(pSemGenMBox,WAIT_FOREVER);  /* wait here for MBox Interrupt */
   bytes = *M32_MAILBOX(MPU332_GENMAIL);
   if (bytes > size)
   {
     DPRINT1(0,"Warning message was truncated by %d\n",bytes - size);
     bytes = size;
   }
   /*
   memcpy(msgbuffer, 
          (char*) AUTO_GEN_MBOX((MPU332_RAM + AUTO_SHARED_MEM_OFFSET)), bytes);
   */
   memcpy(msgbuffer, (char*) AUTO_GEN_MBOX(pSharedMemory), bytes);
   return(bytes);
}

/******************************************************************
*  mboxGenPutMsg - Put a message into General message Mail Box
*
*  One Major assumption is that only one task will be call this 
*  routine. If more than one task try garbled messages will result.
*  Mutex exclusion will be necessary 
*
*
*/
int mboxGenPutMsg(char *msgbuffer, int size)
/* msgbuffer - pointer to message buffer */
/* size - message size */
{
   unsigned long offset;

#ifdef INSTRUMENT
   wvEvent(EVENT_MBOX_GENMSGPUT,NULL,NULL);
#endif
   if (size > 256)
   {
     errLogRet(LOGIT,debugInfo,
	"mboxGenGetMsg: Message Size %d, larger than maximum %d\n",
		size, 256 );
     return(-1);
   }
   /* skip return status int */

   /* offset = MPU332_RAM + AUTO_SHARED_MEM_OFFSET + sizeof(int); */
   offset = ((u_long)pSharedMemory) + sizeof(int);
   memcpy((char*) AUTO_GEN_MBOX((offset)), msgbuffer, size);
   return(OK);
}

void mboxGenMsgComplete(int stat)
{
    /* *(VOL_INT_PTR AUTO_GEN_MBOX_ACK(MPU332_RAM+AUTO_SHARED_MEM_OFFSET)) = stat; */
    *(VOL_INT_PTR AUTO_GEN_MBOX_ACK(pSharedMemory)) = stat;
    *M32_VME_INTRP = AUTO_ITRP_VEC + GEN_ACK_VEC; /* trigger intrp */
#ifdef INSTRUMENT
   wvEvent(EVENT_MBOX_GENMSGACK,NULL,NULL);
#endif
}

int mboxSpinGetMsg(char *msgbuffer, int size)
/* msgbuffer - pointer to message buffer */
/* size - message size */
{
   int bytes;

#ifdef INSTRUMENT
   wvEvent(EVENT_MBOX_SPINMSGGET,NULL,NULL);
#endif
   semTake(pSemSpinMBox,WAIT_FOREVER);  /* wait here for MBox Interrupt */
   bytes = *M32_MAILBOX(MPU332_SPINMAIL);
   if (bytes > size)
   {
     DPRINT1(0,"Warning message was truncated by %d\n",bytes - size);
     bytes = size;
   }
   /*
   memcpy(msgbuffer, 
          (char*) AUTO_SPIN_MBOX((MPU332_RAM+AUTO_SHARED_MEM_OFFSET)),bytes);
   */
   memcpy(msgbuffer, (char*) AUTO_SPIN_MBOX(pSharedMemory),bytes);
   return(bytes);
}

void mboxSpinMsgComplete(int stat)
{
    /* *(VOL_INT_PTR AUTO_SPIN_MBOX_ACK((MPU332_RAM+AUTO_SHARED_MEM_OFFSET))) = stat; */
    *(VOL_INT_PTR AUTO_SPIN_MBOX_ACK(pSharedMemory)) = stat;
    *M32_VME_INTRP = AUTO_ITRP_VEC + SPIN_ACK_VEC; /* trigger intrp*/
#ifdef INSTRUMENT
   wvEvent(EVENT_MBOX_SPINMSGACK,NULL,NULL);
#endif
}

int mboxVTGetMsg(char *msgbuffer, int size, int timeout)
/* msgbuffer - pointer to message buffer */
/* size - message size */
{
   int bytes,flag;

#ifdef INSTRUMENT
   wvEvent(EVENT_MBOX_VTMSGGET,NULL,NULL);
#endif
   flag = semTake(pSemVTMBox,timeout);  /* wait here for MBox Interrupt */
   if (flag != OK)
     return(-1);  /* show a time out */
   bytes = *M32_MAILBOX(MPU332_VTMAIL);
   if (bytes > size)
   {
     DPRINT1(0,"Warning message was truncated by %d\n",bytes - size);
     bytes = size;
   }
   /*
   memcpy(msgbuffer, 
          (char*) AUTO_VT_MBOX((MPU332_RAM+AUTO_SHARED_MEM_OFFSET)),bytes);
   */
   memcpy(msgbuffer, (char*) AUTO_VT_MBOX(pSharedMemory),bytes);
   return(bytes);
}

void mboxVTMsgComplete(int stat)
{
    /* *(VOL_INT_PTR AUTO_VT_MBOX_ACK((MPU332_RAM+AUTO_SHARED_MEM_OFFSET))) = stat; */
    *(VOL_INT_PTR AUTO_VT_MBOX_ACK(pSharedMemory)) = stat;
    *M32_VME_INTRP = AUTO_ITRP_VEC + VT_ACK_VEC; /* trigger intrp*/
#ifdef INSTRUMENT
   wvEvent(EVENT_MBOX_VTMSGACK,NULL,NULL);
#endif
}


int mboxShimGetMsg(char *msgbuffer, int size)
/* msgbuffer - pointer to message buffer */
/* size - message size */
{
   int bytes;

#ifdef INSTRUMENT
   wvEvent(EVENT_MBOX_SHIMMSGGET,NULL,NULL);
#endif
   semTake(pSemShimMBox,WAIT_FOREVER);  /* wait here for MBox Interrupt */
   bytes = *M32_MAILBOX(MPU332_SHIMMAIL);
   if (bytes > size)
   {
     DPRINT1(0,"Warning message was truncated by %d\n",bytes - size);
     bytes = size;
   }
   /*
   memcpy(msgbuffer, 
          (char*) AUTO_SHIM_MBOX((MPU332_RAM+AUTO_SHARED_MEM_OFFSET)),bytes);
   */
   memcpy(msgbuffer, (char*) AUTO_SHIM_MBOX(pSharedMemory),bytes);
   return(bytes);
}

void mboxShimMsgComplete(int stat)
{
    /* *(VOL_INT_PTR AUTO_SHIM_MBOX_ACK((MPU332_RAM+AUTO_SHARED_MEM_OFFSET))) = stat; */
    *(VOL_INT_PTR AUTO_SHIM_MBOX_ACK(pSharedMemory)) = stat;
    *M32_VME_INTRP = AUTO_ITRP_VEC + SHIM_ACK_VEC; /* trigger intrp*/
#ifdef INSTRUMENT
   wvEvent(EVENT_MBOX_SHIMMSGACK,NULL,NULL);
#endif
}

int mboxShimApGetMsg(char *msgbuffer)
/* msgbuffer - pointer to message buffer */
/* size - message size */
{
   char msge[50];
   short code;
   int i,bytes;

#ifdef INSTRUMENT
   wvEvent(EVENT_MBOX_SPINAPMSGGET,NULL,NULL);
#endif
   semTake(pSemApShimMBox,WAIT_FOREVER);  /* wait here for MBox Interrupt */
   bytes = rngBufGet(pShimAPdata, msgbuffer,4); /* sizeof(long)); /* Dac # & Dac Value 2 16bit numbers */

   return(bytes);
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
void mboxShow(int level)
/* int level 	   - level of information */
{
   int i;
   char *pstr;
   unsigned short status;

#ifdef XXXX
   printf("\n\n-------------------------------------------------------------\n\n");
   printf("Mail Boxs: Mbox Addr: 0x%lx, '%s'\n",pAutoId->autoBaseAddr, pAutoId->pIdStr);


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

   printf("Sample is %sLocked\n", ((status & NOT_LOCKED) ? "Not " : "") );
   printf("VT s %sRegulating\n", ((status & VT_ATTEN) ? "Not " : "") );

   status = *AUTO_HSR_SR(pAutoId->autoBaseAddr);
    printf("Configuration Switch Setting: 0x%x\n",(0xf & status));
    printf("Tansmitter Gating Staus : 0x%x\n",(0xf & (status >> 4)));
#endif

   printf("Shared Shim values would be found starting at 0x%x\n",
	   AUTO_SHARED_SHIMS(MPU332_RAM+AUTO_SHARED_MEM_OFFSET));

   printf("Shared Shim values would be found starting at 0x%x\n",
	   AUTO_SHARED_SHIMS(pSharedMemory));
  
 return;
}
