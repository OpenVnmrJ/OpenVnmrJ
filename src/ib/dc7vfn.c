/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* dc7vfn.f -- translated by f2c (version 20090411).
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

/* Subroutine */ int dc7vfn_(integer *iv, doublereal *l, integer *lh, integer 
	*liv, integer *lv, integer *n, integer *p, doublereal *v)
{
    /* Initialized data */

    static doublereal half = .5;

    /* System generated locals */
    integer i__1, i__2;
    doublereal d__1;

    /* Local variables */
    static integer i__, cov;
    extern /* Subroutine */ int dv7scl_(integer *, doublereal *, doublereal *,
	     doublereal *), dl7nvr_(integer *, doublereal *, doublereal *), 
	    dl7tsq_(integer *, doublereal *, doublereal *);


/*  ***  FINISH COVARIANCE COMPUTATION FOR  DRN2G,  DRNSG  *** */



/*  ***  LOCAL VARIABLES  *** */


/*  ***  SUBSCRIPTS FOR IV AND V  *** */


/* /6 */
/*     DATA CNVCOD/55/, COVMAT/26/, F/10/, FDH/74/, H/56/, MODE/35/, */
/*    1     RDREQ/57/, REGD/67/ */
/* /7 */
/* / */
    /* Parameter adjustments */
    --l;
    --iv;
    --v;

    /* Function Body */

/*  ***  BODY  *** */

    iv[1] = iv[55];
    i__ = iv[35] - *p;
    iv[35] = 0;
    iv[55] = 0;
    if (iv[74] <= 0) {
	goto L999;
    }
/* Computing 2nd power */
    i__1 = i__ - 2;
    if (i__1 * i__1 == 1) {
	iv[67] = 1;
    }
    if (iv[57] % 2 != 1) {
	goto L999;
    }

/*     ***  FINISH COMPUTING COVARIANCE MATRIX = INVERSE OF F.D. HESSIAN. */

    cov = abs(iv[56]);
    iv[74] = 0;

    if (iv[26] != 0) {
	goto L999;
    }
    if (i__ >= 2) {
	goto L10;
    }
    dl7nvr_(p, &v[cov], &l[1]);
    dl7tsq_(p, &v[cov], &v[cov]);

L10:
/* Computing MAX */
    i__1 = 1, i__2 = *n - *p;
    d__1 = v[10] / (half * (real) max(i__1,i__2));
    dv7scl_(lh, &v[cov], &d__1, &v[cov]);
    iv[26] = cov;

L999:
    return 0;
/*  ***  LAST LINE OF DC7VFN FOLLOWS  *** */
} /* dc7vfn_ */

