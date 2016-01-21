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

#ifndef INCrngBlkLibh
#define INCrngBlkLibh

#ifndef VNMRS_WIN32
#include <pthread.h>
#else
#include <windows.h>
#endif


/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* HIDDEN */

/* typedefs */

#define FALSE 0
#define TRUE 1


#ifndef VNMRS_WIN32

typedef struct          /* RING_BLKING - blocking ring buffer */
    {
    char*	pRngIdStr;
    pthread_mutex_t     mutex;          /* Mutex for ring data */
    pthread_cond_t      syncOk2Write;  /* Wait for buffers to be available */
    pthread_cond_t      syncOk2Read;  /* Wait for buffers to be available */
    short	readBlocked;
    short	writeBlocked;
    int         blkTilNentries;
    int         triggerLevel;
    int         forcedUnBlock;
    int pToBuf;         /* offset from start of buffer where to write next */
    int pFromBuf;       /* offset from start of buffer where to read next */
    int bufSize;        /* size of ring in elements */
    int maxSize;        /* original & maximum size of ring in elements */
    long *rBuf;         /* pointer to start of buffer */
    } RING_BLKING;

#else /* VNMRS_WIN32 */

typedef struct          /* RING_BLKING - blocking ring buffer */
    {
    char*	pRngIdStr;
    HANDLE      hMutex;                 /* Mutex for ring data */
    HANDLE      hSyncOk2WriteEvent;     /* Wait for buffers to be available */
    HANDLE      hSyncOk2ReadEvent;      /* Wait for buffers to be available */
    BOOL	readBlocked;
    BOOL	writeBlocked;
    int         blkTilNentries;
    int         triggerLevel;
    int         forcedUnBlock;
    int pToBuf;         /* offset from start of buffer where to write next */
    int pFromBuf;       /* offset from start of buffer where to read next */
    int bufSize;        /* size of ring in elements */
    int maxSize;        /* original & maximum size of ring in elements */
    long *rBuf;         /* pointer to start of buffer */
    } RING_BLKING;

#endif /* VNMRS_WIN32 */

/* END_HIDDEN */

typedef RING_BLKING *RINGBLK_ID;

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

#if defined(__STDC__) || defined(__cplusplus) || defined(VNMRS_WIN32)

 
extern    RINGBLK_ID   rngBlkCreate (int nbytes,char* idtsr,int pendlevel);
extern    void         rngBlkDelete (RINGBLK_ID ringId);
extern    int          rngBlkFlush (RINGBLK_ID ringId);
extern    int          rngBlkBufGet (RINGBLK_ID rngId, long *buffer, int maxelem);
extern    int          rngBlkBufPut (RINGBLK_ID rngId, long *buffer, int nelem);
extern    int          rngIsEmpty (RINGBLK_ID ringId);
extern    int          rngIsFull (RINGBLK_ID ringId);
extern    int          rngFreeElem (RINGBLK_ID ringId);
extern    int          rngNElem (RINGBLK_ID ringId);
extern    int 	       rngBlkIsGetPended (RINGBLK_ID ringId);
extern    int 	       rngBlkIsPutPended (RINGBLK_ID ringId);
extern    int          rngBlkNElem (register RINGBLK_ID ringId);
extern    int          rngBlkIsEmpty (register RINGBLK_ID ringId);
 
#endif  /* __STDC__ */
 
#ifdef __cplusplus
}
#endif

#endif /* INCrngBlkLibh */
