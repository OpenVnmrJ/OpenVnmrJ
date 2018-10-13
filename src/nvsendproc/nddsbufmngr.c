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

/* #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#include <stdlib.h>
#include <string.h>
#include "rngBlkLib.h"
#include "rngLLib.h"
#include "nddsbufmngr.h"

/*
modification history
--------------------
11-16-2005,gmb  created
*/

/*
DESCRIPTION

nddsBufMngr is a buffer and blocking msgQ for use with NDDS subscription callback functiona
It provides a pool for buffers that can be accessed quickly for use in the NDDS callback
where time is of the essences.  The callback should not be block for any lenght of time.
during run-time. 

As well there is a blocking message Q, which allow the NDDS callback to post the message recieved and
returned quickly.
Another thread waiting on messages, will block until a message is posted in this manner.
The it up to this thread to return the buffer that was obtain in the callback.

*/
 
/**************************************************************
*
*  nddsBufMngrCreate - Creates a Pool of Buffers for the NDDS callback to copy issues to
*                      and a blocking rngbuffer to post the address of the copied message buffers
*
*  This routine creates a set of buffers and returns
* a handle to a NDDSBUFMNGR_ID. This ID can be used to
* obtain and return individual buffers to and from the pool.
* The blockin ring buffer allow the messages to be posted where another thread can
* block waiting on message or NDDS issues
*
* RETURNS:
* ID of the buffer manager, or NULL if memory cannot be allocated.
*
*       Author Greg Brissey 11/16/2005
*/
NDDSBUFMNGR_ID nddsBufMngrCreate(int nBuffers, int bufSize)
/* int nBuffers - number of buffers in memory pool */
/* int bufSize - byte size of each buffer */
{
  NDDSBUFMNGR_ID pBufMngrId;
  long sizeNBytes;
  char *startAddr;
  int i;
 
  pBufMngrId = (NDDSBUFMNGR_ID) malloc(sizeof(NDDSBUFMNGR_OBJ));  /* create structure */

  if (pBufMngrId == NULL)
     return (NULL);
 
  /* zero out structure */
  memset(pBufMngrId,0,sizeof(NDDSBUFMNGR_OBJ));
 
  pBufMngrId->nBufs = nBuffers;
  pBufMngrId->bufSize = bufSize;
 
  /* the blocking msg Q */
  pBufMngrId->msgeList = rngBlkCreate(nBuffers,"NDDS CallBk Msge Q",1);
  pBufMngrId->freeList = rngLCreate(nBuffers,"NDDS CallBk Buffer Free List");
  /* free list of available buffer addresses */
  pBufMngrId->freeLstSize = nBuffers;    /* ring buffer size */
  pBufMngrId->msgLstSize = nBuffers;    /* ring buffer size */
 
  if (pBufMngrId->freeList == NULL)
  {
    free(pBufMngrId);
    return(NULL);
  }
 
  sizeNBytes = nBuffers * bufSize;
  pBufMngrId->totalSize = sizeNBytes;   /* total malloc'd space */
 
  if ((startAddr = (char *) malloc(sizeNBytes)) == NULL)
  {
       rngLDelete(pBufMngrId->freeList);
       rngBlkDelete(pBufMngrId->msgeList);
       free(pBufMngrId);
       return(NULL);
  }
  pBufMngrId->mallocAddr = (char*) startAddr;

  /* fill in freeList with addresses of buffers */
  nddsBufMngrInitBufs(pBufMngrId);

  return(pBufMngrId);
}


/**************************************************************
*
*  nddsBufMngrInitBufs - clears all the ring buffers, and refills the free list with buffer addresses
*
*
* RETURNS:
* 0 
*
*       Author Greg Brissey 11/16/2005
*/
int nddsBufMngrInitBufs(NDDSBUFMNGR_ID pBufMngrId)
{
   char *startAddr;
   int i, stat, toP;
  /* clear ring buffers */
  rngBlkFlush(pBufMngrId->msgeList);
  rngLFlush(pBufMngrId->freeList);

  startAddr = pBufMngrId->mallocAddr;

  /* Fill Free List with the list of Addresses */
  for (i=0; i < pBufMngrId->freeLstSize; i++)
  {
    /* printf("nddsBufMngrInitBufs(0x%lx): address: 0x%lx\n",pBufMngrId,startAddr); */
    stat = RNG_LONG_PUT(pBufMngrId->freeList,(long) startAddr,toP);
    startAddr += pBufMngrId->bufSize;
  }
 
  return(0);
}


