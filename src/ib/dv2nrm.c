/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* dv2nrm.f -- translated by f2c (version 20090411).
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

doublereal dv2nrm_(integer *p, doublereal *x)
{
    /* Initialized data */

    static doublereal sqteta = 0.;

    /* System generated locals */
    integer i__1;
    doublereal ret_val, d__1;

    /* Builtin functions */
    double sqrt(doublereal);

    /* Local variables */
    static integer i__, j;
    static doublereal r__, t, xi, scale;
    extern doublereal dr7mdc_(integer *);


/*  ***  RETURN THE 2-NORM OF THE P-VECTOR X, TAKING  *** */
/*  ***  CARE TO AVOID THE MOST LIKELY UNDERFLOWS.    *** */


/* /+ */
/* / */

/* /6 */
/*     DATA ONE/1.D+0/, ZERO/0.D+0/ */
/* /7 */
/* / */
    /* Parameter adjustments */
    --x;

    /* Function Body */

    if (*p > 0) {
	goto L10;
    }
    ret_val = 0.;
    goto L999;
L10:
    i__1 = *p;
    for (i__ = 1; i__ <= i__1; ++i__) {
	if (x[i__] != 0.) {
	    goto L30;
	}
/* L20: */
    }
    ret_val = 0.;
    goto L999;

L30:
    scale = (d__1 = x[i__], abs(d__1));
    if (i__ < *p) {
	goto L40;
    }
    ret_val = scale;
    goto L999;
L40:
    t = 1.;
    if (sqteta == 0.) {
	sqteta = dr7mdc_(&c__2);
    }

/*     ***  SQTETA IS (SLIGHTLY LARGER THAN) THE SQUARE ROOT OF THE */
/*     ***  SMALLEST POSITIVE FLOATING POINT NUMBER ON THE MACHINE. */
/*     ***  THE TESTS INVOLVING SQTETA ARE DONE TO PREVENT UNDERFLOWS. */

    j = i__ + 1;
    i__1 = *p;
    for (i__ = j; i__ <= i__1; ++i__) {
	xi = (d__1 = x[i__], abs(d__1));
	if (xi > scale) {
	    goto L50;
	}
	r__ = xi / scale;
	if (r__ > sqteta) {
	    t += r__ * r__;
	}
	goto L60;
L50:
	r__ = scale / xi;
	if (r__ <= sqteta) {
	    r__ = 0.;
	}
	t = t * r__ * r__ + 1.;
	scale = xi;
L60:
	;
    }

    ret_val = scale * sqrt(t);
L999:
    return ret_val;
/*  ***  LAST LINE OF DV2NRM FOLLOWS  *** */
} /* dv2nrm_ */

