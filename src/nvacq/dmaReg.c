/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
Id: dmaReg.c,v 1.3 2004/04/25 13:29:55 rthrift Exp rthrift
=========================================================================
FILE: dmaReg.c
=========================================================================
Purpose:
	Provide interface routines for setting/clearing/examining DMA
	hardware registers in the PowerPC. These routines interface with
	routines in dmaDcr.s provided with VxWorks.
	The routines here provide register access using the channel number
	as an input argument, which reduces the number of separate calls
	that have to be made in order to manage the registers.

Externally callable routines:
	Routine				Affected Register
	------------------  ------------------
	dmaDisableChan		Channel configuration register
						(a.k.a. channel control register)
	---
	dmaGetConfigReg		Channel configuration register
	dmaSetConfigReg		(a.k.a. channel control register)
	---
	dmaGetCountReg		Channel count register
	dmaSetCountReg
	---
	dmaGetDestReg		Channel destination address register
	dmaSetDestReg
	---
	dmaGetSrcReg		Channel source address register
	dmaSetSrcReg
	---
	dmaGetSGReg			Channel scatter/gather descriptor address register
	dmaSetSGReg			(i.e. address of scatter/gather list)
	---
	dmaGetSGCommandReg	Channel scatter/gather command register
	dmaEnableSGCommandReg
	dmaDisableSGCommandReg
	---
	dmaGetStatusReg		DMA status register (all 4 channels)
	dmaSetStatusReg
	dmaGetDevicePacingStatus Test whether DmaReq is on or off
        dmaGetDeviceRunning     Test whether DMA is active
	---
	dmaGetSleepReg		DMA sleep register
	dmaSetSleepReg
	---
	dmaGetPolarityReg	DMA polarity register
	dmaSetPolarityReg
 
Comments:
	Specific to PowerPC 405GPr CPU.
	ANSI C/C++ compilation is assumed.

Author:
	Robert L. Thrift, 2003
=========================================================================
     Copyright (c) 2003, Varian Associates, Inc. All Rights Reserved.
     This software contains proprietary and confidential information
            of Varian Associates, Inc. and its contributors.
  Use, disclosure and reproduction is prohibited without prior consent.
=========================================================================
*/
#include <vxWorks.h>
#include "dmaDcr.h"
#include "dmaReg.h"
#define _DMAREG_C_

/* 
 * -----------------------------------------------------------------------
 * dmaDisableChan
 * Disable a given DMA channel in order to modify its registers.
 * The purpose is to stop the channel if it is running before any
 * registers are modified.
 * 
 * Input:
 * 	chan - which channel (0-3) to disable.
 * Comments:
 *	DMA channel registers can be read at any time, although certain of
 *	their contents might be suspect if the channel is currently running.
 *	However, it is forbidden to write to a channel that is currently
 *	enabled, because very bad things might happen. This routine provides
 *	a call to disable a channel before we write to its registers. We do
 *	not enable it again, as some setup will undoubtedly be required to
 *	start another DMA transfer.
 *
 *  This routine clears both the Channel Enable and the Channel
 *  Interrupt Enable bits in the control register for the specified
 *	channel. It also clears the Scatter/Gather Enable bits for the
 *	channel by simultaneously writing a 1 to the Enable Mask bit and a 0
 *	to the Enable bit.
 * -----------------------------------------------------------------------
 */