/**************************************************************
*
*  msgeBufGet - returns the address of a free buffer 
*
*  This routine returns the address of a buffer which can be copy data into
*
* RETURNS:
*  address of a free buffer, or NULL if none available.
*
*       Author Greg Brissey 11/16/2005
*/
char *msgeBufGet(NDDSBUFMNGR_ID pBufMngrId)
{
   char *bufAddr;
   int stat, toP;

   stat = RNG_LONG_GET(pBufMngrId->freeList, (long*) &bufAddr,toP);
    /* printf("msgeBufGet(0x%lx): address: 0x%lx\n",pBufMngrId,bufAddr); */
   if (stat == 1) 
       return(bufAddr);
   else
       return(NULL);
}


/**************************************************************
*
*  msgeBufReturn - returns the buffer address back into the free list for reuse
*
*  This routine is the counter part of msgeBufGet, returns the 
* buffer address obtain from msgeBufGet back into the free list
*
* RETURNS:
* 0 or -1 if unable to return the address 
*
*       Author Greg Brissey 11/16/2005
*/
int msgeBufReturn(NDDSBUFMNGR_ID pBufMngrId,char *item)
{
   int stat,retstat,toP;

    /* printf("msgeBufReturn(0x%lx): address: 0x%lx\n",pBufMngrId,item); */
   stat = RNG_LONG_PUT(pBufMngrId->freeList,(long) item,toP);
   retstat = (stat == 1) ? 0 : -1;
   return(retstat);
}


/**************************************************************
*
*  msgePost - places the address of the message buffer on to a message Q
*
*  This routine places the address of the message buffer on to a message Q
*
* RETURNS:
* ID of the buffer manager, or NULL if memory cannot be allocated.
*
*       Author Greg Brissey 11/16/2005
*/
int msgePost(NDDSBUFMNGR_ID pBufMngrId, char *item)
{
    int stat;
    /* printf("msgePost(0x%lx): address: 0x%lx\n",pBufMngrId,item); */
    stat = rngBlkPut(pBufMngrId->msgeList,&item,1);
    return(stat);
}

/**************************************************************
*
*  msgeGet - Get the next message buffer address from the message Q
*
*  This routine gets the next message buffer address from the message Q
*  if there are no messages, the the calling routine will be blocked.
*
* RETURNS:
*  address of message buffer, or NULL on error
*
*       Author Greg Brissey 11/16/2005
*/
char *msgeGet(NDDSBUFMNGR_ID pBufMngrId)
{
    char *msgPtr;
    int stat;

    stat = rngBlkGet(pBufMngrId->msgeList,&msgPtr,1);
    /* printf("msgeGet(0x%lx): address: 0x%lx\n",pBufMngrId,msgPtr); */
    if (stat == 1)
      return(msgPtr);
    else
      return(NULL);
}

/**************************************************************
*
*  msgeGetWillPended - return if msgeGet() would pend
*
*  This routine return the if a call to msgeGet() would Pend.
*
* RETURNS:
*  1 is thread is block on msgeGet, else 0
*
*       Author Greg Brissey 11/16/2005
*/
int msgeGetWillPend(NDDSBUFMNGR_ID pBufMngrId)
{
    return( rngBlkIsEmpty(pBufMngrId->msgeList) );
}

/**************************************************************
*
*  msgeGetIsPended - return the blocked state of a thread read the message Q
*
*  This routine return the blocked state of a thread read the message Q
*
* RETURNS:
*  1 is thread is block on msgeGet, else 0
*
*       Author Greg Brissey 11/16/2005
*/
int msgeGetIsPended(NDDSBUFMNGR_ID pBufMngrId)
{
    return( rngBlkIsGetPended(pBufMngrId->msgeList) );
}
