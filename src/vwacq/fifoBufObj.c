/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* fifoBufObj.c  - Fifo Buffer Object Source Modules */
#ifndef LINT
#endif
/* 
 */

#define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#include <vxWorks.h>
#include <stdlib.h>
#include <semLib.h>
#include <msgQLib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "instrWvDefines.h"
#include "logMsgLib.h"
#include "commondefs.h"
#include "fifoBufObj.h"
#include "fifoObj.h"

/*
modification history
--------------------
2-7-97,gmb  created 
*/

/*

	Note at present Abort or FOO or HARD error, which taskRestart the buffer
	result in a loss of buffers this needs to addressed if the system is
        to be used.
*/
extern FIFO_OBJ *pTheFifoObject;

static char *IdStr ="Fifo Buffer Object";
static int  IdCnt = 0;

#ifdef TESTING
static FIFOBUF_ID ptstbuf;
#endif

/**************************************************************
*
*  fifoBufCreate - create the FIFO Buffer Object Data Structure & Semaphore, etc..
*
*
* RETURNS:
* FIFOBUF_ID  - if no error, NULL - if mallocing or semaphore creation failed
*
*	Author Greg Brissey 2/7/97
*/ 

FIFOBUF_ID fifoBufCreate(unsigned long numBuffers, unsigned long bufSize, char* idstr)
/* unsigned long numBuffers - Number of FIxed SIzed Buffers */
/* unsigned long bufSize    - Size (longs) of each Buffers */
/* char* idstr - user indentifier string */
{
  char tmpstr[80];
  FIFOBUF_ID pFifoBufObj;
  int tDRid, tMTid, tDEid;
  short sr;
  long memval;
  int cnt;
  unsigned long maxNumOfEntries;
  unsigned long *address,*prevaddr;

  /* ------- malloc space for STM Object --------- */
  if ( (pFifoBufObj = (FIFO_BUF_OBJ *) malloc( sizeof(FIFO_BUF_OBJ)) ) == NULL )
  {
    errLogSysRet(LOGIT,debugInfo,"stmCreate: ");
    return(NULL);
  }

  /* zero out structure so we don't free something by mistake */
  memset(pFifoBufObj,0,sizeof(FIFO_BUF_OBJ));

  /* --------------  setup given or default ID string ---------------- */
  if (idstr == NULL) 
  {
     sprintf(tmpstr,"%s %d",IdStr,IdCnt);
     pFifoBufObj->pIdStr = (char *) malloc(strlen(tmpstr)+2);
  }
  else
  {
     pFifoBufObj->pIdStr = (char *) malloc(strlen(idstr)+2);
  }

  if (pFifoBufObj->pIdStr == NULL)
  {
     fifoBufDelete(pFifoBufObj);
     return(NULL);
  }

  if (idstr == NULL) 
  {
     strcpy(pFifoBufObj->pIdStr,tmpstr);
  }
  else
  {
     strcpy(pFifoBufObj->pIdStr,idstr);
  }

  pFifoBufObj->pSID = SCCSid;   /* SCCS ID */

  /* -------------------------------------------------------------------*/

  /* create the Fifo Buffer Object Mutual Exclusion semaphore */
  pFifoBufObj->pBufMutex = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE |
					SEM_DELETE_SAFE);

  DPRINT1(0,"fifoBufCreate():  Creating msgQs of %d entries\n",numBuffers);
  pFifoBufObj->pBufsFree = msgQCreate(numBuffers,sizeof(long),MSG_Q_FIFO);
  pFifoBufObj->pBufsRdy =  msgQCreate(numBuffers,sizeof(long),MSG_Q_FIFO);
  pFifoBufObj->NumOfBufs = numBuffers;

  DPRINT4(-1,"fifoBufCreate():  Total Buffer Size = %d(long) * %d(bufSize) * %d(Nbufs) = %d\n",
	sizeof(long),bufSize+2,numBuffers,(sizeof(long) * (bufSize+2) * numBuffers));
  pFifoBufObj->pBufArray = (long *) malloc((sizeof(long) * (bufSize+2) * numBuffers));
  pFifoBufObj->pWrkBuf = pFifoBufObj->pWrkBufEntries = NULL;
  pFifoBufObj->BufSize = bufSize;

  if ( (pFifoBufObj->pBufMutex == NULL) ||
	(pFifoBufObj->pBufsFree == NULL) ||
	(pFifoBufObj->pBufsRdy == NULL) ||
	(pFifoBufObj->pBufArray == NULL)
     )
  {
        fifoBufDelete(pFifoBufObj);
        errLogSysRet(LOGIT,debugInfo,
	   "fifoBufCreate: Failed to allocate some resource:");
        return(NULL);
  }

  fifoBufInit(pFifoBufObj);

  return( pFifoBufObj );
}


