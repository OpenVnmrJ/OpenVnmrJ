/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 */
#define TRUE	1
#define FALSE	0

/* auto data structure (pointed to by lc) */
/* this structure is 256 bytes EXACTLY be CAREFUL */
struct autod {
	long checkmask;		/* bit mask for coil on/off */
	short when_mask;	/* when to shim mask */
	short control_mask;	/* internal autoshim state */
	short best,loops;	/* reports best vertex, # loops */
	short sample_mask;	/* location of sample in tray, */
				/* change sample at begin of experiment, */
				/* if not current sample */
	short sample_error;	/* report sample error # */
	short recgain;		/* reciever gain;lock mode, Z0 field offset */
	short lockpower;	/* lock power in db (0-49) */
	short lockgain;		/* lock gain in db (0-30) */
	short lockphase;	/* lock phase 0-360 degrees */
	short coil_val[32];	/* contains shim coil values */
	char com_string[128];	/* command (method) string */
        short knobs[20];	/* used with loop() */
   };

/* low memory allocation */

struct lc {		/* word allocation of the "acode" data set */
	long	np;		/* 00 number of points */
	long	nt;		/* 04 number of transients */
	long	ct;		/* 08 completed transients */
	long	isum;		/* 0c imaginary sum from noisecheck*/
	long	rsum;		/* 10 real sum from noisecheck*/
	long	dpts;		/* 14 total data points */
	struct	autod	*autop;	/* 18 pntr to auto structure (set by apint) */
	long	stmar;		/* 1c stm card address register */
	long	stmcr;		/* 20 stm card count register */
	long	rtvptr;		/* 24 ptr to malloc'ed real time variables */
unsigned long	elemid;		/* 28 added for U+ compatability */
unsigned long	squi;		/* 2c starting gate pattern */
	char	id;		/* 30 diffrent ID's are incompatible */
	char	versn;		/* 31 versions are compatible */
	short	o2auto;		/* 32 offset to auto structure (set by psg) */
	short	ctctr;		/* 34 ct sent to host every ctctr trans.*/
	short	dsize;		/* 36 blocks of data */
	short	asize;		/* 38 blocks of acode/parameters */
	short	codeb;		/* 3a offset within code of first instruction */
	short	codep;		/* 3c offset within code of current instruction */
	short	status;		/* 3e current system status */
	short	dpf;		/* 40 0=integer, 4=long */
	short	maxscale;	/* 42 max number of shifts allowed */
	short	icmode;		/* 44 input card mode/shift */
	short	stmchk;		/* 46 current stm check/control word */
	short	nflag;		/* 48 current stm control value */
	short	scale;		/* 4a current scale value */
	short	check;		/* 4c stm value to check for overflow */
	short	oph;		/* 4e local value */
	short	bs;		/* 50 block size */
	short	bsct;		/* 52 block size counter */
	short	ss;		/* 54 steady state */
	short	ssct;		/* 56 steady state counter */
	short	ctcom;		/* 58 ct counting backwards */
	short	dptab[4];	/* 5a decoupler phase table */
	short	obsptb[4];	/* 62 observe transmitter phase table */
	short	rfphpt;		/* 6a */
	short	curdec;		/* 6c unused */
	short	cpf;		/* 6e cycle phase flag*/
	short	maxconst;	/* 70 adjustment to maxscale*/
	short	tablert;	/* 72 Added for U+ compatibility */
/* the next 12 loc are for hardware status register image */
	short	ocsr;		/* 74 output card status regiter */
	short	apsr;		/* 76 analog port status regiter */
	short	apdr;		/* 78 analog port data regiter */
	short	apar;		/* 7a analog port address/control register */
	short	icsr;		/* 7c input card status regiter */
	short	icdr;		/* 7e input card data register */
	short	icocsr;		/* 80 input card ocsr register */
	short	stmsr;		/* 82 stm card status register */
/* Unity compatiblitity parameters */
	short	qtpwrr;		/* 84 */
	short	dpwrr;		/* 86 */
	short	tphsr;		/* 88 */
	short	dphsr;		/* 8a */
	short	dlvlr;		/* 8c */
	short	srate;		/* 8e */
	short	rttmp;		/* 90 */
	short	spare1;		/* 92 unused */
/*	start of user variables */
	short	id2;		/* 94 */
	short	zero;		/* 96 */
	short	one;		/* 98 */
	short	two;		/* 9a */
	short	three;		/* 9c */
/*	allocation for ten user tmp locs v1 - v10 */
	short	v1;		/* 9e */
	short	v2;		/* a0 */
	short	v3;		/* a2 */
	short	v4;		/* a4 */
	short	v5;		/* a6 */
	short	v6;		/* a8 */
	short	v7;		/* aa */
	short	v8;		/* ac */
	short	v9;		/* ae */
	short	v10;		/* b0 */
/*	allocation for extra seven user tmp locs v11 - v14 */
	short	v11;		/* b2 */
        short	v12;
        short	v13;
        short	v14;
        short	v15;
        short	v16;
	short	spare2;		/* be filler, makes multiple of 4 bytes */
      } ;
/* *	end of lc structure	* */
/* if acqparms.h in psg is included before lc.h no need to typedef */
#ifndef ACQPARMS
#define	ACQPARMS
/* --- code type definitions, can be changed for different machines */
typedef char codechar;          /* 1 bytes */
typedef short codeint;          /* 2 bytes */
typedef long  codelong;         /* 4 bytes */
typedef unsigned long  codeulong; /* 4 bytes */
#endif

/*	end of lc structure	* */
typedef struct autod autodata;
typedef struct lc Acqparams;
