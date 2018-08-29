/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* dq7rad.f -- translated by f2c (version 20090411).
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

static integer c__1 = 1;
static integer c__6 = 6;
static integer c__5 = 5;
static integer c__2 = 2;

/* Subroutine */ int dq7rad_(integer *n, integer *nn, integer *p, doublereal *
	qtr, logical *qtrset, doublereal *rmat, doublereal *w, doublereal *y)
{
    /* Initialized data */

    static doublereal big = -1.;
    static doublereal bigrt = -1.;
    static doublereal one = 1.;
    static doublereal tiny = 0.;
    static doublereal tinyrt = 0.;
    static doublereal zero = 0.;

    /* System generated locals */
    integer w_dim1, w_offset, i__1, i__2;
    doublereal d__1;

    /* Builtin functions */
    double sqrt(doublereal);

    /* Local variables */
    static integer i__, j, k;
    static doublereal s, t;
    static integer ii, ij, nk;
    static doublereal ri, wi;
    static integer ip1;
    static doublereal ari, qri;
    extern doublereal dr7mdc_(integer *);
    extern /* Subroutine */ int dv7scl_(integer *, doublereal *, doublereal *,
	     doublereal *);
    extern doublereal dd7tpr_(integer *, doublereal *, doublereal *), dv2nrm_(
	    integer *, doublereal *);
    extern /* Subroutine */ int dv2axy_(integer *, doublereal *, doublereal *,
	     doublereal *, doublereal *);


/*  ***  ADD ROWS W TO QR FACTORIZATION WITH R MATRIX RMAT AND */
/*  ***  Q**T * RESIDUAL = QTR.  Y = NEW COMPONENTS OF RESIDUAL */
/*  ***  CORRESPONDING TO W.  QTR, Y REFERENCED ONLY IF QTRSET = .TRUE. */

/*     DIMENSION RMAT(P*(P+1)/2) */
/* /+ */
/* / */

/*  ***  LOCAL VARIABLES  *** */

/* /7 */
/* / */
    /* Parameter adjustments */
    --y;
    w_dim1 = *nn;
    w_offset = 1 + w_dim1;
    w -= w_offset;
    --qtr;
    --rmat;

    /* Function Body */

/* ------------------------------ BODY ----------------------------------- */

    if (tiny > zero) {
	goto L10;
    }
    tiny = dr7mdc_(&c__1);
    big = dr7mdc_(&c__6);
    if (tiny * big < one) {
	tiny = one / big;
    }
L10:
    k = 1;
    nk = *n;
    ii = 0;
    i__1 = *p;
    for (i__ = 1; i__ <= i__1; ++i__) {
	ii += i__;
	ip1 = i__ + 1;
	ij = ii + i__;
	if (nk <= 1) {
	    t = (d__1 = w[k + i__ * w_dim1], abs(d__1));
	}
	if (nk > 1) {
	    t = dv2nrm_(&nk, &w[k + i__ * w_dim1]);
	}
	if (t < tiny) {
	    goto L180;
	}
	ri = rmat[ii];
	if (ri != zero) {
	    goto L100;
	}
	if (nk > 1) {
	    goto L30;
	}
	ij = ii;
	i__2 = *p;
	for (j = i__; j <= i__2; ++j) {
	    rmat[ij] = w[k + j * w_dim1];
	    ij += j;
/* L20: */
	}
	if (*qtrset) {
	    qtr[i__] = y[k];
	}
	w[k + i__ * w_dim1] = zero;
	goto L999;
L30:
	wi = w[k + i__ * w_dim1];
	if (bigrt > zero) {
	    goto L40;
	}
	bigrt = dr7mdc_(&c__5);
	tinyrt = dr7mdc_(&c__2);
L40:
	if (t <= tinyrt) {
	    goto L50;
	}
	if (t >= bigrt) {
	    goto L50;
	}
	if (wi < zero) {
	    t = -t;
	}
	wi += t;
	s = sqrt(t * wi);
	goto L70;
L50:
	s = sqrt(t);
	if (wi < zero) {
	    goto L60;
	}
	wi += t;
	s *= sqrt(wi);
	goto L70;
L60:
	t = -t;
	wi += t;
	s *= sqrt(-wi);
L70:
	w[k + i__ * w_dim1] = wi;
	d__1 = one / s;
	dv7scl_(&nk, &w[k + i__ * w_dim1], &d__1, &w[k + i__ * w_dim1]);
	rmat[ii] = -t;
	if (! (*qtrset)) {
	    goto L80;
	}
	d__1 = -dd7tpr_(&nk, &y[k], &w[k + i__ * w_dim1]);
	dv2axy_(&nk, &y[k], &d__1, &w[k + i__ * w_dim1], &y[k]);
	qtr[i__] = y[k];
L80:
	if (ip1 > *p) {
	    goto L999;
	}
	i__2 = *p;
	for (j = ip1; j <= i__2; ++j) {
	    d__1 = -dd7tpr_(&nk, &w[k + j * w_dim1], &w[k + i__ * w_dim1]);
	    dv2axy_(&nk, &w[k + j * w_dim1], &d__1, &w[k + i__ * w_dim1], &w[
		    k + j * w_dim1]);
	    rmat[ij] = w[k + j * w_dim1];
	    ij += j;
/* L90: */
	}
	if (nk <= 1) {
	    goto L999;
	}
	++k;
	--nk;
	goto L180;

L100:
	ari = abs(ri);
	if (ari > t) {
	    goto L110;
	}
/* Computing 2nd power */
	d__1 = ari / t;
	t *= sqrt(one + d__1 * d__1);
	goto L120;
L110:
/* Computing 2nd power */
	d__1 = t / ari;
	t = ari * sqrt(one + d__1 * d__1);
L120:
	if (ri < zero) {
	    t = -t;
	}
	ri += t;
	rmat[ii] = -t;
	s = -ri / t;
	if (nk <= 1) {
	    goto L150;
	}
	d__1 = one / ri;
	dv7scl_(&nk, &w[k + i__ * w_dim1], &d__1, &w[k + i__ * w_dim1]);
	if (! (*qtrset)) {
	    goto L130;
	}
	qri = qtr[i__];
	t = s * (qri + dd7tpr_(&nk, &y[k], &w[k + i__ * w_dim1]));
	qtr[i__] = qri + t;
L130:
	if (ip1 > *p) {
	    goto L999;
	}
	if (*qtrset) {
	    dv2axy_(&nk, &y[k], &t, &w[k + i__ * w_dim1], &y[k]);
	}
	i__2 = *p;
	for (j = ip1; j <= i__2; ++j) {
	    ri = rmat[ij];
	    t = s * (ri + dd7tpr_(&nk, &w[k + j * w_dim1], &w[k + i__ * 
		    w_dim1]));
	    dv2axy_(&nk, &w[k + j * w_dim1], &t, &w[k + i__ * w_dim1], &w[k + 
		    j * w_dim1]);
	    rmat[ij] = ri + t;
	    ij += j;
/* L140: */
	}
	goto L180;

L150:
	wi = w[k + i__ * w_dim1] / ri;
	w[k + i__ * w_dim1] = wi;
	if (! (*qtrset)) {
	    goto L160;
	}
	qri = qtr[i__];
	t = s * (qri + y[k] * wi);
	qtr[i__] = qri + t;
L160:
	if (ip1 > *p) {
	    goto L999;
	}
	if (*qtrset) {
	    y[k] = t * wi + y[k];
	}
	i__2 = *p;
	for (j = ip1; j <= i__2; ++j) {
	    ri = rmat[ij];
	    t = s * (ri + w[k + j * w_dim1] * wi);
	    w[k + j * w_dim1] += t * wi;
	    rmat[ij] = ri + t;
	    ij += j;
/* L170: */
	}
L180:
	;
    }

L999:
    return 0;
/*  ***  LAST LINE OF DQ7RAD FOLLOWS  *** */
} /* dq7rad_ */

