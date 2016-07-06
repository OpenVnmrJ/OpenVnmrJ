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
/*
modification history
--------------------
3-03-2004,gmb  created , taken from namebufs.c and change to use mBufferLib
                         cluster oriented buffering.
*/

/*
DESCRIPTION

   nameClBufs provides a facility to aloocate buffers which
   may be looked up by name for inter-task communication.  
   A "Pool" of buffers is passed to nClbCreate, and
   each buffer is created by nClbMakeEntry, nClbFree releases buffers
   for reuse, nClbFindEntry looks them up, nClbDelete delete name buffer
   Object.
				Author Greg Brissey
*/

#ifndef ALLREADY_POSIX
 #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#endif
#include "instrWvDefines.h"
#include "commondefs.h"
#include "logMsgLib.h"
#include "nameClBufs.h"
#include "hostAcqStructs.h"

/**************************************************************
*
*  strhash- Creates a hash index from a string
*
*
*
* RETURNS:
* hash index
*
*       Author Greg Brissey 8/5/94
*/
static long strhash(char *keystr, long max)
/* char       *keystr - string to create hash key */
/* max 	      maximum value to return as an index */
{
   register char  *w;
   register long    key;
   register long   max_hash;

   key = 1;
   max_hash = max;
   for (w = keystr; *w; w++)
   {
      if (key == 0)
        key = 1;
      key = (key * (((int) *w % 26) + 1)) % max_hash;
   }
   /* DPRINT2(-1,"strhash: str = '%s', keyindex: %ld\n",keystr,key); */
   return (key);
}
/**************************************************************
*
*  strnhash- Creates a hash index from a string
*	assumption string in form exp#f####
*
*
*
* RETURNS:
* hash index
*
*       Author Greg Brissey 8/5/94
*/
static long strnhash(char *keystr, long max)
/* char       *keystr - string to create hash key */
/* max 	      maximum value to return as an index */
{
   register char  *w;
   register int    i;
   register long    key;
   register long   max_hash;
   int len;

   max_hash = max;
   len = strlen(keystr);
   for (i = len - 1, w = keystr + (len - 1); i >= 0; i-- )
   {
	if (*w < '0' || *w > '9')
	{
	   w++;   /* not a # so move back one */
	   break; 
	}
        w--; 
   }

   key = atol(w);
   key = key % max_hash;

   /* DPRINT2(0,"strnhash: str = '%s', keyindex: %ld\n",keystr,key); */
   return (key);
}
/**************************************************************
*
*  strnnhash- Creates a hash index from a string
*	assumption string in form exp#t#### if last
*      char is a number else it assumes it's just a string
*       and used the strhash method
*
*
* RETURNS:
* hash index
*
*       Author Greg Brissey 11/3/99
*/
static long strnnhash(char *keystr, long max)
/* char       *keystr - string to create hash key */
/* max 	      maximum value to return as an index */
{
   register char  *w;
   register int    i;
   register long    key;
   register long   max_hash;
   int len;

   max_hash = max;
   len = strlen(keystr);
   w = keystr + (len - 1);
   /* if last char not a number use the strhash */
   if (*w < '0' || *w > '9')
   {
	key = strhash(keystr, max);
   }
   else /* use the strnhash */
   {
      for (i = len - 1, w = keystr + (len - 1); i >= 0; i-- )
      {
	if (*w < '0' || *w > '9')
	{
	   w++;   /* not a # so move back one */
	   break; 
	}
        w--;  /* keep going back until a non-digit is found */
      }

      key = atol(w);
      key = key % max_hash;
      /* DPRINT2(-1,"strnhash: str = '%s', keyindex: %ld\n",keystr,key);  */
   }
   /* DPRINT2(0,"strnhash: str = '%s', keyindex: %ld\n",keystr,key); */
   return (key);
}
/*-------------------------------------------------------
| cmpstrs(word)   compare strings
|              return 1 is equal, 0 if not
+-------------------------------------------------------*/
static int cmpstrs(char *str1, char *str2)
/* char   *str1 - string 1 */
/* char   *str2 - string 2 to compare with string 1 */
{
 
   return( (strcmp(str1,str2) == 0) );
}


