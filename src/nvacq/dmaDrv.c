/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
Id: dmaDrv.c,v 1.8 2004/04/30 15:05:12 rthrift Exp rthrift
=========================================================================
FILE: dmaDrv.c
=========================================================================
PURPOSE:
	Provide driver routines for managing DMA channels.

Externally available modules in this file:
	Basic DMA channel management:
	-----------------------------
	dmaInit			  - Initialize DMA controller.
	dmaGetChannel	  - Request an available DMA channel for use.
	dmaGetChannelInfo - Get a pointer to a channel's Info structure
	dmaFreeChannel	  - Release a DMA channel, previously claimed
					    by dmaGetChannel.
	dmaCleanup		  - Clean up tasks, queues, semaphores before exit
	dmaQueueFree	  - Clean out a DMA transfer task queue
	sendDMA			  - Call from application space, for DMA transfer

	Scatter/Gather Lists Management:
	--------------------------------
	dmaSGNodeCreate   - Create a new scatter/gather list node
	dmaSGNodeAppend   - Append a new scatter/gather node to a SG list

	dmaSGListCreate	  - Create a new scatter/gather list structure
	dmaSGListRemove   - Delete and free an entire scatter/gather list

	dmaSGQueueAppend  - Add a scatter/gather list to a SG list queue
	dmaSGQueueFree    - Delete and free a DMA channel's entire queue
	                    of SG lists

	Available but currently commented out:
	--------------------------------------
	dmaSGNodeInsert	     - Insert an SG node into an SG list after a
						   node already in the list (instead of at the end)
	dmaSGNodeRemove      - Remove a specific SG node from an SG list
	dmaSGListRemoveFirst - Remove the first node from an SG list
 
Internal support modules in this file:
	dmaISR 			  - Interrupt service for DMA channels
	runDMA			  - Task to start the DMA hardware for a transfer
	sendDMATask       - Task to prepare for a DMA transfer.
	endDMATask        - Task to clean up after a DMA transfer.
 
COMMENTS:
	Specific to PowerPC 405GPr CPU.
	Standard ANSI C/C++ compilation is assumed.

AUTHOR:
	Robert L. Thrift, 2003
=========================================================================
     Copyright (c) 2003, Varian Associates, Inc. All Rights Reserved.
     This software contains proprietary and confidential information
            of Varian Associates, Inc. and its contributors.
  Use, disclosure and reproduction is prohibited without prior consent.
=========================================================================
*/
#ifndef INSTRUMENT
 #define INSTRUMENT
#endif
#define DMA_DRV_C
#include <vxWorks.h>		/* General VxWorks definitions	*/
#include <stdio.h>
#include <arch/ppc/ivPpc.h>	/* INUM_TO_IVEC		*/
#include <cacheLib.h>		/* Cache management	*/
#include <errnoLib.h>		/* errno definitions	*/
#include <taskLib.h>
#include <drv/timer/ppcDecTimer.h>	/* sysClkRateGet	*/
#include <intLib.h>			/* intConnect		*/
#ifdef INSTRUMENT
#include "wvLib.h"
#include "instrWvDefines.h"
#endif
#include "rngLLib.h"			/* long ring buffer lib */
#include "dmaDcr.h"			/* DMA registers and control bits */
#include "uicDcr.h"			/* UIC registers and control bits */
#include "dmaDrv.h"			/* DMA driver structures, constants, etc. */
#include "dmaReg.h"			/* DMA register interface routines	*/
#include "FIFOdrv.h"		/* FIFO control routines	*/
#ifdef XXXX
#include "rf.h"				/* RF FIFO address offsets */
#endif
#include "logMsgLib.h"
#include "dmaMsgQueue.h"	/* Message queue handlers	*/
#include "dmaDebugOptions.h"

#define HARD_ERROR    15

/* ------- Array of structs for managing PPC405GPr DMA channels ------- */
/* -------- One struct in the array for each hardware channel --------- */
dmainfo_t dmaInfo[MAX_DMA_CHANNELS];

/* --------- Array of heads of queues of scatter/gather lists --------- */
/* --------------- One queue for each hardware channel ---------------- */
/* static qhead_t dmaQueue[MAX_DMA_CHANNELS]; */
/* ---------------------------------------------------------------------*/

static char msgStr[MAX_ERR_MSG_LEN];/* Place to construct msg strings	*/
static int dma_initialized = 0;		/* Set to 1 by dmaInit()	*/
static int runDMATaskID = 0;		/* runDMA task ID		*/
static STATUS runDMAStatus = OK;	/* runDMA task status		*/
static int sendDMATaskID = 0;		/* sendDMA task ID		*/
static int dmaErrMsgTaskID = 0;		/* DMA err messages logging task */
static int endDMATaskID = 0;		/* DMA transfer cleanup task	*/
static int	clkRate;		/* System clock rate, ticks/sec. */
static int endDMAPriority;		/* Priority for endDMA task	*/
static int logMsgPriority;		/* Priority for message logger */
static int runDMAPriority;		/* Priority for runDMA task	*/
static int sendDMAPriority;		/* Priority for sendDMATask	*/
static int myTaskPriority;		/* Priority for DMA driver	*/
extern MSG_Q_ID endDMAQ;

/* Forward references */
STATUS dmaQueueAppend(int, txDesc_t *);
int returnBuffersAndTxDesc(txDesc_t *desc );

/*
=========================================================================
NAME: dmaISR
=========================================================================
PURPOSE:
	Interrupt service for DMA end-of-transfer condition.

DETAILS:
	Clears channel enable and channel interrupt enable bits.
	Posts a message to endDMATask to clean up after the transfer.

COMMENTS:
	A parameter (specified by intConnect) is passed into the ISR to
	identify the interrupting channel.

AUTHOR: 
	Robert L. Thrift (2003)
=========================================================================
*/
/*
 * I've tested using separate ISr for each channel avoid switch construct, however the times
 * are practically identical after the ISR are cached, so I it's simpler to understand the one ISR
 * so I'm leaving it that way.
 *  total ISR time worst case: 28 us down to 10 us , execDma 6 down to 3.2 us
 *     greg Brissey 2/23/06
 */
void dmaISR( int channel)      /* Parameter giving channel no.	*/
{
	UINT32 ctrl, mask, status, ibit, chan;
	txDesc_t	*desc, *nextDesc;
        register int toP;

#ifdef DEBUG_DMA_DRV
	dmaLogMsg("dmaISR: entry");
#endif
#ifdef INSTRUMENT
	wvEvent((event_t) EVENT_DMA_DONE_CHAN+channel, NULL, 0);
#endif
	errnoSet(0);					/* Clear errno before starting	*/
        /* no need to do this, it's handled in the OS ISR that call this one */
        /* and sysDcrUicsrClear() type calls have been replace with sysDcrOutLong(UIC_SR, value) */
        /* type calls e.g. sysDcrOutLong(UIC_SR, 1 << (INT_LEVEL_MAX - vector)) */
	/* ibit = BITSET((DMA_CHAN_INT_BASE + channel)); */
	/* sysDcrUicsrClear(ibit);			/* Clear UIC int. status bit	*/
  
        /* for timing it is close to one microsecond per instruction */
        /* logMsg() - 10 us */
        /*  RNG_LONG_GET  1.6 us */
        /* msgQSend 2 - 5.3 us */

	switch (channel) {   
		case 0:
			ctrl = sysDcrDmacr0Get() & (~(DMA_CR_CE | DMA_CR_CIE));
			sysDcrDmacr0Set(ctrl);		/* Disable chan. 0		*/

			/* -------------- Check channel's error bit --------------- */
			status = sysDcrDmasrGet();
			if (status & DMA_SR_RI_0)
			{
				/* ------- FIXME -- Handle indicated DMA error -------- */
				errnoSet(EFAULT);
			}

			/* ---- Clear status bits that can retrigger interrupt ---- */
			mask = (~(DMA_SR_CS_0 | DMA_SR_TS_0 | DMA_SR_RI_0));
			sysDcrDmasrSet(status & mask);
			break;

		case 1:
			ctrl = sysDcrDmacr1Get() & (~(DMA_CR_CE | DMA_CR_CIE));
			sysDcrDmacr1Set(ctrl);		/* Disable chan. 1	*/

			/* -------------- Check channel's error bit --------------- */
			status = sysDcrDmasrGet();
			if (status & DMA_SR_RI_1)
			{
				/* ------- FIXME -- Handle indicated DMA error -------- */
				errnoSet(EFAULT);
			}

			/* ---- Clear status bits that can retrigger interrupt ---- */
			mask = (~(DMA_SR_CS_1 | DMA_SR_TS_1 | DMA_SR_RI_1));
			sysDcrDmasrSet(status & mask);
			break;

		case 2:
			ctrl = sysDcrDmacr2Get() & (~(DMA_CR_CE | DMA_CR_CIE));
			sysDcrDmacr2Set(ctrl);		/* Disable chan. 2	*/

			/* -------------- Check channel's error bit --------------- */
			status = sysDcrDmasrGet();
                        /* logMsg("dmaISR: status: 0x%lx\n",status); */
			if (status & DMA_SR_RI_2)
			{
				/* ------- FIXME -- Handle indicated DMA error -------- */
				errnoSet(EFAULT);
                                logMsg("DMA chan: %d error, stats: 0x%lx\n",channel,status,3,4,5,6);
			}

			/* ---- Clear status bits that can retrigger interrupt ---- */
			mask = (~(DMA_SR_CS_2 | DMA_SR_TS_2 | DMA_SR_RI_2));
			sysDcrDmasrSet(status & mask);
			/* status = sysDcrDmasrGet(); */
                        /* logMsg("dmaISR: ac status: 0x%lx\n",status); */
			break;

		case 3:
			ctrl = sysDcrDmacr3Get() & (~(DMA_CR_CE | DMA_CR_CIE));
			sysDcrDmacr3Set(ctrl);		/* Disable chan. 3	*/

			/* -------------- Check channel's error bit --------------- */
			status = sysDcrDmasrGet();
			if (status & DMA_SR_RI_3)
			{
				/* ------- FIXME -- Handle indicated DMA error -------- */
				errnoSet(EFAULT);
			}

			/* ---- Clear status bits that can retrigger interrupt ---- */
			mask = (~(DMA_SR_CS_3 | DMA_SR_TS_3 | DMA_SR_RI_3));
			sysDcrDmasrSet(status & mask);
			break;
	}

        /* logMsg("DMA ISR: chan: %d\n",channel,2,3,4,5,6,7,8,9,0); */
        /* prtIntStack(); */   /* 177 us */
	desc = dmaInfo[channel].current;
	desc->txfrStatus = DONE;
        dmaInfo[channel].current = NULL;

        /* right now just the present queue system for a done queue, gmb 6/4/04  */
        /* put this discriptor on the done queue, for the future simplification if we want  */
        /* RNG_LONG_PUT(dmaInfo[channel].doneQueue, (int) desc, toP); */

        /* moved to endDMATask(), we need a task context to lock ring buffer as we use it */
        /* begin next DMA transfer for this channel from the readyQueue if any */
        /*  execNextDmaRequestInQueue(channel);  /* could go here or endDMA task */
        status =  RNG_LONG_GET(dmaInfo[channel].readyQueue, (int*) &nextDesc, toP);
        if (status == 1)
        {
           /* DPRINT1(2,"start queued DMA txDesc: 0x%lx\n",nextDesc); */
           /* dmaPrintTxDesc(nextDesc); */
           execDMA(nextDesc);
        }

	/* --------- Notify endDMA task that it can clean up now ---------- */
         /* 2 - 5.3 us to msgQSend */
	status = msgQSend(endDMAQ, (char *)&desc, sizeof(int), NO_WAIT,
					MSG_PRI_NORMAL);
        if (status == ERROR)
           logMsg("msgQSend: no space to send msge to endTask\n",1,2,3,4,5,6);
         /* DPRINT1(+2,"dmaISR: chan %d\n",channel); */
#ifdef DEBUG_DMA_DRV
	dmaLogMsg("dmaISR: exit");
#endif
}

