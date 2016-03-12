/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
Id: dmaDebug.c,v 1.6 2004/04/12 20:45:23 rthrift Exp rthrift
=========================================================================
FILE: dmaDebug.c
=========================================================================
PURPOSE:
	Diagnostic routines for printing contents of the various DMA
	registers.
	- DMA Channel 0 Control Register
	- DMA Channel 1 Control Register
	- DMA Channel 2 Control Register
	- DMA Channel 3 Control Register
	- DMA Channel 0 Count Register
	- DMA Channel 1 Count Register
	- DMA Channel 2 Count Register
	- DMA Channel 3 Count Register
	- DMA Channel 0 Destination Address Register
	- DMA Channel 1 Destination Address Register
	- DMA Channel 2 Destination Address Register
	- DMA Channel 3 Destination Address Register
	- DMA Channel 0 Source Address Register
	- DMA Channel 1 Source Address Register
	- DMA Channel 2 Source Address Register
	- DMA Channel 3 Source Address Register
	- DMA Channel 0 Scatter/Gather Descriptor Address Register
	- DMA Channel 1 Scatter/Gather Descriptor Address Register
	- DMA Channel 2 Scatter/Gather Descriptor Address Register
	- DMA Channel 3 Scatter/Gather Descriptor Address Register
	- DMA Status Register (all channels)
	- DMA Scatter/Gather Command Register (all channels)
	- DMA Sleep Mode Register (all channels)
	- DMA Polarity Configuration Register (all channels)
	And Universal Interrupt Controller registers:
	- UIC Status Register
	- UIC Enable Register
	- UIC Critical Register
	- UIC Polarity Register
	- UIC Masked Status Register
	And a routine for printing Scatter/Gather list contents:
	- dmaPrintSGList()

COMMENTS:
	Messages are output through the DMA error message queue.

	UIC Vector Register and UIC Vector Configuration Register
	are not reported, as they do not apply to non-critical
	interrupts, and DMA interrupts are intended to be non-critical.

	Standard ANSI C/C++ compilation is assumed.

AUTHOR:
	Robert L. Thrift (2003)
=========================================================================
     Copyright (c) 2003, Varian Associates, Inc. All Rights Reserved.
     This software contains proprietary and confidential information
            of Varian Associates, Inc. and its contributors.
  Use, disclosure and reproduction is prohibited without prior consent.
=========================================================================
*/
#include <stdio.h>
#include <string.h>
#include "dmaDcr.h"
#include "uicDcr.h"
#include "dmaReg.h"
#include "dmaDrv.h"
#include "dmaMsgQueue.h"
#include "dmaDebugOptions.h"

/* Forward prototype declaration */
void dmaPrintSGList(int);

/* Message buffers for error reporting */
static char msgStr[MAX_ERR_MSG_LEN];
static char msgStr2[MAX_ERR_MSG_LEN];

/*
=========================================================================
NAME: dmaPrintStatusReg
=========================================================================
PURPOSE:
	Print out the status register contents for one specific DMA channel.

ARGUMENTS:
	chan - which channel's info to print out.

RETURN VALUE:
	None.

COMMENTS:
	The status register contains status bits for all four DMA channels,
	so we have to pick out the relevant bits for an individual channel.

AUTHOR: 
	Robert L. Thrift, 2003
=========================================================================
*/

void dmaPrintStatusReg(int chan)
{
	UINT32	statusReg;		/* Copy of DMA status register				*/

	/* ------------ Individual bits in DMA Status Register ------------ */
	UINT32	statusCS,		/* Terminal count bit						*/
			statusTS,		/* End-of-transfer bit						*/
			statusRI,		/* Channel error bit						*/
			statusIR,		/* Internal request bit						*/
			statusER,		/* External request bit						*/
			statusCB,		/* Channel busy bit							*/
			statusSG;		/* Scatter/gather bit						*/

	statusReg	= dmaGetStatusReg(chan);

	/* ----------- Print DMA Status Register condition bits ----------- */
	sprintf(msgStr, "\nDMA Status Register for channel %d:", chan);
	dmaLogMsg(msgStr);
	switch(chan) {
		case 0:					/* Set up mask bits for channel 0		*/
			statusCS = DMA_SR_CS_0;
			statusTS = DMA_SR_TS_0;
			statusRI = DMA_SR_RI_0;
			statusIR = DMA_SR_IR_0;
			statusER = DMA_SR_ER_0;
			statusCB = DMA_SR_CB_0;
			statusSG = DMA_SR_SG_0;
			break;
		case 1:					/* Set up mask bits for channel 1		*/
			statusCS = DMA_SR_CS_1;
			statusTS = DMA_SR_TS_1;
			statusRI = DMA_SR_RI_1;
			statusIR = DMA_SR_IR_1;
			statusER = DMA_SR_ER_1;
			statusCB = DMA_SR_CB_1;
			statusSG = DMA_SR_SG_1;
			break;
		case 2:					/* Set up mask bits for channel 2		*/
			statusCS = DMA_SR_CS_2;
			statusTS = DMA_SR_TS_2;
			statusRI = DMA_SR_RI_2;
			statusIR = DMA_SR_IR_2;
			statusER = DMA_SR_ER_2;
			statusCB = DMA_SR_CB_2;
			statusSG = DMA_SR_SG_2;
			break;
		case 3:					/* Set up mask bits for channel 3		*/
			statusCS = DMA_SR_CS_3;
			statusTS = DMA_SR_TS_3;
			statusRI = DMA_SR_RI_3;
			statusIR = DMA_SR_IR_3;
			statusER = DMA_SR_ER_3;
			statusCB = DMA_SR_CB_3;
			statusSG = DMA_SR_SG_3;
			break;
	}
	sprintf(msgStr, "\tTerminal count status:         channel is ");
	if (! (statusReg & statusCS))
		strcat(msgStr, "not ");
	strcat(msgStr, "at TC");
	dmaLogMsg(msgStr);

	sprintf(msgStr, "\tEnd of transfer status:        channel is ");
	if (! (statusReg & statusTS))
		strcat(msgStr, "not ");
	strcat(msgStr, "at EOT.");
	dmaLogMsg(msgStr);

	sprintf(msgStr, "\tChannel error status:          error bit ");
	if (! (statusReg & statusRI))
		strcat(msgStr, "not ");
	strcat(msgStr, "set.");
	dmaLogMsg(msgStr);

	sprintf(msgStr, "\tInternal DMA request:          ");
	if (! (statusReg & statusIR))
		strcat(msgStr, "not ");
	strcat(msgStr, "pending.");
	dmaLogMsg(msgStr);

	sprintf(msgStr, "\tExternal DMA request:          ");
	if (! (statusReg & statusER))
		strcat(msgStr, "not ");
	strcat(msgStr, "pending.");
	dmaLogMsg(msgStr);

	sprintf(msgStr, "\tChannel busy status:           ");
	if (! (statusReg & statusCB))
		strcat(msgStr, "not ");
	strcat(msgStr, "busy.");
	dmaLogMsg(msgStr);

	sprintf(msgStr, "\tScatter/gather status:         ");
	if (! (statusReg & statusSG))
		strcat(msgStr, "not ");
	strcat(msgStr, "enabled.");
	dmaLogMsg(msgStr);

	/* If SG bit enabled, print out SG list */
	if (statusReg & statusSG)
		dmaPrintSGList(chan);
}