/**************************************************************
*
*  NCLB_ID nClbCreate - creates a pool for named buffers
*
*	argument - maximum number of buffers
*	buffers are NULL, zero length, and unnamed.
*
* RETURNS:
*	NCLB_ID - a pointer to the data structure
*
*/ 
NCLB_ID nClbCreate(MBUFFER_ID mBufferId, int numbuf)
/* MBUF_ID mBufferId - pointer to cluster based memory manager mBufferLib.c */
/* int numbuf - maximum number of named buffers */
{   
   NCLB_ID x;
   int nClbPrintEntry();

   if ((x = (NCLB_ID) malloc(sizeof(NCLB_OBJECT))) == NULL)
   {
     errLogRet(LOGIT,debugInfo,
         "nClbCreate: NCLB Object malloc failed.\n");
      return(NULL);
   }

   memset((char*)x,0,sizeof(NCLB_OBJECT));

   x->pMBufferId = mBufferId;

   /* create the Down Load Buf Mutual Exclusion semaphore */
   x->pDlbMutex = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE | 
					SEM_DELETE_SAFE);
   if (x->pDlbMutex == NULL) 
   {
     free(x);
     errLogRet(LOGIT,debugInfo,
         "nClbCreate: Mutex create failed.\n");
     return (NULL);
   }

   x->pDlbFindPendSem = semBCreate(SEM_Q_FIFO,SEM_EMPTY);
   x->findEntryPended = 0;
   x->pDlbMakePendSem = semBCreate(SEM_Q_FIFO,SEM_EMPTY);
   x->pDlbFreeBufsPendSem = semBCreate(SEM_Q_FIFO,SEM_EMPTY);

   x->pNamTbl = hashCreate(numbuf * 2, strnhash, cmpstrs, nClbFree, 
			nClbPrintEntry, "NameBuffer Hash Table",
			EVENT_NAMECLBUF_HASHLIST );

   if (x->pNamTbl == NULL)
   {
     nClbDelete(x);
     errLogRet(LOGIT,debugInfo,
         "nClbCreate: hash create failed.\n");
     return (NULL);
   }

   x->pDLBfBufId = fBufferCreate(numbuf, sizeof(NCLB), NULL,
			EVENT_NAMECLBUF_FREELIST);
   if (x->pDLBfBufId == NULL)
   {
       nClbDelete(x);
       errLogRet(LOGIT,debugInfo,
          "nClbCreate: Fast Buffer create failed.\n");
       return (NULL);
   }
   x->number = numbuf;

   return(x);
}

/**************************************************************
*
*  void nClbDelete - delete a named buffer pool 
*
*  deletes the named buffer and all of its attached entries
*
* RETURNS:
*	void
*/
void nClbDelete(NCLB_ID x)
/* NCLB_ID x - buffer to delete */
{
  if (x != NULL)
  {
    if (x->pNamTbl != NULL)
       hashDelete(x->pNamTbl);
    if (x->pDLBfBufId != NULL)
       fBufferDelete(x->pDLBfBufId);
    if (x->pDlbMutex != NULL)
       semDelete(x->pDlbMutex);
    if (x->pDlbFindPendSem != NULL)
       semDelete(x->pDlbFindPendSem);
    if (x->pDlbMakePendSem != NULL)
       semDelete(x->pDlbMakePendSem);
    if (x->pDlbFreeBufsPendSem != NULL)
       semDelete(x->pDlbFreeBufsPendSem);
    free(x);
  }
  return;
}

/*----------------------------------------------------------------------*/
/* fifoShwResrc							*/
/*     Show system resources used by Object (e.g. semaphores,etc.)	*/
/*	Useful to print then related back to WindView Events		*/
/*----------------------------------------------------------------------*/
void nClbShwResrc(NCLB_ID xx,int indent)
{
   int i;
   char spaces[40];

   for (i=0;i<indent;i++) spaces[i] = ' ';
   spaces[i]='\0';

   printf("%sName Buffer Obj: 0x%lx\n",spaces,xx);
   printf("%s   Binary Sems: pDlbFindPendSem --- 0x%lx\n",spaces,xx->pDlbFindPendSem);
   printf("%s                pDlbMakePendSem --- 0x%lx\n",spaces,xx->pDlbMakePendSem);
   printf("%s   Mutex:       pDlbMutex  -------- 0x%lx\n",spaces,xx->pDlbMutex);
   printf("\n%s   Buffer List:\n",spaces);
   fBufferShwResrc(xx->pDLBfBufId,indent+4);
   printf("\n%s   Hash Table:\n",spaces);
   hashShwResrc(xx->pNamTbl,indent+4);
}

/**************************************************************
*
*  NCLBP nClbMakeEntry - creates and names a buffer
*
*	requested space is allocated and attached to
*	a pool location and named - it's status in LOADING
*	indicating allocated but empty.
*       if an address is passed it used for the data buffer
*
* RETURNS:
*	NCLBP - pointer to the entry made - NULL is failed
*
*/ 