void dmaDisableChan(int chan)
{
	UINT32 regval, ceMask;

    /* ------------------- Mask off CE and CIE bits -------------------- */
	ceMask = ~(DMA_CR_CE | DMA_CR_CIE);
	
	switch (chan) {
		case 0:
			regval = sysDcrDmasgcGet();			/* Read SG command reg.	*/
			sysDcrDmasgcSet(DMA_SGC_EM0 | (regval & (~(DMA_SGC_SSG0))));
			regval = sysDcrDmacr0Get();			/* Read config reg.		*/
			sysDcrDmacr0Set(regval & ceMask);	/* Clear enable bits	*/
			break;
		case 1:
			regval = sysDcrDmasgcGet();			/* Read SG command reg.	*/
			sysDcrDmasgcSet(DMA_SGC_EM1 | (regval & (~(DMA_SGC_SSG1))));
			regval = sysDcrDmacr1Get();			/* Read config reg.		*/
			sysDcrDmacr1Set(regval & ceMask);	/* Clear enable bits	*/
			break;
		case 2:
			regval = sysDcrDmasgcGet();			/* Read SG command reg.	*/
			sysDcrDmasgcSet(DMA_SGC_EM2 | (regval & (~(DMA_SGC_SSG2))));
			regval = sysDcrDmacr2Get();			/* Read config reg.		*/
			sysDcrDmacr2Set(regval & ceMask);	/* Clear enable bits	*/
			break;
		case 3:
			regval = sysDcrDmasgcGet();			/* Read SG command reg.	*/
			sysDcrDmasgcSet(DMA_SGC_EM3 | (regval & (~(DMA_SGC_SSG3))));
			regval = sysDcrDmacr3Get();			/* Read config reg.		*/
			sysDcrDmacr3Set(regval & ceMask);	/* Clear enable bits	*/
			break;
		default:
            /*  FIXME - Channel no. out of range - Put err. msg. here?  */
			break;
	}
}

/* 
 * -----------------------------------------------------------------------
 * dmaGetConfigReg
 * Get contents of DMA channel configuration register
 * (Also known as channel control register in some documentation)
 * 
 * Input:
 * 	chan - which channel (0-3) we are interested in.
 * Returns:
 *	Channel control word as a 32-bit unsigned int.
 * -----------------------------------------------------------------------
 */
UINT32 dmaGetConfigReg(int chan)
{
	switch(chan) {
		case 0:
			return (sysDcrDmacr0Get());
		case 1:
			return (sysDcrDmacr1Get());
		case 2:
			return (sysDcrDmacr2Get());
		case 3:
			return (sysDcrDmacr3Get());
		default:
            /*  FIXME - Channel no. out of range - Put err. msg. here?  */
			return 0;
	}
}

/* 
 * -----------------------------------------------------------------------
 * dmaSetConfigReg
 * Set contents of DMA channel configuration register
 * (Also referred to in in some documentation as channel control register)
 * 
 * Input:
 * 	chan  - which channel (0-3) we are interested in.
 *	value - the word to be stored in the register.
 * -----------------------------------------------------------------------
 */
void dmaSetConfigReg(int chan, UINT32 value)
{
	switch(chan) {
		case 0:
			sysDcrDmacr0Set(value);
			break;
		case 1:
			sysDcrDmacr1Set(value);
			break;
		case 2:
			sysDcrDmacr2Set(value);
			break;
		case 3:
			sysDcrDmacr3Set(value);
			break;
		default:
            /*  FIXME - Channel no. out of range - Put err. msg. here?  */
			break;
	}
}

/* 
 * -----------------------------------------------------------------------
 * dmaGetCountReg
 * Get contents of DMA channel count register
 * 
 * Input:
 * 	chan - which channel (0-3) we are interested in.
 * Returns:
 *	Contents of count register as a 32-bit unsigned int.
 * -----------------------------------------------------------------------
 */
UINT32 dmaGetCountReg(int chan)
{
	switch(chan) {
		case 0:
			return (sysDcrDmact0Get());
		case 1:
			return (sysDcrDmact1Get());
		case 2:
			return (sysDcrDmact2Get());
		case 3:
			return (sysDcrDmact3Get());
		default:
            /*  FIXME - Channel no. out of range - Put err. msg. here?  */
			return 0;
	}
}

/* 
 * -----------------------------------------------------------------------
 * dmaSetCountReg
 * Set contents of DMA channel count register
 * 
 * Input:
 * 	chan  - which channel (0-3) we are interested in.
 *	value - the word to be stored in the count register.
 * -----------------------------------------------------------------------
 */