/*
=========================================================================
NAME: dmaPrintControlReg
=========================================================================
PURPOSE:
	Print out the control register contents for one specific DMA channel.

ARGUMENTS:
	chan - which channel's info to print out.

RETURN VALUE:
	None.

COMMENTS:
	Each channel has its own 32-bit control register.

AUTHOR: 
	Robert L. Thrift, 2003
=========================================================================
*/
void dmaPrintControlReg(int chan)
{
	UINT32	controlReg,
			transferMode,
			transferWidth,
			channelPriority;

	controlReg    = dmaGetConfigReg(chan);
	transferMode  = (controlReg & DMA_CR_TM) >> 21;
	transferWidth = (controlReg & DMA_CR_PW) >> 26;

	sprintf(msgStr, "\nDMA Channel Control Register for channel %d:", chan);
	dmaLogMsg(msgStr);

	sprintf(msgStr, "\tChannel enable:                ");
	if (controlReg & DMA_CR_CE)
		strcat(msgStr, "ON");
	else
		strcat(msgStr, "OFF");
	dmaLogMsg(msgStr);

	sprintf(msgStr, "\tChannel interrupt enable:      ");
	if (controlReg & DMA_CR_CIE)
		strcat(msgStr, "ON");
	else
		strcat(msgStr, "OFF");
	dmaLogMsg(msgStr);

	sprintf(msgStr, "\tTransfer direction:            ");
	switch(transferMode) {		/* interpretation of TD depends on TM	*/
		case 0:
			if (controlReg & DMA_CR_TD)
				strcat(msgStr, "peripheral to memory");	/* peripheral mode	*/
			else
				strcat(msgStr, "memory to peripheral");
			break;
		case 1:
				strcat(msgStr, "invalid setting: 01");	/* undefined bits	*/
			break;
		case 2:
				strcat(msgStr, "not used");			/* software initiated	*/
			break;								/* memory to memory		*/
		case 3:									/* device paced			*/
			if (controlReg & DMA_CR_TD)			/* memory to memory		*/
				strcat(msgStr, "peripheral is at src. addr.");
			else
				strcat(msgStr, "peripheral is at dest. addr.");
			break;
	}
	dmaLogMsg(msgStr);

	sprintf(msgStr, "\tPeripheral location:           ");
	if (controlReg & DMA_CR_PL)
		strcat(msgStr, "OPB (UART0).");
	else
		strcat(msgStr, "EBC bus.");
	dmaLogMsg(msgStr);

	sprintf(msgStr, "\tPeripheral transfer width:     ");
	switch(transferWidth) {
		case 0:
			strcat(msgStr, "byte (8 bits)");
			break;
		case 1:
			strcat(msgStr, "half-word (16 bits)");
			break;
		case 2:
			strcat(msgStr, "word (32 bits)");
			break;
		case 3:
			strcat(msgStr, "double-word (64 bits)");
			break;
	}
	dmaLogMsg(msgStr);

	sprintf(msgStr, "\tDestination address increment: ");
	if (controlReg & DMA_CR_DAI)
		strcat(msgStr, "ON");
	else
		strcat(msgStr, "OFF");
	dmaLogMsg(msgStr);

	sprintf(msgStr, "\tSource address increment:      ");
	if (controlReg & DMA_CR_SAI)
		strcat(msgStr, "ON");
	else
		strcat(msgStr, "OFF");
	dmaLogMsg(msgStr);

	sprintf(msgStr, "\tBuffer enable:                 ");
	if (controlReg & DMA_CR_BEN)
		strcat(msgStr, "ON");
	else
		strcat(msgStr, "OFF");
	dmaLogMsg(msgStr);

	sprintf(msgStr, "\tTransfer mode:                 ");
	switch(transferMode) {
		case 0:
			strcat(msgStr, "peripheral");
			break;
		case 1:
			strcat(msgStr, "invalid setting: 01");
			break;
		case 2:
			strcat(msgStr, "software initiated mem. to mem.");
			break;
		case 3:
			strcat(msgStr, "device paced mem. to mem.");
			break;
	}
	dmaLogMsg(msgStr);

	sprintf(msgStr, "\tPeripheral setup cycles:       ");
	if (transferMode == 0)
		sprintf(msgStr2, "%d", ((controlReg & DMA_CR_PSC) >> 19));
	else
		sprintf(msgStr2, "N/A (peripheral transfers only).");
	strcat(msgStr, msgStr2);
	dmaLogMsg(msgStr);

	sprintf(msgStr, "\tPeripheral wait cycles:        ");
	if (transferMode == 0)
		sprintf(msgStr2, "%d", ((controlReg & DMA_CR_PWC) >> 13));
	else
		sprintf(msgStr2, "N/A (peripheral transfers only).");
	strcat(msgStr, msgStr2);
	dmaLogMsg(msgStr);

	sprintf(msgStr, "\tPeripheral hold cycles:        ");
	if (transferMode == 0)
		sprintf(msgStr2, "%d", ((controlReg & DMA_CR_PHC) >> 10));
	else
		sprintf(msgStr2, "N/A (peripheral transfers only).");
	strcat(msgStr, msgStr2);
	dmaLogMsg(msgStr);

	sprintf(msgStr, "\tEOT/TC pin direction:          ");
	if (controlReg & DMA_CR_ETD)
		strcat(msgStr, "EOT is an output.");
	else
	{
		strcat(msgStr, "EOT is an input.");
		if (transferMode == 2)
		{
			dmaLogMsg(msgStr);
			dmaLogMsg("\t*****ERROR*** wrong setting for softw. init. mem. to mem.");
		}
		else
			dmaLogMsg(msgStr);
	}

	sprintf(msgStr, "\tTerminal count enable bit:     ");
	if (controlReg & DMA_CR_TCE)
	{
		strcat(msgStr, "ON, stops at TC.");
		if (! (controlReg & DMA_CR_ETD))
		{
			dmaLogMsg(msgStr);
			dmaLogMsg("\t*****ERROR***** incompatible with EOT/TC pin direction.");
		}
		else
			dmaLogMsg(msgStr);
	}
	else
	{
		strcat(msgStr, "OFF, doesn't stop at TC.");
		dmaLogMsg(msgStr);
	}

	sprintf(msgStr, "\tChannel priority bits:         ");
	channelPriority = (controlReg & DMA_CR_CP) >> 6;
	switch(channelPriority) {
		case 0:
			strcat(msgStr, "00 - low.");
			break;
		case 1:
			strcat(msgStr, "01 - med. low.");
			break;
		case 2:
			strcat(msgStr, "10 - med. high.");
			break;
		case 3:
			strcat(msgStr, "11 - high.");
			break;
	}
	dmaLogMsg(msgStr);

	sprintf(msgStr, "\tMemory read prefetch:          ");
	if ((((transferMode == 0) && ((controlReg & DMA_CR_TD) == 0))
		|| (transferMode == 3)) && (controlReg & DMA_CR_BEN))
	{
		switch((controlReg & DMA_CR_PF) >> 4) {
			case 0:
				strcat(msgStr, "prefetch 1 doubleword");
				break;
			case 1:
				strcat(msgStr, "prefetch 2 doublewords");
				break;
			case 2:
				strcat(msgStr, "prefetch 4 doublewords");
				break;
			case 3:
				strcat(msgStr, "invalid prefetch value.");
				break;
		}
	}
	else
		strcat(msgStr, "N/A for this transfer mode.");
	dmaLogMsg(msgStr);

	sprintf(msgStr, "\tParity check enable:           ");
	if (controlReg & DMA_CR_PCE)
		strcat(msgStr, "enabled.");
	else
		strcat(msgStr, "disabled.");
	dmaLogMsg(msgStr);

	sprintf(msgStr, "\tAddress decrement:             ");
	if (transferMode != 0)
		strcat(msgStr, "N/A");
	else
	{
		if (controlReg & DMA_CR_DEC)
		{
			strcat(msgStr, "addr. decremented after each transfer.");
			if (controlReg & DMA_CR_BEN)
			{
				dmaLogMsg(msgStr);
				dmaLogMsg("\t*****ERROR***** Buffer Enable must be OFF!");
			}
		}
		else
		{
			strcat(msgStr, "Not used - controlled by SAI, DAI bits.");
		}
	}
	dmaLogMsg(msgStr);

	sprintf(msgStr, "Contents of entire DMA Control Register = 0x%08x",
					controlReg);
	dmaLogMsg(msgStr);
}