NCLBP nClbMakeEntry(NCLB_ID Pool,char *tag,int size, char *bufAddr)
/* Pool - name buffer Object */
/* tag - label or name to assign buffer */
/* size - size of buffer (in bytes) if it is a dynamic buffer type */
{
   int j,npend;
   unsigned long *uu;
   NCLBP entry,pEntry;
   int pAry[4];

   acqerrno = 0;

   DPRINT3(2,"nClbMakeEntry() - tag: '%s', size: %d, bufAddr: 0x%lx\n",
	tag, size, bufAddr);
   if (Pool == NULL)
   {
      errLogRet(LOGIT,debugInfo,
          "nClbMakeEntry: Was Handed a NULL NCLB_ID Pointer.\n");
     acqerrno = S_namebufs_NAMED_BUFFER_PTR_NULL;
     return(NULL);
   }

#ifdef INSTRUMENT
     wvEvent(EVENT_NAMEBUF_MAKEENTRY,NULL,NULL);
#endif

   entry = (NCLBP) fBufferGet(Pool->pDLBfBufId);/* this could block */
   entry->status = LOADING;	/* prevent buffer from being deleted prematurely */

   if ( (pEntry = (NCLBP) hashGet(Pool->pNamTbl,tag)) != NULL)
   {
      errLogRet(LOGIT,debugInfo,
          "nClbMakeEntry: WARNING Buffer Tag: %s already in use\n",tag);
      /*
      printf("Buffer Tag: %s already in use\n",tag);
      printf("fBufEntry: 0x%lx, HashEntry: 0x%lx\n",entry,pEntry);
      printf("id: '%s', HashId: '%s'\n",entry->id,pEntry->id);
      printf("data: 0x%lx, HashDat: 0x%lx\n",entry->data_array,pEntry->data_array);
      */
      fBufferReturn(Pool->pDLBfBufId,(int) entry);
      acqerrno = S_namebufs_NAMED_BUFFER_ALREADY_INUSE;
      return(NULL); /* buffer name already in use */
   }

   
   /* get cluster buffer  if buffer not passed */
   if (bufAddr == NULL)
   {
      entry->bufExtern = 0;
      uu = (unsigned long *) mBufferGet(Pool->pMBufferId, size);
      DPRINT1(2,"nClbMakeEntry() - mBuffer Cluster Addr: 0x%lx\n", uu);
      if (uu == NULL)
      {
        errLogRet(LOGIT,debugInfo,
          "nClbMakeEntry: malloc failed for data buffer.\n");
        fBufferReturn(Pool->pDLBfBufId,(int) entry);
        acqerrno = S_namebufs_NAMED_BUFFER_MALLOC_FAILED;
        return(NULL);  /* INDICATE FAILURE */
      }
    }
    else
    {
      uu = (unsigned long *) bufAddr;
      entry->bufExtern = 1;
    }

   semTake(Pool->pDlbMutex,WAIT_FOREVER);

   strcpy(entry->id,tag);
   entry->data_array = uu;
   entry->nambufptr = Pool;
   entry->data_end = uu + size;
   entry->size = size;

   hashPut(Pool->pNamTbl,entry->id,(char*) entry);

  /* If any task is pended on semaphore Give it */
  if ( ( npend = semInfo(Pool->pDlbFindPendSem,pAry,4)) > 0)
  {
#ifdef DEBUG
     if (DebugLevel > 1)
     {
       for(j=0;j < npend; j++)
       {
         DPRINT1(-1,"Task: 0x%lx Pending on MakeEntry Semaphore\n",pAry[j]);
       }
	
     }
#endif
     Pool->findEntryPended = 0;
     semGive(Pool->pDlbFindPendSem);
     if (npend > 1)
       semFlush(Pool->pDlbFindPendSem);
  }

   semGive(Pool->pDlbMutex);

   return(entry);
}

/**************************************************************
*
*  NCLBP nClbMakeEntryP - creates and names a buffer
*
*	requested space is allocated and attached to
*	a pool location and named - it's status in LOADING
*	indicating allocated but empty.
*       If the requested buffer name is already present then
*       pend till either timeout or buffer has been freed.
*       if an address is passed it used for the data buffer
*
*  Then if timeout is:
*     NO_WAIT 	    routine returns immediately
*     WAIT_FOREVER  routine pends until name buffer is not present
*     Count 	    routine pends until name buffer is not present
*			 or count expires
*
*
* RETURNS:
*	NCLBP - pointer to the entry made - NULL is failed
*
*/ 

