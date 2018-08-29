/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* dq7apl.f -- translated by f2c (version 20090411).
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

/* Subroutine */ int dq7apl_(integer *nn, integer *n, integer *p, doublereal *
	j, doublereal *r__, integer *ierr)
{
    /* System generated locals */
    integer j_dim1, j_offset, i__1;
    doublereal d__1;

    /* Local variables */
    static integer k, l, nl1;
    extern doublereal dd7tpr_(integer *, doublereal *, doublereal *);
    extern /* Subroutine */ int dv2axy_(integer *, doublereal *, doublereal *,
	     doublereal *, doublereal *);

/*     *****PARAMETERS. */

/*     .................................................................. */
/*     .................................................................. */

/*     *****PURPOSE. */
/*     THIS SUBROUTINE APPLIES TO R THE ORTHOGONAL TRANSFORMATIONS */
/*     STORED IN J BY QRFACT */

/*     *****PARAMETER DESCRIPTION. */
/*     ON INPUT. */

/*        NN IS THE ROW DIMENSION OF THE MATRIX J AS DECLARED IN */
/*             THE CALLING PROGRAM DIMENSION STATEMENT */

/*        N IS THE NUMBER OF ROWS OF J AND THE SIZE OF THE VECTOR R */

/*        P IS THE NUMBER OF COLUMNS OF J AND THE SIZE OF SIGMA */

/*        J CONTAINS ON AND BELOW ITS DIAGONAL THE COLUMN VECTORS */
/*             U WHICH DETERMINE THE HOUSEHOLDER TRANSFORMATIONS */
/*             IDENT - U*U.TRANSPOSE */

/*        R IS THE RIGHT HAND SIDE VECTOR TO WHICH THE ORTHOGONAL */
/*             TRANSFORMATIONS WILL BE APPLIED */

/*        IERR IF NON-ZERO INDICATES THAT NOT ALL THE TRANSFORMATIONS */
/*             WERE SUCCESSFULLY DETERMINED AND ONLY THE FIRST */
/*             ABS(IERR) - 1 TRANSFORMATIONS WILL BE USED */

/*     ON OUTPUT. */

/*        R HAS BEEN OVERWRITTEN BY ITS TRANSFORMED IMAGE */

/*     *****APPLICATION AND USAGE RESTRICTIONS. */
/*     NONE */

/*     *****ALGORITHM NOTES. */
/*     THE VECTORS U WHICH DETERMINE THE HOUSEHOLDER TRANSFORMATIONS */
/*     ARE NORMALIZED SO THAT THEIR 2-NORM SQUARED IS 2.  THE USE OF */
/*     THESE TRANSFORMATIONS HERE IS IN THE SPIRIT OF (1). */

/*     *****SUBROUTINES AND FUNCTIONS CALLED. */

/*     DD7TPR - FUNCTION, RETURNS THE INNER PRODUCT OF VECTORS */

/*     *****REFERENCES. */
/*     (1) BUSINGER, P. A., AND GOLUB, G. H. (1965), LINEAR LEAST SQUARES */
/*        SOLUTIONS BY HOUSEHOLDER TRANSFORMATIONS, NUMER. MATH. 7, */
/*        PP. 269-276. */

/*     *****HISTORY. */
/*     DESIGNED BY DAVID M. GAY, CODED BY STEPHEN C. PETERS (WINTER 1977) */
/*     CALL ON DV2AXY SUBSTITUTED FOR DO LOOP, FALL 1983. */

/*     *****GENERAL. */

/*     THIS SUBROUTINE WAS WRITTEN IN CONNECTION WITH RESEARCH */
/*     SUPPORTED BY THE NATIONAL SCIENCE FOUNDATION UNDER GRANTS */
/*     MCS-7600324, DCR75-10143, 76-14311DSS, AND MCS76-11989. */

/*     .................................................................. */
/*     .................................................................. */

/*     *****LOCAL VARIABLES. */
/*     *****FUNCTIONS. */

/*  ***  BODY  *** */

    /* Parameter adjustments */
    --r__;
    j_dim1 = *nn;
    j_offset = 1 + j_dim1;
    j -= j_offset;

    /* Function Body */
    k = *p;
    if (*ierr != 0) {
	k = abs(*ierr) - 1;
    }
    if (k == 0) {
	goto L999;
    }

    i__1 = k;
    for (l = 1; l <= i__1; ++l) {
	nl1 = *n - l + 1;
	d__1 = -dd7tpr_(&nl1, &j[l + l * j_dim1], &r__[l]);
	dv2axy_(&nl1, &r__[l], &d__1, &j[l + l * j_dim1], &r__[l]);
/* L20: */
    }

L999:
    return 0;
/*  ***  LAST LINE OF DQ7APL FOLLOWS  *** */
} /* dq7apl_ */

