/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* fBufferLib.c 11.1 07/09/07  - Fast Buffer Library Source Modules */
/* 
 */

#ifndef ALLREADY_POSIX
 #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#endif
#include <vxWorks.h>
#include <stdlib.h>
#include <semLib.h>
#include "instrWvDefines.h"
#include "commondefs.h"
#include "rngXBlkLib.h"
#include "fBufferLib.h"


/*
modification history
--------------------
7-06-94,gmb  created 
*/

/*
DESCRIPTION
 
fBufferLib is a fast fixed-size buffer simple memory manager. It provides
a pool for buffers that can be used for fast allocation/deallocation
during run-time. This library avoids the time consuming malloc and free
calls. 
  This buffer memory manager can use a given address as the start of the
buffer pool. This is useful where one does not want the book keeping
data of malloc intermixed with the data. For example the memory of a
STM VME board.
  If an address is provide it is the responsibility of the application
to insure: 
     1.  The memory is accessible, 
     2.  That there is enough memory for the pool
     3.  That the memory addresses do not conflict with the system memory 

  If no address is passed then a memory pool large enough is malloc from the
system memory pool.


*/

/**************************************************************
*
*  fBufferCreate - Creates a Pool of Fixed Size Memory Buffers 
*
*  This routine creates a set of buffers and returns
* a handle to a FBUFFER_ID. This ID can be used to
* allocate and free individual buffers from the pool.
* The memory for the buffer pool is obtained via
* "malloc" from the system memory if memAddr is Null.
* Otherwise memAddr is used as the first buffer address.
* WARNING: if an address is passed in memAddr no checks are
*   performed. It is the responsibility of the application to insure: 
*     1.  The memory is accessible, 
*     2.  That there is enough memory for the pool
*     3.  That the memory addresses do not conflict with the system memory 
*
* RETURNS:
* ID of the buffer pool, or NULL if memory cannot be allocated.
*
*	Author Greg Brissey 7/6/94
*/
FBUFFER_ID fBufferCreate(int nBuffers, int bufSize, char* memAddr,int eventid)
/* int nBuffers - number of buffers in memory pool */
/* int bufSize - byte size of each buffer */
/* char* memAddr;   0 - use system memory, else address of 1st buffer */
{
  FBUFFER_ID pFBufferId;
  long sizeNBytes;
  unsigned long startAddr;
  int i;

  pFBufferId = (FBUFFER_ID) malloc(sizeof(FBUFFER_OBJ));  /* create structure */
  if (pFBufferId == NULL) 
     return (NULL);
 
  /* zero out structure */
  memset(pFBufferId,0,sizeof(FBUFFER_OBJ));

  pFBufferId->nBufs = nBuffers;
  pFBufferId->bufSize = bufSize;

  pFBufferId->freeList = rngXBlkCreate(nBuffers,"Fast Buffer Free List",eventid,1);
  pFBufferId->freeLstSiz = nBuffers;	/* ring buffer size */

  if (pFBufferId->freeList == NULL)
  {
    free(pFBufferId);
    return(NULL);
  }

  sizeNBytes = nBuffers * bufSize;
  pFBufferId->totalSize = sizeNBytes;	/* total malloc'd space */

  if (memAddr == NULL)
  {
     if ((startAddr = (unsigned long) malloc(sizeNBytes)) == NULL)
     {
       rngXBlkDelete(pFBufferId->freeList);
       free(pFBufferId);
       return(NULL);
     }
     pFBufferId->poolAddr = (char*) startAddr;
     pFBufferId->mallocAddr = (char*) startAddr;
  }
  else
  {
     startAddr = (unsigned long) memAddr;
     pFBufferId->poolAddr = (char*) startAddr;
  }

  /* Fill Free List with the list of Addresses */
  for (i=0; i<nBuffers; i++)
  {
    rngXBlkPut(pFBufferId->freeList, &startAddr, 1); 
    startAddr += bufSize;
  }

  return(pFBufferId);
}

