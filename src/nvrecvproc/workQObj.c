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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef LINUX
#include <thread.h>
#endif
#include <pthread.h>

// #define _POSIX_C_SOURCE 1
#include "errLogLib.h"
#include "rngBlkLib.h"
#include "hostAcqStructs.h"
#include "fileObj.h"
#include "mfileObj.h"
#include "workQObj.h"

extern int rngBlkGet(RINGBLK_ID rngd,long* buffer,int size);
extern int rngBlkPut(RINGBLK_ID rngd,long* buffer,int size);

/*
modification history
--------------------
4-08-05,gmb  created
*/

/*
DESCRIPTION

 Work Queue Object for recvproc

*/

/* WORKQ_ID workQCreate(void *pWorkDesc, int maxWorkQentries, int dataBufMemLimit_MB) */
WORKQ_ID workQCreate(void *pWorkDesc, int maxWorkQentries)
{
  WORKQ_ID pWorkQ;
  WORKQINVARIENT_ID pWrkQInvar;
  WORKQ_ENTRY_ID wkrQAddrs;
  FID_STAT_BLOCK *fidStatAddr;
  int i,sizebytes;

  pWorkQ = (WORKQ_ID) malloc(sizeof(WORKQ_OBJECT)); /* create structure */
  if (pWorkQ == NULL) 
     return (NULL);

  memset(pWorkQ,0,sizeof(WORKQ_OBJECT));

  pWrkQInvar = (WORKQINVARIENT_ID) malloc(sizeof(WORKQ_INVARIENT)); /* create structure */
  if (pWrkQInvar == NULL) 
     return (NULL);

  pWorkQ->pInvar = pWrkQInvar;

  memset(pWrkQInvar,0,sizeof(WORKQ_INVARIENT));

  pWrkQInvar->pRcvrDesc = pWorkDesc;
  
  pWorkQ->maxWorkQs = maxWorkQentries;
  pWorkQ->numWorkQs = 0;   /* calc and created in workQDataBufsInit() */

  /* maximum allowed memory usage for data buffers */
  // pWorkQ->maxDataBufMem = 1048576 * dataBufMemLimit_MB;  /* now in bytes */

  /* blocking ring buffer that will hold workQ entry addresses */
  pWorkQ->pBlkRng = rngBlkCreate(pWorkQ->maxWorkQs,"Work Queue", 1);

  /*  malloc memory for all the work Qs */
  sizebytes = maxWorkQentries * sizeof(WORKQ_ENTRY);
  pWorkQ->pWrkQBuffs = (WORKQ_ENTRY_ID) malloc( sizebytes);
  memset(pWorkQ->pWrkQBuffs,0,sizebytes);
  DPRINT1(1,"workQCreate: workQ malloc: %d\n", sizebytes);

  /*  malloc memory for all the Fid Stat BLocks */
  sizebytes = maxWorkQentries * sizeof(FID_STAT_BLOCK);
  pWorkQ->pFidStatBufs = (FID_STAT_BLOCK*) malloc( sizebytes );
  memset(pWorkQ->pFidStatBufs,0,sizebytes);
  DPRINT1(1,"workQCreate: FID StstaBlock malloc: %d\n", sizebytes);

  wkrQAddrs = pWorkQ->pWrkQBuffs;
  fidStatAddr = pWorkQ->pFidStatBufs;
  
  /* initialize workQ references, etc. */
  /* fill ring buffer with free list of buffer addresses */
  for(i=0;i< maxWorkQentries;i++)
  {
       /* printf("wrkQaddr: 0x%lx, statBlkAddr: 0x%lx\n",wkrQAddrs,fidStatAddr); */
       wkrQAddrs->pFidStatBlk = fidStatAddr;
       wkrQAddrs->pWorkQObj = pWorkQ;     /* reference back to the workQObj */
       wkrQAddrs->pInvar = pWrkQInvar;     /* reference back to the workQ Invarients */
       rngBlkPut(pWorkQ->pBlkRng, (long *) &wkrQAddrs,1);
       wkrQAddrs++;
       fidStatAddr++;
  }
  
  return(pWorkQ);
}


void setMaxWorkQMemoryUsage(WORKQ_ID pWorkQ, long long maxMemoryUsageBytes)
{
   pWorkQ->maxFidDataMemUsage = maxMemoryUsageBytes;
}

int workQFree(WORKQ_ID pWorkQ, WORKQ_ENTRY_ID workQEntry)
{
   /* printf("put addr: 0x%lx\n",*bufAddr); */
   return( rngBlkPut(pWorkQ->pBlkRng,(long*) &workQEntry,1) );
}

#ifdef XXX
static int maxWorkQSize(WORKQ_ID pWorkQ, int numActiveDDRs, long long memSize, int fidSizeBytes, int nf)
{
    unsigned int calc_nbufs;
    unsigned int FidSize_NF_Corrected;

   /* note we divide by nf since packing happens in Recvproc not console */
   /* i.e. each NF is recieved as a FID, which is subsequitely pack into the 'one fid' block */
   FidSize_NF_Corrected = fidSizeBytes / nf;

    /* note we divid by nf since packing happens in Recvproc not console */
    /*     also max mem is total so divided that amoung all active receivers */
    calc_nbufs = (int) ((memSize / numActiveDDRs)   / FidSize_NF_Corrected);

//    DPRINT2(3,"maxQueueSize: Memory: %llu, calc nbufs: %lu\n",memSize,calc_nbufs);
    return calc_nbufs;
}