#ifdef SEPARATE_ISRS
*void dmaISRChan2( dmainfo_t *pDmaInfo)      /* Parameter giving channel no.	*/
*{
*	UINT32 ctrl, mask, status; /* ibit, chan; */
*	txDesc_t	*desc, *nextDesc;
*        register int toP;
*
*#ifdef INSTRUMENT
*	wvEvent((event_t) EVENT_DMA_DONE_CHAN+2, NULL, 0);
*#endif
*  
*    ctrl = sysDcrDmacr2Get() & (~(DMA_CR_CE | DMA_CR_CIE));    /* 1 us */
*    sysDcrDmacr2Set(ctrl);		/* Disable chan. 2 */ /* 1us */
*
*    errnoSet(0);					/* Clear errno before starting	*/ /* 1us */
*
*    /* -------------- Check channel's error bit --------------- */
*    status = sysDcrDmasrGet(); /* 1us */
*    if (status & DMA_SR_RI_2) /* 1us */
*    {
*        /* ------- FIXME -- Handle indicated DMA error -------- */
*        errnoSet(EFAULT);
*        logMsg("DMA chan: %d error, stats: 0x%lx\n",pDmaInfo->channel,status,3,4,5,6);
*    }
*
*    /* ---- Clear status bits that can retrigger interrupt ---- */
*    mask = (~(DMA_SR_CS_2 | DMA_SR_TS_2 | DMA_SR_RI_2)); /* 1us */
*    sysDcrDmasrSet(status & mask); /* 1us */
*
*
*    /* logMsg("DMA ISR2: chan: %d\n",pDmaInfo->channel,2,3,4,5,6,7,8,9,0); /* 9.5 us */
*    /* prtIntStack();  /* 177 us */
*    desc = pDmaInfo->current;
*    desc->txfrStatus = DONE;
*    pDmaInfo->current = NULL;   /* 3 insturction 1.6 us */
*
*    /* moved to endDMATask(), we need a task context to lock ring buffer as we use it */
*    /* begin next DMA transfer for this channel from the readyQueue if any */
*    status =  RNG_LONG_GET(pDmaInfo->readyQueue, (int*) &nextDesc, toP);
*    if (status == 1)
*    {
*           /* DPRINT1(2,"start queued DMA txDesc: 0x%lx\n",nextDesc); */
*           /* dmaPrintTxDesc(nextDesc); */
*    wvEvent((event_t) 10, NULL, 0);
*           execDMA(nextDesc);
*    wvEvent((event_t) 11, NULL, 0);
*    }
*
*	/* --------- Notify endDMA task that it can clean up now ---------- */
*         /* 5.3 us to msgQSend */
*	 status = msgQSend(endDMAQ, (char *)&desc, sizeof(int), NO_WAIT,
*					MSG_PRI_NORMAL);
*         if (status == ERROR)
*          logMsg("msgQSend: no space to send msge to endTask\n",1,2,3,4,5,6);
*         /* DPRINT1(+2,"dmaISR: chan %d\n",channel); */
*#ifdef DEBUG_DMA_DRV
*	dmaLogMsg("dmaISR: exit");
*#endif
*    wvEvent((event_t) 20, NULL, 0);
*}
#endif
/*
=========================================================================
NAME: dmaInit
=========================================================================
PURPOSE:
	Initialize DMA controllers before first use.

DETAILS:
	Disables all DMA hardware channels.

	Creates a lock semaphore for each channel, initially full.
	When lock semaphore is unavailable, this indicates that some
	task currently owns the channel.

	Initializes Universal Interrupt Controller (UIC) registers to
	make sure DMA channels can generate interrupts. This includes
	the 4 DMA channel bits in the interrupt enable register (UIC0_ER
	in IBM terminology), and the machine state register's critical
	and noncritical bits (MSR[EE,CE]). Also the interrupt polarity
	and trigger registers, etc.

	Creates message queues and starts the runDMA task.

	The static variable dma_initialized is used to indicate whether
	initialization has occurred or not.

AUTHOR: 
	Robert L. Thrift (2003)
=========================================================================
*/
void dmaInit(int queueSize, int sgListSize)
{
	int		chan,
			ctrl_reg,
			myTaskId;
	STATUS	status;
#ifdef XXX
	FPGA_PTR(RF_ID);
#endif


        queueSize = (queueSize <= 0) ? TX_DESC_MSG_Q_MAX : queueSize;
        sgListSize = (sgListSize <= 0) ? SG_MSG_Q_MAX : sgListSize;

	ctrl_reg = STD_CTRL;			/* Defined in dmaReg.h	*/
	clkRate = sysClkRateGet();		/* Get system clk rate in ticks/sec */

	/* ----- Clear enable bits in scatter/gather command register ----- */
	/* --------- By writing 1 to mask bits, 0 to enable bits ---------- */
	sysDcrDmasgcSet(DMA_SGC_EM0 | DMA_SGC_EM1 | DMA_SGC_EM2 | DMA_SGC_EM3);

	/* ---- Initialize registers, control structs for each channel ---- */
	for (chan = 0; chan < MAX_DMA_CHANNELS; chan++)
	{
		UINT32	reg,
				ibit,
				imsk;

		/*  Clear enable bit & interrupt enable bit in control register */
		dmaSetConfigReg(chan, ctrl_reg);

		/* ----------- Clear channel's status register bits ----------- */
		dmaSetStatusReg(chan, 0);

		/* --- Clear count register, source & destination registers --- */
		/* -------------- Just to make debugging easier --------------- */
		dmaSetCountReg(chan, 0);
		dmaSetSrcReg(chan, 0);
		dmaSetDestReg(chan, 0);

		/* -------- Initialize channel polarity register bits --------- */
		/* ------- Set DMAReq, DMAAck, EOT lines to active high ------- */
		dmaSetPolarityReg(chan, 0);

		/* ----------- Initialize channel's dmaInfo struct ------------ */
		dmaInfo[chan].channel	= chan;	/* My channel number, 0-3	*/
		/* dmaInfo[chan].queue		= &(dmaQueue[chan]);	/* Ptr. to its queue*/

		dmaInfo[chan].queue		= NULL; 	/* Ptr. to its queue*/

		dmaInfo[chan].current   = NULL;	/* Initially no txfr in progrs.	*/
		if (! dma_initialized)
		{
			/* -------- Create lock semaphore for this channel -------- */
			dmaInfo[chan].lock		= semBCreate(SEM_Q_FIFO, SEM_FULL);

			/* -------- Create mutex semaphore for this channel -------- */
			dmaInfo[chan].pDmaMutex = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE |
                                                               SEM_DELETE_SAFE);


                        /* create the ready & done queue of transfer descriptors.  gmb 6/3/04 */
                        dmaInfo[chan].readyQueue = rngLCreate(queueSize+5, "DMA Ready Queue");

                        /* a future item - dmaInfo[chan].doneQueue = rngLCreate(TX_DESC_MSG_Q_MAX, "DMA Done Queue"); */

			/* ------ Initialize DMA jobs queue for this channel ------ */
			/*
			 *dmaQueue[chan].first	= NULL;
			 *dmaQueue[chan].last	= NULL;
			 *dmaQueue[chan].count	= 0;
                        */
		}
		else
		{
			/* --- Don't re-create semaphores on re-initializations --- */
			/* ------------------ Just free them up ------------------- */
			status = semGive(dmaInfo[chan].lock);

			/*  On re-initializations just try to clear existing queue  */
			/* dmaQueueFree(chan); */

			/*  On re-initializations just clear existing ready queue  */
			rngLFlush(dmaInfo[chan].readyQueue);
		}

		/*  Set up universal interrupt controller bits for this channel */
		/*  Channel:    bit in all UIC registers:                       */
		/*  --------    -------------------------                       */
		/*     0        bit 5                                           */
		/*     1        bit 6                                           */
		/*     2        bit 7                                           */
		/*     3        bit 8                                           */
		ibit = BITSET((DMA_CHAN_INT_BASE + chan));
		imsk = ~ibit;	/* Mask with this bit zeroed, all others set	*/

						/* UIC Status Register:		*/
		sysDcrUicsrClear(ibit);		/* Clear interrupt status bit	*/
						/* By writing a 1 to it (!!!)	*/

		reg  = sysDcrUiccrGet();	/* UIC Critical Register:	*/
		sysDcrUiccrSet(reg & imsk);	/* Make chan interrupt non-critical	*/
						/* By clearing its bit		*/

		reg  = sysDcrUicprGet();	/* UIC Polarity Register:	*/
		sysDcrUicprSet(reg | ibit);	/* Set chan intr. polarity positive */
						/* By writing a 1 to its bit	*/

		reg  = sysDcrUictrGet();	/* UIC Trigger Register:	*/
		sysDcrUictrSet(reg & imsk);	/* Set trig. to level-sensitive	*/
						/* By clearing its bit		*/

		reg  = sysDcrUicerGet();	/* UIC Enable Register:		*/
		sysDcrUicerSet(reg | ibit);	/* Enable interrupts on this chan. */
						/* By writing a 1 to its bit	*/

		/* ----------- Connect up ISR for this DMA channel ------------ */
		/* -------- Only do this on first initialization call --------- */
		if (! dma_initialized)
                {
		    if (intConnect(INUM_TO_IVEC(DMA0INTNUM+chan), dmaISR, chan) != OK)
			printf("dmaInit: error in DMA %d ISR setup.\n", chan);
#ifdef SEPARATE_ISRS
*		    if (intConnect(INUM_TO_IVEC(DMA0INTNUM+chan), dmaISRChan2, &(dmaInfo[chan])) != OK)
*			printf("dmaInit: error in DMA %d ISR setup.\n", chan);
#endif
                }
	}

	/* -------------- Initialize DMA sleep mode register -------------- */
	dmaSetSleepReg(0);		/* No sleep */

	/* ------------------ Pre-allocate SG list nodes ------------------ */
	if (dmaCreateSGMsgQueue(sgListSize) != OK)
	{
		printf("dmaInit: error in SG list nodes creation.\n");
		exit(ERROR);
	}

	/* ------------ Pre-allocate dma transfer descriptors ------------- */
	if (dmaCreateTxDescMsgQueue(queueSize) != OK)
	{
		printf("dmaInit: error in txfr descriptors creation.\n");
		exit(ERROR);
	}

	/* ------ Adjust DMA driver priority and subtask priorities ------- */
	myTaskId = taskIdSelf();			/* Get our task ID */
	taskPriorityGet(myTaskId, &myTaskPriority);	/* Get our task priority*/

	/* ------------ Adjust main task priority if necessary ------------ */
	if (myTaskPriority < MIN_DMA_PRIORITY)
	{
		myTaskPriority = MIN_DMA_PRIORITY;
		taskPrioritySet(myTaskId, myTaskPriority);
	}

	/* ------------- Calculate priorities for other tasks ------------- */
	/* endDMAPriority  = myTaskPriority - 16; */
	endDMAPriority  = 1;
	logMsgPriority  = myTaskPriority - 12;
	runDMAPriority  = myTaskPriority - 8;
	sendDMAPriority = myTaskPriority - 4;

