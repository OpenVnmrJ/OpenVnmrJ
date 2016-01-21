/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*----------------------------------------------------------------------
|	The RF constant's definitions
+----------------------------------------------------------------------*/
#define TRUE 1
#define FALSE 0

#ifndef NVPSG
#ifndef MAX_RFCHAN_NUM
#define MAX_RFCHAN_NUM 6
#endif

/* ap constant offset addresses */

/* dual usage high speed lines */
/* 1/3/90 VAR1 used for 1st waveform generator */
#define VAR1 		0x1
#define WFG1 		0x1
/* 1/3/90 VAR2 used for 2nd waveform generator */
#define VAR2 		0x2
#define WFG2 		0x2

/* 1/3/90 DECLVL used for second decoupler gate */
#define DECLVL 		0x10
#define DECUPLR2 	0x10

/* 1/3/90 MODA used for 2nd DEC90 */
#define MODMA		0x20
#define DC2_90		0x20

/* 1/3/90 MODB used for 2nd DEC180 */
#define MODMB		0x40
#define DC2_180		0x40
#define DC2_270		0x60

/* 1/3/90 DECPP used for 3rd waveform generator */
#define DECPP  		0x100
#define WFG3 		0x100	/* 3rd waveform generator */

/* single usage high speed lines */
#define HSON		0x80
#define HomoSpoilON 	0x80
#define DC90 		0x200
#define DC180 		0x400
#define DC270 		0x600
#define DECUPLR		0x800
#define RFP90 		0x1000
#define RFP180 		0x2000
#define RFP270 		0x3000
#define SP1 		0x4
#define SP2 		0x8
#define RXOFF 		0x8000
#define TXON 		0x4000

#define INOVA_RCVRGATE	1<<20

/*   pattern ap constants */
#define PATOBSCH    0
#define PATDECCH    8
#define PATAPBASE 0xC00
#define OBSAPADR  0xc00
#define DECAPADR  0xc08

/* XL interface analog port constants */

#define DIGITPOS 4	/* XL Interface digit position in apbuss word */
#define DEVPOS 8 	/* XL Interface device position in apbuss word */

#define TODEV 1
#define DODEV 2
#define DO2DEV 3
#define DO3DEV 4
#define DO4DEV 5
#define DO5DEV 6
#define RFCHAN1 TODEV
#define RFCHAN2 DODEV
#define RFCHAN3 DO2DEV
#define RFCHAN4 4
#define RFCHAN5 5
#define RFCHAN6 6

#define PRG_BG	0	/* background mode for WFG */
#define PRG_FG	1	/* foreground mode for WFG */

#define DLPDEV 2
#define DMFDEV 2
#define FILTERDEV 3
#define	LOCKDEV 3
#define	VTDEV 4
#define PAFDEV 5
#define OBSDEV 5
#define	PTSDEV 7
#define DLPOFFSET 8
#define DMFOFFSET 12
#define	LOCKOFFSET 8
#define	OBSOFFSET 8
#define PTS1OFFSET 0
#define PTS2OFFSET 8
#else
#define TODEV 1
#define DODEV 2
#define DO2DEV 3
#define DO3DEV 4
#define DO4DEV 5
#ifndef MAX_RFCHAN_NUM
#define MAX_RFCHAN_NUM 6
#endif
#define RFCHAN4 4
#endif 
/* NVPSG */

/* --- sequence status constants --- */

#define A 0
#define B 1
#define C 2
#define D 3
#define E 4
#define F 5
#define G 6
#define H 7
#define I 8
#define J 9
#define K 10
#define L 11
#define M 12
#define N 13
#define O 14
#define P 15
#define Q 16
#define R 17
#define S 18
#define T 19
#define U 20
#define V 21
#define W 22
#define X 23
#define Y 24
#define Z 25

#ifndef NVPSG
/* hardware looping and explicit acquisition trigger constants (jrs) */

#define MAXHWLOOPLEN 64
/*const maxhwlooplength = 63;*/

/* filter bandwith max for non WL, used in dofiltercontrol */
#define FBMAX 51200

/* for new rf calculation, flags for setting or not setting the PTS for rftype 'b' */
#define SETUPPTS 1
#define NO_SETUPPTS 0
#define INIT_APVAL  1
#define SET_APVAL   0
#define TRANSIENT_APVAL 2

/* external trigger time bases  */
#define ROTORSYNC_TIMEBASE 0x5000
#define EXT_TRIGGER_TIMEBASE 0x0000
#endif /* NVPSG */
