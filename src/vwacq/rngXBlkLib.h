/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCrngXBlkLibh
#define INCrngXBlkLibh


/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* HIDDEN */

/* typedefs */

typedef struct          /* RING_BLKING - blocking ring buffer */
    {
    char*	pRngIdStr;
    SEM_ID	pToSyncOK2Write;
    SEM_ID	pToSyncOK2Read;
    SEM_ID	pToRngBlkMutex;
    short	readBlocked;
    short	writeBlocked;
    int		wvEventID;	/* WindView Event ID, to indicate the user  */
    int         blkTilNentries;
    int         triggerLevel;
    int         forcedUnBlock;
    int pToBuf;         /* offset from start of buffer where to write next */
    int pFromBuf;       /* offset from start of buffer where to read next */
    int bufSize;        /* size of ring in elements */
    int maxSize;        /* original & maximum size of ring in elements */
    long *rBuf;         /* pointer to start of buffer */
    } RINGX_BLKING;

/* END_HIDDEN */

typedef RINGX_BLKING *RINGXBLK_ID;

/* --------- ANSI/C++ compliant function prototypes --------------- */

/*******************************************************************************
*
* RNG_LONG_GET - get one element from a ring buffer
*
* This macro gets a single element from the specified ring buffer.
* Must supply temporary variable (register int) 'fromP'.
*
* RETURNS: 1 if there was a element in the buffer to return, 0 otherwise
*
* NOMANUAL
*/

#define RNG_LONG_GET(ringId, pCh, fromP)		\
    (						\
    fromP = (ringId)->pFromBuf,			\
    ((ringId)->pToBuf == fromP) ?		\
	0 					\
    :						\
	(					\
	*pCh = (ringId)->rBuf[fromP],		\
	(ringId)->pFromBuf = ((++fromP == (ringId)->bufSize) ? 0 : fromP), \
	1					\
	)					\
    )


/*******************************************************************************
*
* RNG_LONG_PEEK - get one element from a ring buffer, but do not remove from buffer
*
* This macro gets a single element from the specified ring buffer.
* But Does not removed it from the ring buffer. (i.e. read pointer is not incremented
* Must supply temporary variable (register int) 'fromP'.
*
* RETURNS: 1 if there was a element in the buffer to return, 0 otherwise
*
* NOMANUAL
*/

#define RNG_LONG_PEEK(ringId, pCh, fromP)		\
    (						\
    fromP = (ringId)->pFromBuf,			\
    ((ringId)->pToBuf == fromP) ?		\
	0 					\
    :						\
	(					\
	*pCh = (ringId)->rBuf[fromP],		\
	1					\
	)					\
    )


/*******************************************************************************
*
* RNG_LONG_PUT - put one element into a ring buffer
*
* This macro puts a single element into the specified ring buffer.
* Must supply temporary variable (register int) 'toP'.
*
* RETURNS: 1 if there was room in the buffer for the element, 0 otherwise
*
* NOMANUAL
*/

#define RNG_LONG_PUT(ringId, ch, toP)		\
    (						\
    toP = (ringId)->pToBuf,			\
    (toP == (ringId)->pFromBuf - 1) ?		\
	0 					\
    :						\
	(					\
    	(toP == (ringId)->bufSize - 1) ?	\
	    (					\
	    ((ringId)->pFromBuf == 0) ?		\
		0				\
	    :					\
		(				\
		(ringId)->rBuf[toP] = ch,	\
		(ringId)->pToBuf = 0,		\
		1				\
		)				\
	    )					\
	:					\
	    (					\
	    (ringId)->rBuf[toP] = ch,		\
	    (ringId)->pToBuf++,			\
	    1					\
	    )					\
	)					\
    )

/* function declarations */

#if defined(__STDC__) || defined(__cplusplus)

 
IMPORT    RINGXBLK_ID   rngXBlkCreate (int nbytes,char* idtsr,int wveventid,int pendlevel);
IMPORT    VOID         rngXBlkDelete (RINGXBLK_ID ringId);
IMPORT    VOID         rngXBlkFlush (RINGXBLK_ID ringId);
IMPORT    int          rngXBlkBufGet (RINGXBLK_ID rngId, long *buffer, int maxelem);
IMPORT    int          rngXBlkBufPut (RINGXBLK_ID rngId, long *buffer, int nelem);
IMPORT    int 	       rngXBlkNElemZBlk (RINGXBLK_ID ringId, long **ptrBuf);
IMPORT    int          rngXMoveAhead (RINGXBLK_ID ringId, int n);
IMPORT    int          rngXResetBlkLevel(RINGXBLK_ID rngd);
IMPORT    int          rngXUnBlock (RINGXBLK_ID rngd);
/*
IMPORT    BOOL         rngXIsEmpty (RINGXBLK_ID ringId);
IMPORT    BOOL         rngXIsFull (RINGXBLK_ID ringId);
IMPORT    int          rngXFreeElem (RINGXBLK_ID ringId);
IMPORT    int          rngXNElem (RINGXBLK_ID ringId);
IMPORT    VOID         rngXPutAhead (RINGXBLK_ID ringId, long elem, int offset);
IMPORT    VOID         rngXMoveAhead (RINGXBLK_ID ringId, int n);
*/
 
/* --------- NON-ANSI/C++ prototypes ------------  */

#else
 
IMPORT    RINGXBLK_ID   rngXBlkCreate ();
IMPORT    VOID         rngXBlkDelete ();
IMPORT    VOID         rngXBlkFlush ();
IMPORT    int          rngXBlkBufGet ();
IMPORT    int          rngXBlkBufPut ();
IMPORT    int 	       rngXBlkNElemZBlk ();
IMPORT    int          rngXMoveAhead ();
/*
IMPORT    BOOL         rngXIsEmpty ();
IMPORT    BOOL         rngXIsFull ();
IMPORT    int          rngXFreeElem ();
IMPORT    int          rngXNElem ();
IMPORT    VOID         rngXPutAhead ();
IMPORT    VOID         rngXMoveAhead ();
*/
 
#endif  /* __STDC__ */
 
#ifdef __cplusplus
}
#endif

#endif /* INCrngBlkLibh */
