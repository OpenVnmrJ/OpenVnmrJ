/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* namebufs.c 11.1 07/09/07 - Down Load buffer modules */
/* 
 */

/*
modification history
--------------------
8-10-94,ph  created 
11-10-94,gmb  extensive modification to handle both dynamic & fix style
	      buffering. 
5-31-95,gmb  added task pending functionality
*/

/*
DESCRIPTION

   namedbufs provides a facility to malloc/free buffers which
   may be looked up by name for inter-task communication.  
   A "Pool" of buffers is created by dlbCreate, and
   each buffer is created by dlbMakeEntry, dlbFree releases buffers
   for reuse, dlbFindEntry looks them up, dlbDelete delete name buffer
   Object.
				Author Phil Hornung
				Modified: Greg Brissey  11-10-94
*/

#define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#include "instrWvDefines.h"
#include "commondefs.h"
#include "logMsgLib.h"
#include "namebufs.h"
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
*  DLB_ID dlbCreate - creates a pool for named buffers
*
*	argument - maximum number of buffers
*	buffers are NULL, zero length, and unnamed.
*
* RETURNS:
*	DLB_ID - a pointer to the data structure
*
*/ 
DLB_ID dlbCreate(int buftype, int numbuf, int bufsize)
/* int buftype - malloc or fast buffer */
/* int numbuf - maximum number of named buffers */
/* int bufsize - fast buffer size */
{   
   DLB_ID x;
   int dlbPrintEntry();

   if ((x = (DLB_ID) malloc(sizeof(DLB_OBJECT))) == NULL)
   {
     errLogSysRet(LOGIT,debugInfo,
         "dlbCreate: DLB Object malloc failed.\n");
      return(NULL);
   }

   memset((char*)x,0,sizeof(DLB_OBJECT));

   /* create the Down Load Buf Mutual Exclusion semaphore */
   x->pDlbMutex = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE | 
					SEM_DELETE_SAFE);
   if (x->pDlbMutex == NULL) 
   {
     free(x);
     errLogSysRet(LOGIT,debugInfo,
         "dlbCreate: Mutex create failed.\n");
     return (NULL);
   }

   x->pDlbFindPendSem = semBCreate(SEM_Q_FIFO,SEM_EMPTY);
   x->findEntryPended = 0;
   x->pDlbMakePendSem = semBCreate(SEM_Q_FIFO,SEM_EMPTY);

   if (buftype == FIXED_FAST_BUF)
   {
      x->pNamTbl = hashCreate(numbuf * 2, strnhash, cmpstrs, dlbFree, 
			dlbPrintEntry, "NameBuffer Hash Table",
			EVENT_NAMEBUF_FIXHASHLIST );
   }
   else
   {
      x->pNamTbl = hashCreate(numbuf * 2, strnnhash, cmpstrs, dlbFree, 
			dlbPrintEntry, "NameBuffer Hash Table",
			EVENT_NAMEBUF_DYNHASHLIST );
   }
   if (x->pNamTbl == NULL)
   {
     dlbDelete(x);
     errLogSysRet(LOGIT,debugInfo,
         "dlbCreate: hash create failed.\n");
     return (NULL);
   }

   if (buftype == FIXED_FAST_BUF)
   {
     x->pDLBfBufId = fBufferCreate(numbuf, sizeof(DLB) + bufsize, NULL,
			EVENT_NAMEBUF_FIXFREELIST);
     if (x->pDLBfBufId == NULL)
     {
       dlbDelete(x);
       errLogSysRet(LOGIT,debugInfo,
          "dlbCreate: Fast Buffer create failed.\n");
       return (NULL);
     }
     x->dataBufNum = numbuf;
     x->dataBufSize = bufsize;
   }
   else
   {
     x->pDLBfBufId = fBufferCreate(numbuf, sizeof(DLB), NULL,
			EVENT_NAMEBUF_DYNFREELIST);
     if (x->pDLBfBufId == NULL)
     {
       dlbDelete(x);
       errLogSysRet(LOGIT,debugInfo,
          "dlbCreate: Fast Buffer create failed.\n");
       return (NULL);
     }
   }

   x->number = numbuf;

   return(x);
}