static long long workQrequiredMemory(WORKQ_ID pWorkQ, int numActiveDDRs, int qSize, int fidSizeBytes, int nf)
{
    unsigned int FidSize_NF_Corrected;
    long long MemoryRequired;

   /* note we divide by nf since packing happens in Recvproc not console */
   /* i.e. each NF is recieved as a FID, which is subsequitely pack into the 'one fid' block */
   FidSize_NF_Corrected = fidSizeBytes / nf;

    /* note we divid by nf since packing happens in Recvproc not console */
    /*     also max mem is total so divided that amoung all active receivers */

    MemoryRequired = FidSize_NF_Corrected * qSize * numActiveDDRs;

    DPRINT1(3,"requiredMemory: %llu\n",MemoryRequired);
    return(MemoryRequired);
}
#endif

void workQDataBufsInit(WORKQ_ID pWorkQ, MFILE_ID_WRAPPER fiddata,  int fidSizeBytes, int nf, int ddrPos, int numActiveDDRs,PSTMF stmFunc)
{
    unsigned int sizebytes,calc_nbufs;
    WORKQ_ENTRY_ID wkrQAddrs;
    FID_STAT_BLOCK *fidStatAddr;
    int i;
    char *pDataBuf;
    unsigned int FidSize_NF_Corrected;


   pWorkQ->pInvar->fiddatafile =  fiddata;   /* mmap object wrapper */
   pWorkQ->pInvar->NumFids =  nf;	    /* NF */
   pWorkQ->pInvar->rcvrPos =  ddrPos;        /* Rcvrs Postion in FID file */
   pWorkQ->pInvar->numActiveRcvrs =  numActiveDDRs;  /* number of active receivers */
   pWorkQ->pInvar->pStmFunc = stmFunc;

   /* note we divide by nf since packing happens in Recvproc not console */
   /* i.e. each NF is recieved as a FID, which is subsequitely pack into the 'one fid' block */
    if (ddrPos == -1)
       FidSize_NF_Corrected = 1024; /* This DDR is not used so make the size something small */
    else
       FidSize_NF_Corrected = ((unsigned int) fidSizeBytes) / nf;

   DPRINT4(1,"workQDataBufsInit: ddrPos: %d, fidsize: %u, nf: %d. corrected fid size: %u\n",
       ddrPos, fidSizeBytes,nf, FidSize_NF_Corrected);

    /* note we divid by nf since packing happens in Recvproc not console */
    /*     also max mem is total so divided that amoung all active receivers */
    /* calc_nbufs = (pWorkQ->maxDataBufMem / numActiveDDRs)   / FidSize_NF_Corrected; */
    calc_nbufs = (unsigned int) ((pWorkQ->maxFidDataMemUsage / (long long) numActiveDDRs)   / FidSize_NF_Corrected);

    DPRINT1(2,"workQDataBufsInit: calc nbufs: %d\n",calc_nbufs);

    if (ddrPos == -1)
    {
       pWorkQ->numWorkQs = 5;  /* just incase for misc message that be sent by non-active DDRs */
    }
    else if ( calc_nbufs > pWorkQ->maxWorkQs)
    {
         pWorkQ->numWorkQs = pWorkQ->maxWorkQs;   /* can't be more than max */
    }
    else
    {
         pWorkQ->numWorkQs = calc_nbufs; 
    }

    sizebytes = pWorkQ->numWorkQs * FidSize_NF_Corrected;

//    DPRINT4(1,"workQDataBufsInit: buffers: calc: %d, max: %d, memusage: %lu, maxallowed: %llu\n",
//	pWorkQ->numWorkQs,pWorkQ->maxWorkQs,sizebytes, pWorkQ->maxFidDataMemUsage / (long long) numActiveDDRs);

   /* for interleave or RA/SA summing added a buffer if using regular file IO */
   /* free the IL summing buffers, they will malloc is need below  */
   if (pWorkQ->pFidSummingBuf != NULL)
   {
      DPRINT1(1,"workQDataBufsInit: FREE Regular File I/O IL summing buffer: Addr: %p\n", pWorkQ->pFidSummingBuf);
      free(pWorkQ->pFidSummingBuf);
      pWorkQ->pFidSummingBuf = NULL;
   }

   if ( ((fiddata->pMapFile == NULL) && (fiddata->pFile != NULL) ) && (stmFunc != NULL) )
   {
       if (ddrPos != -1)
       {
          pWorkQ->pFidSummingBuf = (char*) malloc(FidSize_NF_Corrected);
          DPRINT2(1,"workQDataBufsInit: malloc Regular File I/O IL summing buffer: Addr: %p, size: %u\n", 
                    pWorkQ->pFidSummingBuf, FidSize_NF_Corrected);
       }
   }

   /* if the is no data buffer yet, or if there is but the the new space 
    * requirement is larger then free this buffer and malloc a new one.
    */
   
   if (pWorkQ->pFidDataBufs == NULL)
   {
      /*  malloc memory for all the Fid Data Buffs */
      /* printf("malloc space\n"); */
      /* pWorkQ->pFidDataBufs = (char*) malloc( pWorkQ->maxDataBufMem );
       * pWorkQ->FidDataBufSize = fidSizeBytes;
       */
      pWorkQ->pFidDataBufs = (char*) malloc( sizebytes );
      pWorkQ->FidDataBufSize = sizebytes;
      DPRINT1(1,"workQDataBufsInit: malloc %u for FIDBufs\n",sizebytes);
   }
   else if ( (pWorkQ->pFidDataBufs != NULL) &&     /* size is greater or less by at least 5 MB then realloc */
           ( ( sizebytes > pWorkQ->FidDataBufSize) || ( (sizebytes + 5242880) < pWorkQ->FidDataBufSize) ) )
   {
      /* printf("free & re malloc space\n"); */
      /* tmp = pWorkQ->pFidDataBufs; */
      free(pWorkQ->pFidDataBufs);
       /*  malloc memory for all the Fid Data Buffs */
      pWorkQ->pFidDataBufs = (char*) malloc( sizebytes );
      /* pWorkQ->pFidDataBufs = (char*) realloc( pWorkQ->pFidDataBufs, sizebytes ); */
      /* DPRINT3(1,"workQDataBufsInit: free 0x%lx & malloc(0x%lx) %ld for FIDBufs\n",tmp,pWorkQ->pFidDataBufs,sizebytes); */
      pWorkQ->FidDataBufSize = sizebytes;
   }
  /* memset(pWorkQ->pFidStatBufs,0,sizebytes); */

 /* clear workQentry  free queue  */
  rngBlkFlush(pWorkQ->pBlkRng);

  /* fill in workQEntries with data buffer addresses */

  wkrQAddrs = pWorkQ->pWrkQBuffs;
  pDataBuf = pWorkQ->pFidDataBufs;
  fidStatAddr = pWorkQ->pFidStatBufs;
  for(i=0;i<pWorkQ->numWorkQs;i++)
  {
       wkrQAddrs->pWorkQObj = pWorkQ;     /* reference back to the workQObj */
       wkrQAddrs->pInvar = pWorkQ->pInvar;  /* reference back to the workQ Invarients */
       wkrQAddrs->pFidStatBlk = fidStatAddr;
       wkrQAddrs->pFidData = pDataBuf;
       rngBlkPut(pWorkQ->pBlkRng, (long *) &wkrQAddrs,1);
       /* printf("workQDataBufsInit: wrkQaddr: 0x%lx, statBlkAddr: 0x%lx\n",wkrQAddrs,pDataBuf); */
       wkrQAddrs++;
       fidStatAddr++;
       pDataBuf += FidSize_NF_Corrected;
  }
  
}