/**************************************************************
*
*  fBufferReUse - Re-initializes an Existing Pool of Fixed Size Memory Buffers 
*
*  This routine allows changing of an existing set of fBuffer buffers 
*  characteristics. The size and number of buffers can be redefined.
*  Based on the initial allocation of the number of buffers and their
*  size the number of buffers avialable are returned.
*  E.G. Number of buffers can never be larger than the initial number of
*       buffers.
*       If the buffer size is much larger then the number will be even
*	less.
*
* RETURNS:
* number of buffers available, as limited by the initial creation of buffers.
*  -1 if NULL Pointer
*  -2 if Requested Buffer Size to Large to handle
*
*	Author Greg Brissey 9/26/94
*/
int fBufferReUse(FBUFFER_ID pFBufferId,int nBuffers, int bufSize)
/* FBUFFER_ID fBufferId - Fast Buffer Id */
/* int nBuffers - number of buffers in memory pool */
/* int bufSize - byte size of each buffer */
{
  long sizeNBytes;
  long newNBufs;
  unsigned long startAddr;
  int i;

  if (pFBufferId == NULL) 
     return (-1);
 
   /* Which is my limiting factors buffer or ring buffer */
   newNBufs = nBuffers;
   if (nBuffers > pFBufferId->freeLstSiz)
   {
      /* --- ring buffer limited so far --- */
      newNBufs = pFBufferId->freeLstSiz;
   }

   sizeNBytes = newNBufs * bufSize;
   if (sizeNBytes > pFBufferId->totalSize)
   {
      /* --- Heap space limited --- */
      newNBufs = pFBufferId->totalSize / bufSize;
   }

  if (newNBufs < 1)
      return(-2);

  
  pFBufferId->nBufs = newNBufs;
  pFBufferId->bufSize = bufSize;

  /* Fill Free List with the list of Addresses */
  rngXBlkReSize(pFBufferId->freeList,newNBufs);  /* can never be larger than
						    at creation time */
  startAddr = (unsigned long) pFBufferId->poolAddr;
  for (i=0; i<newNBufs; i++)
  {
    rngXBlkPut(pFBufferId->freeList, &startAddr, 1); 
    startAddr += bufSize;
  }

  return(newNBufs);
}

/**************************************************************
*
* fBufferDelete - Delete Fast Buffer Pool
*
*
*  Delete Fast Buffer Pool
*  This routine deletes the Fast Buffer Pool releasing memory
*
* RETURNS:
* VOID
*
*               Author Greg Brissey 7/6/94
*/
void fBufferDelete(register FBUFFER_ID fBufferId)
/* FBUFFER_ID fBufferId - Fast Buffer Id */
{
   if (fBufferId != NULL)
   {
       if (fBufferId->mallocAddr != NULL)
	  free(fBufferId->poolAddr);
       rngXBlkDelete(fBufferId->freeList);
       free(fBufferId);
   }
   return;
}

/*----------------------------------------------------------------------*/
/* fBufferShwResrc							*/
/*     Show system resources used by Object (e.g. semaphores,etc.)	*/
/*	Useful to print then related back to WindView Events		*/
/*----------------------------------------------------------------------*/
VOID fBufferShwResrc(register FBUFFER_ID fBufferId,int indent)
{
   int i;
   char spaces[40];

   for (i=0;i<indent;i++) spaces[i] = ' ';
   spaces[i]='\0';

   printf("%s fBuffer Obj: 0x%lx\n",spaces,fBufferId);
   printf("\n%s   Ring Buffer:\n",spaces);
   rngXBlkShwResrc(fBufferId->freeList,indent+4);
}

/**************************************************************
*
* fBufferGet - get the next available buffer 
*
*
*  This routine returns the next available buffer address.
* If there is no buffers available the calling task will BLOCK.
*
*
* RETURNS:
*  The Address of a available buffer.
*
*       Author Greg Brissey 7/06/94
*/
char *fBufferGet(register FBUFFER_ID fBufferId)
/* FBUFFER_ID fBufferId - Fast Buffer to allocate from */
{
  unsigned long bufAddr;

#ifdef INSTRUMENT
     wvEvent(EVENT_FBUF_GET,NULL,NULL);
#endif

  rngXBlkGet(fBufferId->freeList, &bufAddr, 1);

  return((char*)bufAddr); 
}

