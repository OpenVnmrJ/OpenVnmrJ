/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* Id: dmaReg.h,v 1.3 2004/04/12 21:04:22 rthrift Exp rthrift  */
/*
=========================================================================
FILE: dmaReg.h
=========================================================================
Purpose:
	Provide definitions for DMA register get/set calls in dmaReg.c

Author:
	Robert L. Thrift, 2003
=========================================================================
     Copyright (c) 2003, Varian Associates, Inc. All Rights Reserved.
     This software contains proprietary and confidential information
            of Varian Associates, Inc. and its contributors.
  Use, disclosure and reproduction is prohibited without prior consent.
=========================================================================
*/
#ifndef __INCdmaregh
#define __INCdmaregh

#include <vxWorks.h>
#include "dmaDcr.h"

/* Are we compiling dmaReg.c? */
#ifdef _DMAREG_C_
#define EXTERN
#else
#define EXTERN extern
#endif

/* define DMA status register masks by channel, per IBM documentation
Function                   Channel 0  Channel 1  Channel 2  Channel 3
------------------------   ---------  ---------  ---------  ---------
Terminal count status      Bit 0      Bit 1      Bit 2      Bit 3
End of transfer status     Bit 4      Bit 5      Bit 6      Bit 7
Error status               Bit 8      Bit 9      Bit 10     Bit 11
Internal DMA req pending   Bit 12     Bit 13     Bit 14     Bit 15
External DMA req pending   Bit 16     Bit 17     Bit 18     Bit 19
Channel busy               Bit 20     Bit 21     Bit 22     Bit 23
Scatter/gather status      Bit 24     Bit 25     Bit 26     Bit 27
(Bits 28-31 reserved)
*/
#define DMA_SR_BITS_0 (DMA_SR_CS_0 | DMA_SR_TS_0 | DMA_SR_RI_0 | \
					   DMA_SR_IR_0 | DMA_SR_ER_0 | DMA_SR_CB_0 | \
					   DMA_SR_SG_0)
#define DMA_SR_BITS_1 (DMA_SR_CS_1 | DMA_SR_TS_1 | DMA_SR_RI_1 | \
					   DMA_SR_IR_1 | DMA_SR_ER_1 | DMA_SR_CB_1 | \
					   DMA_SR_SG_1)
#define DMA_SR_BITS_2 (DMA_SR_CS_2 | DMA_SR_TS_2 | DMA_SR_RI_2 | \
					   DMA_SR_IR_2 | DMA_SR_ER_2 | DMA_SR_CB_2 | \
					   DMA_SR_SG_2)
#define DMA_SR_BITS_3 (DMA_SR_CS_3 | DMA_SR_TS_3 | DMA_SR_RI_3 | \
					   DMA_SR_IR_3 | DMA_SR_ER_3 | DMA_SR_CB_3 | \
					   DMA_SR_SG_3)
#define DMA_SR_MASK_0 (~DMA_SR_BITS_0)
#define DMA_SR_MASK_1 (~DMA_SR_BITS_1)
#define DMA_SR_MASK_2 (~DMA_SR_BITS_2)
#define DMA_SR_MASK_3 (~DMA_SR_BITS_3)

/* --------------------- Max transfer size is 64K --------------------- */
#define MAX_DMA_TRANSFER_SIZE	0x00010000

/* -------- Define control bits in word 4 of SG list descriptor -------- */
/* LK	Link bit (always 0 for last node in list, otherwise 1)
   TCI	Terminal count enable bit (interrupt when terminal count occurs)
   ETI  End of transfer interrupt (interrupt when end of transfer occurs)
   ERI	Enable error interrupt bit
   CNT	Transfer count, in low 16 bits of word							*/
#define DMA_SG_LK		0x80000000
#define DMA_SG_TCI		0x20000000
#define DMA_SG_ETI		0x10000000
#define DMA_SG_ERI		0x08000000
#define DMA_CNT_MASK	(MAX_DMA_TRANSFER_SIZE - 1)

/* 
 * -----------------------------------------------------------------------
 * Define a standard DMA control word for software-initiated memory-to
 * memory DMA transfers.
 * 	Transfer width: 32 bits
 * 	Source address incrementing
 * 	Destination address incrementing
 * 	Internal buffering enabled
 * 	Mode is software initiated memory to memory
 * 	EOT is a TC output
 * 	Stop when TC is reached
 * 	Medium high priority
 *
 * Note 1: Channel enable and channel interrupt enable bits are off.
 *         So setting this value into the control register will not
 *         start the channel running. In fact, they will stop it.
 *		   This is the way it should be during channel setup.
 *
 * Note 2: EOT = end of transfer; TC = terminal count
 *         Individual bits are defined in dmaDcr.h
 * -----------------------------------------------------------------------
 */
