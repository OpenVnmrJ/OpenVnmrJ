/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
Id: FIFOdrv.h,v 1.4 2004/04/30 21:17:39 rthrift Exp
=========================================================================
NAME: FIFOdrv.h
=========================================================================
PURPOSE:
	Provide definitions for FIFOdrv.c code.

COMMENT:

AUTHOR: 
	Robert L. Thrift, 2004
=========================================================================
     Copyright (c) 2004, Varian Associates, Inc. All Rights Reserved.
     This software contains proprietary and confidential information
            of Varian Associates, Inc. and its contributors.
  Use, disclosure and reproduction is prohibited without prior consent.
=========================================================================
*/
#ifndef __INCfifodrvh
#define __INCfifodrvh

#define FPGA_BASE  		(0x70000000)
#ifndef FIFO_START
#define FIFO_START		0x1
#define FIFO_SYNC_START	0x2
#define FIFO_RESET		0x4
#endif

/* ------------------------- FIFO Status Bits ------------------------- */
#define FF_OVERFLOW    (1)
#define FF_UNDERFLOW   (2)
#define FF_FINISHED    (4)
#define FF_SYSFAIL     (128)
#define FF_SYSWARN     (256)
#define FIFO_STATUS_MASK 0x1ff

/* ---------------------------- FIFO codes ---------------------------- */
#define LFIFO  (0x80000000)
#define TIMER  (1 << 26)
#define GATES  (2 << 26)
#define XTB    (3 << 26)
#define PHASE  (4 << 26)
#define PHASEC (5 << 26)
#define AMP    (6 << 26)
#define AMPS   (7 << 26)
#define USER   (8 << 26)
#define AUX    (9 << 26)
#define FIFO_STOP	(TIMER | 0 | LFIFO)

#define TIMER_MASK (0x03ffffff)

/* ------------------------------ Macros ------------------------------ */
#define FPGA_PTR(arg) volatile unsigned int *p##arg = (unsigned int *) (FPGA_BASE + arg)
#define FPGA_RO_PTR(arg) volatile unsigned const int const *p##arg = (unsigned int *) (FPGA_BASE + arg)

/* ---------------------------- Prototypes ---------------------------- */
void FPGASetReg(volatile unsigned int *, unsigned int, unsigned int);
void resetFPGA(void);
void resetFIFO(void);
int  getFIFOStatus(void);
void startFIFO(int);
void FIFOwrite(unsigned int);
int  isFIFORunning(void);
STATUS FIFOStart(int);

#endif	/* __INCfifodrvh */

