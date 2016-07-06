/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
Id: dmaDebugOptions.h,v 1.4 2004/04/30 21:17:39 rthrift Exp rthrift
=========================================================================
FILE: dmaDebugOptions.h
=========================================================================
PURPOSE:
	Set debug options for DMA driver compilation.

DETAILS:
	By removing the comment symbols, debugging statements in various
	DMA driver code modules can be activated by the compiler.

	This does not mean that there actually are any debugging statements
	in the code, unless you specifically put them there.

	You can insert a debugging line as in the following examples.

	EXAMPLE 1: (formatting a variable into the debugging output)
	#ifdef DEBUG_DMA_DRV
		sprintf(msgStr, "There are %d nodes in the scatter-gather list.",
				num_nodes);
		dmaLogMsg(msgStr);
	#endif

	EXAMPLE 2: (A simple statement with no variables in it)
	#ifdef DEBUG_DMA_TEST
		dmaLogMsg("Starting DMA test 4.");
	#endif

AUTHOR: 
	Robert L. Thrift, 2004
=========================================================================
     Copyright (c) 2004, Varian Associates, Inc. All Rights Reserved.
     This software contains proprietary and confidential information
            of Varian Associates, Inc. and its contributors.
  Use, disclosure and reproduction is prohibited without prior consent.
=========================================================================
*/
#ifndef __INCdebugoptionsh
#define __INCdebugoptionsh

/* #define DEBUG_API */
/* #define DEBUG_FIFO_DRV */
/* #define DEBUG_DMA_TEST */
/* #define DEBUG_DMA_DRV */
/* #define DEBUG_DMA_MSG_QUEUE */
/* #define DEBUG_DMA_REG */
/* #define DEBUG_FIFO_TEST */
/* #define DEBUG_RESULTS_COMPARE */
/* #define DEBUG_DEBUG */

#endif /* __INCdebugoptionsh */