/**************************************************************
*
* fBufferReturn - returns a buffer to the buffer pool
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
*       Author Greg Brissey 7/06/94
*/
void fBufferReturn(register FBUFFER_ID fBufferId,long item)
/* FBUFFER_ID fBufferId - Fast Buffer to deallocate back to */
/* long item - Buffer Address being returned (deallocated) */
{
  /*
  register int toP;
  RNG_LONG_PUT(fBufferId->freeList, item, toP);
  */

#ifdef INSTRUMENT
     wvEvent(EVENT_FBUF_PUT,NULL,NULL);
#endif

  rngXBlkPut(fBufferId->freeList, &item, 1);

  return;
}

/**************************************************************
*
* fBufferClear - clear the fast buffer pool
*
*
*  This routine clears the fast buffer pool. All buffers
* are "deallocated".
*
* RETURNS:
* VOID
*
*       Author Greg Brissey 7/06/94
*/
VOID fBufferClear(register FBUFFER_ID fBufferId)
/* FBUFFER_ID fBufferId - Fast Buffer Pool Id */
{
  unsigned long startAddr;
  register int i,nBufs,size;

  if (fBufferId != NULL)
  {

     rngXBlkFlush(fBufferId->freeList);

     nBufs = fBufferId->nBufs;
     size = fBufferId->bufSize;
     startAddr = (unsigned long) fBufferId->poolAddr;

     /* Fill Free List with the list of Addresses */
     for (i=0; i<nBufs; i++)
     {
       rngXBlkPut(fBufferId->freeList, &startAddr, 1); 
       startAddr += size;
     }
  }
}

/**************************************************************
*
* fBufferFreeBufs - return # of free buffers
*
*  This routine returns the number of free (unallocated) buffers in 
* the fast buffer pool.
*
* RETURNS:
* number of free buffers
*
*       Author Greg Brissey 5/26/94
*/
int  fBufferFreeBufs(register FBUFFER_ID fBufferId)
/* FBUFFER_ID fBufferId - Fast Buffer Pool Id */
{
   /* The number of free buffers is the number of items still
      in the ring buffer. */
   return(rngXBlkNElem(fBufferId->freeList));
}

/**************************************************************
*
* fBufferUsedBufs - return # of used (allocated) buffers
*
*  This routine returns the number of used (allocated) buffers in 
* the buffer pool
*
* RETURNS:
* number of used elements
*
*       Author Greg Brissey 5/26/94
*/
int     fBufferUsedBufs (register FBUFFER_ID fBufferId)
/* FBUFFER_ID fBufferId - Fast Buffer Pool Id */
{
   /* The number of used buffers is the number of unused entries 
      in the ring buffer. */
   return(rngXBlkFreeElem(fBufferId->freeList));
}

/**************************************************************
*
* fBufferMaxSize - return maximum single buffer size 
*
*  This routine returns the maximum single buffer size 
*
* RETURNS:
* maximum single buffer size
*
*       Author Greg Brissey 5/17/95
*/
long  fBufferMaxSize(register FBUFFER_ID fBufferId)
/* FBUFFER_ID fBufferId - Fast Buffer Pool Id */
{
   /* The number of free buffers is the number of items still
      in the ring buffer. */
  return (fBufferId->totalSize); /* total malloc'd space */
}


/**************************************************************
*
* fBufferShow - show Fast Buffer Information
*
*
*  This routine displays the information on a buffer pool
*
* RETURNS:
*  VOID
*
*       Author Greg Brissey 7/6/94
*/
VOID fBufferShow(register FBUFFER_ID fBufferId,int level)
/* FBUFFER_ID fBufferId;    fast buffer pool id */
/* int        level;    level of information display */
{

   printf("Fast Buffer Id: 0x%lx\n",fBufferId);
   printf("Buffer Pool Start Addr:  0x%lx, Malloc Addr:  0x%lx\n",
	   fBufferId->poolAddr,fBufferId->mallocAddr);
   printf("# Buffers: %d, Size: %d\n", fBufferId->nBufs, fBufferId->bufSize);
 
   printf("\nFast Buffer: Free Buffer List\nvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n");
   rngXBlkShow(fBufferId->freeList,level);
   printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");

   return;
}

