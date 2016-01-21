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
//  For Windows compilation be sure to use -D_WIN32_WINNT=0x501 (= XP OS) in compilation

#ifndef VNMRS_WIN32

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#ifndef LINUX
#include <thread.h>
#endif
#include <pthread.h>

#define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */

#else /* VNMRS_WIN32 */

#include <Windows.h>
#include <stdio.h>
#include "rngBlkLib.h"

#endif  /* VNMRS_WIN32 */

#include "rngBlkLib.h"

/*
modification history
--------------------
4-06-05,gmb  created / stolen from vwacq code, but it was mine so it's OK to steal.
*/

/*
DESCRIPTION

Blocking Ring Buffer Library, used with threads library
routines to create a blocking ring buffer.
Threads writing or reading from this buffer can block.
Write will block if buffer is full, read will block if buffer is
empty.
For additional description see rngLib.

*/
static char *RngBlkID ="Blking Ring Buffer";
static int  IdCnt;


#ifndef VNMRS_WIN32

/**************************************************************
*
*  rngBlkCreate - Creates an empty blocking ring buffer 
*
*
*  This routine creates a blocking ring buffer of size <n elements(longs)> 
*and initializes it. Memory for the buffer is allocated from 
*the system memory partition.
*
*
* RETURNS:
* ID of the blocking ring buffer, or NULL if memory cannot be allocated.
*
*	Author Greg Brissey 5/26/94
*/
RINGBLK_ID rngBlkCreate(int nelem,char* idstr, int blklevel)
/* int nelem;  size in elements of ring buffer */
/* char* idstr;  Optional identifier string */
/* int   wvEventID;  Upper User Event ID */
/* int   blklevel;  Block Level, ie. till blklevel entries are in the ring buffer */
{
  RINGBLK_ID pBlkRng;
  char tmpstr[80];
  int stat;

  pBlkRng = (RINGBLK_ID) malloc(sizeof(RING_BLKING));  /* create structure */
  if (pBlkRng == NULL) 
     return (NULL);

  memset(pBlkRng,0,sizeof(RING_BLKING));

  if (idstr == NULL) 
  {
     sprintf(tmpstr,"%s %d\n",RngBlkID,++IdCnt);
     pBlkRng->pRngIdStr = (char *) malloc(strlen(tmpstr)+2);
  }
  else
  {
     pBlkRng->pRngIdStr = (char *) malloc(strlen(idstr)+2);
  }

  if (pBlkRng->pRngIdStr == NULL)
  {
     free(pBlkRng);
     return(NULL);
  }

  if (idstr == NULL) 
  {
     strcpy(pBlkRng->pRngIdStr,tmpstr);
  }
  else
  {
     strcpy(pBlkRng->pRngIdStr,idstr);
  }

  /* initialize blocking flags */
  pBlkRng->readBlocked = FALSE;
  pBlkRng->writeBlocked = FALSE;

  /* create Ring Buffer size buffer nelem + 1 */
  pBlkRng->rBuf = (long *) malloc((sizeof(long) * nelem) + (sizeof(long)) );

  if ( (pBlkRng->rBuf) == NULL)
  {
     free(pBlkRng->pRngIdStr);
     free(pBlkRng);
     return(NULL);
  }

  pBlkRng->pToBuf = pBlkRng->pFromBuf = 0;
  
  pBlkRng->maxSize = pBlkRng->bufSize = nelem + 1;

   /* ie block till one entry present means blkTilNentries == 0 */
  pBlkRng->forcedUnBlock = 0;
  pBlkRng->blkTilNentries = pBlkRng->triggerLevel = blklevel - 1 ;  /* noramlly blklevel = 1 */
  if (pBlkRng->blkTilNentries < 0 )
      pBlkRng->blkTilNentries = pBlkRng->triggerLevel = 0;
  
   /* create the Blocking Ring Buffer structure Mutual Exclusion semaphore */
   stat = pthread_mutex_init(&pBlkRng->mutex,NULL);  /* assign defaults to mutex */
   /* printf("stat: %d, Mutex: 0x%lx\n",stat,pBlkRng->mutex); */

   /* create the two synchronization conditionals (read & write blocking sem) */
   stat = pthread_cond_init(&pBlkRng->syncOk2Read,NULL);
   /* printf("stat: %d, Ok2Read cond: 0x%lx\n",stat,pBlkRng->syncOk2Read); */
   stat = pthread_cond_init(&pBlkRng->syncOk2Write,NULL);
   /* printf("stat: %d, Ok2Write cond: 0x%lx\n",stat,pBlkRng->syncOk2Write); */

   return( pBlkRng );
}