/**************************************************************
*
*  fifoBufDelete - Deletes FIFO Buffer Object and  all resources
*
*
* RETURNS:
*  OK or ERROR
*
*	Author Greg Brissey 2/7/97
*/
void fifoBufDelete(FIFOBUF_ID pFifoBufId)
/* FIFOBUF_ID 	pFifoBufId - fifo Buffer Object identifier */
{
   int tid;

   if (pFifoBufId != NULL)
   {
      if (pFifoBufId->pIdStr != NULL)
	 free(pFifoBufId->pIdStr);
      if (pFifoBufId->pBufArray != NULL)
	 free(pFifoBufId->pBufArray);
      if (pFifoBufId->pBufMutex != NULL)
	 semDelete(pFifoBufId->pBufMutex);
      if (pFifoBufId->pBufsFree != NULL)
	 msgQDelete(pFifoBufId->pBufsFree);
      if (pFifoBufId->pBufsRdy != NULL)
	 msgQDelete(pFifoBufId->pBufsRdy);
      free(pFifoBufId);
   }
}

/**************************************************************
*
*  fifoBufInit - Initializes the Fifo Buffer msgQs 
*
*		WARNING: any FIFO words in the working or ready buffers
*			 is lost!
*
* RETURNS:
*  OK or ERROR
*
*	Author Greg Brissey 2/7/97
*/
int fifoBufInit(FIFOBUF_ID pFifoBufId)
/* FIFOBUF_ID 	pFifoBufId - fifo Buffer Object identifier */
{
  unsigned long *address,*prevaddr,*temp;
  unsigned long bufSize;
  int cnt;

   if (pFifoBufId != NULL)
   {
     taskLock();

     /* semTake(pFifoBufId->pBufMutex,WAIT_FOREVER); */

#ifdef INSTRUMENT
      wvEvent(EVENT_FIFOBUF_INITBUF,NULL,NULL);
#endif

     /* 1st Clean out the MsgQs */
     while ( msgQReceive(pFifoBufId->pBufsFree,(char*) &temp,sizeof(temp),NO_WAIT) != ERROR);
     while ( msgQReceive(pFifoBufId->pBufsRdy,(char*) &temp,sizeof(temp),NO_WAIT) != ERROR);

     /* calc up the addresses for the fixed buffers from the one large pool of memory */
     prevaddr = (long *) pFifoBufId->pBufArray;
     bufSize = pFifoBufId->BufSize + 2;
     for(cnt=0; cnt < pFifoBufId->NumOfBufs; cnt++)
     {
        /* new buffer address = previous buffer address + buffer size */
        if (cnt==0)
        {
            address = prevaddr;
        }
        else
        {
          address = (long*) 
             ( ((unsigned long) prevaddr) + ( bufSize * sizeof(long)) );
        }
        *address = (long) address;  /* 1st entry is addr, 2nd number of fifo word in buffer */
        *(address+1) = 0L;
        msgQSend(pFifoBufId->pBufsFree,(char*)&address,sizeof(address),WAIT_FOREVER,MSG_PRI_NORMAL);
        prevaddr = address;
        DPRINT3(9,"fifoBufInit(): Bufs Address: 0x%lx = val 0x%lx,entries: %d\n",
		address,*address,*(address+1));
     }
      /* Now get the 1st buffer and set it up */
      msgQReceive(pFifoBufId->pBufsFree,(char*) &pFifoBufId->pWrkBuf,sizeof(pFifoBufId->pWrkBuf),WAIT_FOREVER);
      pFifoBufId->pWrkBufEntries = pFifoBufId->pWrkBuf + 1L;
      pFifoBufId->IndexAddr = pFifoBufId->pWrkBuf + 2L;
      DPRINT5(8,"  Wrk Buf: 0x%lx(addr 0x%lx), Entries: %d (addr 0x%lx), IndexPtr: 0x%lx\n",
	*pFifoBufId->pWrkBuf,pFifoBufId->pWrkBuf,*pFifoBufId->pWrkBufEntries,pFifoBufId->pWrkBufEntries,
	pFifoBufId->IndexAddr);

     /*  semGive(pFifoBufId->pBufMutex); */
      taskUnlock();
   }
   return(OK);
}

