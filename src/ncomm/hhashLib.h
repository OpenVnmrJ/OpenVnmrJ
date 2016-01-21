/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INChhashLibh
#define INChhashLibh

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* HIDDEN */

/* typedefs */

#ifndef OK
#define OK 0
#endif
#ifndef ERROR
#define ERROR -1
#endif
#ifndef PFI
typedef int (*PFI)();
#endif
#ifndef PFL
typedef long (*PFL)();
#endif


typedef struct          /* RING_BLKING - blocking ring buffer */
    {
    char* hashKey;
    char* refAddr;
    } HASH_ELEMENT;

typedef HASH_ELEMENT HASH_ENTRY;

typedef struct          /* RING_BLKING - blocking ring buffer */
    {
    char*	pHashIdStr;
    PFL		pHashFunc;
    PFI		pCmpFunc;
    HASH_ENTRY  *pHashEntries;
    long	numEntries;
    long	maxEntries;
    long	collisions;
    } HASH_TABLE;

/* END_HIDDEN */

typedef HASH_TABLE *HASH_ID;


/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

 
extern    HASH_ID     hashCreate(int size, PFL hashfunc, PFI cmpfunc,char* idstr);
extern    void         hashDelete (HASH_ID hashId);
extern    void*        hashGet (HASH_ID hashId, char* key);
extern    int          hashPut (HASH_ID hashId, char* key, char* refaddr);
extern    int          hashRemove (HASH_ID hashId, char* key);
extern    void         hashShow (HASH_ID hashId, int level);
 
/* --------- NON-ANSI/C++ prototypes ------------  */

#else
 
extern    HASH_ID      hashCreate ();
extern    void         hashDelete ();
extern    void*         hashGet ();
extern    int          hashPut ();
extern    int          hashRemove ();
extern    void         hashShow ();
 
#endif  /* __STDC__ */
 
#ifdef __cplusplus
}
#endif

#endif /* INChhashLibh */