/**************************************************************
*
* rngBlkDelete - Delete Blocking Ring Buffer
*
*
*  Delete Blocking Ring Buffer
*  This routine deletes a blocking ring buffer 
*
* RETURNS:
* N/A
*
*		Author Greg Brissey 5/26/94
*/
void rngBlkDelete(register RINGBLK_ID rngd)
/* RINGBLK_ID rngd;  blocking ring buffer to delete */
{
  free(rngd->pRngIdStr);
  free(rngd->rBuf);
  free(rngd);
}

/**************************************************************
*
* rngBlkFlush - make a blocking ring buffer empty
*
*
*  This routine initialized a blocking ring buffer to be empty.
*Any data in the buffer will be lost. If writing is blocked it
*is released.
*
* RETURNS:
* N/A
*
*	Author Greg Brissey 5/26/94
*/
int rngBlkFlush(register RINGBLK_ID rngd)
/* RINGBLK_ID rngd; Blocking ring buffer to initialize */
{
  int npend,pAry[4];
  int status;
  status = pthread_mutex_lock(&rngd->mutex);
  if (status != 0)
      return(status);

  rngd->pToBuf = rngd->pFromBuf = 0;

  /* just flushed the ring buffer, if write is block */
  /* release the writing because there is now room in the buffer */
  if (rngd->writeBlocked)
  {
	/* maybe STATUS taskLock() & STATUS taskUnlock() should be used ? */
     /*
      printf("rngBlkGet: OK2Write given.\n");
     */
      rngd->writeBlocked = FALSE;

      status = pthread_cond_broadcast(&rngd->syncOk2Write);
  }
  status = pthread_mutex_unlock(&rngd->mutex);
  return(status);
}


/**************************************************************
*
* rngBlkPut - put element into a blocking ring buffer
*
*
*  This routine copies n elements from <buffer> into blocking ring buffer
*<rngd>. The specified number of elements will be put into the ring buffer.
*If there is insuffient room the calling task will BLOCK.
*
* RETURNS:
*  The number of elements actually put into ring buffer.
*
*	Author Greg Brissey 5/26/94
*/
int rngBlkPut(register RINGBLK_ID rngd,register long* buffer,register int size)
/* RINGBLK_ID rngd;	blocking ring buffer to put data into */
/* long*      buffer;   buffer to get data from */
/* int	      size;     number of elements to put */
{
   register int fromP;
   int status, fbytes;
   int npend,pAry[4];
   register int result,i;

   status = pthread_mutex_lock(&rngd->mutex);
   if (status != 0)
      return(status);

   while( (fbytes = ( 
	       ( (result = ((rngd->pFromBuf - rngd->pToBuf) - 1)) < 0) ?
                  result + rngd->bufSize : result ) ) < size)
   {
      /*
      printf("rngBlkPut: semGive OK2Read., free bytes: %d \n",fbytes);
      */

      rngd->writeBlocked = TRUE;
      if (rngd->readBlocked)			    /* if read blocked, */
      {
         rngd->readBlocked = FALSE;
         /* printf("give Ok2Read\n"); */
         status = pthread_cond_broadcast(&rngd->syncOk2Read);
      } 

       /* standard while constrcut for waiting on a cond varible */
       while( rngd->writeBlocked)
       {
          /* printf("take Ok2Write\n"); */
          status = pthread_cond_wait( &rngd->syncOk2Write, &rngd->mutex);
          if (status != 0)
          {
              status = pthread_mutex_unlock(&rngd->mutex);
              return(status);
          }
       } 

   }

   for (i = 0; i < size; i++)
   {
     /* this macro inlines the code for speed */
     RNG_LONG_PUT(rngd, (buffer[i]), fromP);
   }

   /* if I just wrote something into the ring buffer & read is block */
   /* release the reading because there is now room in the buffer */

   if ( (rngd->readBlocked) && ( rngBlkNElem(rngd) > rngd->blkTilNentries) )
   {
     /* printf("rngBlkGet: OK2Write given.\n"); */
      rngd->readBlocked = FALSE;

      /* printf("give Ok2Read\n"); */
      status = pthread_cond_broadcast(&rngd->syncOk2Read);
   }
   status = pthread_mutex_unlock(&rngd->mutex);
   return( size );
}

