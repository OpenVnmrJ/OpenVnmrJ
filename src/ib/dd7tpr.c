/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* dd7tpr.f -- translated by f2c (version 20090411).
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

static integer c__2 = 2;

doublereal dd7tpr_(integer *p, doublereal *x, doublereal *y)
{
    /* Initialized data */

    static doublereal sqteta = 0.;

    /* System generated locals */
    integer i__1;
    doublereal ret_val, d__1, d__2, d__3, d__4;

    /* Local variables */
    static integer i__;
    static doublereal t;
    extern doublereal dr7mdc_(integer *);


/*  ***  RETURN THE INNER PRODUCT OF THE P-VECTORS X AND Y.  *** */



/*  ***  DR7MDC(2) RETURNS A MACHINE-DEPENDENT CONSTANT, SQTETA, WHICH */
/*  ***  IS SLIGHTLY LARGER THAN THE SMALLEST POSITIVE NUMBER THAT */
/*  ***  CAN BE SQUARED WITHOUT UNDERFLOWING. */

/* /6 */
/*     DATA ONE/1.D+0/, SQTETA/0.D+0/, ZERO/0.D+0/ */
/* /7 */
    /* Parameter adjustments */
    --y;
    --x;

    /* Function Body */
/* / */

    ret_val = 0.;
    if (*p <= 0) {
	goto L999;
    }
    if (sqteta == 0.) {
	sqteta = dr7mdc_(&c__2);
    }
    i__1 = *p;
    for (i__ = 1; i__ <= i__1; ++i__) {
/* Computing MAX */
	d__3 = (d__1 = x[i__], abs(d__1)), d__4 = (d__2 = y[i__], abs(d__2));
	t = max(d__3,d__4);
	if (t > 1.) {
	    goto L10;
	}
	if (t < sqteta) {
	    goto L20;
	}
	t = x[i__] / sqteta * y[i__];
	if (abs(t) < sqteta) {
	    goto L20;
	}
L10:
	ret_val += x[i__] * y[i__];
L20:
	;
    }

L999:
    return ret_val;
/*  ***  LAST LINE OF DD7TPR FOLLOWS  *** */
} /* dd7tpr_ */

