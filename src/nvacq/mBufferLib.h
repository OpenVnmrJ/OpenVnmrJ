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
#ifndef INCmBufferLibh
#define INCmBufferLibh

#include "rngLLib.h"

typedef struct mbInfo
    {
    int         numClusters;
    int         clSize[20];                 /* cluster type */
    int         clFree[20];                  /* number of clusters */
    int         clNum[20];                /* pre allocated memory area */
    int         memSize[20];                /* pre allocated memory size */
    int         numMallocs;
    int         mallocSize;
    } MBUF_INFO;
 



typedef struct mbDesc
    {
    int         clSize;                 /* cluster type */
    int         clNum;                  /* number of clusters */
    char *      memArea;                /* pre allocated memory area */
    int         memSize;                /* pre allocated memory size */
    } MBUF_DESC;
 
typedef struct          /* FBUFFER_ID - fast buffer */
    {
    char*	poolAddr;   /* starting address of buffer pool */
    char*	mallocAddr;   /* address of system memory malloc buffer pool */
    int		nBufs;
    int		bufSize;
    int         numLists;
    MBUF_DESC  *mbclusters;
    RINGL_ID   freeList[20];
    long       listSizes[20];
    long       clCnt[20];
    long       clFailCnt[20];
    long	totalSize;	/* total malloc'd space */
    long        numMallocs;
    long        maxMalloc;
    long        memReserveLevel; /* leave this much in system memory reserve */
    } MBUFFER_OBJ;

/* END_HIDDEN */

typedef MBUFFER_OBJ *MBUFFER_ID;


/* --------- ANSI/C++ compliant function prototypes --------------- */

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

/* extern MBUFFER_ID mBufferCreate(MBUF_DESC *clusters, int numClusterSize, int eventid); */
extern MBUFFER_ID mBufferCreate(MBUF_DESC *clusters, int numClusterSize, int eventid, int memReserveMB);
extern void mBufferDelete(MBUFFER_ID fBufferId);
extern char *mBufferGet(register MBUFFER_ID mBufferId, unsigned int size);
extern int mBufferReturn(MBUFFER_ID fBufferId,long item);
extern void mBufferClear(MBUFFER_ID fBufferId);
extern long  mBufferMaxClSize(MBUFFER_ID mBufferId);
extern int  mBufferInfo(MBUFFER_ID mBufferId,MBUF_INFO *mBuffInfo);

/* --------- NON-ANSI/C++ prototypes ------------  */
#else
 
extern MBUFFER_ID fBufferCreate();
extern void mBufferDelete();
extern char *mBufferGet();
extern int mBufferReturn();
extern long  mBufferMaxClSize();
extern int  mBufferInfo();

#endif  /* __STDC__ */
 
#ifdef __cplusplus
}
#endif

#endif /* INCrngBlkLibh */