#ifdef DONOT_USE_USE_DPRINT
	/* - Create DMA error message queue and start dmaLogErrMsgs task -- */
	if (dmaCreateErrMsgQueue() != OK)
	{
		printf("dmaInit: error in DMA error msg queue creation.\n");
		exit(ERROR);
	}
    if (! dmaErrMsgTaskID)
        dmaErrMsgTaskID = taskSpawn("t_dmaLogErrMsgs", logMsgPriority, 0,
                        DMA_TSTACK, (FUNCPTR) dmaLogErrMsgs,
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    if (dmaErrMsgTaskID == ERROR)
    {
        dmaErrMsgTaskID = 0;
        printf("dmaInit: cannot spawn dmaLogErrMsgs task.\n");
        exit(ERROR);
    }
#endif

	/* ---- Create message queue for endDMATask and start the task ---- */
	if (dmaCreateEndMsgQueue() != OK)
	{
		printf("dmaInit: error in End msg queue creation.\n");
		exit(ERROR);
	}
    if (! endDMATaskID)
        endDMATaskID = taskSpawn("t_endDMA", endDMAPriority, 0, DMA_TSTACK,
                    (FUNCPTR) endDMATask,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	if (endDMATaskID == ERROR)
	{
		endDMATaskID = 0;
		printf("dmaInit: cannot spawn endDMATask.\n");
		exit(ERROR);
	}

	dma_initialized = 1;	/* Set flag showing init. was done */
}

/*
=========================================================================
NAME: dmaCleanup
=========================================================================
PURPOSE:
	Remove DMA driver tasks and data structures prior to program exit.

RETURN VALUE:
	None.

DETAILS:
	Performs the following tasks:
	1. Deletes subtasks started by dmaInit().
	2. Deletes semaphores created by dmaInit().
	3. Deletes message queues created by dmaInit().
	   (Also frees any memory objects on message queues.)
	4. Clean out any jobs in DMA job queues.

COMMENT:
	This is mostly for debugging but could be called before exiting,
	to clean up tasks and message queues as well.

AUTHOR: 
	Robert L. Thrift, 2004
=========================================================================
*/
void dmaCleanup(void)
{
	int chan;

	/* -------------- Delete tasks started by dmaInit() --------------- */
	if (runDMATaskID)
	{
		taskDelete (runDMATaskID);	/* Delete runDMA task	*/
		runDMATaskID = 0;
	}
	if (sendDMATaskID)
	{
		taskDelete(sendDMATaskID);	/* Delete sendDMATask	*/
		sendDMATaskID = 0;
	}
	if (dmaErrMsgTaskID)
	{
		taskDelete(dmaErrMsgTaskID);/* Delete dmaErrMsgTask	*/
		dmaErrMsgTaskID = 0;
	}
	if (endDMATaskID)
	{
		taskDelete(endDMATaskID);	/* Delete endDMATask	*/
		endDMATaskID = 0;
	}

	/* ---------- Delete message queues created by dmaInit() ---------- */
	dmaDeleteRunMsgQueue();			/* Delete runDMA msg. queue	*/
	dmaDeleteErrMsgQueue();			/* Delete DMA msg logging queue	*/
	dmaDeleteSGMsgQueue();			/* Delete SG list nodes queue	*/
	dmaDeleteTxDescMsgQueue();		/* Delete txfr descriptors queue*/
	dmaDeleteEndMsgQueue();			/* Delete endDMA msg. queue	*/

	/* ----------- Delete semaphores for each DMA channel. ------------ */
	for (chan = 0; chan < MAX_DMA_CHANNELS; chan++)
	{
		if (dmaInfo[chan].lock)
		{
			semDelete(dmaInfo[chan].lock);	/* Delete lock semaphore */
			dmaInfo[chan].lock = NULL;
		}
		/* dmaQueueFree(chan);		/* Clean out DMA transfer queues*/
		rngLDelete(dmaInfo[chan].readyQueue);
		dmaInfo[chan].readyQueue = NULL;
	}
	dma_initialized = 0;
}

/*
=========================================================================
NAME: execDMA
=========================================================================
PURPOSE:
	Provide a means of starting a DMA transfer to/from a peripheral (for
	example a FIFO)

INPUT:
       DMA transfer descriptor ( txDesc_t  *txfrDesc )

OUTPUT:
	Sets static variable runDMAStatus to indicate success or failure
	of the channel startup.
	OK - Call succeeded.
	ERROR	- Call failed.

RETURN VALUE:
	None.

COMMENT:
        This routine can is called with a pointer to a DMA transfer descriptor struct,
        it sets up and fires off a DMA transfer according to the parameters
        in the transfer descriptor.

	This routines is called  by runDMA task, however it may be called directly
        depending who's context the transfer should be begun.

AUTHOR: 
	Robert L. Thrift, 2004
        Greg Brissey, 5/27/004  (repackaging job, into a separate routine to allow direct call)
=========================================================================
*/
execDMA(txDesc_t *desc )   /* DMA transfer descriptor  */
{
   int	chan;

   /* logMsg("execDMA: begin\n"); */
   runDMAStatus = OK;		/* Optimistic preset of status code */
   if (desc->txfrStatus != READY)
   {
     logMsg("execDMA: descriptor not in READY state.",1,2,3,4,5,6);
     runDMAStatus = ERROR;	/* Txfr descriptor says not ready */
     return;		/* Go wait for another message	*/
   }

   chan = desc->channel;
   dmaSetStatusReg(chan, 0);	/* Clear status register	*/

   if (desc->srcType == SG_LIST)	/* If user sent a SG list...	*/
   {
      dmaSetConfigReg(desc->channel, desc->txfrControl);
      /* --- Put 1st SG node address into SG address register --- */
      dmaSetSGReg(desc->channel, (UINT32)(desc->first));

      /* ---------- Start up a scatter-gather transfer ---------- */
      desc->txfrStatus = RUNNING;
      dmaInfo[chan].current = desc;
      /* dmaLogMsg("execDMA: start actual SG DMA transfer."); */
      dmaEnableSGCommandReg(desc->channel);
      /* ------------------ DMA is now running ------------------ */
#ifdef INSTRUMENT
	wvEvent((event_t) EVENT_DMA_START_CHAN+chan, NULL, 0);
#endif
   }
   else if (desc->srcType == NO_SG_LIST)
   {
      /* - Start up a single-buffer non-scatter-gather transfer - */
      /* dmaInfo[desc->channel].current = desc; */
      dmaSetSrcReg(desc->channel, (UINT32)(desc->srcAddr));
      dmaSetDestReg(desc->channel, (UINT32)(desc->dstAddr));
      dmaSetCountReg(desc->channel, (desc->txfrSize & DMA_CNT_MASK));
      desc->txfrStatus = RUNNING;
      dmaInfo[chan].current = desc;
   
      /* dmaLogMsg("execDMA: start actual NON SG DMA transfer."); */
      /* ---- Add enable, interrupt bits to control reg. ---- */
      dmaSetConfigReg(desc->channel, desc->txfrControl
                                    | DMA_CR_CE | DMA_CR_CIE);

      /* --------------- DMA is now running! ---------------- */
#ifdef INSTRUMENT
	wvEvent((event_t) EVENT_DMA_START_CHAN+chan, NULL, 0);
#endif
   }
   else
   {
      /* --------------- Invalid useSG argument value --------------- */
      /* ----------- Was neither SG_LIST nor NO_SG_LIST ------------- */
      logMsg("runDMA: srcType value invalid.",1,2,3,4,5,6);
      desc->txfrStatus = ERR_STATUS;
      runDMAStatus = ERROR;
   }
   /* logMsg("execDMA: end\n"); */
}

/*
=========================================================================
NAME: execNextDmaRequestInQueue
=========================================================================
PURPOSE:
	Provide a means of starting a DMA transfer from the readyQueue of a
        DMA channel 

INPUT:
       DMA channel
       DMA transfer descriptor ( txDesc_t  *txfrDesc )

OUTPUT:
        None

RETURN VALUE:
	None.

COMMENT:
        This routine can is called with a DMA channel number, 
        checks to see if DMA for this channel is active. 
        If DMA is not active then pulls a pointer to a DMA transfer 
        descriptor struct (txDesc_t  *txfrDesc ) from readyQueue. 
        Then fires off a DMA transfer according to the parameters
        in the transfer descriptor.

	This routine is called by the DMA ISR , however it may be called directly
        depending who's context the transfer should be begun.

AUTHOR: 
        Greg Brissey, 6/04/04  
=========================================================================
*/
void execNextDmaRequestInQueue(int channel)
{
   register int toP;
   int status;
   txDesc_t *nextDesc;
   int dmaActive;

   /* semTake(dmaInfo[channel].pDmaMutex, WAIT_FOREVER); */

   /* the dma queue is empty & the dma is not qctive then start DMA */
   /* if channel actively transfering, then don't try to start another on the channel */
   /* if ( ! dmaGetDeviceActiveStatus(channel) ) */
   /* dmaActive = dmaGetDeviceActiveStatus(channel); */
   /* if ( (!dmaActive) && (dmaInfo[channel].current != NULL)) 
    * {
    *  DPRINT3(-4,"execNextDmaRequestInQueue: ERROR Cond, chan: %d, DMA Status: %d, current: 0x%lx\n",
    * 		channel, dmaActive,dmaInfo[channel].current);
    * }
    */
   /* prior to this test the single test of dmaGetDeviceActiveStatus() was ineffective.
    * What would happen was that the during the time the DMA finished (i.e. not active) and
    * the time the the ISR took to start (interrupt latency) this a task task calling this
    * routine could come in a pull a queue entry off the fing buffer only to be preempted
    * by the ISR which took the next and start the DMA on it. Thus an entire buffer of
    * FIFO instruction were being lost (see bugzilla # 711) , check the currect insures the
    * ISR has run an no further queue entries were there.
    *
    *       Greg B. 9/20/2005
    */
   if ( (! dmaGetDeviceActiveStatus(channel)) && (dmaInfo[channel].current == NULL) )
   {
	/* since more than one task can put an entry into the queue we MUST protect the access */
        /* we can use either a mutex or taskLock(),taskUnlock(), I choose taskLock() for now */
	/* taskLock();   only the ITR or this call from task context pulls an items from the queue
           so as long as the ISR is running this one can not */
        semTake(dmaInfo[channel].pDmaMutex, WAIT_FOREVER);    /* 2.5 us */
        status =  RNG_LONG_GET(dmaInfo[channel].readyQueue, (int*) &nextDesc, toP);   /* 1.5 us */
        semGive(dmaInfo[channel].pDmaMutex);    /* 2.5 us */
        /* taskUnlock(); for a particular queue only one task queues */
       /* if the is a transfer discriptor on the readyQueue then start the DMA */
       if (status == 1)
       {
           if (DebugLevel > 2)
           {
              logMsg("Start queued DMA[chan=%d] txDesc: 0x%lx\n",channel,nextDesc,3,4,5,6);
              /* logMsg("*******+++++++))))))))!!!!!   DMA[%d]: start\n",channel); */
              /* dmaPrintTxDesc(nextDesc); */
           }
           execDMA(nextDesc);
       }
       else
       {
         DPRINT(2," DMA readyQueue Empty\n");
         /* diagPrint(NULL," execNextDmaRequestInQueue() readyQueue Empty\n"); */
       }
  }
  else
  {
     DPRINT(2," execNextDmaRequestInQueue: DMA channel enabled/active\n");
     /* diagPrint(NULL," execNextDmaRequestInQueue() channel active\n"); */
  } 
   /* semGive(dmaInfo[channel].pDmaMutex); */

  return;
}

/*
=========================================================================
NAME: abortActiveDma
=========================================================================
PURPOSE:
	Disable the DMA for the given channel.
        If the TxDesc is still marked as RUNNING then 
        return this TxDesc and buffer back to their free list.
INPUT:
       DMA channel

OUTPUT:
        None

RETURN VALUE:
	None.
*/
abortActiveDma(int channel)
{
   txDesc_t  *desc;
   dmaDisableChan(channel);
   desc = dmaInfo[channel].current;  /* TxDesc of current transfer */
   /* if the desc had not been completed then this buffer need to be returned */
   /* dmaPrintTxDesc(desc); */
   if (desc->txfrStatus == RUNNING)  /* not completed then return to free lists */
   {
       desc->txfrStatus = DONE;
       DPRINT1(+1,"abortActiveDma: return active Desc: 0x%lx\n",desc);
       /* dmaPrintTxDesc(desc); */
       returnBuffersAndTxDesc(desc);
   }
   dmaInfo[channel].current = NULL;   /* used as test in starting DMA so make sure it's null */
}

/*
=========================================================================
NAME: clearDmaRequestQueue
=========================================================================
PURPOSE:
	Provide a means of removing all queued DMA transfer requests from the 
        readyQueue of a DMA channel 

        The DMA channel should be disabled prior to this call.
        
        e.g. 
            dmaDisableChan(chan)

INPUT:
       DMA channel
       DMA transfer descriptor ( txDesc_t  *txfrDesc )

OUTPUT:
        None

RETURN VALUE:
	None.

COMMENT:
        This routine can is called with a DMA channel number, 
        checks to see if DMA for this channel is active. 
        If DMA is not active then pulls a pointer to a DMA transfer 
        descriptor struct (txDesc_t  *txfrDesc ) from readyQueue. 
        Then return the buffers, etc back to their free lists.

AUTHOR: 
        Greg Brissey, 8/11/04  
=========================================================================
*/
void clearDmaRequestQueue(int channel)
{
   register int toP;
   int status;
   txDesc_t *nextDesc;

   /* semTake(dmaInfo[channel].pDmaMutex, WAIT_FOREVER); */

   /* if channel actively transfering, then don't try to remove desc from readyQueue */
   if ( ! dmaGetDeviceActiveStatus(channel) )
   {
	/* since more than one task can put an entry into the queue we MUST protect the access */
        /* we can use either a mutex or taskLock(),taskUnlock(), I choose taskLock() for now */
        while(1)
        {
	   taskLock();
           status =  RNG_LONG_GET(dmaInfo[channel].readyQueue, (int*) &nextDesc, toP);
           taskUnlock();
           /* if the is a transfer discriptor on the readyQueue then start the DMA */
           if (status == 1)
           {
               DPRINT1(2,"clearDmaRequestQueue: start queued DMA txDesc: 0x%lx\n",nextDesc);
               /* dmaPrintTxDesc(nextDesc); */
               returnBuffersAndTxDesc(nextDesc);
           }
           else
           {
               DPRINT(2,"clearDmaRequestQueue: DMA readyQueue Empty\n");
               /* diagPrint(NULL," DMA readyQueue Empty\n"); */
	       break;
           }
       }
  }
  else
  {
     DPRINT(2,"clearDmaRequestQueue: DMA channel enabled/active\n");
     /* diagPrint(NULL," DMA channel enabled/active\n"); */
  }
  /* semGive(dmaInfo[channel].pDmaMutex); */

  return;
}


/*
=========================================================================
NAME: dmaXfer
=========================================================================
PURPOSE:
	Provide a general purpose user interface for transferring data
	over a DMA channel.

ARGUMENTS:
	channel		- The channel number to use. Caller should have
				  previously obtained the use of a DMA channel via
				  dmaGetChannel().
	transferType	- The kind of DMA transfer desired.
				MEMORY_TO_MEMORY.....(straight RAM to RAM copy)
				MEMORY_TO_PERIPHERAL (i.e. device-paced transfer
									  to FIFO)
				PERIPHERAL_TO_MEMORY (i.e. device-paced transfer
									  from FIFO)
				MEMORY_TO_FPGA.......(a non-device paced trans-
									  fer through a register
									  that has a fixed address)
				FPGA_TO_MEMORY.......(a non-device paced trans-
									  fer through a register
									  that has a fixed address)
	useSG		- SG_LIST if scatter-gather list is to be used,
				  or NO_SG_LIST if it is a single-buffer transfer.
	srcAddr		- If useSG is NO_SG_LIST, this is the address of
				  the data buffer to be transferred.
				  If useSG is SG_LIST, this is the address of the
				  head of the scatter-gather list.
				  (The head of a scatter-gather list is a standard
				  DMA transfer descriptor, i.e. struct dmaTransferDesc,
				  a.k.a. txDesc_t and it contains a pointer to the
				  first scatter-gather list node. See dmaDrv.h)
	destAddr	- The destination address for start of transfer. This
				  could be a memory address or an FPGA register
				  address, depending on the type of transfer.
	txferSize	- No. of 32-bit words (not bytes) in the transfer.
				  This argument is ignored if a scatter-gather list
				  is in use, since each scatter-gather block carries
				  its own word count.
	srcMsgQueue	- The message queue to which caller's source data
				  buffer pointers should be returned. If NULL, the DMA
				  driver will not attempt to return data buffers
				  to the caller's queue. (For instance if caller
				  has specifically malloc'd a buffer just for this
				  purpose.) If the caller has a queue of free buffer
				  addresses, put the queue's ID here to get the
				  buffer(s) returned automatically to the free
				  source buffers queue.
	dstMsgQueue	- The message queue to which caller's destination data
				  buffer pointers should be returned. If NULL, the DMA
				  driver will not attempt to return data buffers
				  to the caller's queue. (For instance if caller
				  has specifically malloc'd a buffer just for this
				  purpose.) If the caller has a queue of free buffer
				  addresses, put the queue's ID here to get the
				  buffer(s) returned automatically to the free
				  destination buffers queue.
OUTPUT:
	Error messages only. Data is transferred from the source address to
	the destination address using the specified transfer type and SG mode.

RETURN VALUE:
	OK		- Successful operation.
	ERROR	- Some error occured, DMA transfer was not performed.

COMMENT:
	This routine assembles and posts a message to sendDMATask and
	returns immediately to avoid suspending the caller. The caller will
	be (and should be) suspended only if the sendDMATask message queue
	is full.

AUTHOR: 
	Robert L. Thrift, 2004
        Altered: Greg Brissey    8/4/04
=========================================================================
*/


/*
=========================================================================
NAME: getDMA_ControlWord
=========================================================================
PURPOSE:
	Set the appropiate DMA control bits within a UINT32 for the type of transfer 

INPUT:
       Type of DMA transfer    (txfr_t  transferType) 
      MEMORY_TO_MEMORY
      MEMORY_TO_FPGA
      MEMORY_TO_PERIPHERAL
      PERIPHERAL_TO_MEMORY

OUTPUT:
	DMA Control Word  
	ERROR	- 0

RETURN VALUE:
	DMA Control Word.

COMMENT:
        This routine can is called with a DMA transfer type  enum,
        it sets up the DMA control bit for the DMA transfer 

AUTHOR: 
        Greg Brissey, 5/27/004  (repackaging job, into a separate routine to allow direct call)
=========================================================================
*/
UINT32 getDMA_ControlWord(txfr_t  transferType)
{
   UINT32 controlWord;	/* Bits for DMA Control Register	*/

   /* --- Set up DMA control register bits common to all modes --- */
   controlWord = DMA_CR_PW_WORD	        /* Transfer width 32 bits	*/
                  | DMA_CR_BEN		/* Internal buffering enabled	*/
                  | DMA_CR_TCE		/* Stop on transfer count = 0	*/
                  | DMA_CR_CP_MEDH;	/* Priority medium-high		*/

   /*  Set more control bits according to requested transfer type  */
   switch (transferType) {
      case MEMORY_TO_MEMORY:
         /*Setup for software-initiated memory-to-memory transfer*/
         /* -------------- Set ETD as a TC output -------------- */
         controlWord	|= DMA_CR_TM_SWMM	/* Transfer mode	*/
                        | DMA_CR_PF_4	/* Memory prefetch 4 wds.*/
                        | DMA_CR_SAI	/* Src. addr. increment	*/
                        | DMA_CR_DAI	/* Dest. addr. increment*/
                        | DMA_CR_ETD;	/* EOT is a TC output	*/
         break;

      case MEMORY_TO_MEMORY_SRC_PACED:
         /*Setup for software-initiated memory-to-memory transfer*/
         /* -------------- Set ETD as a TC output -------------- */
         controlWord	|= DMA_CR_TM_HWDP	/* Transfer mode	*/
                        | DMA_CR_PF_4	/* Memory prefetch 4 wds.*/
                        | DMA_CR_TD	/* Src. paced */
                        | DMA_CR_SAI	/* Src. addr. increment	*/
                        | DMA_CR_DAI	/* Dest. addr. increment*/
                        | DMA_CR_ETD;	/* EOT is a TC output	*/
         break;

      case MEMORY_TO_MEMORY_DST_PACED:
         /*Setup for software-initiated memory-to-memory transfer*/
         /* -------------- Set ETD as a TC output -------------- */
         controlWord	|= DMA_CR_TM_HWDP	/* Transfer mode	*/
                        | DMA_CR_PF_4	/* Memory prefetch 4 wds.*/
                        | DMA_CR_SAI	/* Src. addr. increment	*/
                        | DMA_CR_DAI	/* Dest. addr. increment*/
                        | DMA_CR_ETD;	/* EOT is a TC output	*/
			/* PL = 0, deviced pace is from Destination */
         break;



      case MEMORY_TO_FPGA:
         /*Setup for software-initiated memory-to-memory transfer*/
         /*Except transfer is to a fixed location (FPGA register)*/
         controlWord |= DMA_CR_TM_SWMM	/* Transfer mode		*/
                     | DMA_CR_SAI	/* Src. addr. increment	*/
                     | DMA_CR_ETD;	/* EOT is a TC output	*/
         break;

      case MEMORY_TO_PERIPHERAL:
         /* ------ Set up for device-paced transfer (out) ------ */
         /*Add device pacing, source incrementing, memory prefetch*/
         /* FIFO dma to highest priority                         */
         /* -------------- Set ETD as a TC output -------------- */
         controlWord |= DMA_CR_TM_HWDP	/* Transfer mode		*/
                     | DMA_CR_SAI	/* Src. addr. increment	*/
                     | DMA_CR_PF_4	/* Memory prefetch 4 wds.*/
                     | DMA_CR_ETD	/* EOT is a TC output	*/
                     | DMA_CR_CP_HIGH;	/* Priority high	*/
         break;

      case PERIPHERAL_TO_MEMORY:
         /* ------ Set up for device-paced transfer (in) ------- */
         /*Add device pacing, transfer direction, dest. incrementing*/
         controlWord |= DMA_CR_TM_HWDP	/* Transfer mode		*/
                     | DMA_CR_TD	/* Incoming direction	*/
                     | DMA_CR_DAI;	/* Dest. addr. increment*/
         break;

      default:
                           /* ------------ Unrecognized transfer mode ------------ */
         dmaLogMsg("sendDMATask: invalid txfer mode, default MEMORY_TO_MEMORY.");
         controlWord	|= DMA_CR_TM_SWMM	/* Transfer mode	*/
                        | DMA_CR_SAI	/* Src. addr. increment	*/
                        | DMA_CR_DAI	/* Dest. addr. increment*/
                        | DMA_CR_ETD;	/* EOT is a TC output	*/
	    break;
   }
   return( controlWord );
}

/*
=========================================================================
NAME: buildSG_List
=========================================================================
PURPOSE:
	Provide a means of building a Scatter gather List 

INPUT:
       DMA transfer descriptor ( txDesc_t  *txfrDesc )
        the following member of the structure must of been set 
           txfrDesc->txfrControl
           txfrDesc->srcAddr
           txfrDesc->dstAddr
           txfrDesc->txfrSize
       

OUTPUT:
       BUild SG list is attach to the DMA transfer descriptor  (txfrDesc->first)

RETURN VALUE:
	None.

COMMENT:
        * -------------------------------------------------------------------
        * Following code constructs an internal scatter-gather list to
        * handle transfers that were given to us as a single one-shot buffer
        * whose size was larger than the maximum allowed in a single DMA
        * transfer. Instead of returning an error to the caller, we create
        * a scatter-gather list internally and divide the DMA transfer up
        * into legal-sized pieces.
        * -------------------------------------------------------------------

AUTHOR: 
	Robert L. Thrift, 2004
        Greg Brissey, 5/27/004  (repackaging job, into a separate routine to allow direct call)
=========================================================================
*/
void buildSG_List(txDesc_t *txfrDesc )   /* DMA transfer descriptor  */
{
   sgnode_t	*sgNode;	/* Node in scatter-gather list		*/
   UINT32	*src,		/* Temporary address			*/
               *dest;		/* Temporary address			*/
   STATUS	status;
   int num_nodes,
       SGtxfrSize,
       last_SGtxfrSize;

   /* 
   * -------------------------------------------------------------------
   * Following code constructs an internal scatter-gather list to
   * handle transfers that were given to us as a single one-shot buffer
   * whose size was larger than the maximum allowed in a single DMA
   * transfer. Instead of returning an error to the caller, we create
   * a scatter-gather list internally and divide the DMA transfer up
   * into legal-sized pieces.
   * -------------------------------------------------------------------
   */

      src  = (UINT32 *)(txfrDesc->srcAddr);	/* Temporaries	*/
      dest = (UINT32 *)(txfrDesc->dstAddr);
	
      /* ----- Calculate no. list nodes needed, rounding up ----- */
      /* ---------- Using integer division, remember! ----------- */
      num_nodes = (txfrDesc->txfrSize
                     + (MAX_DMA_TRANSFER_SIZE - 1)) / MAX_DMA_TRANSFER_SIZE;

      /* ----- Calculate size of each transfer except last ------ */
      SGtxfrSize = (txfrDesc->txfrSize + num_nodes - 1) / num_nodes;

      /* ------ Calculate remainder for last transfer size ------ */
      last_SGtxfrSize = txfrDesc->txfrSize - (num_nodes - 1) * SGtxfrSize;

      /* - String together the needed scatter-gather list nodes - */
      while (num_nodes > 0)
      {
         if (num_nodes == 1)
         {
            /* Last node, give it the remaining transfer count  */
            sgNode = dmaSGNodeCreate((UINT32)src, (UINT32)dest, last_SGtxfrSize);
         }
         else
         {
            /* - Not last node, use calculated transfer size -- */
            sgNode = dmaSGNodeCreate((UINT32)src, (UINT32)dest, SGtxfrSize);
            /* -- Bump buffer pointers for the next transfer -- */
            if ((txfrDesc->txfrControl & DMA_CR_SAI)) /*  DMA_CR_SAI Src. addr. increment	*/
                src  += SGtxfrSize;	/* pointer math, not integer*/
            if ((txfrDesc->txfrControl & DMA_CR_DAI)) /*  DMA_CR_SAI Dest. addr. increment	*/
                dest += SGtxfrSize;
         }
         /* ------------ Append new node to SG list ------------ */
         dmaSGNodeAppend(sgNode, txfrDesc);

         num_nodes--;
      }	 /* SG list is now built		*/
  return;
}

/*
=========================================================================
NAME: setupSG_List4Dma
=========================================================================
PURPOSE:
	Setup the DMA member of the SG List for DMA 
	And cache flushes the SG elements and data buffers

INPUT:
       DMA transfer descriptor ( txDesc_t  *txfrDesc )
       

OUTPUT:
       SG list that is ready for DMA 

RETURN VALUE:
	None.

COMMENT:
       * -------------------------------------------------------------------
       * Traverse scatter-gather list, adding control bits to each SG node
       * and flush the cache for each node. 
       * -------------------------------------------------------------------

AUTHOR: 
	Robert L. Thrift, 2004
        Greg Brissey, 5/27/004  (repackaging job, into a separate routine to allow direct call)
=========================================================================
*/
void setupSG_List4Dma(txDesc_t  *txfrDesc)
{
   sgnode_t  *sgNode;
   UINT32 controlWord;
   UINT32 size;

   /* 
   * -------------------------------------------------------------------
   * Traverse scatter-gather list, adding control bits to each SG node
   * and flush the cache for each node. We don't do this while building
   * the internal SG list above, because the same operation needs to be
   * done for an internally built list or for a list pre-constructed by
   * the user.
   * -------------------------------------------------------------------
   */
	UINT32	dmaCCW;				/* DMA Channel Control Word				*/
	UINT32	dmaSrcAddr;			/* DMA source address					*/
	UINT32	dmaDstAddr;			/* DMA destination address				*/
	UINT32	dmaCtrlBits;		/* LK, TCI, ETI, ERI bits and Count		*/
	struct dmaSGListNode *next;	/* Ptr. to next entry in list			*/

   controlWord = txfrDesc->txfrControl;
   sgNode = txfrDesc->first;
   while(sgNode)
   {
/*
         printf("dmaCCW: 0x%lx, dmaSrcAddr: 0x%lx, dmaDstAddr: 0x%lx, dmaCtrlBits: 0x%lx, next: 0x%lx\n",
	     sgNode->dmaCCW, sgNode->dmaSrcAddr, sgNode->dmaDstAddr, sgNode->dmaCtrlBits, sgNode->next);
*/
		
         sgNode->dmaCCW = controlWord | DMA_CR_CE | DMA_CR_CIE;
         if (! (sgNode->next))
            /* ----- Last node in list gets special treatment ----- */
            sgNode->dmaCtrlBits = (sgNode->dmaCtrlBits & DMA_CNT_MASK)
                                    | STD_SG_CTRL_LAST;
         else
            /* -------- Ordinary node embedded in SG list --------- */
            sgNode->dmaCtrlBits = (sgNode->dmaCtrlBits & DMA_CNT_MASK)
                                    | STD_SG_CTRL;
         /* --------- Flush the cache on this SG list node --------- */
         cacheFlush(DATA_CACHE, (void *) sgNode,
                        (size_t)(sizeof(struct dmaSGListNode)));

         /* --- Also flush the cache on the node's data buffer. ---- */
         size = (sgNode->dmaCtrlBits & DMA_CNT_MASK);
         if (size == 0L)
            size = MAX_DMA_TRANSFER_SIZE;
         
         cacheFlush(DATA_CACHE, (void *) sgNode->dmaSrcAddr, (size_t)(size * sizeof(int)));
         sgNode = sgNode->next;
   }
   cachePipeFlush(); /* won't return until cache has been flushed */
   return;
}

/*
=========================================================================
NAME: queueDmaTransfer     queue a dma transfer (doesn't start it)
=========================================================================
PURPOSE:
	Provide a general purpose user interface for queuing DMA transfer
	requests over a DMA channel.

ARGUMENTS:
	 see below dmaXfer() routine 
OUTPUT:
	DMA transfer descriptor is placed on the channel's readyQueue.

RETURN VALUE:
	OK		- Successful operation.
	ERROR	- Some error occured, DMA transfer was not placed on readyQueue for some reason.

COMMENT:

        places the DMA request on the reaadyQueue for later processing.
        This routine does not actually start the transfer.
	(see execNextDmaRequestInQueue() routine)

AUTHOR: 
	Greg Brissey, 6/04/2004
=========================================================================
*/
STATUS queueDmaTransfer(
	int	channel,		/* Which channel is ours		*/
	txfr_t	transferType,		/* As described in comments above	*/
	usesg_t useSG,			/* As described in comments above */
	UINT32	srcAddr,		/* Source address if no SG list	*/
					/* Otherwise addr. of head of S/G list	*/
	UINT32	destAddr,		/* Destination address			*/
	int	txferSize,		/* No. of words to transfer		*/
	MSG_Q_ID srcMsgQueue,		/* Caller's msg queue for src. buffers	*/
	MSG_Q_ID dstMsgQueue		/* Caller's msg queue for dst. buffers	*/
	)
{
   txDesc_t	*txfrDesc;	/* DMA transfer descriptor		*/
   int status;
   int toP;

   if ((channel < 0) || (channel > MAX_DMA_CHANNELS))
   {
      dmaLogMsg("xferDMA: invalid DMA channel\n");
      /* dmaLogMsg("xferDMA: invalid DMA channel: %d\n",channel); */
      return(-1);
   }
   /* - Device-paced FIFO transfers must be chan. 2 or 3 - */
   /* ----- Because that's how the FIFO is wired up ------ */
   if ( ((transferType == PERIPHERAL_TO_MEMORY) | ( transferType == MEMORY_TO_PERIPHERAL)) &&
	((channel < 2) || (channel > 3)) )
   {
      dmaLogMsg("xferDMA: invalid PERIPHERAL DMA channel\n");
      /* dmaLogMsg("xferDMA: invalid PERIPHERAL DMA channel: %d\n",channel); */
      return(-1);
   }

   /* Oops this lock up the system , since it can pend in dmaGetFreeTxDesc() thus locking out
      the DMA endDmaTask(), resulting in FIFO underflow */
   /* semTake(dmaInfo[channel].pDmaMutex, WAIT_FOREVER); */

   /* -- A message was received, time to set up a DMA transfer --- */
   if (useSG == NO_SG_LIST)
   {
      /*  Get a free DMA transfer descriptor from its msg. queue  */
      txfrDesc = dmaGetFreeTxDesc(); /* all members are zeroed, may pend if no available buffers */
      txfrDesc->channel	 = channel;
      txfrDesc->txfrStatus  = NOT_READY;
      txfrDesc->srcType	 = useSG;
      txfrDesc->transferType = transferType;
      txfrDesc->srcAddr        = (UINT32 *) (srcAddr);
      txfrDesc->dstAddr        = (UINT32 *) (destAddr);
      txfrDesc->txfrSize       = txferSize;
      txfrDesc->srcMsgQ	 = srcMsgQueue;
      txfrDesc->dstMsgQ	 = dstMsgQueue;

      /* - Attach new descriptor to this channel's DMA job queue  */
#ifdef XXXX
      /* status = dmaQueueAppend(channel, txfrDesc);   /* this is a NO-OP now */
      /* if (status != OK)
       * {
       *   dmaLogMsg("sendDMATask: cannot attach new transfer to queue.");
       *   /* semGive(dmaInfo[channel].pDmaMutex); */
       *    return(-1);
       * }
       */
#endif
   }
   else /* useSG != NO_SG_LIST */
   {
      /* ----- We assume user has already built own SG list ----- */
      /* ------- And it is already attached to the queue -------- */
      txfrDesc = (txDesc_t *) srcAddr;
      txfrDesc->transferType	 = transferType;
      txfrDesc->srcMsgQ		 = srcMsgQueue;
      txfrDesc->dstMsgQ		 = dstMsgQueue;
   }

   txfrDesc->txfrControl = getDMA_ControlWord(transferType);

   if (useSG == NO_SG_LIST)
   {
      /* --- If single txfer too large, force use of SG list ---- */
      if (txferSize > MAX_DMA_TRANSFER_SIZE)
      {
         useSG = txfrDesc->srcType = SG_LIST;
         txfrDesc->internalSGFlag = 1;	/* Mark SG list as created internally */
         buildSG_List(txfrDesc);      /* build internal SG List  */
      }
   }

   if (useSG == SG_LIST)
   {
      /*
       * -------------------------------------------------------------------
       * Traverse scatter-gather list, adding control bits to each SG node
       * and flush the cache for each data node.
       * -------------------------------------------------------------------
       */
      setupSG_List4Dma(txfrDesc);  

      /* dmaPrintSGTree(txfrDesc);  diagnostic output */

   }
   else
   {
      /* -- Single-buffer transfer, not a scatter-gather list --- */
      /* -- Flush the cache on the whole incoming data buffer --- */
      cacheFlush(DATA_CACHE, (void *)(txfrDesc->srcAddr),
                              (txferSize * sizeof(int)));
   }

   if ((transferType == MEMORY_TO_MEMORY) || (transferType == PERIPHERAL_TO_MEMORY))
   {
      /* dmaLogMsg("sendDMATask: flush dest buffer."); */
      /*  Flush cache on dest. buffer for debugging purposes  */
      /* cacheFlush(DATA_CACHE, (void *) txfrDesc->dstAddr,
                        (size_t)((txfrDesc->txfrSize) * sizeof(int))); */
       /* Much faster than cahceFlush and achieves the same end result */
       cacheInvalidate(DATA_CACHE, (void *) txfrDesc->dstAddr,
                        (size_t)((txfrDesc->txfrSize) * sizeof(int)));
   }

   cachePipeFlush(); /* won't return until cache has been flushed */

   txfrDesc->txfrStatus = READY;	/* Mark txfer as ready to run	*/

   /* --------- Flush the cache on this txfrDesc  --------- */
   /* CPU place value from structure into DMA registers, thus no cache coherency problem exist */
   /* cacheFlush(DATA_CACHE, (void *) txfrDesc,
    *                    (size_t)(sizeof(struct dmaTxfrDesc)) ); */

   /* DPRINT1(1,"queueDmaTransfer: queuing txfDesc: 0x%lx\n",txfrDesc); */

   /* place the descriptor into the DMA channel's readyQueue */
   /* since more than one task can put an entry into the queue we MUST protect the access */
   /* we can use either a mutex or taskLock(),taskUnlock(), I choose taskLock() for now */
   /* taskLock(); */
   semTake(dmaInfo[channel].pDmaMutex, WAIT_FOREVER);
   status = ( RNG_LONG_PUT(dmaInfo[channel].readyQueue, (long) txfrDesc, toP)  == 0 )  ? -1 : 0;
   semGive(dmaInfo[channel].pDmaMutex);
   /* taskUnlock(); */

   /* semGive(dmaInfo[channel].pDmaMutex); */

   return(status);
}