NCLBP nClbMakeEntryP(NCLB_ID Pool,char *tag,int size,char *bufAddr, int timeout)
/* Pool - name buffer Object */
/* tag - label or name to assign buffer */
/* size - size of buffer (in bytes) if it is a dynamic buffer type */
/* timeout - pending timeout argument */
{
   int j,npend;
   unsigned long *uu;
   NCLBP entry,pEntry;
   int pAry[4];
   int timedOut;

   timedOut = acqerrno = 0;

   if (Pool == NULL)
   {
      errLogRet(LOGIT,debugInfo,
          "nClbMakeEntry: Was Handed a NULL NCLB_ID Pointer.\n");
     acqerrno = S_namebufs_NAMED_BUFFER_PTR_NULL;
     return(NULL);
   }

#    ifdef INSTRUMENT
     wvEvent(EVENT_NAMEBUF_MAKEENTRY,NULL,NULL);
#    endif

   entry = (NCLBP) fBufferGet(Pool->pDLBfBufId);/* this could block */
   entry->status = LOADING; /* prevent buffer from being deleted prematurely */

  /* Pend here if entry is already present */
  taskSafe();
  while ( ((pEntry = (NCLBP)hashGet(Pool->pNamTbl,tag)) != NULL) && 
          (timedOut != ERROR) )
  {
#    ifdef INSTRUMENT
     wvEvent(EVENT_NAMEBUF_MAKEENTRY_PEND,NULL,NULL);
#    endif
     DPRINT1(0,"nClbMakeEntryP: '%s' entry present, Pending till Deleted\n",
	tag);
     timedOut = semTake(Pool->pDlbMakePendSem,timeout);
  }
  taskUnsafe();
  if (timedOut == ERROR)
  {
     acqerrno = S_namebufs_NAMED_BUFFER_TIMED_OUT;
     return(NULL);
  }

  /* get cluster buffer  if buffer not passed */
  if (bufAddr == NULL)
  {
     /* cluster buffer */
     entry->bufExtern = 0;
     uu = (unsigned long *) mBufferGet(Pool->pMBufferId, size);
     if (uu == NULL)
     {
       errLogRet(LOGIT,debugInfo,
          "nClbMakeEntry: malloc failed for data buffer.\n");
        fBufferReturn(Pool->pDLBfBufId,(int) entry);
        acqerrno = S_namebufs_NAMED_BUFFER_MALLOC_FAILED;
        return(NULL);  /* INDICATE FAILURE */
     }
  }
  else
  {
      uu = (unsigned long *) bufAddr;
      entry->bufExtern = 1;
  }

  semTake(Pool->pDlbMutex,WAIT_FOREVER);

  strcpy(entry->id,tag);
  entry->data_array = uu;
  entry->nambufptr = Pool;
  entry->data_end = uu + size;
  entry->size = size;

  hashPut(Pool->pNamTbl,entry->id,(char*) entry);

  /* If any task is pended on semaphore Give it */
  if ( ( npend = semInfo(Pool->pDlbFindPendSem,pAry,4)) > 0)
  {
#ifdef DEBUG
     if (DebugLevel > 0)
     {
       for(j=0;j < npend; j++)
       {
         DPRINT1(-1,"Task: 0x%lx Pending on MakeEntry Semaphore\n",pAry[j]);
       }
	
     }
#endif
     Pool->findEntryPended = 0;
     semGive(Pool->pDlbFindPendSem);
     if (npend > 1)
       semFlush(Pool->pDlbFindPendSem);
  }

  semGive(Pool->pDlbMutex);

  return(entry);
}

#ifdef XXX
/**************************************************************
*
*  int nClbReuse - Resizes fix buffer size
*
*	Resizes the fixed buffer size and returns the number of
* buffers avialible at this new size.
*
* RETURNS:
*	Number of New buffers, or -1 for Error
*
*/
int nClbReuse(NCLB_ID xx, int nBuffers, int bufSize)
{
   int nbufs;
   int npend,pAry[4];

   if (xx == NULL)
   {
      errLogRet(LOGIT,debugInfo,
          "nClbReuse: Was Handed a NULL NCLB_ID Pointer.\n");
     return(-1);
   }

   /* Only works for Fixed Buffers */
   if (xx->dataBufNum > 0)
   {
      /* If any task is pended on semaphore Give it */
      if ( ( npend = semInfo(xx->pDlbFindPendSem,pAry,4)) > 0)
      {
         xx->findEntryPended = 0;
         semGive(xx->pDlbFindPendSem);
         if (npend > 1)
            semFlush(xx->pDlbFindPendSem);
      }

#ifdef XXX
      if (xx->findEntryPended)
      {
	xx->findEntryPended = 0;
	semGive(xx->pDlbFindPendSem);
      }
#endif

     nClbFreeAll(xx);
     nbufs = fBufferReUse(xx->pDLBfBufId,nBuffers,sizeof(DLB)+bufSize);
     if ( nbufs >= 1 )
     {
        xx->dataBufNum = nbufs;
        xx->dataBufSize = bufSize;
     }
     else   /* Oh Oh, fBufferReUse returned -2 */
     {
       errLogRet(LOGIT,debugInfo,
          "nClbReuse: Buffers could not be resize to %d bytes.\n",
		bufSize);
       return(nbufs);
     }
     return(xx->dataBufNum);
   }
   else
    return(xx->number);
}

#endif

