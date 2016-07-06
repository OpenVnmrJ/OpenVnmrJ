/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
Id: FIFOdrv.c,v 1.7 2004/04/30 21:17:39 rthrift Exp
=========================================================================
FILE: FIFOdrv.c
=========================================================================
PURPOSE:
	Define low-level driver & utility routines for FIFO handling.

COMMENT:
	Borrows heavily from test routines originally written by
	Phil Hornung, 2003.

	FIFO control bits are edge triggered, so we must turn one off and
	then turn it back on to set it. The register pointers are classified
	as volatile, so the optimizer lets us get away with this without
	eliminating the first zeroing step as redundant.

AUTHOR: 
	Robert L. Thrift, 2004
=========================================================================
     Copyright (c) 2004, Varian Associates, Inc. All Rights Reserved.
     This software contains proprietary and confidential information
            of Varian Associates, Inc. and its contributors.
  Use, disclosure and reproduction is prohibited without prior consent.
=========================================================================
*/
#include <stdio.h>
#include <vxWorks.h>
#include <stdlib.h>
#include <taskLib.h>					/* taskDelay					*/
#include <drv/timer/ppcDecTimer.h>		/* sysClkRateGet				*/
#include "dmaMsgQueue.h"
#include "dmaDebugOptions.h"
#include "rf.h"
#include "FIFOdrv.h"

#if 0
/* 
 * -----------------------------------------------------------------------
 * Pointers into FPGA Registers.
 * Depends on register offset definitions in rf.h
 * and macros defined in FIFOdrv.h
 * -----------------------------------------------------------------------
 */
FPGA_PTR(RF_FIFOControl);
FPGA_PTR(RF_FIFOOutputSelect);
FPGA_PTR(RF_FIFOInstructionWrite);
FPGA_PTR(RF_InstructionFIFOCount);
FPGA_PTR(RF_DataFIFOCount);
FPGA_PTR(RF_InterruptStatus);
FPGA_PTR(RF_InterruptClear);
FPGA_PTR(RF_InterruptEnable);
FPGA_PTR(RF_ClearCumulativeDuration);
#endif
#define FIFO_RESET	0x4

/* -- No. of seconds to wait for something in FIFO before giving up --- */
#ifndef FIFO_WAIT_TIMEOUT_SECS
#define FIFO_WAIT_TIMEOUT_SECS  10
#endif

/* --- How many times per second to check for something in the FIFO --- */
#define FIFO_CHECK_FREQUENCY     4

/* -------- One possible definition of "something in the FIFO" -------- */
#ifndef MIN_DATA_FIFO_COUNT
#define MIN_DATA_FIFO_COUNT     20
#endif

static char msgStr[128];
/*
=========================================================================
NAME: FPGASetReg
=========================================================================
PURPOSE:
	General-purpose routine to set a bit or bit pattern into an edge-
	triggered FPGA register where the bit(s) must be inverted in the
	register, then set to the desired value in order for the desired
	value to take effect.

	This is an alternative to placing two writes as in-line code
	every time we want to affect an FPGA register bit.

INPUT:
	reg		Address of the desired register, normally constructed as
			(FPGA Base Address + Register Offset)
	mask	Unsigned int in which the bits to be modified are 1's.
	value	Unsigned int holding the final desired value of the bits.

OUTPUT:
	Writes new value out to designated FPGA register.

RETURN VALUE:
	None.

COMMENT:
	To set a 1 into a register, for example:
	a. Call this routine with mask = 1, value = 1.
	b. The register is read and exclusive-or'd with the mask.
	   Thus the bit is inverted from whatever it was originally.
	c. Write this value back out to the register.
	d. Clear the bits defined by the mask, in original register contents.
	e. OR the value of 1 into the original register contents.
	f. Write this new value out to the register.
Actual writes to the register are done last in successive instructions,
to minimize the time that the register could conceivably be set wrong,
since we really don't know what effect in general that might have.

AUTHOR: 
	Robert L. Thrift, 2004
=========================================================================
*/
void FPGASetReg(
		volatile unsigned int *reg,  /* address of FPGA register		*/
		unsigned int mask,			 /* defines the bits of interest	*/
		unsigned int value			 /* how the bits are to be set		*/
)
{
	unsigned int regvalue, xorvalue, newvalue;

	regvalue = *reg;			/* read register's contents				*/
	xorvalue = regvalue ^ mask;	/* toggle the bit(s) of interest		*/
	regvalue &= (~mask);		/* zero the bits in original value		*/
	newvalue = regvalue | (value & mask); /* OR in new value			*/
								/* only the bits in the mask are allowed*/

	/* ------------------ Now do the register writes ------------------ */
	*reg    = xorvalue;			/* flip bits in register				*/
	*reg    = newvalue;			/* set bits to final state				*/
}