/*
=========================================================================
NAME: dmaPrintCountReg
=========================================================================
PURPOSE:
	Print out the count register contents for one specific DMA channel.

ARGUMENTS:
	chan - which channel's info to print out.

RETURN VALUE:
	None.

COMMENTS:
	Each channel has its own 32-bit count register.

AUTHOR: 
	Robert L. Thrift, 2003
=========================================================================
*/
void dmaPrintCountReg(int chan)
{
	UINT32 countReg;

	countReg = dmaGetCountReg(chan);
	sprintf(msgStr, "\nDMA Channel Count Register for channel %d:", chan);
	dmaLogMsg(msgStr);
	sprintf(msgStr, "\tCount is:                      0x%08x (%d)",
				countReg, countReg);
	dmaLogMsg(msgStr);
}


/*
=========================================================================
NAME: dmaPrintDestReg
=========================================================================
PURPOSE:
	Print out the destination address register contents for one
	specific DMA channel.

ARGUMENTS:
	chan - which channel's info to print out.

RETURN VALUE:
	None.

COMMENTS:
	Each channel has its own 32-bit destination address register.

AUTHOR: 
	Robert L. Thrift, 2003
=========================================================================
*/
void dmaPrintDestReg(int chan)
{
	UINT32 destReg;

	destReg = dmaGetDestReg(chan);
	sprintf(msgStr, "\nDMA Channel Destination Address Register for channel %d:", chan);
	dmaLogMsg(msgStr);
	sprintf(msgStr, "\tDestination address is:        0x%08x", destReg);
	dmaLogMsg(msgStr);
}

/*
=========================================================================
NAME: dmaPrintSrcReg
=========================================================================
PURPOSE:
	Print out the source address register contents for one
	specific DMA channel.

ARGUMENTS:
	chan - which channel's info to print out.

RETURN VALUE:
	None.

COMMENTS:
	Each channel has its own 32-bit source address register.

AUTHOR: 
	Robert L. Thrift, 2003
=========================================================================
*/
void dmaPrintSrcReg(int chan)
{
	UINT32 srcReg;

	srcReg = dmaGetSrcReg(chan);
	sprintf(msgStr, "\nDMA Channel Source Address Register for channel %d:",
					chan);
	dmaLogMsg(msgStr);
	sprintf(msgStr, "\tSource address is:             0x%08x", srcReg);
	dmaLogMsg(msgStr);
}


/*
=========================================================================
NAME: dmaPrintSGReg
=========================================================================
PURPOSE:
	Print out the scatter/gather descriptor address register contents
	for one specific DMA channel.

ARGUMENTS:
	chan - which channel's SG address register to print out.

RETURN VALUE:
	None.

COMMENTS:
	Each channel has its own 32-bit scatter/gather descriptor address
	register.

AUTHOR: 
	Robert L. Thrift, 2003
=========================================================================
*/
void dmaPrintSGReg(int chan)
{
	UINT32 SGReg;

	SGReg = dmaGetSGReg(chan);
	sprintf(msgStr, "\nScatter/gather Descriptor Address Register for channel %d:", chan);
	dmaLogMsg(msgStr);
	sprintf(msgStr, "\tS/G descriptor address is:     0x%08x", SGReg);
	dmaLogMsg(msgStr);
}