/**************************************************************
*
*  int nClbFree - deletes an entry in the named buffer pool
*
*	frees memory associated with the NCLBP (entry) and
*	resets it for re-use.
*
* RETURNS:
*	0 - OK, -1 entry not found, -2 Handed NULL NCLB_ID Pointer
*
*/ 
int nClbFree(NCLBP xx)
/* NCLBP xx - pointer to element data structure*/
{
  int j,npend;
  int pAry[4];
  NCLBP pEntry;
  char *pData;
  int status;

  if (xx == NULL)
  {
    /*
      errLogRet(LOGIT,debugInfo,
          "nClbFree: Was Handed a NULL NCLB_ID Pointer.\n");
    */
      return(-2);
  }

  DPRINT1(1,"nClbFree: Free Entry '%s'\n",xx->id);
  if (xx->status == LOADING)
  {
      DPRINT1(-1,"nClbFree: '%s' - Buffer being LOADED, Deletion Prevented.\n",
		xx->id);
      return(-(LOADING));
  }
  if (xx->status == PARSING)
  {
      DPRINT1(-1,"nClbFree: WARNING: '%s' - Buffer being PARSED, Is Being Deleted.\n",xx->id);
  }

  pData = (char*) xx->data_array;

  pEntry = (NCLBP) hashGet(xx->nambufptr->pNamTbl,xx->id);
  if (pEntry == NULL)   /* not present */
  {
     DPRINT1(-1,"nClbFree: Entry '%s' Not found \n",xx->id);
     return(-1);
  }

  semTake(xx->nambufptr->pDlbMutex,WAIT_FOREVER);

  hashRemove(xx->nambufptr->pNamTbl,xx->id);

  xx->id[0] = '\0';
  xx->data_array = NULL;
  xx->data_end   = NULL;
  xx->size = 0;
  xx->status = MT;
  semGive(xx->nambufptr->pDlbMutex);

  /* if a bufAddr was given, then don't return it to the buffers */
  DPRINT1(1,"nClbFree: bufExtern %d\n",xx->bufExtern); 
  if (xx->bufExtern == 0)
  {
     if (pData != NULL)
     {
        /* DPRINT1(-1,"nClbFree: mBufferReturn() 0x%lx\n",pData); */
        status = mBufferReturn(xx->nambufptr->pMBufferId,(long) pData);
     }
  }

#ifdef INSTRUMENT
     wvEvent(EVENT_NAMEBUF_FREE,NULL,NULL);
#endif

  fBufferReturn(xx->nambufptr->pDLBfBufId,(long) pEntry);

  /* If any task is pended on semaphore Give it */
  if ( ( npend = semInfo(xx->nambufptr->pDlbMakePendSem,pAry,4)) > 0)
  {
#ifdef DEBUG
     if (DebugLevel > 0)
     {
       for(j=0;j < npend; j++)
       {
         DPRINT1(-1,"Task: 0x%lx Pending on MakeEntry Semaphore\n",pAry[j]);
       }
	
     }
#endif
     semGive(xx->nambufptr->pDlbMakePendSem);
     if (npend > 1)
        semFlush(xx->nambufptr->pDlbMakePendSem);
  }

  /* freed a buffer, so give this FreeBufs Semaphore */
  semGive(xx->nambufptr->pDlbFreeBufsPendSem);

  /* if mBufferReturn() failed then propagate error */
  if (status == -1)
  {
     return -3;
  }
  else
  {
     return(0);
  }
} 
/**************************************************************
*
*  int nClbFreeByName - deletes an entry in the named buffer pool
*
*	frees memory associated with the NCLBP (entry) and
*	resets it for re-use.
*
* RETURNS:
*	0 - OK, -1 entry not found, -2 Handed NULL NCLB_ID Pointer
*
*/ 
int nClbFreeByName(NCLB_ID nClbId, char* label)
/* NCLB_ID nClbId - name buffer Object */
/* char *label - label of name buffer to be deleted */
{
  if (nClbId == NULL)
  {
      errLogRet(LOGIT,debugInfo,
          "nClbFreeByName: Was Handed a NULL NCLB_ID Pointer.\n");
      return(-2);
  }
  DPRINT1(-1,"nClbFreeByName: '%s'\n",label);
  return( nClbFree( ((NCLBP)hashGet(nClbId->pNamTbl,label)) ) );
}


/**************************************************************
*
*  NCLBP nClbRename - Rename a named buffer pool
*
*       Renames a named buffer pool, if a buffer with this new name
*	is already present it is removed.
*
* RETURNS:
*       NULL or NCLBP pointer
*
*/
NCLBP nClbRename(NCLB_ID nClbId, char* presentlabel,char* newlabel)
/* NCLB_ID nClbId - name buffer Object */
/* char *presentlabel - label of name buffer to be renamed */
/* char *newlabel - label to replace previous name */
{
   NCLBP entry,nentry;
   NCLB  tmpdlb;

   if (nClbId == NULL)
      return(NULL);

   entry = nClbFindEntry(nClbId, presentlabel);
   if (entry == NULL)
   {
      errLogRet(LOGIT,debugInfo,
          "nClbRename: Label: %s not found in buffer pool.\n",presentlabel);
      return(NULL);
   }
   /*
   DPRINT1(-1,"old entry: '%s' --\n",presentlabel);
   nClbPrintEntry(entry);
   */

   /* make a copy of the old entry so it can be swapped with the new */
   memcpy(&tmpdlb,entry,sizeof(NCLB));

   /* nClbPrintEntry(&tmpdlb); */

   nentry = nClbFindEntry(nClbId, newlabel);
   if (nentry == NULL)
   {
      errLogRet(LOGIT,debugInfo,
          "nClbRename: Label: %s not found in buffer pool.\n",newlabel);
      return(NULL);
   }
   /*
   DPRINT1(-1,"new entry: '%s' --\n",newlabel);
   nClbPrintEntry(nentry);
   */

   /* Replace present Entry reference with the new reference (or NCLBP) */
   /* change old entry with new entry values */
   entry->size = nentry->size;
   entry->status = nentry->status;
   entry->nambufptr = nentry->nambufptr;
   entry->data_array = nentry->data_array;
   entry->data_end = nentry->data_end;

   /* change new entry with old entry values */
   nentry->size = tmpdlb.size;
   nentry->status = tmpdlb.status;
   nentry->nambufptr = tmpdlb.nambufptr;
   nentry->data_array = tmpdlb.data_array;
   nentry->data_end = tmpdlb.data_end;

   /* nClbPrintEntry(nentry); */

   /* lock down the named buffer while renaming it */
   semTake(nClbId->pDlbMutex,WAIT_FOREVER);

   nClbFree(nentry);

   semGive(nClbId->pDlbMutex);

   return( entry );
}

