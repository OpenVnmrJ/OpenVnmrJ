/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* ds7lvm.f -- translated by f2c (version 20090411).
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

/* Subroutine */ int ds7lvm_(integer *p, doublereal *y, doublereal *s, 
	doublereal *x)
{
    /* System generated locals */
    integer i__1, i__2;

    /* Local variables */
    static integer i__, j, k;
    static doublereal xi;
    static integer im1;
    extern doublereal dd7tpr_(integer *, doublereal *, doublereal *);


/*  ***  SET  Y = S * X,  S = P X P SYMMETRIC MATRIX.  *** */
/*  ***  LOWER TRIANGLE OF  S  STORED ROWWISE.         *** */

/*  ***  PARAMETER DECLARATIONS  *** */

/*     DIMENSION S(P*(P+1)/2) */

/*  ***  LOCAL VARIABLES  *** */


/*  ***  NO INTRINSIC FUNCTIONS  *** */

/*  ***  EXTERNAL FUNCTION  *** */


/* ----------------------------------------------------------------------- */

    /* Parameter adjustments */
    --x;
    --y;
    --s;

    /* Function Body */
    j = 1;
    i__1 = *p;
    for (i__ = 1; i__ <= i__1; ++i__) {
	y[i__] = dd7tpr_(&i__, &s[j], &x[1]);
	j += i__;
/* L10: */
    }

    if (*p <= 1) {
	goto L999;
    }
    j = 1;
    i__1 = *p;
    for (i__ = 2; i__ <= i__1; ++i__) {
	xi = x[i__];
	im1 = i__ - 1;
	++j;
	i__2 = im1;
	for (k = 1; k <= i__2; ++k) {
	    y[k] += s[j] * xi;
	    ++j;
/* L30: */
	}
/* L40: */
    }

L999:
    return 0;
/*  ***  LAST CARD OF DS7LVM FOLLOWS  *** */
} /* ds7lvm_ */

