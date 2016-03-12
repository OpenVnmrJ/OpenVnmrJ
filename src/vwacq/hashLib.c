/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* hashLib.c  11.1 07/09/07 - Hash table Source Modules */
/* 
 */


#ifndef ALLREADY_POSIX
 #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#endif
#include <vxWorks.h>
#include <semLib.h>
#include <memLib.h>
#include "commondefs.h"
#include "logMsgLib.h"
#include "hashLib.h"

/*
modification history
--------------------
8-6-93,gmb  created 
11-22-94,gmb  major architecture change in hash Table 
	      from handling collision (i.e. go to next table entry)
	      to one that adds "additional buckets" to the entry
	      via fBufferLib. This allows deletion of hash Entries
	      that was not supported by the previous design.

*/


/*
DESCRIPTION

Hash Library,  provides facilities for the creation and usage
of a fix sized hash table. The hash table consist of a key and a reference
address. The key is hashed to determine the position in the table.
The User can use the reference address as a pointer to the information
being hashed or the information itself (limited to a long integer). 
The User also provides the key hashing and comparison functions.

E.G.
  hashId = HashCreate(size, myKeyHashFunc, myKeyCmpFunc);

  hashPut(hashId,myKey,myRefAddr)  - into hash table

  myRefAddr = hashGet(hashId,myKey)  - get refaddr for key 

  dataIWhat = myRefAddr->myInfoStructMemberIWhat

 The hash table is an array of hash entries, collisions are handled
 by addition of hash entries in a link list fashion to that hash
 table entry. Space for these new entries are pre-allocated up front
 and are handled throught the fast buffer library. Therefore if enough
 new entries are gotten the is a chance of blocking.
*/

static char *HashIDStr ="Hash Table";
static int  HIdCnt = 0;

/**************************************************************
*
*  hash- Creates a hash index 
*
*
*
* RETURNS:
* hash index 
*
*	Author Greg Brissey 8/5/93
*/
static long hash(register long key, long max)
/* long key - key to hash */
/* long max - maximum key value to return */
{
   register long  hashkey;
   long index;

 
   index = (key & 0x00ffffff) / 256;

   return (index);
}

/****************************************************
* equal - compare to long values return true if equal false if not
*/
static int equal(long a, long b)
/* long a - 1st arg to cmp */
/* long b - 2nd arg to cmp */
{
   return( (a == b) );
}