void dmaSetCountReg(int chan, UINT32 value)
{
	UINT32 count;

	/* ------------ Count register uses lower 16 bits only ------------ */
	/* --------------- Zero actually means 0x10000, 64K --------------- */
	count = value & DMA_CNT_MASK;

	switch(chan) {
		case 0:
			sysDcrDmact0Set(count);
			break;
		case 1:
			sysDcrDmact1Set(count);
			break;
		case 2:
			sysDcrDmact2Set(count);
			break;
		case 3:
			sysDcrDmact3Set(count);
			break;
		default:
            /*  FIXME - Channel no. out of range - Put err. msg. here?  */
			break;
	}
}

/* 
 * -----------------------------------------------------------------------
 * dmaGetDestReg
 * Get contents of DMA channel destination address register
 * 
 * Input:
 * 	chan - which channel (0-3) we are interested in.
 * Returns:
 *	Current destination address as a 32-bit unsigned int.
 * -----------------------------------------------------------------------
 */
UINT32 dmaGetDestReg(int chan)
{
	switch(chan) {
		case 0:
			return (sysDcrDmada0Get());
		case 1:
			return (sysDcrDmada1Get());
		case 2:
			return (sysDcrDmada2Get());
		case 3:
			return (sysDcrDmada3Get());
		default:
            /*  FIXME - Channel no. out of range - Put err. msg. here?  */
			return 0;
	}
}

/* 
 * -----------------------------------------------------------------------
 * dmaSetDestReg
 * Set contents of DMA channel destination address register
 * 
 * Input:
 * 	chan  - which channel (0-3) we are interested in.
 *	value - the word to be stored as the destination address.
 * -----------------------------------------------------------------------
 */
void dmaSetDestReg(int chan, UINT32 value)
{
	switch(chan) {
		case 0:
			sysDcrDmada0Set(value);
			break;
		case 1:
			sysDcrDmada1Set(value);
			break;
		case 2:
			sysDcrDmada2Set(value);
			break;
		case 3:
			sysDcrDmada3Set(value);
			break;
		default:
            /*  FIXME - Channel no. out of range - Put err. msg. here?  */
			break;
	}
}

/* 
 * -----------------------------------------------------------------------
 * dmaGetSrcReg
 * Get contents of DMA source address register
 * 
 * Input:
 * 	chan - which channel (0-3) we are interested in.
 * Returns:
 *	Current source address as a 32-bit unsigned int.
 * -----------------------------------------------------------------------
 */
UINT32 dmaGetSrcReg(int chan)
{
	switch(chan) {
		case 0:
			return (sysDcrDmasa0Get());
		case 1:
			return (sysDcrDmasa1Get());
		case 2:
			return (sysDcrDmasa2Get());
		case 3:
			return (sysDcrDmasa3Get());
		default:
            /*  FIXME - Channel no. out of range - Put err. msg. here?  */
			return 0;
	}
}

/* 
 * -----------------------------------------------------------------------
 * dmaSetSrcReg
 * Set contents of DMA source address register
 * 
 * Input:
 * 	chan  - which channel (0-3) we are interested in.
 *	value - the word to be stored as the source address.
 * -----------------------------------------------------------------------
 */
void dmaSetSrcReg(int chan, UINT32 value)
{
	switch(chan) {
		case 0:
			sysDcrDmasa0Set(value);
			break;
		case 1:
			sysDcrDmasa1Set(value);
			break;
		case 2:
			sysDcrDmasa2Set(value);
			break;
		case 3:
			sysDcrDmasa3Set(value);
			break;
		default:
            /*  FIXME - Channel no. out of range - Put err. msg. here?  */
			break;
	}
}

