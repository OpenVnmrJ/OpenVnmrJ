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
#include <vxWorks.h>
#include <stdlib.h>
#include <semLib.h>
#include <msgQLib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "nvhardware.h"
#include "instrWvDefines.h"
#include "logMsgLib.h"
#include "commondefs.h"
#include "fifoFuncs.h"
#include "dmaDrv.h"
#include "dmaReg.h"
#include "cntrlFifoBufObj.h"

/*
modification history
--------------------
5-28-04,gmb  created , from an adaptation of the Inova fifo buffer Object
*/

/*

*/

#define USE_DUFF_DEVICE 

extern FIFOBUF_ID pCntrlFifoBuf;   /* global reference to this object */

static int cntrFifoDmaChannel = -1;

static volatile unsigned int* pFifoWriteAddr;

static CNTRLBUF_DECODEFUNC pFifoWordDecoder = NULL;

/* queue mode of buffers, 0 = DMA buffer, 1 = send buffer to decoder */

static int queueMode = CNTRL_DMA_MODE;

int decodeFifoWordsFlag = 0;  /* if true queueTransfer will decode fifo words to be dma for duration */
int prtdecodeflag = 0;        /* this controls with the fifo word decode prints each word in decoded form */

unsigned long cumFifoWordsDma = 0L;

/* dignostics parameters */
unsigned long long fidduration;
unsigned long long cumduration;

#ifdef ENABLE_BUFFER_DURATION_CALC
* unsigned long bufduration; /* time duration of FIFO words in the working buffer */
* int bufdurflag = 0;
#endif

/**************************************************************
*
*  cntlrFifoBufCreate - create the FIFO Buffer Object Data Structure & Semaphore, etc..
*
*
* RETURNS:
* FIFOBUF_ID  - if no error, NULL - if mallocing or semaphore creation failed
*
*	Author Greg Brissey 5/28/04
*/ 
FIFOBUF_ID cntlrFifoBufCreate(unsigned long numBuffers, unsigned long bufSize, int dmaChannel, volatile unsigned int* pFifoWrite )
/* unsigned long numBuffers - Number of FIxed SIzed Buffers */
/* unsigned long bufSize    - Size (longs) of each Buffers */
/* dmaChannel - dma channel to use to send FIFO words to FIFO */
/* pFifoWrite - write address into the instruction FIFO */
{
   FIFOBUF_ID pFifoBufObj;
   int cnt;

  /* ------- malloc space for STM Object --------- */
  if ( (pFifoBufObj = (FIFO_BUF_OBJ *) malloc( sizeof(FIFO_BUF_OBJ)) ) == NULL )
  {
    errLogSysRet(LOGIT,debugInfo,"stmCreate: ");
    return(NULL);
  }

  /* zero out structure so we don't free something by mistake */
  memset(pFifoBufObj,0,sizeof(FIFO_BUF_OBJ));

  /* -------------------------------------------------------------------*/

  /* create the Fifo Buffer Object Mutual Exclusion semaphore */
  pFifoBufObj->pBufMutex = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE |
					SEM_DELETE_SAFE);

  DPRINT1(-10,"fifoBufCreate():  Creating msgQs of %d entries\n",numBuffers);
  pFifoBufObj->pBufsFree = msgQCreate(numBuffers,sizeof(long),MSG_Q_FIFO);
  pFifoBufObj->NumOfBufs = numBuffers;

  DPRINT4(+4,"fifoBufCreate():  Total Buffer Size = %d(long) * %d(bufSize) * %d(Nbufs) = %d\n",
	sizeof(long),bufSize,numBuffers,(sizeof(long) * (bufSize) * numBuffers));

  pFifoBufObj->pBufArray = (unsigned long **) malloc((sizeof(long) * numBuffers));
  memset(pFifoBufObj->pBufArray,0,(sizeof(long) * numBuffers));
  for ( cnt=0; cnt < numBuffers; cnt++ )
  {
     /* each buffer cached aling since these get DMA'd to the FIFO */
     pFifoBufObj->pBufArray[cnt] = (unsigned long *) memalign(_CACHE_ALIGN_SIZE,(sizeof(long) * bufSize));
     DPRINT3(+2,"fifoBufCreate():  pBufArray[%d] = 0x%lx, size: %d bytes\n", cnt,pFifoBufObj->pBufArray[cnt], sizeof(long) * (bufSize));
     memset(pFifoBufObj->pBufArray[cnt],0,(sizeof(long) * bufSize));
  }
  pFifoBufObj->pWrkBuf = NULL;
  pFifoBufObj->WrkBufEntries = 0;
  pFifoBufObj->BufSize = bufSize;

  if ( (pFifoBufObj->pBufMutex == NULL) ||
	(pFifoBufObj->pBufsFree == NULL) ||
	(pFifoBufObj->pBufArray == NULL)
     )
  {
        cntrlFifoBufDelete(pFifoBufObj);
        errLogSysRet(LOGIT,debugInfo,
	   "fifoBufCreate: Failed to allocate some resource:");
        return(NULL);
  }

  cntrFifoDmaChannel = dmaChannel;
  pFifoWriteAddr = pFifoWrite;
  cntrlFifoBufInit(pFifoBufObj);

  return( pFifoBufObj );
}

/**************************************************************
*
*  cntrlFifoBufDelete - Deletes FIFO Buffer Object and  all resources
*
*
* RETURNS:
*  OK or ERROR
*
*	Author Greg Brissey 2/7/97
*/
void cntrlFifoBufDelete(FIFOBUF_ID pFifoBufId)
/* FIFOBUF_ID 	pFifoBufId - fifo Buffer Object identifier */
{
   int tid;
   int i;

   if (pFifoBufId != NULL)
   {
      if (pFifoBufId->pBufArray != NULL)
      {
         for(i=0; i < pFifoBufId->NumOfBufs; i++)
         {
             if (pFifoBufId->pBufArray[i] != NULL)
		 free(pFifoBufId->pBufArray[i]);
         }
	 free(pFifoBufId->pBufArray);
      }
      if (pFifoBufId->pBufMutex != NULL)
	 semDelete(pFifoBufId->pBufMutex);
      if (pFifoBufId->pBufsFree != NULL)
	 msgQDelete(pFifoBufId->pBufsFree);
      free(pFifoBufId);
   }
}

