/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* rngXBlkLib.c  11.1 07/09/07 - Blocking Ring Buffer Object Source Modules */
/* 
 */


#ifndef ALLREADY_POSIX
 #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#endif
#include <vxWorks.h>
#include <rngLib.h>
#include <semLib.h>
#include <string.h>
#include "instrWvDefines.h"
#include "rngXBlkLib.h"

/*
modification history
--------------------
5-26-94,gmb  created 
*/

/*
DESCRIPTION

X Blocking Ring Buffer Library, uses the semaphore library
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
*  rngXBlkCreate - Creates an empty blocking ring buffer 
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
RINGXBLK_ID rngXBlkCreate(int nelem,char* idstr, int wvEventid, int blklevel)
/* int nelem;  size in elements of ring buffer */
/* char* idstr;  Optional identifier string */
/* int   wvEventID;  Upper User Event ID */
/* int   blklevel;  Block Level, ie. till blklevel entries are in the ring buffer */
{
  RINGXBLK_ID pBlkRng;
  char tmpstr[80];

  pBlkRng = (RINGXBLK_ID) malloc(sizeof(RINGX_BLKING));  /* create structure */
  if (pBlkRng == NULL) 
     return (NULL);

  memset(pBlkRng,0,sizeof(RINGX_BLKING));

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
  pBlkRng->wvEventID = wvEventid;


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
  pBlkRng->blkTilNentries = pBlkRng->triggerLevel = blklevel - 1 ;
  if (pBlkRng->blkTilNentries < 0 )
      pBlkRng->blkTilNentries = pBlkRng->triggerLevel = 0;
  
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
     semDelete(pBlkRng->pToSyncOK2Write);
     semDelete(pBlkRng->pToSyncOK2Read);
     semDelete(pBlkRng->pToRngBlkMutex);
     free(pBlkRng->pRngIdStr);
     free(pBlkRng->rBuf);
     free(pBlkRng);
     return (NULL);
   }
   
   return( pBlkRng );
}

/**************************************************************
*
* rngXBlkDelete - Delete Blocking Ring Buffer
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
VOID rngXBlkDelete(register RINGXBLK_ID rngd)
/* RINGXBLK_ID rngd;  blocking ring buffer to delete */
{
  semDelete(rngd->pToSyncOK2Write);
  semDelete(rngd->pToSyncOK2Read);
  semDelete(rngd->pToRngBlkMutex);
  free(rngd->pRngIdStr);
  free(rngd->rBuf);
  free(rngd);
}

/**************************************************************
*
* rngXBlkReSize - resize  the blocking ring buffer 
*
*
*  This routine resizes the blocking ring buffer. The number
*new number of elements can be no larger than at the ring
*buffer`s creation time.
*  Any data in the buffer will be lost. If writing is blocked it
*is released.
*
* RETURNS:
* 0 - OK, or -1 if newSize exceeds maximum number of elements
*		size is reverted to the maximum
*
*	Author Greg Brissey 11/18/94
*/
int  rngXBlkReSize(RINGXBLK_ID rngd, int newSize)
/* RINGXBLK_ID rngd; Blocking ring buffer to initialize */
/* int      newSize; New Number of elements in ring buffer */
{
  int stat = 0;
  int npend,pAry[4];

  newSize++;  /* size = nelem + 1 */
  if (newSize > rngd->maxSize)
  {
     newSize = rngd->maxSize;
     stat = -1;
  }

  semTake(rngd->pToRngBlkMutex, WAIT_FOREVER );

  rngd->pToBuf = rngd->pFromBuf = 0;
  rngd->bufSize = newSize;


  /* just flushed the ring buffer, if write is block */
  /* release the writing because there is now room in the buffer */
  if (rngd->writeBlocked)
  {
	/* maybe STATUS taskLock() & STATUS taskUnlock() should be used ? */
     /*
      printf("rngBlkGet: OK2Write given.\n");
     */
      rngd->writeBlocked = FALSE;

      npend = semInfo(rngd->pToSyncOK2Write,pAry,4);
      semGive( rngd->pToSyncOK2Write );
      if (npend > 1)
      {
	 printf("rngXBlkReSize: %d Tasks Pending\n",npend);
         semFlush(rngd->pToSyncOK2Write);
      }
  }
  semGive(rngd->pToRngBlkMutex);
}