/**************************************************************
*
*  fifoBufReset - Reset the Fifo Buffers back to the Initial state
*
*		WARNING: any FIFO words in the working or ready buffers
*			 is lost!
*
* RETURNS:
*  OK or ERROR
*
*	Author Greg Brissey 2/7/97
*/
fifoBufReset(FIFOBUF_ID pFifoBufId)
/* FIFOBUF_ID 	pFifoBufId - fifo Buffer Object identifier */
{
    long nrdy,nfree,total,nwrk,i;
    unsigned long *temp;

    if (pFifoBufId == NULL)
	return(ERROR);

    fifoBufInit(pFifoBufId); /* drastic action needed */
#ifdef XXXX
    nrdy = msgQNumMsgs(pFifoBufId->pBufsRdy);
    nfree = msgQNumMsgs(pFifoBufId->pBufsFree);
    nwrk = (pFifoBufId->pWrkBuf != NULL) ? 1 : 0;
     
    total = nrdy + nfree + nwrk;
    DPRINT4(0,"fifoBufReset(): %d total = %d rdy + %d free + %d wrkbuf\n",
		total,nrdy,nfree,nwrk);

    if (total == pFifoBufId->NumOfBufs) /* no need to re-init */
    {
	/* clear out the ready buffers and put them back into free list */
        for(i=0; i < nrdy; i++)
        {
            msgQReceive(pFifoBufId->pBufsRdy,(char*) &temp,sizeof(temp),NO_WAIT);
            msgQSend(pFifoBufId->pBufsFree,(char*)&temp,sizeof(temp),WAIT_FOREVER,MSG_PRI_NORMAL);
	  DPRINT2(1,"fifoBufReset: %d - Moving 0x%lx from Ready to Free List\n",i+1,temp);
        }
  
 	/* Reset the Working buffer Index and Count */
        if (pFifoBufId->pWrkBuf != NULL)
        {
	   DPRINT1(1,"fifoBufReset: reset WrkBuf 0x%lx \n",pFifoBufId->pWrkBuf);
           *pFifoBufId->pWrkBufEntries = 0L;	/* count to Zero */
           pFifoBufId->IndexAddr = pFifoBufId->pWrkBuf + 2L;  /* set index back to beginning */
	}
   }
   else
   {
       /* lost a buffer somewhere, rebuild the buffers */
       DPRINT(-1,"fifoBufReset: Aaaarg, lost a buffer Re-Init buffers\n");
       fifoBufInit(pFifoBufId); /* drastic action needed */
   }
#endif
   return(OK);
}