int cntrlFifoDmaChanGet()
{
    return ( cntrFifoDmaChannel );
}

/**************************************************************
*
*  cntrlFifoBufInit - Initializes the Fifo Buffer msgQs 
*
*		WARNING: any FIFO words in the working or ready buffers
*			 is lost!
*
* RETURNS:
*  OK or ERROR
*
*	Author Greg Brissey 2/7/97
*/
int cntrlFifoBufInit(FIFOBUF_ID pFifoBufId)
/* FIFOBUF_ID 	pFifoBufId - fifo Buffer Object identifier */
{
  unsigned long *address,*prevaddr,*temp;
  unsigned long bufSize;
  int cnt;

  queueMode = CNTRL_DMA_MODE;

   if (pFifoBufId != NULL)
   {
     /* taskLock();  going to use mutex now,  9/1/05 GMB */
     semTake(pFifoBufId->pBufMutex,WAIT_FOREVER);

     pFifoBufId->queueMode = CNTRL_DMA_MODE;

#ifdef INSTRUMENT
      wvEvent(EVENT_FIFOBUF_INITBUF,NULL,NULL);
#endif

     /* 1st Clean out the MsgQs */
     while ( msgQReceive(pFifoBufId->pBufsFree,(char*) &temp,sizeof(temp),NO_WAIT) != ERROR);

     /* calc up the addresses for the fixed buffers from the one large pool of memory */
     prevaddr = (long *) pFifoBufId->pBufArray;
     bufSize = pFifoBufId->BufSize;

     for(cnt=0; cnt < pFifoBufId->NumOfBufs; cnt++)
     {
        msgQSend(pFifoBufId->pBufsFree,(char*) &(pFifoBufId->pBufArray[cnt]) ,sizeof(long),WAIT_FOREVER,MSG_PRI_NORMAL);
     }
#ifdef XXXX
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
        /* no need for this in buffer since the DMA is scehdule directly here not a another task */
        /* *address = (long) address;  /* 1st entry is addr, 2nd number of fifo word in buffer */
        /* *(address+1) = 0L;  */
        msgQSend(pFifoBufId->pBufsFree,(char*)&address,sizeof(address),WAIT_FOREVER,MSG_PRI_NORMAL);
        prevaddr = address;
        DPRINT3(9,"fifoBufInit(): Bufs Address: 0x%lx = val 0x%lx,entries: %d\n",
		address,*address,*(address+1));
     }
#endif

#ifdef ENABLE_BUFFER_DURATION_CALC
*      bufduration = 0L;
#endif

      /* Now get the 1st buffer and set it up */
      msgQReceive(pFifoBufId->pBufsFree,(char*) &pFifoBufId->pWrkBuf,sizeof(pFifoBufId->pWrkBuf),WAIT_FOREVER);
      /* memset(pFifoBufId->pWrkBuf,0,(sizeof(long) * pFifoBufId->BufSize)); */

      /*  -------------------------------------------  */
      /* Example of using duf device */
      /* int *d = pFifoBufId->pWrkBuf; */
      /* DUFF_DEVICE_8(pFifoBufId->BufSize, *d++ = 0); */
      /*  -------------------------------------------  */

      pFifoBufId->WrkBufEntries = 0;
      pFifoBufId->IndexAddr = pFifoBufId->pWrkBuf;

      DPRINT4(+8,"  Wrk Buf: 0x%lx(addr 0x%lx), Entries: %d , IndexPtr: 0x%lx\n",
	*pFifoBufId->pWrkBuf,pFifoBufId->pWrkBuf,pFifoBufId->WrkBufEntries,
	pFifoBufId->IndexAddr);

      semGive(pFifoBufId->pBufMutex);
      /* taskUnlock(); */
   }
   return(OK);
}

/**************************************************************
*
*  cntrlFifoBufReset - Reset the Fifo Buffers back to the Initial state
*
*		WARNING: any FIFO words in the working or ready buffers
*			 is lost!
*
* RETURNS:
*  OK or ERROR
*
*	Author Greg Brissey 2/7/97
*/
cntrlFifoBufReset(FIFOBUF_ID pFifoBufId)
/* FIFOBUF_ID 	pFifoBufId - fifo Buffer Object identifier */
{
    long nrdy,nfree,total,nwrk,i;
    unsigned long *temp;

    if (pFifoBufId == NULL)
	return(ERROR);

    cntrlFifoBufInit(pFifoBufId); /* drastic action needed */
   return(OK);
}

void cntrlFifoBufSetDecoder(FIFOBUF_ID pFifoBufId, CNTRLBUF_DECODEFUNC funcptr)
{
   pFifoWordDecoder = funcptr;

   semTake(pFifoBufId->pBufMutex,WAIT_FOREVER);
      pFifoBufId->pFifoWordDecoder = funcptr;
   semGive(pFifoBufId->pBufMutex);
}

int cntrlFifoBufModeSet(FIFOBUF_ID pFifoBufId,int mode)
{
  if ((mode >= 0) && (mode <= 2))
  {
      queueMode = mode;
      semTake(pFifoBufId->pBufMutex,WAIT_FOREVER);
         pFifoBufId->queueMode = mode;
      semGive(pFifoBufId->pBufMutex);
      return 0;
  }
  else
     return -1;
}