/*
=========================================================================
NAME: dmaReqsInQueue  
=========================================================================
PURPOSE:
	Provides a general purpose user interface for determining how many DMA
        transfer have been Queued in a particular DMA channel.

AUTHOR: 
	Greg Brissey, 6/04/2004
=========================================================================
*/

int dmaReqsInQueue(int channel)
{
      return( rngLNElem( dmaInfo[channel].readyQueue ) );
}


/*
=========================================================================
NAME: dmaReqsFreeToQueue  
=========================================================================
PURPOSE:
	Provides a general purpose user interface for determining how many 
        free Queue entries remain in a particular DMA channel.

AUTHOR: 
	Greg Brissey, 6/04/2004
=========================================================================
*/
int dmaReqsFreeToQueue(int channel)
{
    return( rngLFreeElem( dmaInfo[channel].readyQueue ) );
}

/*
=========================================================================
NAME: dmaXfer     within context of calling task
=========================================================================
PURPOSE:
	Provide a general purpose user interface for transferring data
	over a DMA channel.

        Unlike sendDMA(), this initiates the DMA within the conext of the calling task.

ARGUMENTS:
	channel		- The channel number to use. Caller should have
				  previously obtained the use of a DMA channel via
				  dmaGetChannel().
	transferType	- The kind of DMA transfer desired.
				MEMORY_TO_MEMORY.....(straight RAM to RAM copy)
				MEMORY_TO_PERIPHERAL (i.e. device-paced transfer
									  to FIFO)
				PERIPHERAL_TO_MEMORY (i.e. device-paced transfer
									  from FIFO)
				MEMORY_TO_FPGA.......(a non-device paced trans-
									  fer through a register
									  that has a fixed address)
				FPGA_TO_MEMORY.......(a non-device paced trans-
									  fer through a register
									  that has a fixed address)
	useSG		- SG_LIST if scatter-gather list is to be used,
				  or NO_SG_LIST if it is a single-buffer transfer.
	srcAddr		- If useSG is NO_SG_LIST, this is the address of
				  the data buffer to be transferred.
				  If useSG is SG_LIST, this is the address of the
				  head of the scatter-gather list.
				  (The head of a scatter-gather list is a standard
				  DMA transfer descriptor, i.e. struct dmaTransferDesc,
				  a.k.a. txDesc_t and it contains a pointer to the
				  first scatter-gather list node. See dmaDrv.h)
	destAddr	- The destination address for start of transfer. This
				  could be a memory address or an FPGA register
				  address, depending on the type of transfer.
	txferSize	- No. of 32-bit words (not bytes) in the transfer.
				  This argument is ignored if a scatter-gather list
				  is in use, since each scatter-gather block carries
				  its own word count.
	srcMsgQueue	- The message queue to which caller's source data
				  buffer pointers should be returned. If NULL, the DMA
				  driver will not attempt to return data buffers
				  to the caller's queue. (For instance if caller
				  has specifically malloc'd a buffer just for this
				  purpose.) If the caller has a queue of free buffer
				  addresses, put the queue's ID here to get the
				  buffer(s) returned automatically to the free
				  source buffers queue.
	dstMsgQueue	- The message queue to which caller's destination data
				  buffer pointers should be returned. If NULL, the DMA
				  driver will not attempt to return data buffers
				  to the caller's queue. (For instance if caller
				  has specifically malloc'd a buffer just for this
				  purpose.) If the caller has a queue of free buffer
				  addresses, put the queue's ID here to get the
				  buffer(s) returned automatically to the free
				  destination buffers queue.
OUTPUT:
	Error messages only. Data is transferred from the source address to
	the destination address using the specified transfer type and SG mode.

RETURN VALUE:
	OK		- Successful operation.
	ERROR	- Some error occured, DMA transfer was not performed.

COMMENT:

        Unlike sendDMA this routine initiates the DMA from the context of the call
        task.  Thus skipping sendDMATask, and runDMATask altogether.

AUTHOR: 
	Greg Brissey, 5/27/2004
=========================================================================
*/
STATUS dmaXfer(
	int	channel,		/* Which channel is ours		*/
	txfr_t	transferType,		/* As described in comments above	*/
	usesg_t useSG,			/* As described in comments above */
	UINT32	srcAddr,		/* Source address if no SG list	*/
					/* Otherwise addr. of head of S/G list	*/
	UINT32	destAddr,		/* Destination address			*/
	int	txferSize,		/* No. of words to transfer		*/
	MSG_Q_ID srcMsgQueue,		/* Caller's msg queue for src. buffers	*/
	MSG_Q_ID dstMsgQueue		/* Caller's msg queue for dst. buffers	*/
	)
{
   txDesc_t	*txfrDesc;	/* DMA transfer descriptor		*/
   int status;
   int toP;

   status =  queueDmaTransfer(channel,transferType,useSG,srcAddr,destAddr,txferSize,srcMsgQueue,dstMsgQueue);
   if ( status == -1)
   {
       dmaLogMsg("xferDMA: unable to queue transfer\n");
       return(-1);
   }
   DPRINT(2,"dmaXfer():  execNextDmaRequestInQueue\n");
   execNextDmaRequestInQueue(channel);   /* fire off DMA engine and begin actual transfer */

#ifdef XXX
*   /* ---- Send runDMA task a message to perform the transfer ---- */
*   /* status = msgQSend(runDMAQ, (char *)&txfrDesc,
*               (UINT)(sizeof(txDesc_t *)), WAIT_FOREVER, MSG_PRI_NORMAL); */
*   status = msgQSend(runDMAQ, (char *)&channel,
*               (UINT)(sizeof(int)), WAIT_FOREVER, MSG_PRI_NORMAL);
*   if (status != OK)
*   {
*      dmaLogMsg("sendDMATask: unable to send DMA job to runDMA task.");
*      return(-1);
*   }
#endif
   return(0);
}
/*
=========================================================================
NAME: endDMATask
=========================================================================
PURPOSE:
	Provides a task for cleaning up any data or data structures left
	after a DMA transfer is completed. Responds to a message from dmaISR
	which is sent after servicing an end-of-transfer interrupt.

INPUT:
	Message on endDMA message queue. The message is a single integer
	containing the channel number for the completed transfer.

OUTPUT:
	None.

RETURN VALUE:
	None.

DETAILS:
	1. Given the channel number, look in the queue for that channel
	   and find the transfer descriptor whose status is marked
	   as DONE. If none found, log an error message and exit.
	2. If a scatter-gather queue was involved in the DMA transfer, it
	   is dismantled and the SG queue nodes are returned to the SG node
	   message queue for re-use. Any data buffers referenced in the
	   scatter-gather list are returned to the user's free buffer queue
	   for re-use.
	3. If the DMA transfer was a single-buffer non-scatter-gather
	   transfer, the single buffer is returned to the user's free
	   buffer queue for re-use.
	4. If the target destination for the transfer resides in 405Gpr
	   memory, the cache is invalidated for that memory segment so
	   that subsequent reads from that memory will show the new data.
	5. The transfer descriptor for the completed transfer is removed
	   from the queue and is returned to our internal free transfer
	   descriptor queue for re-use.

COMMENT:
	By triggering cleanup activities by the ISR, it is not necessary for
	any user routines or DMA transfer tasks to wait for the end of the
	DMA transfer.

AUTHOR: 
	Robert L. Thrift, 2004
=========================================================================
*/
void endDMATask(
	int		dummy01,	/* Unused params from taskSpawn()	*/
	int		dummy02,
	int		dummy03,
	int		dummy04,
	int		dummy05,
	int		dummy06,
	int		dummy07,
	int		dummy08,
	int		dummy09,
	int		dummy10)
{
	int			chan;
	txDesc_t	*desc;
	qhead_t		*qhead;
	STATUS		status;
	UINT32		*srcAddr;
	UINT32		*dstAddr;

/* testing */
	txDesc_t	*nextDesc;
        register int toP;
/* testing */

	FOREVER
	{
		/* -- Normally, task is pended here, waiting for a message. --- */
		msgQReceive(endDMAQ, (char *)&desc, sizeof(int), WAIT_FOREVER);
                chan = desc->channel;

#ifdef DEBUG_DMA_DRV
		dmaLogMsg("endDMATask: msg rcvd.");
#endif
               /* get this discriptor from the done queue */
               /* RNG_LONG_GET(dmaInfo[chan].doneQueue, (int) desc, toP); */

               /* moved to endDMATask(), we need a task context to lock ring buffer as we use it */
               /* begin next DMA transfer for this channel from the readyQueue if any */
               /* diagPrint(NULL,"endDmaTask(): Chk DMA Q\n"); */

               /* Moved to ISR directly */
               /* DPRINT(2," endDmaTask():  execNextDmaRequestInQueue\n"); */
               /* execNextDmaRequestInQueue(chan); now in ISR */

		/* ----- Assume the first descriptor marked DONE is ours. ----- */
		while(desc && (desc->txfrStatus != DONE))
		{
			/* -------------- Not this one, try the next -------------- */
			desc = desc->next;
		}
		if (desc == NULL)
		{
			/* ----- We never found the DONE transfer descriptor. ----- */
			sprintf(msgStr,
			"endDMATask: channel %d: DONE txfr descriptor not found.",chan);
			dmaLogMsg(msgStr);
			continue;
		}

                /*-------------------------------------------------------------*/
                /* returnBuffersAndTxDesc(int chan,  txDesc_t *desc )		*/
		/* replaces all the following code				*/
                /* Note the CacheInvalidate will still need to be in this routine */
                /*-------------------------------------------------------------*/
                /*|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
                /*-------------------------------------------------------------*/
		/* -------------- Dismantle scatter-gather list --------------- */
		if (desc->srcType == SG_LIST)
			dmaSGListRemove(chan, desc);

		/* --- If caller had only a single source buffer, return it --- */
		/* ------ dmaSGListRemove call above took care of this -------- */
		/* ----------------- For scatter-gather lists ----------------- */
		if ((desc->internalSGFlag) || (desc->srcType == NO_SG_LIST))
		{

                  /*|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
			/* ----- If txfr came into RAM, invalidate its cache ------ */
			if ((desc->transferType == MEMORY_TO_MEMORY)
				|| (desc->transferType == PERIPHERAL_TO_MEMORY)
				|| (desc->transferType == FPGA_TO_MEMORY))
			cacheInvalidate(DATA_CACHE, (void *) desc->dstAddr,
						(desc->txfrSize * sizeof(int)));
                  /*|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/

			/*  If caller provided src. buffer queue, return src. buffer  */
			if (desc->srcMsgQ)
			{
				srcAddr = desc->srcAddr;
				DPRINT1(1,"endDMATask: Return Src Addr: 0x%lx\n",srcAddr);
	                        taskLock();   /* must be sure the send is atomic */
				status = msgQSend(desc->srcMsgQ, (char *)&(srcAddr),
						(UINT)sizeof(addr_t), NO_WAIT, MSG_PRI_NORMAL);
                                taskUnlock();
				if (status == ERROR)
				{
					sprintf(msgStr,
					"dmaDrv > endDMATask: can't return caller's src buffer 0x%08x to queue pointer 0x%08x.", (unsigned int)srcAddr, (unsigned int)desc->srcMsgQ);
                                        errLogRet(LOGIT,debugInfo,msgStr);
					/* dmaLogMsg(msgStr); */
                                        /* execFunc("sendException",HARD_ERROR, 1220, 0,0,NULL,0,0,0); */
				}
			}

			/*  If caller provided dest. buffer queue, return dest. buffer  */
			if (desc->dstMsgQ)
			{
				dstAddr = desc->dstAddr;
				DPRINT2(1,"endDMATask: Return Dst Addr: 0x%lx to MsgQ: 0x%lx\n",dstAddr,desc->dstMsgQ);
	                        taskLock();   /* must be sure the send is atomic */
				/* status = msgQSend(desc->dstMsgQ, (char *)&(dstAddr),
						(UINT)sizeof(addr_t), NO_WAIT, MSG_PRI_NORMAL); */
				status = msgQSend(desc->dstMsgQ, (char *)&(dstAddr),
						 4, NO_WAIT, MSG_PRI_NORMAL);
                                taskUnlock();
				if (status == ERROR)
				{
					sprintf(msgStr,
					"dmaDrv > endDMATask: can't return caller's dst buffer 0x%08x to queue pointer 0x%08x.", (unsigned int)dstAddr, (unsigned int)desc->dstMsgQ);
                                        errLogRet(LOGIT,debugInfo,msgStr);
					/* dmaLogMsg(msgStr); */
                                        /* execFunc("sendException",HARD_ERROR, 1221, 0,0,NULL,0,0,0); */
				}
			}
		}

		/* ----- Adjust queue pointers to remove this descriptor ------ */
		/* ------- If SG list, dmaSGListRemove already did this ------- */
		if (desc->srcType != SG_LIST)
		{
			if (qhead->last == desc)
			{
				if (qhead->first == desc)   /* Was last in the queue	*/
					qhead->first = qhead->last = NULL;  /* And first!   */
				else
				{
					desc->prev->next = NULL;/* Was last, but not first  */
					qhead->last = desc->prev;
				}
			}
			else if (qhead->first == desc)
			{
				desc->next->prev = NULL;	/* Was first, but not last  */
				qhead->first = desc->next;
			}
			else
			{
				desc->prev->next = desc->next; /* Between 2 others	  */
				desc->next->prev = desc->prev;
			}
			qhead->count--;
                        
		        DPRINT1(2,"endDMATask: Freeing TxDesc: 0x%lx\n",desc);
			dmaPutFreeTxDesc(desc);	 /* Push desc. back to free list */
		}
                /*|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
		/* Go back to sleep and wait for next message				*/
#ifdef DEBUG_DMA_DRV
		dmaLogMsg("endDMATask: going back to sleep.");
#endif
	}	/* End of FOREVER loop */
}


