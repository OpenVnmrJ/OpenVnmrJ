/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCdmah
#define INCdmah

/* ------------- Make C header file C++ compliant ------------------- */
#include <drv/vme/vmechip2.h> 

/*******************************************************/
/* Transfer Mode of DMA				       */ 
/*******************************************************/
#define D16	1
#define D32	2
#define BLK32	3
#define BLK64	4

/********************************/
/* Transfer Direction of DMA    */
/********************************/
#define LOCAL_TO_VME	0x200
#define VME_TO_LOCAL	0x000

/********************************/
/* VME Address Incremented      */
/********************************/
#define INC_VME_NOT	0
#define INC_VME		1

/********************************/
/* Interrupt Levels for the DMA */
/********************************/
#define INT_LEVEL1	1
#define INT_LEVEL2	2
#define INT_LEVEL3	3
#define INT_LEVEL4	4
#define INT_LEVEL5	5
#define INT_LEVEL6	6
#define INT_LEVEL7	7

/**********************************/
/* Bus Request Levels for the DMA */
/**********************************/
#define BREQ_LEVEL0		DMACR1_DREQ_L0	/* VMEbus req level 0 for dma */
#define BREQ_LEVEL1		DMACR1_DREQ_L1	/* VMEbus req level 1 for dma */
#define BREQ_LEVEL2		DMACR1_DREQ_L2	/* VMEbus req level 2 for dma */
#define BREQ_LEVEL3		DMACR1_DREQ_L3	/* VMEbus req level 3 for dma */

/*********************************/
/* Bus Request Modes for the DMA */
/*********************************/
#define	DMA_FAIR	DMACR1_DFAIR	/* VMEbus request on no request */
#define	DMA_NOT_FAIR	0		/* VMEbus request as needed */

/*********************************/
/* Bus Release Modes for the DMA */
/*********************************/
#define	REL_TO_AND_REQ	DMACR1_TIMER_BRX     /* VMEbus release on timeout and
					        a bus req at this same level.
                                                the dma will not release for
                                                levels of greater priority  
                                                and it ignores BCLR*       */
#define	REL_TO		DMACR1_TIMER         /* VMEbus release on timeout */
#define	REL_REQ		DMACR1_BRX  	     /* VMEbus release on a bus req at 
						this same level */
#define	REL_TO_OR_REQ	DMACR1_TIMER_OR_BRX  /* VMEbus release on timeout or
					        a bus req at this same level */

/*******************/
/* DMAC bus timing */
/*******************/
#define	DMA_OFF_0US	TIMEOUTCR_OFF_0US	/* DMAC off VMEbus for 0 us */
#define	DMA_OFF_16US	TIMEOUTCR_OFF_16US	/* DMAC off VMEbus for 16 us */
#define	DMA_OFF_32US	TIMEOUTCR_OFF_32US	/* DMAC off VMEbus for 32 us */
#define	DMA_OFF_64US	TIMEOUTCR_OFF_64US	/* DMAC off VMEbus for 64 us */
#define	DMA_OFF_128US	TIMEOUTCR_OFF_128US	/* DMAC off VMEbus for 128 us */
#define	DMA_OFF_256US	TIMEOUTCR_OFF_256US	/* DMAC off VMEbus for 256 us */
#define	DMA_OFF_512US	TIMEOUTCR_OFF_512US	/* DMAC off VMEbus for 512 us */
#define	DMA_OFF_1024US	TIMEOUTCR_OFF_1024US	/* DMAC off VMEbus for 1024us */

#define	DMA_ON_16US	TIMEOUTCR_ON_16US	/* DMAC on VMEbus for 16 us */
#define	DMA_ON_32US	TIMEOUTCR_ON_32US	/* DMAC on VMEbus for 32 us */
#define	DMA_ON_64US	TIMEOUTCR_ON_64US	/* DMAC on VMEbus for 64 us */
#define	DMA_ON_128US	TIMEOUTCR_ON_128US	/* DMAC on VMEbus for 128 us */
#define	DMA_ON_256US	TIMEOUTCR_ON_256US	/* DMAC on VMEbus for 256 us */
#define	DMA_ON_512US	TIMEOUTCR_ON_512US	/* DMAC on VMEbus for 512 us */
#define	DMA_ON_1024US	TIMEOUTCR_ON_1024US	/* DMAC on VMEbus for 1024 us */
#define	DMA_ON_TIL_DONE	TIMEOUTCR_ON_DONE	/* DMAC on VMEbus until done */

/*******************************************************/
/* Additional Address Modifier Codes not defined in    */
/*  vme.h         				       */
/*******************************************************/
#define VME_AM_STD_SUP_BLOCK 0x3F
#define VME_AM_STD_SUP_D64_BLOCK 0x3C
#define VME_AM_STD_USR_BLOCK 0x3B
#define VME_AM_STD_USR_D64_BLOCK 0x38
#define VME_AM_EXT_SUP_BLOCK 0x0F
#define VME_AM_EXT_SUP_D64_BLOCK 0x0C
#define VME_AM_EXT_USR_BLOCK 0x0B
#define VME_AM_EXT_USR_D64_BLOCK 0x08
/*******************************************************/
/* Address Modifier Codes are defined in vme.h         */
/*  OR in these codes for VME64 or block transfer mode */
/*******************************************************/
#define	NOT_BLK		0	/* Normal VME transfers using D32 */
#define	NOT_BLK_D16	0x100 	/* Normal VME transfers using D16 */
#define	BLK_D32		0x40	/* 32 bit block transfers */
#define	BLK_D64		0xc0	/* VME_64 bit block transfers */

/************************************/
/* Snoop Options for DMA controller */
/************************************/
#define	DMA_NO_SNOOP	0     /* No snooping for uncached data*/
#define	DMA_SNOOP_COPYBACK	DMACR2_SINK_DATA  /* Use this for snooping cached data*/
#define	DMA_SNOOP_WRITETHROUGH	DMACR2_INVALIDATE  /* Use this for snooping cached data*/

/******************************/
/* Interrupt or Polled Option */
/******************************/
#define	POLLED_DMA		0
#define INTERRUPT_DRIVEN_DMA	1

/**********************************************************/
/* Status values returned when an error occurs during dma */
/**********************************************************/
#define	DMA_TIMED_OUT	1	/* 10 min timeout waiting for dma to complete */
#define	VME_BUS_ERROR	2	/* VMEbus error occurred during transfer */
#define	LOCAL_TIMEOUT	3	/* local bus timed out during transfer */
#define	OFF_BD_TEA	4	/* off board TEA occurred during transfer */
#define	PARITY_ERROR	5	/* Parity error occurred during transfer */
#define	TEA		6	/* TEA occurred during transfer */


#ifdef __cplusplus
extern "C" {
#endif

/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

extern int dmaInit(void);

extern int dma(ulong_t Source,ulong_t Destination, ulong_t byteCount, 
			int XfrDir, int AMcode, int timeOut);

#else
/* --------- NON-ANSI/C++ prototypes ------------  */

extern int dmaInit();
extern int dma();

#endif

#ifdef __cplusplus
}
#endif

#endif