/**************************************************************
*
* rngBlkGet - get elements from a blocking ring buffer 
*
*
*  This routine copies elements from blocking ring buffer <rngd> into <buffer>
*The specified number of elements will be put into the buffer.
*If there is insuffient elements in the blocking ring buffer the calling 
*task will BLOCK.
*
* NOTE:  Presently only supports size = 1
*
* RETURNS:
*  The number of elements actually put into buffer.
*
*	Author Greg Brissey 5/26/94
*/
int rngBlkGet(register RINGBLK_ID rngd,long* buffer,int size)
/* RINGBLK_ID rngd;	blocking ring buffer to get data from */
/* char*      buffer;   point to buffer to receive data */
/* int	      size;     number of elements to get */
{
   register int fromP;
   int status, bytes;
   int npend,pAry[4];

   status = pthread_mutex_lock(&rngd->mutex);
   if (status != 0)
      return(status);

   while( (RNG_LONG_GET(rngd, buffer,fromP)) == 0)
   {
      /* printf("rngBlkGet: Nothing in Ring buffer, fromP = %d\n", fromP); */

      rngd->readBlocked = TRUE;
      if (rngd->writeBlocked)
      {
         rngd->writeBlocked = FALSE;
         status = pthread_cond_broadcast(&rngd->syncOk2Write);
      } 

       /* standard while construct for waiting on cond var */
       while( rngd->readBlocked)
       {
          /* printf("take Ok2Read\n"); */
          status = pthread_cond_wait( &rngd->syncOk2Read, &rngd->mutex);
          if (status != 0)
          {
              status = pthread_mutex_unlock(&rngd->mutex);
              return(status);
          }
       }
   }

   /* if I just read something out of the ring buffer & write is block */
   /* release the writing because there is now room in the buffer */
   if (rngd->writeBlocked)
   {
      /* printf("rngBlkGet: OK2Write given.\n"); */
      rngd->writeBlocked = FALSE;

      status = pthread_cond_broadcast(&rngd->syncOk2Write);
   }
   status = pthread_mutex_unlock(&rngd->mutex);
   return ( 1 );   /* For now only get one item at a time. */
}

/**************************************************************
*
* rngBlkShow - show blocking ring buffer information 
*
*
*  This routine displays the information on a blocking ring buffer <rngd> 
*
* RETURNS:
*  VOID 
*
*	Author Greg Brissey 8/9/93
*/
void rngBlkShow(RINGBLK_ID rngd,int level)
/* RINGBLK_ID rngd;	blocking ring buffer to get data from */
/* int	      level;    level of information display */
{
    int used,free,total;
    int npend,pAry[4];
    int i;

   used = rngBlkNElem (rngd);
   free = rngBlkFreeElem (rngd);
   total = used + free;

   printf("Blk Ring BufferID: '%s', 0x%lx\n",rngd->pRngIdStr,rngd);
   printf("Buffer Addr: 0x%lx, Size: %d (0x%x), UnBlocking Trigger Level: %d entries\n",rngd->rBuf,
		rngd->bufSize,rngd->bufSize,rngd->triggerLevel+1);
   printf("Entries  Used: %d, Free: %d, Total: %d\n", used, free, total);

   printf("Buffer Put Index: %d (0x%x), Get Index: %d (0x%x)\n",
		rngd->pToBuf,rngd->pToBuf,rngd->pFromBuf,rngd->pFromBuf);

   printf("\nBlocking Flags Status:  ");
   printf("READ: %s, ",((rngd->readBlocked == 0) ? "READY" : "BLOCKED"));
   printf("WRITE: %s \n",((rngd->writeBlocked == 0) ? "READY" : "BLOCKED"));
   if (level > 0) 
   {
     long *buff;

     buff = (long*) rngd->rBuf;
     for(i=0; i < total + 1 ; i++)
       printf("item[%d] = %ld (0x%lx)\n",i,buff[i],buff[i]);
   }

}


