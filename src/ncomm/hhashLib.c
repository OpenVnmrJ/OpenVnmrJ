/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>

#include "errLogLib.h"
#include "hhashLib.h"

/*
modification history
--------------------
8-4-94,gmb  created 
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
*/

static char *HashIDStr ="Hash Table";
static int  HIdCnt = 0;

/**************************************************************
*
*  strhash- Creates a hash index from a string
*
*
*
* RETURNS:
* hash index 
*
*	Author Greg Brissey 8/5/94
*/
static long strhash(char *keystr, long max)
/* HASH_ID    hashid;	hash table to generate hash key for */
/* char       *keystr - string to create hash key */
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

#ifdef XXX
/****************************************************
* equal - compare to long values return true if equal false if not
*/
static int equal(long a, long b)
/* long a - 1st arg to cmp */
/* long b - 2nd arg to cmp */
{
   return( (a == b) );
}
#endif


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
HASH_ID hashCreate(int size,PFL hashf, PFI cmpf, char* idstr)
/* int size;  size of hash table */
/* PFL hashf;  pointer to user defined hashing function */
/* PFI cmpf;   pointer to user defined comparison function */
/* char* idstr;   Identifing string for user */
{
  HASH_ID pHashTbl;
  HASH_ENTRY *pHashAry;
  register long i;
  char tmpstr[80];

  pHashTbl = (HASH_ID) malloc(sizeof(HASH_TABLE));  /* create structure */

  if (pHashTbl == NULL) 
     return (NULL);
 

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
     free(pHashTbl->pHashIdStr);
     free(pHashTbl);
     return (NULL);
  }

  pHashTbl->numEntries =(long) 0;
  pHashTbl->maxEntries = size;
  pHashTbl->collisions = (long) 0;

  if (hashf == NULL)
     pHashTbl->pHashFunc = strhash; /* if users func is NULL use default hash */
  else
     pHashTbl->pHashFunc = hashf; /* set up users hash function */

  if (cmpf == NULL)
     pHashTbl->pCmpFunc = cmpstrs;  /* if users func is NULL use default hash */
  else
     pHashTbl->pCmpFunc = cmpf; 	  /* set up users hash function */

  pHashAry = pHashTbl->pHashEntries;
  /* initialize hash table */
  for(i=0;i<size;i++)
  {
    pHashAry[i].hashKey = NULL;
    pHashAry[i].refAddr= NULL;
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
void hashDelete(HASH_ID hashid)
/* HASH_ID hashid;  hash table to delete */
{
  free(hashid->pHashIdStr);
  free(hashid->pHashEntries);
  free(hashid);
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
   long index;
   register HASH_ENTRY *hashlist;

   index = (*hashid->pHashFunc)((char *) key, hashid->maxEntries);
   if (index < 0 || index > hashid->maxEntries)
   {
      errLogRet(ErrLogOp,debugInfo,"hashPut: computed index out-of-bounds: %ld\n",index);
      return(ERROR);
   }
   hashlist = hashid->pHashEntries;

   while (hashlist[index].hashKey)     /* look for next free slot */
   {
      hashid->collisions++;
         DPRINT3(4,"hash:collision with 0x%p  and %s , total: %ld\n",
                key, hashlist[index].hashKey, hashid->collisions);
      ++index;   /* increment key */
      index %= hashid->maxEntries;
   }

   hashlist[index].hashKey = key;
   hashlist[index].refAddr = refaddr;

   if ( ++(hashid->numEntries) > hashid->maxEntries)
   {
      errLogRet(ErrLogOp,debugInfo,"hash: ERROR Number of identifiers exceeds hash table\n");
   }

   DPRINT3(4,"hash: table[%ld] -  key %s, refadr: %s \n", index, 
	 hashlist[index].hashKey,hashlist[index].refAddr);

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
*  Reference Address of key, -1 if not found .
*
*	Author Greg Brissey 8/5/93
*/
void* hashGet (HASH_ID hashid, char* key)
/* HASH_ID    hashid;	hash table to get entry from */
/* char*      key;   	the key to base the index in table */
{
   long index;
   register HASH_ENTRY *hashlist;
   char* refaddr = (char*) -1L;

   index = (*hashid->pHashFunc)((char *) key, hashid->maxEntries);
   if (index < 0 || index > hashid->maxEntries)
   {
      errLogRet(ErrLogOp,debugInfo,"hashGet: computed index out-of-bounds: %ld\n",index);
      return((char*)ERROR);
   }
   hashlist = hashid->pHashEntries;

   while (hashlist[index].hashKey)     /* look for entry */
   {
      if ( (*hashid->pCmpFunc)((char*)key,(char*)hashlist[index].hashKey) )
      {
         refaddr = hashlist[index].refAddr; /* do this tobe multi thread safe */
	 break;
      }
      ++index;   /* increment key */
      index %= hashid->maxEntries;
   }

   return((void*)refaddr);
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
   int stat = ERROR;

   index = (*hashid->pHashFunc)((long)key);
   if (index < 0 || index > hashid->maxEntries)
   {
      errLogRet(ErrLogOp,debugInfo,"hashRemove: computed index out-of-bounds: %ld\n",index);
      return(ERROR);
   }
   hashlist = hashid->pHashEntries;

   while (hashlist[index].hashKey)     /* look for entry */
   {
      if ( (*hashid->pCmpFunc)((long)key,(long)hashlist[index].hashKey) )
      {
	 hashlist[index].hashKey = NULL;
	 hashlist[index].refAddr = NULL;
         (hashid->numEntries)--;
         stat = OK;
	 break;
      }
      ++index;   /* increment key */
      index %= hashid->maxEntries;
   }

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

   DPRINT2(3,"Hash Table ID: '%s', %p\n",hashid->pHashIdStr,hashid);
   DPRINT(3,"Hash Stats:\n");
   DPRINT2(3,"Hash Table Address:  %p, %p\n",
		hashid->pHashEntries,hashid->pHashEntries);
   DPRINT2(3,"Entries: %ld, Max Entries: %ld\n", hashid->numEntries, 
			hashid->maxEntries);
   DPRINT1(3,"Number of Hashing Collisions: %ld\n",hashid->collisions);
   DPRINT2(3,"Hash Func: %p, Comparison Func: %p\n",hashid->pHashFunc,
			hashid->pCmpFunc);

   if (level > 0)
   {
     long i;
     register HASH_ENTRY *hashlist;

     hashlist = hashid->pHashEntries;

     DPRINT(3,"\n\nHash Table Entries:\n");
     for(i=0;i<hashid->maxEntries;i++)
     {
      if (hashlist[i].hashKey)
      {
	DPRINT3(3,"Hash Index[%ld]: Key: %s, Reference Addr: %s\n",
	 i,hashlist[i].hashKey,hashlist[i].refAddr);
      }
     }

   }

}