/**************************************************************
*
*  hashCreate - Creates an empty hash table
*
*
*  This routine creates am empty hash table of size <size> 
*  and initializes it. Memory for the buffer is allocated from 
*  the system memory partition.
*
*
* RETURNS:
* ID of the hash table , or NULL if memory cannot be allocated.
*
*	Author Greg Brissey 8/5/93
*/
HASH_ID hashCreate(int size,PFL hashf, PFI cmpf, PFI deletef, PFI showf, char* idstr, int wveventid)
/* int size;  size of hash table */
/* PFL hashf;  pointer to user defined hashing function */
/* PFI cmpf;   pointer to user defined comparison function */
/* char* idstr;   Identifing string for user */
{
  HASH_ID pHashTbl;
  HASH_ENTRY *pHashAry;
  register long i;
  char tmpstr[80];
    char*	pHashIdStr;

  long hash(long,long);
  int equal(long,long);

  pHashTbl = (HASH_ID) malloc(sizeof(HASH_TABLE));  /* create structure */

  if (pHashTbl == NULL) 
  {
     errLogSysRet(LOGIT,debugInfo,
         "hashCreate: hash table malloc failed.\n");
     return (NULL);
  }
 
  memset((char*) pHashTbl, 0, sizeof(HASH_TABLE));

  if (idstr == NULL) 
  {
     sprintf(tmpstr,"%s %d\n",HashIDStr,++HIdCnt);
     pHashTbl->pHashIdStr = (char *) malloc(strlen(tmpstr)+2);
  }
  else
  {
     pHashTbl->pHashIdStr = (char *) malloc(strlen(idstr)+2);
  }

  if (pHashTbl->pHashIdStr == NULL)
  {
     errLogSysRet(LOGIT,debugInfo,
         "hashCreate: hash id string malloc failed.\n");
     free(pHashTbl);
     return(NULL);
  }

  if (idstr == NULL) 
  {
     strcpy(pHashTbl->pHashIdStr,tmpstr);
  }
  else
  {
     strcpy(pHashTbl->pHashIdStr,idstr);
  }

  /* create space for all table entries  */
  pHashTbl->pHashEntries = (HASH_ENTRY *) malloc( (size * sizeof(HASH_ENTRY)) );  

  if (pHashTbl->pHashEntries == NULL) 
  {
     errLogSysRet(LOGIT,debugInfo,
         "hashCreate: hash table entries malloc failed.\n");
     free(pHashTbl->pHashIdStr);
     free(pHashTbl);
     return (NULL);
  }

  pHashTbl->bucketMax = (long) size/2;
  pHashTbl->bucketMaxUsed = (long) 0;
  pHashTbl->pFreeBuckets = fBufferCreate(size/2, sizeof(HASH_ENTRY), NULL,
				wveventid);
  if (pHashTbl->pFreeBuckets == NULL)
  {
     errLogSysRet(LOGIT,debugInfo,
         "hashCreate: hash free buckets malloc failed.\n");
     free(pHashTbl->pHashIdStr);
     free(pHashTbl->pHashEntries);
     free(pHashTbl);
     return (NULL);
  }

   /* create the Hash Table Mutual Exclusion semaphore */
   pHashTbl->pHashMutex = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE | 
					SEM_DELETE_SAFE);

  if (pHashTbl->pHashMutex == NULL) 
  {
     errLogSysRet(LOGIT,debugInfo,
         "hashCreate: hash mutex creation failed.\n");
     fBufferDelete(pHashTbl->pFreeBuckets);
     free(pHashTbl->pHashIdStr);
     free(pHashTbl->pHashEntries);
     free(pHashTbl);
     return (NULL);
  }

  pHashTbl->numEntries =(long) 0;
  pHashTbl->maxEntries = size;
  pHashTbl->collisions = (long) 0;


  if (hashf == NULL)
     pHashTbl->pHashFunc = hash; /* if users func is NULL use default hash */
  else
     pHashTbl->pHashFunc = hashf; /* set up users hash function */

  if (cmpf == NULL)
     pHashTbl->pCmpFunc = equal;  /* if users func is NULL use default hash */
  else
     pHashTbl->pCmpFunc = cmpf; 	  /* set up users hash function */

  if (deletef != NULL)
     pHashTbl->pDeleteFunc = deletef;

  if (showf != NULL)
     pHashTbl->pShowFunc = showf;

  pHashAry = pHashTbl->pHashEntries;
  /* initialize hash table */
  for(i=0;i<size;i++)
  {
    pHashAry[i].hashKey = NULL;
    pHashAry[i].refAddr= NULL;
    pHashAry[i].next= NULL;
  }
   
  return( pHashTbl );
}

/**************************************************************
*
* hashDelete - Delete Hash Table
*
*
*  Delete Hash Table
*
* RETURNS:
* N/A
*
*		Author Greg Brissey 8/5/93
*/
VOID hashDelete(HASH_ID hashid)
/* HASH_ID hashid;  hash table to delete */
{
  int i;
  HASH_ENTRY *pHashAry;
  HASH_ENTRY *entry;

  semTake(hashid->pHashMutex,WAIT_FOREVER);/* take semi, never tobe releases again */

  pHashAry = hashid->pHashEntries;
  for(i=0;i<hashid->maxEntries;i++)
  {
    entry = pHashAry[i].next;
    while(entry)
    {
      
      /* if passed delete function then for each entry call it */
      if ((hashid->pDeleteFunc != NULL) && (pHashAry[i].refAddr != NULL))
          (*hashid->pDeleteFunc) (entry->refAddr);
      
      entry = entry->next;
      fBufferReturn(hashid->pFreeBuckets,(long)entry);
    }
    entry = &pHashAry[i];
      /* if passed delete function then for each entry call it */
      if ((hashid->pDeleteFunc != NULL) && (pHashAry[i].refAddr != NULL))
          (*hashid->pDeleteFunc) (entry->refAddr);
  }
   
  fBufferDelete(hashid->pFreeBuckets);
  semDelete(hashid->pHashMutex); /* because we just got rid of it */
  free(hashid->pHashIdStr);
  free(hashid->pHashEntries);
  free(hashid);
}