/**************************************************************
*
*  cntrlFifoBufPut - Put Words into a Working Fifo Buffer 
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
/* FIFOBUF_ID 	pFifoBufId - fifo Buffer Object identifier */
/* unsigned long *words - array of longs to put into buffer */
/* long nWords - Number of longs to put into buffer */
void cntrlFifoBufPut(FIFOBUF_ID pFifoBufId, unsigned long *words, long nWords )
{
   unsigned long max,Entries;
   int i;
   unsigned long *pTmpWrkBuf;       /* local copy of Working Buffer pointer */
   unsigned long BufEntries;   /* The number of entries in ther working buffer */
   int mutexRelFlag;		/* indicate if mutex has been given because MsgQ is going to pend */

#ifdef ENABLE_BUFFER_DURATION_CALC
*   unsigned long calcbufdur(unsigned long *fifowords, int number);
#endif

   DPRINT2(1,"cntrlFifoBufPut(): buffer Addr: 0x%lx, words: %d\n", words,nWords);
   /* taskLock();  use mutex now, 9/1/05  GMB */
   semTake(pFifoBufId->pBufMutex,WAIT_FOREVER);

   Entries = (pFifoBufId->IndexAddr - pFifoBufId->pWrkBuf);
/*  DDR now uses 4096 buffer reset use 1024 
 *  if ( Entries > 1024 )
 *  {
 *    DPRINT3(-4,"cntrlFifoBufPut: Waraning: Entries %lu = Index addr 0x%lx - Bufstrt Addr 0x%lx\n",Entries,
 *		pFifoBufId->IndexAddr,pFifoBufId->pWrkBuf);
 *   }
 */
   /*
    * DPRINT3(-1,"cntrlFifoBufPut: Entries %lu = Index addr 0x%lx - Bufstrt Addr 0x%lx\n",Entries,
    *		pFifoBufId->IndexAddr,pFifoBufId->pWrkBuf);
   */

   max = pFifoBufId->BufSize - Entries;
   while (nWords >= max)
   {
#ifdef USE_DUFF_DEVICE
      /* Example of using duff device */
      /*  -------------------------------------------  */
      if (max > 0)
      {
        DUFF_DEVICE_8(max, *pFifoBufId->IndexAddr++ = *words++);
      }
      /*  -------------------------------------------  */
#else
      for (i=0;i<max;i++)
      {
	*pFifoBufId->IndexAddr++ = *words++;
      }
#endif      

      if ( (pFifoBufId->IndexAddr - pFifoBufId->pWrkBuf) != pFifoBufId->BufSize )
      {
        errLogRet(LOGIT,debugInfo,"cntrlFifoBufPut: buffer entries should equal max, actual entries: %d, max: %d\n",
          (pFifoBufId->IndexAddr - pFifoBufId->pWrkBuf), pFifoBufId->BufSize);
      }
 
      pFifoBufId->WrkBufEntries = pFifoBufId->BufSize;
      DPRINT3(-1,"cntrlFifoBufPut(): Send Buf: 0x%lx = 0x%lx, entries: %lu\n",
		pFifoBufId->pWrkBuf,*pFifoBufId->pWrkBuf,pFifoBufId->WrkBufEntries);
#ifdef INSTRUMENT
      wvEvent(EVENT_FIFOBUF_SNDRDY,NULL,NULL);
#endif

      pTmpWrkBuf = pFifoBufId->pWrkBuf;
       pFifoBufId->IndexAddr = pFifoBufId->pWrkBuf = NULL;   /* Mark Working Buffer as invalid */
      pFifoBufId->WrkBufEntries = 0L;

      /* This should be safe, and giving is need incase of an AA or some other error */
      semGive(pFifoBufId->pBufMutex);

        /* this could pend */
        queueTransfer(pFifoBufId, pTmpWrkBuf,pFifoBufId->BufSize);

      semTake(pFifoBufId->pBufMutex,WAIT_FOREVER);

      if (pFifoBufId->pWrkBuf != NULL)
        errLogRet(LOGIT,debugInfo,"cntrlFifoBufPut: working buffer should be NULL but is NOT, this means a task got in and got a buffer while cntrlFifoBufPut() was executing, will lose a buffer!!!\n" );

#ifdef INSTRUMENT
      wvEvent(EVENT_FIFOBUF_RCVFREE,NULL,NULL);
#endif
      /* if fifoBufForceRdy runs inbetween here, since pFifoBufId->pWrkBuf == NULL, it will not
	 do any thing, so we don't have to worry about it already getting a free buffer */
      /* was == 0 --- leave a couple free */
      /* DPRINT2(-1,"fifoBufPut(): buffs left: %d, Fifo: '%s'\n",
	msgQNumMsgs(pFifoBufId->pBufsFree), ((cntrlFifoRunning() == 1) ? "RUNNING" : "STOPPED")); */
      /* if ((msgQNumMsgs(pFifoBufId->pBufsFree) < 2) && !cntrlFifoRunning()) */
      if ((msgQNumMsgs(pFifoBufId->pBufsFree) < 2) && !getStart4ExpFlag())   /* chnge to getStart4ExpFlag 10/27/05  GMB */
      {
#ifdef INSTRUMENT
        wvEvent(99,NULL,NULL);
#endif
        DPRINT(-1,"fifoBufPut(): startFifo4Exp()\n");
        startFifo4Exp(); /* cntrlFifoStart(); */
      }

      /* This should be safe, and giving is need incase of an AA or some other error */
      /* if not going to pend then keep the mutex */
      if (msgQNumMsgs(pFifoBufId->pBufsFree) < 1)
      {
        semGive(pFifoBufId->pBufMutex);
        mutexRelFlag = 1;
      }
      else
      {
        mutexRelFlag = 0;
      }

      /* Maybe Pending Here */
      msgQReceive(pFifoBufId->pBufsFree,(char*) &pFifoBufId->pWrkBuf,sizeof(pFifoBufId->pWrkBuf),
		WAIT_FOREVER);

      /* ony re-acquire mutex if it had been released */
      if (mutexRelFlag == 1)
         semTake(pFifoBufId->pBufMutex,WAIT_FOREVER);

      /* for dianostics, clear the buffer with zeros  */
      /* memset(pFifoBufId->pWrkBuf,0,(sizeof(long) * pFifoBufId->BufSize)); */


#ifdef ENABLE_BUFFER_DURATION_CALC
*      bufduration = 0L;
#endif

      pFifoBufId->WrkBufEntries = 0L;
      pFifoBufId->IndexAddr = pFifoBufId->pWrkBuf;
      nWords = nWords - max;
      max = pFifoBufId->BufSize;	/* just got an empty buffer, reset max to maximum */

      DPRINT4(-1,"fifoBufPut(): New Wrk Buf: 0x%lx(addr 0x%lx), Entries: %d, IndexPtr: 0x%lx\n",
	*pFifoBufId->pWrkBuf,pFifoBufId->pWrkBuf,pFifoBufId->WrkBufEntries,pFifoBufId->IndexAddr);
   }

#ifdef USE_DUFF_DEVICE
   /* Example of using duff device */
   /*  -------------------------------------------  */
   if (nWords > 0)
   {
#ifdef ENABLE_BUFFER_DURATION_CALC
     /*  an attempt to use duration as a metric to determine when to flush fifo buffer for Read User Byte  */
     /* was to CPU intensive for gradient controller */
     * if (bufdurflag > 0)
     * {
     *    bufduration += calcbufdur(words, nWords);
     * }
#endif
     DUFF_DEVICE_8(nWords, *pFifoBufId->IndexAddr++ = *words++);
   }
   /*  -------------------------------------------  */
#else
   for (i=0;i<nWords;i++)
   {
       DPRINT2(2,"instr[%d] = 0x%lx\n",i,*words);
       *pFifoBufId->IndexAddr++ = *words++;
   }
#endif

   pFifoBufId->WrkBufEntries = (pFifoBufId->IndexAddr - pFifoBufId->pWrkBuf);

   semGive(pFifoBufId->pBufMutex);
   /* taskUnlock(); */

      /* testing */
#ifdef ENABLE_BUFFER_DURATION_CALC
   * if ( (bufdurflag > 0) && (DebugLevel > 2) )
   * {
   *      /* DPRINT1(-5,"---->  Working Buffer Entries: %d\n",pFifoBufId->WrkBufEntries); */
   *      prtbufduration(pFifoBufId->WrkBufEntries);
   *   }
#endif

   return;
}