int returnBuffersAndTxDesc(txDesc_t *desc )
{
   int chan;
   qhead_t		*qhead;
   STATUS		status;
   UINT32		*srcAddr;
   UINT32		*dstAddr;

  chan = desc->channel;

  semTake(dmaInfo[chan].pDmaMutex, WAIT_FOREVER);

   DPRINT2(+2,"returnBuffersAndTxDesc: chan: %d, desc = 0x%lx\n", chan, desc);

   /* -------------- Dismantle scatter-gather list --------------- */
   if (desc->srcType == SG_LIST)
   {
       DPRINT(+2,"returnBuffersAndTxDesc: dmaSGListRemove\n");
       dmaSGListRemove(chan, desc);
   }

   /* --- If caller had only a single source buffer, return it --- */
   /* ------ dmaSGListRemove call above took care of this -------- */
   /* ----------------- For scatter-gather lists ----------------- */
   if ((desc->internalSGFlag) || (desc->srcType == NO_SG_LIST))
   {
      /*  If caller provided src. buffer queue, return src. buffer  */
      if (desc->srcMsgQ)
      {
         srcAddr = desc->srcAddr;
         DPRINT1(+2,"returnBuffersAndTxDesc: Return Src Addr: 0x%lx\n",srcAddr);
         taskLock();   /* must be sure the send is atomic */
         status = msgQSend(desc->srcMsgQ, (char *)&(srcAddr),
                           (UINT)sizeof(addr_t), NO_WAIT, MSG_PRI_NORMAL);
         taskUnlock();
         if (status == ERROR)
         {
            sprintf(msgStr,
               "dmaDrv > endDMATask: can't return caller's src buffer 0x%08x to queue pointer 0x%08x.", (unsigned int)srcAddr, (unsigned int)desc->srcMsgQ);
            errLogRet(LOGIT,debugInfo,msgStr);
            /* dmaLogMsg(msgStr); */
            /* execFunc("sendException",HARD_ERROR, 1220, 0,0,NULL,0,0,0); */
         }
      }

      /*  If caller provided dest. buffer queue, return dest. buffer  */
      if (desc->dstMsgQ)
      {
         dstAddr = desc->dstAddr;
         DPRINT1(+2,"returnBuffersAndTxDesc: Return Dst Addr: 0x%lx\n",dstAddr);
         taskLock();   /* must be sure the send is atomic */
         status = msgQSend(desc->dstMsgQ, (char *)&(dstAddr),
                              (UINT)sizeof(addr_t), NO_WAIT, MSG_PRI_NORMAL);
         taskUnlock();
         if (status == ERROR)
         {
            sprintf(msgStr,
                  "dmaDrv > endDMATask: can't return caller's dst buffer 0x%08x to queue pointer 0x%08x.", (unsigned int)dstAddr, (unsigned int)desc->dstMsgQ);
            errLogRet(LOGIT,debugInfo,msgStr);
            /* dmaLogMsg(msgStr); */
            /* execFunc("sendException",HARD_ERROR, 1221, 0,0,NULL,0,0,0); */
         }
      }
   }

   /* ----- Adjust queue pointers to remove this descriptor ------ */
   /* ------- If SG list, dmaSGListRemove already did this ------- */
   if (desc->srcType != SG_LIST)
   {
#ifdef XXXX
      if (qhead->last == desc)
      {
         if (qhead->first == desc)   /* Was last in the queue	*/
            qhead->first = qhead->last = NULL;  /* And first!   */
         else
         {
            desc->prev->next = NULL;/* Was last, but not first  */
            qhead->last = desc->prev;
         }
      }
      else if (qhead->first == desc)
      {
         desc->next->prev = NULL;	/* Was first, but not last  */
         qhead->first = desc->next;
      }
      else
      {
         desc->prev->next = desc->next; /* Between 2 others	  */
         desc->next->prev = desc->prev;
      }
      qhead->count--;
#endif
                        
      DPRINT1(2,"endDMATask: Freeing TxDesc: 0x%lx\n",desc);
      dmaPutFreeTxDesc(desc);	 /* Push desc. back to free list */
   }
   semGive( dmaInfo[chan].pDmaMutex);
}