int workQFreeDataBufs(WORKQ_ID pWorkQ)
{
    /* printf("workQFreeDataBufs: free: 0x%lx\n",pWorkQ->pFidDataBufs); */
    if (pWorkQ->pFidDataBufs != NULL)
    {
       free( pWorkQ->pFidDataBufs );
       pWorkQ->pFidDataBufs = NULL;
    }
    return(0);
}

int workQReset(WORKQ_ID pWorkQ)
{
  int i;
  WORKQ_ENTRY_ID wkrQAddrs;

  wkrQAddrs = pWorkQ->pWrkQBuffs;
  /* clear workQentry  free queue  */
  rngBlkFlush(pWorkQ->pBlkRng);

  /* refill ring buffer with free list of workQentry buffer addresses */
  for(i=0; i<pWorkQ->numWorkQs; i++)
  {
       /* printf("wrkQaddr: 0x%lx, statBlkAddr: 0x%lx\n",wkrQAddrs,fidStatAddr); */
       rngBlkPut(pWorkQ->pBlkRng, (long *) &wkrQAddrs,1);
       wkrQAddrs++;
  }
  return(0);
}

int workQGetWillPend(WORKQ_ID pWorkQ)
{
   
   return ( rngBlkIsEmpty (pWorkQ->pBlkRng) );
}

int numAvailWorkQs(WORKQ_ID pWorkQ)
{
   return( rngBlkNElem(pWorkQ->pBlkRng) );
}

WORKQ_ENTRY_ID workQGet(WORKQ_ID pWorkQ)
{
   int stat __attribute__((unused));
   long tmpptr;
   stat = rngBlkGet(pWorkQ->pBlkRng, &tmpptr,1);
   /* printf("bufaddr: 0x%lx\n",bufAddr); */
   return( (WORKQ_ENTRY_ID)tmpptr );
}


#ifdef XXX
*int workQFidIndexFree(WORKQ_ENTRY_ID pWorkQEntry, long index)
*{
*   /* printf("put addr: 0x%lx\n",*bufAddr); */
*   /* return( rngBlkPut(pWorkQEntry->pFidBufIndices,(long*) &index,1) ); */
*   return( rngBlkPut(pWorkQEntry->pInvar->pFidBufIndices,(long*) &index,1) );
*}
*
*
*/*
*int workQSetInData(WORKQ_ID pWorkQ, MFILE_ID_WRAPPER indata, RINGBLK_ID pIndices, PSTMF stmFunc)
*{
*  pWorkQ->pInvar->indatafile = indata;
*  pWorkQ->pInvar->pFidBufIndices = pIndices;
*  pWorkQ->pInvar->pStmFunc = stmFunc;
*  return(0);
*}
**/
*
*int workQClrInData(WORKQ_ID pWorkQ)
*{
*  if (pWorkQ->pInvar->indatafile != NULL)
*     free(pWorkQ->pInvar->indatafile);
*  pWorkQ->pInvar->indatafile = NULL;
*  return 0;
*}
*
*
*int workQSetFidData(WORKQ_ID pWorkQ, MFILE_ID_WRAPPER fiddata, long nf, int ddrPos, int numActiveDDRs)
*{
*
*  pWorkQ->pInvar->fiddatafile =  fiddata;   /* mmap object wrapper */
*  pWorkQ->pInvar->NumFids =  nf;	    /* NF */
*  pWorkQ->pInvar->rcvrPos =  ddrPos;        /* Rcvrs Postion in FID file */
*  pWorkQ->pInvar->numActiveRcvrs =  numActiveDDRs;  /* number of active receivers */
*  return(0);
*}
*
*int workQClrFidData(WORKQ_ID pWorkQ)
*{
*
*  /* if (pWorkQ->pInvar->fiddatafile != NULL) */
*     /* free(pWorkQ->pInvar->fiddatafile); */
*  // pWorkQ->pInvar->fiddatafile =  NULL;   /* mmap object wrapper */
*  pWorkQ->pInvar->NumFids =  1;	    /* NF */
*  pWorkQ->pInvar->numActiveRcvrs =  1;  /* number of active receivers */
*  return(0);
*}
*
#endif