/**************************************************************
*
* rngBlkFreeElem - return the number of Free Elements in the 
*		    ring buffer empty
*
*
*  This routine returns the number of free elements in the blocking ring 
*  buffer. 
*
* RETURNS:
* number of free elements
*
*	Author Greg Brissey 5/26/94
*/
int 	rngBlkFreeElem (register RINGBLK_ID ringId)
{
   register int result;

   return( ( (result = ((ringId->pFromBuf - ringId->pToBuf) - 1)) < 0) ? 
	   result + ringId->bufSize : result );
}

/**************************************************************
*
* rngBlkNElem - return the number of Used Elements in the 
*		    ring buffer empty
*
*
*  This routine returns the number of used elements in the blocking ring 
*buffer. 
*
* RETURNS:
* number of used elements
*
*	Author Greg Brissey 5/26/94
*/
int 	rngBlkNElem (register RINGBLK_ID ringId)
{
   register int result;

   return( ( (result = (ringId->pToBuf - ringId->pFromBuf)) < 0) ? 
	   result + ringId->bufSize : result );
}

/**************************************************************
*
* rngBlkIsEmpty - returns 1 if ring buffer is empty
*
*
*  This routine returns 1 if ring buffer is empty
*
* RETURNS:
*  0 or 1 
*
*	Author Greg Brissey 5/26/94
*/
int 	rngBlkIsEmpty (register RINGBLK_ID ringId)
{
    return ( (ringId->pToBuf == ringId->pFromBuf) ? 1 : 0 );
}

/**************************************************************
*
* rngBlkIsFull - returns 1 if ring buffer is full
*
*
*  This routine returns 1 if ring buffer is full
*
* RETURNS:
*  0 or 1 
*
*	Author Greg Brissey 5/26/94
*/
int 	rngBlkIsFull (RINGBLK_ID ringId)
{
    register int result;

    if ( (result = ((ringId->pToBuf - ringId->pFromBuf) + 1)) == 0)
    {
	return( 1 );
    }
    else if ( result != ringId->bufSize)
    {
	return( 0 );
    }
    else
        return( 1 );
}

/**************************************************************
*
* rngBlkIsGetPended - returns 1 if reading has been pended
*
*
*  This routine returns 1 if rngBlkGet has been pended
*
* RETURNS:
*  0 or 1 
*
*	Author Greg Brissey 5/26/94
*/
int 	rngBlkIsGetPended (register RINGBLK_ID ringId)
{
   return ( (ringId->readBlocked == TRUE) ? 1 : 0 );
}

/**************************************************************
*
* rngBlkIsPutPended - returns 1 if writing has been pended
*
*
*  This routine returns 1 if rngBlkPut has been pended
*
* RETURNS:
*  0 or 1 
*
*	Author Greg Brissey 5/26/94
*/
int 	rngBlkIsPutPended (register RINGBLK_ID ringId)
{
   return ( (ringId->writeBlocked == TRUE) ? 1 : 0 );
}


/* #define TEST_MAIN */
#ifdef TEST_MAIN
void *reader_routine (void *arg)
{
   RINGBLK_ID pRngBufs = (RINGBLK_ID) arg;
   int cnt, status;
   long buffer;
   int state;

   cnt = 0;
   while(1)
   {
       printf("get buffer: %d, ",++cnt);
       rngBlkGet(pRngBufs, &buffer,1);
       printf("addr: 0x%lx\n",buffer);
   }
}