/*----------------------------------------------------------------------*/
/* fifoShwResrc							*/
/*     Show system resources used by Object (e.g. semaphores,etc.)	*/
/*	Useful to print then related back to WindView Events		*/
/*----------------------------------------------------------------------*/
void hashShwResrc (HASH_ID hashid, int indent)
{
   int i;
   char spaces[40];

   for (i=0;i<indent;i++) spaces[i] = ' ';
   spaces[i]='\0';


   printf("%s Hash Obj: '%s', 0x%lx\n",spaces,hashid->pHashIdStr,hashid);
   printf("%s    Mutex:       pHashMutex  ------ 0x%lx\n",spaces,hashid->pHashMutex);
   printf("\n%s    Fast Buffer:\n",spaces);
   fBufferShwResrc(hashid->pFreeBuckets,indent+4);
}


/**************************************************************
*
* hashPut - put entry into a hash table 
*
*
*  This routine places an entry into the hash table based on the
*  key.
*
* RETURNS:
*  OK, ERROR .
*
*	Author Greg Brissey 8/5/93
*/
int hashPut (HASH_ID hashid, char* key, char* refaddr)
/* HASH_ID    hashid;	hash table to put entry into */
/* char*      key;   	the key to base the index in table */
/* char*      refaddr;  reference address of info hashed */
{
   long index,bucket,nbuckets;
   register HASH_ENTRY *hashlist;
   register HASH_ENTRY *entry;

   index = (*hashid->pHashFunc)((long) key,hashid->maxEntries);
/*
   printf("\nPut\n");
   print_Entry(hashid,index);
*/
   if (index < 0 || index > hashid->maxEntries)
   {
      errLogRet(LOGIT,debugInfo,
         "hashPut: computed index out-of-bounds: %ld\n",index);
      return(ERROR);
   }
   hashlist = hashid->pHashEntries;
   entry = &hashlist[index];

   semTake(hashid->pHashMutex,WAIT_FOREVER); /* take mutex semaphore on hash struct */

   bucket = 0;
   if (entry->hashKey)     /* look for next free slot */
   {
      bucket++;
      while(entry->next)   /* go down the list of buckets */
      {
        hashid->collisions++;
        DPRINT3(3,"hash:collision with 0x%lx  and 0x%lx , total: %ld\n",
		key, entry->hashKey, hashid->collisions);
        entry = entry->next;
        bucket++;
      }
      nbuckets = fBufferFreeBufs(hashid->pFreeBuckets);
      if ( nbuckets  < 2 )
      {
        errLogRet(LOGIT,debugInfo,
         "hashPut: Warning: The free Bucket list of is almost empty.\n");
      }
      if ((hashid->bucketMax - nbuckets) > hashid->bucketMaxUsed)
         hashid->bucketMaxUsed = hashid->bucketMax - nbuckets;

      entry->next = (HASH_ENTRY*) fBufferGet(hashid->pFreeBuckets);
      entry = entry->next;
      entry->next = NULL;
   }
   entry->hashKey = key;
   entry->refAddr = refaddr;

   if ( ++(hashid->numEntries) > hashid->maxEntries)
   {
     errLogSysRet(LOGIT,debugInfo,
         "hashPut: Warning: Number of entries(%d) has exceeded the hash table size (%d)\n",
	 hashid->numEntries,hashid->maxEntries);
   }

   semGive(hashid->pHashMutex); /* give mutex semaphore back */

   DPRINT4(2,"hashPut: table[%ld,%d] -  key 0x%lx, refadr: 0x%lx \n", index, 
	 bucket, entry->hashKey,entry->refAddr);

/*
   print_Entry(hashid,index);
   printf("Put\n\n");
*/
   return( OK );
}

/**************************************************************
*
* hashGet - get entry from a hash table 
*
*
*  This routine obtains a hash entry from the hash table based on the
*  key.
*
* RETURNS:
*  Reference Address of key, NULL if not found .
*
*	Author Greg Brissey 8/5/93
*/
char* hashGet (HASH_ID hashid, char* key)
/* HASH_ID    hashid;	hash table to get entry from */
/* char*      key;   	the key to base the index in table */
{
   long index;
   register HASH_ENTRY *hashlist,*entry;
   char* refaddr = NULL;

   index = (*hashid->pHashFunc)((long)key, hashid->maxEntries);
/*
   printf("hashGet:");
   print_Entry(hashid,index);
*/
   if (index < 0 || index > hashid->maxEntries)
   {
     errLogSysRet(LOGIT,debugInfo,
         "hashGet: computed index out-of-bounds: %ld\n",index);
     return(refaddr);
   }
   hashlist = hashid->pHashEntries;
   entry = &hashlist[index];

   semTake(hashid->pHashMutex,WAIT_FOREVER); /* take mutex semaphore on hash struct */

   while(entry)
   {
      /* printf("Get: id %s,0x%lx\n",entry->hashKey,entry->hashKey); */
      if ( entry->hashKey )
      {
        if ( (*hashid->pCmpFunc)((long)key,(long)entry->hashKey) )
        {
	   /* do this tobe multi thread safe */
           refaddr = entry->refAddr; 
	   break;
        }
      }
      entry = entry->next;
   }

   semGive(hashid->pHashMutex); /* give mutex semaphore back */

   return(refaddr);
}