/*
 * Schedule a  buffer to be DMA'd into the fifo for the number of times requested
 *
 */
int cntrlFifoListCopy(FIFOBUF_ID pFifoBufId, long *buffer, int size, int ntimes,int rem, int startfifo)
{
    int freeSGnodes,freeTxDesc,maxTimes;

    if (size > MAX_DMA_TRANSFER_SIZE)
    {
        errLogRet(LOGIT,debugInfo,"cntrlFifoListCopy: request to DMA buffer of %d word, max: %d",
                                                                size,MAX_DMA_TRANSFER_SIZE);
	return(-1);
    }

    /* force any previous pending FIFO words into the FIFO, prior to copying this buffer */
    cntrlFifoBufForceRdy(pFifoBufId);


    freeSGnodes = dmaNumFreeSGListNodes();
    freeTxDesc = dmaNumFreeTxDescs();
    maxTimes = freeSGnodes * freeTxDesc;
    DPRINT5(-1,"cntrlFifoListCopy(): buffer: 0x%lx, size: %d, times: %d, rem: %d, startfifo: %d\n",
	buffer, size, ntimes, rem, startfifo);

    DPRINT3(-1,"cntrlFifoListCopy(): Free TxDesc: %d, Free SgNodes: %d, MaxTimes: %d\n", freeTxDesc, freeSGnodes,maxTimes);

    if (queueMode == CNTRL_DMA_MODE)
    { 
       if ( (freeTxDesc > 0) && (freeSGnodes > 1) )
       {
          int sgNodesRequired;
          sgNodesRequired = ntimes + ((rem > 0) ? 1 : 0);
          if (sgNodesRequired <= freeSGnodes)
          {
              DPRINT2(-1,"cntrlFifoListCopy(): call cntrlFifoXferSGList: Times: %d, Rem: %d\n",ntimes,rem);
           cntrlFifoXferSGList(pFifoBufId, buffer, size, ntimes, rem, startfifo);
	      ntimes = 0;
          }
          else
          {
              DPRINT2(-1,"cntrlFifoListCopy(): call cntrlFifoXferSGList: Times: %d, Rem: %d\n",freeSGnodes,0);
              cntrlFifoXferSGList(pFifoBufId, buffer, size, freeSGnodes, 0, startfifo);
	      ntimes -= freeSGnodes;
          }
          execNextDmaRequestInQueue(cntrFifoDmaChannel);   /* fire off DMA engine and begin actual transfer */
       }

       if ((ntimes > 0) || (rem > 0))
       {
           DPRINT2(-1,"cntrlFifoListCopy(): call cntrlFifoXferCopy: Times: %d, Rem: %d\n",ntimes,rem);
           cntrlFifoXferCopy(pFifoBufId, buffer, size, ntimes, rem, startfifo);
       }

       execNextDmaRequestInQueue(cntrFifoDmaChannel);   /* fire off DMA engine and begin actual transfer */
   }
   else if (queueMode == CNTRL_DECODER_MODE)
   {
         DPRINT5(-1,"cntrlFifoListCopy(): buffer: 0x%lx, size: %d, ntimes: %d, rem: %d, startfifo: %d\n",
			buffer,size,ntimes,rem,startfifo);
         if (pFifoWordDecoder != NULL)
	     (*pFifoWordDecoder)((UINT32*) buffer,size);
   }
   else
   {
         DPRINT(-1,"cntrlFifoListCopy(): invalid mode\n");
   }
}

int cntrlFifoBufCopy(FIFOBUF_ID pFifoBufId, long *buffer, int size, int ntimes, int rem, int startfifo)
{
   int cntrlFifoXferCopy(FIFOBUF_ID pFifoBufId, long *buffer, int size, int ntimes,int rem, int startfifo);

   /* any previous pending FIFO words into the FIFO, prior to copying this buffer */
   cntrlFifoBufForceRdy(pFifoBufId);

   cntrlFifoXferCopy(pFifoBufId, buffer, size, ntimes, rem, startfifo);

   execNextDmaRequestInQueue(cntrFifoDmaChannel);   /* fire off DMA engine and begin actual transfer */

   return(0);
}

int cntrlFifoBufXfer(FIFOBUF_ID pFifoBufId, long *buffer, int size, int ntimes, int rem, int startfifo)
{
   int cntrlFifoXferSGList(FIFOBUF_ID pFifoBufId, long *buffer, int size, int ntimes,int rem, int startfifo);

   /* any previous pending FIFO words into the FIFO, prior to copying this buffer */
   cntrlFifoBufForceRdy(pFifoBufId);

   cntrlFifoXferSGList(pFifoBufId, buffer, size, ntimes, rem, startfifo);

   execNextDmaRequestInQueue(cntrFifoDmaChannel);   /* fire off DMA engine and begin actual transfer */

   return(0);
}

/*
 *
 * This method just queues the buffers ntimes, does not use SG Lists
 *
 */