/**************************************************************
*
*  NCLBP nClbFindEntry()
*
*	looks up a name in the named buffer pool for use
*
* RETURNS:
*	NCLBP (entry pointer) if found, NULL otherwise
*
*/
NCLBP nClbFindEntry(NCLB_ID xx, char *label)
/* NCLB_ID xx <-data structure of the pool  char *label - name to look up */
{
  NCLBP entry;
  entry = (NCLBP)hashGet(xx->pNamTbl,label);
  if ( entry == NULL )
  {
     DPRINT1(-1,"nClbFindEntry: '%s' Not Found\n",label);
  }
  return(entry);
}

/**************************************************************
*
*  NCLBP nClbFindEntryP()  - This routine allows task to pend on name
*			    search buffer
*
*  Looks up a name in the named buffer pool for use, if not present
*  Then if timeout is:
*     NO_WAIT 		routine returns immediately
*     WAIT_FOREVER 	routine pends until name buffer is found
*     Count 		routine pends unitl name buffer is found or count expires
*
*  In addition if it find the named buffer but is marked as LOADING
*  the routine enters a spin watch checking 10times/sec until either the
*  status changes or the timeout is used up.
*
* RETURNS:
*	NCLBP (entry pointer) if found, NULL otherwise
*
*/
NCLBP nClbFindEntryP(NCLB_ID xx, char *label, int timeout)
/* NCLB_ID xx <-data structure of the pool  char *label - name to look up */
{
  NCLBP entry;
  int timedOut = 0;
  int cnt;

  taskSafe();
  while ( ((entry = (NCLBP)hashGet(xx->pNamTbl,label)) == NULL) && 
          (timedOut != ERROR) )
  {
#ifdef INSTRUMENT
     wvEvent(EVENT_NAMEBUF_FINDENTRY_PEND,NULL,NULL);
#endif
     DPRINT1(-1,"nClbFindEntryP: '%s' entry not present, Pending \n",label);
     xx->findEntryPended = 1;
     taskUnsafe();
     timedOut = semTake(xx->pDlbFindPendSem,timeout);
     taskSafe();
  }
  taskUnsafe();
  if (timedOut != ERROR)
  {
     if ( entry->status == LOADING )
     {
      DPRINT1(-1,"nClbFindEntryP: '%s' entry present, BUT being LOADED, Pending\n",label);
      switch(timeout)
      {
	case NO_WAIT:
		entry = NULL;
		break;

	case WAIT_FOREVER:
		/* check forever */
     		while((entry->status == LOADING))
     		{
			taskDelay(0);
     		} 
		break;
	default:
  		/* when we haved waited the timeout period return */
     		cnt = timeout;
     		while((entry->status == LOADING) && (cnt >= 0))
     		{
                        taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
			cnt -= 1;   /* decrement count */
     		} 
     		if (cnt < 1)	/* timed out ? */
       		  entry = NULL;	/* Yes, make entry NULL */
	        break;
     }
    }
  }

  return(entry);
}