/*
=========================================================================
NAME: dmaGetChannel
=========================================================================
PURPOSE:
	Find a free DMA channel and assign it to the caller.

ARGUMENTS:
	base_chan - the lowest numbered channel that's acceptable. If the
				channel will be used for device paced transfers
				to/from a FIFO, base_chan should be 2, because only
				channels 2 and 3 are connected up for device pacing.
				If the channel will be used for non-device paced
				transfers, base_chan can be 0, as all 4 channels
				are usable.
RETURN VALUE:
	Returns an int for the assigned channel number (0-3).
	Returns -1 if no free channels available.

DETAILS:
	Beginning with base_chan, tries each DMA channel in succession,
	trying to obtain a lock semaphore. If it succeeds in getting one,
	then that channel is locked and assigned to the caller.
	The scatter/gather list queue for the assigned channel is
	initialized as an empty queue.

COMMENT:
	There is no limit on how long the caller may keep the channel.
	If the application requires the use of a dedicated DMA channel,
	just obtain a channel by calling this routine, then keep it forever.
	(i.e., never call dmaFreeChannel.) Please bear in mind that
	there are limits on the amount of resources available, however.

	If no channel is available, caller has 3 choices:
	1. Wait and try again later.
	2. Use a programmed I/O data transfer instead.
	   (Caution: Device-paced FIFO transfers will be unavailable.)
	3. Disable use of the feature that wanted the DMA channel.

	Caller can obtain a pointer to the assigned DMA channel info
	structure by calling dmaGetChannelInfo using the channel no.
	as argument.

AUTHOR: 
	Robert L. Thrift, 2003
=========================================================================
*/
int dmaGetChannel(int base_chan)
{
	int chan;
	STATUS status;

	for (chan = base_chan; chan < MAX_DMA_CHANNELS; chan++)
	{
		/* ----------------- Try to lock this channel ----------------- */
		status = semTake(dmaInfo[chan].lock, NO_WAIT);
		if (status == OK)
		{
			/* -------- Got one, initialize its SG list queue --------- */
			/*
			* dmaQueue[chan].first = NULL;
			* dmaQueue[chan].last  = NULL;
			* dmaQueue[chan].count = 0;
			*/
			return chan;		/* Return assigned channel number	*/
		}
	}
	/* ------------------ No DMA channels available ------------------- */
	return -1;				/* Return impossible channel no.	*/
}