/*
=========================================================================
NAME: dmaPrintSGCommandReg
=========================================================================
PURPOSE:
	Print out the scatter/gather command register contents for one
	specific DMA channel.

ARGUMENTS:
	chan - which channel's SG command register to print out.

RETURN VALUE:
	None.

COMMENTS:
	The register contains command bits for all four DMA channels,
	so we have to pick out the relevant bits for an individual channel.

AUTHOR: 
	Robert L. Thrift, 2003
=========================================================================
*/
void dmaPrintSGCommandReg(int chan)
{
	UINT32 sgCommandReg;

	sgCommandReg = dmaGetSGCommandReg(chan);
	sprintf(msgStr, "\nScatter/gather Command Register for channel %d:", chan);
	dmaLogMsg(msgStr);
	sprintf(msgStr, "\tEnable mask bit for ch. %d      ", chan);
	sprintf(msgStr2, "\tSG enable bit for ch. %d        ", chan);
	switch(chan) {
		case 0:
			if (sgCommandReg & DMA_SGC_EM0)
				strcat(msgStr, "enabled.");
			else
				strcat(msgStr, "disabled.");
			if (sgCommandReg & DMA_SGC_SSG0)
				strcat(msgStr2, "enabled.");
			else
				strcat(msgStr2, "disabled.");
			break;
		case 1:
			if (sgCommandReg & DMA_SGC_EM1)
				strcat(msgStr, "enabled.");
			else
				strcat(msgStr, "disabled.");
			if (sgCommandReg & DMA_SGC_SSG1)
				strcat(msgStr2, "enabled.");
			else
				strcat(msgStr2, "disabled.");
			break;
		case 2:
			if (sgCommandReg & DMA_SGC_EM2)
				strcat(msgStr, "enabled.");
			else
				strcat(msgStr, "disabled.");
			if (sgCommandReg & DMA_SGC_SSG2)
				strcat(msgStr2, "enabled.");
			else
				strcat(msgStr2, "disabled.");
			break;
		case 3:
			if (sgCommandReg & DMA_SGC_EM3)
				strcat(msgStr, "enabled.");
			else
				strcat(msgStr, "disabled.");
			if (sgCommandReg & DMA_SGC_SSG3)
				strcat(msgStr2, "enabled.");
			else
				strcat(msgStr2, "disabled.");
			break;
	}
	dmaLogMsg(msgStr);
	dmaLogMsg(msgStr2);
}


/*
=========================================================================
NAME: dmaPrintSleepModeReg
=========================================================================
PURPOSE:
	Print out the sleep mode register contents.

ARGUMENTS:
	chan - This argument is only for format compatibility with other
		   calls, and is ignored.

RETURN VALUE:
	None.

COMMENTS:
	There is only one sleep mode register. It applies to all channels.

AUTHOR: 
	Robert L. Thrift, 2003
=========================================================================
*/
void dmaPrintSleepModeReg(int chan)
{
	UINT32 sleepTimer,
		   sleepEnable;

	sleepTimer  = dmaGetSleepReg();
	sleepEnable = sleepTimer >> 21;
	sleepTimer  = sleepTimer >> 22;
	dmaLogMsg("\nDMA Sleep Mode Register:");
	sprintf(msgStr, "\tSleep mode:                    ");
	if (sleepEnable & 1)
	{
		sprintf(msgStr2, "enabled; timer count = 0x%08x (%d).",
				sleepTimer, sleepTimer);
		strcat(msgStr, msgStr2);
	}
	else
		strcat(msgStr, "disabled.");
	dmaLogMsg(msgStr);
}


/*
=========================================================================
NAME: dmaPrintPolarityConfigReg
=========================================================================
PURPOSE:
	Print out the DMA polarity configuration register contents.

ARGUMENTS:
	chan - which channel's polarity config register to print out.

RETURN VALUE:
	None.

COMMENTS:
	There is only one polarity configuration register.
	It has different bits internally for the different channels.

AUTHOR: 
	Robert L. Thrift, 2003
=========================================================================
*/
void dmaPrintPolarityConfigReg(int chan)
{
	UINT32	polarityReg,
			polarityBits;

	polarityReg = dmaGetPolarityReg(chan);
	sprintf(msgStr, "\nDMA Polarity Configuration for channel %d:", chan);
	dmaLogMsg(msgStr);
	switch(chan) {
		case 0:
			polarityBits = (polarityReg >> 29) & 7;
			break;
		case 1:
			polarityBits = (polarityReg >> 26) & 7;
			break;
		case 2:
			polarityBits = (polarityReg >> 23) & 7;
			break;
		case 3:
			polarityBits = (polarityReg >> 20) & 7;
			break;
	}

	if (polarityBits & 4)
		sprintf(msgStr, "\tDMA Req polarity:              active low");
	else
		sprintf(msgStr, "\tDMA Req polarity:              active high");
	dmaLogMsg(msgStr);

	if (polarityBits & 2)
		sprintf(msgStr, "\tDMA Ack polarity:              active low");
	else
		sprintf(msgStr, "\tDMA Ack polarity:              active high");
	dmaLogMsg(msgStr);

	if (polarityBits & 1)
		sprintf(msgStr, "\tEOT/TC polarity:               active low");
	else
		sprintf(msgStr, "\tEOT/TC polarity:               active high");
	dmaLogMsg(msgStr);
}

/*
=========================================================================
NAME: dmaPrintUICStatusReg
=========================================================================
PURPOSE:
	Print out the DMA related bits in the UIC status register.

ARGUMENTS:
	chan - which channel's info to print out.

RETURN VALUE:
	None.

COMMENTS:

AUTHOR: 
	Robert L. Thrift, 2003
=========================================================================
*/
void dmaPrintUICStatusReg(int chan)
{
	UINT32 uicStatusReg, ibit, nbit;

	uicStatusReg = sysDcrUicsrGet();
	nbit = DMA_CHAN_INT_BASE + chan;
	ibit = BITSET(nbit);

	sprintf(msgStr, "\nUIC Status Register for channel %d:     Interrupt bit (%d) is ",
				chan, nbit);
	if (uicStatusReg & ibit)
		strcat(msgStr, " 1 (set).");
	else
		strcat(msgStr, " 0 (cleared).");
	dmaLogMsg(msgStr);
	sprintf(msgStr, "Contents of entire UIC Status Register = 0x%08x",
					uicStatusReg);
	dmaLogMsg(msgStr);

}

