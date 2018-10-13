/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCfifobufh
#define INCfifobufh

#include <semLib.h>
#include <msgQLib.h>

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif


/* ------------------- Fifo Buffer Object Structure ------------------- */
typedef struct {
	char* 	pIdStr; 	/* user identifier string */
	char* 	pSID; 		/* SCCS ID string */
 unsigned long *pWrkBuf; 	/* Present Working Buffer (fifo word being put into) */
 unsigned long *pWrkBufEntries; /* Point to the number of entries in ther working buffer */
 unsigned long *IndexAddr;	/* Pointer into buffer for copying */
 unsigned long *pBufArray;	/* ptr to array of fifo buffer */
 unsigned long  BufSize;	/* Fixed size of buffers */
 unsigned long  NumOfBufs;	/* Number of Buffers */
	SEM_ID  pBufMutex;   	/* Mutex Semaphore for STM Object */
       MSG_Q_ID pBufsFree;	/* available Buffer addresses  */
       MSG_Q_ID pBufsRdy; 	/* Msg Q of Buffers Ready to Stuff */
} FIFO_BUF_OBJ;

typedef FIFO_BUF_OBJ *FIFOBUF_ID;


/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)
extern FIFOBUF_ID fifoBufCreate(unsigned long numBuffers, unsigned long bufSize, char* idstr);
extern int 	  fifoBufInit(FIFOBUF_ID pFifoBufId);
extern void 	  fifoBufDelete(FIFOBUF_ID pFifoBufId);
extern void       fifoBufPut(FIFOBUF_ID pFifoBufId, unsigned long *words , long nWords );
extern void       fifoBufGet(FIFOBUF_ID pFifoBufId, unsigned long **BufferAddr, long *entries);
extern void       fifoBufForceRdy(FIFOBUF_ID pFifoBufId);
extern int	  fifoBufReset(FIFOBUF_ID pFifoBufId);
extern unsigned long fifoBufWkEntries(FIFOBUF_ID pFifoBufId);
extern void 	  fifoBufShow(FIFOBUF_ID pFifoBufId, int level);
extern void 	  fifoBufShwResrc(FIFOBUF_ID pFifoBufId, int indent);

#else
/* --------- NON-ANSI/C++ prototypes ------------  */

#endif

#ifdef __cplusplus
}
#endif

#endif