int isInDataActive(WORKQ_ID pWorkQ)
{
   int retval = 1;
   if (pWorkQ->pInvar->indatafile == NULL)
      retval = 0;
   if (pWorkQ->pInvar->indatafile->pMapFile == NULL)
      retval = 0;

   return (retval);
}
int isFidDataActive(WORKQ_ID pWorkQ)
{
   int retval = 1;
   if (pWorkQ->pInvar->fiddatafile == NULL)
      retval = 0;
   if ( (pWorkQ->pInvar->fiddatafile->pMapFile == NULL) && (pWorkQ->pInvar->fiddatafile->pFile == NULL) )
      retval = 0;

   return (retval);
}

/*
 * Obtains the point into either the buffer where host summing of the FID is required ( IL or RA )
 * otherwise pointer directly into the mmap FID file itself. 
 * Position within the FID file is effect by NF and the reciever (DDR) relative position 'ddrPos'
 * as specified within the experiment settings
 */
#ifdef XXXX
*char *getWorkQNewFidBufferPtr(WORKQ_ID pWorkQ,WORKQ_ENTRY_ID pWorkQEntry)
*{
*     char *dataPtr;
*     int status;
*     unsigned long fidNum, bufNum, fidSizeBytes, traceNum;
*
*     /* if not buffering (il,ra) then we are using the FID datafile directly */
*     /* if (pWorkQEntry->indatafile == NULL) */
*     /* if (pWorkQEntry->pInvar->indatafile->pMapFile == NULL) */
*     if (isInDataActive(pWorkQ) == 0)
*     {
*        if ( isFidDataActive(pWorkQ) == 1)  /* besure FID file is open & active */
*        {
*        fidNum = pWorkQEntry->pFidStatBlk->elemId;  /* FID number */
*
*        /* corrections NF packing */
*        /* for nf = 4, then elemId 1-4 packed as traces into elemId 1 */
*        /*                  elemId 5-8 packed as traces into elemId 2 */
*        /*                  elemId 9-12 packed as traces into elemId 3 */
*        /* for modulo to work out nicely it's best to start at 'zero' hence the '-1' because */
*        /* elemId starts at 1 */
*        fidNum = ((pWorkQEntry->pFidStatBlk->elemId - 1) / pWorkQEntry->pInvar->NumFids) + 1;
*        traceNum = (pWorkQEntry->pFidStatBlk->elemId - 1) % pWorkQEntry->pInvar->NumFids;
*
*        /* the Inova equivilent elemID, procs expect this elemId in processing and tests, etc.  */
*        /* pWorkQEntry->rcvrPos + 1 is because rcvrPos starts at zero */
*        /* pWorkQEntry->trueElemId = ((fidNum-1)  * pWorkQEntry->numActiveRcvrs) + pWorkQEntry->rcvrPos + 1; */
*        pWorkQEntry->trueElemId = ((fidNum-1)  * pWorkQEntry->pInvar->numActiveRcvrs) + pWorkQEntry->pInvar->rcvrPos + 1;
*
*        /* DPRINT4(-1,"getWorkQNewFidBufferPtr: elemId: %lu, %lu / nf (%lu) = fidNum (%lu)\n",
*         *    pWorkQEntry->pFidStatBlk->elemId, (pWorkQEntry->pFidStatBlk->elemId - 1), pWorkQEntry->NumFids, fidNum);
*         * DPRINT3(-1,"getWorkQNewFidBufferPtr: traceNum: %lu =  nf (%lu) %% elemId(%lu)-1)\n",
*         *    traceNum, pWorkQEntry->NumFids, pWorkQEntry->pFidStatBlk->elemId); */
*        DPRINT4(+2,"getWorkQNewFidBufferPtr: elemId: %lu, nf: %lu, ---> fidNum %lu, Trace: %lu\n",
*              pWorkQEntry->pFidStatBlk->elemId, pWorkQEntry->pInvar->NumFids, fidNum,traceNum);
*        DPRINT4(+2,"getWorkQNewFidBufferPtr: elemId: %lu, rcvrPos: %lu, numActiveRcvrs: %lu, ---> True ElemId %lu \n",
*              pWorkQEntry->pFidStatBlk->elemId,pWorkQEntry->pInvar->rcvrPos, pWorkQEntry->pInvar->numActiveRcvrs,pWorkQEntry->trueElemId);
*
*
*        /* (FID size * nf ) + blockheader in bytes */
*        fidSizeBytes = (pWorkQEntry->pFidStatBlk->dataSize  * pWorkQEntry->pInvar->NumFids) + sizeof(struct datablockhead);
*        /* printf("elemId: %d, bbtyes: %lu\n", fidNum, fidSizeBytes); */
*
*        mMutexLock(pWorkQEntry->pInvar->fiddatafile->pMapFile);
*        /* status =  mFidSeek(pWorkQEntry->fiddatafile, fidNum, sizeof(struct datafilehead), fidSizeBytes); */
*        status =  mFidSeek(pWorkQEntry->pInvar->fiddatafile->pMapFile, pWorkQEntry->trueElemId, sizeof(struct datafilehead), fidSizeBytes);
*        dataPtr = pWorkQEntry->pInvar->fiddatafile->pMapFile->offsetAddr + sizeof(struct datablockhead);
*        dataPtr += (pWorkQEntry->pFidStatBlk->dataSize * traceNum);
*        mMutexUnlock(pWorkQEntry->pInvar->fiddatafile->pMapFile);
*
*        }
*        else
*        {
*	   DPRINT(1,"getWorkQNewFidBufferPtr: fid file not open Return NULL\n");
*           dataPtr = NULL;
*        }
*     }
*     else  /* buffering in a temp file to allow summing within recvproc */
*     {
*
*        /* buffer temp files is just data NO headers */
*        fidSizeBytes = pWorkQEntry->pFidStatBlk->dataSize;  
*
*        /* get a free buffer index */
*        status = rngBlkGet(pWorkQEntry->pInvar->pFidBufIndices, (long*) &bufNum,1);
*        pWorkQEntry->bufferIndex = bufNum;
*
*        /* Now seek to proper position within the buffer mmap file */
*        mMutexLock(pWorkQEntry->pInvar->indatafile->pMapFile);
*          status =  mFidSeek(pWorkQEntry->pInvar->indatafile->pMapFile, bufNum, 0, fidSizeBytes);
*          dataPtr = pWorkQEntry->pInvar->indatafile->pMapFile->offsetAddr;
*        mMutexUnlock(pWorkQEntry->pInvar->indatafile->pMapFile);
*
*     }
*     return(dataPtr);
*}
*
#endif 
#ifdef XXXX
*char *getWorkQFidBufferPtr(WORKQ_ID pWorkQ,WORKQ_ENTRY_ID pWorkQEntry)
*{
*     char *dataPtr;
*     int status;
*     unsigned long fidNum, bufNum, fidSizeBytes;
*
*     /* buffer temp files is just data NO headers */
*     fidSizeBytes = pWorkQEntry->pFidStatBlk->dataSize;  
*
*     bufNum = pWorkQEntry->bufferIndex;
*
*     /* Now seek to proper position within the buffer mmap file */
*     mMutexLock(pWorkQEntry->pInvar->indatafile->pMapFile);
*        status =  mFidSeek(pWorkQEntry->pInvar->indatafile->pMapFile, bufNum, 0, fidSizeBytes);
*        dataPtr = pWorkQEntry->pInvar->indatafile->pMapFile->offsetAddr;
*     mMutexUnlock(pWorkQEntry->pInvar->indatafile->pMapFile);
*
*     return(dataPtr);
*}
#endif