/* 
 * -----------------------------------------------------------------------
 * dmaGetSGReg
 * Get contents of DMA channel scatter/gather descriptor address register
 * 
 * Input:
 * 	chan - which channel (0-3) we are interested in.
 * Returns:
 *	Current scatter/gather descriptor address as a 32-bit unsigned int.
 * Comments:
 *	This is the address of the current scatter/gather list.
 * -----------------------------------------------------------------------
 */
UINT32 dmaGetSGReg(int chan)
{
	switch(chan) {
		case 0:
			return (sysDcrDmasg0Get());
		case 1:
			return (sysDcrDmasg1Get());
		case 2:
			return (sysDcrDmasg2Get());
		case 3:
			return (sysDcrDmasg3Get());
		default:
            /*  FIXME - Channel no. out of range - Put err. msg. here?  */
			return 0;
	}
}

/* 
 * -----------------------------------------------------------------------
 * dmaSetSGReg
 * Set contents of DMA channel scatter/gather descriptor address register
 * 
 * Input:
 * 	chan  - which channel (0-3) we are interested in.
 *	value - the desired scatter/gather descriptor address.
 * Comments:
 *	This is the address of the current scatter/gather list.
 * -----------------------------------------------------------------------
 */
void dmaSetSGReg(int chan, UINT32 value)
{
	switch(chan) {
		case 0:
			sysDcrDmasg0Set(value);
			break;
		case 1:
			sysDcrDmasg1Set(value);
			break;
		case 2:
			sysDcrDmasg2Set(value);
			break;
		case 3:
			sysDcrDmasg3Set(value);
			break;
		default:
            /*  FIXME - Channel no. out of range - Put err. msg. here?  */
			break;
	}
}

/* 
 * -----------------------------------------------------------------------
 * dmaGetSGCommandReg
 * Get contents of DMA channel scatter/gather command register
 * 
 * Input:
 * 	chan - which channel (0-3) we are interested in.
 * Returns:
 *	Current channel scatter/gather command word as 32-bit unsigned int.
 * Comments:
 *  There is only one SG command word for all 4 channels.
 *	Two bits are defined in the command word for each channel:
 *		DMA_SGC_SSG[0-3] - Start Scatter/Gather
 *		DMA_SGC_EM[0-3]  - Scatter/Gather Enable Mask bit
 *	These two bits are not contiguous in the word. See dmaDcr.h
 *	Only the bits for the desired channel are returned, others masked out.
 * -----------------------------------------------------------------------
 */
UINT32 dmaGetSGCommandReg(int chan)
{
	UINT32 mask;
	switch(chan) {
		case 0:
			mask = (UINT32) (DMA_SGC_SSG0 | DMA_SGC_EM0);
			return ((sysDcrDmasgcGet()) & mask);
		case 1:
			mask = (UINT32) (DMA_SGC_SSG1 | DMA_SGC_EM1);
			return ((sysDcrDmasgcGet()) & mask);
		case 2:
			mask = (UINT32) (DMA_SGC_SSG2 | DMA_SGC_EM2);
			return ((sysDcrDmasgcGet()) & mask);
		case 3:
			mask = (UINT32) (DMA_SGC_SSG3 | DMA_SGC_EM3);
			return ((sysDcrDmasgcGet()) & mask);
		default:
            /*  FIXME - Channel no. out of range - Put err. msg. here?  */
			return 0;
	}
}

/* 
 * -----------------------------------------------------------------------
 * dmaEnableSGCommandReg
 * Set contents of DMA channel scatter/gather command register
 * This call actually starts up a scatter/gather DMA transfer by turning
 * on the enable bit for a given channel in the SG command word. This bit
 * will be cleared automatically at the end of the transfer.
 * 
 * Input:
 * 	chan  - which channel (0-3) we are interested in.
 *
 * Comments:
 *  1. There is only one SG command word for all 4 channels.
 *	2. Two bits are defined in the command word for each channel:
 *			DMA_SGC_SSG[0-3] - Start Scatter/Gather
 *			DMA_SGC_EM[0-3]  - Scatter/Gather Enable Mask bit
 *	   These two bits are not contiguous in the word. See dmaDcr.h
 *	   Both bits must be turned on to enable the channel.
 * -----------------------------------------------------------------------
 */
