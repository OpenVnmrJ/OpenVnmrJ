/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*	lc.h	*/

/*****************************************************************************
|  BEWARE changes to any structures MUST adhere to SUN4 alignment rules !!! 
******************************************************************************
|   Modified   Author     Purpose
|   --------   ------     -------
|   6/6/88     Greg B.    1. Changed lc.h structure for SUN4 compatiblity
|			  2. Added lc_struct.h features now vnmr and acqproc
|				can use lc.h with the ACQPROC_LC defined
|   12/15/88   Greg B.    1. replaced spare1 with tablert 
|			  2. replaced spare2 with real-time parameter 
|			     dedicated to table operation
|			  3. added rttmp for interlock usage
|   5/2/91     Greg B.    1. made elemid a unsigned long and moved it to top 
|			     of structure
+----------------------------------------------------------------------------*/
/* --- Key change made in ``lc.h'' structure for SUN4 compatibility. 
       autod structure was already properly aligned  */



#define TRUE	1
#define FALSE	0


/* vm02 acquisition parmeter structure (low memory allocation) */
/* Low Core or lc is a historical name and implies nothing now 6/12/87 */

/* since structure element need to have unique names it was neseccary to
   change them for PSG, Acqproc, and Vnmr this is the reason for this 
   condition compile statement 	*/

/*  if it is not lc for psg or acqproc or vnmr use structure for acquisition */
/*  test latter for psg or acqproc                                 */
#if !defined( PSG_LC) && !defined(ACQPROC_LC)

/* auto data structure (pointed to by lc) */
struct autod {
	long checkmask;		/* bit mask for coil on/off */
	int when_mask;		/* when to shim mask */
	int control_mask;	/* internal autoshim state */
	int best;		/* reports best vertex */
	int sample_mask;	/* location of sample in tray, */
				/* change sample at begin of experiment, */
				/* if not current sample */
	int sample_error;	/* report sample error # */
	int recgain;		/* receiver gain;lock mode, Z0 field offset */
        int z0;                 /* lock z0 value */
	int lockpower;		/* lock power in db (0-63) */
	int lockgain;		/* lock gain in db (0-70) */
	int lockphase;		/* lock phase 0-360 degrees */
   };

/* low memory allocation  for acquisition  */

  struct lc {		/* word allocation of the "acode" data set */
	long	np;		/* number of points */
	long	nt;		/* number of transients */
	long	ct;		/* completed transients */
	long	isum;		/*imaginary sum from noisecheck*/
	long	rsum;		/*real sum from noisecheck*/
	long	dpts;		/* total data points */
	struct	autod	*autop;	/* pointer to auto structure (set by apint) */
	long	stmar;		/* stm card address register */
	long	stmcr;		/* stm card count register */
        long	rtvptr; 	/* ptr to malloc'ed real time variables   */
	unsigned long	elemid;	/* element number of this code section */
	unsigned long	squi;	/* starting & ending gate pattern  */
	int	idver;		/* diffrent ID's are incompatible */
	int	o2auto;		/* offset to auto structure (set by psg) */
	int	ctctr;		/* ct will be sent to host every ctctr trans.*/
	int	dsize;		/* blocks of data */
	int	asize;		/* blocks of acode/parameters */
	int	codeb;		/* offset within code of first instruction */
	int	codep;		/* offset within code of current instruction */
	int	status;		/* current system status */
	int	dpf;		/* 0=integer, 4=long */
	int	maxscale;	/* max number of shifts allowed */
	int	icmode;		/* input card mode/shift */
	int	stmchk;		/* current stm check/control word */
	int	nflag;		/* current stm control value */
	int	scale;		/* current scale value */
	int	check;		/* stm value to check for overflow */
	int	oph;		/* local value */
	int	bs;		/* block size */
	int	bsct;		/* block size counter */
	int	ss;		/* steady state */
	int	ssct;		/* steady state counter */
	int	ctcom;		/* ct counting backwards */
	int	rfphpt;
	int	curdec;		/* unused */
	int	cpf;		/*cycle phase flag*/
	int	maxconst;	/*adjustment to maxscale*/
	int	tablert;	/* real-time table parameter */
/* the next 12 loc are for hardware status register image */
	int	ocsr;		/* output card status regiter */
	int	apsr;		/* analog port status regiter */
	int	apdr;		/* analog port data regiter */
	int	apar;		/* analog port address/control register */
	int	icsr;		/* input card status regiter */
	int	icdr;		/* input card data register */
	int	icocsr;		/* input card ocsr register */
	int	stmsr;		/* stm card status register */
/*      initialize 500RF hardware */
        int     tpwrr;  	/* transmitter power */
	int     dpwrr;		/* decoupler power */
	int	tphsr;		/* tranmitter phase shift */
	int	dphsr;		/* decoupler phase shift */
	int	dlvlr;		/* decoupler level */
	int	srate;		/* High Speed Rotor Spin Freq */
	int     rttmp; 		/* temp real time parameter for interlock use*/
	int 	spare1; 	/* unused*/
/*	start of user variables */
	int	id2;
	int	id3;
	int	id4;
	int	zero;
	int	one;
	int	two;
	int	three;
/*	allocation for ten user tmp locs v1 - v10 */
	int	v1;
	int	v2;
	int	v3;
	int	v4;
	int	v5;
	int	v6;
	int	v7;
	int	v8;
	int	v9;
	int	v10;
	int	v11;
	int	v12;
	int	v13;
	int	v14;
/*	allocation for extra two user tmp locs */
	int	v_spare[2];
      } ;