/*
=========================================================================
NAME: dmaGetChannelInfo
=========================================================================
PURPOSE:
	Obtain access to DMA channel info structure for a given channel.

ARGUMENTS:
	chan	- Channel number (0-3) of the channel of interest.

RETURN VALUE:
	Pointer to struct dma_info (a.k.a. dmainfo_t) containing info about
	this DMA channel. Defined in dmaDrv.h.
	If channel number out of range, returns NULL.

COMMENTS:
	Caller should check for a NULL return value before trying to access
	the channel information.

AUTHOR: 
	Robert L. Thrift, 2003
=========================================================================
*/
dmainfo_t *dmaGetChannelInfo(int chan)
{
	/* ----------------------- Arg range check ------------------------ */
	if ((chan < 0) || (chan >= MAX_DMA_CHANNELS))
		return NULL;

	return &(dmaInfo[chan]);
}

/*
=========================================================================
NAME: dmaFreeChannel
=========================================================================
PURPOSE:
	Free a DMA channel (make it available for use) after some application
	task is done with it.

ARGUMENTS:
	chan	 - the number (0-3) of the channel to be freed.

COMMENTS:
	There is presently no way to verify that the caller is the current
	owner of the channel, or whether indeed the channel is owned by
	anyone. In order to enforce at least a minimal level of organized
	control, the application task should not try to manipulate the
	channel's lock semaphore directly.

AUTHOR: 
	Robert L. Thrift, 2003
=========================================================================
*/
void dmaFreeChannel(
		int chan	/* DMA channel no., 0-3		*/
		)
{
	STATUS status;

	/* ----------------------- Arg range check ------------------------ */
	if ((chan < 0) || (chan >= MAX_DMA_CHANNELS))
		return;
	/* ---- Channel's queue should be empty but try cleanup anyway ---- */
	/* status = dmaQueueFree(chan);	/* Try to free its queue	*/

        /*  Clear existing ready queue  */
        rngLFlush(dmaInfo[chan].readyQueue);

	/* --------------- Release channel's lock semaphore --------------- */
	semGive(dmaInfo[chan].lock);
}

/*
=========================================================================
NAME: dmaSGNodeCreate
=========================================================================
PURPOSE:
	Create a new scatter/gather node. Populate the node with source and
	destination address, transfer count, and standard control bits.

ARGUMENTS:
	srcAddr   - Source address for the DMA transfer.
	dstAddr   - Destination address for the DMA transfer.
	txfrCount - No. of transfers required (i.e., the number of 32-bit
				words in the buffer to be transferred).

RETURN VALUE:
	Pointer to new SG list node.
	srcAddr, dstAddr, and txfrCount will be inserted into the right
	places in the node structure.
	Returns NULL if node cannot be created for some reason.
	Also returns NULL if txfrCount is less than 1, or exceeds
	MAX_DMA_TRANSFER_SIZE. Caller should treat this as an error.

DEFAULT SETUP:
	1. Channel control word (word 0) in the node will zeroed.
	   At transmit time, sendDMA() will update this according to the
	   desired transfer type, so no changes need to be done at the
	   calling level.
	2. Control bits (word 4) will contain the transfer count, but
	   other control bits will be zeroed. sendDMA() will handle these
	   before starting the transfer.
	5. The transfer count is masked down to 16 bits (64K transfers).
	   Thus the max transfer count is 65535 (0xffff, 64K - 1). However,
	   the DMA hardware takes a count of zero to mean 65536 (64K). So a
	   count of either 0x10000 or 0x00000 will result in a 64K transfer.

COMMENTS:
	Does not actually "create" a node in the sense of allocating memory.
	Instead, it fetches a free node pointer from the free SG nodes
	message list. The actual nodes are all allocated at initialization.

AUTHOR: 
	Robert L. Thrift, 2003
=========================================================================
*/
sgnode_t *dmaSGNodeCreate(
		UINT32 srcAddr,		/* Transfer source address		*/
		UINT32 dstAddr,		/* Transfer destination address		*/
		UINT32 txfrCount	/* Transfer count, in 32-bit words	*/
		)
{
	sgnode_t *new;
	
	new = dmaGetFreeSGNode();	/* All members are zeroed		*/

	if (new != NULL)
	{
		/* ------------------ Fill in default values ------------------ */
		new->dmaSrcAddr  = srcAddr;
		new->dmaDstAddr  = dstAddr;
		new->dmaCtrlBits = (UINT32) txfrCount & DMA_CNT_MASK;
	}
	else
	{
		dmaLogMsg("dmaDrv -> dmaSGNodeCreate: failed to get new SG list node.");
	}
	return new;
}


/*
=========================================================================
NAME: dmaSGNodeAppend
=========================================================================
PURPOSE:
	Append a Scatter/Gather list node to an existing scatter/gather list.

ARGUMENTS:
	node	- pointer to the scatter/gather list node to be added.
	head	- The head of the SG list to add it to.

AUTHOR: 
	Robert L. Thrift, 2003
=========================================================================
*/
void dmaSGNodeAppend(
		sgnode_t *node,		/* The node to be added		*/
		txDesc_t *head		/* The SG List to add it to	*/
		)
{
	if (head->count == 0)
		head->first = node;	/* Adding to empty SG list	*/
	else
		head->last->next = node; /* Add on our new node		*/

	node->next = NULL;		/* New node is last in the list	*/
	head->last = node;
	head->count++;			/* Update node count in list	*/
}