char *getWorkQFidBlkHdrPtr(WORKQ_ID pWorkQ,WORKQ_ENTRY_ID pWorkQEntry)
{
     char *fidblkhdrSpot;
     int status __attribute__((unused));
     unsigned int fidNum, traceNum, fidSizeBytes;

#ifdef XXX
     fidNum = pWorkQEntry->pFidStatBlk->elemId;  /* FID number */
     fidSizeBytes = pWorkQEntry->pFidStatBlk->dataSize + sizeof(struct datablockhead);  
#endif
     /* corrections NF packing */
     /* for nf = 4, then elemId 1-4 packed as traces into elemId 1 */
     /*                  elemId 5-8 packed as traces into elemId 2 */
     /*                  elemId 9-12 packed as traces into elemId 3 */
     /* for modulo to work out nicely it's best to start at 'zero' hence the '-1' becuase */
     /* elemId starts at 1 */
     fidNum = ((pWorkQEntry->pFidStatBlk->elemId - 1) / pWorkQEntry->pInvar->NumFids) + 1;
     traceNum = (pWorkQEntry->pFidStatBlk->elemId - 1) % pWorkQEntry->pInvar->NumFids;

     /* the Inova equivilent elemID, procs expect this elemId in processing and tests, etc.  */
     /* pWorkQEntry->rcvrPos + 1 is because rcvrPos starts at zero */
     pWorkQEntry->trueElemId = ((fidNum-1)  * pWorkQEntry->pInvar->numActiveRcvrs) + pWorkQEntry->pInvar->rcvrPos + 1;

     DPRINT4(2,"getWorkQFidBlkHdrPtr: elemId: %u, nf: %u, ---> fidNum %u, Trace: %u\n",
              pWorkQEntry->pFidStatBlk->elemId, pWorkQEntry->pInvar->NumFids, fidNum,traceNum);
     DPRINT4(2,"getWorkQFidBlkHdrPtr: elemId: %u, rcvrPos: %u, numActiveRcvrs: %u, ---> True ElemId %u \n",
              pWorkQEntry->pFidStatBlk->elemId,pWorkQEntry->pInvar->rcvrPos, pWorkQEntry->pInvar->numActiveRcvrs,pWorkQEntry->trueElemId);

     /* (FID size * nf ) + blockheader in bytes */
     fidSizeBytes = (pWorkQEntry->pFidStatBlk->dataSize  * pWorkQEntry->pInvar->NumFids) + sizeof(struct datablockhead);
     /* DPRINT2(-2,"getWorkQFidBlkHdrPtr: fidSizeBytes: %ld, sizeof filehead: %d\n",fidSizeBytes,sizeof(struct datafilehead)); */

     if (pWorkQEntry->pInvar->fiddatafile->pMapFile != NULL)
     {
        mMutexLock(pWorkQEntry->pInvar->fiddatafile->pMapFile);
        /* status =  mFidSeek(pWorkQEntry->fiddatafile, fidNum, sizeof(struct datafilehead), fidSizeBytes); */
        status =  mFidSeek(pWorkQEntry->pInvar->fiddatafile->pMapFile, pWorkQEntry->trueElemId, sizeof(struct datafilehead), fidSizeBytes);
        fidblkhdrSpot = pWorkQEntry->pInvar->fiddatafile->pMapFile->offsetAddr;
        mMutexUnlock(pWorkQEntry->pInvar->fiddatafile->pMapFile);
     }
     else
     {
          long long offset;
          offset =  fFidSeek(pWorkQEntry->pInvar->fiddatafile->pFile, pWorkQEntry->trueElemId, sizeof(struct datafilehead), fidSizeBytes);
          pWorkQEntry->RW_FileOffset = offset;
          fidblkhdrSpot = (char*) offset;
          /* DPRINT2(-12,"getWorkQFidBlkHdrPtr: fidblkhdrSpot: %lu, offset: %llu\n",fidblkhdrSpot,offset); */
     }

    return(fidblkhdrSpot);

}

