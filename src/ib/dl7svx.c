/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* dl7svx.f -- translated by f2c (version 20090411).
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

doublereal dl7svx_(integer *p, doublereal *l, doublereal *x, doublereal *y)
{
    /* System generated locals */
    integer i__1, i__2;
    doublereal ret_val, d__1;

    /* Local variables */
    static doublereal b;
    static integer i__, j;
    static doublereal t;
    static integer j0, ji, jj, ix;
    static doublereal yi;
    static integer jm1, pm1, jjj;
    static doublereal blji, splus;
    extern doublereal dd7tpr_(integer *, doublereal *, doublereal *), dv2nrm_(
	    integer *, doublereal *);
    extern /* Subroutine */ int dv2axy_(integer *, doublereal *, doublereal *,
	     doublereal *, doublereal *);
    static integer pplus1;
    static doublereal sminus;


/*  ***  ESTIMATE LARGEST SING. VALUE OF PACKED LOWER TRIANG. MATRIX L */

/*  ***  PARAMETER DECLARATIONS  *** */

/*     DIMENSION L(P*(P+1)/2) */

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

/*  ***  PURPOSE  *** */

/*     THIS FUNCTION RETURNS A GOOD UNDER-ESTIMATE OF THE LARGEST */
/*     SINGULAR VALUE OF THE PACKED LOWER TRIANGULAR MATRIX L. */

/*  ***  PARAMETER DESCRIPTION  *** */

/*  P (IN)  = THE ORDER OF L.  L IS A  P X P  LOWER TRIANGULAR MATRIX. */
/*  L (IN)  = ARRAY HOLDING THE ELEMENTS OF  L  IN ROW ORDER, I.E. */
/*             L(1,1), L(2,1), L(2,2), L(3,1), L(3,2), L(3,3), ETC. */
/*  X (OUT) IF DL7SVX RETURNS A POSITIVE VALUE, THEN X = (L**T)*Y IS AN */
/*             (UNNORMALIZED) APPROXIMATE RIGHT SINGULAR VECTOR */
/*             CORRESPONDING TO THE LARGEST SINGULAR VALUE.  THIS */
/*             APPROXIMATION MAY BE CRUDE. */
/*  Y (OUT) IF DL7SVX RETURNS A POSITIVE VALUE, THEN Y = L*X IS A */
/*             NORMALIZED APPROXIMATE LEFT SINGULAR VECTOR CORRESPOND- */
/*             ING TO THE LARGEST SINGULAR VALUE.  THIS APPROXIMATION */
/*             MAY BE VERY CRUDE.  THE CALLER MAY PASS THE SAME VECTOR */
/*             FOR X AND Y (NONSTANDARD FORTRAN USAGE), IN WHICH CASE X */
/*             OVER-WRITES Y. */

/*  ***  ALGORITHM NOTES  *** */

/*     THE ALGORITHM IS BASED ON ANALOGY WITH (1).  IT USES A */
/*     RANDOM NUMBER GENERATOR PROPOSED IN (4), WHICH PASSES THE */
/*     SPECTRAL TEST WITH FLYING COLORS -- SEE (2) AND (3). */

/*  ***  SUBROUTINES AND FUNCTIONS CALLED  *** */

/*        DV2NRM - FUNCTION, RETURNS THE 2-NORM OF A VECTOR. */

/*  ***  REFERENCES  *** */

/*     (1) CLINE, A., MOLER, C., STEWART, G., AND WILKINSON, J.H.(1977), */
/*         AN ESTIMATE FOR THE CONDITION NUMBER OF A MATRIX, REPORT */
/*         TM-310, APPLIED MATH. DIV., ARGONNE NATIONAL LABORATORY. */

/*     (2) HOAGLIN, D.C. (1976), THEORETICAL PROPERTIES OF CONGRUENTIAL */
/*         RANDOM-NUMBER GENERATORS --  AN EMPIRICAL VIEW, */
/*         MEMORANDUM NS-340, DEPT. OF STATISTICS, HARVARD UNIV. */

/*     (3) KNUTH, D.E. (1969), THE ART OF COMPUTER PROGRAMMING, VOL. 2 */
/*         (SEMINUMERICAL ALGORITHMS), ADDISON-WESLEY, READING, MASS. */

/*     (4) SMITH, C.S. (1971), MULTIPLICATIVE PSEUDO-RANDOM NUMBER */
/*         GENERATORS WITH PRIME MODULUS, J. ASSOC. COMPUT. MACH. 18, */
/*         PP. 586-593. */

/*  ***  HISTORY  *** */

/*     DESIGNED AND CODED BY DAVID M. GAY (WINTER 1977/SUMMER 1978). */

/*  ***  GENERAL  *** */

/*     THIS SUBROUTINE WAS WRITTEN IN CONNECTION WITH RESEARCH */
/*     SUPPORTED BY THE NATIONAL SCIENCE FOUNDATION UNDER GRANTS */
/*     MCS-7600324, DCR75-10143, 76-14311DSS, AND MCS76-11989. */

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

/*  ***  LOCAL VARIABLES  *** */


/*  ***  CONSTANTS  *** */


/*  ***  EXTERNAL FUNCTIONS AND SUBROUTINES  *** */


/* /6 */
/*     DATA HALF/0.5D+0/, ONE/1.D+0/, R9973/9973.D+0/, ZERO/0.D+0/ */
/* /7 */
/* / */

/*  ***  BODY  *** */

    /* Parameter adjustments */
    --y;
    --x;
    --l;

    /* Function Body */
    ix = 2;
    pplus1 = *p + 1;
    pm1 = *p - 1;

/*  ***  FIRST INITIALIZE X TO PARTIAL SUMS  *** */

    j0 = *p * pm1 / 2;
    jj = j0 + *p;
    ix = ix * 3432 % 9973;
    b = ((real) ix / 9973. + 1.) * .5;
    x[*p] = b * l[jj];
    if (*p <= 1) {
	goto L40;
    }
    i__1 = pm1;
    for (i__ = 1; i__ <= i__1; ++i__) {
	ji = j0 + i__;
	x[i__] = b * l[ji];
/* L10: */
    }

/*  ***  COMPUTE X = (L**T)*B, WHERE THE COMPONENTS OF B HAVE RANDOMLY */
/*  ***  CHOSEN MAGNITUDES IN (.5,1) WITH SIGNS CHOSEN TO MAKE X LARGE. */

/*     DO J = P-1 TO 1 BY -1... */
    i__1 = pm1;
    for (jjj = 1; jjj <= i__1; ++jjj) {
	j = *p - jjj;
/*       ***  DETERMINE X(J) IN THIS ITERATION. NOTE FOR I = 1,2,...,J */
/*       ***  THAT X(I) HOLDS THE CURRENT PARTIAL SUM FOR ROW I. */
	ix = ix * 3432 % 9973;
	b = ((real) ix / 9973. + 1.) * .5;
	jm1 = j - 1;
	j0 = j * jm1 / 2;
	splus = 0.;
	sminus = 0.;
	i__2 = j;
	for (i__ = 1; i__ <= i__2; ++i__) {
	    ji = j0 + i__;
	    blji = b * l[ji];
	    splus += (d__1 = blji + x[i__], abs(d__1));
	    sminus += (d__1 = blji - x[i__], abs(d__1));
/* L20: */
	}
	if (sminus > splus) {
	    b = -b;
	}
	x[j] = 0.;
/*        ***  UPDATE PARTIAL SUMS  *** */
	dv2axy_(&j, &x[1], &b, &l[j0 + 1], &x[1]);
/* L30: */
    }

/*  ***  NORMALIZE X  *** */

L40:
    t = dv2nrm_(p, &x[1]);
    if (t <= 0.) {
	goto L80;
    }
    t = 1. / t;
    i__1 = *p;
    for (i__ = 1; i__ <= i__1; ++i__) {
/* L50: */
	x[i__] = t * x[i__];
    }

/*  ***  COMPUTE L*X = Y AND RETURN SVMAX = TWONORM(Y)  *** */

    i__1 = *p;
    for (jjj = 1; jjj <= i__1; ++jjj) {
	j = pplus1 - jjj;
	ji = j * (j - 1) / 2 + 1;
	y[j] = dd7tpr_(&j, &l[ji], &x[1]);
/* L60: */
    }

/*  ***  NORMALIZE Y AND SET X = (L**T)*Y  *** */

    t = 1. / dv2nrm_(p, &y[1]);
    ji = 1;
    i__1 = *p;
    for (i__ = 1; i__ <= i__1; ++i__) {
	yi = t * y[i__];
	x[i__] = 0.;
	dv2axy_(&i__, &x[1], &yi, &l[ji], &x[1]);
	ji += i__;
/* L70: */
    }
    ret_val = dv2nrm_(p, &x[1]);
    goto L999;

L80:
    ret_val = 0.;

L999:
    return ret_val;
/*  ***  LAST CARD OF DL7SVX FOLLOWS  *** */
} /* dl7svx_ */