main()
{
   RINGBLK_ID pRngBufs;
   pthread_t  RdthreadId;
   int status,i;
   long bufcnt;
   bufcnt=0;
   /* create buffer object */
   pRngBufs = rngBlkCreate(10,"test rng buffer", 1);
   rngBlkShow(pRngBufs,1);
   for(i=0;i< 9; i++)
   {
     printf("put in buf: %d\n",++bufcnt);
     rngBlkPut(pRngBufs,&bufcnt,1);
   }
   rngBlkShow(pRngBufs,1);
   status = pthread_create (&RdthreadId,
               NULL, reader_routine, (void*) pRngBufs);
   if (status != 0)
   {
        printf("pthread_create error\n");
         exit(1);
   }
   
   for(i=0;i< 9; i++)
   {
     printf("put in buf: %d\n",++bufcnt);
     rngBlkPut(pRngBufs,&bufcnt,1);
   }
   while(1)
   {
     sleep(2);
     printf("put in buf: %d\n",++bufcnt);
     rngBlkPut(pRngBufs,&bufcnt,1);
   }
}
#endif


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
#else   /* VNMRS_WIN32 */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */


/**************************************************************
*
*  rngBlkCreate - Creates an empty blocking ring buffer 
*
*
*  This routine creates a blocking ring buffer of size <n elements(longs)> 
*and initializes it. Memory for the buffer is allocated from 
*the system memory partition.
*
*
* RETURNS:
* ID of the blocking ring buffer, or NULL if memory cannot be allocated.
*
*	Author Greg Brissey 5/26/94
*/
RINGBLK_ID rngBlkCreate(int nelem,char* idstr, int blklevel)
/* int nelem;  size in elements of ring buffer */
/* char* idstr;  Optional identifier string */
/* int   wvEventID;  Upper User Event ID */
/* int   blklevel;  Block Level, ie. till blklevel entries are in the ring buffer */
{
  RINGBLK_ID pBlkRng;
  char tmpstr[80];

  pBlkRng = (RINGBLK_ID) malloc(sizeof(RING_BLKING));  /* create structure */
  if (pBlkRng == NULL) 
     return (NULL);

  memset(pBlkRng,0,sizeof(RING_BLKING));

  if (idstr == NULL) 
  {
     sprintf(tmpstr,"%s %d\n",RngBlkID,++IdCnt);
     pBlkRng->pRngIdStr = (char *) malloc(strlen(tmpstr)+2);
  }
  else
  {
     pBlkRng->pRngIdStr = (char *) malloc(strlen(idstr)+2);
  }

  if (pBlkRng->pRngIdStr == NULL)
  {
     free(pBlkRng);
     return(NULL);
  }

  if (idstr == NULL) 
  {
     strcpy(pBlkRng->pRngIdStr,tmpstr);
  }
  else
  {
     strcpy(pBlkRng->pRngIdStr,idstr);
  }

  /* initialize blocking flags */
  pBlkRng->readBlocked = FALSE;
  pBlkRng->writeBlocked = FALSE;

  /* create Ring Buffer size buffer nelem + 1 */
  pBlkRng->rBuf = (long *) malloc((sizeof(long) * nelem) + (sizeof(long)) );

  if ( (pBlkRng->rBuf) == NULL)
  {
     free(pBlkRng->pRngIdStr);
     free(pBlkRng);
     return(NULL);
  }

  pBlkRng->pToBuf = pBlkRng->pFromBuf = 0;
  
  pBlkRng->maxSize = pBlkRng->bufSize = nelem + 1;

   /* ie block till one entry present means blkTilNentries == 0 */
  pBlkRng->forcedUnBlock = 0;
  pBlkRng->blkTilNentries = pBlkRng->triggerLevel = blklevel - 1 ;  /* noramlly blklevel = 1 */
  if (pBlkRng->blkTilNentries < 0 )
      pBlkRng->blkTilNentries = pBlkRng->triggerLevel = 0;

   /* create the Blocking Ring Buffer structure Mutual Exclusion semaphore */
   pBlkRng->hMutex = CreateMutex(NULL, FALSE, NULL);
   
   /* create the two synchronization events (read & write 'blocking events') */
   pBlkRng->hSyncOk2ReadEvent =  CreateEvent(NULL, TRUE, TRUE, NULL);
   pBlkRng->hSyncOk2WriteEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
  
   return( pBlkRng );
}