/**************************************************************
*
* hashReplace - replace present key reference with a new reference
*
*
*  This routine obtains a hash entry from the hash table based on the
*  key.
*
* RETURNS:
*  Replaced Reference Address of key, NULL if not found .
*
*	Author Greg Brissey 1/25/99
*/
char* hashReplace (HASH_ID hashid, char* key, char* newref)
/* HASH_ID    hashid;	hash table to get entry from */
/* char*      key;   	the key to base the index in table */
{
   long index;
   register HASH_ENTRY *hashlist,*entry;
   char* refaddr = NULL;

   index = (*hashid->pHashFunc)((long)key, hashid->maxEntries);

   /*
   printf("hashRepalce:");
   print_Entry(hashid,index);
   */

   if (index < 0 || index > hashid->maxEntries)
   {
     errLogSysRet(LOGIT,debugInfo,
         "hashGet: computed index out-of-bounds: %ld\n",index);
     return(refaddr);
   }
   hashlist = hashid->pHashEntries;
   entry = &hashlist[index];

   semTake(hashid->pHashMutex,WAIT_FOREVER); /* take mutex semaphore on hash struct */

   while(entry)
   {
      /* printf("Get: id %s,0x%lx\n",entry->hashKey,entry->hashKey); */
      if ( entry->hashKey )
      {
        if ( (*hashid->pCmpFunc)((long)key,(long)entry->hashKey) )
        {
	   /* do this tobe multi thread safe */
	   /*printf("hashReplace(): key: '%s', 0x%lx orig vs 0x%lx new RefAddr replaced\n",
	         key,entry->refAddr,newref);	 */
           refaddr = entry->refAddr;
           entry->refAddr = newref; 
	   break;
        }
      }
      entry = entry->next;
   }

   semGive(hashid->pHashMutex); /* give mutex semaphore back */

   return(refaddr);
}



