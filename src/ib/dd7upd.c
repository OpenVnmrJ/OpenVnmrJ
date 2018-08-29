/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* dd7upd.f -- translated by f2c (version 20090411).
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

static doublereal c_b4 = 0.;

/* Subroutine */ int dd7upd_(doublereal *d__, doublereal *dr, integer *iv, 
	integer *liv, integer *lv, integer *n, integer *nd, integer *nn, 
	integer *n2, integer *p, doublereal *v)
{
    /* System generated locals */
    integer dr_dim1, dr_offset, i__1, i__2;
    doublereal d__1, d__2, d__3;

    /* Builtin functions */
    double sqrt(doublereal);

    /* Local variables */
    static integer i__, k;
    static doublereal t;
    static integer d0, sii, jcn0, jcn1, jcni, jtol0;
    static doublereal vdfac;
    static integer jtoli;
    extern /* Subroutine */ int dv7scp_(integer *, doublereal *, doublereal *)
	    ;


/*  ***  UPDATE SCALE VECTOR D FOR NL2IT  *** */

/*  ***  PARAMETER DECLARATIONS  *** */

/*     DIMENSION V(*) */

/*  ***  LOCAL VARIABLES  *** */


/*     ***  CONSTANTS  *** */


/*  ***  INTRINSIC FUNCTIONS  *** */
/* /+ */
/* / */
/*  ***  EXTERNAL SUBROUTINE  *** */


/* DV7SCP... SETS ALL COMPONENTS OF A VECTOR TO A SCALAR. */

/*  ***  SUBSCRIPTS FOR IV AND V  *** */

/* /6 */
/*     DATA DFAC/41/, DTYPE/16/, JCN/66/, JTOL/59/, NITER/31/, S/62/ */
/* /7 */
/* / */

/* /6 */
/*     DATA ZERO/0.D+0/ */
/* /7 */
/* / */

/* -------------------------------  BODY  -------------------------------- */

    /* Parameter adjustments */
    --iv;
    --v;
    dr_dim1 = *nd;
    dr_offset = 1 + dr_dim1;
    dr -= dr_offset;
    --d__;

    /* Function Body */
    if (iv[16] != 1 && iv[31] > 0) {
	goto L999;
    }
    jcn1 = iv[66];
    jcn0 = abs(jcn1) - 1;
    if (jcn1 < 0) {
	goto L10;
    }
    iv[66] = -jcn1;
    dv7scp_(p, &v[jcn1], &c_b4);
L10:
    i__1 = *p;
    for (i__ = 1; i__ <= i__1; ++i__) {
	jcni = jcn0 + i__;
	t = v[jcni];
	i__2 = *nn;
	for (k = 1; k <= i__2; ++k) {
/* L20: */
/* Computing MAX */
	    d__2 = t, d__3 = (d__1 = dr[k + i__ * dr_dim1], abs(d__1));
	    t = max(d__2,d__3);
	}
	v[jcni] = t;
/* L30: */
    }
    if (*n2 < *n) {
	goto L999;
    }
    vdfac = v[41];
    jtol0 = iv[59] - 1;
    d0 = jtol0 + *p;
    sii = iv[62] - 1;
    i__1 = *p;
    for (i__ = 1; i__ <= i__1; ++i__) {
	sii += i__;
	jcni = jcn0 + i__;
	t = v[jcni];
	if (v[sii] > 0.) {
/* Computing MAX */
	    d__1 = sqrt(v[sii]);
	    t = max(d__1,t);
	}
	jtoli = jtol0 + i__;
	++d0;
	if (t < v[jtoli]) {
/* Computing MAX */
	    d__1 = v[d0], d__2 = v[jtoli];
	    t = max(d__1,d__2);
	}
/* Computing MAX */
	d__1 = vdfac * d__[i__];
	d__[i__] = max(d__1,t);
/* L50: */
    }

L999:
    return 0;
/*  ***  LAST CARD OF DD7UPD FOLLOWS  *** */
} /* dd7upd_ */