/**************************************************************
*
*  fifoBufPut - Put Words into a Working Fifo Buffer 
*
*	Copy and array of longs into the Working Fifo Buffer
*       When the Buffer Fills the Buffer Addres is placed on the 
*	Ready List, A new Buffer is obtain off the Free List and
*	made the Working Buffer 
*
* RETURNS:
*  OK or ERROR
*
*	Author Greg Brissey 2/7/97
*/
void fifoBufPut(FIFOBUF_ID pFifoBufId, unsigned long *words , long nWords )
/* FIFOBUF_ID 	pFifoBufId - fifo Buffer Object identifier */
/* unsigned long *words - array of longs to put into buffer */
/* long nWords - Number of longs to put into buffer */
{
   unsigned long max,Entries;
   int i;

   taskLock();

   /* semTake(pFifoBufId->pBufMutex,WAIT_FOREVER);  no need if taskLock used */
   Entries = (pFifoBufId->IndexAddr - pFifoBufId->pWrkBuf) - 2;
/*
   DPRINT3(1,"Entries %lu = Index addr 0x%lx - Bufstrt Addr 0x%lx\n",Entries,
		pFifoBufId->IndexAddr,pFifoBufId->pWrkBuf);
*/

   max = pFifoBufId->BufSize - Entries;
   while (nWords >= max)
   {
      for (i=0;i<max;i++)
      {
	*pFifoBufId->IndexAddr++ = *words++;
      }
      *(pFifoBufId->pWrkBufEntries) = pFifoBufId->BufSize;
/*
      DPRINT3(1,"fifoBufPut(): Send Buf: *0x%lx = 0x%lx, entries: %lu\n",
		pFifoBufId->pWrkBuf,*pFifoBufId->pWrkBuf,*pFifoBufId->pWrkBufEntries);
*/
#ifdef INSTRUMENT
      wvEvent(EVENT_FIFOBUF_SNDRDY,NULL,NULL);
#endif
      msgQSend(pFifoBufId->pBufsRdy,(char*)&pFifoBufId->pWrkBuf,sizeof(pFifoBufId->pWrkBuf),
		WAIT_FOREVER,MSG_PRI_NORMAL);

      pFifoBufId->pWrkBuf = NULL;   /* Mark Working Buffer as invalid */
      /* semGive(pFifoBufId->pBufMutex); no need w taskLock  /* allow stuffer to gain access if needed after the Send */

#ifdef INSTRUMENT
      wvEvent(EVENT_FIFOBUF_RCVFREE,NULL,NULL);
#endif
      /* if fifoBufForceRdy runs inbetween here, since pFifoBufId->pWrkBuf == NULL, it will not
	 do any thing, so we don't have to worry about it already getting a free buffer */
      /* was == 0 --- leave a couple free */
      if ((msgQNumMsgs(pFifoBufId->pBufsFree) < 2) && !fifoRunning(pTheFifoObject))
      {
#ifdef INSTRUMENT
        wvEvent(99,NULL,NULL);
#endif
        fifoStart4Exp(pTheFifoObject);
      }

      /* Maybe Pending Here */
      msgQReceive(pFifoBufId->pBufsFree,(char*) &pFifoBufId->pWrkBuf,sizeof(pFifoBufId->pWrkBuf),
		WAIT_FOREVER);

      pFifoBufId->pWrkBufEntries = pFifoBufId->pWrkBuf + 1L;
      pFifoBufId->IndexAddr = pFifoBufId->pWrkBuf + 2L;
      nWords = nWords - max;
      max = pFifoBufId->BufSize;	/* just got an empty buffer, reset max to maximum */
      /* semTake(pFifoBufId->pBufMutex,WAIT_FOREVER); No Need with taskLock */

/*
      DPRINT5(1,"fifoBufPut(): Wrk Buf: 0x%lx(addr 0x%lx), Entries: %d (addr 0x%lx), IndexPtr: 0x%lx\n",
	*pFifoBufId->pWrkBuf,pFifoBufId->pWrkBuf,*pFifoBufId->pWrkBufEntries,pFifoBufId->pWrkBufEntries,
	pFifoBufId->IndexAddr);
*/
   }

   for (i=0;i<nWords;i++)
   {
       *pFifoBufId->IndexAddr++ = *words++;
   }
   *pFifoBufId->pWrkBufEntries = (pFifoBufId->IndexAddr - pFifoBufId->pWrkBuf) - 2;

   /* semGive(pFifoBufId->pBufMutex); No Need w taskLock */

   taskUnlock();

   return;
}

