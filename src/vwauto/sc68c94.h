/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* sc68c94.h - SC68c94 QUART tty driver definitions */
/*  "sc68c94.h Copyright (c) 1994-1996 Varian Assoc.,Inc. All Rights Reserved" */
/*
*/
/* sc68c94.h QUART header file */
/* m68681.h - Motorola M68681 serial chip header file */

/* Copyright 1984-1996 Wind River Systems, Inc. */
/*
modification history
--------------------
28feb95,gad	converted to sc6894 QUART.
--------------------
01h,22sep92,rrr  added support for c++
01g,14aug92,caf  added TY_CO_DEV and function declarations for 5.1 upgrade.
		 for 5.0.x compatibility, define INCLUDE_TY_CO_DRV_50 when
		 including this header.
01f,26may92,rrr  the tree shuffle
01e,04oct91,rrr  passed through the ansification filter
		  -changed ASMLANGUAGE to _ASMLANGUAGE
		  -changed copyright notice
01d,05oct90,shl  added copyright notice.
                 made #endif ANSI style.
01c,10sep89,rld  added DUART registers.
01b,30apr88,gae  added definition INCm68681h.
01a,06jan88,miz	 written for Mizar Digital Systems.
*/

/*
This file contains constants for the Signetics SC68C94 QUART serial chip.

The constant and SERIAL_QSC must defined when including this header file.
*/

#ifndef __INCsc68c94h
#define __INCsc68c94h