int cntrlFifoXferCopy(FIFOBUF_ID pFifoBufId, long *buffer, int size, int ntimes,int rem, int startfifo)
{
   int i,status;
   /* Now queue up ntimes DMA trasnfer of same buffer, later when we have time we can change
      this to a SG List, and obtain bettter performance
   */
   i = 0;
   DPRINT3(-1,"cntrlFifoBufCopy(): queue dma: %d, src: 0x%lx, size: %d\n",i,buffer,size);
   for(i=0; i < ntimes; i++)
   {
       status = queueDmaTransfer(cntrFifoDmaChannel, MEMORY_TO_PERIPHERAL, NO_SG_LIST,
       		(UINT32) buffer, (UINT32) pFifoWriteAddr,
                              size, NULL, NULL);
       cumFifoWordsDma += size;
   }
   if (rem > 0)
   {
      DPRINT2(-1,"cntrlFifoBufCopy(): queue dma: %d, src: 0x%lx\n",i,buffer);
      status = queueDmaTransfer(cntrFifoDmaChannel, MEMORY_TO_PERIPHERAL, NO_SG_LIST,
			(UINT32) buffer, (UINT32) pFifoWriteAddr,
                        rem, NULL, NULL);
      cumFifoWordsDma += rem;
   }
   return status;
}

int cntrlFifoXferSGList(FIFOBUF_ID pFifoBufId, long *buffer, int size, int ntimes,int rem, int startfifo)
{
   int i,status;
   txDesc_t *sgHead;
   sgnode_t *newNode;
   int startFreeTxDesc, startFreeSGnodes;   /* just for diagnostics output */
   int freeTxDesc, freeSGnodes;   /* just for diagnostics output */

   /* for diagnostics, cacheFlush(DATA_CACHE, (void *) buffer, (size_t)(size * sizeof(int))); */

   /* Now queue up ntimes DMA trasnfer of same buffer, later when we have time we can change
      this to a SG List, and obtain bettter performance
   */
   /* startFreeTxDesc = dmaNumFreeTxDescs(); */
   /* startFreeSGnodes = dmaNumFreeSGListNodes(); */
    /* ------- Create an SG list header and add it to the queue ------- */
     /* Gets a TxDesc fills in certain field and apppends it to dmaQueue */
   status = -1;
   sgHead = dmaSGListCreate(cntrFifoDmaChannel);
   if (sgHead != NULL)
   {

      for(i=0; i < ntimes; i++)
      {
        if (size <= MAX_DMA_TRANSFER_SIZE)
        {
           /* ------------ Get a SG list node from the driver ------------ */
           /* get free SGNode, fill in sructure, will pend if no free SGNodes  */
           newNode = dmaSGNodeCreate((UINT32) buffer, (UINT32) pFifoWriteAddr, (UINT32) size);
           cumFifoWordsDma += size;

            /* ----------- Attach the list node to the SG list ------------ */
            /* DPRINT1(-1,"cntrlFifoBufXfer: SG node: %d\n",i+1); */
            dmaSGNodeAppend(newNode, sgHead);
        }
        else
        {
          char *pbuf;
          int tsize,xferSize;

          sgHead->txfrControl = getDMA_ControlWord(MEMORY_TO_PERIPHERAL);
          sgHead->transferType = MEMORY_TO_PERIPHERAL;
          sgHead->srcAddr = (unsigned int*) buffer;
          sgHead->dstAddr = (unsigned int*) pFifoWriteAddr;
          sgHead->txfrSize = size;
          buildSG_List(sgHead);   /* DMA transfer descriptor  */
          cumFifoWordsDma += size;
          


#ifdef THE_OLD_WAY_HAS_BUG
          /* stimes = size / MAX_DMA_TRANSFER_SIZE + 
		   (((size % MAX_DMA_TRANSFER_SIZE) > 0) ? 1 : 0); */
          pbuf = (char*) buffer;
          tsize = size;
          while ( tsize > 0 )
          {
            xferSize = (tsize > MAX_DMA_TRANSFER_SIZE) ? MAX_DMA_TRANSFER_SIZE : tsize;
            newNode = dmaSGNodeCreate((UINT32) pbuf, (UINT32) pFifoWriteAddr, (UINT32) xferSize);
            DPRINT3(2,"cntrlFifoXferSGList: dmaSGNodeCreate(src: 0x%lx,fifo: 0x%lx, size: %ld)\n",
                          pbuf,pFifoWriteAddr,xferSize);
            dmaSGNodeAppend(newNode, sgHead);
            tsize = tsize - xferSize;
            pbuf = pbuf + xferSize;
          }
#endif
        }

      }
      if (rem > 0)
      {
        /* ------------ Get a SG list node from the driver ------------ */
        /* get free SGNode, fill in sructure, will pend if no free SGNodes  */
        newNode = dmaSGNodeCreate((UINT32) buffer, (UINT32) pFifoWriteAddr, (UINT32) rem);

         /* ----------- Attach the list node to the SG list ------------ */
         DPRINT1(-1,"cntrlFifoBufXfer: SG node: %d\n",i+1);
         dmaSGNodeAppend(newNode, sgHead);
         cumFifoWordsDma += rem;
      }
      status = queueDmaTransfer(cntrFifoDmaChannel, MEMORY_TO_PERIPHERAL, SG_LIST,
			(UINT32) sgHead, (UINT32) 0, 0, NULL, NULL);

      /* dmaPrintSGTree(sgHead); */

      /* freeTxDesc = dmaNumFreeTxDescs(); */
      /* freeSGnodes = dmaNumFreeSGListNodes(); */
      /* DPRINT4(-1,"cntrlFifoXferSGList(): TxDesc Used: %d, Left: %d, SGnodes Used: %d, Left: %d\n",
       * startFreeTxDesc - freeTxDesc,freeTxDesc,startFreeSGnodes - freeSGnodes,freeSGnodes); */
   }
   return status;
}