/*
=========================================================================
NAME: resetFIFO
=========================================================================
PURPOSE:
	Reset FIFO functions.

INPUT:
	None.

OUTPUT:
	None.

RETURN VALUE:
	None.

COMMENT:

AUTHOR: 
	Robert L. Thrift, 2004
	Code borrowed from Phil Hornung's rsStart.c
=========================================================================
*/
void resetFIFO(void)
{
	FPGA_PTR(RF_FIFOControl);		/* Ptrs to FPGA registers (rf.h)	*/
	FPGA_PTR(RF_FIFOOutputSelect);
	FPGA_PTR(RF_InterruptClear);
	FPGA_PTR(RF_InterruptEnable);
	FPGA_PTR(RF_ClearCumulativeDuration);

	/* ------ Remove following line when FIFO reset is wired up ------- */
	resetFPGA();   			/* Hopefully this will become unnecessary	*/

	*pRF_FIFOControl = 0;			/* Trigger FIFO reset				*/
	*pRF_FIFOControl = FIFO_RESET;

	*pRF_FIFOOutputSelect = 0;		/* Select hardware output			*/
	*pRF_FIFOOutputSelect = 1;

	*pRF_InterruptClear = 0;		/* Clear interrupt status bits		*/
	*pRF_InterruptClear = 0x1ff;

	*pRF_InterruptEnable = 0;		/* Enable FIFO interrupts			*/
	*pRF_InterruptEnable = 0x1f;

	*pRF_ClearCumulativeDuration = 0; /* Clear cumulative duration reg.	*/
	*pRF_ClearCumulativeDuration = 1;
}

/*
=========================================================================
NAME: getFIFOStatus
=========================================================================
PURPOSE:
	Obtain FIFO controller interrupt status.

INPUT:
	None.

OUTPUT:
	None.

RETURN VALUE:
	Interrupt status word.

COMMENT:

AUTHOR: 
	Robert L. Thrift, 2004
	Code borrowed from Phil Hornung's rsStart.c
=========================================================================
*/
int getFIFOStatus(void)
{
	FPGA_PTR(RF_InterruptStatus);		/* Ptr to FPGA register			*/

	return(*pRF_InterruptStatus & FIFO_STATUS_MASK);
}

/*
=========================================================================
NAME: isFIFORunning
=========================================================================
PURPOSE:
	Check to see whether an output FIFO (like RF FIFO) is running.

INPUT:
	None.

OUTPUT:
	None.

RETURN VALUE:
	1	- FIFO is running.
	0	- FIFO is not running.

COMMENTS:
	1. We don't assume that the FIFO stopped bit in the status
	   register will always indicate when the FIFO is not running.
	   After a FPGA reset, the bit may be cleared even though the
	   FIFO is not running.
	2. We assume the FIFO is running if there is a nonzero count
	   in the data FIFO, and bit 0x4 (stopped bit) is not set in
	   the FIFO's status register.
	3. Results are nondeterministic in a multitasking environment;
	   i.e., FIFO can start or stop after it has been checked, but
	   before we can return with an answer.

AUTHOR: 
	Robert L. Thrift, 2004
=========================================================================
*/
int isFIFORunning(void)
{
	FPGA_PTR(RF_InterruptStatus);				/* FPGA register ptr	*/
	FPGA_PTR(RF_DataFIFOCount);

	if (((*pRF_InterruptStatus & 0x4L) == 0)	/* If stopped bit clear	*/
		 && (*pRF_DataFIFOCount != 0L))			/* and data count != 0	*/
	{
#ifdef DEBUG_FIFO_DRV
	dmaLogMsg("isFIFORunning: returning 1.");
#endif
		return 1;								/* FIFO is running,		*/
	}
  	else										/* Otherwise			*/
	{
#ifdef DEBUG_FIFO_DRV
	dmaLogMsg("isFIFORunning: returning 0.");
#endif
		return 0;								/* Not running.			*/
	}
}