/**************************************************************
*
* hashGetNxtEntry - get the next entry from a hash table (non-hashed)
*
*
*  This routine allow the an application to obtain a 1st entry at an index 
*   in the hash table. First call with index to zero. The call returns with
*   the index set to the index where the enrty was found. 
*  Primary usage is when deleting all entries of the hash table (namebufs.c)
*  (actually its the only real use for this function since without deletion
*  of the entry returned this routine would always return the same entry )
* 
* RETURNS:
*  Reference Address of key, NULL if not found .
*
*	Author Greg Brissey 11/3/99
*/
char* hashGetNxtEntry (HASH_ID hashid, int *index)
/* HASH_ID    hashid;	hash table to get entry from */
/* int        *index;   Index into hash Table */
/* int        *bucket;  Bucket of a hash Table Index */
{
   register HASH_ENTRY *hashlist,*entry;
   int bucketloc;
   char* refaddr = NULL;

   hashlist = hashid->pHashEntries;

   semTake(hashid->pHashMutex,WAIT_FOREVER); /* take mutex semaphore on hash struct */

   while( *index < hashid->maxEntries )
   {
      entry = &hashlist[*index];
      while (entry)
      {
        if ( entry->hashKey != NULL )     /* look for entry */
        {
           refaddr = entry->refAddr;/* do this tobe multi thread safe */
           semGive(hashid->pHashMutex); /* give mutex semaphore back */
	   return(refaddr);
        }
        else 
	{
	   entry = entry->next; /* next bucket */
	}
      }
      (*index) += 1;
   }

   semGive(hashid->pHashMutex); /* give mutex semaphore back */

   return(refaddr);
}
/**************************************************************
*




/**************************************************************
*
* hashGetEntry - get the next entry from a hash table (non-hashed)
*
*
*  This routine allow the an application to obtain a list of all entries
*   in the hash table. First call with index to zero. The call returns with
*   the index set to the next index value. Continue to call till a
*   NULL is returned.
*
* RETURNS:
*  Reference Address of key, NULL if not found .
*
*	Author Greg Brissey 8/5/93
*/
char* hashGetEntry (HASH_ID hashid, int *index, int *bucket)
/* HASH_ID    hashid;	hash table to get entry from */
/* int        *index;   Index into hash Table */
/* int        *bucket;  Bucket of a hash Table Index */
{
   register HASH_ENTRY *hashlist,*entry;
   int bucketloc;
   char* refaddr = NULL;

   hashlist = hashid->pHashEntries;

   semTake(hashid->pHashMutex,WAIT_FOREVER); /* take mutex semaphore on hash struct */

   while( *index < hashid->maxEntries )
   {
      entry = &hashlist[*index];
      bucketloc = 0;
      while(entry)
      {
        if ( (bucketloc >= (*bucket)) && 
	     (entry->hashKey != NULL) )     /* look for entry */
        {
           refaddr = entry->refAddr;/* do this tobe multi thread safe */
	   (*bucket) = bucketloc + 1;
   	   semGive(hashid->pHashMutex); /* give mutex semaphore back */
	   return(refaddr);
        }
        entry = entry->next;
        bucketloc++;
     }
     (*index) += 1;
     (*bucket) = bucketloc  = 0;
   }

   semGive(hashid->pHashMutex); /* give mutex semaphore back */

   return(refaddr);
}
/**************************************************************
*
* hashGetNumEntries - get the number of entrries in hash table 
*
*
*
* RETURNS:
*  Number in Hash Table 
*
*	Author Greg Brissey 8/5/93
*/
int hashGetNumEntries (HASH_ID hashid)
/* HASH_ID    hashid;	hash table to get entry from */
{
    return(hashid->numEntries);
}
/**************************************************************
*
* hashRemove - remove entry from a hash table 
*
*
*  This routine removes a hash entry from the hash table based on the
*  value of the key.
*
* RETURNS:
*  OK, ERROR .
*
*	Author Greg Brissey 8/5/93
*/
int hashRemove (HASH_ID hashid, char* key)
/* HASH_ID    hashid;	hash table to remove entry from */
/* char*      key;   	the key to base the index in table */
{
   long index;
   register HASH_ENTRY *hashlist;
   register HASH_ENTRY *entry,*lastEntry,*attachEntry;
   int noBucketsLeft;
   int stat = ERROR;

   index = (*hashid->pHashFunc)((long)key, hashid->maxEntries);
   DPRINT1(2,"hashRemove: '%s'\n",key);
   /* print_Entry(hashid,index); */
   if (index < 0 || index > hashid->maxEntries)
   {
     errLogSysRet(LOGIT,debugInfo,
        "hashRemove: computed index out-of-bounds: %ld\n",index);
      return(ERROR);
   }
   hashlist = hashid->pHashEntries;
   entry = hashlist[index].next;
   lastEntry = entry;
   attachEntry = NULL;

   noBucketsLeft = 0;

   semTake(hashid->pHashMutex,WAIT_FOREVER); /* take mutex semaphore on hash struct */

/*
   printf("idx: %d, entry: 0x%lx, entry->Key: 0x%lx, entry->Ref: 0x%lx, next: 0x%lx\n",
	 index,entry,entry->hashKey,entry->refAddr,entry->next);
*/
   while(entry)
   {
/*
      printf("Link entry->key: '%s', findkey: '%s', next: 0x%lx\n",
	    entry->hashKey,key,entry->next);
*/
      if ( (*hashid->pCmpFunc)((long)key,(long)entry->hashKey) )
      {
         if (lastEntry != entry)   /* if not 1st bucket then just hook links */
            lastEntry->next = entry->next;   /* around this one */
         else  /* 1st bucket then attach rest of buckets to main entry or */
         {     /* there may be no more buckets, set next to NULL */
            if (entry->next)
               attachEntry = entry->next;
            else
               noBucketsLeft = 1;
         }
	 entry->hashKey = NULL;
	 entry->refAddr = NULL;
	 entry->next = NULL;
         fBufferReturn(hashid->pFreeBuckets,(long) entry);
         (hashid->numEntries)--;
         stat = OK;
	 break;
      }
      entry = entry->next;
   }
/*
   printf("Removed from LinkList: %d\n",stat);
   printf("AttachEntry: 0x%lx (nonZero if 1st in link deleted)\n",attachEntry);
   printf("noBucketsLeft: %d (nonZero if last link deleted)\n",noBucketsLeft);
*/
   if (stat != OK)
   {
/*
      printf("Table entry: key: '%s', findkey: '%s', next: 0x%lx\n",
	    hashlist[index].hashKey,key,hashlist[index].next);
*/
      if ( (*hashid->pCmpFunc)((long)key,(long)hashlist[index].hashKey) )
      {
	 hashlist[index].hashKey = NULL;
	 hashlist[index].refAddr = NULL;
         (hashid->numEntries)--;
         stat = OK;
      }
   }
   else if (noBucketsLeft == 1)
     hashlist[index].next = NULL;
   else if (attachEntry != NULL)
     hashlist[index].next = attachEntry;
     

   semGive(hashid->pHashMutex); /* give mutex semaphore back */
   /* print_Entry(hashid,index); */

   return( stat );
}

