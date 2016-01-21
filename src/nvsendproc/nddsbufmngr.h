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


#ifndef INC_nddsbufmngr_h
#define INC_nddsbufmngr_h

#include "rngLLib.h"
#include "rngBlkLib.h"

typedef struct          /* NDDSBUFMNGR_ID - NDDS sub callback buffer manager */
    {
    char*       poolAddr;   /* starting address of buffer pool */
    char*       mallocAddr;   /* address of system memory malloc buffer pool */
    int         nBufs;
    int         bufSize;
    RINGL_ID    freeList;
    RINGBLK_ID  msgeList;
    long        totalSize;      /* total malloc'd space */
    long        freeLstSize;     /* ring buffer size */
    long        msgLstSize;     /* ring buffer size */
    } NDDSBUFMNGR_OBJ;

/* END_HIDDEN */

typedef NDDSBUFMNGR_OBJ *NDDSBUFMNGR_ID;


/* --------- ANSI/C++ compliant function prototypes --------------- */

/* ------------- Make C header file C++ compliant ------------------- */
#if defined(__STDC__) || defined(__cplusplus)
 
extern NDDSBUFMNGR_ID nddsBufMngrCreate(int nBuffers, int bufSize);
extern char *msgeBufGet(NDDSBUFMNGR_ID bufMngrId);
extern int msgeBufReturn(NDDSBUFMNGR_ID pBufMngrId,char *item);
extern int msgePost(NDDSBUFMNGR_ID pBufMngrId, char *item);
extern char *msgeGet(NDDSBUFMNGR_ID pBufMngrId);
extern int msgeGetWillPend(NDDSBUFMNGR_ID pBufMngrId);
extern int msgeGetIsPended(NDDSBUFMNGR_ID pBufMngrId);
 
/* --------- NON-ANSI/C++ prototypes ------------  */
#else

#endif  /* __STDC__ */

#ifdef __cplusplus     
}
#endif
 
#endif /* INCrngBlkLibh */