/*
=========================================================================
NAME: dmaPrintUICEnableReg
=========================================================================
PURPOSE:
	Print out the DMA related bits in the UIC enable register.

ARGUMENTS:
	chan - which channel's info to print out.

RETURN VALUE:
	None.

COMMENTS:

AUTHOR: 
	Robert L. Thrift, 2003
=========================================================================
*/
void dmaPrintUICEnableReg(int chan)
{
	UINT32 uicEnableReg, ibit, nbit;

	uicEnableReg = sysDcrUicerGet();
	nbit = DMA_CHAN_INT_BASE + chan;
	ibit = BITSET(nbit);

	sprintf(msgStr, "\nUIC Enable Register for channel %d:     Enable bit (%d) is ",
			chan, nbit);
	if (uicEnableReg & ibit)
		strcat(msgStr, "1 (enabled).");
	else
		strcat(msgStr, "0 (disabled).");
	dmaLogMsg(msgStr);
	sprintf(msgStr, "Contents of entire UIC Enable Register = 0x%08x",
					uicEnableReg);
	dmaLogMsg(msgStr);
}

/*
=========================================================================
NAME: dmaPrintUICCriticalReg
=========================================================================
PURPOSE:
	Print out the DMA related bits in the UIC critical register.

ARGUMENTS:
	chan - which channel's info to print out.

RETURN VALUE:
	None.

COMMENTS:

AUTHOR: 
	Robert L. Thrift, 2003
=========================================================================
*/
void dmaPrintUICCriticalReg(int chan)
{
	UINT32 uicCriticalReg, ibit;

	uicCriticalReg = sysDcrUiccrGet();
	ibit = BITSET((DMA_CHAN_INT_BASE + chan));

	sprintf(msgStr, "\nUIC Critical Register for channel %d:   Interrupts are ",
			  chan);
	if (! (uicCriticalReg & ibit))
		strcat(msgStr, "non-");
	strcat(msgStr, "critical.");
	dmaLogMsg(msgStr);
	if ((uicCriticalReg & ibit))
	{
		sprintf(msgStr, "\t*****ERROR*****: channel %d interrupts should be noncritical.",
				chan);
		dmaLogMsg(msgStr);
	}
	sprintf(msgStr, "Contents of entire UIC Critical Register = 0x%08x",
				uicCriticalReg);
	dmaLogMsg(msgStr);
}

/*
=========================================================================
NAME: dmaPrintUICPolarityReg
=========================================================================
PURPOSE:
	Print out the DMA related bits in the UIC polarity register.

ARGUMENTS:
	chan - which channel's info to print out.

RETURN VALUE:
	None.

COMMENTS:

AUTHOR: 
	Robert L. Thrift, 2003
=========================================================================
*/
void dmaPrintUICPolarityReg(int chan)
{
	UINT32 uicPolarityReg, ibit, nbit;

	uicPolarityReg = sysDcrUicprGet();
	nbit = DMA_CHAN_INT_BASE + chan;
	ibit = BITSET(nbit);

	sprintf(msgStr, "\nUIC Polarity Register for channel %d:   Polarity bit (%d) is ",
			  chan, nbit);
	if (uicPolarityReg & ibit)
	{
		strcat(msgStr, " 1 (positive).");
		dmaLogMsg(msgStr);
	}
	else
	{
		strcat(msgStr, "0 (negative).");
		dmaLogMsg(msgStr);
		dmaLogMsg("\t*****ERROR***** must be set to  1 (positive).");
	}
	sprintf(msgStr, "Contents of entire UIC Polarity Register = 0x%08x",
				uicPolarityReg);
	dmaLogMsg(msgStr);
}

/*
=========================================================================
NAME: dmaPrintUICTriggerReg
=========================================================================
PURPOSE:
	Print out the DMA related bits in the UIC trigger register.

ARGUMENTS:
	chan - which channel's info to print out.

RETURN VALUE:
	None.

COMMENTS:

AUTHOR: 
	Robert L. Thrift, 2003
=========================================================================
*/
void dmaPrintUICTriggerReg(int chan)
{
	UINT32 uicTriggerReg, ibit, nbit;

	uicTriggerReg = sysDcrUictrGet();
	nbit = DMA_CHAN_INT_BASE + chan;
	ibit = BITSET(nbit);

	sprintf(msgStr, "\nUIC Trigger Register for channel %d:    Trigger bit (%d) is ",
			  chan, nbit);
	if (uicTriggerReg & ibit)
	{
		strcat(msgStr, "edge-sensitive.");
		dmaLogMsg(msgStr);
		dmaLogMsg("\t*****ERROR*****: must be set to level sensitive.");
	}
	else
	{
		strcat(msgStr, "level-sensitive.");
		dmaLogMsg(msgStr);
	}
	sprintf(msgStr, "Contents of entire UIC Trigger Register = 0x%08x",
					uicTriggerReg);
	dmaLogMsg(msgStr);
}

/*
=========================================================================
NAME: dmaPrintUICMaskedStatusReg
=========================================================================
PURPOSE:
	Print out the DMA related bits in the UIC masked status register.

ARGUMENTS:
	chan - which channel's info to print out.

RETURN VALUE:
	None.

COMMENTS:

AUTHOR: 
	Robert L. Thrift, 2003
=========================================================================
*/
void dmaPrintUICMaskedStatusReg(int chan)
{
	UINT32 uicMaskedStatusReg, ibit, nbit;

	uicMaskedStatusReg = sysDcrUicmsrGet();
	nbit = DMA_CHAN_INT_BASE + chan;
	ibit = BITSET(nbit);

	sprintf(msgStr, "\nUIC Masked Status Reg. for channel %d:  Interrupt bit (%d) is ",
			  chan, nbit);
	if (uicMaskedStatusReg & ibit)
		strcat(msgStr, "set.");
	else
		strcat(msgStr, "cleared.");
	dmaLogMsg(msgStr);
	sprintf(msgStr, "Contents of entire UIC Masked Status Register = 0x%08x",
					uicMaskedStatusReg);
	dmaLogMsg(msgStr);
}

