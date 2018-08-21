/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* do7prd.f -- translated by f2c (version 20090411).
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

/* Subroutine */ int do7prd_(integer *l, integer *ls, integer *p, doublereal *
	s, doublereal *w, doublereal *y, doublereal *z__)
{
    /* Initialized data */

    static doublereal zero = 0.;

    /* System generated locals */
    integer y_dim1, y_offset, z_dim1, z_offset, i__1, i__2, i__3;

    /* Local variables */
    static integer i__, j, k, m;
    static doublereal wk, yi;


/*  ***  FOR I = 1..L, SET S = S + W(I)*Y(.,I)*(Z(.,I)**T), I.E., */
/*  ***        ADD W(I) TIMES THE OUTER PRODUCT OF Y(.,I) AND Z(.,I). */

/*     DIMENSION S(P*(P+1)/2) */

    /* Parameter adjustments */
    --w;
    --s;
    z_dim1 = *p;
    z_offset = 1 + z_dim1;
    z__ -= z_offset;
    y_dim1 = *p;
    y_offset = 1 + y_dim1;
    y -= y_offset;

    /* Function Body */

    i__1 = *l;
    for (k = 1; k <= i__1; ++k) {
	wk = w[k];
	if (wk == zero) {
	    goto L30;
	}
	m = 1;
	i__2 = *p;
	for (i__ = 1; i__ <= i__2; ++i__) {
	    yi = wk * y[i__ + k * y_dim1];
	    i__3 = i__;
	    for (j = 1; j <= i__3; ++j) {
		s[m] += yi * z__[j + k * z_dim1];
		++m;
/* L10: */
	    }
/* L20: */
	}
L30:
	;
    }

/* L999: */
    return 0;
/*  ***  LAST CARD OF DO7PRD FOLLOWS  *** */
} /* do7prd_ */

