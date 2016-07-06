/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* quart.h - SC68c94 QUART tty Initializer definitions */
#ifndef LINT
/*  "quart.h Copyright (c) 1991-1996 Varian Assoc.,Inc. All Rights Reserved" */
#endif
/*
*/
/* Quart addendum  (Should be at system level) */

/* Local I/O address map */

#define	QSC_BASE_ADRS	(0x00200000)	/* sc6894 (Serial Comm Contr) */
#define	SERIAL_QSC	QSC_BASE_ADRS

#define NUM_QSC_CHANNELS    4	/* number of serial channels on chip */

/* interrupt vector locations */

#define	INT_VEC_QSC		128	/* 0x80 (Must be on modulo 32 base) */

#ifndef _ASMLANGUAGE

#define QSC_CIR ((unsigned char *) 0x200050)
#define QSC_IVR ((unsigned char *) 0x200052)
#define QSC_GRxFIFO ((unsigned char *) 0x200056)
#define QSC_GTxFIFO ((unsigned char *) 0x200056)
#define QSC_ICR ((unsigned char *) 0x200058)

#define INT_VEC_QSC_M0(qsc_base)	qsc_base + 0x00	/* Mode 0 IVR  */

#define INT_VEC_QSC_A_M1(qsc_base)	qsc_base + 0x00	/* Mode 1 IVR + CH #  */
#define INT_VEC_QSC_B_M1(qsc_base)	qsc_base + 0x01	/* Mode 1 IVR + CH #  */
#define INT_VEC_QSC_C_M1(qsc_base)	qsc_base + 0x02	/* Mode 1 IVR + CH #  */
#define INT_VEC_QSC_D_M1(qsc_base)	qsc_base + 0x03	/* Mode 1 IVR + CH #  */

/* The following 28 defines are for Mode 2, IVR + Type + CH # */

#define INT_VEC_QSC_A_CS(qsc_base)	qsc_base + 0x04	/* Change of State */
#define INT_VEC_QSC_B_CS(qsc_base)	qsc_base + 0x05
#define INT_VEC_QSC_C_CS(qsc_base)	qsc_base + 0x06
#define INT_VEC_QSC_D_CS(qsc_base)	qsc_base + 0x07

#define INT_VEC_QSC_A_WR0(qsc_base)	qsc_base + 0x08	/* Xmit ready */
#define INT_VEC_QSC_B_WR0(qsc_base)	qsc_base + 0x09
#define INT_VEC_QSC_C_WR0(qsc_base)	qsc_base + 0x0a
#define INT_VEC_QSC_D_WR0(qsc_base)	qsc_base + 0x0b

#define INT_VEC_QSC_A_RDNE(qsc_base)	qsc_base + 0x0c /* Read No Error */
#define INT_VEC_QSC_B_RDNE(qsc_base)	qsc_base + 0x0d
#define INT_VEC_QSC_C_RDNE(qsc_base)	qsc_base + 0x0e
#define INT_VEC_QSC_D_RDNE(qsc_base)	qsc_base + 0x0f

#define	INT_VEC_QSC_A_BK(qsc_base)	qsc_base + 0x10 /* Break */
#define	INT_VEC_QSC_B_BK(qsc_base)	qsc_base + 0x11
#define	INT_VEC_QSC_C_BK(qsc_base)	qsc_base + 0x12
#define	INT_VEC_QSC_D_BK(qsc_base)	qsc_base + 0x13

#define INT_VEC_QSC_A_TM(qsc_base)	qsc_base + 0x14 /* Timer */
#define INT_VEC_QSC_B_TM(qsc_base)	qsc_base + 0x15
#define INT_VEC_QSC_C_TM(qsc_base)	qsc_base + 0x16
#define INT_VEC_QSC_D_TM(qsc_base)	qsc_base + 0x17

#define INT_VEC_QSC_A_WR1(qsc_base)	qsc_base + 0x18 /* Xmit ready */
#define INT_VEC_QSC_B_WR1(qsc_base)	qsc_base + 0x19
#define INT_VEC_QSC_C_WR1(qsc_base)	qsc_base + 0x1a
#define INT_VEC_QSC_D_WR1(qsc_base)	qsc_base + 0x1b

#define INT_VEC_QSC_A_RDER(qsc_base)	qsc_base + 0x1c /* Read + Error */
#define INT_VEC_QSC_B_RDER(qsc_base)	qsc_base + 0x1d
#define INT_VEC_QSC_C_RDER(qsc_base)	qsc_base + 0x1e
#define INT_VEC_QSC_D_RDER(qsc_base)	qsc_base + 0x1f

#endif  /* _ASMLANGUAGE */

#define	QSC_IRQ_LEVEL		4	/* serial comm IRQ level      */

/* end quart.h */