#else

/* The following is identical for either psg, acqproc, vnmr        */

/* if acqparms.h in psg is included before lc.h no need to typedef */
#ifndef ACQPARMS
/* --- code type definitions, can be changed for different machines */
typedef char codechar;          /* 1 bytes */
typedef short codeint;          /* 2 bytes */
typedef int  codelong;         /* 4 bytes */
typedef unsigned int  codeulong; /* 4 bytes */
#endif

/* auto data structure (pointed to by lc) */
struct autod {
	codelong checkmask;	/* bit mask for coil on/off */
	codeint when_mask;	/* when to shim mask */
	codeint control_mask;	/* internal autoshim state */
	codeint best;	/* reports best vertex, # loops */
	codeint sample_mask;	/* location of sample in tray, */
				/* change sample at begin of experiment, */
				/* if not current sample */
	codeint sample_error;	/* report sample error # */
	codeint recgain;	/* receiver gain;lock mode, Z0 field offset */
	codeint z0;		/* lock z0 value */
	codeint lockpower;	/* lock power in db (0-63) */
	codeint lockgain;	/* lock gain in db (0-70) */
	codeint lockphase;	/* lock phase 0-360 degrees */
   };


/*  lc for psg?  */
#ifdef PSG_LC

/* low memory allocation  for PSG  */

struct _lc {            /* word allocation of the "acode" data set */
	codelong acqnp;		/* number of points */
	codelong acqnt;		/* number of transients */
	codelong acqct;		/* completed transients */
	codelong acqisum;	/*imaginary sum from noisecheck*/
	codelong acqrsum;	/*real sum from noisecheck*/
	codelong acqdpts;	/* total data points */
//	struct autod *acqautop; /* pointer to auto struct(set by apint) */
	int      acqautop;   /* use int for 64-bit compatibility */
	codelong acqstmar;	/* stm card address register */
	codelong acqstmcr;	/* stm card count register */
        codelong acqrtvptr; 	/* ptr to malloc'ed real time variables   */
	codeulong acqelemid;	/* element number of this code section */
	codeulong acqsqui;	/* starting & ending gate pattern  */
	codeint	acqidver;	/* diffrent ID's are incompatible */
	codeint	acqo2auto;	/* offset to auto structure (set by psg) */
	codeint	acqctctr;	/* ct will be sent to host every ctctr trans.*/
	codeint	acqdsize;	/* blocks of data */
	codeint	acqasize;	/* blocks of acode/parameters */
	codeint	acqcodeb;	/* offset within code of first instruction */
	codeint	acqcodep;	/* offset within code of current instruction */
	codeint	acqstatus;	/* current system status */
	codeint	acqdpf;		/* 0=integer, 4=long */
	codeint	acqmaxscale;	/* max number of shifts allowed */
	codeint	acqicmode;	/* input card mode/shift */
	codeint	acqstmchk;	/* current stm check/control word */
	codeint	acqnflag;	/* current stm control value */
	codeint	acqscale;	/* current scale value */
	codeint	acqcheck;	/* stm value to check for overflow */
	codeint	acqoph;	/* local value */
	codeint	acqbs;		/* block size */
	codeint	acqbsct;	/* block size counter */
	codeint	acqss;		/* steady state */
	codeint	acqssct;	/* steady state counter */
	codeint	acqctcom;	/* ct counting backwards */
	codeint	acqrfphpt;
	codeint	acqcurdec;	/* unused */
	codeint	acqcpf;		/*cycle phase flag*/
	codeint	acqmaxconst;	/*adjustment to maxscale*/
	codeint	acqtablert;	/* real-time table parameter */
/* the next 12 loc are for hardware status register image */
	codeint	acqocsr;	/* output card status regiter */
	codeint	acqapsr;	/* analog port status regiter */
	codeint	acqapdr;	/* analog port data regiter */
	codeint	acqapar;	/* analog port address/control register */
	codeint	acqicsr;	/* input card status regiter */
	codeint	acqicdr;	/* input card data register */
	codeint	acqicocsr;	/* input card ocsr register */
	codeint	acqstmsr;	/* stm card status register */
/*      initialize 500RF hardware */
        codeint acqtpwrr;  	/* transmitter power */
	codeint acqdpwrr;	/* decoupler power */
	codeint	acqtphsr;	/* tranmitter phase shift */
	codeint	acqdphsr;	/* decoupler phase shift */
	codeint	acqdlvlr;	/* decoupler level */
	codeint	acqsrate;	/* High Speed Rotor Spin Freq */
	codeint acqrttmp; 	/* temp real time parameter for interlock use*/
	codeint acqspare1; 	/* unused */
/*	start of user variables */
	codeint	acqid2;
	codeint	acqid3;
	codeint	acqid4;
	codeint	acqzero;
	codeint	acqone;
	codeint	acqtwo;
	codeint	acqthree;
/*	allocation for ten user tmp locs v1 - v10 */
	codeint	acqv1;
	codeint	acqv2;
	codeint	acqv3;
	codeint	acqv4;
	codeint	acqv5;
	codeint	acqv6;
	codeint	acqv7;
	codeint	acqv8;
	codeint	acqv9;
	codeint	acqv10;
	codeint	acqv11;
	codeint	acqv12;
	codeint	acqv13;
	codeint	acqv14;
/*	allocation for extra two user tmp locs */
	codeint	acqv_spare[2];
      } ;