/**************************************************************
*
* rngXBlkFlush - make a blocking ring buffer empty
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
VOID rngXBlkFlush(register RINGXBLK_ID rngd)
/* RINGXBLK_ID rngd; Blocking ring buffer to initialize */
{
  int npend,pAry[4];
  semTake(rngd->pToRngBlkMutex, WAIT_FOREVER );
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

      npend = semInfo(rngd->pToSyncOK2Write,pAry,4);
      semGive( rngd->pToSyncOK2Write );
      if (npend > 1)
      {
	 printf("rngXBlkFlush: %d Tasks Pending\n",npend);
         semFlush(rngd->pToSyncOK2Write);
      }
  }
  semGive(rngd->pToRngBlkMutex);
}


/**************************************************************
*
* rngXBlkPut - put element into a blocking ring buffer
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
int rngXBlkPut(register RINGXBLK_ID rngd,register long* buffer,register int size)
/* RINGXBLK_ID rngd;	blocking ring buffer to put data into */
/* long*      buffer;   buffer to get data from */
/* int	      size;     number of elements to put */
{
   register int fromP;
   int fbytes;
   int npend,pAry[4];
   register int result,i;

   /* while( (fbytes = rngXBlkFreeElem(rngd)) < size) */
   while( (fbytes = ( 
	       ( (result = ((rngd->pFromBuf - rngd->pToBuf) - 1)) < 0) ?
                  result + rngd->bufSize : result ) ) < size)
   {
      /*
      printf("rngXBlkPut: semGive OK2Read., free bytes: %d \n",fbytes);
      */
      semTake(rngd->pToRngBlkMutex, WAIT_FOREVER ); /* take mutex on stuct */
      rngd->writeBlocked = TRUE;
      if (rngd->readBlocked)			    /* if read blocked, */
      {
         rngd->readBlocked = FALSE;

#ifdef INSTRUMENT
     wvEvent(rngd->wvEventID + EVENT_RNGX_GETFREED,NULL,NULL);
#endif
         npend = semInfo(rngd->pToSyncOK2Read,pAry,4);
         semGive( rngd->pToSyncOK2Read );	    /* give sync sem OK 2 Read */
         if (npend > 1)
         {
	    printf("rngXBlkPut: %d Tasks Pending\n",npend);
            semFlush(rngd->pToSyncOK2Read);
         }
      }  /* we may be preempted above but mutex is inversion safe so its OK */

      semGive(rngd->pToRngBlkMutex);		    /* give back mutex on struct */

#ifdef INSTRUMENT
     wvEvent(rngd->wvEventID + EVENT_RNGX_PUTBLOCK,NULL,NULL);
#endif

      semTake( rngd->pToSyncOK2Write, WAIT_FOREVER );/* take & block since empty */
   }

   taskSafe();
   for (i = 0; i < size; i++)
   {
     /* this macro inlines the code for speed */
     RNG_LONG_PUT(rngd, (buffer[i]), fromP);
   }

   /* if I just wrote something into the ring buffer & read is block */
   /* release the reading because there is now room in the buffer */
   /* if (rngd->readBlocked) */
   semTake(rngd->pToRngBlkMutex, WAIT_FOREVER );
   if ( (rngd->readBlocked) && ( rngXBlkNElem(rngd) > rngd->blkTilNentries) )
   {
	/* maybe STATUS taskLock() & STATUS taskUnlock() should be used ? */
     /*
      printf("rngBlkGet: OK2Write given.\n");
     */
      /* semTake(rngd->pToRngBlkMutex, WAIT_FOREVER ); */
       rngd->readBlocked = FALSE;
      /* semGive(rngd->pToRngBlkMutex); */

      npend = semInfo(rngd->pToSyncOK2Read,pAry,4);

#ifdef INSTRUMENT
     wvEvent(rngd->wvEventID + EVENT_RNGX_GETFREED,NULL,NULL);
#endif
      semGive(rngd->pToRngBlkMutex);
      semGive( rngd->pToSyncOK2Read );

      if (npend > 1)
      {
         printf("rngXBlkPut: %d Tasks Pending\n",npend);
         semFlush(rngd->pToSyncOK2Read);
      }
   }
   else
   {
     semGive(rngd->pToRngBlkMutex);
   }
   taskUnsafe();
   return( size );
}

