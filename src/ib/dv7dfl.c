/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* dv7dfl.f -- translated by f2c (version 20090411).
   You must link the resulting object file with libf2c:
	on Microsoft Windows system, link with libf2c.lib;
	on Linux or Unix systems, link with .../path/to/libf2c.a -lm
	or, if you install libf2c.a in a standard place, with -lf2c -lm
	-- in that order, at the end of the command line, as in
		cc *.o -lf2c -lm
	Source for libf2c is in /netlib/f2c/libf2c.zip, e.g.,

		http://www.netlib.org/f2c/libf2c.zip
*/

#include "f2c.h"

/* Table of constant values */

static integer c__3 = 3;
static integer c__4 = 4;
static doublereal c_b4 = .33333333333333331;
static integer c__5 = 5;

/* Subroutine */ int dv7dfl_(integer *alg, integer *lv, doublereal *v)
{
    /* System generated locals */
    doublereal d__1, d__2, d__3;

    /* Builtin functions */
    double pow_dd(doublereal *, doublereal *);

    /* Local variables */
    extern doublereal dr7mdc_(integer *);
    static doublereal machep, mepcrt, sqteps;


/*  ***  SUPPLY ***SOL (VERSION 2.3) DEFAULT VALUES TO V  *** */

/*  ***  ALG = 1 MEANS REGRESSION CONSTANTS. */
/*  ***  ALG = 2 MEANS GENERAL UNCONSTRAINED OPTIMIZATION CONSTANTS. */


/* DR7MDC... RETURNS MACHINE-DEPENDENT CONSTANTS */


/*  ***  SUBSCRIPTS FOR V  *** */


/* /6 */
/*     DATA ONE/1.D+0/, THREE/3.D+0/ */
/* /7 */
/* / */

/*  ***  V SUBSCRIPT VALUES  *** */

/* /6 */
/*     DATA AFCTOL/31/, BIAS/43/, COSMIN/47/, DECFAC/22/, DELTA0/44/, */
/*    1     DFAC/41/, DINIT/38/, DLTFDC/42/, DLTFDJ/43/, DTINIT/39/, */
/*    2     D0INIT/40/, EPSLON/19/, ETA0/42/, FUZZ/45/, HUBERC/48/, */
/*    3     INCFAC/23/, LMAX0/35/, LMAXS/36/, PHMNFC/20/, PHMXFC/21/, */
/*    4     RDFCMN/24/, RDFCMX/25/, RFCTOL/32/, RLIMIT/46/, RSPTOL/49/, */
/*    5     SCTOL/37/, SIGMIN/50/, TUNER1/26/, TUNER2/27/, TUNER3/28/, */
/*    6     TUNER4/29/, TUNER5/30/, XCTOL/33/, XFTOL/34/ */
/* /7 */
/* / */

/* -------------------------------  BODY  -------------------------------- */

    /* Parameter adjustments */
    --v;

    /* Function Body */
    machep = dr7mdc_(&c__3);
    v[31] = 1e-20;
    if (machep > 1e-10) {
/* Computing 2nd power */
	d__1 = machep;
	v[31] = d__1 * d__1;
    }
    v[22] = .5;
    sqteps = dr7mdc_(&c__4);
    v[41] = .6;
    v[39] = 1e-6;
    mepcrt = pow_dd(&machep, &c_b4);
    v[40] = 1.;
    v[19] = .1;
    v[23] = 2.;
    v[35] = 1.;
    v[36] = 1.;
    v[20] = -.1;
    v[21] = .1;
    v[24] = .1;
    v[25] = 4.;
/* Computing MAX */
/* Computing 2nd power */
    d__3 = mepcrt;
    d__1 = 1e-10, d__2 = d__3 * d__3;
    v[32] = max(d__1,d__2);
    v[37] = v[32];
    v[26] = .1;
    v[27] = 1e-4;
    v[28] = .75;
    v[29] = .5;
    v[30] = .75;
    v[33] = sqteps;
    v[34] = machep * 100.;

    if (*alg >= 2) {
	goto L10;
    }

/*  ***  REGRESSION  VALUES */

/* Computing MAX */
    d__1 = 1e-6, d__2 = machep * 100.;
    v[47] = max(d__1,d__2);
    v[38] = 0.;
    v[44] = sqteps;
    v[42] = mepcrt;
    v[43] = sqteps;
    v[45] = 1.5;
    v[48] = .7;
    v[46] = dr7mdc_(&c__5);
    v[49] = .001;
    v[50] = 1e-4;
    goto L999;

/*  ***  GENERAL OPTIMIZATION VALUES */

L10:
    v[43] = .8;
    v[38] = -1.;
    v[42] = machep * 1e3;

L999:
    return 0;
/*  ***  LAST CARD OF DV7DFL FOLLOWS  *** */
} /* dv7dfl_ */

