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
#include <memLib.h>
#include "instrWvDefines.h"
#include "commondefs.h"
#include "logMsgLib.h"
#include "rngLLib.h"
#include "mBufferLib.h"


/*
modification history
--------------------
3-02-04,gmb  created 
*/

/*
DESCRIPTION
 
mBufferLib is a fast fixed-size buffer simple memory cluster manager. It provides
a pool of different size  buffers (clusters) that can be used for fast allocation/deallocation
during run-time. This library avoids the time consuming malloc and free
calls. 

*/
/*
    fill in the cluse/buffer free list ring buffers
    Preceeding the addres is the index into with cluster goup this address belongs.
    99 indecates the memory was malloc'd directly and should be freed.

*	Author Greg Brissey 3/2/04

*/
fillRngwAddrs(MBUFFER_ID pMBufferId)
{
   register int toP;
   int i,index;
   long *indxAddr;
   char *tmpAddr;
   char *clAddr;
   MBUF_DESC *clusters;

  clusters = pMBufferId->mbclusters;
   
  for ( i=0; i < pMBufferId->numLists; i++)
  {
    tmpAddr = clusters[i].memArea;
    for ( index=0; index < clusters[i].clNum; index++)
    {
       indxAddr = (long *) tmpAddr;
       *indxAddr = 0xc0de0000 |  (0xffff & i);  /* in front of buffer place size index */
        clAddr = tmpAddr + sizeof(long);
        /* DPRINT3(-1," rngput %d, index: %ld, addr: 0x%lx\n",index, *tmpAddr, clAddr); */
        RNG_LONG_PUT(pMBufferId->freeList[i], (long) clAddr, toP);
        tmpAddr += clusters[i].clSize;
    }
  }

}
/**************************************************************
*
*  mBufferCreate - Creates a Pool of Fixed Size Memory Buffer Sets
*                  similar to network clusters
*
*  This routine creates a set of buffers and returns
* a handle to a MBUFFER_ID. This ID can be used to
* allocate and free individual buffers from the pool.
* The memory for the buffer pool is obtained via
* "malloc" from the system memory.
*
* RETURNS:
* ID of the buffer pool, or NULL if memory cannot be allocated.
*
*	Author Greg Brissey 3/2/04
*/

MBUFFER_ID mBufferCreate(MBUF_DESC *clusters, int numClusterSize, int eventid, int memReserveMB)
/* MBUF_DESC *clusters - tables of clusters sizes to allocate for the memory pool */
/* numClusterSize - number of cluster sets. */
/* eventid WindViwe Event ID */
{
  MBUFFER_ID pMBufferId;
  int toP;
  long sizeNBytes;
  int i,j,index;

  DPRINT1(1,"mbCLTbl: bufSzie: %d\n",clusters[1].clSize);
  pMBufferId = (MBUFFER_ID) malloc(sizeof(MBUFFER_OBJ));  /* create structure */
  if (pMBufferId == NULL) 
     return (NULL);
 
  /* zero out structure */
  memset(pMBufferId,0,sizeof(MBUFFER_OBJ));

  pMBufferId->mbclusters = clusters;
  pMBufferId->numLists = numClusterSize;
  pMBufferId->memReserveLevel = memReserveMB * 1048576;


  /* malloc space for the different size buffers */
  for ( i=0; i < numClusterSize; i++)
  {
    clusters[i].memSize = clusters[i].clNum * ( clusters[i].clSize + sizeof(long));
    /* clusters[i].memArea = (char *) malloc( clusters[i].memSize ); */
    /* clusters[i].memArea = (char *) memalign(4, clusters[i].memSize );  /* align to 4 bytes */
    clusters[i].memArea = (char *) memalign(_CACHE_ALIGN_SIZE, clusters[i].memSize );  /* align to cache line 32 bytes for 405 */
    DPRINT5(1,"cluster[%d]: size: %d, num: %d, memSize: %d, memArea: 0x%lx\n",
	     i,clusters[i].clSize,clusters[i].clNum,clusters[i].memSize,clusters[i].memArea);
    if (clusters[i].memArea == NULL)
    {
	free(pMBufferId);
        for (j=i-1; j >= 0; j--)
            free(clusters[i].memArea); 
        return(NULL);
    }
    pMBufferId->listSizes[i] = clusters[i].clSize;
    pMBufferId->freeList[i] = rngLCreate(clusters[i].clNum,"ClusterFreeList");
  }
  fillRngwAddrs(pMBufferId);

  return(pMBufferId);
}

