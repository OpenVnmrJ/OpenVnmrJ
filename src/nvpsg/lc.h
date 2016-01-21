/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* lc.h */

#ifndef INClc_h
#define INClc_h

#define TRUE	1
#define FALSE	0


/* vm02 acquisition parmeter structure (low memory allocation) */
/* Low Core or lc is a historical name and implies nothing now 6/12/87 */

/* since structure element need to have unique names it was neseccary to
   change them for PSG, Acqproc, and Vnmr this is the reason for this
   condition compile statement 	*/

/*  if it is not lc for psg or acqproc or vnmr use structure for acquisition */
/*  test latter for psg or acqproc                                 */

/* auto data structure (pointed to by lc) */
typedef struct autod {
	int checkmask;		/* bit mask for coil on/off */
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
   } autodata ;

/* low memory allocation  for acquisition  */

typedef struct _lc {		/* word allocation of the "acode" data set */
	int	np;		/* number of points */
	int	nt;		/* number of transients */
	int	ct;		/* completed transients */
	int	dpts;		/* total data points */
	struct	autod	*autop;	/* pointer to auto structure (set by apint) */
  int     il_incr;              /* what interleave increment? used by nvacq-il */
  int     il_incrBsCnt;         /* block count increment? used by nvacq-il */
  int     ra_fidnum;            /* which fid to start ??? */
  int     ra_ctnum;             /* what ct to use on ra */

        int	rtvptr; 	/* ptr to malloc'ed real time variables   */
	int	idver;		/* diffrent ID's are incompatible */
	int	o2auto;		/* offset to auto structure (set by psg) */

	int	dpf;		/* 0=integer, 4=long */
	int	oph;		/* local value */
	int	bs;		/* block size */
	int	bsct;		/* block size counter */
	int	ss;		/* steady state */
	int	ssct;		/* steady state counter */
	int	cpf;		/*cycle phase flag*/
	int	cfct;		/*acq only use */

	int	tablert;	/* real-time table parameter */

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

/*	allocation for 32  user tmp locs v1 - v32 */
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
        int     v15;
        int     v16;
        int     v17;
        int     v18;
        int     v19;
        int     v20;

        int     v21;
        int     v22;
        int     v23;
        int     v24;
        int     v25;
        int     v26;
        int     v27;
        int     v28;
        int     v29;
        int     v30;

        int     v31;
        int     v32;
        int     v33;
        int     v34;
        int     v35;
        int     v36;
        int     v37;
        int     v38;
        int     v39;
        int     v40;
        int     v41;
        int     v42;
  /* these are reserved use rtvars */
        int     res_hdec_cntr;   /* used */
        int     res_hdec_lcnt;   /* used */
        int     res_1_internal;  /* used */
        int     res_2_internal;  /* used */
        int     rtonce;          /* used */
        int     res_4_internal;
        int     res_5_internal;
        int     res_6_internal;
        int     res_7_internal;
        int     res_8_internal;
        /* 9 reserved rtvars for imaging IR module */
        int     vslice_ctr;   /* imaging IR module */
        int     vslices;      /* imaging IR module */
        int     virblock;     /* imaging IR module */
        int     vnirpulses;   /* imaging IR module */
        int     vir;          /* imaging IR module */
        int     virslice_ctr; /* imaging IR module */
        int     vnir;         /* imaging IR module */
        int     vnir_ctr;     /* imaging IR module */
        int     vtest;        /* imaging IR module */
        int     vtabskip;     /* imaging peloop var */
        int     vtabskip2;    /* imaging peloop2 var */
      } Acqparams ;

/* the following are V variables used for special purposes */
/* should update */

/* The following is identical for either psg, acqproc, vnmr        */

/* if acqparms.h in psg is included before lc.h no need to typedef */
#ifndef ACQPARMS
/* --- code type definitions, can be changed for different machines */
typedef char codechar;          /* 1 bytes */
/* typedef short int;           2 bytes */
/* typedef long  int;          4 bytes */
typedef unsigned int  codeulong; /* 4 bytes */
#endif

#endif