void dmaEnableSGCommandReg(int chan)
{
	UINT32 regval;

	regval = sysDcrDmasgcGet();			/* Get SG command word				*/
	switch(chan) {
		case 0:
			sysDcrDmasgcSet(regval | DMA_SGC_EM0 | DMA_SGC_SSG0);
			break;
		case 1:
			sysDcrDmasgcSet(regval | DMA_SGC_EM1 | DMA_SGC_SSG1);
			break;
		case 2:
			sysDcrDmasgcSet(regval | DMA_SGC_EM2 | DMA_SGC_SSG2);
			break;
		case 3:
			sysDcrDmasgcSet(regval | DMA_SGC_EM3 | DMA_SGC_SSG3);
			break;
		default:
            /*  FIXME - Channel no. out of range - Put err. msg. here?  */
			break;
	}
	/* --------- At this point, the DMA transfer is running! ---------- */
}

/* 
 * -----------------------------------------------------------------------
 * dmaDisableSGCommandReg
 * Clear contents of DMA channel scatter/gather command register.
 * This call stops a scatter/gather DMA transfer by clearing the
 * scatter/gather enable bits for the specified channel.
 * Note that it does not stop the transfer that is already in progress,
 * according to IBM documentation.
 * 
 * Input:
 * 	chan  - which channel (0-3) we are interested in.
 *
 * Comments:
 *  1. There is only one SG command word for all 4 channels.
 *	2. Two bits are defined in the command word for each channel:
 *			DMA_SGC_SSG[0-3] - Start Scatter/Gather
 *			DMA_SGC_EM[0-3]  - Scatter/Gather Enable Mask bit
 *	   These two bits are not contiguous in the word. See dmaDcr.h
 * -----------------------------------------------------------------------
 */
void dmaDisbleSGCommandReg(int chan)
{
	UINT32 regval;

	regval = sysDcrDmasgcGet();
	switch(chan) {
		case 0:
			sysDcrDmasgcSet(DMA_SGC_EM0 | (regval & (~(DMA_SGC_SSG0))));
			break;
		case 1:
			sysDcrDmasgcSet(DMA_SGC_EM1 | (regval & (~(DMA_SGC_SSG1))));
			break;
		case 2:
			sysDcrDmasgcSet(DMA_SGC_EM2 | (regval & (~(DMA_SGC_SSG2))));
			break;
		case 3:
			sysDcrDmasgcSet(DMA_SGC_EM3 | (regval & (~(DMA_SGC_SSG3))));
			break;
		default:
            /*  FIXME - Channel no. out of range - Put err. msg. here?  */
			break;
	}
}

/* 
 * -----------------------------------------------------------------------
 * dmaSetStatusReg
 * Set contents of DMA status register
 * 
 * Input:
 *	chan  - which channel (0-3) we are interested in.
 * 	value - status bits, already aligned to the right places in the word.
 *
 * Comments:
 *	1. There is only one status register covering all four channels.
 *	2. The status bits for each channel are not contiguous in the register.
 *	   See a more detailed description in dmaReg.h
 *	3. Before setting the bits for the given channel, we mask out all of the
 *	   bit positions for the other channels, to avoid disturbing them.
 * -----------------------------------------------------------------------
 */
void dmaSetStatusReg(int chan, UINT32 value)
{
	UINT32 regval, oldval, newval;

	if (value)						/* If not setting to 0...			*/
		dmaDisableChan(chan);		/* Make sure channel is disabled	*/
	regval = (sysDcrDmasrGet());
	switch(chan) {
		case 0:
			oldval = regval & DMA_SR_MASK_0;
			newval = regval | (value & DMA_SR_BITS_0);
			break;
		case 1:
			oldval = regval & DMA_SR_MASK_1;
			newval = regval | (value & DMA_SR_BITS_1);
			break;
		case 2:
			oldval = regval & DMA_SR_MASK_2;
			newval = regval | (value & DMA_SR_BITS_2);
			break;
		case 3:
			oldval = regval & DMA_SR_MASK_3;
			newval = regval | (value & DMA_SR_BITS_3);
			break;
		default:
            /*  FIXME - Channel no. out of range - Put err. msg. here?  */
			return;
	}

	sysDcrDmasrSet(newval);
}