/**************************************************************
*
* mBufferDelete - Delete Fast Buffer Pool
*
*
*  Delete Fast Buffer Pool
*  This routine deletes the Fast Buffer Pool releasing memory
*
* RETURNS:
* VOID
*
*               Author Greg Brissey 3/2/04
*/
void mBufferDelete(MBUFFER_ID mBufferId)
/* MBUFFER_ID mBufferId - Fast Buffer Id */
{
   int i;
   if (mBufferId != NULL)
   {
       for ( i=0; i < mBufferId->numLists; i++)
       {
          if (mBufferId->mbclusters[i].memArea != NULL)
              free(mBufferId->mbclusters[i].memArea);
          rngLDelete(mBufferId->freeList[i]);
       }
       free(mBufferId);
   }
   return;
}

/*----------------------------------------------------------------------*/
/* mBufferShwResrc							*/
/*     Show system resources used by Object (e.g. semaphores,etc.)	*/
/*	Useful to print then related back to WindView Events		*/
/*----------------------------------------------------------------------*/
VOID mBufferShwResrc(register MBUFFER_ID mBufferId,int indent)
{
   int i;
   char spaces[40];

   for (i=0;i<indent;i++) spaces[i] = ' ';
   spaces[i]='\0';

   printf("%s mBuffer Obj: 0x%lx\n",spaces,mBufferId);
   printf("\n%s   Ring Buffer:\n",spaces);
   for (i=0; i <  mBufferId->numLists; i++)
   {
     rngLShow(mBufferId->freeList[i],1);
   }
}

/**************************************************************
*
* mBufferGet - get the next available buffer 
*
*
*  This routine returns the next available buffer address.
*
*
* RETURNS:
*  The Address of a available buffer.
*
*       Author Greg Brissey 3/02/04
*/
char *mBufferGet(register MBUFFER_ID mBufferId, unsigned int size)
/* MBUFFER_ID mBufferId - Fast Buffer to allocate from */
{
  int i,stat;
  long *indxAddr;
  char *bufAddr;
  register int toP;
  MEM_PART_STATS  memStats;

#ifdef INSTRUMENT
     wvEvent(EVENT_FBUF_GET,NULL,NULL);
#endif

  taskLock();  /* critical code */

  bufAddr = 0;
  for ( i=0; i < mBufferId->numLists; i++)
  {
      DPRINT2(1,"mBufferGet: size %d, cluster size %d\n",size,mBufferId->listSizes[i]);
      if ( (size+16) < mBufferId->listSizes[i] )
      {
	stat = RNG_LONG_GET(mBufferId->freeList[i], (long*) &bufAddr,toP);
        DPRINT3(1," mBufferGet: stat %d, Cluster: %d, Addr: 0x%lx\n",stat, mBufferId->listSizes[i],bufAddr);
	if ( stat != 0)
        {
           DPRINT(1," mBufferGet(): Obtain cluster entry\n");
           mBufferId->clCnt[i] += 1;
           taskUnlock();
           return(bufAddr);
        }
        else
        {
           mBufferId->clFailCnt[i] += 1;
           continue;  /* if we want to try next size up */
           /* return(NULL); */
        }
      }
  }

  memPartInfoGet(memSysPartId,&memStats);
  DPRINT2(0," MemStat's Alloc'd: %lu bytes, Free: %lu bytes\n", memStats.numBytesAlloc, memStats.numBytesFree);
  /* DPRINT2(-1," Alloc: %lu blocks, Free: %lu blocks\n", memStats.numBlocksAlloc, memStats.numBlocksFree); */

  DPRINT(0," mBufferGet(): No clusters, Malloc buffer\n");
  /* bufAddr = malloc( size + sizeof(long) ); */
  /* bufAddr = (char *) memalign(4, size + sizeof(long) );  /* align to 4 bytes */
  /* if we are going over the limit i.e. will drive memory free below memReserveLevel, then FAIL */
  if ( (memStats.numBytesFree  - size) >  mBufferId->memReserveLevel)
  {
    bufAddr = (char *) memalign(_CACHE_ALIGN_SIZE, size + sizeof(long) );  /* align to 4 bytes */
    if (bufAddr != NULL)
    {
      indxAddr = (long *) bufAddr;
      *indxAddr = 0xbada110c;     /*99  in front of buffer place size index */
      bufAddr += sizeof(long);
      mBufferId->numMallocs++;
      DPRINT3(1,"Malloc: indx: 0x%lx, val: %ld, bufAddr: 0x%lx\n", indxAddr, *indxAddr, bufAddr);
      if (size > mBufferId->maxMalloc)
         mBufferId->maxMalloc = size;
    }
  }
  else
  {
    DPRINT1(-1," mBufferGet(): to malloc would drive free system memory below reserve of: %lu bytes\n", mBufferId->memReserveLevel);
    bufAddr = NULL;
  }

  taskUnlock();
  return(bufAddr);
}