/**************************************************************
*
* rngBlkDelete - Delete Blocking Ring Buffer
*
*
*  Delete Blocking Ring Buffer
*  This routine deletes a blocking ring buffer 
*
* RETURNS:
* N/A
*
*		Author Greg Brissey 5/26/94
*/
void rngBlkDelete(RINGBLK_ID rngd)
/* RINGBLK_ID rngd;  blocking ring buffer to delete */
{
  free(rngd->pRngIdStr);
  free(rngd->rBuf);
  free(rngd);
}

/**************************************************************
*
* rngBlkFlush - make a blocking ring buffer empty
*
*
*  This routine initialized a blocking ring buffer to be empty.
*Any data in the buffer will be lost. If writing is blocked it
*is released.
*
* RETURNS:
* N/A
*
*	Author Greg Brissey 5/26/94
*/
int rngBlkFlush(RINGBLK_ID rngd)
/* RINGBLK_ID rngd; Blocking ring buffer to initialize */
{
  DWORD winStatus;
  
  winStatus = WaitForSingleObject(rngd->hMutex, INFINITE);
  if (winStatus == WAIT_FAILED) {
     return -1;
  }  

  rngd->pToBuf = rngd->pFromBuf = 0;

  /* just flushed the ring buffer, if write is block */
  /* release the writing because there is now room in the buffer */
  if (rngd->writeBlocked) {
     rngd->writeBlocked = FALSE;
     SetEvent(rngd->hSyncOk2WriteEvent);
  } 

  winStatus = ReleaseMutex(rngd->hMutex);
  return winStatus;
}


/**************************************************************
*
* rngBlkPut - put element into a blocking ring buffer
*
*
*  This routine copies n elements from <buffer> into blocking ring buffer
*<rngd>. The specified number of elements will be put into the ring buffer.
*If there is insuffient room the calling task will BLOCK.
*
* RETURNS:
*  The number of elements actually put into ring buffer.
*
*	Author Greg Brissey 5/26/94
*/
int rngBlkPut(RINGBLK_ID rngd,register long* buffer,register int size)
/* RINGBLK_ID rngd;	blocking ring buffer to put data into */
/* long*      buffer;   buffer to get data from */
/* int	      size;     number of elements to put */
{
   register int fromP;
   int fbytes;
   register int result,i;

   DWORD winStatus;
   winStatus = WaitForSingleObject(rngd->hMutex, INFINITE);
   if (winStatus == WAIT_FAILED) {
      return -1;
   }   

   while( (fbytes = ( 
	       ( (result = ((rngd->pFromBuf - rngd->pToBuf) - 1)) < 0) ?
                  result + rngd->bufSize : result ) ) < size)
   {
      /*
      printf("rngBlkPut: semGive OK2Read., free bytes: %d \n",fbytes);
      */

      rngd->writeBlocked = TRUE;

      if (rngd->readBlocked) {                  /* if read blocked, */
         rngd->readBlocked = FALSE;
         SetEvent(rngd->hSyncOk2ReadEvent);
      } 


      while( rngd->writeBlocked) {
         SignalObjectAndWait(rngd->hMutex, rngd->hSyncOk2WriteEvent, INFINITE, FALSE);
         winStatus = WaitForSingleObject(rngd->hMutex, INFINITE);
         if (winStatus == WAIT_FAILED) {
            return -1;
         }
      } 

   }

   for (i = 0; i < size; i++)
   {
     /* this macro inlines the code for speed */
     RNG_LONG_PUT(rngd, (buffer[i]), fromP);
   }

   /* if I just wrote something into the ring buffer & read is block */
   /* release the reading because there is now room in the buffer */

   if ( (rngd->readBlocked) && ( rngBlkNElem(rngd) > rngd->blkTilNentries) )
   {
     /* printf("rngBlkGet: OK2Write given.\n"); */
      rngd->readBlocked = FALSE;
      SetEvent(rngd->hSyncOk2ReadEvent);
   }
   ReleaseMutex(rngd->hMutex);     
   return( size );
}