char *getWorkQFidPtr(WORKQ_ID pWorkQ,WORKQ_ENTRY_ID pWorkQEntry)
{
     char *dataPtr;
     int status __attribute__((unused));
     unsigned int fidNum, fidSizeBytes,traceNum;

#ifdef XXX
     fidNum = pWorkQEntry->pFidStatBlk->elemId;  /* FID number */
     /* FID size + blockheader in bytes */
     fidSizeBytes = pWorkQEntry->pFidStatBlk->dataSize + sizeof(struct datablockhead);  
     /* printf("elemId: %d, bbtyes: %lu\n", fidNum, fidSizeBytes); */
#endif

     /* corrections NF packing */
     /* for nf = 4, then elemId 1-4 packed as traces into elemId 1 */
     /*                  elemId 5-8 packed as traces into elemId 2 */
     /*                  elemId 9-12 packed as traces into elemId 3 */
     /* for modulo to work out nicely it's best to start at 'zero' hence the '-1' becuase */
     /* elemId starts at 1 */
     fidNum = ((pWorkQEntry->pFidStatBlk->elemId - 1) / pWorkQEntry->pInvar->NumFids) + 1;
     traceNum = (pWorkQEntry->pFidStatBlk->elemId - 1) % pWorkQEntry->pInvar->NumFids;
     pWorkQEntry->cf = traceNum;

     /* the Inova equivilent elemID, procs expect this elemId in processing and tests, etc.  */
     /* pWorkQEntry->rcvrPos + 1 is because rcvrPos starts at zero */
     pWorkQEntry->trueElemId = ((fidNum-1)  * pWorkQEntry->pInvar->numActiveRcvrs) + pWorkQEntry->pInvar->rcvrPos + 1;


     /*
     DPRINT4(-1,"getWorkQFidPtr: elemId: %lu, %lu / nf (%lu) = fidNum (%lu)\n",
          pWorkQEntry->pFidStatBlk->elemId, (pWorkQEntry->pFidStatBlk->elemId - 1), pWorkQEntry->NumFids, fidNum);
     DPRINT3(-1,"getWorkQFidPtr: traceNum: %lu =  nf (%lu) %% elemId(%lu)-1)\n",
          traceNum, pWorkQEntry->NumFids, pWorkQEntry->pFidStatBlk->elemId);
     */

     DPRINT4(2,"getWorkQFidPtr: elemId: %u, nf: %u, ---> fidNum %u, Trace: %u\n",
              pWorkQEntry->pFidStatBlk->elemId, pWorkQEntry->pInvar->NumFids, fidNum,traceNum);
     DPRINT4(2,"getWorkQFidPtr: elemId: %u, rcvrPos: %u, numActiveRcvrs: %u, ---> True ElemId %u \n",
              pWorkQEntry->pFidStatBlk->elemId,pWorkQEntry->pInvar->rcvrPos, pWorkQEntry->pInvar->numActiveRcvrs,pWorkQEntry->trueElemId);

     /* (FID size * nf ) + blockheader in bytes */
     fidSizeBytes = (pWorkQEntry->pFidStatBlk->dataSize  * pWorkQEntry->pInvar->NumFids) + sizeof(struct datablockhead);


     if (pWorkQEntry->pInvar->fiddatafile->pMapFile != NULL)
     {
        mMutexLock(pWorkQEntry->pInvar->fiddatafile->pMapFile);
          /* status =  mFidSeek(pWorkQEntry->fiddatafile, fidNum, sizeof(struct datafilehead), fidSizeBytes); */
          status =  mFidSeek(pWorkQEntry->pInvar->fiddatafile->pMapFile, pWorkQEntry->trueElemId, sizeof(struct datafilehead), fidSizeBytes);
          dataPtr = pWorkQEntry->pInvar->fiddatafile->pMapFile->offsetAddr + sizeof(struct datablockhead);
          dataPtr += (pWorkQEntry->pFidStatBlk->dataSize * traceNum);
        mMutexUnlock(pWorkQEntry->pInvar->fiddatafile->pMapFile);
     /* printf("mmap file ptr: 0x%lx\n",dataPtr); */
     }
     else
     {
          long long offset;
          offset =  fFidSeek(pWorkQEntry->pInvar->fiddatafile->pFile, pWorkQEntry->trueElemId, sizeof(struct datafilehead), fidSizeBytes);
          dataPtr = (char*) (offset + sizeof(struct datablockhead));
          dataPtr += (pWorkQEntry->pFidStatBlk->dataSize * traceNum);
          pWorkQEntry->RW_FileOffset = offset + sizeof(struct datablockhead) + (pWorkQEntry->pFidStatBlk->dataSize * traceNum);
	  /* DPRINT3(-12,"getWorkQFidPtr: dataPtr: 0x%lx, offset: 0x%llx, %llu\n", 
                        dataPtr,pWorkQEntry->RW_FileOffset,pWorkQEntry->RW_FileOffset); */
     }

     return(dataPtr);
}