/**************************************************************
*
* mBufferReturn - returns a buffer to the buffer pool
*
*
*  This routine returns the buffer back to the buffer pool
* so that it may be reused. The contents of the returned
* buffer are not altered.
* If there is no space available in the  free list 
* the calling task will BLOCK. This won't happen unless
* a buffer is accidentaly freed multiple times. 
* WARNING:
*    Don't return buffers multiple times or the buffer can
*    be multiply allocated. Not Good.
*
*
* RETURNS:
*  VOID
*
*       Author Greg Brissey 3/02/04
*/
int mBufferReturn(MBUFFER_ID mBufferId,long item)
/* MBUFFER_ID mBufferId - Fast Buffer to deallocate back to */
/* long item - Buffer Address being returned (deallocated) */
{
  long *sizeAddr;
  int index;
  register int toP;
  int stat;
  /*
   * RNG_LONG_PUT(fBufferId->freeList, item, toP);
  */

  if (item == 0)
  {
     errLogRet(LOGIT,debugInfo,"mBufferReturn(): Error: Given NULL pointer to Return\n");
     return -1;
  }

#ifdef INSTRUMENT
     wvEvent(EVENT_FBUF_PUT,NULL,NULL);
#endif

  stat = 0;

  taskLock();

  sizeAddr = (long*) (item - sizeof(long));  /* size index is just ahead of buffer address */
  /* fprintf(" item: 0x%lx, sizeAddr: 0x%lx, value: %ld\n", item,sizeAddr,*sizeAddr); */
  index = *sizeAddr;
  DPRINT3(1,"mBufferReturn() - address: 0x%lx,  sizeAddr: 0x%lx, keyIndex: 0x%lx\n",
	item,sizeAddr,index);

  if (index == 0xbada110c )  /* 99 */
  {
     DPRINT2(0,"mBufferReturn(): Free buffer: 0x%lx, keyL 0x%lx\n", sizeAddr,index);
     free(sizeAddr);
     stat = 1;
  }
  else if ( (index & 0xffff0000) == 0xc0de0000)  /* cluster index */
  {
    index = index & 0x0000ffff;
    if ((index >= 0) && (index < mBufferId->numLists))
    {
       DPRINT1(1," mBufferReturn() - into ringbuffer, cluster index: %d \n",index);
       stat = RNG_LONG_PUT(mBufferId->freeList[index],item,toP);
    }
    else
    {
       errLogRet(LOGIT,debugInfo, "mBufferReturn(): Error:  IndexKey= 0x%lx illegal, legal values: %d - %d, (memory curruption)\n",index,0,mBufferId->numLists);
       stat = -1;
    }
  }
  else
  {
     errLogRet(LOGIT,debugInfo, "mBufferReturn(): Error:  IndexKey= 0x%lx illegal.legal keys: 0xbada11loc or 0xc0de+index  (memory curruption)\n",index);
     stat = -1;
  }

  taskUnlock();
  return stat;
}

/**************************************************************
*
* mBufferClear - clear the fast buffer pool
*
*
*  This routine clears the fast buffer pool. All buffers
* are "deallocated".
*
* RETURNS:
* VOID
*
*       Author Greg Brissey 3/02/04
*/
VOID mBufferClear(register MBUFFER_ID mBufferId)
/* MBUFFER_ID mBufferId - Fast Buffer Pool Id */
{
  unsigned long startAddr;
  register int i,nBufs,size;

  if (mBufferId != NULL)
  {
     taskLock();
     for ( i=0; i < mBufferId->numLists; i++)
     {
      rngLFlush(mBufferId->freeList[i]);
     }
     fillRngwAddrs(mBufferId);
     taskUnlock();
  }
}

