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
#ifndef INCcntrlfifobufh
#define INCcntrlfifobufh

#include <semLib.h>
#include <msgQLib.h>

/* defines for all the controller FIFO buffer allocations */
#define MASTER_NUM_FIFO_BUFS 64
#define MASTER_SIZE_FIFO_BUFS 1024
#define RF_NUM_FIFO_BUFS 128
#define RF_SIZE_FIFO_BUFS 1024
#define PFG_NUM_FIFO_BUFS 128
#define PFG_SIZE_FIFO_BUFS 1024
#define GRADIENT_NUM_FIFO_BUFS 128
#define GRADIENT_SIZE_FIFO_BUFS 1024

/* should give an approx duration of 12 us for minimum DMA, longer than ISR time */ 
#define MINIMUM_ENTRIES_FOR_DMA 128  

// #define DDR_NUM_FIFO_BUFS 64
// #define DDR_NUM_FIFO_BUFS 1024   /* temp fix for bug #379 */
// #define DDR_SIZE_FIFO_BUFS 1024
#define DDR_NUM_FIFO_BUFS 128
#define DDR_SIZE_FIFO_BUFS 4096


#define CNTRL_DMA_MODE 0
#define CNTRL_DECODER_MODE 1
#define CNTRL_PIO_MODE 2

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

typedef int (*CNTRLBUF_DECODEFUNC)(UINT32 *, int);

/* ------------------- Fifo Buffer Object Structure ------------------- */
typedef struct {
          unsigned long *pWrkBuf; 	/* Present Working Buffer (fifo word being put into) */
          unsigned long WrkBufEntries;   /* The number of entries in ther working buffer */
          unsigned long *IndexAddr;	/* Pointer into buffer for copying */
          unsigned long **pBufArray;	/* ptr to an array of fifo buffer pointers */
          unsigned long  totalMemAlloc; /* total memory allocated for buffers */
          unsigned long  BufSize;	/* Size of buffers */
          unsigned long  NumOfBufs;	/* Number of Buffers */
	  SEM_ID  pBufMutex;   	/* Mutex Semaphore for STM Object */
          MSG_Q_ID pBufsFree;	/* available Buffer addresses  */
          CNTRLBUF_DECODEFUNC pFifoWordDecoder;
          int queueMode;
} FIFO_BUF_OBJ;

typedef FIFO_BUF_OBJ *FIFOBUF_ID;

 /* unsigned long *pWrkBufEntries; Point to the number of entries in ther working buffer */
 /*       MSG_Q_ID pBufsRdy; 	 Msg Q of Buffers Ready to Stuff */
 /* VOIDFUNCPTR    modeFunc;       /* function to be called to transfer buff to FIFO */


/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)
extern FIFOBUF_ID cntlrFifoBufCreate(unsigned long numBuffers, unsigned long bufSize, int dmaChannel, volatile unsigned int* pFifoWrite );
extern int 	  cntrlFifoBufInit(FIFOBUF_ID pFifoBufId);
extern void 	  cntrlFifoBufDelete(FIFOBUF_ID pFifoBufId);
extern int        cntrlFifoDmaChanGet(void);
extern void       cntrlFifoBufSetDecoder(FIFOBUF_ID pFifoBufId, CNTRLBUF_DECODEFUNC funcptr);
extern int        cntrlFifoBufModeSet(FIFOBUF_ID pFifoBufId,int mode);
extern void       cntrlFifoBufPut(FIFOBUF_ID pFifoBufId, unsigned long *words , long nWords );
extern void       cntrlFifoBufGet(FIFOBUF_ID pFifoBufId, unsigned long **BufferAddr, long *entries);
extern int        cntrlFifoListCopy(FIFOBUF_ID pFifoBufId, long *buffer, int size, int ntimes,int rem, int startfifo);
extern int        cntrlFifoBufCopy(FIFOBUF_ID pFifoBufId, long *buffer, int size, int ntimes, int rem, int startfifo);
extern int        cntrlFifoBufXfer(FIFOBUF_ID pFifoBufId, long *buffer, int size, int ntimes, int rem, int startfifo);
extern int       cntrlFifoXferCopy(FIFOBUF_ID pFifoBufId, long *buffer, int size, int ntimes,int rem, int startfifo);
extern int       cntrlFifoXferSGList(FIFOBUF_ID pFifoBufId, long *buffer, int size, int ntimes,int rem, int startfifo);
extern void       cntrlFifoBufForceRdy(FIFOBUF_ID pFifoBufId);
extern int	  cntrlFifoBufReset(FIFOBUF_ID pFifoBufId);
extern unsigned long cntrlFifoBufWkEntries(FIFOBUF_ID pFifoBufId);
extern void 	  cntrlFifoBufShow(FIFOBUF_ID pFifoBufId, int level);
extern void 	  cntrlFifoBufShwResrc(FIFOBUF_ID pFifoBufId, int indent);

#else
/* --------- NON-ANSI/C++ prototypes ------------  */

#endif

#ifdef __cplusplus
}
#endif

#endif