/*
 *  Use these function when regular File I/O is being used to access the FId Data
 *
 */

/*
 *  Writes out the Fid block header base on elemId,  etc...
 *  via regular file I/O
 *
 *    Author:  Greg Brissey  2/01/2007
 */
int writeWorkQFidHeader2File(WORKQ_ID pWorkQ,WORKQ_ENTRY_ID pWorkQEntry, void *pHeader, int bytelen)
{
    int bytes;
    /* get proper offset into file */
   DPRINT2(2,"\nwriteWorkQFidHeader2File: HeaderAddr: %p, len: %d\n", pHeader, bytelen);
   getWorkQFidBlkHdrPtr(pWorkQEntry->pWorkQObj,pWorkQEntry);
   DPRINT4(2,"writeWorkQFidHeader2File: fd: %d, data: %p, len: %d, offset: %llu\n",
	pWorkQEntry->pInvar->fiddatafile->pFile->fd, pHeader,  bytelen, pWorkQEntry->RW_FileOffset);
   bytes = fFileWrite(pWorkQEntry->pInvar->fiddatafile->pFile, pHeader,  bytelen, pWorkQEntry->RW_FileOffset);
   /* DPRINT(2,"\n\n"); */
   return( bytes );   
}

/*
 *  Reads the Fid block header base on elemId,  etc...
 *  via regular file I/O into buffer address
 *
 *    Author:  Greg Brissey  2/01/2007
 */
int readWorkQFidHeaderFromFile(WORKQ_ID pWorkQ,WORKQ_ENTRY_ID pWorkQEntry, void *pHeader, int bytelen)
{
    int bytes;
    /* get proper offset into file */
   DPRINT2(2,"\nreadWorkQFidHeaderFromFile: HeaderAddr: %p, len: %d\n", pHeader, bytelen);
   getWorkQFidBlkHdrPtr(pWorkQEntry->pWorkQObj,pWorkQEntry);
   DPRINT4(2,"readWorkQFidHeaderFromFile: fd: %d, data: %p, len: %d, offset: %llu\n",
	pWorkQEntry->pInvar->fiddatafile->pFile->fd, pHeader,  bytelen, pWorkQEntry->RW_FileOffset);
   bytes = fFileRead(pWorkQEntry->pInvar->fiddatafile->pFile, pHeader,  bytelen, pWorkQEntry->RW_FileOffset);
   /* DPRINT(2,"\n\n"); */
   return( bytes );   
}


/*
 *  Writes out the Fid Data base on elemId,  etc...
 *  via regular file I/O into the fid data file
 *
 *    Author:  Greg Brissey  2/01/2007
 */
int writeWorkQFid2File(WORKQ_ID pWorkQ,WORKQ_ENTRY_ID pWorkQEntry, int bytelen)
{
    int bytes;
    /* get proper offset into file */
   DPRINT2(2,"\n\nwriteWorkQFid2File: FidAddr: %p, len: %d\n", pWorkQEntry->pFidData, bytelen);
   getWorkQFidPtr(pWorkQ,pWorkQEntry);
//   DPRINT4(2,"writeWorkQFid2File: fd: %d, data: 0x%lx, len: %d, offset: %llu\n",
//        pWorkQEntry->pInvar->fiddatafile->pFile, pWorkQEntry->pFidData,  bytelen, pWorkQEntry->RW_FileOffset);
   bytes = fFileWrite(pWorkQEntry->pInvar->fiddatafile->pFile, pWorkQEntry->pFidData,  bytelen, pWorkQEntry->RW_FileOffset);
   /* DPRINT(-12,"\n\n"); */
   return( bytes );   
}

/*
 *  reads the Fid Data base on elemId,  etc...
 *  via regular file I/O into the buffer given 
 *
 *    Author:  Greg Brissey  2/01/2007
 */
int readWorkQFidFromFile(WORKQ_ID pWorkQ,WORKQ_ENTRY_ID pWorkQEntry, char* pFidBuffer, int bytelen)
{
    int bytes;
    /* get proper offset into file */
   DPRINT2(2,"\nreadWorkQFidFromFile: FidAddr: %p, len: %d\n", pFidBuffer, bytelen);
   getWorkQFidPtr(pWorkQ,pWorkQEntry);
//   DPRINT4(2,"readWorkQFidFromFile: fd: %d, data: 0x%lx, len: %d, offset: %llu\n",
//        pWorkQEntry->pInvar->fiddatafile->pFile, pWorkQEntry->pFidData,  bytelen, pWorkQEntry->RW_FileOffset);
   bytes = fFileRead(pWorkQEntry->pInvar->fiddatafile->pFile, pFidBuffer,  bytelen, pWorkQEntry->RW_FileOffset);
   /* DPRINT(-12,"\n\n"); */
   return( bytes );   
}