#define STD_CTRL DMA_CR_PW_WORD	\
            | DMA_CR_SAI		\
            | DMA_CR_DAI		\
            | DMA_CR_BEN		\
            | DMA_CR_TM_SWMM	\
            | DMA_CR_ETD		\
            | DMA_CR_TCE		\
            | DMA_CR_CP_MEDH

/* 
 * -----------------------------------------------------------------------
 * Define a standard DMA control word for device-paced memory-to-memory
 * DMA transfers to a peripheral (i.e., to an output FIFO).
 * 	Transfer width 32 bits
 * 	Source address incrementing (but not dest. address incrementing)
 * 	Internal buffering enabled
 * 	Mode is device paced memory to memory
 * 	EOT is a TC output
 * 	Stop when TC is reached
 * 	Medium high priority
 * 	Memory read prefetch (32 bytes) enabled
 *
 * Note 1: Channel enable and channel interrupt enable bits are off.
 *         So setting this value into the control register will not
 *         start the channel running. In fact, they will stop it.
 *		   This is the way it should be during channel setup.
 *
 * Note 2: EOT = end of transfer; TC = terminal count
 *         Individual bits are defined in dmaDcr.h
 * -----------------------------------------------------------------------
 */
#define STD_FIFO_OUT_CTRL (DMA_CR_PW_WORD \
            | DMA_CR_SAI		\
            | DMA_CR_BEN		\
            | DMA_CR_TM_HWDP	\
            | DMA_CR_ETD		\
            | DMA_CR_TCE		\
            | DMA_CR_CP_MEDH	\
			| DMA_CR_PF_4)

/* 
 * -----------------------------------------------------------------------
 * Define standard DMA S/G control bits for NMR application
 * This is NOT the same as the channel control word. The channel
 * control word goes into the first word in a SG list node.
 * These bits go into the fourth word in the SG list node.
 *
 * In all S/G nodes except the last, the link bit is set to let the DMA
 * controller hardware know that this is not the last node in the chain.
 *
 * Before using in a scatter/gather transfer, the tranfer count must be set
 * into the low 16 bits of the word, no matter whether it is last or not.
 * -----------------------------------------------------------------------
 */
#define STD_SG_CTRL	(DMA_SG_ERI \
			| DMA_SG_LK)

#define STD_SG_CTRL_LAST (DMA_SG_ERI \
			| DMA_SG_TCI \
			| DMA_SG_ETI)

/* 
 * -----------------------------------------------------------------------
 * Prototypes
 * -----------------------------------------------------------------------
 */
/* --------- Disable a channel before changing its registers ---------- */
EXTERN void		dmaDisableChan(int);

/* ------------ Get/Set DMA channel configuration register ------------ */
EXTERN UINT32	dmaGetConfigReg(int);
EXTERN void		dmaSetConfigReg(int, UINT32);

/* ---------------- Get/Set DMA channel count register ---------------- */
EXTERN UINT32	dmaGetCountReg(int);
EXTERN void		dmaSetCountReg(int, UINT32);

/* --------- Get/Set DMA channel destination address register --------- */
EXTERN UINT32	dmaGetDestReg(int);
EXTERN void		dmaSetDestReg(int, UINT32);

/* ----------- Get/Set DMA channel source address register ------------ */
EXTERN UINT32	dmaGetSrcReg(int);
EXTERN void		dmaSetSrcReg(int, UINT32);

/* -- Get/Set DMA channel scatter/gather descriptor address register -- */
EXTERN UINT32	dmaGetSGReg(int);
EXTERN void		dmaSetSGReg(int, UINT32);

/* -------- Get/Set DMA channel scatter/gather command register ------- */
EXTERN UINT32	dmaGetSGCommandReg(int);
EXTERN void		dmaEnableSGCommandReg(int);
EXTERN void		dmaDisableSGCommandReg(int);

/* ------------------- Get/Set DMA status register -------------------- */
EXTERN UINT32	dmaGetStatusReg(int);
EXTERN void		dmaSetStatusReg(int, UINT32);
EXTERN int		dmaGetDevicePacingStatus(int);
EXTERN int		dmaGetDeviceActiveStatus(int);

/* -------------------- Get/Set DMA sleep register -------------------- */
EXTERN UINT32	dmaGetSleepReg(void);
EXTERN void		dmaSetSleepReg(UINT32);

/* ------------------ Get/Set DMA polarity register ------------------- */
EXTERN UINT32	dmaGetPolarityReg(int);
EXTERN void		dmaSetPolarityReg(int, UINT32);

#endif		/* __INCdmaregh */