/**************************************************************
*
* rngXBlkGet - get elements from a blocking ring buffer 
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
rngXBlkGet(register RINGXBLK_ID rngd,long* buffer,int size)
/* RINGXBLK_ID rngd;	blocking ring buffer to get data from */
/* char*      buffer;   point to buffer to receive data */
/* int	      size;     number of elements to get */
{
   register int fromP;
   int bytes;
   int npend,pAry[4];

   /* while( (bytes = rngBufGet(rngd->pToRngBuf, buffer,size)) == 0) */
   while( (RNG_LONG_GET(rngd, buffer,fromP)) == 0)
   {
    /*
      printf("rngXBlkGet: Nothing in Ring buffer, fromP = %d\n", fromP);
    */
      semTake(rngd->pToRngBlkMutex, WAIT_FOREVER );
      rngd->readBlocked = TRUE;
      if (rngd->writeBlocked)
      {
         rngd->writeBlocked = FALSE;

         npend = semInfo(rngd->pToSyncOK2Write,pAry,4);
#ifdef INSTRUMENT
     wvEvent(rngd->wvEventID + EVENT_RNGX_PUTFREED,NULL,NULL);
#endif
         semGive(rngd->pToRngBlkMutex);
         semGive( rngd->pToSyncOK2Write );
         if (npend > 1)
         {
	    printf("rngXBlkGet: %d Tasks Pending\n",npend);
            semFlush(rngd->pToSyncOK2Write);
         }
      }  /* we may be preempted above but mutex is inversion safe so its OK */
      else
      {
        semGive(rngd->pToRngBlkMutex);
      }
#ifdef INSTRUMENT
     wvEvent(rngd->wvEventID + EVENT_RNGX_GETBLOCK,NULL,NULL);
#endif
      semTake( rngd->pToSyncOK2Read, WAIT_FOREVER);
   }

   taskSafe();
   /* if I just read something out of the ring buffer & write is block */
   /* release the writing because there is now room in the buffer */
   semTake(rngd->pToRngBlkMutex, WAIT_FOREVER );
   if (rngd->writeBlocked)
   {
	/* maybe STATUS taskLock() & STATUS taskUnlock() should be used ? */
     /*
      printf("rngXBlkGet: OK2Write given.\n");
     */
      /* semTake(rngd->pToRngBlkMutex, WAIT_FOREVER ); */
       rngd->writeBlocked = FALSE;
      /* semGive(rngd->pToRngBlkMutex); */

      npend = semInfo(rngd->pToSyncOK2Write,pAry,4);

#ifdef INSTRUMENT
     wvEvent(rngd->wvEventID + EVENT_RNGX_PUTFREED,NULL,NULL);
#endif
      semGive(rngd->pToRngBlkMutex);
      semGive( rngd->pToSyncOK2Write );

      if (npend > 1)
      {
	 printf("rngXBlkGet: %d Tasks Pending\n",npend);
         semFlush(rngd->pToSyncOK2Write);
      }
   }
   else
   {
      semGive(rngd->pToRngBlkMutex);
   }
   taskUnsafe();
   return ( 1 );   /* For now only get one item at a time. */
}

/**************************************************************
*
* rngXBlkShow - show blocking ring buffer information 
*
*
*  This routine displays the information on a blocking ring buffer <rngd> 
*
* RETURNS:
*  VOID 
*
*	Author Greg Brissey 8/9/93
*/
VOID rngXBlkShow(register RINGXBLK_ID rngd,int level)
/* RINGXBLK_ID rngd;	blocking ring buffer to get data from */
/* int	      level;    level of information display */
{
    int used,free,total;
    int npend,pAry[4];
    int i;

   used = rngXBlkNElem (rngd);
   free = rngXBlkFreeElem (rngd);
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
   printSemInfo(rngd->pToSyncOK2Read,"READ Semaphore",1);
   printSemInfo(rngd->pToSyncOK2Write,"WRITE Semaphore",1);

   if (level > 0)
   {
     printf("\nREAD Blk Sem:");
     semShow(rngd->pToSyncOK2Read,1);

     printf("WRITE Blk Sem:");
     semShow(rngd->pToSyncOK2Write,1);

     printf("Blk RING  Mutex Info:");
     semShow(rngd->pToRngBlkMutex,1);
   }

   if (level > 0) 
   {
     long *buff;

     buff = (long*) rngd->rBuf;
     for(i=0; i < total + 1 ; i++)
       printf("item[%d] = %ld (0x%lx)\n",i,buff[i],buff[i]);
   }

/*
   npend = semInfo(rngd->pToSyncOK2Read,pAry,4);
   for(i=0;i < npend; i++)
   {
       printf("Task: '%s' - 0x%lx Pending on Read Semaphore\n",taskName(pAry[i]),pAry[i]);
   }
   npend = semInfo(rngd->pToSyncOK2Write,pAry,4);
   for(i=0;i < npend; i++)
   {
       printf("Task: '%s' - 0x%lx Pending on Write Semaphore\n",taskName(pAry[i]),pAry[i]);
   }
*/

}