/**************************************************************
*
* rngBlkGet - get elements from a blocking ring buffer 
*
*
*  This routine copies elements from blocking ring buffer <rngd> into <buffer>
*The specified number of elements will be put into the buffer.
*If there is insuffient elements in the blocking ring buffer the calling 
*task will BLOCK.
*
* NOTE:  Presently only supports size = 1
*
* RETURNS:
*  The number of elements actually put into buffer.
*
*	Author Greg Brissey 5/26/94
*/
rngBlkGet(RINGBLK_ID rngd,long* buffer,int size)
/* RINGBLK_ID rngd;	blocking ring buffer to get data from */
/* char*      buffer;   point to buffer to receive data */
/* int	      size;     number of elements to get */
{
   register int fromP;
   DWORD winStatus;
   
   winStatus = WaitForSingleObject(rngd->hMutex, INFINITE);
   if (winStatus == WAIT_FAILED) {
      return -1;
   }

   while( (RNG_LONG_GET(rngd, buffer,fromP)) == 0)
   {
      /* printf("rngBlkGet: Nothing in Ring buffer, fromP = %d\n", fromP); */

      rngd->readBlocked = TRUE;

      if (rngd->writeBlocked) {
        rngd->writeBlocked = FALSE;
        SetEvent(rngd->hSyncOk2WriteEvent);
      } 

      while( rngd->readBlocked) {
         SignalObjectAndWait(rngd->hMutex, rngd->hSyncOk2ReadEvent, INFINITE, FALSE);
         winStatus = WaitForSingleObject(rngd->hMutex, INFINITE);
         if (winStatus == WAIT_FAILED) {
            return -1;
         }
      }

   }

   /* if I just read something out of the ring buffer & write is block */
   /* release the writing because there is now room in the buffer */
   if (rngd->writeBlocked)
   {
      /* printf("rngBlkGet: OK2Write given.\n"); */
      rngd->writeBlocked = FALSE;
      SetEvent(rngd->hSyncOk2WriteEvent);
   }

   ReleaseMutex(rngd->hMutex);
   return ( 1 );   /* For now only get one item at a time. */
}

/**************************************************************
*
* rngBlkShow - show blocking ring buffer information 
*
*
*  This routine displays the information on a blocking ring buffer <rngd> 
*
* RETURNS:
*  VOID 
*
*	Author Greg Brissey 8/9/93
*/
void rngBlkShow(RINGBLK_ID rngd,int level)
/* RINGBLK_ID rngd;	blocking ring buffer to get data from */
/* int	      level;    level of information display */
{
    int used,free,total;
    int i;

   used = rngBlkNElem (rngd);
   free = rngBlkFreeElem (rngd);
   total = used + free;

   printf("Blk Ring BufferID: '%s', 0x%lx\n",rngd->pRngIdStr,rngd);
   printf("Buffer Addr: 0x%lx, Size: %d (0x%x), UnBlocking Trigger Level: %d entries\n",rngd->rBuf,
		rngd->bufSize,rngd->bufSize,rngd->triggerLevel+1);
   printf("Entries  Used: %d, Free: %d, Total: %d\n", used, free, total);

   printf("Buffer Put Index: %d (0x%x), Get Index: %d (0x%x)\n",
		rngd->pToBuf,rngd->pToBuf,rngd->pFromBuf,rngd->pFromBuf);

   printf("\nBlocking Flags Status:  ");
   printf("READ: %s, ",((rngd->readBlocked == 0) ? "READY" : "BLOCKED"));
   printf("WRITE: %s \n",((rngd->writeBlocked == 0) ? "READY" : "BLOCKED"));
   if (level > 0) 
   {
     long *buff;

     buff = (long*) rngd->rBuf;
     for(i=0; i < total + 1 ; i++)
       printf("item[%d] = %ld (0x%lx)\n",i,buff[i],buff[i]);
   }

}


/**************************************************************
*
* rngBlkFreeElem - return the number of Free Elements in the 
*		    ring buffer empty
*
*
*  This routine returns the number of free elements in the blocking ring 
*  buffer. 
*
* RETURNS:
* number of free elements
*
*	Author Greg Brissey 5/26/94
*/
int 	rngBlkFreeElem (RINGBLK_ID ringId)
{
   register result;

   return( ( (result = ((ringId->pFromBuf - ringId->pToBuf) - 1)) < 0) ? 
	   result + ringId->bufSize : result );
}