/**************************************************************
*
*  fifoBufGet - Get the Address & number of longs from the Ready List
*
*	Returns the Ready Buffer Address & Number of longs into
*	the passed arguments
*
* RETURNS:
*   NONE 
*
*	Author Greg Brissey 2/7/97
*/
void fifoBufGet(FIFOBUF_ID pFifoBufId, unsigned long **BufferAddr, long *entries)
/* FIFOBUF_ID 	pFifoBufId - fifo Buffer Object identifier */
/* unsigned long **words - address to put buffer pointer */
/* long entries - Number of longs in buffer */
{
   taskLock();
#ifdef INSTRUMENT
    wvEvent(EVENT_FIFOBUF_RCVRDY,NULL,NULL);
#endif
    msgQReceive(pFifoBufId->pBufsRdy,(char*) BufferAddr,sizeof(BufferAddr),WAIT_FOREVER);
    memcpy(entries,(*BufferAddr+1),sizeof(entries));
   taskUnlock();
   return;
}

/**************************************************************
*
*  fifoBufTaskPended -  Is Task pended on the fifoBufRdyList
*
*	Returns 0 or 1 
*
* RETURNS:
*   NONE 
*
*	Author Greg Brissey 2/7/97
*/
int fifoBufTaskPended(FIFOBUF_ID pFifoBufId, int taskID)
/* FIFOBUF_ID 	pFifoBufId - fifo Buffer Object identifier */
/* unsigned long **words - address to put buffer pointer */
/* long entries - Number of longs in buffer */
{
   MSG_Q_INFO msgQinfo;
   int taskList[1];
   int status = 0;
   msgQinfo.taskIdListMax = 1;
   msgQinfo.taskIdList = taskList;
   msgQinfo.msgListMax = 0;
   msgQInfoGet(pFifoBufId->pBufsRdy,&msgQinfo);

   /* DPRINT2(-1,"fifoBufTaskPended: numMsgs: %d, numTasks: %d\n",msgQinfo.numMsgs,msgQinfo.numTasks); */

   /* numMsgs is > 0 then no task  can be pended on buffers */
   if (msgQinfo.numMsgs > 0)
      return(status);

   /* any task pending ? */
   if (msgQinfo.numTasks > 0)
   {
      /* if numMsgs < than max, then its not a put but a get pending */
      if ( (msgQinfo.numMsgs < msgQinfo.maxMsgs) )
      {
           if (taskID == taskList[0])
           {
	       status = 1;
   	       /* DPRINT1(-1,"fifoBufTaskPended: taskID: %d pended\n",taskList[0]); */
           }
      }
  }

  return(status);
}

/**************************************************************
*
*  fifoBufForceRdy - Force an unfilled buffer to be placed on the Read List
*
*	If the Working Buffer Address is None NULL then place it on the Ready List
*	Then obtain a new Working Buffer
*
* RETURNS:
*   NONE 
*
*	Author Greg Brissey 2/7/97
*/
void fifoBufForceRdy(FIFOBUF_ID pFifoBufId)
/* FIFOBUF_ID 	pFifoBufId - fifo Buffer Object identifier */
{

   taskLock();

   /* Take Mutex, This allows the fifoBufPut to complete any critical section */
   /* semTake(pFifoBufId->pBufMutex,WAIT_FOREVER); No Need w taskLock */

   /* while waiting for Mutex fifoBufPut may have put a buffer in the ready Q */
   if (pFifoBufId->pWrkBuf != NULL)
   {
     *pFifoBufId->pWrkBufEntries = (pFifoBufId->IndexAddr - pFifoBufId->pWrkBuf) - 2;
   /* if ( (msgQNumMsgs(pFifoBufId->pBufsRdy) == 0) && (*pFifoBufId->pWrkBufEntries != 0) ) */
     if ( (*pFifoBufId->pWrkBufEntries != 0) )
     {
/*
      DPRINT3(-1,"fifoBufForce(): Send Buf: *0x%lx=0x%lx, entries: %lu\n",
		pFifoBufId->pWrkBuf,*pFifoBufId->pWrkBuf,*pFifoBufId->pWrkBufEntries);
*/
#ifdef INSTRUMENT
        wvEvent(EVENT_FIFOBUF_SNDRDY,NULL,NULL);
#endif
       msgQSend(pFifoBufId->pBufsRdy,(char*) &pFifoBufId->pWrkBuf,sizeof(pFifoBufId->pWrkBuf),
		WAIT_FOREVER,MSG_PRI_NORMAL);
       pFifoBufId->pWrkBuf = NULL;   /* Mark Working Buffer as invalid */
#ifdef INSTRUMENT
        wvEvent(EVENT_FIFOBUF_RCVFREE,NULL,NULL);
#endif
       msgQReceive(pFifoBufId->pBufsFree,(char*) &pFifoBufId->pWrkBuf,sizeof(pFifoBufId->pWrkBuf),
		  WAIT_FOREVER);
       pFifoBufId->pWrkBufEntries = pFifoBufId->pWrkBuf + 1L;
       pFifoBufId->IndexAddr = pFifoBufId->pWrkBuf + 2L;
/*
      DPRINT5(-1,"fifoBufForce(): Wrk Buf: 0x%lx(addr 0x%lx), Entries: %d (addr 0x%lx), IndexPtr: 0x%lx\n",
	*pFifoBufId->pWrkBuf,pFifoBufId->pWrkBuf,*pFifoBufId->pWrkBufEntries,pFifoBufId->pWrkBufEntries,
	pFifoBufId->IndexAddr);
*/
     }
   }
   /* semGive(pFifoBufId->pBufMutex); No Need w taskLock */
   taskUnlock();
   return;
}