VOID rngXBlkShwResrc(register RINGXBLK_ID rngd, int indent )
{
   int i;
   char spaces[40];

   for (i=0;i<indent;i++) spaces[i] = ' ';
   spaces[i]='\0';

   printf("\n%sBlk Ring BufferID: '%s', 0x%lx\n",spaces,rngd->pRngIdStr,rngd);
   printf("%s    Binary Sems: pToSyncOK2Write - 0x%lx\n",spaces,rngd->pToSyncOK2Write);
   printf("%s                 pToSyncOK2Read  - 0x%lx\n",spaces,rngd->pToSyncOK2Read);
   printf("%s    Mutex:       pToRngBlkMutex  - 0x%lx\n",spaces,rngd->pToRngBlkMutex);
}

/**************************************************************
*
* rngXBlkFreeElem - return the number of Free Elements in the 
*		    ring buffer empty
*
*
*  This routine returns the number of free elements in the blocking ring 
*buffer. 
*
* RETURNS:
* number of free elements
*
*	Author Greg Brissey 5/26/94
*/
int 	rngXBlkFreeElem (register RINGXBLK_ID ringId)
{
   register result;

   return( ( (result = ((ringId->pFromBuf - ringId->pToBuf) - 1)) < 0) ? 
	   result + ringId->bufSize : result );
}

/* This function (rngXBlkNElemZBlk()) is designed with the FIFO stuffing
   operation. And used solely by the FIFO stuffer
*/
/**************************************************************
*
* rngXBlkNElemZBlk - return the number of Used Elements in the 
*		    ring buffer, Blocks if number of entries below the blkTilNentries 
*	            (triggerLevel) once unblocked it sets this level to zero
*		    (i.e. block if no entries)
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
int 	rngXBlkNElemZBlk (register RINGXBLK_ID ringId, long **ptrBuf)
{
   register result;
   register int fromP;

#ifdef BLKLEVEL
   if (ringId->forcedUnBlock)
   {
	ringId->forcedUnBlock = 0;
        ( ((result = (ringId->pToBuf - ringId->pFromBuf)) < 0) ? (result += ringId->bufSize) : result );
   }
   else
   {
#endif

   /* while(  ( ((result = (ringId->pToBuf - ringId->pFromBuf)) < 0) ? (result += ringId->bufSize) : result ) == 0) */
   while(  ( ((result = (ringId->pToBuf - ringId->pFromBuf)) < 0) ? (result += ringId->bufSize) : result ) <=  
		ringId->blkTilNentries)	/* standard operation if blkTilNentries == 0 */
   {
      semTake(ringId->pToRngBlkMutex, WAIT_FOREVER );
      ringId->readBlocked = TRUE;

#ifdef BLKLEVEL
      ringId->blkTilNentries = ringId->triggerLevel;	/* reset trigger level, if nothing left in buffer  */
#endif

      if (ringId->writeBlocked)
      {
         ringId->writeBlocked = FALSE;

#ifdef INSTRUMENT
         wvEvent(ringId->wvEventID + EVENT_RNGX_PUTFREED,NULL,NULL);
#endif
         semGive(ringId->pToRngBlkMutex);
         semGive( ringId->pToSyncOK2Write );
      }  /* we may be preempted above but mutex is inversion safe so its OK */
      else
      {
         semGive(ringId->pToRngBlkMutex);
      }

#ifdef INSTRUMENT
      wvEvent(ringId->wvEventID + EVENT_RNGX_GETBLOCK,NULL,NULL);
#endif
      semTake( ringId->pToSyncOK2Read, WAIT_FOREVER);
      if (ringId->forcedUnBlock)
      {
	ringId->forcedUnBlock = 0;
        ( ((result = (ringId->pToBuf - ringId->pFromBuf)) < 0) ? (result += ringId->bufSize) : result );
	break;
      }
   }