/*
=========================================================================
NAME: dmaPrintSGList
=========================================================================
PURPOSE:
	Print out the contents of the scatter/gather list.

ARGUMENTS:
	chan - which channel's info to print out.

RETURN VALUE:
	None.

COMMENTS:

AUTHOR: 
	Robert L. Thrift, 2003
=========================================================================
*/
void dmaPrintSGList(int chan)
{
        void dmaPrintSGTree(txDesc_t *sgHead);
	txDesc_t	*sgHead;
	dmainfo_t	*dmaInfo;

	sprintf(msgStr, "\nContents of channel %d scatter-gather list:",
					chan);
	dmaLogMsg(msgStr);
	dmaInfo = dmaGetChannelInfo(chan);
	sgHead = dmaInfo->current;
        dmaPrintSGTree(sgHead);
        return;
}

void dmaPrintSGTree(txDesc_t *sgHead)
{
	sgnode_t	*sgNode;
	UINT32	transferMode,
			transferWidth,
			channelPriority;
	int i;
	dmainfo_t	*dmaInfo;

	if (sgHead)
	{
		int j;
		sgnode_t *node;
		i = j = 0;
		sprintf(msgStr, "\tSG transfer descriptor is at: 0x%08x",
						(unsigned int) sgHead);
		dmaLogMsg(msgStr);
		sprintf(msgStr, "\tHead of scatter/gather list is at: 0x%08x",
						(unsigned int) sgHead);
		dmaLogMsg(msgStr);
		if (sgHead->count == 1)
			sprintf(msgStr, "\tThere is 1 node in the scatter/gather list.");
		else
			sprintf(msgStr, "\tThere are %d nodes in the scatter/gather list.",
					  sgHead->count);
		dmaLogMsg(msgStr);
		node = sgHead->first;
		if (sgHead->count)
			while (node)
			{
				sprintf(msgStr, "\t\tnode %d address: 0x%08x",
								j++, (unsigned int) node);
				dmaLogMsg(msgStr);
				node = node->next;
			}

		sgNode = sgHead->first;
		while (sgNode)
		{
			sprintf(msgStr, "\tNode %d at address 0x%08x:", i++,
					(unsigned int) sgNode);
			dmaLogMsg(msgStr);
			sprintf(msgStr, "\t\tChannel control word: 0x%08x",
							sgNode->dmaCCW);
			dmaLogMsg(msgStr);

			/* ----- Print out relevant control word bit settings ----- */
			transferMode  = (sgNode->dmaCCW & DMA_CR_TM) >> 21;
			transferWidth = (sgNode->dmaCCW & DMA_CR_PW) >> 26;

			/* ------------------ Channel enable bit ------------------ */
			sprintf(msgStr, "\t\t\tChannel enable:                ");
			if (sgNode->dmaCCW & DMA_CR_CE)
				strcat(msgStr, "ON");
			else
				strcat(msgStr, "OFF");
			dmaLogMsg(msgStr);

			/* ------------- Channel interrupt enable bit ------------- */
			sprintf(msgStr, "\t\t\tChannel interrupt enable:      ");
			if (sgNode->dmaCCW & DMA_CR_CIE)
				strcat(msgStr, "ON");
			else
				strcat(msgStr, "OFF");
			dmaLogMsg(msgStr);

			/* ---------------- Transfer direction bit ---------------- */
			sprintf(msgStr, "\t\t\tTransfer direction:            ");
			switch(transferMode) {
				case 0:
					if (sgNode->dmaCCW & DMA_CR_TD)
						strcat(msgStr, "peripheral to memory");
					else
						strcat(msgStr, "memory to peripheral");
					break;
				case 1:
						strcat(msgStr, "invalid setting: 01");
					break;
				case 2:
						strcat(msgStr, "not used");
					break;
				case 3:
					if (sgNode->dmaCCW & DMA_CR_TD)
						strcat(msgStr, "peripheral is at src. addr.");
					else
						strcat(msgStr, "peripheral is at dest. addr.");
					break;
			}
			dmaLogMsg(msgStr);

			/* --------------- Peripheral location bit ---------------- */
			if (transferMode != 2)
			{
				sprintf(msgStr, "\t\t\tPeripheral location:           ");
				if (sgNode->dmaCCW & DMA_CR_PL)
					strcat(msgStr, "OPB (UART0)");
				else
					strcat(msgStr, "Extern. peripheral (EBC) bus");
				dmaLogMsg(msgStr);
			}

			/* ----------------- Transfer width bits ------------------ */
			sprintf(msgStr, "\t\t\tPeripheral transfer width:     ");
			switch(transferWidth) {
				case 0:
					strcat(msgStr, "byte (8 bits)");
					break;
				case 1:
					strcat(msgStr, "half-word (16 bits)");
					break;
				case 2:
					strcat(msgStr, "word (32 bits)");
					break;
				case 3:
					strcat(msgStr, "double-word (64 bits)");
					break;
			}
			dmaLogMsg(msgStr);

			/* ---- If addr. decrment bit off, check DAI, SAI bits ---- */
			if ( ! (sgNode->dmaCCW & DMA_CR_DEC))
			{
				/* -------- Destination address increment bit --------- */
				sprintf(msgStr, "\t\t\tDestination address increment: ");
				if (sgNode->dmaCCW & DMA_CR_DAI)
					strcat(msgStr, "ON");
				else
					strcat(msgStr, "OFF");
				dmaLogMsg(msgStr);

				/* ----------- Source address increment bit ----------- */
				sprintf(msgStr, "\t\t\tSource address increment:      ");
				if (sgNode->dmaCCW & DMA_CR_SAI)
					strcat(msgStr, "ON");
				else
					strcat(msgStr, "OFF");
			}
			dmaLogMsg(msgStr);

			/* ------------------ Buffer enable bit ------------------- */
			sprintf(msgStr, "\t\t\tBuffer enable:                 ");
			if (sgNode->dmaCCW & DMA_CR_BEN)
				strcat(msgStr, "ON");
			else
				strcat(msgStr, "OFF");
			dmaLogMsg(msgStr);
		
			/* ------------------ Transfer mode bits ------------------ */
			sprintf(msgStr, "\t\t\tTransfer mode:                 ");
			switch(transferMode) {
				case 0:
					strcat(msgStr, "peripheral");
					break;
				case 1:
					strcat(msgStr, "*****ERROR***** invalid setting: 01");
					break;
				case 2:
					strcat(msgStr, "software initiated mem. to mem");
					break;
				case 3:
					strcat(msgStr, "device paced mem. to mem");
					break;
			}
			dmaLogMsg(msgStr);
		
			if (transferMode != 2)
			{
				/* -------- Peripheral setup, wait, hold bits --------- */
				sprintf(msgStr, "\t\t\tPeripheral setup cycles:       %d",
						((sgNode->dmaCCW & DMA_CR_PSC) >> 19));
				dmaLogMsg(msgStr);

				sprintf(msgStr, "\t\t\tPeripheral wait cycles:        %d",
						((sgNode->dmaCCW & DMA_CR_PWC) >> 13));
				dmaLogMsg(msgStr);
		
				sprintf(msgStr, "\t\t\tPeripheral hold cycles:        %d",
						((sgNode->dmaCCW & DMA_CR_PHC) >> 10));
				dmaLogMsg(msgStr);

				sprintf(msgStr, "\t\t\tEOT/TC pin direction:          ");
				if (sgNode->dmaCCW & DMA_CR_ETD)
					strcat(msgStr, "EOT is an output.");
				else
					strcat(msgStr, "EOT is an input.");
				dmaLogMsg(msgStr);
			}
			else
			{
				if (! (sgNode->dmaCCW & DMA_CR_ETD))
				{
					dmaLogMsg("\t\t\tEOT/TC pin direction is configured as an input.");
					dmaLogMsg("\t\t\t*****ERROR*** wrong setting for softw. init. mem. to mem.");
				}
			}

			/* -------------- Terminal count enable bit --------------- */
			sprintf(msgStr, "\t\t\tTerminal count enable bit:     ");
			if (sgNode->dmaCCW & DMA_CR_TCE)
			{
				strcat(msgStr, "ON (stops at TC.)");
			}
			else
				strcat(msgStr, "OFF (does not stop at TC.)");

			dmaLogMsg(msgStr);
			if (! (sgNode->dmaCCW & DMA_CR_ETD))
				dmaLogMsg("\t\t\t*****ERROR***** incompatible with EOT/TC pin direction.");

			/* ---------------- Channel priority bits ----------------- */
			sprintf(msgStr, "\t\t\tChannel priority bits:         ");
			channelPriority = (((sgNode->dmaCCW & DMA_CR_CP) >> 6) & 3);
			switch(channelPriority) {
				case 0:
					strcat(msgStr, "00 - low.");
					break;
				case 1:
					strcat(msgStr, "01 - med. low.");
					break;
				case 2:
					strcat(msgStr, "10 - med. high.");
					break;
				case 3:
					strcat(msgStr, "11 - high.");
					break;
			}
			dmaLogMsg(msgStr);

			/* ---------- Memory read prefetch control bits ----------- */
			if ((((transferMode == 0) && ((sgNode->dmaCCW & DMA_CR_TD) == 0))
				|| (transferMode == 3)) && (sgNode->dmaCCW & DMA_CR_BEN))
			{
				sprintf(msgStr, "\t\t\tMemory read prefetch:          ");
				switch((sgNode->dmaCCW & DMA_CR_PF) >> 4) {
					case 0:
						strcat(msgStr, "prefetch 1 doubleword");
						break;
					case 1:
						strcat(msgStr, "prefetch 2 doublewords");
						break;
					case 2:
						strcat(msgStr, "prefetch 4 doublewords");
						break;
					case 3:
						strcat(msgStr, "invalid prefetch value.");
						break;
				}
					dmaLogMsg(msgStr);
			}

			/* --------------- Parity check enable bit ---------------- */
			sprintf(msgStr, "\t\t\tParity check enable:           ");
			if (sgNode->dmaCCW & DMA_CR_PCE)
				strcat(msgStr, "enabled.");
			else
				strcat(msgStr, "disabled.");
			dmaLogMsg(msgStr);
		
			/* -------------- Auto address decrement bit -------------- */
			sprintf(msgStr, "\t\t\tAddress decrement:             ");
			msgStr2[0] = '\000';
			if (transferMode != 0)
				strcat(msgStr, "N/A");
			else
			{
				if (sgNode->dmaCCW & DMA_CR_DEC)
				{
					strcat(msgStr, "addr. decremented after each transfer.");
					if (sgNode->dmaCCW & DMA_CR_BEN)
						sprintf(msgStr2, "\t\t\t*****ERROR***** Buffer Enable must be OFF!");
				}
				else
					sprintf(msgStr, "Not used - controlled by SAI, DAI bits.");
			}
			dmaLogMsg(msgStr);
			if (msgStr2[0])
				dmaLogMsg(msgStr2);

			/* -------------- Done with SG control word --------------- */

			sprintf(msgStr, "\t\tSource address: 0x%08x",
							sgNode->dmaSrcAddr);
			dmaLogMsg(msgStr);

			sprintf(msgStr, "\t\tDestination address: 0x%08x",
							sgNode->dmaDstAddr);
			dmaLogMsg(msgStr);

			/* ---------------- LK, TCI, ETI, ERI bits ---------------- */
			msgStr2[0] = '\000';
			sprintf(msgStr, "\t\tControl Bits: 0x%08x",
							sgNode->dmaCtrlBits);
			dmaLogMsg(msgStr);
			sprintf(msgStr, "\t\t\tLink bit is ");
			if (sgNode->dmaCtrlBits & DMA_SG_LK)
				strcat(msgStr, "ON.");
			else
			{
				strcat(msgStr, "OFF.");
				if (sgNode->next)	/* Should be on if not last node	*/
					sprintf(msgStr2, "*****ERROR***** should be on.");
			}
			dmaLogMsg(msgStr);
			if (msgStr2[0])
				dmaLogMsg(msgStr2);

			msgStr2[0] = '\000';
			sprintf(msgStr, "\t\t\tTerminal count interrupt bit is ");
			if (sgNode->dmaCtrlBits & DMA_SG_TCI)
			{
				strcat(msgStr, "ON.");
				if (sgNode->next)
					sprintf(msgStr2, "*****ERROR***** should be off.");
			}
			else
			{
				strcat(msgStr, "OFF.");
				if (! (sgNode->next))
					sprintf(msgStr2, "****ERROR***** should be on.");
			}
			dmaLogMsg(msgStr);
			if (msgStr2[0])
				dmaLogMsg(msgStr2);

			sprintf(msgStr, "\t\t\tEnd-of-transfer interrupt bit is ");
			if (sgNode->dmaCtrlBits & DMA_SG_ETI)
				strcat(msgStr, "ON.");
			else
				strcat(msgStr, "OFF.");
			dmaLogMsg(msgStr);

			msgStr2[0] = '\000';
			sprintf(msgStr, "\t\t\tError interrupt bit is ");
			if (sgNode->dmaCtrlBits & DMA_SG_ERI)
				strcat(msgStr, "ON.");
			else
			{
				strcat(msgStr, "OFF.");
				sprintf(msgStr2, "****ERROR***** should be on.");
			}
			dmaLogMsg(msgStr);
			if (msgStr2[0])
				dmaLogMsg(msgStr2);

			/* ------------- Transfer count for this node ------------- */
			sprintf(msgStr, "\t\t\tTransfer count for this node is 0x%x (%d). (0 eq. 64K)",
				sgNode->dmaCtrlBits & DMA_CNT_MASK,
				sgNode->dmaCtrlBits & DMA_CNT_MASK);
			dmaLogMsg(msgStr);

			/* ------------ Go print the next SG node, now ------------ */
			if (sgNode->next)
				sprintf(msgStr, "\t\tNext node is at address: 0x%08x",
							(unsigned int) sgNode->next);
			else
				sprintf(msgStr, "\t\tNext node is a NULL pointer.");
			dmaLogMsg(msgStr);
			sgNode = sgNode->next;
		}
		/* ---------------- End of scatter/gather list ---------------- */
		sprintf(msgStr, " ");
		dmaLogMsg(msgStr);
	}
	else
	{
		dmaLogMsg("\tHead of scatter/gather list is NULL.");
	}
}


