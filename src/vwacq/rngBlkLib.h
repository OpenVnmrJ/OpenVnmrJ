/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCrngBlkLibh
#define INCrngBlkLibh


/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* HIDDEN */

/* typedefs */

typedef struct          /* RING_BLKING - blocking ring buffer */
    {
    char*	pRngIdStr;
    RING_ID	pToRngBuf;
    SEM_ID	pToSyncOK2Write;
    SEM_ID	pToSyncOK2Read;
    SEM_ID	pToRngBlkMutex;
    short	readBlocked;
    short	writeBlocked;
    } RING_BLKING;

/* END_HIDDEN */

typedef RING_BLKING *RINGBLK_ID;

/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

 
IMPORT    RINGBLK_ID   rngBlkCreate (int nbytes,char* idtsr);
IMPORT    VOID         rngBlkDelete (RINGBLK_ID ringId);
IMPORT    VOID         rngBlkFlush (RINGBLK_ID ringId);
IMPORT    int          rngBlkBufGet (RINGBLK_ID rngId, char *buffer, int maxbytes);
IMPORT    int          rngBlkBufPut (RINGBLK_ID rngId, char *buffer, int nbytes);
/*
IMPORT    BOOL         rngIsEmpty (RINGBLK_ID ringId);
IMPORT    BOOL         rngIsFull (RINGBLK_ID ringId);
IMPORT    int          rngFreeBytes (RINGBLK_ID ringId);
IMPORT    int          rngNBytes (RINGBLK_ID ringId);
IMPORT    VOID         rngPutAhead (RINGBLK_ID ringId, char byte, int offset);
IMPORT    VOID         rngMoveAhead (RINGBLK_ID ringId, int n);
*/
 
/* --------- NON-ANSI/C++ prototypes ------------  */

#else
 
IMPORT    RINGBLK_ID   rngBlkCreate ();
IMPORT    VOID         rngBlkDelete ();
IMPORT    VOID         rngBlkFlush ();
IMPORT    int          rngBlkBufGet ();
IMPORT    int          rngBlkBufPut ();
/*
IMPORT    BOOL         rngIsEmpty ();
IMPORT    BOOL         rngIsFull ();
IMPORT    int          rngFreeBytes ();
IMPORT    int          rngNBytes ();
IMPORT    VOID         rngPutAhead ();
IMPORT    VOID         rngMoveAhead ();
*/
 
#endif  /* __STDC__ */
 
#ifdef __cplusplus
}
#endif

#endif /* INCrngBlkLibh */