/**************************************************************
*
*  fifoBufReturn - Put the Given Buffer Addres on the Free List
*
*
* RETURNS:
*  NONE
*
*	Author Greg Brissey 2/7/97
*/
void fifoBufReturn(FIFOBUF_ID pFifoBufId,long *bufaddr)
/* FIFOBUF_ID 	pFifoBufId - fifo Buffer Object identifier */
/* long *bufaddr - buffer  address to be placed on the free list */
{
   taskLock();
#ifdef INSTRUMENT
   wvEvent(EVENT_FIFOBUF_SNDFREE,NULL,NULL);
#endif
   msgQSend(pFifoBufId->pBufsFree,(char*)&bufaddr,sizeof(bufaddr),
		NO_WAIT,MSG_PRI_NORMAL);
   taskUnlock();
}

/**************************************************************
*
*  fifoBufWkEntries - Returns number of Entries in the present Working Buffer
*
*
* RETURNS:
*  number of entries in working buffer
*
*	Author Greg Brissey 2/7/97
*/
unsigned long fifoBufWkEntries(FIFOBUF_ID pFifoBufId)
/* FIFOBUF_ID 	pFifoBufId - fifo Buffer Object identifier */
{
   ulong_t NumEntries;
   taskLock();
   NumEntries = *pFifoBufId->pWrkBufEntries;	/* assure an atomic operation */
   taskUnlock();
   return (NumEntries);
}

