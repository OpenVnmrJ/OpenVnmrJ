/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* divset.f -- translated by f2c (version 20090411).
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

static integer c__1 = 1;

/* Subroutine */ int divset_(integer *alg, integer *iv, integer *liv, integer 
	*lv, doublereal *v)
{
    /* Initialized data */

    static integer miniv[4] = { 82,59,103,103 };
    static integer minv[4] = { 98,71,101,85 };

    static integer mv, miv, alg1;
    extern integer i7mdcn_(integer *);
    extern /* Subroutine */ int dv7dfl_(integer *, integer *, doublereal *);


/*  ***  SUPPLY ***SOL (VERSION 2.3) DEFAULT VALUES TO IV AND V  *** */

/*  ***  ALG = 1 MEANS REGRESSION CONSTANTS. */
/*  ***  ALG = 2 MEANS GENERAL UNCONSTRAINED OPTIMIZATION CONSTANTS. */


/* I7MDCN... RETURNS MACHINE-DEPENDENT INTEGER CONSTANTS. */
/* DV7DFL.... PROVIDES DEFAULT VALUES TO V. */


/*  ***  SUBSCRIPTS FOR IV  *** */


/*  ***  IV SUBSCRIPT VALUES  *** */

/* /6 */
/*     DATA ALGSAV/51/, COVPRT/14/, COVREQ/15/, DRADPR/101/, DTYPE/16/, */
/*    1     HC/71/, IERR/75/, INITH/25/, INITS/25/, IPIVOT/76/, */
/*    2     IVNEED/3/, LASTIV/44/, LASTV/45/, LMAT/42/, MXFCAL/17/, */
/*    3     MXITER/18/, NFCOV/52/, NGCOV/53/, NVDFLT/50/, NVSAVE/9/, */
/*    4     OUTLEV/19/, PARPRT/20/, PARSAV/49/, PERM/58/, PRUNIT/21/, */
/*    5     QRTYP/80/, RDREQ/57/, RMAT/78/, SOLPRT/22/, STATPR/23/, */
/*    6     VNEED/4/, VSAVE/60/, X0PRT/24/ */
/* /7 */
/* / */
    /* Parameter adjustments */
    --iv;
    --v;

    /* Function Body */

/* -------------------------------  BODY  -------------------------------- */

    if (21 <= *liv) {
	iv[21] = i7mdcn_(&c__1);
    }
    if (51 <= *liv) {
	iv[51] = *alg;
    }
    if (*alg < 1 || *alg > 4) {
	goto L40;
    }
    miv = miniv[*alg - 1];
    if (*liv < miv) {
	goto L20;
    }
    mv = minv[*alg - 1];
    if (*lv < mv) {
	goto L30;
    }
    alg1 = (*alg - 1) % 2 + 1;
    dv7dfl_(&alg1, lv, &v[1]);
    iv[1] = 12;
    if (*alg > 2) {
	iv[101] = 1;
    }
    iv[3] = 0;
    iv[44] = miv;
    iv[45] = mv;
    iv[42] = mv + 1;
    iv[17] = 200;
    iv[18] = 150;
    iv[19] = 1;
    iv[20] = 1;
    iv[58] = miv + 1;
    iv[22] = 1;
    iv[23] = 1;
    iv[4] = 0;
    iv[24] = 1;

    if (alg1 >= 2) {
	goto L10;
    }

/*  ***  REGRESSION  VALUES */

    iv[14] = 3;
    iv[15] = 1;
    iv[16] = 1;
    iv[71] = 0;
    iv[75] = 0;
    iv[25] = 0;
    iv[76] = 0;
    iv[50] = 32;
    iv[60] = 58;
    if (*alg > 2) {
	iv[60] += 3;
    }
    iv[49] = iv[60] + 9;
    iv[80] = 1;
    iv[57] = 3;
    iv[78] = 0;
    goto L999;

/*  ***  GENERAL OPTIMIZATION VALUES */

L10:
    iv[16] = 0;
    iv[25] = 1;
    iv[52] = 0;
    iv[53] = 0;
    iv[50] = 25;
    iv[49] = 47;
    if (*alg > 2) {
	iv[49] = 61;
    }
    goto L999;

L20:
    iv[1] = 15;
    goto L999;

L30:
    iv[1] = 16;
    goto L999;

L40:
    iv[1] = 67;

L999:
    return 0;
/*  ***  LAST CARD OF DIVSET FOLLOWS  *** */
} /* divset_ */