/**************************************************************
*
*  void dlbDelete - delete a named buffer pool 
*
*  deletes the named buffer and all of its attached entries
*
* RETURNS:
*	void
*/
void dlbDelete(DLB_ID x)
/* DLB_ID x - buffer to delete */
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
    free(x);
  }
  return;
}

/*----------------------------------------------------------------------*/
/* fifoShwResrc							*/
/*     Show system resources used by Object (e.g. semaphores,etc.)	*/
/*	Useful to print then related back to WindView Events		*/
/*----------------------------------------------------------------------*/
void dlbShwResrc(DLB_ID xx,int indent)
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
*  DLBP dlbMakeEntry - creates and names a buffer
*
*	requested space is allocated and attached to
*	a pool location and named - it's status in LOADING
*	indicating allocated but empty.
*
* RETURNS:
*	DLBP - pointer to the entry made - NULL is failed
*
*/ 

DLBP dlbMakeEntry(DLB_ID Pool,char *tag,int size)
/* Pool - name buffer Object */
/* tag - label or name to assign buffer */
/* size - size of buffer (in bytes) if it is a dynamic buffer type */
{
   int j,npend;
   unsigned long *uu;
   DLBP entry,pEntry;
   int pAry[4];

   acqerrno = 0;

   if (Pool == NULL)
   {
      errLogRet(LOGIT,debugInfo,
          "dlbMakeEntry: Was Handed a NULL DLB_ID Pointer.\n");
     acqerrno = S_namebufs_NAMED_BUFFER_PTR_NULL;
     return(NULL);
   }

#ifdef INSTRUMENT
     wvEvent(EVENT_NAMEBUF_MAKEENTRY,NULL,NULL);
#endif
   entry = (DLBP) fBufferGet(Pool->pDLBfBufId);/* this could block */
   entry->status = LOADING;	/* prevent buffer from being deleted prematurely */

   if ( (pEntry = (DLBP) hashGet(Pool->pNamTbl,tag)) != NULL)
   {
      errLogRet(LOGIT,debugInfo,
          "dlbMakeEntry: WARNING Buffer Tag: %s already in use\n",tag);
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

   /* decide if it's a Dynamic or Fix Buffer */
   if (Pool->dataBufNum > 0)
   {
      /* Fixed, data space is just behind this DLB struct */
      uu = (unsigned long *) ((char*)entry + sizeof(DLB)); 
   }
   else
   {
      /* dynamic, malloc space */
      uu = (unsigned long *) malloc(size);
      if (uu == NULL)
      {
        errLogSysRet(LOGIT,debugInfo,
          "dlbMakeEntry: malloc failed for data buffer.\n");
        fBufferReturn(Pool->pDLBfBufId,(int) entry);
        acqerrno = S_namebufs_NAMED_BUFFER_MALLOC_FAILED;
        return(NULL);  /* INDICATE FAILURE */
      }
   }

   semTake(Pool->pDlbMutex,WAIT_FOREVER);

   strcpy(entry->id,tag);
   entry->data_array = uu;
   entry->nambufptr = Pool;

   if (Pool->dataBufNum > 0)
   {
      entry->data_end = uu + Pool->dataBufSize;
      entry->size = Pool->dataBufSize;
   }
   else
   {
      entry->data_end = uu + size;
      entry->size = size;
   }


   hashPut(Pool->pNamTbl,entry->id,(char*) entry);

  /* If any task is pended on semaphore Give it */
  if ( ( npend = semInfo(Pool->pDlbFindPendSem,pAry,4)) > 0)
  {
#ifdef DEBUG
     if (DebugLevel > 1)
     {
       for(j=0;j < npend; j++)
       {
         DPRINT1(1,"Task: 0x%lx Pending on MakeEntry Semaphore\n",pAry[j]);
       }
	
     }
#endif
     Pool->findEntryPended = 0;
     semGive(Pool->pDlbFindPendSem);
     if (npend > 1)
       semFlush(Pool->pDlbFindPendSem);
  }
#ifdef XXX
   if (Pool->findEntryPended)
   {
	Pool->findEntryPended = 0;
	semGive(Pool->pDlbFindPendSem);
   }
#endif

   semGive(Pool->pDlbMutex);

   return(entry);
}

/**************************************************************
*
*  DLBP dlbMakeEntryP - creates and names a buffer
*
*	requested space is allocated and attached to
*	a pool location and named - it's status in LOADING
*	indicating allocated but empty.
*       If the requested buffer name is already present then
*       pend till either timeout or buffer has been freed.
*
*  Then if timeout is:
*     NO_WAIT 	    routine returns immediately
*     WAIT_FOREVER  routine pends until name buffer is not present
*     Count 	    routine pends until name buffer is not present
*			 or count expires
*
*
* RETURNS:
*	DLBP - pointer to the entry made - NULL is failed
*
*/ 

DLBP dlbMakeEntryP(DLB_ID Pool,char *tag,int size,int timeout)
/* Pool - name buffer Object */
/* tag - label or name to assign buffer */
/* size - size of buffer (in bytes) if it is a dynamic buffer type */
/* timeout - pending timeout argument */
{
   int j,npend;
   unsigned long *uu;
   DLBP entry,pEntry;
   int pAry[4];
   int timedOut;

   timedOut = acqerrno = 0;

   if (Pool == NULL)
   {
      errLogRet(LOGIT,debugInfo,
          "dlbMakeEntry: Was Handed a NULL DLB_ID Pointer.\n");
     acqerrno = S_namebufs_NAMED_BUFFER_PTR_NULL;
     return(NULL);
   }

#    ifdef INSTRUMENT
     wvEvent(EVENT_NAMEBUF_MAKEENTRY,NULL,NULL);
#    endif

   entry = (DLBP) fBufferGet(Pool->pDLBfBufId);/* this could block */
   entry->status = LOADING; /* prevent buffer from being deleted prematurely */

  /* Pend here if entry is already present */
  taskSafe();
  while ( ((pEntry = (DLBP)hashGet(Pool->pNamTbl,tag)) != NULL) && 
          (timedOut != ERROR) )
  {
#    ifdef INSTRUMENT
     wvEvent(EVENT_NAMEBUF_MAKEENTRY_PEND,NULL,NULL);
#    endif
     DPRINT1(0,"dlbMakeEntryP: '%s' entry present, Pending till Deleted\n",
	tag);
     timedOut = semTake(Pool->pDlbMakePendSem,timeout);
  }
  taskUnsafe();
  if (timedOut == ERROR)
  {
     acqerrno = S_namebufs_NAMED_BUFFER_TIMED_OUT;
     return(NULL);
  }

  /* decide if it's a Dynamic or Fix Buffer */
  if (Pool->dataBufNum > 0)
  {
      /* Fixed, data space is just behind this DLB struct */
      uu = (unsigned long *) ((char*)entry + sizeof(DLB)); 
  }
  else
  {
     /* dynamic, malloc space */
     uu = (unsigned long *) malloc(size);
     if (uu == NULL)
     {
       errLogSysRet(LOGIT,debugInfo,
          "dlbMakeEntry: malloc failed for data buffer.\n");
        fBufferReturn(Pool->pDLBfBufId,(int) entry);
        acqerrno = S_namebufs_NAMED_BUFFER_MALLOC_FAILED;
        return(NULL);  /* INDICATE FAILURE */
     }
  }

  semTake(Pool->pDlbMutex,WAIT_FOREVER);

  strcpy(entry->id,tag);
  entry->data_array = uu;
  entry->nambufptr = Pool;

  if (Pool->dataBufNum > 0)
  {
     entry->data_end = uu + Pool->dataBufSize;
     entry->size = Pool->dataBufSize;
  }
  else
  {
     entry->data_end = uu + size;
     entry->size = size;
  }


  hashPut(Pool->pNamTbl,entry->id,(char*) entry);

  /* If any task is pended on semaphore Give it */
  if ( ( npend = semInfo(Pool->pDlbFindPendSem,pAry,4)) > 0)
  {
#ifdef DEBUG
     if (DebugLevel > 0)
     {
       for(j=0;j < npend; j++)
       {
         DPRINT1(1,"Task: 0x%lx Pending on MakeEntry Semaphore\n",pAry[j]);
       }
	
     }
#endif
     Pool->findEntryPended = 0;
     semGive(Pool->pDlbFindPendSem);
     if (npend > 1)
       semFlush(Pool->pDlbFindPendSem);
  }
#ifdef XXX
  if (Pool->findEntryPended)
  {
	Pool->findEntryPended = 0;
	semGive(Pool->pDlbFindPendSem);
  }
#endif

  semGive(Pool->pDlbMutex);

  return(entry);
}

/**************************************************************
*
*  int dlbReuse - Resizes fix buffer size
*
*	Resizes the fixed buffer size and returns the number of
* buffers avialible at this new size.
*
* RETURNS:
*	Number of New buffers, or -1 for Error
*
*/
int dlbReuse(DLB_ID xx, int nBuffers, int bufSize)
{
   int nbufs;
   int npend,pAry[4];

   if (xx == NULL)
   {
      errLogRet(LOGIT,debugInfo,
          "dlbReuse: Was Handed a NULL DLB_ID Pointer.\n");
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

     dlbFreeAll(xx);
     nbufs = fBufferReUse(xx->pDLBfBufId,nBuffers,sizeof(DLB)+bufSize);
     if ( nbufs >= 1 )
     {
        xx->dataBufNum = nbufs;
        xx->dataBufSize = bufSize;
     }
     else   /* Oh Oh, fBufferReUse returned -2 */
     {
       errLogRet(LOGIT,debugInfo,
          "dlbReuse: Buffers could not be resize to %d bytes.\n",
		bufSize);
       return(nbufs);
     }
     return(xx->dataBufNum);
   }
   else
    return(xx->number);
}

/**************************************************************
*
*  int dlbFree - deletes an entry in the named buffer pool
*
*	frees memory associated with the DLBP (entry) and
*	resets it for re-use.
*
* RETURNS:
*	0 - OK, -1 entry not found, -2 Handed NULL DLB_ID Pointer
*
*/ 
int dlbFree(DLBP xx)
/* DLBP xx - pointer to element data structure*/
{
  int j,npend;
  int pAry[4];
  DLBP pEntry;
  char *pData;

  if (xx == NULL)
  {
    /*
      errLogRet(LOGIT,debugInfo,
          "dlbFree: Was Handed a NULL DLB_ID Pointer.\n");
    */
      return(-2);
  }

  DPRINT1(1,"dlbFree: Free Entry '%s'\n",xx->id);
  if (xx->status == LOADING)
  {
      DPRINT1(1,"dlbFree: '%s' - Buffer being LOADED, Deletion Prevented.\n",
		xx->id);
      return(0);
  }
  if (xx->status == PARSING)
  {
      DPRINT1(1,"dlbFree: WARNING: '%s' - Buffer being PARSED, Is Being Deleted.\n",xx->id);
  }

  pData = (char*) xx->data_array;

  pEntry = (DLBP) hashGet(xx->nambufptr->pNamTbl,xx->id);
  if (pEntry == NULL)   /* not present */
  {
     DPRINT1(1,"dlbFree: Entry '%s' Not found \n",xx->id);
     return(-1);
  }

  semTake(xx->nambufptr->pDlbMutex,WAIT_FOREVER);

  hashRemove(xx->nambufptr->pNamTbl,xx->id);
  /*
  if ( (tEntry = (DLBP) hashGet(xx->nambufptr->pNamTbl,xx->id)) != NULL)
  {
     printf("dlbFree: 0x%lx,'%s', did't get removed from hashTable\n",
	tEntry,tEntry->id);
     exit(0);
  }
  */

  xx->id[0] = '\0';
  xx->data_array = NULL;
  xx->data_end   = NULL;
  xx->size = 0;
  xx->status = MT;
  semGive(xx->nambufptr->pDlbMutex);

  /* if it was malloc'd dataspace then free it */
  if( xx->nambufptr->dataBufNum == 0)
  {
     if (pData != NULL)
       free(pData);
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
         DPRINT1(1,"Task: 0x%lx Pending on MakeEntry Semaphore\n",pAry[j]);
       }
	
     }
#endif
     semGive(xx->nambufptr->pDlbMakePendSem);
     if (npend > 1)
        semFlush(xx->nambufptr->pDlbMakePendSem);
  }

  return(0);
} 
/**************************************************************
*
*  int dlbFreeByName - deletes an entry in the named buffer pool
*
*	frees memory associated with the DLBP (entry) and
*	resets it for re-use.
*
* RETURNS:
*	0 - OK, -1 entry not found, -2 Handed NULL DLB_ID Pointer
*
*/ 
int dlbFreeByName(DLB_ID dlbId, char* label)
/* DLB_ID dlbId - name buffer Object */
/* char *label - label of name buffer to be deleted */
{
  if (dlbId == NULL)
  {
      errLogRet(LOGIT,debugInfo,
          "dlbFreeByName: Was Handed a NULL DLB_ID Pointer.\n");
      return(-2);
  }
  return( dlbFree( ((DLBP)hashGet(dlbId->pNamTbl,label)) ) );
}