#ifdef BLKLEVEL
   }
   /* ringId->blkTilNentries = 0;	/* unblocked, so now set trigger level to zero to let stuffer do its job */
#else
   ringId->blkTilNentries = 0;	/* unblocked, so now set trigger level to zero to let stuffer do its job */
#endif
   fromP = (ringId)->pFromBuf;
   *ptrBuf = &((ringId)->rBuf[fromP]);  /* address of word */
   return( result );
}

/**************************************************************
*
* rngXBlkNElem - return the number of Used Elements in the 
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
int 	rngXBlkNElem (register RINGXBLK_ID ringId)
{
   register result;

   return( ( (result = (ringId->pToBuf - ringId->pFromBuf)) < 0) ? 
	   result + ringId->bufSize : result );
}

/**************************************************************
*
* rngXBlkIsEmpty - returns 1 if ring buffer is empty
*
*
*  This routine returns 1 if ring buffer is empty
*
* RETURNS:
*  0 or 1 
*
*	Author Greg Brissey 5/26/94
*/
BOOL 	rngXBlkIsEmpty (register RINGXBLK_ID ringId)
{
    return ( (ringId->pToBuf == ringId->pFromBuf) ? 1 : 0 );
}

/**************************************************************
*
* rngXBlkIsFull - returns 1 if ring buffer is full
*
*
*  This routine returns 1 if ring buffer is full
*
* RETURNS:
*  0 or 1 
*
*	Author Greg Brissey 5/26/94
*/
BOOL 	rngXBlkIsFull (RINGXBLK_ID ringId)
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
VOID         rngXPutAhead (RINGXBLK_ID ringId, long elem, int offset)
{
   printf("rngXPutAhead: Not implimented yet.\n");
}

/* the following functions are designed with the FIFO stuffing
   operation
*/
/**************************************************************
*
* rngXMoveAhead - moves Ring buffer pointers ahead as if those
*		  number of entries were removed 
*
*
*
* RETURNS:
*  0 or -1 
*
*	Author Greg Brissey 1/26/96
*/
int         rngXMoveAhead (RINGXBLK_ID ringId, int n)
{
   register int fromP;
   register int result;

   fromP = (ringId)->pFromBuf;
   if ((ringId)->pToBuf == fromP) /* nothing in buffer ignore move ahead */
   {
      return(-1);
   }
   result = (ringId)->pFromBuf + n;
   if ( result < (ringId)->bufSize)
   {
      (ringId)->pFromBuf = result;
   }
   else if (result == (ringId)->bufSize)
   {
      (ringId)->pFromBuf = 0;
   }
   else	/* result > (ringId)->bufSize */
   {
      (ringId)->pFromBuf = (result - (ringId)->bufSize);
   }
   taskSafe();
   /* if I just read something out of the ring buffer & write is block */
   /* release the writing because there is now room in the buffer */
   if (ringId->writeBlocked)
   {
	/* maybe STATUS taskLock() & STATUS taskUnlock() should be used ? */
     /*
      printf("rngXBlkGet: OK2Write given.\n");
     */
      semTake(ringId->pToRngBlkMutex, WAIT_FOREVER );
       ringId->writeBlocked = FALSE;
      semGive(ringId->pToRngBlkMutex);
#ifdef INSTRUMENT
     wvEvent(ringId->wvEventID + EVENT_RNGX_PUTFREED,NULL,NULL);
#endif
      semGive( ringId->pToSyncOK2Write );
   }
   taskUnsafe();
   return(0);
}

