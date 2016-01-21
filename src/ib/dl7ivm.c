/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* dl7ivm.f -- translated by f2c (version 20090411).
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

/* Subroutine */ int dl7ivm_(integer *n, doublereal *x, doublereal *l, 
	doublereal *y)
{
    /* System generated locals */
    integer i__1, i__2;

    /* Local variables */
    static integer i__, j, k;
    static doublereal t;
    extern doublereal dd7tpr_(integer *, doublereal *, doublereal *);


/*  ***  SOLVE  L*X = Y, WHERE  L  IS AN  N X N  LOWER TRIANGULAR */
/*  ***  MATRIX STORED COMPACTLY BY ROWS.  X AND Y MAY OCCUPY THE SAME */
/*  ***  STORAGE.  *** */

/* /6 */
/*     DATA ZERO/0.D+0/ */
/* /7 */
/* / */

    /* Parameter adjustments */
    --y;
    --x;
    --l;

    /* Function Body */
    i__1 = *n;
    for (k = 1; k <= i__1; ++k) {
	if (y[k] != 0.) {
	    goto L20;
	}
	x[k] = 0.;
/* L10: */
    }
    goto L999;
L20:
    j = k * (k + 1) / 2;
    x[k] = y[k] / l[j];
    if (k >= *n) {
	goto L999;
    }
    ++k;
    i__1 = *n;
    for (i__ = k; i__ <= i__1; ++i__) {
	i__2 = i__ - 1;
	t = dd7tpr_(&i__2, &l[j + 1], &x[1]);
	j += i__;
	x[i__] = (y[i__] - t) / l[j];
/* L30: */
    }
L999:
    return 0;
/*  ***  LAST CARD OF DL7IVM FOLLOWS  *** */
} /* dl7ivm_ */

