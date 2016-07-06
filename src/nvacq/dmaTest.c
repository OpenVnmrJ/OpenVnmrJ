/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
Id: dmaTest.c,v 1.10 2004/04/30 21:19:24 rthrift Exp rthrift
=========================================================================
FILE: dmaTest.c
=========================================================================
PURPOSE:
	Test DMA driver operation. The test routines can also serve as
	examples for writing user code to utilize the DMA driver.

TESTS:
	test1 - Straight memory-to-memory (RAM to RAM) transfer.
			Allocate 2 buffers, transfer data from one to the other,
			and verify that correct data was transferred.
	
	test1loop - Runs test1 in a loop a number of times specified by
			an integer argument.
	
	test2 - Straight memory-to-memory (RAM to RAM transfer.
			Allocate 2 buffers, transfer data from one to the other,
			and verify that correct data was transferred.
			This is like test1, except that the data buffers are too
			large for the DMA hardware to handle in a single transfer,
			so the driver will be forced to construct an internal
			scatter-gather list to accomplish its job.
	
	test2loop - Runs test2 in a loop a number of times specified by
			an integer argument.
	
	test3 - Memory-to-memory (RAM to RAM) transfer with explicitly
			built scatter-gather list. Allocate 2 buffers, break up the
			source buffer into a series of scatter-gather transfers.
			After transfer verify that correct data was transferred.
	
	test3loop - Runs test3 in a loop a number of times specified by
			an integer argument.
	
	test4 - Single buffer to output FIFO transfer. Outputs a cycling
			LED pattern to RF FIFO, per Phil Hornung.
	
	test5 - Scatter-gather list to output FIFO transfer. Same as test
			3 except for use of scatter-gather list.
	
	test6 - Single buffer to output FIFO transfer, with buffer too large
			for a single transfer. (Driver should create scatter-gather
			list internally and send the buffer anyway.)

COMMENTS:
	Standard ANSI C/C++ compilation is assumed. There is no accomdation
	for K&R C. This is intentional.

	 Begin small lecture:
	 -------------------
	 Notice that the final argument to sendDMA() is a message queue
	 pointer to the user's free message queue for destination data
	 buffers. If this pointer is not NULL, the DMA driver code will
	 automatically restore the destination buffer's address to the
	 user's free buffer queue. This would normally only happen with
	 DMA transfers that transfer into 405 memory.

	 Automatically restoring the buffer to its free list is not always
	 a safe thing to do. The danger is that some other task might take
	 this same free buffer pointer from the queue and re-use it before
	 the task that requested the data transfer had a chance to do
	 something with the data that came in.

	 In these simple tests, that won't happen because no other task
	 will be using these particular buffers.

	 If the final sendDMA argument is a NULL, the driver will not
	 attempt to automatically restore the free buffer, but then the
	 calling task has the responsibility of restoring it after it
	 has used the data in the buffer. The problem then becomes one
	 of knowing when the DMA transfer is over.
	 
	 Another safe alternative would be to place a different message
	 queue pointer other than the free buffer queue, into sendDMA's last
	 argument. Then the driver would automatically post the buffer
	 address to that queue.  Another task pending on that message queue
	 would receive the pointer, do something with the data, then take
	 the responsibility for sending the pointer back to its free buffer
	 message queue.
	 End lecture.
	 -------------------

MORE COMMENTS:
	Currently these test routines are only compiled for the RF board.
	(i.e., they #include rf.h)

AUTHOR:
	Robert L. Thrift (2004)
=========================================================================
	 Copyright (c) 2004, Varian Associates, Inc. All Rights Reserved.
	 This software contains proprietary and confidential information
			of Varian Associates, Inc. and its contributors.
  Use, disclosure and reproduction is prohibited without prior consent.
=========================================================================
*/
#include <stdio.h>
#include <stdlib.h>
#include <cacheLib.h>
#include <taskLib.h>
#include <drv/timer/ppcDecTimer.h>  /* sysClkRateGet                    */
#ifdef INSTRUMENT
#include "wvLib.h"			/* wvEvent()								*/
#endif
#include "dmaDrv.h"
#include "rf.h"
#include "FIFOdrv.h"
#include "dmaMsgQueue.h"
#include "dmaDebugOptions.h"

/* -------- No. of times per second to check for FIFO finished -------- */
#define FIFO_CHECK_FREQUENCY 4

/* ------- Define the required buffer size for the RF1 pattern -------- */
#define RF1_PATTERN_SIZE 1088

/* ------------ Define a size for a single buffer transfer ------------ */
#define SINGLE_BUFF_SIZE 32768
#define SINGLE_LARGE_BUFF_SIZE 524288

/* -------------- Define buffer size for the LED flasher -------------- */
#define LED_FLASH_BUFF_SIZE	1088

/* ----- Define no. of scatter-gather list elements in a test run ----- */
#define NUM_SG_TXFRS	16

/* -------- A string buffer for debugging and error reporting --------- */
static char msgStr[MAX_ERR_MSG_LEN];

static int			clkRate;			/* System clock rate, ticks/sec	*/
MSG_Q_ID			srcBufferQ = NULL;	/* Msg queue for free src. bufs	*/
MSG_Q_ID			dstBufferQ = NULL;	/* Msg queue for free dst. bufs	*/

/* ------------- Prototypes for internal support routines ------------- */
/* ----- Buffer queue routines make good examples for user code. ------ */
int    compareBuffers(const UINT32 *, const UINT32 *, const int);
void   deleteFreeBufferQueue(MSG_Q_ID *);
STATUS createFreeBufferQueue(MSG_Q_ID *, const int, int);
STATUS putFreeBuffer(MSG_Q_ID *, UINT32 *);
UINT32 *getFreeBuffer(MSG_Q_ID *);

/*
=========================================================================
NAME: test1 - memory to memory DMA transfer, single buffer
=========================================================================
PURPOSE:
	An example of a simple memory-to-memory data transfer, from one RAM
	buffer to another.

DETAILS:
	1. Obtain a free DMA channel from the DMA driver.
	2. Create two buffer message queues for source and destination.
	   The source buffer is pulled from the free source buffer queue;
	   The destination buffer is pulled from the free dest. buff. queue.
	3. Fill the source buffer with a known number pattern.
	   Zero the destination buffer.
	4. Transfer source buffer data to destination buffer via DMA.
	5. Source buffer is returned to its free list by the DMA driver.
	6. Dest. buffer is returned to its free list by the DMA driver.
	6. Release the DMA channel.
	7. Compare the two buffers and verify that they are identical.
	8. Print results.
	9. Free the destination buffer.

OUTPUT:
	Various debugging print statements and error messages are possible.

RETURN VALUE:
	OK		- Successful test.
	ERROR	- Some error occurred.

COMMENT:
	DMA driver should take care of cache flushing of source buffers and,
	in cases where the transfer came into RAM, invalidating the cache on
	the destination buffer(s) after the transfer.

AUTHOR:
	Robert L. Thrift, 2004
=========================================================================
*/
STATUS test1(void)
{
	int			chan;				/* Which DMA channel we have		*/
	UINT32		*srcAddr;			/* Internal source address			*/
	UINT32		*dstAddr;			/* Internal destination address		*/
	int			nerrs;				/* Error count						*/
	int			i;					/* Temporary loop variable			*/
	STATUS		status;				/* Return code for certain calls	*/
	int			ntxfrs;				/* Transfer size in 32-bit words	*/
	int			timeout;			/* Timeout counter for DMA done		*/

	status	= OK;
	srcAddr = dstAddr = NULL;		/* In case of early error exit		*/
	clkRate = sysClkRateGet();		/* System clock rate in ticks/sec	*/

	/* -------------------- Initialize DMA driver --------------------- */
         dmaInit(TX_DESC_MSG_Q_MAX,SG_MSG_Q_MAX);

	/* ----------- Output a header message about this test ------------ */
	dmaLogMsg("\nTest 1: straight memory-to-memory single buffer transfer\n");

	/* -------- Create queue of free buffers for data sources --------- */
	/* --------- Put 2 buffers in it but we will only use one --------- */
	ntxfrs	= SINGLE_BUFF_SIZE;
	status = createFreeBufferQueue(&srcBufferQ, ntxfrs, 2);
	if (status != OK)
	{
		dmaLogMsg("Test 1: failed to create free src buffer queue, aborting.");
		status = ERROR;
		goto abortTest;
	}

	/* ------ Create queue of free buffers for data destinations ------ */
	status = createFreeBufferQueue(&dstBufferQ, ntxfrs, 2);
	if (status != OK)
	{
		dmaLogMsg("Test 1: failed to create free dst buffer queue, aborting.");
		status = ERROR;
		goto abortTest;
	}

	/* ----------- Get a DMA channel allocated for our use ------------ */
	if ((chan = dmaGetChannel(0)) == -1)
	{
		dmaLogMsg("Test 1: no DMA channels available, aborting.");
		status = ERROR;
		goto abortTest;
	}

	/* ----- Pick a destination data buffer off the message queue ----- */
	if ((dstAddr = getFreeBuffer(&dstBufferQ)) == NULL)
	{
		dmaLogMsg("Test 1: cannot get a free dest. buffer, aborting.");
		status = ERROR;
		goto abortTest;
	}

	/* ------- Pick a source data buffer off the message queue -------- */
	if ((srcAddr = getFreeBuffer(&srcBufferQ)) == NULL)
	{
		dmaLogMsg("Test 1: cannot get a free source buffer, aborting.");
		status = ERROR;
		goto abortTest;
	}

	/* ------ Initialize data in source and destination buffers ------- */
	for (i = 0; i < ntxfrs; i++)
	{
		srcAddr[i] = i;			/* Source data = 1, 2, 3, ...			*/
		dstAddr[i] = 0;			/* Destination all zeros				*/
	}

	/* ---- Request the DMA data transfer from src to dest buffer ----- */
	dmaLogMsg("Test 1: DMA Transfer requested.");

	status = dmaXfer(chan, MEMORY_TO_MEMORY, NO_SG_LIST, (UINT32) srcAddr,
					 (UINT32) dstAddr, ntxfrs, srcBufferQ, dstBufferQ);
	if (status != OK)
	{
		dmaLogMsg("Test 1: sendDMA() failed, aborting.");
		status = ERROR;
		goto abortTest;
	}

	/*
	 * -------------------------------------------------------------------
	 * Wait until the transfer is over before running the buffer compare.
	 * Do this by monitoring the free buffer queue until we see that it
	 * contains two buffers again, which means that the driver put our
	 * source buffer back onto the free queue.
	 * This is only for testing. Most production code will not need to do
	 * this, since it will not be running a buffer compare, thus does not
	 * need to know exactly when the transfer is over.
	 * -------------------------------------------------------------------
	 */
	timeout = clkRate * 40;			/* 10 second timeout @ 4 checks/sec.*/
	while ((msgQNumMsgs(srcBufferQ) < 2) && timeout)
	{
		timeout--;
		taskDelay(calcSysClkTicks(250));  /* Check 4 times per sec. */
	}
	if (timeout == 0)
	{
		dmaLogMsg("Test 1: timed out waiting for DMA transfer completion.");
		dmaLogMsg("        source buffer was not returned on time.");
		status = ERROR;
		goto abortTest;
	}

	/* ----------- Do the same thing for dest. buffer queue ----------- */
	/* ------------- Make sure buffer was returned to us -------------- */
	/*  Caution! See small lecture in opening remarks of file header!   */

	timeout = clkRate * 40;			/* 10 second timeout				*/
	while ((msgQNumMsgs(dstBufferQ) < 2) && timeout)
	{
		timeout--;
		taskDelay(calcSysClkTicks(250));  /* Check 4 times per sec. */
	}
	if (timeout == 0)
	{
		dmaLogMsg("Test 1: timed out waiting for DMA transfer completion.");
		dmaLogMsg("        destination buffer was not returned on time.");
		status = ERROR;
		goto abortTest;
	}

	/* ------------- Compare source, destination contents ------------- */
	dmaLogMsg("Test 1: Verify Transfer, compare buffers .");
        printf("Test 1: Verify Transfer, compare buffers .\n");
	nerrs = compareBuffers(srcAddr, dstAddr, ntxfrs);

	/* --------------------- Release the channel ---------------------- */
	dmaFreeChannel(chan);

	/* ------------------------ Report results ------------------------ */
	dmaLogMsg("\n---------------");
	dmaLogMsg("Test 1 results:");
	dmaLogMsg("---------------");
	sprintf(msgStr, "Assigned channel             = %d", chan);
	dmaLogMsg(msgStr);
	sprintf(msgStr, "Source address               = 0x%08x",
			(unsigned int)srcAddr);
	dmaLogMsg(msgStr);
	sprintf(msgStr, "Destination address          = 0x%08x",
			(unsigned int)dstAddr);
	dmaLogMsg(msgStr);
	sprintf(msgStr, "Transfer size (words)        = %d (0x%08x)",
					ntxfrs, ntxfrs);
	dmaLogMsg(msgStr);
	sprintf(msgStr, "No. of src/dest compare errs = %d\n", nerrs);
	dmaLogMsg(msgStr);

	if (nerrs)
		status = ERROR;

abortTest:
	/* --- These operations would not be needed in production code ---- */
	deleteFreeBufferQueue(&srcBufferQ);
	deleteFreeBufferQueue(&dstBufferQ);

        /* allow time for printout to complete prior to deleting task that is printing */
	taskDelay(calcSysClkTicks(2000));  /* 2 sec */
	dmaCleanup();
	return status;
}	/* End of test 1 */

/*
=========================================================================
NAME: test1loop	- run test1 repeatedly in a loop
=========================================================================
PURPOSE:
	Run test1 in a loop, a number of user-specified times.

ARGUMENTS:
	n	- Number of times to loop.

OUTPUT:
	None.

RETURN VALUE:
	ERROR	- test1 routine returned an error.
	OK		- all loops returned OK status.

COMMENT:
	If test1 returns an error, looping stops and test1loop exits.

AUTHOR:
	Robert L. Thrift, 2004
=========================================================================
*/
STATUS test1loop(int n)
{
	STATUS status;

	if (n < 1)
		n = 1;
	status = OK;

	while (n-- && (status == OK))
		status = test1();

	if (status != OK)
		dmaLogMsg("test1loop: stopped because of error in test1.\n");

	return status;
}

/*
=========================================================================
NAME: test2 - memory-to-memory DMA transter with internal SG list
=========================================================================
PURPOSE:
	An example of a simple memory-to-memory data transfer, from one RAM
	buffer to another. This is like test1, except that the data buffers
	are intentionally made too large for the DMA hardware to handle in
	a single transfer, so the DMA driver is forced to construct an
	internal scatter-gather list in order to move the data.

DETAILS:
	1. Obtain a free DMA channel from the DMA driver.
	2. Create two large buffers for source and destination.
	   The source buffer is pulled from the free source buffer queue;
	   The dest. buffer is pulled from the free dest. buffer queue.
	3. Fill the source buffer with a known number pattern.
	   Zero the destination buffer.
	4. Transfer source buffer data to destination buffer via DMA.
	5. Source buffer is returned to the free list by the DMA driver.
	6. Release the DMA channel.
	7. Compare the two buffers and verify that they are identical.
	8. Print results.

OUTPUT:
	Various debugging print statements and error messages are possible.

RETURN VALUE:
	OK		- Successful test.
	ERROR	- Some error occurred.

COMMENT:
	DMA driver should take care of cache flushing of source buffers and,
	in cases where the transfer came into RAM, invalidating the source
	cache on the destination buffer(s) after the transfer.

	Since the scatter-gather list is constructed internally in this case,
	the DMA driver must take care of destroying it after use.

AUTHOR:
	Robert L. Thrift, 2004
=========================================================================
*/
STATUS test2(void)
{
	int			chan;				/* Which channel we have			*/
	UINT32		*srcAddr;			/* Internal source address			*/
	UINT32		*dstAddr;			/* Internal destination address		*/
	int			nerrs;				/* Error count						*/
	int			i;					/* Temporary loop variable			*/
	STATUS		status;				/* Return code for certain calls	*/
	int			ntxfrs;				/* Transfer size in 32-bit words	*/
	int			timeout;			/* Timeout counter for DMA done		*/

	status = OK;					/* Optimistic assumption			*/
	srcAddr = dstAddr = NULL;		/* In case of early error exit		*/
	clkRate = sysClkRateGet();

	/* -------------------- Initialize DMA driver --------------------- */
         dmaInit(TX_DESC_MSG_Q_MAX,SG_MSG_Q_MAX);

	/* ----------- Output a header message about this test ------------ */
	dmaLogMsg("\nTest 2: straight memory-to-memory DMA transfer");
	dmaLogMsg("with internally generated scatter-gather list\n");

	/* ----------- Get a DMA channel allocated for our use ------------ */
	if ((chan = dmaGetChannel(0)) == -1)
	{
		dmaLogMsg("Test 2: no DMA channels available, aborting.");
		status = ERROR;
		goto abortTest;
	}

	/* -------- Create queue of free buffers for data sources --------- */
	/* --------- Put 2 buffers in it but we will only use one --------- */
	ntxfrs	= SINGLE_LARGE_BUFF_SIZE;
	status = createFreeBufferQueue(&srcBufferQ, ntxfrs, 2);
	if (status != OK)
	{
		dmaLogMsg("Test 2: failed to create free src buffer queue, aborting.");
		status = ERROR;
		goto abortTest;
	}

	/* ------ Create queue of free buffers for data destinations ------ */
	status = createFreeBufferQueue(&dstBufferQ, ntxfrs, 2);
	if (status != OK)
	{
		dmaLogMsg("Test 2: failed to create free dst buffer queue, aborting.");
		status = ERROR;
		goto abortTest;
	}

	/* ----------- Get a DMA channel allocated for our use ------------ */
	if ((chan = dmaGetChannel(0)) == -1)
	{
		dmaLogMsg("Test 2: no DMA channels available, aborting.");
		status = ERROR;
		goto abortTest;
	}

	/* ----- Pick a destination data buffer off the message queue ----- */
	if ((dstAddr = getFreeBuffer(&dstBufferQ)) == NULL)
	{
		dmaLogMsg("Test 2: cannot get a free dest. buffer, aborting.");
		status = ERROR;
		goto abortTest;
	}

	/* ------- Pick a source data buffer off the message queue -------- */
	if ((srcAddr = getFreeBuffer(&srcBufferQ)) == NULL)
	{
		dmaLogMsg("Test 2: cannot get a free source buffer, aborting.");
		status = ERROR;
		goto abortTest;
	}

	/* ------ Initialize data in source and destination buffers ------- */
	for (i = 0; i < ntxfrs; i++)
	{
		srcAddr[i] = i;			/* Source data = 1, 2, 3, ...			*/
		dstAddr[i] = 0;			/* Make destination all zeros			*/
	}

	/* ---- Request the DMA data transfer from src to dest buffer ----- */

	status = dmaXfer(chan, MEMORY_TO_MEMORY, NO_SG_LIST, (UINT32) srcAddr,
					 (UINT32) dstAddr, ntxfrs, srcBufferQ, dstBufferQ);
	if (status != OK)
	{
		dmaLogMsg("Test 2: dmaXfer() failed, aborting.");
		status = ERROR;
		goto abortTest;
	}

	/*
	 * -------------------------------------------------------------------
	 * Wait until the transfer is over before running the buffer compare.
	 * Do this by monitoring the free buffer queue until we see that it
	 * contains two buffers again, which means that the driver put our
	 * source buffer back onto the free queue.
	 * Production code will not need to do this since it will not be
	 * running a buffer compare, thus does not need to wait.
	 * -------------------------------------------------------------------
	 */
	timeout = clkRate * 40;			/* 10 second timeout				*/
	while ((msgQNumMsgs(srcBufferQ) < 2) && timeout)
	{
		timeout--;
		taskDelay(calcSysClkTicks(250));  /* Check 4 times per sec. */
	}
	if (timeout == 0)
	{
		dmaLogMsg("Test 2: timeout waiting for DMA transfer completion.");
		status = ERROR;
		goto abortTest;
	}

	/* ----------- Do the same thing for dest. buffer queue ----------- */
	/* ------------- Make sure buffer was returned to us -------------- */
	/*  Caution! See small lecture in opening remarks of file header!   */

	timeout = clkRate * 40;			/* 10 second timeout				*/
	while ((msgQNumMsgs(dstBufferQ) < 2) && timeout)
	{
		timeout--;
		taskDelay(calcSysClkTicks(250));  /* Check 4 times per sec. */
	}
	if (timeout == 0)
	{
		dmaLogMsg("Test 2: timeout waiting for DMA transfer completion.");
		dmaLogMsg("        destination buffer was not returned on time.");
		status = ERROR;
		goto abortTest;
	}

	/* ------------- Compare source, destination contents ------------- */
	nerrs = compareBuffers(srcAddr, dstAddr, ntxfrs);

	/* --------------------- Release the channel ---------------------- */
	dmaFreeChannel(chan);

	/* ------------------------ Report results ------------------------ */
	dmaLogMsg("\n---------------");
	dmaLogMsg("Test 2 results:");
	dmaLogMsg("---------------");
	sprintf(msgStr, "Assigned channel             = %d", chan);
	dmaLogMsg(msgStr);
	sprintf(msgStr, "Source address               = 0x%08x",
				(unsigned int) srcAddr);
	dmaLogMsg(msgStr);
	sprintf(msgStr, "Destination address          = 0x%08x",
				(unsigned int) dstAddr);
	dmaLogMsg(msgStr);
	sprintf(msgStr, "Transfer size (words)        = %d (0x%08x)",
					ntxfrs, ntxfrs);
	dmaLogMsg(msgStr);
	sprintf(msgStr, "No. of src/dest compare errs = %d\n", nerrs);
	dmaLogMsg(msgStr);
	if (nerrs)
		status = ERROR;

abortTest:
	/* --- These operations would not be needed in production code ---- */
	deleteFreeBufferQueue(&srcBufferQ);
	deleteFreeBufferQueue(&dstBufferQ);
        /* allow time for printout to complete prior to deleting task that is printing */
	taskDelay(calcSysClkTicks(2000));  /*  2 sec */
	dmaCleanup();
	return status;
}	/* End of test 2 */

/*
=========================================================================
NAME: test2loop - run test2 repeatedly in a loop
=========================================================================
PURPOSE:
	Run test2 in a loop, a number of user-specified times.

ARGUMENTS:
	n	- Number of times to loop.

OUTPUT:
	None.

RETURN VALUE:
	ERROR	- test2 routine returned an error.
	OK		- all loops returned OK status.

COMMENT:
	If test2 returns an error, looping stops and test2loop exits.

AUTHOR:
	Robert L. Thrift, 2004
=========================================================================
*/
STATUS test2loop(int n)
{
	STATUS status;

	if (n < 1)
		n = 1;
	status = OK;

	while (n-- && (status == OK))
		status = test2();

	if (status != OK)
		dmaLogMsg("test2loop: stopped because of error in Test 2.");

	return status;
}

/*
=========================================================================
NAME: test3 - memory-to-memory DMA transfer with user's SG list
=========================================================================
PURPOSE:
	Test memory-to-memory DMA transfers using an explicitly built
	scatter/gather list.

DETAILS:
	 1. Obtain a DMA channel from DMA driver.
	 2. Create a queue of 16 free buffers for source data.
	 3. Allocate one large memory buffer for data destination.
	 4. Create and initialize a SG list head and attach it to the
		channel's queue.
	 5. Create and initialize a SG list node for this SG list.
	 6. Take a free source buffer from the free buffer queue and
	    initialize it with a known data pattern.
	 7. Fill in transfer info for this SG node. Add the new buffer's
	    address, the destination address within the destination
	    buffer, and the word count for the transfer.
	 8. Append the SG list node to the SG list.
	 9. Repeat steps 5 - 8 until all 16 source buffers are chained
	    into the scatter-gather list.
	10. Start the DMA transfer.
	11. Wait until the transfer is done before comparing source and
	    destination data. (Transfer is done when all 16 source buffers
	    have reappeared in the free buffer queue.)
	12. Test transfer; compare source and destination contents.
	13. Print results.

RETURN VALUE:
	OK	  - No errors when comparing source and destination data.
	ERROR - An error occurred in one of the above steps.

COMMENT:

AUTHOR:
	Robert L. Thrift, 2004
=========================================================================
*/
STATUS test3(void)
{
	int		   chan;				/* Which channel we have			*/
	txDesc_t  *sgHead;				/* New head of scatter/gather list	*/
	sgnode_t  *newNode;				/* New scatter/gather list node		*/
	UINT32	  *srcAddr;				/* Internal source address			*/
	UINT32	  *dstAddr;				/* Internal destination address		*/
	UINT32	  *txfrDest;			/* Temporary						*/
	int		   i, j;				/* Temporaries						*/
	int		   nerrs;				/* Error count						*/
	int		   txfrSize;			/* Size of source buffer			*/
	int			total_txfrSize;		/* Total words transferred			*/
	int		   numSGNodes;			/* No. SG nodes being created		*/
	int			timeout;			/* Timeout counter for DMA done		*/
	STATUS	   status;

	status   = OK;					/* Optimistic assumption			*/
	srcAddr  = dstAddr = NULL;		/* In case of early error exit		*/
	txfrSize = SINGLE_BUFF_SIZE;	/* Size of one SG node's transfer	*/
	clkRate  = sysClkRateGet();		/* Get clock rate in ticks per sec.	*/

	/* -------------------- Initialize DMA driver --------------------- */
         dmaInit(TX_DESC_MSG_Q_MAX,SG_MSG_Q_MAX);

	/* ----------- Output a header message about this test ------------ */
	dmaLogMsg("\nTest 3: Memory to memory DMA transfer using");
	dmaLogMsg("        user-generated scatter-gather list.\n");

	/* ----------- Get a DMA channel allocated for our use ------------ */
	if ((chan = dmaGetChannel(0)) == -1)
	{
		dmaLogMsg("Test 3: cannot allocate a DMA channel, aborting.\n");
		status = ERROR;
		goto abortTest;
	}

	/* ------- Create a queue of free buffers for data sources -------- */
	status = createFreeBufferQueue(&srcBufferQ, txfrSize, NUM_SG_TXFRS);
	if (status != OK)
	{
		dmaLogMsg("Test 3: failed to create free src buffer queue, aborting.");
		status = ERROR;
		goto abortTest;
	}

	/* ------ Create queue of free buffers for data destinations ------ */
	/* ------- Create 2 large buffers but we will only use one. ------- */
	/* -- Each buffer is large enough to hold the whole SG transfer. -- */
	total_txfrSize = txfrSize * NUM_SG_TXFRS;
	status = createFreeBufferQueue(&dstBufferQ, total_txfrSize, 2);
	if (status != OK)
	{
		dmaLogMsg("Test 3: failed to create free dst buffer queue, aborting.");
		status = ERROR;
		goto abortTest;
	}

	/* - Pull out one large memory buffer for the destination in RAM -- */
	if ((dstAddr = getFreeBuffer(&dstBufferQ)) == NULL)
	{
		dmaLogMsg("Test 3: cannot get a free dest. buffer, aborting.");
		status = ERROR;
		goto abortTest;
	}

	/* ----------------- Zero the destination buffer ------------------ */
	for (i = 0; i < (NUM_SG_TXFRS * txfrSize); i++)
		dstAddr[i] = 0;

	/* ------- Create an SG list header and add it to the queue ------- */
	sgHead = dmaSGListCreate(chan);
	if (sgHead == NULL)
	{
		printf("Test 3: cannot create SG list head, aborting.\n");
		status = ERROR;
		goto abortTest;
	}

	/* ------ Create SG list nodes and add them to the SG list. ------- */
	txfrDest = dstAddr;
	numSGNodes = NUM_SG_TXFRS;		/* For final printout later			*/
	for (i = 0; i < NUM_SG_TXFRS; i++)
	{
		/* ----- Get a source buffer from the free buffers queue ------ */
		if ((srcAddr = getFreeBuffer(&srcBufferQ)) == NULL)
		{
			dmaLogMsg("Test 3: cannot get a free source buffer, aborting.");
			status = ERROR;
			goto abortTest;
		}

		/* ----- Fill source buffer with a simple number pattern ------ */
		for (j = 0; j < txfrSize; j++)
			srcAddr[j] = j;

		/* ------------ Get a SG list node from the driver ------------ */
		newNode = dmaSGNodeCreate((UINT32) srcAddr,
					(UINT32) txfrDest, (UINT32) txfrSize);

		/* ----------- Attach the list node to the SG list ------------ */
		dmaSGNodeAppend(newNode, sgHead);
		txfrDest   += txfrSize;		/* Bump the destination pointer		*/
	}

	/* ---- Request the DMA data transfer from src to dest buffer ----- */
	status = dmaXfer(chan, MEMORY_TO_MEMORY, SG_LIST, (UINT32) sgHead,
					 0, 0, srcBufferQ, NULL);
	if (status != OK)
	{
		dmaLogMsg("Test 3: dmaXfer() failed, aborting.");
		status = ERROR;
		goto abortTest;
	}

	/*
	 * -------------------------------------------------------------------
	 * Wait until the transfer is over before checking the results.
	 * Do this by monitoring the free buffer queue until we see that it
	 * contains all of our source buffers again, which means that the
	 * driver put them all back onto the free queue.
	 * Production code will not need to do this since it will not be
	 * running a buffer compare.
	 * -------------------------------------------------------------------
	 */
	timeout = clkRate * 40;			/* 10 second timeout				*/
	while ((msgQNumMsgs(srcBufferQ) < NUM_SG_TXFRS) && (timeout--))
		taskDelay(calcSysClkTicks(250));  /* Check 4 times per sec. */

	if (timeout < 1)
	{
		dmaLogMsg("Test 3: timed out waiting for DMA transfer completion.");
		status = ERROR;
		goto abortTest;
	}

	/* 
	 * -------------------------------------------------------------------
	 * Notice that we sent a NULL to sendDMA for the free destination
	 * buffer queue pointer, so the DMA driver will not automatically
	 * restore the destination buffer to its free queue. That's because
	 * we want to check its contents before returning it to the queue.
	 * See small lecture in opening remarks of file header!
	 * -------------------------------------------------------------------
	 */

	/* ------ Check destination buffer contents and count errors ------ */
	nerrs = 0;
	for (i = 0; i < total_txfrSize; i++)
		if (dstAddr[i] != (i % txfrSize))
		{
#ifdef DEBUG_RESULTS_COMPARE
			sprintf(msgStr, "%d\t0x%08x\t0x%08x", i, addr1[i], addr2[i]);
			dmaLogMsg(msgStr);
#endif
			nerrs++;
		}

	/*  Now we can put the destination buffer back onto its free list   */
	status = putFreeBuffer(&dstBufferQ, dstAddr);

	/* --------------------- Release the channel ---------------------- */
	dmaFreeChannel(chan);

	/* ------------------------ Report results ------------------------ */
	printf("\n---------------\n");
	printf("Test 3 results:\n");
	printf("---------------\n");
	printf("Assigned channel             = %d\n", chan);
	printf("Destination address          = 0x%08x\n",
			(unsigned int) dstAddr);
	printf("Total txfr size (words)      = %d (0x%08x)\n",
			total_txfrSize, total_txfrSize);
	printf("No. of SG list nodes         = %d\n", numSGNodes);
	printf("Transfer size per node       = %d (0x%08x)\n", txfrSize,
			txfrSize);
	printf("No. of src/dest compare errs = %d\n\n", nerrs);

	if (nerrs)
		status = ERROR;

abortTest:
	/* --- These operations would not be needed in production code ---- */
	deleteFreeBufferQueue(&srcBufferQ);
	deleteFreeBufferQueue(&dstBufferQ);
        /* allow time for printout to complete prior to deleting task that is printing */
	taskDelay(calcSysClkTicks(2000));  /*  2 sec */
	dmaCleanup();
	return status;
}

/*
=========================================================================
NAME: test3loop - run test3 repeatedly in a loop
=========================================================================
PURPOSE:
	Run test3 in a loop, a number of user-specified times.

ARGUMENTS:
	n	- Number of times to loop.

OUTPUT:
	None.

RETURN VALUE:
	ERROR	- test3 routine returned an error.
	OK		- all loops returned OK status.

COMMENT:
	If test3 returns an error, looping stops and test3loop exits.

AUTHOR:
	Robert L. Thrift, 2004
=========================================================================
*/
STATUS test3loop(int n)
{
	STATUS status;

	if (n < 1)
		n = 1;
	status = OK;

	while (n-- && (status == OK))
		status = test3();

	if (status != OK)
		dmaLogMsg("test3loop: stopped because of error in Test 3.");

	return status;
}

/*
=========================================================================
NAME: test4 - DMA transfer of memory buffer to output FIFO
=========================================================================
PURPOSE:
	An example of a simple memory-to-memory data transfer, from a RAM
	buffer to an output FIFO.

DETAILS:
	1. Obtain a free DMA channel from the DMA driver.
	2. Create two buffer message queues for source and destination.
	   The source buffer is pulled from the free source buffer queue;
	   The destination buffer is pulled from the free dest. buff. queue.
	3. Fill the source buffer with a known number pattern.
	   Zero the destination buffer.
	4. Transfer source buffer data to FIFO via DMA.
	5. Source buffer is returned to its free list by the DMA driver.
	6. Start and run the FIFO.
	6. Release the DMA channel.
	8. Print results.

OUTPUT:
	Various debugging print statements and error messages are possible.

RETURN VALUE:
	OK		- Successful test.
	ERROR	- Some error occurred.

COMMENTS:
	1. DMA driver should take care of cache flushing of source buffers.
	2. Currently the test is set up to run on an RF board.

AUTHOR:
	Robert L. Thrift, 2004
=========================================================================
*/
STATUS test4(
		int    period               /* No. of ticks per FIFO state      */
	)

{
	int			chan;				/* Which DMA channel we have		*/
	UINT32		*srcAddr;			/* Internal source address			*/
	int			i, j, k;			/* Temporaries						*/
	STATUS		status;				/* Return code for certain calls	*/
	int			ntxfrs;				/* Transfer size in 32-bit words	*/
	int			timeout;			/* Timeout counter for DMA done		*/
	int			size;				/* size of source buffer (words)	*/
	UINT32		dur;				/* Duration of 1 FIFO state			*/
	FPGA_PTR(RF_FIFOInstructionWrite);/* Address of instruction FIFO	*/

	size	= 1088;					/* Size of Phil's LED pattern		*/
	status	= OK;
	srcAddr = NULL;					/* In case of early error exit		*/
	clkRate = sysClkRateGet();		/* System clock rate in ticks/sec	*/

	/* -------------------- Initialize DMA driver --------------------- */
         dmaInit(TX_DESC_MSG_Q_MAX,SG_MSG_Q_MAX);

	if (period < 1)
	{
		dmaLogMsg("Usage: test4 <period-in-80-MHz-clock-ticks>");
		goto abortTest;
	}

	/* ----------- Output a header message about this test ------------ */
	dmaLogMsg("\nTest 4: memory-to-FIFO non-scatter-gather transfer\n");

	/* -------- Create queue of free buffers for data sources --------- */
	ntxfrs	= LED_FLASH_BUFF_SIZE + 1;
	status = createFreeBufferQueue(&srcBufferQ, ntxfrs, 2);
	if (status != OK)
	{
		dmaLogMsg("Test 4: failed to create free src buffer queue, aborting.");
		status = ERROR;
		goto abortTest;
	}

	/* ----------- Get a DMA channel allocated for our use ------------ */
	/* ---------- Note for FIFO lowest channel possible is 2 ---------- */
	if ((chan = dmaGetChannel(2)) == -1)
	{
		dmaLogMsg("Test 4: no DMA channels available, aborting.");
		status = ERROR;
		goto abortTest;
	}
#ifdef DEBUG_DMA_TEST
	sprintf(msgStr, "Test 4: channel %d.", chan);
	dmaLogMsg(msgStr);
#endif

	/* ------- Pick a source data buffer off the message queue -------- */
	if ((srcAddr = getFreeBuffer(&srcBufferQ)) == NULL)
	{
		dmaLogMsg("Test 4: cannot get a free source buffer, aborting.");
		status = ERROR;
		goto abortTest;
	}

	/* ---- Fill source buffer with Phil's LED pattern (RF1 test) ----- */
    dur = period;
    k = 0;  /* index into buffer */

    for (i = 0; i < 16; i++)
    {
		unsigned int scale;

        scale = 0xffff - i * 0x4000/8;
        srcAddr[k++] = AMP | scale;
        srcAddr[k++] = PHASE | (i * 0x4000/8);
        srcAddr[k++] = (TIMER | dur | LFIFO);   /* Word 0 loaded    */
        for (j = 0; j < 16; j++)
        {
            srcAddr[k++] = AMPS | (1<<j);       /* words 1-16       */
            srcAddr[k++] = PHASEC | (1<<j) | LFIFO ;
        }
        for (j = 0; j < 16; j++)
        {
            srcAddr[k++] = AMPS | ( 0xffff - j*0x1000 );  /* words 17-32 */
            srcAddr[k++] = PHASEC | (0xffff - j*0x1000)| LFIFO;
        }
        srcAddr[k++] = (TIMER | dur | LFIFO);
    }
	srcAddr[k] = FIFO_STOP;		/* The last instruction for the FIFO	*/
#ifdef DEBUG_DMA_TEST
	dmaLogMsg("Test 4: source buffer filled with LED pattern.");
#endif

	/* ---- Request the DMA data transfer from src to dest buffer ----- */
	status = dmaXfer(chan, MEMORY_TO_PERIPHERAL, NO_SG_LIST,
					(UINT32) srcAddr, (UINT32) pRF_FIFOInstructionWrite,
					ntxfrs, srcBufferQ, NULL);
	if (status != OK)
	{
		dmaLogMsg("Test 4: dmaXfer() failed, aborting.");
		status = ERROR;
		goto abortTest;
	}

#ifdef DEBUG_DMA_TEST
	dmaLogMsg("Test 4: starting FIFO.");
#endif
	/* -------------------- Start the FIFO running -------------------- */
    status = FIFOStart(FIFO_START);
    if (status != OK)
    {
        dmaLogMsg("Test 4: FIFOStart() failed, aborting.");
        status = ERROR;
        goto abortTest;
    }

	/*
	 * -------------------------------------------------------------------
	 * Monitor the free source buffer queue to see when the transfer is
	 * over. When the transfer is over, the driver will restore the source
	 * buffer back into the free buffer queue, and the queue will now
	 * contain two buffers again.
	 * This is only for testing. Most production code will not need to
	 * know precisely when the DMA transfer is over.
	 * -------------------------------------------------------------------
	 */
#ifdef DEBUG_DMA_TEST
	dmaLogMsg("Test 4: waiting for buffer to return.");
#endif
	timeout = clkRate * 40;			/* 10 second timeout @ 4 checks/sec.*/
	while ((msgQNumMsgs(srcBufferQ) < 2) && timeout)
	{
		timeout--;
		taskDelay(calcSysClkTicks(250));  /* Check 4 times per sec. */
	}
	if (timeout < 1)
	{
		dmaLogMsg("Test 4: timed out waiting for DMA transfer completion.");
		dmaLogMsg("        source buffer was not returned on time.");
		status = ERROR;
		goto abortTest;
	}
#ifdef DEBUG_DMA_TEST
	else
		dmaLogMsg("Test 4: buffer returned.");
#endif

	/* --------------------- Release the channel ---------------------- */
	dmaFreeChannel(chan);

	/* ------------------------ Report results ------------------------ */
	dmaLogMsg("\n---------------");
	dmaLogMsg("Test 4 results:");
	dmaLogMsg("---------------");
	sprintf(msgStr, "Assigned channel             = %d", chan);
	dmaLogMsg(msgStr);
	sprintf(msgStr, "Source address               = 0x%08x",
			(unsigned int)srcAddr);
	dmaLogMsg(msgStr);
	dmaLogMsg(msgStr);
	sprintf(msgStr, "Transfer size (words)        = %d (0x%08x)",
					ntxfrs, ntxfrs);
	dmaLogMsg(msgStr);

abortTest:
	/* --- These operations would not be needed in production code ---- */
	deleteFreeBufferQueue(&srcBufferQ);
        /* allow time for printout to complete prior to deleting task that is printing */
	taskDelay(calcSysClkTicks(2000));  /*  2 sec */
	dmaCleanup();
	return status;
}	/* End of test 4 */

/*
=========================================================================
NAME: test5
=========================================================================
PURPOSE:
	Test DMA transfer to output FIFO. Outputs a cycling LED pattern to
	RF FIFO, per Phil Hornung's example. Same as test 4, except uses a
	user-built scatter-gather list instead of a single source buffer.

DETAILS:
	 1. Obtain a DMA channel from DMA driver.
	 2. Create a queue of 16 free buffers for source data.
	 3. Allocate one large memory buffer for data destination.
	 4. Create and initialize a SG list head and attach it to the
		channel's queue.
	 5. Create and initialize a SG list node for this SG list.
	 6. Take a free source buffer from the free buffer queue.
	 7. Fill the source buffer with A-codes to cycle the LEDs on the
	    test board.
	 8. Fill in transfer info for this SG node. Add the new buffer's
	    address, the destination address (a FIFO register), and the
	    word count for the transfer.
	 9. Append the SG list node to the SG list.
	10. Repeat steps 5 - 9 until all 16 source buffers are chained
	    into the scatter-gather list.
	11. Transfer source buffer data to output FIFO via DMA using
	    device paced transfer mode.
	12. Wait until data has started appearing in the data FIFO, then
	    start the FIFO running.
	13. Source buffer is returned to the free list by the DMA driver.
	14. Release the DMA channel.
	15. Free the destination buffer.

RETURN VALUE:
	OK	  - No errors.
	ERROR - An error occurred in one of the above steps.

COMMENT:
	FIFO operation is confirmed by visual observation of the LEDs, and
	by the fact that the FIFO stops after the pattern is output. (The
	output pattern has a STOP code appended to the end of the data.)

	Correct DMA driver operation will automatically restore all of the
	user's original data buffers to the free buffer list.

AUTHOR:
	Robert L. Thrift, 2004
=========================================================================
*/
STATUS test5(void)
{
	printf("\nTest 5: not yet implemented.\n\n");
	return OK;
}

/*
=========================================================================
NAME: test6
=========================================================================
PURPOSE:
	Test DMA transfer of a single buffer to output FIFO. Outputs a
	cycling LED pattern to RF FIFO, per Phil Hornung's example.
	In this test, the buffer is intentionally made too large for the DMA
	hardware to handle in a single transfer, so the DMA driver is forced
	to construct an internal scatter-gather list in order to move the
	data.

DETAILS:
	1. Obtain a free DMA channel from the DMA driver.
	2. Create a queue of two large buffers to use for source data.
	   (The test will use only one of them.)
	   Take a free buffer from the queue for the DMA transfer.
	3. Fill the source buffer with A-codes to cycle the LEDs on the
	   test board.
	4. Request a DMA transfer of the source buffer data to output FIFO
	   via DMA using device paced transfer mode. Since the buffer is
	   larger than the DMA hardware single-transfer limit (64K words),
	   the DMA driver should split the transfer up into a list of
	   scatter-gather transfers.
	5. Wait until data has started appearing in the data FIFO, then
	   start the FIFO running.
	6. When the DMA operation is done, driver should dismantle the
	   internal scatter-gather list and reclaim its SG list nodes.
	   It should return the original data buffer to the user's free list.
	7. Release the DMA channel.
	8. Free the destination buffer.

RETURN VALUE:
	OK	  - No errors.
	ERROR - An error occurred in one of the above steps.

COMMENT:
	FIFO operation is confirmed by visual observation of the LEDs, and
	by the fact that the FIFO stops after the pattern is output. (The
	output pattern has a STOP code appended to the end of the data.)

AUTHOR:
	Robert L. Thrift, 2004
=========================================================================
*/
STATUS test6(void)
{
	printf("\nTest 6: not yet implemented.\n\n");
	return OK;
}

/*
=========================================================================
NAME: compareBuffers
=========================================================================
PURPOSE:
	Compare two memory buffers, count differences.
	Supports test routines that need to check memory-to-memory DMA for
	correct operation.

ARGUMENTS:
	addr1	- address of first buffer
	addr2	- address of second buffer
	words	- no. of words to compare

OUTPUT:
	If DEBUG_RESULTS_COMPARE is defined, prints a listing of differences
	found, which are presumed to be errors.

RETURN VALUE:
	No. of differences found.

COMMENT:

AUTHOR:
	Robert L. Thrift, 2004
=========================================================================
*/
int compareBuffers(const UINT32 *addr1, const UINT32 *addr2, const int words)
{
	int i, ndiffs;

	ndiffs = 0;
	for (i = 0; i < words; i++)
		if (addr1[i] != addr2[i])
		{
#ifdef DEBUG_RESULTS_COMPARE
			sprintf(msgStr, "%d\t0x%08x\t0x%08x", i, addr1[i], addr2[i]);
			dmaLogMsg(msgStr);
#endif
			ndiffs++;
		}
	return ndiffs;
}

/*
=========================================================================
NAME: deleteFreeBufferQueue
=========================================================================
PURPOSE:
	Delete queue of free data buffers created by createFreeBufferQueue().

ARGUMENT:	bufQ - message queue pointer

DETAILS:
	Retrieves pointers to free data buffers off the message queue, and
	frees the buffers until the message queue is empty, then deletes
	the message queue itself.

COMMENTS:
	This is a user-level message queue, (as opposed to DMA driver level),
	used for managing data buffers intended for holding data to be
	transferred via DMA.

AUTHOR:
	Robert L. Thrift, 2004
=========================================================================
*/
void deleteFreeBufferQueue(MSG_Q_ID *bufQ)
{
	UINT32 *buf;
	STATUS status;

	if (*bufQ)
	{
		while (msgQNumMsgs(*bufQ))
		{
			msgQReceive(*bufQ, (char *) &buf, sizeof(UINT32 *), NO_WAIT);
			if ((int)buf != ERROR)
				free((void *) buf);
			else
				dmaLogMsg("deleteFreeBufferQueue: msg receive err.");
		}
		status = msgQDelete(*bufQ);
		if (status == ERROR)
			dmaLogMsg("deleteFreeBufferQueue: err deleting msg queue.");
		*bufQ = NULL;
	}
}

/*
=========================================================================
NAME: createFreeBufferQueue
=========================================================================
PURPOSE:
	Create a queue for holding pointers to free data buffers, and
	populate the queue with pre-allocated buffers.

ARGUMENTS:
	nbufs - The number of buffers with which to pre-load the queue.
	size  - The size (in 32-bit words) of each buffer to be created.

RETURN VALUE:
	OK	  - successful operation.
	ERROR - could not create the queue, or could not populate it.

DETAILS:
	Creates the free buffer message queue.
	mallocs <nbufs> buffers of size <size> and places their addresses
	on the queue.

COMMENTS:
	This is a user-level message queue, (as opposed to DMA driver level),
	used for managing data buffers intended for holding data to be
	transferred via DMA.

	If queue already exists, it is deleted and re-created.

AUTHOR:
	Robert L. Thrift, 2004
=========================================================================
*/
STATUS createFreeBufferQueue(MSG_Q_ID *bufQ, const int size, int num)
{
	STATUS status;
	unsigned int *buf, n;

	n = num;
	if (*bufQ)
		deleteFreeBufferQueue(bufQ);

	*bufQ = msgQCreate(num, sizeof(UINT32 *), MSG_Q_FIFO);
	if (*bufQ == NULL)
		return ERROR;

	while (n-- > 0)
	{
		if ((buf = (UINT32 *)(malloc(sizeof(UINT32 *) * size))) != NULL)
		{
			status = msgQSend(*bufQ, (char *)&buf,
						sizeof(UINT32 *), NO_WAIT, MSG_PRI_NORMAL);
			if (status == ERROR)
			{
				dmaLogMsg(
				"dmaTest: createFreeBufferQueue: cannot add to queue.");
				return ERROR;
			}
		}
		else
		{
			dmaLogMsg("createFreeBufferMsgQueue: malloc error.");
			return ERROR;
		}
	}
	return OK;
}

/*
=========================================================================
NAME: getFreeBuffer
=========================================================================
PURPOSE:
	Retrieve a pointer to a free data buffer from the queue.

RETURN VALUE:
	Pointer to a free data buffer. Returns NULL on error.

COMMENTS:
	This is a user-level message queue, (as opposed to DMA driver level),
	used for managing data buffers intended for holding data to be
	transferred via DMA.

	Pends waiting for a free buffer if queue is currently empty.
	Free buffers should be returned to the queue by a different task,
	so if this call pends waiting for a buffer, the other task is still
	able to run and place a buffer on the queue. Normally, free buffers
	are automatically returned to the queue by the DMA driver as their
	data is transferred via DMA, so the user level code does not have
	to manage that.

AUTHOR:
	Robert L. Thrift, 2004
=========================================================================
*/
UINT32 *getFreeBuffer(MSG_Q_ID *bufQ)
{
	UINT32 *buf;
	STATUS status;

    /* ------ We will pend here if all free buffers are in use. ------- */
	status = msgQReceive(*bufQ, (char *)&buf, sizeof(UINT32 *),
						 WAIT_FOREVER);
	if (status == ERROR)
	{
		dmaLogMsg("getFreeBuffer: failed to get buffer ptr.");
		return NULL;
	}

	return buf;
}

/*
=========================================================================
NAME: putFreeBuffer
=========================================================================
PURPOSE:
	Place a pointer to a free data buffer into the queue.

ARGUMENTS:
	buf		- address of the data buffer.

RETURN VALUE:
	OK		- successful operation.
	ERROR	- queue full, or queue does not exist.

COMMENTS:
	This is a user-level message queue, (as opposed to DMA driver level),
	used for managing data buffers intended for holding data to be
	transferred via DMA.

AUTHOR:
	Robert L. Thrift, 2004
=========================================================================
*/
STATUS putFreeBuffer(MSG_Q_ID *bufQ, UINT32 *buf)
{
	UINT32 *temp;
	STATUS status;

	if (! *bufQ)
		return ERROR;

	temp = buf;
	status = msgQSend(*bufQ, (char *)&temp, sizeof(UINT32 *),
						NO_WAIT, MSG_PRI_NORMAL);
	if (status == ERROR)
		dmaLogMsg("putFreeBuffer: failed to queue buffer.");
	return status;
}


/* readDataFifo */
/* Just to check the data FIFO from the shell */
int readDataFifo(void)
{
	FPGA_PTR(RF_DataFIFOCount);

	return (*pRF_DataFIFOCount);
}

/*
 * Just a slight modification to test1, instead of sending a messsage to sendDMA task the 
 * the dma is done within the context of the calling routine, via dmaXfer() routine.
 *   Author:  Greg Brissey
 */
STATUS test1A(void)
{
	int			chan;				/* Which DMA channel we have		*/
	UINT32		*srcAddr;			/* Internal source address			*/
	UINT32		*dstAddr;			/* Internal destination address		*/
	int			nerrs;				/* Error count						*/
	int			i;					/* Temporary loop variable			*/
	STATUS		status;				/* Return code for certain calls	*/
	int			ntxfrs;				/* Transfer size in 32-bit words	*/
	int			timeout;			/* Timeout counter for DMA done		*/

	status	= OK;
	srcAddr = dstAddr = NULL;		/* In case of early error exit		*/
	clkRate = sysClkRateGet();		/* System clock rate in ticks/sec	*/

	/* -------------------- Initialize DMA driver --------------------- */
         dmaInit(TX_DESC_MSG_Q_MAX,SG_MSG_Q_MAX);

	/* ----------- Output a header message about this test ------------ */
	dmaLogMsg("\nTest 1: straight memory-to-memory single buffer transfer\n");

	/* -------- Create queue of free buffers for data sources --------- */
	/* --------- Put 2 buffers in it but we will only use one --------- */
	ntxfrs	= SINGLE_BUFF_SIZE;
	status = createFreeBufferQueue(&srcBufferQ, ntxfrs, 2);
	if (status != OK)
	{
		dmaLogMsg("Test 1: failed to create free src buffer queue, aborting.");
		status = ERROR;
		goto abortTest;
	}

	/* ------ Create queue of free buffers for data destinations ------ */
	status = createFreeBufferQueue(&dstBufferQ, ntxfrs, 2);
	if (status != OK)
	{
		dmaLogMsg("Test 1: failed to create free dst buffer queue, aborting.");
		status = ERROR;
		goto abortTest;
	}

	/* ----------- Get a DMA channel allocated for our use ------------ */
	if ((chan = dmaGetChannel(0)) == -1)
	{
		dmaLogMsg("Test 1: no DMA channels available, aborting.");
		status = ERROR;
		goto abortTest;
	}

	/* ----- Pick a destination data buffer off the message queue ----- */
	if ((dstAddr = getFreeBuffer(&dstBufferQ)) == NULL)
	{
		dmaLogMsg("Test 1: cannot get a free dest. buffer, aborting.");
		status = ERROR;
		goto abortTest;
	}

	/* ------- Pick a source data buffer off the message queue -------- */
	if ((srcAddr = getFreeBuffer(&srcBufferQ)) == NULL)
	{
		dmaLogMsg("Test 1: cannot get a free source buffer, aborting.");
		status = ERROR;
		goto abortTest;
	}

	/* ------ Initialize data in source and destination buffers ------- */
	for (i = 0; i < ntxfrs; i++)
	{
		srcAddr[i] = i;			/* Source data = 1, 2, 3, ...			*/
		dstAddr[i] = 0;			/* Destination all zeros				*/
	}

	/* ---- Request the DMA data transfer from src to dest buffer ----- */
	dmaLogMsg("Test 1: DMA Transfer requested.");

        status = dmaXfer(chan, MEMORY_TO_MEMORY, NO_SG_LIST, (UINT32) srcAddr,
                               (UINT32) dstAddr, ntxfrs, srcBufferQ, dstBufferQ);

	/*
	 * -------------------------------------------------------------------
	 * Wait until the transfer is over before running the buffer compare.
	 * Do this by monitoring the free buffer queue until we see that it
	 * contains two buffers again, which means that the driver put our
	 * source buffer back onto the free queue.
	 * This is only for testing. Most production code will not need to do
	 * this, since it will not be running a buffer compare, thus does not
	 * need to know exactly when the transfer is over.
	 * -------------------------------------------------------------------
	 */
	timeout = clkRate * 40;			/* 10 second timeout @ 4 checks/sec.*/
	while ((msgQNumMsgs(srcBufferQ) < 2) && timeout)
	{
		timeout--;
		taskDelay(calcSysClkTicks(250));  /* Check 4 times per sec. */
	}
	if (timeout == 0)
	{
		dmaLogMsg("Test 1: timed out waiting for DMA transfer completion.");
		dmaLogMsg("        source buffer was not returned on time.");
		status = ERROR;
		goto abortTest;
	}

	/* ----------- Do the same thing for dest. buffer queue ----------- */
	/* ------------- Make sure buffer was returned to us -------------- */
	/*  Caution! See small lecture in opening remarks of file header!   */

	timeout = clkRate * 40;			/* 10 second timeout				*/
	while ((msgQNumMsgs(dstBufferQ) < 2) && timeout)
	{
		timeout--;
		taskDelay(calcSysClkTicks(250));  /* Check 4 times per sec. */
	}
	if (timeout == 0)
	{
		dmaLogMsg("Test 1: timed out waiting for DMA transfer completion.");
		dmaLogMsg("        destination buffer was not returned on time.");
		status = ERROR;
		goto abortTest;
	}

	/* ------------- Compare source, destination contents ------------- */
	dmaLogMsg("Test 1: Verify Transfer, compare buffers .");
        printf("Test 1: Verify Transfer, compare buffers .\n");
	nerrs = compareBuffers(srcAddr, dstAddr, ntxfrs);

	/* --------------------- Release the channel ---------------------- */
	dmaFreeChannel(chan);

	/* ------------------------ Report results ------------------------ */
	dmaLogMsg("\n---------------");
	dmaLogMsg("Test 1 results:");
	dmaLogMsg("---------------");
	sprintf(msgStr, "Assigned channel             = %d", chan);
	dmaLogMsg(msgStr);
	sprintf(msgStr, "Source address               = 0x%08x",
			(unsigned int)srcAddr);
	dmaLogMsg(msgStr);
	dmaLogMsg(msgStr);
	sprintf(msgStr, "Transfer size (words)        = %d (0x%08x)",
					ntxfrs, ntxfrs);
	dmaLogMsg(msgStr);
	sprintf(msgStr, "No. of src/dest compare errs = %d\n", nerrs);
	dmaLogMsg(msgStr);

	if (nerrs)
		status = ERROR;

abortTest:
	/* --- These operations would not be needed in production code ---- */
	deleteFreeBufferQueue(&srcBufferQ);
	deleteFreeBufferQueue(&dstBufferQ);
        /* allow time for printout to complete prior to deleting task that is printing */
	taskDelay(calcSysClkTicks(2000));  /*  2 sec */
	dmaCleanup();
	return status;
}	/* End of test 1 */


/*
 *  Simple test of queue DMA transfers
 *   transfes of the the same buffer are queued, then the trasnfer if begun.
 *
 * Author: Greg Brissey
 */
STATUS testDmaQ(void)
{
	int			chan;				/* Which DMA channel we have		*/
	UINT32		*srcAddr;			/* Internal source address			*/
	UINT32		*dstAddr;			/* Internal destination address		*/
	int			nerrs;				/* Error count						*/
	int			i;					/* Temporary loop variable			*/
	STATUS		status;				/* Return code for certain calls	*/
	int			ntxfrs;				/* Transfer size in 32-bit words	*/
	int			timeout;			/* Timeout counter for DMA done		*/

	status	= OK;
	srcAddr = dstAddr = NULL;		/* In case of early error exit		*/
	clkRate = sysClkRateGet();		/* System clock rate in ticks/sec	*/

	/* -------------------- Initialize DMA driver --------------------- */
         dmaInit(TX_DESC_MSG_Q_MAX,SG_MSG_Q_MAX);

	/* ----------- Output a header message about this test ------------ */
	dmaLogMsg("\nTest 1: straight memory-to-memory single buffer transfer\n");

	/* -------- Create queue of free buffers for data sources --------- */
	/* --------- Put 2 buffers in it but we will only use one --------- */
	ntxfrs	= SINGLE_BUFF_SIZE;
	status = createFreeBufferQueue(&srcBufferQ, ntxfrs, 2);
	if (status != OK)
	{
		dmaLogMsg("Test 1: failed to create free src buffer queue, aborting.");
		status = ERROR;
		goto abortTest;
	}

	/* ------ Create queue of free buffers for data destinations ------ */
	status = createFreeBufferQueue(&dstBufferQ, ntxfrs, 2);
	if (status != OK)
	{
		dmaLogMsg("Test 1: failed to create free dst buffer queue, aborting.");
		status = ERROR;
		goto abortTest;
	}

	/* ----------- Get a DMA channel allocated for our use ------------ */
	if ((chan = dmaGetChannel(0)) == -1)
	{
		dmaLogMsg("Test 1: no DMA channels available, aborting.");
		status = ERROR;
		goto abortTest;
	}

	/* ----- Pick a destination data buffer off the message queue ----- */
	if ((dstAddr = getFreeBuffer(&dstBufferQ)) == NULL)
	{
		dmaLogMsg("Test 1: cannot get a free dest. buffer, aborting.");
		status = ERROR;
		goto abortTest;
	}

	/* ------- Pick a source data buffer off the message queue -------- */
	if ((srcAddr = getFreeBuffer(&srcBufferQ)) == NULL)
	{
		dmaLogMsg("Test 1: cannot get a free source buffer, aborting.");
		status = ERROR;
		goto abortTest;
	}

	/* ------ Initialize data in source and destination buffers ------- */
	for (i = 0; i < ntxfrs; i++)
	{
		srcAddr[i] = i;			/* Source data = 1, 2, 3, ...			*/
		dstAddr[i] = 0;			/* Destination all zeros				*/
	}

	/* ---- Request the DMA data transfer from src to dest buffer ----- */
	dmaLogMsg("Test 1: DMA Transfer requested.");

        status = queueDmaTransfer(chan, MEMORY_TO_MEMORY, NO_SG_LIST, (UINT32) srcAddr,
                               (UINT32) dstAddr, ntxfrs, srcBufferQ, dstBufferQ);
        status = queueDmaTransfer(chan, MEMORY_TO_MEMORY, NO_SG_LIST, (UINT32) srcAddr,
                               (UINT32) dstAddr, ntxfrs, NULL, NULL);
        status = queueDmaTransfer(chan, MEMORY_TO_MEMORY, NO_SG_LIST, (UINT32) srcAddr,
                               (UINT32) dstAddr, ntxfrs, NULL, NULL);
        status = queueDmaTransfer(chan, MEMORY_TO_MEMORY, NO_SG_LIST, (UINT32) srcAddr,
                               (UINT32) dstAddr, ntxfrs, NULL, NULL);
        status = queueDmaTransfer(chan, MEMORY_TO_MEMORY, NO_SG_LIST, (UINT32) srcAddr,
                               (UINT32) dstAddr, ntxfrs, NULL, NULL);

        execNextDmaRequestInQueue(chan);

	/*
	 * -------------------------------------------------------------------
	 * Wait until the transfer is over before running the buffer compare.
	 * Do this by monitoring the free buffer queue until we see that it
	 * contains two buffers again, which means that the driver put our
	 * source buffer back onto the free queue.
	 * This is only for testing. Most production code will not need to do
	 * this, since it will not be running a buffer compare, thus does not
	 * need to know exactly when the transfer is over.
	 * -------------------------------------------------------------------
	 */
	timeout = clkRate * 40;			/* 10 second timeout @ 4 checks/sec.*/
	while ((msgQNumMsgs(srcBufferQ) < 2) && timeout)
	{
		timeout--;
		taskDelay(calcSysClkTicks(250));  /* Check 4 times per sec. */
	}
	if (timeout == 0)
	{
		dmaLogMsg("Test 1: timed out waiting for DMA transfer completion.");
		dmaLogMsg("        source buffer was not returned on time.");
		status = ERROR;
		goto abortTest;
	}

	/* ----------- Do the same thing for dest. buffer queue ----------- */
	/* ------------- Make sure buffer was returned to us -------------- */
	/*  Caution! See small lecture in opening remarks of file header!   */

	timeout = clkRate * 40;			/* 10 second timeout				*/
	while ((msgQNumMsgs(dstBufferQ) < 2) && timeout)
	{
		timeout--;
		taskDelay(calcSysClkTicks(250));  /* Check 4 times per sec. */
	}
	if (timeout == 0)
	{
		dmaLogMsg("Test 1: timed out waiting for DMA transfer completion.");
		dmaLogMsg("        destination buffer was not returned on time.");
		status = ERROR;
		goto abortTest;
	}

	/* ------------- Compare source, destination contents ------------- */
	dmaLogMsg("Test 1: Verify Transfer, compare buffers .");
        printf("Test 1: Verify Transfer, compare buffers .\n");
	nerrs = compareBuffers(srcAddr, dstAddr, ntxfrs);

	/* --------------------- Release the channel ---------------------- */
	dmaFreeChannel(chan);

	/* ------------------------ Report results ------------------------ */
	dmaLogMsg("\n---------------");
	dmaLogMsg("Test 1 results:");
	dmaLogMsg("---------------");
	sprintf(msgStr, "Assigned channel             = %d", chan);
	dmaLogMsg(msgStr);
	sprintf(msgStr, "Source address               = 0x%08x",
			(unsigned int)srcAddr);
	dmaLogMsg(msgStr);
	sprintf(msgStr, "Destination address          = 0x%08x",
			(unsigned int)dstAddr);
	dmaLogMsg(msgStr);
	sprintf(msgStr, "Transfer size (words)        = %d (0x%08x)",
					ntxfrs, ntxfrs);
	dmaLogMsg(msgStr);
	sprintf(msgStr, "No. of src/dest compare errs = %d\n", nerrs);
	dmaLogMsg(msgStr);

	if (nerrs)
		status = ERROR;

abortTest:
	/* --- These operations would not be needed in production code ---- */
	deleteFreeBufferQueue(&srcBufferQ);
	deleteFreeBufferQueue(&dstBufferQ);
        /* allow time for printout to complete prior to deleting task that is printing */
	taskDelay(calcSysClkTicks(5000));  /*  5 sec */
	dmaCleanup();
	return status;
}	/* End of test 1 */