/**************************************************************
*
* hashShow - show hash table information 
*
*
*  This routine display that state and optionally the
*  hash table contents.
*
* RETURNS:
*
*	Author Greg Brissey 8/5/93
*/
void hashShow (HASH_ID hashid, int level)
/* HASH_ID    hashid;	hash table to show */
/* int        level;   	level of information output */
{

   printf("Hash Table:  Id - 0x%lx: '%s'\n",hashid,hashid->pHashIdStr);
   printf("Hash Table Entry Address:  0x%lx\n", hashid->pHashEntries);
   printf("Table Entries: %ld, Max Entries: %ld\n", hashid->numEntries, 
			hashid->maxEntries);
   printf("Number of Hashing Collisions: %ld\n",hashid->collisions);
   printf("Number of Buckets: %ld, Inuse: %ld, Free; %ld, Max Used: %ld\n",
    hashid->bucketMax, (long) (fBufferUsedBufs(hashid->pFreeBuckets)),
    (long) (fBufferFreeBufs(hashid->pFreeBuckets)),hashid->bucketMaxUsed);
   printf("Hash Func: 0x%lx, Comparison Func: 0x%lx\n",hashid->pHashFunc,
			hashid->pCmpFunc);
   if (level > 0)
   {
     printf("\nHash Mutex Info:\n");
     semShow(hashid->pHashMutex,1);
   }
   printf("Hash Free Bucket List (Fast Buffer):\nvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n");
   fBufferShow(hashid->pFreeBuckets,level);
   printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");

   if (level > 1)
   {
     long i,j;
     register HASH_ENTRY *hashlist,*entry;

     hashlist = hashid->pHashEntries;

     printf("\n\nHash Table Entries:\n");
     for(i=0,j=0;i<hashid->maxEntries;i++)
     {
      entry = &hashlist[i];
      j = 0;
      while(entry)
      {
        if (entry->hashKey)
        {
          if (hashid->pShowFunc == NULL)
          {
	    printf("Hash Index[%ld,%d]: Key: 0x%lx, Reference Addr: 0x%lx\n",
	      i,j,entry->hashKey,entry->refAddr);
          }
	  else
          {
	     (*hashid->pShowFunc) (entry->refAddr);
          }
        }
        j++;
        entry = entry->next;
      }
     }

   }
}

void prtHashKeys(HASH_ID hashid)
{
     int index;
  HASH_ENTRY *hashlist,*entry;
  long k, i,j;

     hashlist = hashid->pHashEntries;
     for (index=0; index < hashid->maxEntries; index++)
     {
         entry = &hashlist[index];
         if (entry->hashKey != NULL)
         {
            j = 0;
           while(entry)
           {
             if (entry->hashKey)
             {
	       /* printf("Hash Index[%d]: Key: 0x%lx, Reference Addr: 0x%lx\n", */
	       /*  [index,bucket]  */
	       printf("--- [%d,%d]: '%s', 0x%lx, ",
	         index,j,entry->hashKey,entry->refAddr);
             }
             j++;
             entry = entry->next;
           }
           printf("\n");
	}
     }
}

static print_Entry(HASH_ID hashid, int index)
{
  HASH_ENTRY *hashlist,*entry;
  long i,j;

     hashlist = hashid->pHashEntries;
     entry = &hashlist[index];

      j = 0;
      while(entry)
      {
        if (entry->hashKey)
        {
	    /* printf("Hash Index[%d]: Key: 0x%lx, Reference Addr: 0x%lx\n", */
	    printf("--- [%d,%d]: '%s', 0x%lx, ",
	      index,j,entry->hashKey,entry->refAddr);
        }
        j++;
        entry = entry->next;
      }
    printf("\n");
}