/*
 *  Summs the Fid Data base on elemId,  etc...
 *  from the NDDS workQ buffer with that on disk and writes the result
 *   back out to disk 
 *
 *    Author:  Greg Brissey  2/01/2007
 */
int sumWorkQFidData(WORKQ_ID pWorkQ,WORKQ_ENTRY_ID pWorkQEntry,int bytelen,int np)
{
   int bytes,overflow;

   /* 1st read fid from disk into a buffer so that it can be summed */
   bytes = readWorkQFidFromFile(pWorkQ,pWorkQEntry, pWorkQ->pFidSummingBuf, bytelen);
   DPRINT2(2,"sumWorkQFidData: read %d bytes, expected: %d bytes\n",bytes,bytelen);
   /* summ the Fid data from the two buffers */
   overflow = (pWorkQEntry->pInvar->pStmFunc)((void*) pWorkQEntry->pFidData, (void*) pWorkQ->pFidSummingBuf, np );
   /* write the summed result backup to file */
   bytes = writeWorkQFid2File(pWorkQ,pWorkQEntry, bytelen);
   DPRINT2(2,"sumWorkQFidData: Summed: %d bytes written, overflow: %d\n",bytes,overflow);
   return( overflow );
}

#ifdef XXX
void workQShow(WORKQ_ID pWorkQ)
{
   int i;
   WORKQ_ENTRY_ID wkrQAddrs;

    printf("Number in workQ: %d\n",pWorkQ->numWorkQs);
    printf("rngBuf Addr: 0x%lx, pWrkQBuffs: 0x%lx, pFidStatBufs: 0x%lx\n",
	pWorkQ->pBlkRng, pWorkQ->pWrkQBuffs,pWorkQ->pFidStatBufs);

    wkrQAddrs = pWorkQ->pWrkQBuffs;
    for(i=0; i < pWorkQ->numWorkQs; i++)
    {
        printf("pFidStatBlk: 0x%lx, indatafile: 0x%lx, fiddatafile: 0x%lx, backref WorkQObj: 0x%lx\n",
		wkrQAddrs->pFidStatBlk,wkrQAddrs->pInvar->indatafile,wkrQAddrs->pInvar->fiddatafile,
		wkrQAddrs->pWorkQObj);
        wkrQAddrs++;
    }

    return;
}
#endif

/* ------------------------------------------------------------------------------------------ */
#ifdef  WORK_IN_PROCESS
   /* discover if free all DDR data bufs prior to realloc then memory usage stays the same
      so no need for this, so it's abandon, and my be deleted later on....
        GMB 11/15/05
   */
/*
 *  Single memory pool used by all DDR threads for NDDS work buffers
 *  system used to free & malloc for each thread how every the memory usage would increase
 *  since free does NOT release memory to the system just the application
 *
 *   Greg Brissey   11/15/05
 */
*
*static char *pMemoryPool = NULL;
*static long MaxSizeOfPool = 0;
*static long memoryPoolSize = 0;
*typedef struct _mempoolallocs {
*        char *pAllocAddr;
*        long  sizeofbuf;
*   } WORKQ_MEMPOOL, *WORKQMEMPOOL_ID;
*
*static WORKQ_MEMPOOL bufsAllocd[64];
*static int numAllocs = 0;
*
*/*************************************************************/
*
*char *memPoolInit( long maxDataBufMem ); 
*{
*   if ( (pMemoryPool == NULL) && ( memoryPoolSize == 0) )
*   {
*      MaxSizeOfPool = maxDataBufMem;
*      pMemoryPool = (char*) malloc(MaxSizeOfPool);
*   }
*   return pMemoryPool;
*}
*
*memPoolGetBufPtr(long sizeinbytes)
*{
*}
*
#endif
/* #define TEST_MAIN */
#ifdef TEST_MAIN

RINGBLK_ID pipeQ;

void *reader_routine (void *arg)
{
   WORKQ_ID pWorkQ = (WORKQ_ID) arg;
   int cnt, status;
   WORKQ_ENTRY_ID pWrkQentry;
   int state;

   cnt = 0;
   while(1)
   {
       printf("get buffer: %d, ",++cnt);
       pWrkQentry = workQGet(pWorkQ);
       printf("addr: 0x%lx \n",pWrkQentry);
       rngBlkPut(pipeQ,(long*) &pWrkQentry,1);
   }
}

main()
{
   WORKQ_ID pWorkQ;
   WORKQ_ENTRY_ID pWrkQentry;
   pthread_t  RdthreadId;
   int status,i;
   /* create buffer object */
   pWorkQ = workQCreate(10);
   workQShow(pWorkQ);

   rngBlkShow(pWorkQ->pBlkRng,1);
   pipeQ = rngBlkCreate(10,"pipeQ", 1);

   status = pthread_create (&RdthreadId,
               NULL, reader_routine, (void*) pWorkQ);

   if (status != 0)
   {
        printf("pthread_create error\n");
         exit(1);
   }
   
   while(1)
   {
     sleep(2);
     
     rngBlkGet(pipeQ, &pWrkQentry,1);
     printf("put in buf: 0x%lx\n",pWrkQentry);
     workQFree(pWorkQ, pWrkQentry);
   }
}
#endif