/**************************************************************
*
*  cntrlFifoBufForceRdy - Force an unfilled buffer to be placed on the Read List
*
*	If the Working Buffer Address is None NULL then place it on the Ready List
*	Then obtain a new Working Buffer
*
* RETURNS:
*   NONE 
*
*	Author Greg Brissey 2/7/97
*/
void cntrlFifoBufForceRdy(FIFOBUF_ID pFifoBufId)
/* FIFOBUF_ID 	pFifoBufId - fifo Buffer Object identifier */
{

    unsigned long *pTmpWrkBuf;       /* local copy of Working Buffer pointer */
    unsigned long BufEntries;   /* The number of entries in ther working buffer */
    int mutexRelFlag;

   /* taskLock(); using mutex now, 9/1/05 GMB */
   semTake(pFifoBufId->pBufMutex,WAIT_FOREVER);

   /* while waiting for Mutex fifoBufPut may have put a buffer in the ready Q */
   if (pFifoBufId->pWrkBuf != NULL)
   {
     pFifoBufId->WrkBufEntries = (pFifoBufId->IndexAddr - pFifoBufId->pWrkBuf);
     if ( (pFifoBufId->WrkBufEntries != 0) )
     {
      DPRINT3(-1,"fifoBufForce(): Send Buf: *0x%lx=0x%lx, entries: %lu\n",
		pFifoBufId->pWrkBuf,*pFifoBufId->pWrkBuf,pFifoBufId->WrkBufEntries);
#ifdef INSTRUMENT
        wvEvent(EVENT_FIFOBUF_SNDRDY,NULL,NULL);
#endif

       BufEntries = pFifoBufId->WrkBufEntries;
       pTmpWrkBuf = pFifoBufId->pWrkBuf;
       pFifoBufId->IndexAddr = pFifoBufId->pWrkBuf = NULL;   /* Mark Working Buffer as invalid */
       pFifoBufId->WrkBufEntries = 0L;

      semGive(pFifoBufId->pBufMutex);  /* incase we pend in the following command */
      queueTransfer(pFifoBufId, pTmpWrkBuf, BufEntries);
      semTake(pFifoBufId->pBufMutex,WAIT_FOREVER);

      if (pFifoBufId->pWrkBuf != NULL)
        errLogRet(LOGIT,debugInfo,"fifoBufForce: working buffer should be NULL but is NOT, this means a task got in and got a buffer while fifoBufForce() was executing, will lose a buffer!!!\n" );
#ifdef INSTRUMENT
        wvEvent(EVENT_FIFOBUF_RCVFREE,NULL,NULL);
#endif


       /* DPRINT2(-1,"fifoBufForce(): buffs left: %d, Fifo: '%s'\n",
	   msgQNumMsgs(pFifoBufId->pBufsFree), ((cntrlFifoRunning() == 1) ? "RUNNING" : "STOPPED")); */
       /* if ((msgQNumMsgs(pFifoBufId->pBufsFree) < 2) && !cntrlFifoRunning()) */
       if ((msgQNumMsgs(pFifoBufId->pBufsFree) < 2) && !getStart4ExpFlag())   /* chnge to getStart4ExpFlag 10/27/05  GMB */
       {
#ifdef INSTRUMENT
           wvEvent(99,NULL,NULL);
#endif
           DPRINT(-1,"fifoBufForce(): startFifo4Exp()\n");
           startFifo4Exp(); /* cntrlFifoStart(); */
       }

      /* if not going to pend then keep the mutex */
      if (msgQNumMsgs(pFifoBufId->pBufsFree) < 1)
      {
        semGive(pFifoBufId->pBufMutex);
        mutexRelFlag = 1;
      }
      else
      {
        mutexRelFlag = 0;
      }

       msgQReceive(pFifoBufId->pBufsFree,(char*) &pFifoBufId->pWrkBuf,sizeof(pFifoBufId->pWrkBuf),
		  WAIT_FOREVER);
      /* for dianostics, clear the buffer with zeros  */
      /* memset(pFifoBufId->pWrkBuf,0,(sizeof(long) * pFifoBufId->BufSize)); */

      /* ony re-acquire mutex if it had been released */
      if (mutexRelFlag == 1)
         semTake(pFifoBufId->pBufMutex,WAIT_FOREVER);

      /* DPRINT2(-1,"fifoBufForce(): buffs left: %d, Fifo: '%s'\n",
  	* msgQNumMsgs(pFifoBufId->pBufsFree), ((cntrlFifoRunning() == 1) ? "RUNNING" : "STOPPED")); */

#ifdef ENABLE_BUFFER_DURATION_CALC
*      bufduration = 0L;
#endif

      pFifoBufId->WrkBufEntries = 0L;
      pFifoBufId->IndexAddr = pFifoBufId->pWrkBuf;
/*
      DPRINT4(-1,"fifoBufForce(): Wrk Buf: 0x%lx (addr 0x%lx), Entries: %d, IndexPtr: 0x%lx\n",
	*pFifoBufId->pWrkBuf,pFifoBufId->pWrkBuf,pFifoBufId->WrkBufEntries,
	pFifoBufId->IndexAddr);
*/
     }
   }
   semGive(pFifoBufId->pBufMutex);
   /* taskUnlock(); */
   return;
}

/**************************************************************
*
*  cntrlFifoBufScheduleRdy - Schedule an unfilled buffer to be placed on the Read List
*
*	If the Working Buffer Address is not NULL "AND" there are sufficient words in the buffer
*       then place it on the Ready List
*	Then obtain a new Working Buffer
*
* RETURNS:
*   NONE 
*
*	Author Greg Brissey 2/21/2006
*/
void cntrlFifoBufScheduleRdy(FIFOBUF_ID pFifoBufId)
/* FIFOBUF_ID 	pFifoBufId - fifo Buffer Object identifier */
{
    /* This is an attempt to avoid a problem that has been seen. Where multiple DMAs are queued
     * which have small number of words to be DMA'd. This can result in the DMA interrupt happening faster than
     * the DMA ISR in VxWorks can handle. And for some unknown reason at present, the interrupt stack gets consumed
     * until some point the stack overflows causing system failure, typically controller rebooting.
     * Thus if the ISR takes ~ 10 us then DMAing no less than 128 words should ensure DMA duration is longer than 10 us

     * In places where it is important to be sure the buffer is queued for DMA then cntrlFifoBufForceRdy() remains
     * the method to use.  cntrlFifoBufScheduleRdy can probably be safely used at the NEXTCODESET rather than 
     * cntrlFifoBufForceRdy which is used now. Though it may break IMAGING acquire loops (EPI)
     */
    if (pFifoBufId->WrkBufEntries >= MINIMUM_ENTRIES_FOR_DMA)
    {
        cntrlFifoBufForceRdy(pFifoBufId);
    }
}

/**************************************************************
*
*  cntrlFifoBufFree - return the Number of buffer still in the
*                     Free list.  
*          Note this does not include the working buffer
*
*
* RETURNS:
*  Number buffers still in the free list
*
*       Author Greg Brissey 6/9/04
*/
int cntrlFifoBufFree(FIFOBUF_ID pFifoBufId)
{
    return( msgQNumMsgs(pFifoBufId->pBufsFree) );
}