/*
=========================================================================
NAME: dmaSGListRemove
=========================================================================
PURPOSE:
	Delete and free an entire scatter/gather list and list head.
	Adjust pointers in the queue of which the list was a member.

ARGUMENTS:
	chan     - Number of channel that has a queue containing the list.
	sgHead	 - Pointer to the head of the SG list to be removed.
	bufQueue - Id of VxWorks message queue to which addresses
			   of data buffers should be returned.

OUTPUT:
	None.

RETURN VALUE:
	OK		- The list was successfully deleted.
	ERROR	- The list head was not in the specified channel's queue.

COMMENTS:
	To destroy a SG list which was not a member of a queue, supply
	-1 as argument for the channel number. The list will be deleted
	with no attempt to adjust queue pointers.

AUTHOR: 
	Robert L. Thrift, 2003
=========================================================================
*/
STATUS dmaSGListRemove(
		int chan,	/* Which channel's list			*/
		txDesc_t *listHead	/* Head of list to remove	*/
		)
{
	txDesc_t *head;			/* Temp. pointers		*/
	qhead_t  *qhead;

	/* qhead = &(dmaQueue[chan]); */

   semTake(dmaInfo[chan].pDmaMutex, WAIT_FOREVER);

	/* ----------------- Free every node in the list ------------------ */
	DPRINT1(+2,"dmaSGListRemove: SG Nodes in List: %d\n",listHead->count);
	while (listHead->count)
	{
		sgnode_t *p;
		STATUS status;

		p = listHead->first;
		status = OK;

		/* --- Return user's data buffer if we have a queue pointer --- */
	        DPRINT2(+4,"dmaSGListRemove: listHead->srcMsgQ: 0x%lx, listHead->internalSGFlag: %d\n",
				listHead->srcMsgQ,listHead->internalSGFlag);
		if ((listHead->srcMsgQ) && (! listHead->internalSGFlag))
		{
	             DPRINT2(+2,"dmaSGListRemove: Return Src Buffer 0x%lx to user via MsgQ: 0x%lx\n",
				p->dmaSrcAddr,listHead->srcMsgQ);
			status = msgQSend(listHead->srcMsgQ, (char *)&(p->dmaSrcAddr),
						(UINT)sizeof(addr_t), NO_WAIT, MSG_PRI_NORMAL);
			if (status != OK)
			{
				sprintf(msgStr,
				"dmaSGListRemove: can't return src buffer 0x%08x to queue ptr 0x%08x.",
				(unsigned int)p->dmaSrcAddr, (unsigned int)listHead->srcMsgQ);
				dmaLogMsg(msgStr);
			}
		}
		if ((listHead->dstMsgQ) && (! listHead->internalSGFlag))
		{
	             DPRINT2(+2,"dmaSGListRemove: Return Dst Buffer 0x%lx to user via MsgQ: 0x%lx\n",
				p->dmaSrcAddr,listHead->dstMsgQ);
			status = msgQSend(listHead->dstMsgQ, (char *)&(p->dmaSrcAddr),
						(UINT)sizeof(addr_t), NO_WAIT, MSG_PRI_NORMAL);
			if (status != OK)
			{
				sprintf(msgStr,
				"dmaSGListRemove: can't return dst buffer 0x%08x to queue ptr 0x%08x.",
				(unsigned int)p->dmaSrcAddr, (unsigned int)listHead->dstMsgQ);
				dmaLogMsg(msgStr);
			}
		}
		listHead->first = p->next;
		dmaPutFreeSGNode(p);	/* Return SG node to free list	*/
		listHead->count--;
	}

        DPRINT1(2,"dmaSGListRemove: All SGNodes Freed, Now Free TxDesc: 0x%lx\n",listHead);
	dmaPutFreeTxDesc(listHead);			/* Push descriptor to free list	*/

     semGive(dmaInfo[chan].pDmaMutex);

     return OK;
}

/*
=========================================================================
NAME: dmaQueueFree
=========================================================================
PURPOSE:
	Delete and free an entire queue of DMA transfer descriptors for
	a given DMA channel.

ARGUMENTS:
	chan    - The channel (0-3) whose queue is to be cleared.

RETURN VALUE:
	OK		- The queue was successfully cleared.
	ERROR	- The queue was ill-formed; i.e. the queue count and the
			  linked list pointers did not agree.
	          Also returns ERROR if channel number out of range.
COMMENT:
	An error return indicates mismanagement of the queue due to a
	programming error. In this case, some memory could be orphaned.
	
AUTHOR: 
	Robert L. Thrift, 2003
=========================================================================
*/
#ifdef XXXXX
*
*STATUS dmaQueueFree(
*		int chan	/* Channel whose queue is to be freed	*/
*		)
*{
*	txDesc_t *desc;
*	qhead_t *qhead;
*	STATUS	status;
*	
*	/* ----------------------- Arg range check ------------------------ */
*	if ((chan < 0) || (chan >= MAX_DMA_CHANNELS))
*		return ERROR;	/* Channel number out of range	*/
*
*	status = OK;
*	qhead = dmaInfo[chan].queue;
*	while (qhead->count > 0)
*	{
*		desc = qhead->first;			/* Get first descriptor in queue*/
*        /* -------- Check for null pointer, but nonzero count --------- */
*		if (desc == NULL)
*		{
*			status = ERROR;	  /* Err if null, count nonzero	*/
*			break;
*		}
*		if (desc->srcType == SG_LIST)
*			/* -------- Handle removal of scatter-gather list --------- */
*			dmaSGListRemove(chan, desc);
*		else
*		{
*			/* ----- Handle removal of single non-SG DMA transfer ----- */
*			if (desc->srcMsgQ)
*			{
*				/*  Return user src. buffer if we have a msg queue ptr  */
*				status = msgQSend(desc->srcMsgQ,
*					(char *)&(desc->srcAddr), (UINT)sizeof(addr_t),
*				 	NO_WAIT, MSG_PRI_NORMAL);
*			}
*			if (desc->dstMsgQ)
*			{
*				/*  Return user dst. buffer if we have a msg queue ptr  */
*				status = msgQSend(desc->srcMsgQ,
*					(char *)&(desc->dstAddr), (UINT)sizeof(addr_t),
*				 	NO_WAIT, MSG_PRI_NORMAL);
*			}
*
*			/* --- Adjust queue pointers to remove this descriptor ---- */
*			if (qhead->last == desc)
*			{
*				if (qhead->first == desc)	/* Was last in the queue	*/
*					qhead->first = qhead->last = NULL;	/* And first!	*/
*				else
*				{
*					desc->prev->next = NULL;/* Was last, but not first	*/
*					qhead->last = desc->prev;
*				}
*			}
*			else if (qhead->first == desc)
*			{
*				desc->next->prev = NULL;	/* Was first, but not last	*/
*				qhead->first = desc->next;
*			}
*			else
*			{
*				desc->prev->next = desc->next; /* Between 2 others		*/
*				desc->next->prev = desc->prev;
*			}
*			qhead->count--;
*		        DPRINT1(2,"dmaQueueFree: Freeing TxDesc: 0x%lx\n",desc);
*			dmaPutFreeTxDesc(desc);		/* Push desc. back to free list	*/
*		}
*	}
*	
*    /* --------------- List pointers should now be NULL --------------- */
*	if ((qhead->first != NULL) || (qhead->last != NULL))
*		status = ERROR;
*
*	/* -------------- Forced cleanup if error condition --------------- */
*	/* -------- Could orphan some memory but we can't help it --------- */
*	/* --------- Should never happen, possible software bug? ---------- */
*	if (status == ERROR)
*	{
*        /* --------- Count was wrong, or pointers were wrong ---------- */
*		dmaLogMsg("Warning: dmaDrv > dmaQueueFree: count or ptrs. wrong.");
*		sprintf(msgStr, "qhead->first = 0x%08x", (unsigned int)qhead->first);
*		dmaLogMsg(msgStr);
*		sprintf(msgStr, "qhead->last  = 0x%08x", (unsigned int)qhead->last);
*		dmaLogMsg(msgStr);
*		sprintf(msgStr, "queue count  = %d", qhead->count);
*		dmaLogMsg(msgStr);
*		qhead->first = qhead->last = NULL;	/* Re-initialize queue		*/
*		qhead->count = 0;
*	}
*
*	return status;
*}
*
#endif
/*
=========================================================================
NAME: dmaSGListCreate
=========================================================================
PURPOSE:
	Create a new DMA scatter/gather list and add it to the queue for
	a given DMA channel.

ARGUMENTS:
	chan	- The channel whose queue will receive the new list.

RETURN VALUE:
	Returns a pointer to the new SG list head if successful.
	Returns NULL if not successful.

COMMENTS:
	This routine creates the head of an empty list. One or more SG list
	nodes representing DMA transfers, must be created and added to the
	list in order for it to be useful.

AUTHOR: 
	Robert L. Thrift, 2003
=========================================================================
*/
txDesc_t * dmaSGListCreate(
		int chan	/* Channel to receive new list		*/
		)
{
	txDesc_t *sgHead;
	STATUS   status;

	/* ------- Get a free transfer descriptor for a list header ------- */
	sgHead = dmaGetFreeTxDesc();	/* All members are zeroed	*/
    if (sgHead == NULL)
    {
    	dmaLogMsg("dmaSGListCreate: No free transfer descriptors.");
    	return NULL;
    }
    
	/* --------- Initialize new list header (initially empty) --------- */
	sgHead->channel	   = chan;
	sgHead->txfrStatus = NOT_READY; /* Not ready to send at this time	*/
	sgHead->srcType	   = SG_LIST;	/* Mark it as scatter-gather list	*/

	/* -- Append it to this channel's queue but not ready to run yet -- */
	status = dmaQueueAppend(chan, sgHead);   /* this is a noop now */
	if (status == ERROR)
	{
    	dmaLogMsg("dmaSGListCreate: cannot append SG list to queue.");
		dmaPutFreeTxDesc(sgHead);	/* Free it if attachment error	*/
		return NULL;
	}
	return sgHead;
}

/*
=========================================================================
NAME: dmaQueueAppend
=========================================================================
PURPOSE:
	Attach a new transfer descriptor to a DMA channel's queue.

ARGUMENTS:
	chan	- The DMA channel whose queue gets the new SG descriptor.
	desc	- Pointer to DMA transfer descriptor to be attached to queue.

RETURN VALUE:
	OK		- Successful operation.
	ERROR	- Failure.
	Currently, always returns OK.

COMMENT:
	Always appends the new descriptor to the tail of the queue.

AUTHOR: 
	Robert L. Thrift, 2003
=========================================================================
*/
STATUS dmaQueueAppend(
		int chan,	/* Channel to receive new desc.		*/
		txDesc_t *desc	/* Ptr. to new transfer descriptor	*/
		)
{
    desc->prev = NULL;        /* NULL if list was empty   */
    desc->next = NULL;
#ifdef XXXX
    if (dmaQueue[chan].count == 0)
        dmaQueue[chan].first = desc;         /* Adding to empty queue	*/
    else
        dmaQueue[chan].last->next = desc;    /* Add to non-empty queue	*/
    desc->prev = dmaQueue[chan].last;        /* NULL if list was empty   */
    desc->next = NULL;
    dmaQueue[chan].last = desc;
    dmaQueue[chan].count++;
#endif
	return OK;
}

/* utility routine for diagnostic */
/* turn it all off, kernel however doesn't allow this */
dataCacheOff()
{
    return (cacheDisable(DATA_CACHE));
}

/* turn it all on */
dataCacheOn()
{
    return(cacheEnable(DATA_CACHE));
}

#if 1
/*
=========================================================================
NAME: format_long
=========================================================================
PURPOSE:
	Format a long int into a decimal number with commas.
	Mostly for debugging purposes to make very large numbers more readable.

ARGUMENT:
	buf - caller's buffer for returning formatted string.
	num - long integer to be formatted into a string.

OUTPUT:
	Writes a formatted number string in caller's buffer.

RETURN VALUE:
	None.

COMMENT:
	Treats input parameter as a signed number.
	Caller should declare a buffer at least 12 chars in length.

AUTHOR: 
	Robert L. Thrift, 2004
=========================================================================
     Copyright (c) 2004, Varian Associates, Inc. All Rights Reserved.
     This software contains proprietary and confidential information
            of Varian Associates, Inc. and its contributors.
  Use, disclosure and reproduction is prohibited without prior consent.
=========================================================================
*/
void format_long(char *buf, long num)
{
	int billions,
		millions,
		thousands,
		ones;
	char *outbuf;

	outbuf = buf;
	if (num < 0)
	{
		num = -num;		/* Handle negative numbers */
		*outbuf++ = '-';	/* Prepend leading minus   */
	}
	/* ----------------- Using integer division below ----------------- */
	billions = (int)(num / 1000000000L);
	num = num - ((long) billions * 1000000000L);
	millions = (int) (num / 1000000L);
	num = num - ((long) millions * 1000000L);
	thousands = (int) (num / 1000L);
	num = num - ((long) thousands * 1000L);
	ones = (int) num;

	if (billions)
		sprintf(outbuf, "%d,%03d,%03d,%03d", billions, millions, thousands, ones);
	else
		if (millions)
			sprintf(outbuf, "%d,%03d,%03d", millions, thousands, ones);
		else
			if (thousands)
				sprintf(outbuf, "%d,%03d", thousands, ones);
			else
				sprintf(outbuf, "%d", ones);
}
#endif

chkIntStack()
{
     FOREVER
     {
        prtIntStack();
        taskDelay(calcSysClkTicks(500));
     }
}

int chkTid;
chkintstack()
{
        chkTid = taskSpawn("tChkIntStk", 250, 0,
                        5000, (FUNCPTR) chkIntStack,
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}
chkintstp()
{
    taskDelete(chkTid);
}