/**************************************************************
*
*  void nClbFreeByRootName(NCLB_ID obj)
*
*	frees all entries but leaves the top level OBJ intact
*
* RETURNS:
*	nothing
*
*/
int nClbFreeByRootName(NCLB_ID xx,char *rootLabel)
/* NCLB_ID obj - handle of the named memory manager */
/* rootLabel - root name to match for deletion */
{
     NCLBP entry;
     int index,bucket,done,deleted,numdeleted;
     char *strstr(char*,char*);

     numdeleted = done = index = bucket = 0;
  taskSafe();
     while(!done)
     {
     entry = (NCLBP) -1;
      deleted = 0;
      while(entry != NULL)
      { 
         entry = (NCLBP) hashGetEntry(xx->pNamTbl,&index,&bucket);
         DPRINT3(1,"nClbFreeByRootName: entry: 0x%lx, index=%d, bucket: %d\n",entry,index,bucket);
         if (entry != NULL)
         {
           DPRINT2(1,"nClbFreeByRootName: root name: '%s', full name: '%s'\n",rootLabel,entry->id);
           if (strstr(entry->id,rootLabel) != NULL)
           {
              DPRINT1(1,"nClbFreeByRootName: delete: '%s'\n",entry->id);
              nClbFree(entry);
              numdeleted++;
              deleted = 1;
              /* index = bucket = 0; */
              /* semGive(numBufSemId); */
           } 
         }  
     }
     if (deleted)
        index = bucket = 0;
     else
        done = 1;
    }
  taskUnsafe();

  DPRINT1(1,"nClbFreeByRootName: num deleted: %d\n",numdeleted);
  return(numdeleted);
}
/**************************************************************
*
*  void nClbFreeAll(NCLB_ID obj)
*
*	frees all entries but leaves the top level OBJ intact
*
* RETURNS:
*	nothing
*
*/
void nClbFreeAll(NCLB_ID xx)
/* NCLB_ID obj - handle of the named memory manager */
{
  int i,j;
  NCLBP entry;
  
  i = j = 0;
  /* go through hash table get all entries & free space if necessary */
  /* DPRINT1(-1,"hash entries: %d\n",hashGetNumEntries(xx->pNamTbl)); */
  /* prtHashKeys(xx->pNamTbl);  print key contents of hash table */
  taskSafe();
  /* hashGetEntry had a problem,  i & j are incremented as the hash
  *  table was being traversed, but since we are deleting entries as
  *  we go, an entry could be missed since it's position within the
  *  bucket list could change. The new function always starts at the
  *  bucket entry. Thus without deletion you would always get the
  * same entry.								*/
  /* while((entry = (NCLBP) hashGetEntry(xx->pNamTbl,&i,&j)) != NULL) */
  while((entry = (NCLBP) hashGetNxtEntry(xx->pNamTbl,&i)) != NULL)
  {
    nClbFree(entry);
  }
  taskUnsafe();
  return;
}

void nClbFreeAllNotLoading(NCLB_ID xx)
/* NCLB_ID obj - handle of the named memory manager */
{
  int i,j,tot;
  NCLBP entry;
  
  i = j = 0;
  tot =  nClbUsedBufs(xx);
  /* go through hash table get all entries & free space if necessary */
  /* DPRINT1(-1,"hash entries: %d\n",hashGetNumEntries(xx->pNamTbl)); */
  /* prtHashKeys(xx->pNamTbl);  print key contents of hash table */
  taskSafe();
  /* hashGetEntry had a problem,  i & j are incremented as the hash
  *  table was being traversed, but since we are deleting entries as
  *  we go, an entry could be missed since it's position within the
  *  bucket list could change. The new function always starts at the
  *  bucket entry. Thus without deletion you would always get the
  * same entry.								*/
  /* while((entry = (NCLBP) hashGetEntry(xx->pNamTbl,&i,&j)) != NULL) */
  for (j=0; j < tot; j++)
  {
   if ((entry = (NCLBP) hashGetNxtEntry(xx->pNamTbl,&i)) != NULL)
   {
    nClbFree(entry);
   }
  }
  taskUnsafe();
  return;
}

/**************************************************************
*
* nClbFreeBufs - return # of free buffers
*
*  This routine returns the number of free (unallocated) buffers in 
* the named buffer pool.
*
* RETURNS:
* number of free buffers
*
*       Author Greg Brissey 12/5/94
*/
int  nClbFreeBufs(NCLB_ID x)
/* NCLB_ID x - Fast Buffer Pool Id */
{
   return(fBufferFreeBufs(x->pDLBfBufId));
}

/**************************************************************
*
* nClbFreeBufsP - return # of free buffers
*
*  This routine returns the number of free (unallocated) buffers in 
* the named buffer pool.  IF the number is zero then this routine will pend
*
* RETURNS:
* number of free buffers
*
*       Author Greg Brissey 12/5/94
*/
int  nClbFreeBufsP(NCLB_ID x)
/* NCLB_ID x - Fast Buffer Pool Id */
{
  int freeBufs;
  /* lock task while checking if needs to pend */
  freeBufs = fBufferFreeBufs(x->pDLBfBufId);
  while (freeBufs < 1)
  {
     /* while (semTake(x->pDlbFreeBufsPendSem,NO_WAIT) != ERROR); */
     semTake(x->pDlbFreeBufsPendSem,WAIT_FOREVER);
     freeBufs = fBufferFreeBufs(x->pDLBfBufId);
  }
  return(freeBufs);
}