/*
=========================================================================
NAME: dmaDebug
=========================================================================
PURPOSE:
	Print out the contents of all of the DMA controller registers
	for one specific DMA channel.

ARGUMENTS:
	chan - which channel's info to print out.

RETURN VALUE:
	Always returns OK.

COMMENTS:
	Calls separate print routines for each register.

AUTHOR: 
	Robert L. Thrift, 2003
=========================================================================
*/
STATUS dmaDebug(int chan)
{
	dmaPrintStatusReg(chan);
	dmaPrintControlReg(chan);
	dmaPrintCountReg(chan);
	dmaPrintDestReg(chan);
	dmaPrintSrcReg(chan);
	dmaPrintSGReg(chan);
	dmaPrintSGCommandReg(chan);
	dmaPrintSleepModeReg(chan);
	dmaPrintPolarityConfigReg(chan);
	dmaPrintUICStatusReg(chan);
	dmaPrintUICEnableReg(chan);
	dmaPrintUICCriticalReg(chan);
	dmaPrintUICPolarityReg(chan);
	dmaPrintUICTriggerReg(chan);
	dmaPrintUICMaskedStatusReg(chan);
	return OK;
}

/*
=========================================================================
NAME: dmaPrintTxDesc
=========================================================================
PURPOSE:
	Print out the contents of transfer descriptor struct.

ARGUMENTS:
	desc - address of the transfer descriptor.

RETURN VALUE:
	Always returns OK.

COMMENTS:
	Prints out the contents of each field in a struct dmaTxfrDesc.

AUTHOR: 
	Robert L. Thrift, 2003
=========================================================================
*/
void dmaPrintTxDesc(txDesc_t *desc)
{
	sprintf(msgStr, "\nContents of transfer descriptor at address 0x%08x:",
			(unsigned int) desc);
	dmaLogMsg(msgStr);
	sprintf(msgStr, "\tTransfer status:               ");
	switch (desc->txfrStatus) {
        case NOT_READY:
            strcat(msgStr, "NOT_READY");
            break;
        case READY:
            strcat(msgStr, "READY");
            break;
        case RUNNING:
            strcat(msgStr, "RUNNING");
            break;
        case DONE:
            strcat(msgStr, "DONE");
            break;
        default:
            strcat(msgStr, "UNRECOGNIZED STATUS CODE");
            break;
    }
	dmaLogMsg(msgStr);

	sprintf(msgStr, "\tSource type:                   ");
    switch (desc->srcType) {
        case SG_LIST:
            strcat(msgStr, "SG_LIST");
            break;
        case NO_SG_LIST:
            strcat(msgStr, "NO_SG_LIST");
            break;
        default:
            strcat(msgStr, "UNRECOGNIZED SOURCE TYPE");
            break;
    }
	dmaLogMsg(msgStr);

	sprintf(msgStr, "\tChannel control bits:          0x%08x",
					desc->txfrControl);
	dmaLogMsg(msgStr);
	sprintf(msgStr, "\tPrevious descriptor in queue:  0x%08x",
					(unsigned int) desc->prev);
	dmaLogMsg(msgStr);
	sprintf(msgStr, "\tNext descriptor in queue:      0x%08x",
					(unsigned int) desc->next);
	dmaLogMsg(msgStr);
	if (desc->srcType == NO_SG_LIST)
	{
		sprintf(msgStr, "\tSingle-buffer source address:  0x%08x",
					(unsigned int) desc->srcAddr);
		dmaLogMsg(msgStr);
		sprintf(msgStr, "\tSingle-buffer dest. address:   0x%08x",
					(unsigned int) desc->dstAddr);
		dmaLogMsg(msgStr);
		sprintf(msgStr, "\tSingle-buffer transfer size:   %d",
					desc->txfrSize);
		dmaLogMsg(msgStr);
	}
	if (desc->srcType == SG_LIST)
	{
		sprintf(msgStr, "\tFirst node in SG list at:      0x%08x",
					(unsigned int) desc->first);
		dmaLogMsg(msgStr);
		sprintf(msgStr, "\tLast node in SG list at:       0x%08x",
					(unsigned int) desc->last);
		dmaLogMsg(msgStr);
		sprintf(msgStr, "\tNo. of nodes in SG list:       %d",
					desc->count);
		dmaLogMsg(msgStr);
		sprintf(msgStr, "\tInternally-built SG list:      ");
		if (desc->internalSGFlag)
			strcat(msgStr, "yes");
		else
			strcat(msgStr, "no");
		dmaLogMsg(msgStr);
	}
	if (desc->srcMsgQ)
		sprintf(msgStr, "\tSource message queue pointer:  0x%08x\n",
				(unsigned int) desc->srcMsgQ);
	else
		sprintf(msgStr, "\tSource Message queue pointer:  NULL\n");
	dmaLogMsg(msgStr);

	if (desc->dstMsgQ)
		sprintf(msgStr, "\tDest. message queue pointer:   0x%08x\n",
				(unsigned int) desc->dstMsgQ);
	else
		sprintf(msgStr, "\tDest. message queue pointer:   NULL\n");
	dmaLogMsg(msgStr);
}