/**************************************************************
*
*  cntrlFifoBufReturn - Put the Given Buffer Addres on the Free List
*
*
* RETURNS:
*  NONE
*
*	Author Greg Brissey 2/7/97
*/
void cntrlFifoBufReturn(FIFOBUF_ID pFifoBufId,long *bufaddr)
/* FIFOBUF_ID 	pFifoBufId - fifo Buffer Object identifier */
/* long *bufaddr - buffer  address to be placed on the free list */
{
   /* taskLock(); don't use taskLock, 9/1/05 GMB */
#ifdef INSTRUMENT
   wvEvent(EVENT_FIFOBUF_SNDFREE,NULL,NULL);
#endif
   msgQSend(pFifoBufId->pBufsFree,(char*)&bufaddr,sizeof(bufaddr),
		NO_WAIT,MSG_PRI_NORMAL);
   /* taskUnlock(); */
}

/*  clear the calculated FIFO Instruction words queued for DMA to FIFO */
cntrlClrCumDmaCnt()
{
   cumFifoWordsDma = 0L;
}

/*  return the value of the calculated FIFO Instruction words queued for DMA to FIFO */
cntrlGetCumDmaCnt()
{

 return cumFifoWordsDma;
}

/*
 * Turn on the Fifo Instruction software decoder, to calc duration of timer words
 * Warning this can software intensive so FIFO underflows may occur!
 *
 *   Author: Greg Brissey 9/16/05
 */
cntrlDecodeOn(int prtflag)
{
   decodeFifoWordsFlag = 1;  /* if true queueTransfer will decode fifo words to be dma for duration */
   prtdecodeflag = (prtflag > 0) ? 0 : 1;
   printf("decodeFifoWordsFlag: %d, prtdecodeflag: %d\n",decodeFifoWordsFlag,prtdecodeflag);
}

/*
 * Turn off the Fifo Instruction software decoder, to calc duration of timer words
 * Warning this can software intensive so FIFO underflows may occur!
 *
 *   Author: Greg Brissey 9/16/05
 */
cntrlDecodeOff()
{
   decodeFifoWordsFlag = 0;  /* if true queueTransfer will decode fifo words to be dma for duration */
   prtdecodeflag = 1;
   printf("decodeFifoWordsFlag: %d, prtdecodeflag: %d\n",decodeFifoWordsFlag,prtdecodeflag);
}


queueTransfer(FIFOBUF_ID pFifoBufId, UINT32 buffer, int size)
{
   int i;
   unsigned long duration;
   switch(queueMode)
   {
      case 0:
           dmaXfer(cntrFifoDmaChannel, MEMORY_TO_PERIPHERAL, NO_SG_LIST,
			(UINT32) buffer, (UINT32) pFifoWriteAddr,
                        size, pFifoBufId->pBufsFree, NULL);
           cumFifoWordsDma += size;

/* #ifdef DIAGNOSTIC_USE  */
           if (decodeFifoWordsFlag > 0 )
           {
             int *tmpptr;
             execFunc("clrBufTime",NULL, NULL, NULL, NULL, NULL);
             for (i=0,tmpptr = (int*) buffer; i < size; i++)  
             {
               /* DPRINT2(-1,"QXfer: FifoWord[%d]: 0x%lx\n",i, *tmpptr); */
               if ( (*tmpptr & 0x7C000000) == 0L )
               {
                   DPRINT2(-1,"QXfer: FifoWord[%d]: 0x%lx\n",i, *tmpptr);
               }
               execFunc("fifoDecode",*tmpptr, prtdecodeflag, NULL, NULL, NULL);
               *tmpptr++;
             }
	     duration = execFunc("getDecodedDuration",NULL, NULL, NULL, NULL);
             fidduration += duration;
	     cumduration +=  duration;
             execFunc("prtFifoTime","Q_DMA", NULL, NULL, NULL, NULL);
            }
/* #endif */

           break;
      case 1:
           if (pFifoWordDecoder != NULL)
	     (*pFifoWordDecoder)((UINT32*) buffer,size);
           else
             errLogRet(LOGIT,debugInfo,"No deocder specified, skipping\n");

           cntrlFifoBufReturn(pFifoBufId,(long*)buffer);
           break;

      default:
         dmaXfer(cntrFifoDmaChannel, MEMORY_TO_PERIPHERAL, NO_SG_LIST,
			(UINT32) buffer, (UINT32) pFifoWriteAddr,
                        size, pFifoBufId->pBufsFree, NULL);
            break;

      /*   FIFO programmed IO varient */
      /* cntrlFifoPIO(buffer,size);
         fifoBufReturn(pFifoBufId,buffer); */
   }
}

cntrlFifoBufXfrAbort(FIFOBUF_ID pFifoBufId)
{
   if (queueMode == 0) /* DMA */
   {
      abortActiveDma(cntrFifoDmaChannel);
      clearDmaRequestQueue(cntrFifoDmaChannel);
   }
}

/**************************************************************
*
*  cntrlFifoBufWkEntries - Returns number of Entries in the present Working Buffer
*
*
* RETURNS:
*  number of entries in working buffer
*
*	Author Greg Brissey 2/7/97
*/
unsigned long cntrlFifoBufWkEntries(FIFOBUF_ID pFifoBufId)
/* FIFOBUF_ID 	pFifoBufId - fifo Buffer Object identifier */
{
   ulong_t NumEntries;
   /* taskLock(); don't use taskLock,  9/1/05 GMB */
   semTake(pFifoBufId->pBufMutex,WAIT_FOREVER);
     NumEntries = pFifoBufId->WrkBufEntries;	/* assure an atomic operation */
   semGive(pFifoBufId->pBufMutex);
   /* taskUnlock(); */
   return (NumEntries);
}