/**************************************************************
*
* rngXResetBlkLevel - reset the blocking triggerLevel back to
*		      its initial value
*
*
*
* RETURNS:
*  0
*
*	Author Greg Brissey 1/26/96
*/
int   rngXResetBlkLevel(RINGXBLK_ID rngd)
{
      /* printf("rngXResetBlkLevel: to %d\n",rngd->triggerLevel+1); */
      semTake(rngd->pToRngBlkMutex, WAIT_FOREVER );
       rngd->blkTilNentries = rngd->triggerLevel;	/* reset blocking trigger level */
      semGive(rngd->pToRngBlkMutex);
      return(0);
}

#ifdef BLKLEVEL
/* Only used in Interrupt service routines (fifoAMF) */
int   rngXResetBlkLevelISRlvl(RINGXBLK_ID rngd)
{
       rngd->blkTilNentries = rngd->triggerLevel;	/* reset blocking trigger level */
      DPRINT1(-1,"reset BlkLvl: %d\n",ringId->blkTilNentries);
      return(0);
}
#endif

/**************************************************************
*
* rngXSetBlkLevel - Set the blocking triggerLevel
*
*
*
* RETURNS:
*  0
*
*       Author Greg Brissey 2/22/96
*/
int   rngXSetBlkLevel(RINGXBLK_ID rngd,int blklevel)
{
      /* printf("rngXResetBlkLevel: to %d\n",rngd->triggerLevel+1); */
      semTake(rngd->pToRngBlkMutex, WAIT_FOREVER );
       rngd->blkTilNentries = rngd->triggerLevel = blklevel - 1;        /* Set blocking trigger level */
       if (rngd->blkTilNentries < 0 )
           rngd->blkTilNentries = rngd->triggerLevel = 0;
      semGive(rngd->pToRngBlkMutex);

      /* just changed trigger level, so should the rngbuf be unblocked ? */
      if ( (rngd->readBlocked) && ( rngXBlkNElem(rngd) > rngd->blkTilNentries) )
      {  
         semTake(rngd->pToRngBlkMutex, WAIT_FOREVER );
          rngd->readBlocked = FALSE;
         semGive(rngd->pToRngBlkMutex);
#ifdef INSTRUMENT
        wvEvent(rngd->wvEventID + EVENT_RNGX_GETFREED,NULL,NULL);
#endif
         semGive( rngd->pToSyncOK2Read );
      }  
      return(0);
}


/**************************************************************
*
* rngXUnBlock - force ring buffer to unblock even if number of
*		entries are below the trigger level and also
*		set the trigger level to 0
*
*
*
* RETURNS:
*  0
*
*	Author Greg Brissey 1/26/96
*/
int   rngXUnBlock (RINGXBLK_ID rngd)
{
   taskSafe();
   if (rngXBlkNElem(rngd) > 0) /* if nothing in buffer don't bother */
   {
      rngd->blkTilNentries = 0;	/* drop unblocking trigger level back down to zero */
      if (rngd->readBlocked)
      {
         /* printf("rngXUnBlock: UnBlocking ringbuffer of %d Entries\n",rngXBlkNElem(rngd)); */
	   /* maybe STATUS taskLock() & STATUS taskUnlock() should be used ? */
        /*
         printf("rngBlkGet: OK2Write given.\n");
        */
         semTake(rngd->pToRngBlkMutex, WAIT_FOREVER );
          rngd->readBlocked = FALSE;
         semGive(rngd->pToRngBlkMutex);
#ifdef INSTRUMENT
        wvEvent(rngd->wvEventID + EVENT_RNGX_GETFREED,NULL,NULL);
#endif
	 rngd->forcedUnBlock = 1;
         semGive( rngd->pToSyncOK2Read );
      }
   }
   taskUnsafe();
   return(0);
}

#ifdef BLKLEVEL
/* only used with interrupt service routines (fifoAMMT) */
int   rngXUnBlockISRlvl (RINGXBLK_ID rngd)
{
      rngd->blkTilNentries = 0;	/* drop unblocking trigger level back down to zero */
      if (rngd->readBlocked)
      {
          rngd->readBlocked = FALSE;
	  rngd->forcedUnBlock = 1;
#ifdef INSTRUMENT
        wvEvent(rngd->wvEventID + EVENT_RNGX_GETFREED,NULL,NULL);
#endif
	 rngd->forcedUnBlock = 1;
         semGive( rngd->pToSyncOK2Read );
      }
}
#endif