#ifdef __cplusplus
extern "C" {
#endif

#ifndef	_ASMLANGUAGE

#include <tyLib.h>

typedef struct          /* TYQ_CO_DEV */
    {
    TY_DEV tyDev;
    BOOL created;       /* true if this device has really been created */
		int  heartbeat;     /* this interrupt heartbeat                    */
    char numChannels;   /* number of channels to support               */
		char baud;          /* current csr setting                         */
    char *mr;           /* mode register                               */
    char *sr;           /* status register                             */
    char *csr;          /* clock select register                       */
    char *cr;           /* control register                            */
    char *dr;           /* data port                                   */
    char *iopcr;        /* input port configuration register           */
    char *bcr;          /* bidding control register                    */
    char rem;           /* bit for receive-enable mask                 */
    char tem;           /* bit for transmit-enable mask                */
         /* The Following registers are shared                         */
    char *ipcr;         /* input port change register                  */
    char *acr;          /* auxiliary control register                  */
    char *isr;          /* interrupt status register                   */
    char *imr;          /* interrupt mask register                     */
    char *ctur;         /* counter timer upper register                */
    char *ctlr;         /* counter timer lower register                */
    char *opr;          /* output port register                        */
    char *ipr;          /* input port register                         */
    char *ctron;        /* counter on                                  */
    char *ctroff;       /* counter off                                 */
    char *kimr;         /* keep current interrupt mask register value  */
    VOIDFUNCPTR tickRtn; /* timer ISR (NULL if none)		       */
		int mode;           /* for testing interrupt mode */
		char newchar;       /* current char for testing */
    } TYQ_CO_DEV;

#endif	/* _ASMLANGUAGE */

/* equates for mode reg. A, B, C or D, mode 1 */
#define	RX_RTS		0x80		/* 0 = no, 1 = yes */
#define	RX_INT		0x40		/* 0 = RxRDY, 1 = FFULL */
#define	ERR_MODE	0x20		/* 0 = char, 1 = block */
#define	PAR_MODE_MULTI	0x18		/* multi_drop mode */
#define	PAR_MODE_NO	0x10		/* no parity mode */
#define	PAR_MODE_FORCE	0x08		/* force parity mode */
#define	PAR_MODE_YES	0x00		/* parity mode */
#define	PARITY_TYPE	0x04		/* 0 = even, 1 = odd */
#define	BITS_CHAR_8	0x03		/* 8 bits */
#define	BITS_CHAR_7	0x02		/* 7 bits */
#define	BITS_CHAR_6	0x01		/* 6 bits */
#define	BITS_CHAR_5	0x00		/* 5 bits */
/* equates for mode reg. A, B, C or D, mode 2 */
#define	CHAN_MODE_RL	0xc0		/* remote loop */
#define	CHAN_MODE_LL	0x80		/* local loop */
#define	CHAN_MODE_AECHO	0x40		/* auto echo */
#define	CHAN_MODE_NORM	0x00		/* normal */
#define	TX_RTS		0x20		/* 0 = no, 1 = yes */
#define	CTS_ENABLE	0x10		/* 0 = no, 1 = yes */
#define	STOP_BITS_2	0x0f		/* 2 */
#define	STOP_BITS_1	0x07		/* 1 */
/* equates for clock select reg. A, B, C or D */
#define	RX_CLK_SEL	0xf0		/* receiver clock select */
#define	RX_CLK_19200	0xc0		/* 19200 */
#define	RX_CLK_9600	0xb0		/* 9600 */
#define	RX_CLK_1800	0xa0		/* 1800 */
#define	RX_CLK_4800	0x90		/* 4800 */
#define	RX_CLK_2400	0x80		/* 2400 */
#define	RX_CLK_2000	0x70		/* 2000 */
#define	RX_CLK_1200	0x60		/* 1200 */
#define	RX_CLK_600	0x50		/* 600 */
#define	RX_CLK_300	0x40		/* 300 */
#define	RX_CLK_150	0x30		/* 150 */
#define	RX_CLK_38_4K	0x20		/* 38.4k */
#define	RX_CLK_110	0x10		/* 110 */
#define	RX_CLK_75	0x00		/* 75 */
#define	TX_CLK_SEL	0x0f		/* transmitter clock select */
#define	TX_CLK_19200	0x0c		/* 19200 */
#define	TX_CLK_9600	0x0b		/* 9600 */
#define	TX_CLK_1800	0x0a		/* 1800 */
#define	TX_CLK_4800	0x09		/* 4800 */
#define	TX_CLK_2400	0x08		/* 2400 */
#define	TX_CLK_2000	0x07		/* 2000 */
#define	TX_CLK_1200	0x06		/* 1200 */
#define	TX_CLK_600	0x05		/* 600 */
#define	TX_CLK_300	0x04		/* 300 */
#define	TX_CLK_150	0x03		/* 150 */
#define	TX_CLK_38_4K	0x02		/* 38.4k */
#define	TX_CLK_110	0x01		/* 110 */
#define	TX_CLK_75	0x00		/* 75 */
/* equates for status reg. A, B, C or D */
#define	RXD_BREAK	0x80		/* 0 = no, 1 = yes */
#define	FRAMING_ERR	0x40		/* 0 = no, 1 = yes */
#define	PARITY_ERR	0x20		/* 0 = no, 1 = yes */
#define	OVERRUN_ERR	0x10		/* 0 = no, 1 = yes */
#define	TXEMT		0x08		/* 0 = no, 1 = yes */
#define	TXRDY		0x04		/* 0 = no, 1 = yes */
#define	FFULL		0x02		/* 0 = no, 1 = yes */
#define	RXRDY		0x01		/* 0 = no, 1 = yes */
/* equates for command reg. A, B, C or D */
/* miscellaneous commands: 0x70 */
#define	DIS_TO_MODE	0xc0		/* disable timeout mode */
#define	SET_MR_PTR_ZRO	0xb0		/* set mr pointer to 0 command */
#define	SET_TO_MODE_ON	0xa0		/* set timeout mode on */
#define	NEG_RTSN_CMD	0x90		/* negate RTSN (high) */
#define	AST_RTSN_CMD	0x80		/* assert RTSN (low) */
#define	STP_BREAK_CMD	0x70		/* stop break command */
#define	STR_BREAK_CMD	0x60		/* start break command */
#define	RST_BRK_INT_CMD	0x50		/* reset break int. command */
#define	RST_ERR_STS_CMD	0x40		/* reset error status command */
#define	RST_TX_CMD	0x30		/* reset transmitter command */
#define	RST_RX_CMD	0x20		/* reset receiver command */
#define	RST_MR_PTR_CMD	0x10		/* reset mr pointer to 1 command */
#define	NO_COMMAND	0x00		/* no command */
#define	TX_DISABLE	0x08		/* 0 = no, 1 = yes */
#define	TX_ENABLE	0x04		/* 0 = no, 1 = yes */
#define	RX_DISABLE	0x02		/* 0 = no, 1 = yes */
#define	RX_ENABLE	0x01		/* 0 = no, 1 = yes */
/* equates for auxiliary control reg. (timer and counter clock selects) */
#define	BRG_SELECT	0x80		/* baud rate generator select */
					/* 0 = set 1; 1 = set 2 */
					/* NOTE above equates are set 2 ONLY */
#define	TMR_EXT_CLK_16	0x70		/* external clock divided by 16 */
#define	TMR_EXT_CLK	0x60		/* external clock */
#define	TMR_IO1_16	0x50		/* io1 divided by 16 */
#define	TMR_IO1		0x40		/* io1 */
#define	CTR_EXT_CLK_16	0x30		/* external clock divided by 16 */
#define	CTR_TXC 	0x20		/* channel transmitter clock */
#define	CTR_IO1_16	0x10		/* io1 divided by 16 */
#define	CTR_IO1		0x00		/* io1 */
#define	DELTA_IO1bd_INT	0x08		/* delta io1b int. */
#define	DELTA_IO0bd_INT	0x04		/* delta io0b int. */
#define	DELTA_IO1ac_INT	0x02		/* delta io1a int. */
#define	DELTA_IO0ac_INT	0x01		/* delta io0a int. */
/* equates for input port change reg. */
#define	DELTA_IO1bd	0x80		/* 0 = no, 1 = yes */
#define	DELTA_IO0bd	0x40		/* 0 = no, 1 = yes */
#define	DELTA_IO1ac	0x20		/* 0 = no, 1 = yes */
#define	DELTA_IO0ac	0x10		/* 0 = no, 1 = yes */
#define	IO1bd		0x08		/* 0 = low, 1 = high */
#define	IO0bd		0x04		/* 0 = low, 1 = high */
#define	IO1ac		0x02		/* 0 = low, 1 = high */
#define	IO0ac		0x01		/* 0 = low, 1 = high */
/* equates for int. mask reg. */
#define	INPUT_DELTA_INT	0x80		/* 0 = off, 1 = on */
#define	BREAK_BD_INT	0x40		/* 0 = off, 1 = on */
#define	RX_RDY_BD_INT	0x20		/* 0 = off, 1 = on */
#define	TX_RDY_BD_INT	0x10		/* 0 = off, 1 = on */
#define	CTR_RDY_INT	0x08		/* 0 = off, 1 = on */
#define	BREAK_AC_INT	0x04		/* 0 = off, 1 = on */
#define	RX_RDY_AC_INT	0x02		/* 0 = off, 1 = on */
#define	TX_RDY_AC_INT	0x01		/* 0 = off, 1 = on */
/* equates for int. status reg. */
#define	INPUT_DELTA	0x80		/* 0 = no, 1 = yes */
#define	BREAK_BD	0x40		/* 0 = no, 1 = yes */
#define	RX_RDY_BD	0x20		/* 0 = no, 1 = yes */
#define	TX_RDY_BD	0x10		/* 0 = no, 1 = yes */
#define	CTR_RDY		0x08		/* 0 = no, 1 = yes */
#define	BREAK_AC	0x04		/* 0 = no, 1 = yes */
#define	RX_RDY_AC	0x02		/* 0 = no, 1 = yes */
#define	TX_RDY_AC	0x01		/* 0 = no, 1 = yes */
/* equates for I/O port config. reg. */
#define	IOPCR_AUTO	0x50		/* IO0=CTS, 1=CD, 2=RTS, 3=DTR */

/* equates for Global CIR type */
#define CIR_CHAN  0x03    /* bits 0,1 */
#define CIR_TYPE  0x1c    /* bits 2,3,4 */
#define CIR_COS   0x04    /* 1 = Change of State */
#define CIR_BRK   0x10    /* 4 = Receiver Break */
#define CIR_CT    0x14    /* 5 = Counter Timer */
#define CIR_RX    0x0c    /* 3 = Receive available no errors */
#define CIR_RXNE  0x0c    /* 3 = Receive available no errors */
#define CIR_RXE   0x1c    /* 7 = Receive available with errors */
#define CIR_TX    0x08    /* 2 = Transmit available */
#define CIR_TX2   0x18    /* 6 = Transmit available */

/* function declarations */

#ifndef	INCLUDE_TY_CO_DRV_50
#ifndef	_ASMLANGUAGE
#if defined(__STDC__) || defined(__cplusplus)

IMPORT  void    tyQCoIntM0 (int channel);
IMPORT  void    tyQCoIntM1 (int channel);
IMPORT  void    tyQCoIntWr (int channel);
IMPORT  void    tyQCoIntRd (int channel);
IMPORT  void    tyQCoIntRdE (int channel);
IMPORT  void    tyQCoIntBk (int channel);
IMPORT  void    tyQCoIntTm (int channel);
IMPORT  void    tyQCoIntCs (int channel);

#else	/* __STDC__ */

IMPORT  void    tyQCoIntM0 ();
IMPORT  void    tyQCoIntM1 ();
IMPORT  void    tyQCoIntWr ();
IMPORT  void    tyQCoIntRd ();
IMPORT  void    tyQCoIntRdE ();
IMPORT  void    tyQCoIntBk ();
IMPORT  void    tyQCoIntTm ();
IMPORT  void    tyQCoIntCs ();

#endif	/* __STDC__ */
#endif	/* _ASMLANGUAGE */
#endif	/* INCLUDE_TY_CO_DRV_50 */

#ifdef __cplusplus
}
#endif

#endif /* __INCsc68c94h */