/* 
 * -----------------------------------------------------------------------
 * dmaGetStatusReg
 * Get contents of DMA status register
 * 
 * Input:
 *	chan  - which channel (0-3) we are interested in.
 *
 * Comments:
 *	1. There is only one status register covering all four channels.
 *	2. The status bits for each channel are not contiguous in the register.
 *	   See a more detailed description in dmaReg.h
 *	3. Only the bits for the given channel are returned. Bits for the other
 *	   channels are masked to appear as zeros in their respective positions.
 * -----------------------------------------------------------------------
 */
UINT32 dmaGetStatusReg(int chan)
{
	switch(chan) {
		case 0:
			return (sysDcrDmasrGet() & DMA_SR_BITS_0);
		case 1:
			return (sysDcrDmasrGet() & DMA_SR_BITS_1);
		case 2:
			return (sysDcrDmasrGet() & DMA_SR_BITS_2);
		case 3:
			return (sysDcrDmasrGet() & DMA_SR_BITS_3);
		default:
            /*  FIXME - Channel no. out of range - Put err. msg. here?  */
			return 0;
	}
}

/* 
 * -----------------------------------------------------------------------
 * dmaGetDevicePacingStatus
 * Check to see whether DMA transfers are in a suspended state due to
 * being turned off by device pacing feature.
 * 
 * Input:
 *	chan  - which channel (0-3) we are interested in.
 *
 * Returns:
 *	nonzero - channel is suspended by device paced peripheral.
 *  zero	- channel is not suspended by device paced peripheral,
 *			  OR, channel is not running at all,
 *			  OR, channel is not in device paced mode.
 *
 * Comments:
 *	If transfer direction is toward peripheral, we check whether the
 *	external DMA Request bit is asserted; if from peripheral to 405, we
 *	check whether the internal DMA Request bit is asserted. If the
 *	appropriate bit is asserted, then we are NOT suspended by the device.
 * -----------------------------------------------------------------------
 */
int dmaGetDevicePacingStatus(int chan)
{
	UINT32 statusReg;
	UINT32 controlReg;

	statusReg  = dmaGetStatusReg(chan);
	controlReg = dmaGetConfigReg(chan);

	/* ----------- First check whether the channel is enabled ------------ */
	if (! (controlReg & DMA_CR_CE))
		return 0;				/* Chan. not ON */

#ifdef XXX    /* when pended on device paced the busy bit is False! */
*	/* ----------- First check whether the channel is busy ------------ */
*	if (! (statusReg & DMA_SR_CB))
*		return 0;				/* Chan. not running, so not suspended	*/
#endif
	/* -------- Check whether channel is in device paced mode --------- */
	if (! (controlReg & DMA_CR_TM_HWDP))
		return 0;				/* Chan. not in device paced mode		*/

        /* For a channel that stops at TC and the channel is not at TC but it's not busy, then it IS suspended */
        if ( (controlReg & DMA_CR_TCE) &&  /* stops on TC */
             ( !(statusReg & DMA_SR_CS) ) &&  /* Not at TC */
             ( (! (statusReg & DMA_SR_CB)) )  )
           return 1;
	else
           return 0;

#ifdef XXXX
*	if (controlReg & DMA_CR_TD)
*	{
*		/* - Txfr direction is to peripheral, check extern DmaReq bit - */
*		if (statusReg & DMA_SR_ER)
*			return 0;			/* Not suspended by device pacing		*/
*		else
*			return 1;			/* Suspended by device pacing			*/
*	}
*	else
*	{
*		/* - Txfr direction from peripheral, check intern request bit - */
*		if (statusReg & DMA_SR_IR)
*			return 0;			/* Not suspended by device pacing		*/
*		else
*			return 1;			/* Suspended by device pacing			*/
*	}
#endif
}