#else
/* low memory allocation  for Acqproc  or Vnmr   supercedes lc_struc.h */

struct _lc {            /* word allocation of the "acode" data set */
	codelong np;		/* number of points */
	codelong nt;		/* number of transients */
	codelong ct;		/* completed transients */
	codelong isum;		/*imaginary sum from noisecheck*/
	codelong rsum;		/*real sum from noisecheck*/
	codelong dpts;		/* total data points */
	struct autod *autop;/* pointer to auto structure (set by apint) */
	codelong stmar;		/* stm card address register */
	codelong stmcr;		/* stm card count register */
        codelong rtvptr; 	/* ptr to malloc'ed real time variables   */
	codeulong elemid;	/* element number of this code section */
	codeulong squi;		/* starting & ending gate pattern  */
	codeint	idver;		/* diffrent ID's are incompatible */
	codeint	o2auto;		/* offset to auto structure (set by psg) */
	codeint	ctctr;		/* ct will be sent to host every ctctr trans.*/
	codeint	dsize;		/* blocks of data */
	codeint	asize;		/* blocks of acode/parameters */
	codeint	codeb;		/* offset within code of first instruction */
	codeint	codep;		/* offset within code of current instruction */
	codeint	status;		/* current system status */
	codeint	dpf;		/* 0=integer, 4=long */
	codeint	maxscale;	/* max number of shifts allowed */
	codeint	icmode;		/* input card mode/shift */
	codeint	stmchk;		/* current stm check/control word */
	codeint	nflag;		/* current stm control value */
	codeint	scale;		/* current scale value */
	codeint	check;		/* stm value to check for overflow */
	codeint	oph;	/* local value */
	codeint	bs;		/* block size */
	codeint	bsct;		/* block size counter */
	codeint	ss;		/* steady state */
	codeint	ssct;		/* steady state counter */
	codeint	ctcom;		/* ct counting backwards */
	codeint	rfphpt;
	codeint	curdec;	/* unused */
	codeint	cpf;		/*cycle phase flag*/
	codeint	maxconst;	/*adjustment to maxscale*/
	codeint	tablert;	/* real-time table parameter */
/* the next 12 loc are for hardware status register image */
	codeint	ocsr;	/* output card status regiter */
	codeint	apsr;	/* analog port status regiter */
	codeint	apdr;	/* analog port data regiter */
	codeint	apar;	/* analog port address/control register */
	codeint	icsr;	/* input card status regiter */
	codeint	icdr;	/* input card data register */
	codeint	icocsr;	/* input card ocsr register */
	codeint	stmsr;	/* stm card status register */
/*      initialize 500RF hardware */
        codeint tpwrr;  /* transmitter power */
	codeint dpwrr;	/* decoupler power */
	codeint	tphsr;	/* tranmitter phase shift */
	codeint	dphsr;	/* decoupler phase shift */
	codeint	dlvlr;	/* decoupler level */
	codeint	srate;	/* High Speed Rotor Spin Freq */
	codeint rttmp;  /* temp real time parameter for interlock use*/
	codeint spare1; /* unused*/
/*	start of user variables */
	codeint	id2;
	codeint	id3;
	codeint	id4;
	codeint	zero;
	codeint	one;
	codeint	two;
	codeint	three;
/*	allocation for ten user tmp locs v1 - v10 */
	codeint	v1;
	codeint	v2;
	codeint	v3;
	codeint	v4;
	codeint	v5;
	codeint	v6;
	codeint	v7;
	codeint	v8;
	codeint	v9;
	codeint	v10;
	codeint	v11;
	codeint	v12;
	codeint	v13;
	codeint	v14;
/*	allocation for extra seven user tmp locs v11 - v14 */
	codeint	v_spare[2];
      } ;
#endif
#endif

/*	end of lc structure	* */
typedef struct autod autodata;
typedef struct _lc Acqparams;