/*
=========================================================================
NAME: startFIFO
=========================================================================
PURPOSE:
	Start the RF FIFO running.

INPUT:
	start_type - should be FIFO_START or FIFO_START_SYNC depending on
				 the type of start desired.

OUTPUT:
	None.

RETURN VALUE:
	None.

COMMENT:

AUTHOR: 
	Robert L. Thrift, 2004
=========================================================================
*/
void startFIFO(int start_type)
{
	FPGA_PTR(RF_FIFOControl);			/* FPGA register ptr.			*/

	*pRF_FIFOControl = 0;
	*pRF_FIFOControl = start_type;
}

/*
=========================================================================
NAME: FIFOwrite
=========================================================================
PURPOSE:
	Write a word into the RF FIFO using programmed I/O
	(as opposed to DMA transfer)

INPUT:
	None.

OUTPUT:
	None.

RETURN VALUE:
	None.

COMMENT:
	Sometimes this is handy for appending a STOP instruction to the
	end of the FIFO after a DMA transfer into the FIFO.

AUTHOR: 
	Robert L. Thrift, 2004
	Code plagiarized from Phil Hornung's rsStart.c
=========================================================================
*/
void FIFOwrite(unsigned int val)
{
	FPGA_PTR(RF_FIFOInstructionWrite);		/* FPGA register ptr.		*/

   *pRF_FIFOInstructionWrite = val;
}

/*
=========================================================================
NAME: FIFOStart
=========================================================================
PURPOSE:
    Start the output FIFO.

ARGUMENTS:
	start_type - should be FIFO_START or FIFO_START_SYNC depending on
				 the type of start desired.
RETURN VALUE:
    OK      - FIFO started successfully, or was already running.
    ERROR   - Timed out waiting for stuff to appear in data FIFO.
              (i.e., DMA transfer must have failed for some reason.)

DETAILS:
    If the FIFO is already running, just return without doing anything.
    Otherwise, wait in a loop checking the data FIFO count 4 times per
    second. When the count reaches a defined minimum start level of 20
    instructions, start the FIFO running and return.

    If we wait more than 10 seconds for something in the FIFO,
    either abort with an error, or start the FIFO anyway, depending
	in which way the #if statement below is set.

AUTHOR:
    Robert L. Thrift, 2004
=========================================================================
*/
STATUS FIFOStart(int start_type)
{
    FPGA_PTR(RF_DataFIFOCount);         /* See macro in dmaDrv.h        */
                                        /* RF_DataFIFOCount in rf.h     */
    int delayTicks;                     /* System clock rate, ticks/sec */
    int FIFOtimeout;                    /* Timeout counter              */

    /* ------------ If FIFO not already running, start it ------------- */
/*    if (! isFIFORunning())	*/
    {
        delayTicks = sysClkRateGet() / FIFO_CHECK_FREQUENCY;
        FIFOtimeout = FIFO_WAIT_TIMEOUT_SECS * FIFO_CHECK_FREQUENCY;

#ifdef DEBUG_FIFO_DRV
		dmaLogMsg("FIFOStart: looking for data in FIFO.");
#endif
        /* ------- Loop while too little data in the data FIFO -------- */
        while (*pRF_DataFIFOCount < MIN_DATA_FIFO_COUNT)
		{
            taskDelay(delayTicks);
			FIFOtimeout--;
		}

        if (FIFOtimeout < 1)
        {
            /*  Timed out waiting for something to appear in the FIFO  */
#if 0
            sprintf(msgStr,
			"FIFOStart: FIFO count only %d after %d secs, aborting.",
                    *pRF_DataFIFOCount, FIFO_WAIT_TIMEOUT_SECS);
			dmaLogMsg(msgStr);
            return ERROR;
#else
            sprintf(msgStr,
			"FIFOStart: FIFO count only %d after %d secs, starting anyway.",
                    *pRF_DataFIFOCount, FIFO_WAIT_TIMEOUT_SECS);
			dmaLogMsg(msgStr);
#endif
        }

        /* ---------- Necessary conditions met, start it up ----------- */
#ifdef DEBUG_FIFO_DRV
		sprintf(msgStr, "FIFOStart: %d in data FIFO, starting FIFO.",
				*pRF_DataFIFOCount);
		dmaLogMsg(msgStr);
#endif
        startFIFO(start_type);
    }
    return OK;
}