/**************************************************************
*
* mBufferInfo - return # of free buffers
*
*  This routine fills the MBUF_INFO structure with statics on the clusters
*
* RETURNS:
* number of free buffers
*
*       Author Greg Brissey 3/2/04
*/
int  mBufferInfoGet(MBUFFER_ID mBufferId,MBUF_INFO *mBuffInfo)
/* MBUFFER_ID mBufferId - Fast Buffer Pool Id */
{
   int i;
   /* The number of free buffers is the number of items still
      in the ring buffer. */
    if (mBufferId != NULL)
    {

     mBuffInfo->numClusters = mBufferId->numLists;
     for ( i=0; i < mBufferId->numLists; i++)
     {
       mBuffInfo->clFree[i] = rngLNElem(mBufferId->freeList[i]);
       mBuffInfo->clSize[i] = mBufferId->mbclusters[i].clSize;
       mBuffInfo->clNum[i] =  mBufferId->mbclusters[i].clNum;
       mBuffInfo->memSize[i] = mBufferId->mbclusters[i].memSize;
     }
     mBuffInfo->numMallocs = mBufferId->numMallocs;
     mBuffInfo->mallocSize = mBufferId->maxMalloc;
   }
    return 0;
}

#ifdef XXX
/**************************************************************
*
* mBufferUsedBufs - return # of used (allocated) buffers
*
*  This routine returns the number of used (allocated) buffers in 
* the buffer pool
*
* RETURNS:
* number of used elements
*
*       Author Greg Brissey 5/26/94
*/
int     mBufferUsedBufs (register MBUFFER_ID mBufferId)
/* MBUFFER_ID mBufferId - Fast Buffer Pool Id */
{
   /* The number of used buffers is the number of unused entries 
      in the ring buffer. */
   return(rngXBlkFreeElem(mBufferId->freeList));
}
#endif

/**************************************************************
*
* mBufferMaxClSize - return maximum cluster buffer size 
*
*  This routine returns the maximum cluster buffer size 
*
* RETURNS:
* maximum single buffer size
*
*       Author Greg Brissey 5/17/95
*/
long  mBufferMaxClSize(register MBUFFER_ID mBufferId)
/* MBUFFER_ID mBufferId - Fast Buffer Pool Id */
{
   int maxindex;
   /* The number of free buffers is the number of items still
      in the ring buffer. */
  maxindex = mBufferId->numLists - 1;
  return (mBufferId->listSizes[maxindex]); /* total malloc'd space */
}

/**************************************************************
*
* mBufferShow - show Fast Buffer Information
*
*
*  This routine displays the information on a buffer pool
*
* RETURNS:
*  VOID
*
*       Author Greg Brissey 7/6/94
*/
void mBufferShow(MBUFFER_ID mBufferId,int level)
/* MBUFFER_ID mBufferId;    fast buffer pool id */
/* int        level;    level of information display */
{
   int i,free;
   printf("\nM Buffer: List\nvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n");
   printf("MBuffer Id: 0x%lx\n",mBufferId);
   printf("__________________\n");
   printf("CLUSTER POOL TABLE\n");
   printf("_______________________________________________________________________________\n");
   printf("     size     clusters   free    usage   failed      memArea          memSize\n");
   printf("-------------------------------------------------------------------------------\n");
   for (i=0; i < mBufferId->numLists; i++)
   {
     free = rngLNElem( mBufferId->freeList[i] );
     printf("%8d\t%4d\t%4d\t%4d\t%4d\t    0x%08lx\t%8ld\n",
	   mBufferId->mbclusters[i].clSize,mBufferId->mbclusters[i].clNum,
           free, mBufferId->clCnt[i],  mBufferId->clFailCnt[i],
	   mBufferId->mbclusters[i].memArea,mBufferId->mbclusters[i].memSize);
   }
   printf("-------------------------------------------------------------------------------\n");
   printf("Forced to Malloc: %d, Max Size: %d\n",
           mBufferId->numMallocs, mBufferId->maxMalloc);
   printf("_______________________________________________________________________________\n");

   return;
}

