/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCfBufferLibh
#define INCfBufferLibh

#include "rngXBlkLib.h"

typedef struct          /* FBUFFER_ID - fast buffer */
    {
    char*	poolAddr;   /* starting address of buffer pool */
    char*	mallocAddr;   /* address of system memory malloc buffer pool */
    int		nBufs;
    int		bufSize;
    RINGXBLK_ID  freeList;
    long	totalSize;	/* total malloc'd space */
    long	freeLstSiz;	/* ring buffer size */
    } FBUFFER_OBJ;

/* END_HIDDEN */

typedef FBUFFER_OBJ *FBUFFER_ID;


/* --------- ANSI/C++ compliant function prototypes --------------- */

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

extern FBUFFER_ID fBufferCreate(int nBuffers, int bufSize, char* memAddr,int wveventid);
extern int  fBufferReUse(FBUFFER_ID pFBufferId,int nBuffers, int bufSize);
extern void fBufferDelete(FBUFFER_ID fBufferId);
extern char *fBufferGet(FBUFFER_ID fBufferId);
extern void fBufferReturn(FBUFFER_ID fBufferId,long item);
extern void fBufferClear(FBUFFER_ID fBufferId);
extern int  fBufferFreeBufs(FBUFFER_ID fBufferId);
extern int  fBufferUsedBufs(FBUFFER_ID fBufferId);
extern long fBufferMaxSize(FBUFFER_ID fBufferId);

/* --------- NON-ANSI/C++ prototypes ------------  */
#else
 
extern FBUFFER_ID fBufferCreate();
extern int  fBufferReUse();
extern void fBufferDelete();
extern char *fBufferGet();
extern void fBufferReturn();
extern void fBufferClear();
extern int  fBufferFreeBufs();
extern int  fBufferUsedBufs ();
extern long fBufferMaxSize();

#endif  /* __STDC__ */
 
#ifdef __cplusplus
}
#endif

#endif /* INCrngBlkLibh */
