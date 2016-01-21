/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* dl7sqr.f -- translated by f2c (version 20090411).
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

/* Subroutine */ int dl7sqr_(integer *n, doublereal *a, doublereal *l)
{
    /* System generated locals */
    integer i__1, i__2, i__3;

    /* Local variables */
    static integer i__, j, k;
    static doublereal t;
    static integer i0, j0, ii, ij, ik, jj, jk, ip1, np1;


/*  ***  COMPUTE  A = LOWER TRIANGLE OF  L*(L**T),  WITH BOTH */
/*  ***  L  AND  A  STORED COMPACTLY BY ROWS.  (BOTH MAY OCCUPY THE */
/*  ***  SAME STORAGE. */

/*  ***  PARAMETERS  *** */

/*     DIMENSION A(N*(N+1)/2), L(N*(N+1)/2) */

/*  ***  LOCAL VARIABLES  *** */


    /* Parameter adjustments */
    --l;
    --a;

    /* Function Body */
    np1 = *n + 1;
    i0 = *n * (*n + 1) / 2;
    i__1 = *n;
    for (ii = 1; ii <= i__1; ++ii) {
	i__ = np1 - ii;
	ip1 = i__ + 1;
	i0 -= i__;
	j0 = i__ * (i__ + 1) / 2;
	i__2 = i__;
	for (jj = 1; jj <= i__2; ++jj) {
	    j = ip1 - jj;
	    j0 -= j;
	    t = 0.;
	    i__3 = j;
	    for (k = 1; k <= i__3; ++k) {
		ik = i0 + k;
		jk = j0 + k;
		t += l[ik] * l[jk];
/* L10: */
	    }
	    ij = i0 + j;
	    a[ij] = t;
/* L20: */
	}
/* L30: */
    }
/* L999: */
    return 0;
} /* dl7sqr_ */