/**************************************************************
*
*  DLBP dlbRename - Rename a named buffer pool
*
*       Renames a named buffer pool, if a buffer with this new name
*	is already present it is removed.
*
* RETURNS:
*       NULL or DLBP pointer
*
*/
DLBP dlbRename(DLB_ID dlbId, char* presentlabel,char* newlabel)
/* DLB_ID dlbId - name buffer Object */
/* char *presentlabel - label of name buffer to be renamed */
/* char *newlabel - label to replace previous name */
{
   DLBP entry,nentry;
   DLB  tmpdlb;

   if (dlbId == NULL)
      return(NULL);

   entry = dlbFindEntry(dlbId, presentlabel);
   if (entry == NULL)
   {
      errLogRet(LOGIT,debugInfo,
          "dlbRename: Label: %s not found in buffer pool.\n",presentlabel);
      return(NULL);
   }
   /*
   DPRINT1(-1,"old entry: '%s' --\n",presentlabel);
   dlbPrintEntry(entry);
   */

   /* make a copy of the old entry so it can be swapped with the new */
   memcpy(&tmpdlb,entry,sizeof(DLB));

   /* dlbPrintEntry(&tmpdlb); */

   nentry = dlbFindEntry(dlbId, newlabel);
   if (nentry == NULL)
   {
      errLogRet(LOGIT,debugInfo,
          "dlbRename: Label: %s not found in buffer pool.\n",newlabel);
      return(NULL);
   }
   /*
   DPRINT1(-1,"new entry: '%s' --\n",newlabel);
   dlbPrintEntry(nentry);
   */

   /* Replace present Entry reference with the new reference (or DLBP) */
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

   /* dlbPrintEntry(nentry); */

   /* lock down the named buffer while renaming it */
   semTake(dlbId->pDlbMutex,WAIT_FOREVER);

   dlbFree(nentry);

   semGive(dlbId->pDlbMutex);

   return( entry );
}