/**************************************************************
*
* rngBlkNElem - return the number of Used Elements in the 
*		    ring buffer empty
*
*
*  This routine returns the number of used elements in the blocking ring 
*buffer. 
*
* RETURNS:
* number of used elements
*
*	Author Greg Brissey 5/26/94
*/
int 	rngBlkNElem (RINGBLK_ID ringId)
{
   register result;

   return( ( (result = (ringId->pToBuf - ringId->pFromBuf)) < 0) ? 
	   result + ringId->bufSize : result );
}

/**************************************************************
*
* rngBlkIsEmpty - returns 1 if ring buffer is empty
*
*
*  This routine returns 1 if ring buffer is empty
*
* RETURNS:
*  0 or 1 
*
*	Author Greg Brissey 5/26/94
*/
int 	rngBlkIsEmpty (RINGBLK_ID ringId)
{
    return ( (ringId->pToBuf == ringId->pFromBuf) ? 1 : 0 );
}

/**************************************************************
*
* rngBlkIsFull - returns 1 if ring buffer is full
*
*
*  This routine returns 1 if ring buffer is full
*
* RETURNS:
*  0 or 1 
*
*	Author Greg Brissey 5/26/94
*/
int 	rngBlkIsFull (RINGBLK_ID ringId)
{
    register int result;

    if ( (result = ((ringId->pToBuf - ringId->pFromBuf) + 1)) == 0)
    {
	return( 1 );
    }
    else if ( result != ringId->bufSize)
    {
	return( 0 );
    }
    else
        return( 1 );
}

/**************************************************************
*
* rngBlkIsGetPended - returns 1 if reading has been pended
*
*
*  This routine returns 1 if rngBlkGet has been pended
*
* RETURNS:
*  0 or 1 
*
*	Author Greg Brissey 5/26/94
*/
int 	rngBlkIsGetPended (RINGBLK_ID ringId)
{
   return ( (ringId->readBlocked == TRUE) ? 1 : 0 );
}

/**************************************************************
*
* rngBlkIsPutPended - returns 1 if writing has been pended
*
*
*  This routine returns 1 if rngBlkPut has been pended
*
* RETURNS:
*  0 or 1 
*
*	Author Greg Brissey 5/26/94
*/
int 	rngBlkIsPutPended (RINGBLK_ID ringId)
{
   return ( (ringId->writeBlocked == TRUE) ? 1 : 0 );
}

// #define TEST_MAIN

#ifdef TEST_MAIN
DWORD WINAPI reader_routine(void *arg) {
    RINGBLK_ID pRngBufs = (RINGBLK_ID) arg;
    int cnt;
    long buffer;

    cnt = 0;
    while(1) {
        printf("get buffer: %d, ", ++cnt);
        rngBlkGet(pRngBufs, &buffer, 1);
        printf("addr: 0x%lx\n", buffer);
    }
}

main() {
    RINGBLK_ID pRngBufs;
    HANDLE     rdThread;
    DWORD      rdThreadId;
    int        i;
    DWORD      status;
    long       bufcnt = 0;
    
    /* create buffer object */
    pRngBufs = rngBlkCreate(10, "test rng buffer", 1);

    rngBlkShow(pRngBufs, 1);
    
    for(i=0; i< 9; i++) {
        printf("put in buf: %d\n",++bufcnt);
        rngBlkPut(pRngBufs,&bufcnt,1);
    }
    
    rngBlkShow(pRngBufs, 1);
    
    rdThread = CreateThread(NULL, 0, reader_routine, (void*) pRngBufs, 0, &rdThreadId);
    
    if (rdThread == NULL) {
        printf("pthread_create error\n");
        exit(-1);
    }
   
    for(i=0; i< 9; i++) {
        printf("put in buf: %d\n", ++bufcnt);
        rngBlkPut(pRngBufs,&bufcnt,1);
    } 

    while(1) {
        Sleep(2000);
        printf("put in buf: %d\n", ++bufcnt);
        rngBlkPut(pRngBufs,&bufcnt, 1);
    }
}
#endif //TEST_MAIN

//#if defined(_WIN32)
//    #undef _WIN32
// #endif //VNMRS_WIN32




#endif   /* VNMRS_WIN32 */
