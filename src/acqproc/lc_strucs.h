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

/* --- code type definitions, can be changed for different machines */
typedef char codechar;		/* 1 bytes */
typedef short codeint;		/* 2 bytes */
typedef long  codelong;		/* 4 bytes */

/*-----------------------------------------------------------------------
|	auto data structure (pointed to by lc) 
| 	this structure is 256 bytes EXACTLY be CAREFUL 
|
+--------------------------------------------------------------------*/
struct _autod {
	codeint coil_val[32];	/* contains shim coil values */
	codelong checkmask;	/* bit mask for coil on/off */
	codechar com_string[128];	/* command (method) string */
	codeint when_mask;	/* when to shim mask */
	codeint control_mask;	/* internal autoshim state */
	codeint best,loops;	/* reports best vertex, # loops */
	codeint sample_mask;	/* location of sample in tray, */
				/* change sample at begin of experiment, */
				/* if not current sample */
	codeint sample_error;	/* report sample error # */
	codeint recgain;	/* reciever gain;lock mode, Z0 field offset */
	codeint lockpower;	/* lock power in db (0-63) */
	codeint lockgain;	/* lock gain in db (0-70) */
	codeint lockphase;	/* lock phase 0-360 degrees */
        codeint knobs[20];	/* acq. knobs info, not to be used by acq. */
   };

typedef struct _autod autodata;

/* low memory allocation */

struct _lc {		/* word allocation of the "acode" data set */
	codechar  id;		/* diffrent ID's are incompatible */
	codechar  versn;	/* versions are compatible */
	codeint	  o2auto;	/* offset to auto structure (set by psg) */
	codelong  autop;
	/*autodata   *autop;	/* pointer to auto structure (set by apint) */
	codeint	  spare1;	/* unused */
	codelong  dpts;		/* total data points */
	codeint	  spare2;	/* unused */
	codeint	  ctctr;	/* ct will be sent to host every ctctr trans.*/
	codeint	  dsize;	/* blocks of data */
	codeint	  asize;	/* blocks of acode/parameters */
	codeint	  codeb;	/* offset within code of first instruction */
	codeint	  codep;	/* offset within code of current instruction */
	codelong  np;		/* number of points */
	codelong  nt;		/* number of transients */
	codelong  ct;		/* completed transients */
	codeint	  status;	/* current system status */
	codeint	  dpf;		/* 0=integer, 4=long */
	codeint	  maxscale;	/* max number of shifts allowed */
	codeint	  icmode;	/* input card mode/shift */
	codeint	  stmchk;	/* current stm check/control word */
	codeint	  nflag;	/* current stm control value */
	codeint	  scale;	/* current scale value */
	codeint	  check;	/* stm value to check for overflow */
	codeint	  oph;		/* local value */
	codeint	  bs;		/* block size */
	codeint	  bsct;		/* block size counter */
	codeint	  ss;		/* steady state */
	codeint	  ssct;		/* steady state counter */
	codeint	  ctcom;	/* ct counting backwards */
	codeint	  dptab[4];	/* decoupler phase table */
	codeint	  obsptb[4];	/* observe transmitter phase table */
	codeint	  rfphpt;
	codeint	  squi;		/* starting gate pattern */
	codeint	  curdec;	/* unused */
/* the next 12 loc are for hardware status register image */
	codeint	  ocsr;		/* output card status regiter */
	codeint	  apsr;		/* analog port status regiter */
	codeint	  apdr;		/* analog port data regiter */
	codeint	  apar;		/* analog port address/control register */
	codeint	  icsr;		/* input card status regiter */
	codeint	  icdr;		/* input card data register */
	codeint	  icocsr;	/* input card ocsr register */
	codeint	  stmsr;	/* stm card status register */
	codelong  stmar;	/* stm card address register */
	codelong  stmcr;	/* stm card count register */
/*	start of user variables */
	codeint	  zero;
	codeint	  one;
	codeint	  two;
	codeint	  three;
/*	allocation for ten user tmp locs v1 - v10 */
	codeint	  v1;
	codeint	  v2;
	codeint	  v3;
	codeint	  v4;
	codeint	  v5;
	codeint	  v6;
	codeint	  v7;
	codeint	  v8;
	codeint	  v9;
	codeint	  v10;
/*	allocation for extra seven user tmp locs v11 - v14 */
	codeint	  v11_14[4];
	codeint	  cpf;		/*cycle phase flag*/
	codelong  isum;		/*imaginary sum from noisecheck*/
	codelong  rsum;		/*real sum from noisecheck*/
	codeint	  maxconst;	/*adjustment to maxscale*/
      } ;
/* *	end of lc structure	* */

typedef struct _lc Acqparams;

