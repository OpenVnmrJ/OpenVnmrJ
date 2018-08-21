/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* df7hes.f -- translated by f2c (version 20090411).
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

/* Subroutine */ int df7hes_(doublereal *d__, doublereal *g, integer *irt, 
	integer *iv, integer *liv, integer *lv, integer *p, doublereal *v, 
	doublereal *x)
{
    /* System generated locals */
    integer i__1;
    doublereal d__1, d__2, d__3;

    /* Local variables */
    static integer i__, k, l, m, mm1;
    static doublereal del;
    static integer hmi, hes, hpi, hpm, stp0, kind, mm1o2, pp1o2, stpi, stpm, 
	    gsave1;
    extern /* Subroutine */ int dv7cpy_(integer *, doublereal *, doublereal *)
	    ;


/*  ***  COMPUTE FINITE-DIFFERENCE HESSIAN, STORE IT IN V STARTING */
/*  ***  AT V(IV(FDH)) = V(-IV(H)). */

/*  ***  IF IV(COVREQ) .GE. 0 THEN DF7HES USES GRADIENT DIFFERENCES, */
/*  ***  OTHERWISE FUNCTION DIFFERENCES.  STORAGE IN V IS AS IN DG7LIT. */

/* IRT VALUES... */
/*     1 = COMPUTE FUNCTION VALUE, I.E., V(F). */
/*     2 = COMPUTE G. */
/*     3 = DONE. */


/*  ***  PARAMETER DECLARATIONS  *** */


/*  ***  LOCAL VARIABLES  *** */


/*  ***  EXTERNAL SUBROUTINES  *** */


/* DV7CPY.... COPY ONE VECTOR TO ANOTHER. */

/*  ***  SUBSCRIPTS FOR IV AND V  *** */


/* /6 */
/*     DATA HALF/0.5D+0/, NEGPT5/-0.5D+0/, ONE/1.D+0/, TWO/2.D+0/, */
/*    1     ZERO/0.D+0/ */
/* /7 */
/* / */

/* /6 */
/*     DATA COVREQ/15/, DELTA/52/, DELTA0/44/, DLTFDC/42/, F/10/, */
/*    1     FDH/74/, FX/53/, H/56/, KAGQT/33/, MODE/35/, NFGCAL/7/, */
/*    2     SAVEI/63/, SWITCH/12/, TOOBIG/2/, W/65/, XMSAVE/51/ */
/* /7 */
/* / */

/* +++++++++++++++++++++++++++++++  BODY  ++++++++++++++++++++++++++++++++ */

    /* Parameter adjustments */
    --iv;
    --v;
    --x;
    --g;
    --d__;

    /* Function Body */
    *irt = 4;
    kind = iv[15];
    m = iv[35];
    if (m > 0) {
	goto L10;
    }
    iv[56] = -abs(iv[56]);
    iv[74] = 0;
    iv[33] = -1;
    v[53] = v[10];
L10:
    if (m > *p) {
	goto L999;
    }
    if (kind < 0) {
	goto L110;
    }

/*  ***  COMPUTE FINITE-DIFFERENCE HESSIAN USING BOTH FUNCTION AND */
/*  ***  GRADIENT VALUES. */

    gsave1 = iv[65] + *p;
    if (m > 0) {
	goto L20;
    }
/*        ***  FIRST CALL ON DF7HES.  SET GSAVE = G, TAKE FIRST STEP  *** */
    dv7cpy_(p, &v[gsave1], &g[1]);
    iv[12] = iv[7];
    goto L90;

L20:
    del = v[52];
    x[m] = v[51];
    if (iv[2] == 0) {
	goto L40;
    }

/*     ***  HANDLE OVERSIZE V(DELTA)  *** */

    if (del * x[m] > 0.) {
	goto L30;
    }
/*             ***  WE ALREADY TRIED SHRINKING V(DELTA), SO QUIT  *** */
    iv[74] = -2;
    goto L220;

/*        ***  TRY SHRINKING V(DELTA)  *** */
L30:
    del *= -.5;
    goto L100;

L40:
    hes = -iv[56];

/*  ***  SET  G = (G - GSAVE)/DEL  *** */

    i__1 = *p;
    for (i__ = 1; i__ <= i__1; ++i__) {
	g[i__] = (g[i__] - v[gsave1]) / del;
	++gsave1;
/* L50: */
    }

/*  ***  ADD G AS NEW COL. TO FINITE-DIFF. HESSIAN MATRIX  *** */

    k = hes + m * (m - 1) / 2;
    l = k + m - 2;
    if (m == 1) {
	goto L70;
    }

/*  ***  SET  H(I,M) = 0.5 * (H(I,M) + G(I))  FOR I = 1 TO M-1  *** */

    mm1 = m - 1;
    i__1 = mm1;
    for (i__ = 1; i__ <= i__1; ++i__) {
	v[k] = (v[k] + g[i__]) * .5;
	++k;
/* L60: */
    }

/*  ***  ADD  H(I,M) = G(I)  FOR I = M TO P  *** */

L70:
    ++l;
    i__1 = *p;
    for (i__ = m; i__ <= i__1; ++i__) {
	v[l] = g[i__];
	l += i__;
/* L80: */
    }

L90:
    ++m;
    iv[35] = m;
    if (m > *p) {
	goto L210;
    }

/*  ***  CHOOSE NEXT FINITE-DIFFERENCE STEP, RETURN TO GET G THERE  *** */

/* Computing MAX */
    d__2 = 1. / d__[m], d__3 = (d__1 = x[m], abs(d__1));
    del = v[44] * max(d__2,d__3);
    if (x[m] < 0.) {
	del = -del;
    }
    v[51] = x[m];
L100:
    x[m] += del;
    v[52] = del;
    *irt = 2;
    goto L999;

/*  ***  COMPUTE FINITE-DIFFERENCE HESSIAN USING FUNCTION VALUES ONLY. */

L110:
    stp0 = iv[65] + *p - 1;
    mm1 = m - 1;
    mm1o2 = m * mm1 / 2;
    if (m > 0) {
	goto L120;
    }
/*        ***  FIRST CALL ON DF7HES.  *** */
    iv[63] = 0;
    goto L200;

L120:
    i__ = iv[63];
    hes = -iv[56];
    if (i__ > 0) {
	goto L180;
    }
    if (iv[2] == 0) {
	goto L140;
    }

/*     ***  HANDLE OVERSIZE STEP  *** */

    stpm = stp0 + m;
    del = v[stpm];
    if (del * x[51] > 0.) {
	goto L130;
    }
/*             ***  WE ALREADY TRIED SHRINKING THE STEP, SO QUIT  *** */
    iv[74] = -2;
    goto L220;

/*        ***  TRY SHRINKING THE STEP  *** */
L130:
    del *= -.5;
    x[m] = x[51] + del;
    v[stpm] = del;
    *irt = 1;
    goto L999;

/*  ***  SAVE F(X + STP(M)*E(M)) IN H(P,M)  *** */

L140:
    pp1o2 = *p * (*p - 1) / 2;
    hpm = hes + pp1o2 + mm1;
    v[hpm] = v[10];

/*  ***  START COMPUTING ROW M OF THE FINITE-DIFFERENCE HESSIAN H.  *** */

    hmi = hes + mm1o2;
    if (mm1 == 0) {
	goto L160;
    }
    hpi = hes + pp1o2;
    i__1 = mm1;
    for (i__ = 1; i__ <= i__1; ++i__) {
	v[hmi] = v[53] - (v[10] + v[hpi]);
	++hmi;
	++hpi;
/* L150: */
    }
L160:
    v[hmi] = v[10] - v[53] * 2.;

/*  ***  COMPUTE FUNCTION VALUES NEEDED TO COMPLETE ROW M OF H.  *** */

    i__ = 1;

L170:
    iv[63] = i__;
    stpi = stp0 + i__;
    v[52] = x[i__];
    x[i__] += v[stpi];
    if (i__ == m) {
	x[i__] = v[51] - v[stpi];
    }
    *irt = 1;
    goto L999;

L180:
    x[i__] = v[52];
    if (iv[2] == 0) {
	goto L190;
    }
/*        ***  PUNT IN THE EVENT OF AN OVERSIZE STEP  *** */
    iv[74] = -2;
    goto L220;

/*  ***  FINISH COMPUTING H(M,I)  *** */

L190:
    stpi = stp0 + i__;
    hmi = hes + mm1o2 + i__ - 1;
    stpm = stp0 + m;
    v[hmi] = (v[hmi] + v[10]) / (v[stpi] * v[stpm]);
    ++i__;
    if (i__ <= m) {
	goto L170;
    }
    iv[63] = 0;
    x[m] = v[51];

L200:
    ++m;
    iv[35] = m;
    if (m > *p) {
	goto L210;
    }

/*  ***  PREPARE TO COMPUTE ROW M OF THE FINITE-DIFFERENCE HESSIAN H. */
/*  ***  COMPUTE M-TH STEP SIZE STP(M), THEN RETURN TO OBTAIN */
/*  ***  F(X + STP(M)*E(M)), WHERE E(M) = M-TH STD. UNIT VECTOR. */

/* Computing MAX */
    d__2 = 1. / d__[m], d__3 = (d__1 = x[m], abs(d__1));
    del = v[42] * max(d__2,d__3);
    if (x[m] < 0.) {
	del = -del;
    }
    v[51] = x[m];
    x[m] += del;
    stpm = stp0 + m;
    v[stpm] = del;
    *irt = 1;
    goto L999;

/*  ***  RESTORE V(F), ETC.  *** */

L210:
    iv[74] = hes;
L220:
    v[10] = v[53];
    *irt = 3;
    if (kind < 0) {
	goto L999;
    }
    iv[7] = iv[12];
    gsave1 = iv[65] + *p;
    dv7cpy_(p, &g[1], &v[gsave1]);
    goto L999;

L999:
    return 0;
/*  ***  LAST CARD OF DF7HES FOLLOWS  *** */
} /* df7hes_ */