/* 
 * -----------------------------------------------------------------------
 * dmaGetDeviceActiveStatus
 * Check to see whether DMA transfers are in a suspended state due to
 * being turned off by device pacing feature.
 * 
 * Input:
 *	chan  - which channel (0-3) we are interested in.
 *
 * Returns:
 *	nonzero - channel is active (running) in a dma transfer.
 *  zero	- channel is not active 
 *
 * Comments:
 *	Check the active bit for the DMA channel 
 *
 *   Author:  Greg Brissey  6/04/2004
 * -----------------------------------------------------------------------
 */
int dmaGetDeviceActiveStatus(int chan)
{
   UINT32 controlReg;

   controlReg = dmaGetConfigReg(chan);

   /* ----------- check whether the channel is enabled ------------ */
   if (! (controlReg & DMA_CR_CE))
      return 0;                               /* Chan. not ON */
   else
      return 1;			/* Chan is enabled and running, but may not be 'busy'
				   in the case of device paced and suspended   */
}

/* 
 * -----------------------------------------------------------------------
 * dmaGetSleepReg
 * Get contents of DMA channel sleep register
 * 
 * Input:
 * 	None.
 * Returns:
 *	Current DMA channel sleep mode register as 32-bit unsigned int.
 *
 * Comments:
 *	1. There is only one sleep mode register for all channels.
 *	2. This call is only provided for consistency with other DMA register
 *	   routines; sysDcrDmaslpGet() would do just as well.
 *  3. Bits 0..9 as returned are the timeout value of the idle timer.
 *     Only the upper 5 bits are settable; the lower 5 are always ones.
 *  4. Bit 10 as returned is the sleep enable bit.
 *  5. Bits 11..31 are not used.
 * -----------------------------------------------------------------------
 */
UINT32 dmaGetSleepReg(void)
{
	return (sysDcrDmaslpGet());
}

/* 
 * -----------------------------------------------------------------------
 * dmaSetSleepReg
 * Set contents of DMA sleep register
 * 
 * Input:
 * 	value - desired value of sleep register.
 *
 * Comments:
 *	1. There is only one sleep mode register for all channels.
 *  2. To enable sleep mode, set the sleep mode enable bit (bit 10) and
 *     also set the upper 5 bits (bits 0..4) to the desired timeout value
 *     for the idle timer. When the DMA controller becomes idle, the
 *     idle timer begins counting down at the PLB clock rate. When it
 *     reaches 0, the DMA controller will enter sleep mode.
 *  3. To enable sleep mode, the power management enable bit (PME bit) in
 *     the clock and power management register (CPM0_ER[PME] in IBM's
 *     notation) must be set as well. We don't do that here.
 * -----------------------------------------------------------------------
 */
void dmaSetSleepReg(UINT32 value)
{
	sysDcrDmaslpSet(value);
}

/* 
 * -----------------------------------------------------------------------
 * dmaGetPolarityReg
 * Get contents of DMA polarity configuration register
 * 
 * Input:
 * 	chan - which channel (0-3) we are interested in.
 * Returns:
 *	Polarity control information from DMA polarity configuration Register,
 *  as a 32-bit unsigned integer.
 * Comments:
 *  1. There are 3 polarity configuration bits for each channel. For each
 *     bit, polarity is active high if the bit is zero, and active low if
 *     the bit is 1.
 *		a. DMA Req line polarity
 *		b. DMA Ack line polarity
 *		c. EOTn[TCn] polarity
 *  2. See dmaDcr.h for bit definitions.
 *	3. Only the bits for the given channel are returned. Bits for the other
 *	   channels are masked to appear as zeros in their respective positions.
 * -----------------------------------------------------------------------
 */
