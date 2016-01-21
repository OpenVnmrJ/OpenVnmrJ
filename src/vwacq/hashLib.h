/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INChashLibh
#define INChashLibh

#include <semLib.h>
#include "commondefs.h"
#include "fBufferLib.h"

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* HIDDEN */

/* typedefs */

typedef struct _hashEntry_ {
  char* hashKey;
  char* refAddr;
  struct _hashEntry_* next;
} HASH_ELEMENT;

typedef HASH_ELEMENT HASH_ENTRY;

typedef struct          /* Hash Object */
    {
    char*	pHashIdStr;
    SEM_ID	pHashMutex;
    PFL		pHashFunc;
    PFI		pCmpFunc;
    PFI		pDeleteFunc;
    PFI		pShowFunc;
    HASH_ENTRY  *pHashEntries;
    FBUFFER_ID  pFreeBuckets;
    long	numEntries;
    long	maxEntries;
    long	collisions;
    long	bucketMaxUsed;
    long	bucketMax;
    } HASH_TABLE;

/* END_HIDDEN */

typedef HASH_TABLE *HASH_ID;


/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

 
extern	  HASH_ID      hashCreate(int size,PFL hashf, PFI cmpf, PFI deletef, PFI showf, char* idstr,int wveventid);
extern    VOID         hashDelete (HASH_ID hashId);
extern    char*        hashGet (HASH_ID hashId, char* key);
extern    char*        hashGetEntry (HASH_ID hashid, int *index, int *bucket);
extern    int 	       hashGetNumEntries (HASH_ID hashid);
extern    int          hashPut (HASH_ID hashId, char* key, char* refaddr);
extern    int          hashRemove (HASH_ID hashId, char* key);
extern    VOID         hashShow (HASH_ID hashId, int level);
 
/* --------- NON-ANSI/C++ prototypes ------------  */

#else
 
extern    HASH_ID      hashCreate ();
extern    VOID         hashDelete ();
extern    char*        hashGet ();
extern	  char*        hashGetEntry ();
extern    int 	       hashGetNumEntries ();
extern    int          hashPut ();
extern    int          hashRemove ();
extern    VOID         hashShow ();
 
#endif  /* __STDC__ */
 
#ifdef __cplusplus
}
#endif

#endif /* INCrngBlkLibh */