/**************************************************************
*
* nClbUsedBufs - return # of used (allocated) buffers
*
*  This routine returns the number of used (allocated) buffers in 
* the named buffer pool
*
* RETURNS:
* number of used elements
*
*       Author Greg Brissey 12/5/94
*/
int     nClbUsedBufs (NCLB_ID x)
/* NCLB_ID x - Fast Buffer Pool Id */
{
   return(fBufferUsedBufs(x->pDLBfBufId));
}

/**************************************************************
*
* nClbMaxSingleBuf - return the max size of a single buffer
*
*
* RETURNS:
* max size of a single buffer 
*
*       Author Greg Brissey 5/17/95
*/
long     nClbMaxSingleBuf (NCLB_ID x)
/* NCLB_ID x - Fast Buffer Pool Id */
{
   return(mBufferMaxClSize(x->pMBufferId));
}

/**************************************************************
*
*  nClbSetReady - mark a named buffer ready for use
*
*	set the status field indicating the buffer 
*	has data.
*
* RETURNS:
*	void - nothing
*
*/ 
void nClbSetReady(NCLBP xx)
/* NCLBP xx - buffer entry to set */
{
  xx->status = READY;
}

/**************************************************************
*
*  nClbRelease(NCLBP xx)
*
*	mark the entry as re-loadable - data may be bad 
*
* RETURNS:
*	int - no meaning (yet)
*/
int nClbRelease(NCLBP xx)
/* NCLBP xx - entry to release */
{
  /* could zero memory */
  xx->status = MT;
}

/**************************************************************
*
*  void  *nClbGetPntr(NCLBP xx) - get a pointer to the allocated 
*				  memory
*
*	gets data pointer
*
* RETURNS:
*	void * -- pointer to memory or NULL if failed
*
*/
void  *nClbGetPntr(NCLBP xx)
{
  return(xx->data_array);
}

/**************************************************************
*
*  int nClbPrintEntry(NCLBP entry)
*
*	prints data about the pool
*
* RETURNS:
*	nothing
*
*/
int nClbPrintEntry(NCLBP xx)
/* NCLB_ID obj - handle of the named memory manager */
{
  printf("Label: '%s' Data Buf: 0x%lx\n",xx->id,xx->data_array);
  return(0);
}

/**************************************************************
*
*  void nClbPrintBuffers(NCLB_ID obj)
*
*	prints data about the pool
*
* RETURNS:
*	nothing
*
* status 
*   MT	    (1)
*   READY   (2)
*   DONE    (4)
*   LOADING (8)
*   PARSING (16)
*
*/
void nClbPrintBuffers(NCLB_ID xx)
/* NCLB_ID obj - handle of the named memory manager */
{
  int i,j;
  NCLBP entry;
  char *statstr;
  
  i = j = 0;
  printf("Namebuffer Object: Entries Present: %d, Max: %d\n",
	hashGetNumEntries(xx->pNamTbl),xx->number);

  while((entry = (NCLBP) hashGetEntry(xx->pNamTbl,&i,&j)) != NULL)
  {
    switch(entry->status)
    {
       case MT:       statstr = "Empty"; break;
       case READY:    statstr = "Ready"; break;
       case DONE:     statstr = "Done"; break;
       case LOADING:  statstr = "Loading"; break;
       case PARSING:  statstr = "Parsing"; break;
       default:       statstr = "Unknown"; break;
    }
    printf("Entry[%d,%d]: '%s', Addr: 0x%lx; Data Addr: 0x%lx,  Bufsize %d,  status: '%s' (%d)\n",
	i,j-1,entry->id,entry,entry->data_array,entry->size,statstr,entry->status);
  }
}

/**************************************************************
*
*  void nClbShow(NCLB_ID obj)
*
*	prints data about the Down Load Buffer Object 
*
* RETURNS:
*	nothing
*
*/
void nClbShow(NCLB_ID xx,int level)
{
  int nentries = hashGetNumEntries(xx->pNamTbl);
  printf("  Cluster Based Buffer Type\n");
  printf("\nNamebuffer Object: Id - 0x%lx, Entries Present: %d, Max: %d\n",
	xx,nentries,xx->number);
  if ( nentries > 0)
    nClbPrintBuffers(xx);

  printf("\nTasks Pending Flag(0x%lx): %s \n", &(xx->findEntryPended),( (xx->findEntryPended == 1) ? "TRUE" : "FALSE" ));
  printf("Name Buffer Blocking Semaphore [nClbMakeEntryP()/nClbFindEntryP()]: Id - 0x%lx\n",xx->pDlbFindPendSem);
  printSemInfo(xx->pDlbFindPendSem,"Name Buffer Semaphore",1);

  if (level > 0)
  {
    printf("\nName Buffer: Hash Table:\nvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n\n");
    hashShow(xx->pNamTbl,level-1);
    printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");

    printf("\nName Buffer: Fast Buffer List:\nvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n\n");
    fBufferShow(xx->pDLBfBufId ,level-1);
    printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");

    printf("\nName Buffer: M Buffer List:\nvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n\n");
    mBufferShow(xx->pMBufferId ,level-1);
    printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
  }

  return;
}
