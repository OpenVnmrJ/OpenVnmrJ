/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* rngBlkLib.c  11.1 07/09/07 - Blocking Ring Buffer Object Source Modules */
/* 
 */


#define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#include <vxWorks.h>
#include <rngLib.h>
#include <semLib.h>
#include <string.h>
#include "rngBlkLib.h"

/*
modification history
--------------------
7-20-92,gmb  created 
*/

/*
DESCRIPTION

Blocking Ring Buffer Library, uses the ring buffer and semaphore library
routines to create a blocking ring buffer.
Tasks writing or reading from this buffer can block.
Write will block if buffer is full, read will block if buffer is
empty.
For additional description see rngLib.

*/
static char *RngBlkID ="Blking Ring Buffer";
static int  IdCnt;

/**************************************************************
*
*  rngBlkCreate - Creates an empty blocking ring buffer 
*
*
*  This routine creates a blocking ring buffer of size <nbytes> 
*and initializes it. Memory for the buffer is allocated from 
*the system memory partition.
*
*
* RETURNS:
* ID of the blocking ring buffer, or NULL if memory cannot be allocated.
*
*	Author Greg Brissey 7/20/92
*/
RINGBLK_ID rngBlkCreate(int nbytes,char* idstr)
/* int nbytes;  size in bytes of ring buffer */
/* char* idstr;  Optional identifier string */
{
  RINGBLK_ID pBlkRng;
  char tmpstr[80];

  pBlkRng = (RINGBLK_ID) malloc(sizeof(RING_BLKING));  /* create structure */
  if (pBlkRng == NULL) 
     return (NULL);

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

  pBlkRng->pToRngBuf = rngCreate(nbytes);	/* create Ring Buffer */
  if (pBlkRng->pToRngBuf == NULL)
  {
     free(pBlkRng->pRngIdStr);
     free(pBlkRng);
     return (NULL);
  }

   /* create the two synchronization semaphore (read & write blocking sem) */
   pBlkRng->pToSyncOK2Write = semBCreate(SEM_Q_FIFO,SEM_EMPTY);
   pBlkRng->pToSyncOK2Read = semBCreate(SEM_Q_FIFO,SEM_EMPTY);

   /* create the Blocking Ring Buffer structure Mutual Exclusion semaphore */
   pBlkRng->pToRngBlkMutex = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE | 
					SEM_DELETE_SAFE);

   if ( (pBlkRng->pToSyncOK2Write == NULL) || 
	(pBlkRng->pToSyncOK2Read == NULL)  ||
	(pBlkRng->pToRngBlkMutex == NULL))
   {
     rngDelete(pBlkRng->pToRngBuf);
     semDelete(pBlkRng->pToSyncOK2Write);
     semDelete(pBlkRng->pToSyncOK2Read);
     semDelete(pBlkRng->pToRngBlkMutex);
     free(pBlkRng->pRngIdStr);
     free(pBlkRng);
     return (NULL);
   }
   
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
*		Author Greg Brissey 7/20/92
*/
VOID rngBlkDelete(RINGBLK_ID rngd)
/* RINGBLK_ID rngd;  blocking ring buffer to delete */
{
  rngDelete(rngd->pToRngBuf);
  semDelete(rngd->pToSyncOK2Write);
  semDelete(rngd->pToSyncOK2Read);
  semDelete(rngd->pToRngBlkMutex);
  free(rngd->pRngIdStr);
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
*	Author Greg Brissey 7/20/92
*/
VOID rngBlkFlush(RINGBLK_ID rngd)
/* RINGBLK_ID rngd; Blocking ring buffer to initialize */
{
  rngFlush(rngd->pToRngBuf);
  /* just flushedthe ring buffer, if write is block */
  /* release the writing because there is now room in the buffer */
  if (rngd->writeBlocked)
  {
	/* maybe STATUS taskLock() & STATUS taskUnlock() should be used ? */
     /*
      printf("rngBlkGet: OK2Write given.\n");
     */
      semTake(rngd->pToRngBlkMutex, WAIT_FOREVER );
       rngd->writeBlocked = FALSE;
      semGive(rngd->pToRngBlkMutex);
      semGive( rngd->pToSyncOK2Write );
  }
}


/**************************************************************
*
* rngBlkPut - put bytes into a blocking ring buffer
*
*
*  This routine copies bytes from <buffer> into blocking ring buffer
*<rngd>. The specified number of bytes will be put into the ring buffer.
*If there is insuffient room the calling task will BLOCK.
*
* RETURNS:
*  The number of bytes actually put into ring buffer.
*
*	Author Greg Brissey 7/20/92
*/
int rngBlkPut(RINGBLK_ID rngd,char *buffer,int size)
/* RINGBLK_ID rngd;	blocking ring buffer to put data into */
/* char*      buffer;   buffer to get data from */
/* int	      size;     number of bytes to put */
{
   int fbytes, bytes;
   while( (fbytes = rngFreeBytes(rngd->pToRngBuf)) < size)
   {
      /*
      printf("rngBlkPut: semGive OK2Read., free bytes: %d \n",fbytes);
      */
      semTake(rngd->pToRngBlkMutex, WAIT_FOREVER ); /* take mutex on stuct */
       rngd->writeBlocked = TRUE;
      if (rngd->readBlocked)			    /* if read blocked, */
      {
         rngd->readBlocked = FALSE;
         semGive( rngd->pToSyncOK2Read );	    /* give sync sem OK 2 Read */
      }  /* we may be preempted above but mutex is inversion safe so its OK */

      semGive(rngd->pToRngBlkMutex);		    /* give back mutex on struct */

      semTake( rngd->pToSyncOK2Write, WAIT_FOREVER );/* take & block since empty */
   }

   bytes = rngBufPut(rngd->pToRngBuf, buffer,size);

   /* if I just wrote something into the ring buffer & read is block */
   /* release the reading because there is now room in the buffer */
   if (rngd->readBlocked)
   {
	/* maybe STATUS taskLock() & STATUS taskUnlock() should be used ? */
     /*
      printf("rngBlkGet: OK2Write given.\n");
     */
      semTake(rngd->pToRngBlkMutex, WAIT_FOREVER );
       rngd->readBlocked = FALSE;
      semGive(rngd->pToRngBlkMutex);
      semGive( rngd->pToSyncOK2Read );
   }
   /*
   fbytes = rngFreeBytes(rngd->pToRngBuf);
   printf("tAI: rngBufPut, byteson: %d, free bytes: %d \n",bytes,fbytes);
   */
   return( bytes );
}

/**************************************************************
*
* rngBlkGet - get bytes from a blocking ring buffer 
*
*
*  This routine copies bytes from blocking ring buffer <rngd> into <buffer>
*The specified number of bytes will be put into the buffer.
*If there is insuffient bytes in the blocking ring buffer the calling 
*task will BLOCK.
*
* RETURNS:
*  The number of bytes actually put into buffer.
*
*	Author Greg Brissey 7/20/92
*/
rngBlkGet(RINGBLK_ID rngd,char* buffer,int size)
/* RINGBLK_ID rngd;	blocking ring buffer to get data from */
/* char*      buffer;   point to buffer to receive data */
/* int	      size;     number of bytes to get */
{
   int bytes;

   while( (bytes = rngBufGet(rngd->pToRngBuf, buffer,size)) == 0)
   {
    /*
      printf("rngBlkGet: Nothing in Ring buffer.\n");
    */
      semTake(rngd->pToRngBlkMutex, WAIT_FOREVER );
      rngd->readBlocked = TRUE;
      if (rngd->writeBlocked)
      {
         rngd->writeBlocked = FALSE;
         semGive( rngd->pToSyncOK2Write );
      }  /* we may be preempted above but mutex is inversion safe so its OK */
      semGive(rngd->pToRngBlkMutex);
      semTake( rngd->pToSyncOK2Read, WAIT_FOREVER);
   }

   /* if I just read something out of the ring buffer & write is block */
   /* release the writing because there is now room in the buffer */
   if (rngd->writeBlocked)
   {
	/* maybe STATUS taskLock() & STATUS taskUnlock() should be used ? */
     /*
      printf("rngBlkGet: OK2Write given.\n");
     */
      semTake(rngd->pToRngBlkMutex, WAIT_FOREVER );
       rngd->writeBlocked = FALSE;
      semGive(rngd->pToRngBlkMutex);
      semGive( rngd->pToSyncOK2Write );
   }
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
VOID rngBlkShow(RINGBLK_ID rngd,int level)
/* RINGBLK_ID rngd;	blocking ring buffer to get data from */
/* int	      level;    level of information display */
{
    int used,free,total;

   used = rngNBytes(rngd->pToRngBuf);
   free = rngFreeBytes(rngd->pToRngBuf);
   total = used + free;

   printf("Blk Ring BufferID: '%s', 0x%lx\n",rngd->pRngIdStr,rngd);
   printf("Buf Addr:  0x%lx\n",rngd->pToRngBuf);
   printf("Bytes  Used: %d, Free: %d, Total: %d\n", used, free, total);

   printf("\nBlocking Status:  ");
   printf("READ: %s, ",((rngd->readBlocked == 0) ? "READY" : "BLOCKED"));
   printf("WRITE: %s \n",((rngd->writeBlocked == 0) ? "READY" : "BLOCKED"));

   printf("\nREAD Blk Sem:");
   semShow(rngd->pToSyncOK2Read,level);

   printf("WRITE Blk Sem:");
   semShow(rngd->pToSyncOK2Write,level);

   printf("Blk RING  Mutex Info:");
   semShow(rngd->pToRngBlkMutex,level);

}
