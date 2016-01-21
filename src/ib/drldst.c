/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* drldst.f -- translated by f2c (version 20090411).
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

doublereal drldst_(integer *p, doublereal *d__, doublereal *x, doublereal *x0)
{
    /* System generated locals */
    integer i__1;
    doublereal ret_val, d__1, d__2;

    /* Local variables */
    static integer i__;
    static doublereal t, emax, xmax;


/*  ***  COMPUTE AND RETURN RELATIVE DIFFERENCE BETWEEN X AND X0  *** */
/*  ***  NL2SOL VERSION 2.2  *** */


/* /6 */
/*     DATA ZERO/0.D+0/ */
/* /7 */
/* / */

/*  ***  BODY  *** */

    /* Parameter adjustments */
    --x0;
    --x;
    --d__;

    /* Function Body */
    emax = 0.;
    xmax = 0.;
    i__1 = *p;
    for (i__ = 1; i__ <= i__1; ++i__) {
	t = (d__1 = d__[i__] * (x[i__] - x0[i__]), abs(d__1));
	if (emax < t) {
	    emax = t;
	}
	t = d__[i__] * ((d__1 = x[i__], abs(d__1)) + (d__2 = x0[i__], abs(
		d__2)));
	if (xmax < t) {
	    xmax = t;
	}
/* L10: */
    }
    ret_val = 0.;
    if (xmax > 0.) {
	ret_val = emax / xmax;
    }
/* L999: */
    return ret_val;
/*  ***  LAST CARD OF DRLDST FOLLOWS  *** */
} /* drldst_ */

