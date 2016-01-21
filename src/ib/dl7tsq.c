/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* dl7tsq.f -- translated by f2c (version 20090411).
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

/* Subroutine */ int dl7tsq_(integer *n, doublereal *a, doublereal *l)
{
    /* System generated locals */
    integer i__1, i__2, i__3;

    /* Local variables */
    static integer i__, j, k, m, i1, ii;
    static doublereal lj, lii;
    static integer iim1;


/*  ***  SET A TO LOWER TRIANGLE OF (L**T) * L  *** */

/*  ***  L = N X N LOWER TRIANG. MATRIX STORED ROWWISE.  *** */
/*  ***  A IS ALSO STORED ROWWISE AND MAY SHARE STORAGE WITH L.  *** */

/*     DIMENSION A(N*(N+1)/2), L(N*(N+1)/2) */


    /* Parameter adjustments */
    --l;
    --a;

    /* Function Body */
    ii = 0;
    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
	i1 = ii + 1;
	ii += i__;
	m = 1;
	if (i__ == 1) {
	    goto L30;
	}
	iim1 = ii - 1;
	i__2 = iim1;
	for (j = i1; j <= i__2; ++j) {
	    lj = l[j];
	    i__3 = j;
	    for (k = i1; k <= i__3; ++k) {
		a[m] += lj * l[k];
		++m;
/* L10: */
	    }
/* L20: */
	}
L30:
	lii = l[ii];
	i__2 = ii;
	for (j = i1; j <= i__2; ++j) {
/* L40: */
	    a[j] = lii * l[j];
	}
/* L50: */
    }

/* L999: */
    return 0;
/*  ***  LAST CARD OF DL7TSQ FOLLOWS  *** */
} /* dl7tsq_ */