UINT32 dmaGetPolarityReg(int chan)
{
	UINT32 mask;
	switch(chan) {
		case 0:
			mask = (UINT32) (DMA_POL_R0P | DMA_POL_A0P | DMA_POL_E0P);
			return ((sysDcrDmapolGet()) & mask);
		case 1:
			mask = (UINT32) (DMA_POL_R1P | DMA_POL_A1P | DMA_POL_E1P);
			return ((sysDcrDmapolGet()) & mask);
		case 2:
			mask = (UINT32) (DMA_POL_R2P | DMA_POL_A2P | DMA_POL_E2P);
			return ((sysDcrDmapolGet()) & mask);
		case 3:
			mask = (UINT32) (DMA_POL_R3P | DMA_POL_A3P | DMA_POL_E3P);
			return ((sysDcrDmapolGet()) & mask);
		default:
            /*  FIXME - Channel no. out of range - Put err. msg. here?  */
			return 0;
	}
}

/* 
 * -----------------------------------------------------------------------
 * dmaSetPolarityReg
 * Set contents of DMA polarity configuration register
 * 
 * Input:
 * 	chan  - which channel (0-3) we are interested in.
 *	value - the word to be stored in the register.
 * Comments:
 *  1. There are 3 polarity configuration bits for each channel. For each
 *     bit, polarity is active high if the bit is zero, and active low if
 *     the bit is 1.
 *		a. DMA Req line polarity
 *		b. DMA Ack line polarity
 *		c. EOTn[TCn] polarity
 *  2. See dmaDcr.h for bit definitions.
 *	3. Before setting the bits for the given channel, we mask out all of the
 *	   bit positions for the other channels, to avoid disturbing them.
 *  4. IBM advises clearing the EOT status bit for the channel before
 *     enabling the channel after changing the polarities, to prevent a
 *     channel from being disabled due to an incorrect EOT status stored in
 *     the status bits; so we take care of that here as well.
 *  5. This only needs to be done once during initial setup.
 * -----------------------------------------------------------------------
 */
void dmaSetPolarityReg(int chan, UINT32 value)
{
	UINT32 regval, oldval, newval, mask, ts_mask, status;

	/* --------------- Set masks for indicated channel ---------------- */
	switch (chan) {
		case 0:
			mask = (UINT32) (DMA_POL_R0P | DMA_POL_A0P | DMA_POL_E0P);
			ts_mask = (UINT32) (DMA_SR_TS_0);
			break;
		case 1:
			mask = (UINT32) (DMA_POL_R1P | DMA_POL_A1P | DMA_POL_E1P);
			ts_mask = (UINT32) (DMA_SR_TS_1);
			break;
		case 2:
			mask = (UINT32) (DMA_POL_R2P | DMA_POL_A2P | DMA_POL_E2P);
			ts_mask = (UINT32) (DMA_SR_TS_2);
			break;
		case 3:
			mask = (UINT32) (DMA_POL_R3P | DMA_POL_A3P | DMA_POL_E3P);
			ts_mask = (UINT32) (DMA_SR_TS_3);
			break;
		default:
            /*  FIXME - Channel no. out of range - Put err. msg. here?  */
			return;
	}

	/* ------------------- Set new polarity values -------------------- */
	regval = sysDcrDmapolGet();			/* Read the polarity register	*/
	oldval = regval & (~mask);			/* Mask out old values			*/
	newval = value & mask;				/* Isolate new values			*/
	sysDcrDmapolSet(oldval | newval);	/* Write the register			*/

	/* -------------- Clear the channel's EOT status bit -------------- */
	status = sysDcrDmasrGet();			/* Read the status register		*/
	status &= ~ts_mask;					/* Clear EOT status bit			*/
	sysDcrDmasrSet(status);				/* Rewrite the register			*/
}