/**************************************************************
*
*  DLBP dlbFindEntry()
*
*	looks up a name in the named buffer pool for use
*
* RETURNS:
*	DLBP (entry pointer) if found, NULL otherwise
*
*/
DLBP dlbFindEntry(DLB_ID xx, char *label)
/* DLB_ID xx <-data structure of the pool  char *label - name to look up */
{
  DLBP entry;
  entry = (DLBP)hashGet(xx->pNamTbl,label);
  if ( entry == NULL )
  {
     DPRINT1(1,"dlbFindEntry: '%s' Not Found\n",label);
  }
  return(entry);
/*
  return((DLBP)hashGet(xx->pNamTbl,label));
*/
}

/**************************************************************
*
*  DLBP dlbFindEntryP()  - This routine allows task to pend on name
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
*	DLBP (entry pointer) if found, NULL otherwise
*
*/
DLBP dlbFindEntryP(DLB_ID xx, char *label, int timeout)
/* DLB_ID xx <-data structure of the pool  char *label - name to look up */
{
  DLBP entry;
  int timedOut = 0;
  int cnt;

  taskSafe();
  while ( ((entry = (DLBP)hashGet(xx->pNamTbl,label)) == NULL) && 
          (timedOut != ERROR) )
  {
#ifdef INSTRUMENT
     wvEvent(EVENT_NAMEBUF_FINDENTRY_PEND,NULL,NULL);
#endif
     DPRINT1(1,"dlbFindEntryP: '%s' entry not present, Pending \n",label);
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
      DPRINT1(1,"dlbFindEntryP: '%s' entry present, BUT being LOADED, Pending\n",label);
      switch(timeout)
      {
	case NO_WAIT:
		entry = NULL;
		break;

	case WAIT_FOREVER:
		/* check forever */
     		while((entry->status == LOADING))
     		{
			/* taskDelay(6); */
			taskDelay(0);
     		} 
		break;
	default:
  		/* when we haved waited the timeout period return */
     		cnt = timeout;
     		while((entry->status == LOADING) && (cnt >= 0))
     		{
			/* taskDelay(6);
			cnt -= 6;   /* decrement count */
			taskDelay(1);
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
*  void dlbFreeAll(DLB_ID obj)
*
*	frees all entries but leaves the top level OBJ intact
*
* RETURNS:
*	nothing
*
*/
void dlbFreeAll(DLB_ID xx)
/* DLB_ID obj - handle of the named memory manager */
{
  int i,j;
  DLBP entry;
  
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
  /* while((entry = (DLBP) hashGetEntry(xx->pNamTbl,&i,&j)) != NULL) */
  while((entry = (DLBP) hashGetNxtEntry(xx->pNamTbl,&i)) != NULL)
  {
    dlbFree(entry);
  }
  taskUnsafe();
  return;
}

void dlbFreeAllNotLoading(DLB_ID xx)
/* DLB_ID obj - handle of the named memory manager */
{
  int i,j,tot;
  DLBP entry;
  
  i = j = 0;
  tot =  dlbUsedBufs(xx);
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
  /* while((entry = (DLBP) hashGetEntry(xx->pNamTbl,&i,&j)) != NULL) */
  for (j=0; j < tot; j++)
  {
   if ((entry = (DLBP) hashGetNxtEntry(xx->pNamTbl,&i)) != NULL)
   {
    dlbFree(entry);
   }
  }
  taskUnsafe();
  return;
}

/**************************************************************
*
* dlbFreeBufs - return # of free buffers
*
*  This routine returns the number of free (unallocated) buffers in 
* the named buffer pool.
*
* RETURNS:
* number of free buffers
*
*       Author Greg Brissey 12/5/94
*/
int  dlbFreeBufs(DLB_ID x)
/* DLB_ID x - Fast Buffer Pool Id */
{
   return(fBufferFreeBufs(x->pDLBfBufId));
}

/**************************************************************
*
* dlbUsedBufs - return # of used (allocated) buffers
*
*  This routine returns the number of used (allocated) buffers in 
* the named buffer pool
*
* RETURNS:
* number of used elements
*
*       Author Greg Brissey 12/5/94
*/
int     dlbUsedBufs (DLB_ID x)
/* DLB_ID x - Fast Buffer Pool Id */
{
   return(fBufferUsedBufs(x->pDLBfBufId));
}

/**************************************************************
*
* dlbMaxSingleBuf - return the max size of a single buffer
*
*
* RETURNS:
* max size of a single buffer 
*
*       Author Greg Brissey 5/17/95
*/
long     dlbMaxSingleBuf (DLB_ID x)
/* DLB_ID x - Fast Buffer Pool Id */
{
   return(fBufferMaxSize(x->pDLBfBufId));
}

/**************************************************************
*
*  dlbSetReady - mark a named buffer ready for use
*
*	set the status field indicating the buffer 
*	has data.
*
* RETURNS:
*	void - nothing
*
*/ 
void dlbSetReady(DLBP xx)
/* DLBP xx - buffer entry to set */
{
  xx->status = READY;
}

/**************************************************************
*
*  dlbRelease(DLBP xx)
*
*	mark the entry as re-loadable - data may be bad 
*
* RETURNS:
*	int - no meaning (yet)
*/
int dlbRelease(DLBP xx)
/* DLBP xx - entry to release */
{
  /* could zero memory */
  xx->status = MT;
}

/**************************************************************
*
*  void  *dlbGetPntr(DLBP xx) - get a pointer to the allocated 
*				  memory
*
*	gets data pointer
*
* RETURNS:
*	void * -- pointer to memory or NULL if failed
*
*/
void  *dlbGetPntr(DLBP xx)
{
  return(xx->data_array);
}

/**************************************************************
*
*  int dlbPrintEntry(DLBP entry)
*
*	prints data about the pool
*
* RETURNS:
*	nothing
*
*/
int dlbPrintEntry(DLBP xx)
/* DLB_ID obj - handle of the named memory manager */
{
  printf("Label: '%s' Data Buf: 0x%lx\n",xx->id,xx->data_array);
  return(0);
}

/**************************************************************
*
*  void dlbPrintBuffers(DLB_ID obj)
*
*	prints data about the pool
*
* RETURNS:
*	nothing
*
*/
void dlbPrintBuffers(DLB_ID xx)
/* DLB_ID obj - handle of the named memory manager */
{
  int i,j;
  DLBP entry;
  
  i = j = 0;
  printf("Namebuffer Object: Entries Present: %d, Max: %d\n",
	hashGetNumEntries(xx->pNamTbl),xx->number);
  if (xx->dataBufNum > 0)
    printf("  Fast Fix Buffer Type, fBuffer Addr: 0x%lx\n",xx->pDLBfBufId);
  else
   printf("  Dynamic (malloc) Buffer Type\n");

  while((entry = (DLBP) hashGetEntry(xx->pNamTbl,&i,&j)) != NULL)
  {
    printf("Entry[%d,%d]: '%s', Addr: 0x%lx; Data Addr: 0x%lx,  Bufsize %d,  status %d\n",
	i,j-1,entry->id,entry,entry->data_array,entry->size,entry->status);
  }
}

/**************************************************************
*
*  void dlbShow(DLB_ID obj)
*
*	prints data about the Down Load Buffer Object 
*
* RETURNS:
*	nothing
*
*/
void dlbShow(DLB_ID xx,int level)
{
  printf("\nNamebuffer Object: Id - 0x%lx, Entries Present: %d, Max: %d\n",
	xx,hashGetNumEntries(xx->pNamTbl),xx->number);
  if (xx->dataBufNum > 0)
  {
    printf("  Fast Fix Buffer Type, fBuffer Addr: 0x%lx\n",xx->pDLBfBufId);
    printf("  Fast Fix Buffer Addr: 0x%lx, Number of Buffers: %d, Size: %d\n",
	xx->pDLBfBufId,xx->dataBufNum,xx->dataBufSize);
  }
  else
   printf("  Dynamic (malloc) Buffer Type\n");


  printf("\nTasks Pending Flag(0x%lx): %s \n", &(xx->findEntryPended),( (xx->findEntryPended == 1) ? "TRUE" : "FALSE" ));
  printf("Name Buffer Blocking Semaphore [dlbMakeEntryP()/dlbFindEntryP()]: Id - 0x%lx\n",xx->pDlbFindPendSem);
  printSemInfo(xx->pDlbFindPendSem,"Name Buffer Semaphore",1);

  if (level > 0)
  {
    printf("\nName Buffer: Hash Table:\nvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n\n");
    hashShow(xx->pNamTbl,level-1);
    printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");

    printf("\nName Buffer: Fast Buffer List:\nvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n\n");
    fBufferShow(xx->pDLBfBufId ,level-1);
    printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
  }

  return;
}
