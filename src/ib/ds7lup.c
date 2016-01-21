/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* ds7lup.f -- translated by f2c (version 20090411).
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

/* Subroutine */ int ds7lup_(doublereal *a, doublereal *cosmin, integer *p, 
	doublereal *size, doublereal *step, doublereal *u, doublereal *w, 
	doublereal *wchmtd, doublereal *wscale, doublereal *y)
{
    /* System generated locals */
    integer i__1, i__2;
    doublereal d__1, d__2, d__3;

    /* Local variables */
    static integer i__, j, k;
    static doublereal t, ui, wi;
    extern doublereal dd7tpr_(integer *, doublereal *, doublereal *), dv2nrm_(
	    integer *, doublereal *);
    extern /* Subroutine */ int ds7lvm_(integer *, doublereal *, doublereal *,
	     doublereal *);
    static doublereal denmin, sdotwm;


/*  ***  UPDATE SYMMETRIC  A  SO THAT  A * STEP = Y  *** */
/*  ***  (LOWER TRIANGLE OF  A  STORED ROWWISE       *** */

/*  ***  PARAMETER DECLARATIONS  *** */

/*     DIMENSION A(P*(P+1)/2) */

/*  ***  LOCAL VARIABLES  *** */


/*     ***  CONSTANTS  *** */

/*  ***  EXTERNAL FUNCTIONS AND SUBROUTINES  *** */


/* /6 */
/*     DATA HALF/0.5D+0/, ONE/1.D+0/, ZERO/0.D+0/ */
/* /7 */
/* / */

/* ----------------------------------------------------------------------- */

    /* Parameter adjustments */
    --a;
    --y;
    --wchmtd;
    --w;
    --u;
    --step;

    /* Function Body */
    sdotwm = dd7tpr_(p, &step[1], &wchmtd[1]);
    denmin = *cosmin * dv2nrm_(p, &step[1]) * dv2nrm_(p, &wchmtd[1]);
    *wscale = 1.;
    if (denmin != 0.) {
/* Computing MIN */
	d__2 = 1., d__3 = (d__1 = sdotwm / denmin, abs(d__1));
	*wscale = min(d__2,d__3);
    }
    t = 0.;
    if (sdotwm != 0.) {
	t = *wscale / sdotwm;
    }
    i__1 = *p;
    for (i__ = 1; i__ <= i__1; ++i__) {
/* L10: */
	w[i__] = t * wchmtd[i__];
    }
    ds7lvm_(p, &u[1], &a[1], &step[1]);
    t = (*size * dd7tpr_(p, &step[1], &u[1]) - dd7tpr_(p, &step[1], &y[1])) * 
	    .5;
    i__1 = *p;
    for (i__ = 1; i__ <= i__1; ++i__) {
/* L20: */
	u[i__] = t * w[i__] + y[i__] - *size * u[i__];
    }

/*  ***  SET  A = A + U*(W**T) + W*(U**T)  *** */

    k = 1;
    i__1 = *p;
    for (i__ = 1; i__ <= i__1; ++i__) {
	ui = u[i__];
	wi = w[i__];
	i__2 = i__;
	for (j = 1; j <= i__2; ++j) {
	    a[k] = *size * a[k] + ui * w[j] + wi * u[j];
	    ++k;
/* L30: */
	}
/* L40: */
    }

/* L999: */
    return 0;
/*  ***  LAST CARD OF DS7LUP FOLLOWS  *** */
} /* ds7lup_ */