/********************************************************************
* fifoBufShow - display the status information on the FIFO Buffer Object
*
*  This routine display the status information of the FIFO Buffer Object
*
*
*  RETURN
*   VOID
*
*/
void fifoBufShow(FIFOBUF_ID pFifoBufId, int level)
/* FIFOBUF_ID 	pFifoBufId - fifo Buffer Object identifier */
/* int level - level of output */
{
   if (pFifoBufId == NULL)
   {
     printf("fifoShow: FIFO Object pointer is NULL.\n");
     return;
   }
   printf("FIFO Buffer Object: '%s', 0x%lx\n", pFifoBufId->pIdStr,pFifoBufId);
   printf("SCCSid: '%s'\n\n", pFifoBufId->pSID);

   printf("Buffer Addr: 0x%lx\n",pFifoBufId->pBufArray);
   printf("Buffer Size: %lu\n",pFifoBufId->BufSize);
   printf("Working Buffer: Addr - 0x%lx, Val - 0x%lx\n",
	    pFifoBufId->pWrkBuf,*pFifoBufId->pWrkBuf);
   printf("       Entries: Addr - 0x%lx, Num - %lu\n",
	    pFifoBufId->pWrkBufEntries,*pFifoBufId->pWrkBufEntries);
   printf("         Index: Addr - 0x%lx, Val - 0x%lx\n",
	    pFifoBufId->IndexAddr,*pFifoBufId->IndexAddr);

   printf("\nFIFO Buffer Free List MsgQ (0x%lx): \n",pFifoBufId->pBufsFree);
   msgQInfoPrint(pFifoBufId->pBufsFree);
   if (level > 0)
     printmsgQEntries(pFifoBufId->pBufsFree);
   printf("\nFIFO Buffer Ready List MsgQ (0x%lx): \n",pFifoBufId->pBufsRdy);
   msgQInfoPrint(pFifoBufId->pBufsRdy);
   if (level > 0)
     printmsgQEntries(pFifoBufId->pBufsRdy);
   printf("\nFIFO Buffer Mutex Semaphore (0x%lx): \n",pFifoBufId->pBufMutex);
   printSemInfo(pFifoBufId->pBufMutex,"Fifo Buf Mutex",1);
   semShow(pFifoBufId->pBufMutex,level);

}
printmsgQEntries(MSG_Q_ID pMsgQ)
{
   int i,pPendTasks[4];
   char *msgList[50];
   int   msgLenList[50];
   MSG_Q_INFO MsgQinfo;
   unsigned long *ptr;

   memset(&MsgQinfo,0,sizeof(MsgQinfo));
   MsgQinfo.taskIdListMax = 4;
   MsgQinfo.taskIdList = &pPendTasks[0];
   MsgQinfo.msgListMax = 50;
   MsgQinfo.msgPtrList = &msgList[0];
   MsgQinfo.msgLenList = &msgLenList[0];

   msgQInfoGet(pMsgQ,&MsgQinfo);

   printf("MsgQ Entries: \n");
   printf("Buffer Addr 		*Addr		Entries\n"); 
   for(i=0;i < MsgQinfo.numMsgs; i++)
   {
      memcpy(&ptr,MsgQinfo.msgPtrList[i],sizeof(long)); /* copy buffer address into pointer */
      printf("%d: 0x%lx		0x%lx	%d\n", i,ptr,*ptr,*(ptr+1));
   }
}
/*----------------------------------------------------------------------*/
/* stmShwResrc							*/
/*     Show system resources used by Object (e.g. semaphores,etc.)	*/
/*	Useful to print then related back to WindView Events		*/
/*----------------------------------------------------------------------*/
VOID fifoBufShwResrc(FIFOBUF_ID pFifoBufId, int indent)
{
   int i;
   char spaces[40];

   for (i=0;i<indent;i++) spaces[i] = ' ';
   spaces[i]='\0';

   printf("%sFifo Buffer Obj: '%s', 0x%lx\n",spaces,pFifoBufId->pIdStr,pFifoBufId);
   printf("%s    Mutex:       pBufMutex  ---- 0x%lx\n",spaces,pFifoBufId->pBufMutex);
   printf("%s    MsgQs:       pBufsFree  ---- 0x%lx\n",spaces,pFifoBufId->pBufsFree);
   printf("%s    MsgQs:       pRdyBufs  ----- 0x%lx\n",spaces,pFifoBufId->pBufsRdy);
   printf("\n");
}

#ifdef TESTING
tstbufc( int num, int size)
{
     ptstbuf = fifoBufCreate(num,size,"Test Buffer Object");
     printf("Buffer Object: 0x%lx\n",ptstbuf);
     fifoBufShow(ptstbuf,0);
}

tstbufp(int num)
{
    long words[80],i;
    for (i=0; i<num; i++)
       words[i] = i;
    fifoBufPut(ptstbuf,words,num);
}
tstbufget()
{
    unsigned long *fwords, entries,i;
    fifoBufGet(ptstbuf, &fwords, &entries);
    printf("entries: %d\n",entries);
    printf("word[0] - 0x%lx (address of this buffer)\n",fwords[0]);
    printf("word[1] --- %lu (Number in the buffer)\n\n",fwords[1]);
    for (i=2;i<entries+2;i++)
      printf("word[%d] - 0x%lx\n",i,fwords[i]);
    fifoBufReturn(ptstbuf,fwords);
}
tstbuff()
{
  fifoBufForceRdy(ptstbuf);
}

tstshw()
{
   fifoBufShow(ptstbuf,0);
}
#endif