/********************************************************************
* cntrlFifoBufShow - display the status information on the FIFO Buffer Object
*
*  This routine display the status information of the FIFO Buffer Object
*
*
*  RETURN
*   VOID
*
*/
void cntrlFifoBufShow(FIFOBUF_ID pFifoBufId, int level)
/* FIFOBUF_ID 	pFifoBufId - fifo Buffer Object identifier */
/* int level - level of output */
{
   if (pFifoBufId == NULL)
   {
     printf("fifoShow: FIFO Object pointer is NULL.\n");
     return;
   }
   printf("Addr of This Obj: 0x%lx\n",pFifoBufId);
   printf("Buffer Pool Addr: 0x%lx \n",pFifoBufId->pBufArray);
   printf("Buffers:   Num %d,  Size: %lu\n",pFifoBufId->NumOfBufs, pFifoBufId->BufSize);
   printf("Working Buffer: Addr - 0x%lx, Val - 0x%lx\n",
	    pFifoBufId->pWrkBuf,*pFifoBufId->pWrkBuf);
   printf("       Entries: Addr - 0x%lx, Num - %lu\n",
	    &pFifoBufId->WrkBufEntries,pFifoBufId->WrkBufEntries);
   printf("         Index: Addr - 0x%lx, Val - 0x%lx\n",
	    pFifoBufId->IndexAddr,*pFifoBufId->IndexAddr);

   printf("\nFIFO Buffer Free List MsgQ (0x%lx): \n",pFifoBufId->pBufsFree);
   msgQInfoPrint(pFifoBufId->pBufsFree);
   if (level > 0)
     printmsgQEntries(pFifoBufId->pBufsFree);
/*
   printf("\nFIFO Buffer Ready List MsgQ (0x%lx): \n",pFifoBufId->pBufsRdy);
   msgQInfoPrint(pFifoBufId->pBufsRdy);
   if (level > 0)
     printmsgQEntries(pFifoBufId->pBufsRdy);
*/
   printf("\nFIFO Buffer Mutex Semaphore (0x%lx): \n",pFifoBufId->pBufMutex);
   printSemInfo(pFifoBufId->pBufMutex,"Fifo Buf Mutex",1);
   semShow(pFifoBufId->pBufMutex,level);

}
printmsgQEntries(MSG_Q_ID pMsgQ)
{
   int i,pPendTasks[4];
   char *msgList[1024];
   int   msgLenList[1024];
   MSG_Q_INFO MsgQinfo;
   unsigned long *ptr;
   int numMsgs;

   memset(&MsgQinfo,0,sizeof(MsgQinfo));
   MsgQinfo.taskIdListMax = 4;
   MsgQinfo.taskIdList = &pPendTasks[0];
   MsgQinfo.msgListMax = 1024;
   MsgQinfo.msgPtrList = &msgList[0];
   MsgQinfo.msgLenList = &msgLenList[0];

   msgQInfoGet(pMsgQ,&MsgQinfo);

   printf("MsgQ Entries: \n");
   printf("Buffer Addr 		*Addr		\n"); 
   numMsgs = (MsgQinfo.numMsgs <= MsgQinfo.msgListMax) ? MsgQinfo.numMsgs : MsgQinfo.msgListMax;
   for(i=0;i < numMsgs; i++)
   {
      memcpy(&ptr,MsgQinfo.msgPtrList[i],sizeof(long)); /* copy buffer address into pointer */
      printf("%d: 0x%lx		0x%lx	\n", i,ptr,*ptr);
   }
}

fifoBufsShow(int level)
{
   cntrlFifoBufShow(pCntrlFifoBuf, level);
   return 0;
}
/*----------------------------------------------------------------------*/
/* cntrlFifoBufShwResrc							*/
/*     Show system resources used by Object (e.g. semaphores,etc.)	*/
/*	Useful to print then related back to WindView Events		*/
/*----------------------------------------------------------------------*/
VOID cntrlFifoBufShwResrc(FIFOBUF_ID pFifoBufId, int indent)
{
   int i;
   char spaces[40];

   for (i=0;i<indent;i++) spaces[i] = ' ';
   spaces[i]='\0';

   printf("%sFifo Buffer Obj: 0x%lx\n",spaces,pFifoBufId);
   printf("%s    Mutex:       pBufMutex  ---- 0x%lx\n",spaces,pFifoBufId->pBufMutex);
   printf("%s    MsgQs:       pBufsFree  ---- 0x%lx\n",spaces,pFifoBufId->pBufsFree);
   /* printf("%s    MsgQs:       pRdyBufs  ----- 0x%lx\n",spaces,pFifoBufId->pBufsRdy); */
   printf("\n");
}

#ifdef ENABLE_BUFFER_DURATION_CALC
* enableFifoBufDur()
* {
*    unsigned long calcbufdur(unsigned long *fifowords, int number);
*    bufdurflag = 1;
*    if (pCntrlFifoBuf->WrkBufEntries > 0)
*    {
*       bufduration = calcbufdur((unsigned long *) pCntrlFifoBuf->pWrkBuf, pCntrlFifoBuf->WrkBufEntries);
*       prtbufduration(pCntrlFifoBuf->WrkBufEntries);
*    }
*      
* }
* disableFifoBufDur()
* {
*    bufdurflag = 0;
* }
* unsigned long calcbufdur(unsigned long *fifowords, int number)
* {
*     int *tmpptr;
*     int i;
*     unsigned long duration;
*     execFunc("clrBufTime",NULL, NULL, NULL, NULL, NULL);
*     for (i=0,tmpptr = (int*) fifowords; i < number; i++)  
*     {
*       execFunc("fifoDecode",*tmpptr, 1, NULL, NULL, NULL); /* decode without printing */
*       *tmpptr++;
*     }
*     duration = execFunc("getDecodedDuration",NULL, NULL, NULL, NULL);
*     DPRINT3(+5,"buf Incr: %d entries, dur: %lu ticks, %7.4f usec\n",
* 		number, duration, ((double) duration) / 80.0 );
*     /* execFunc("prtFifoTime","Working_Buffer", NULL, NULL, NULL, NULL); */
*     return duration;
* }
*
* unsigned long bufDurationGet()
* {
*    return( bufduration );
* }
* 
* prtbufduration(int num)
* {
*    /* DPRINT2(-1,"----->>  Wrk Buf Duration: %llu ticks, %18.4f usecBuffer: %lu ticks, %7.4f usec\n\n", */
*    DPRINT3(-5,"----->>  Wrk Bufr: %d entries,  Duration: %lu ticks, %7.4f usec\n",
* 		num, bufduration, ((double) bufduration) / 80.0);
*    return 0;
* }
* 
#endif

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
  cntrlFifoBufForceRdy(ptstbuf);
}

tstshw()
{
   fifoBufShow(ptstbuf,0);
}
#endif
