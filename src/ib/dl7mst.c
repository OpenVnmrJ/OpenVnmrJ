/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* dl7mst.f -- translated by f2c (version 20090411).
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

static integer c__6 = 6;

/* Subroutine */ int dl7mst_(doublereal *d__, doublereal *g, integer *ierr, 
	integer *ipivot, integer *ka, integer *p, doublereal *qtr, doublereal 
	*r__, doublereal *step, doublereal *v, doublereal *w)
{
    /* Initialized data */

    static doublereal big = 0.;

    /* System generated locals */
    integer i__1, i__2, i__3;
    doublereal d__1, d__2;

    /* Builtin functions */
    double sqrt(doublereal);

    /* Local variables */
    static doublereal a, b;
    static integer i__, k, l;
    static doublereal t, d1, d2;
    static integer i1, j1;
    static doublereal lk, si, sj, uk, wl;
    static integer lk0, ip1, uk0;
    static doublereal adi, rad, phi;
    static integer res;
    static doublereal dst;
    static integer res0, pp1o2, rmat;
    static doublereal dtol;
    static integer rmat0, kalim;
    extern doublereal dr7mdc_(integer *);
    extern /* Subroutine */ int dl7ivm_(integer *, doublereal *, doublereal *,
	     doublereal *);
    extern doublereal dd7tpr_(integer *, doublereal *, doublereal *), dv2nrm_(
	    integer *, doublereal *);
    extern /* Subroutine */ int dl7itv_(integer *, doublereal *, doublereal *,
	     doublereal *), dv7cpy_(integer *, doublereal *, doublereal *);
    extern doublereal dl7svn_(integer *, doublereal *, doublereal *, 
	    doublereal *);
    static doublereal alphak, dfacsq, psifac, oldphi, phimin, phimax;
    static integer phipin, dstsav;
    static doublereal sqrtak, twopsi;


/*  ***  COMPUTE LEVENBERG-MARQUARDT STEP USING MORE-HEBDEN TECHNIQUE  ** */
/*  ***  NL2SOL VERSION 2.2.  *** */

/*  ***  PARAMETER DECLARATIONS  *** */

/*     DIMENSION W(P*(P+5)/2 + 4) */

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

/*  ***  PURPOSE  *** */

/*        GIVEN THE R MATRIX FROM THE QR DECOMPOSITION OF A JACOBIAN */
/*     MATRIX, J, AS WELL AS Q-TRANSPOSE TIMES THE CORRESPONDING */
/*     RESIDUAL VECTOR, RESID, THIS SUBROUTINE COMPUTES A LEVENBERG- */
/*     MARQUARDT STEP OF APPROXIMATE LENGTH V(RADIUS) BY THE MORE- */
/*     TECHNIQUE. */

/*  ***  PARAMETER DESCRIPTION  *** */

/*      D (IN)  = THE SCALE VECTOR. */
/*      G (IN)  = THE GRADIENT VECTOR (J**T)*R. */
/*   IERR (I/O) = RETURN CODE FROM QRFACT OR DQ7RGS -- 0 MEANS R HAS */
/*             FULL RANK. */
/* IPIVOT (I/O) = PERMUTATION ARRAY FROM QRFACT OR DQ7RGS, WHICH COMPUTE */
/*             QR DECOMPOSITIONS WITH COLUMN PIVOTING. */
/*     KA (I/O).  KA .LT. 0 ON INPUT MEANS THIS IS THE FIRST CALL ON */
/*             DL7MST FOR THE CURRENT R AND QTR.  ON OUTPUT KA CON- */
/*             TAINS THE NUMBER OF HEBDEN ITERATIONS NEEDED TO DETERMINE */
/*             STEP.  KA = 0 MEANS A GAUSS-NEWTON STEP. */
/*      P (IN)  = NUMBER OF PARAMETERS. */
/*    QTR (IN)  = (Q**T)*RESID = Q-TRANSPOSE TIMES THE RESIDUAL VECTOR. */
/*      R (IN)  = THE R MATRIX, STORED COMPACTLY BY COLUMNS. */
/*   STEP (OUT) = THE LEVENBERG-MARQUARDT STEP COMPUTED. */
/*      V (I/O) CONTAINS VARIOUS CONSTANTS AND VARIABLES DESCRIBED BELOW. */
/*      W (I/O) = WORKSPACE OF LENGTH P*(P+5)/2 + 4. */

/*  ***  ENTRIES IN V  *** */

/* V(DGNORM) (I/O) = 2-NORM OF (D**-1)*G. */
/* V(DSTNRM) (I/O) = 2-NORM OF D*STEP. */
/* V(DST0)   (I/O) = 2-NORM OF GAUSS-NEWTON STEP (FOR NONSING. J). */
/* V(EPSLON) (IN) = MAX. REL. ERROR ALLOWED IN TWONORM(R)**2 MINUS */
/*             TWONORM(R - J*STEP)**2.  (SEE ALGORITHM NOTES BELOW.) */
/* V(GTSTEP) (OUT) = INNER PRODUCT BETWEEN G AND STEP. */
/* V(NREDUC) (OUT) = HALF THE REDUCTION IN THE SUM OF SQUARES PREDICTED */
/*             FOR A GAUSS-NEWTON STEP. */
/* V(PHMNFC) (IN)  = TOL. (TOGETHER WITH V(PHMXFC)) FOR ACCEPTING STEP */
/*             (MORE*S SIGMA).  THE ERROR V(DSTNRM) - V(RADIUS) MUST LIE */
/*             BETWEEN V(PHMNFC)*V(RADIUS) AND V(PHMXFC)*V(RADIUS). */
/* V(PHMXFC) (IN)  (SEE V(PHMNFC).) */
/* V(PREDUC) (OUT) = HALF THE REDUCTION IN THE SUM OF SQUARES PREDICTED */
/*             BY THE STEP RETURNED. */
/* V(RADIUS) (IN)  = RADIUS OF CURRENT (SCALED) TRUST REGION. */
/* V(RAD0)   (I/O) = VALUE OF V(RADIUS) FROM PREVIOUS CALL. */
/* V(STPPAR) (I/O) = MARQUARDT PARAMETER (OR ITS NEGATIVE IF THE SPECIAL */
/*             CASE MENTIONED BELOW IN THE ALGORITHM NOTES OCCURS). */

/* NOTE -- SEE DATA STATEMENT BELOW FOR VALUES OF ABOVE SUBSCRIPTS. */

/*  ***  USAGE NOTES  *** */

/*     IF IT IS DESIRED TO RECOMPUTE STEP USING A DIFFERENT VALUE OF */
/*     V(RADIUS), THEN THIS ROUTINE MAY BE RESTARTED BY CALLING IT */
/*     WITH ALL PARAMETERS UNCHANGED EXCEPT V(RADIUS).  (THIS EXPLAINS */
/*     WHY MANY PARAMETERS ARE LISTED AS I/O).  ON AN INTIIAL CALL (ONE */
/*     WITH KA = -1), THE CALLER NEED ONLY HAVE INITIALIZED D, G, KA, P, */
/*     QTR, R, V(EPSLON), V(PHMNFC), V(PHMXFC), V(RADIUS), AND V(RAD0). */

/*  ***  APPLICATION AND USAGE RESTRICTIONS  *** */

/*     THIS ROUTINE IS CALLED AS PART OF THE NL2SOL (NONLINEAR LEAST- */
/*     SQUARES) PACKAGE (REF. 1). */

/*  ***  ALGORITHM NOTES  *** */

/*     THIS CODE IMPLEMENTS THE STEP COMPUTATION SCHEME DESCRIBED IN */
/*     REFS. 2 AND 4.  FAST GIVENS TRANSFORMATIONS (SEE REF. 3, PP. 60- */
/*     62) ARE USED TO COMPUTE STEP WITH A NONZERO MARQUARDT PARAMETER. */
/*        A SPECIAL CASE OCCURS IF J IS (NEARLY) SINGULAR AND V(RADIUS) */
/*     IS SUFFICIENTLY LARGE.  IN THIS CASE THE STEP RETURNED IS SUCH */
/*     THAT  TWONORM(R)**2 - TWONORM(R - J*STEP)**2  DIFFERS FROM ITS */
/*     OPTIMAL VALUE BY LESS THAN V(EPSLON) TIMES THIS OPTIMAL VALUE, */
/*     WHERE J AND R DENOTE THE ORIGINAL JACOBIAN AND RESIDUAL.  (SEE */
/*     REF. 2 FOR MORE DETAILS.) */

/*  ***  FUNCTIONS AND SUBROUTINES CALLED  *** */

/* DD7TPR - RETURNS INNER PRODUCT OF TWO VECTORS. */
/* DL7ITV - APPLY INVERSE-TRANSPOSE OF COMPACT LOWER TRIANG. MATRIX. */
/* DL7IVM - APPLY INVERSE OF COMPACT LOWER TRIANG. MATRIX. */
/* DV7CPY  - COPIES ONE VECTOR TO ANOTHER. */
/* DV2NRM - RETURNS 2-NORM OF A VECTOR. */

/*  ***  REFERENCES  *** */

/* 1.  DENNIS, J.E., GAY, D.M., AND WELSCH, R.E. (1981), AN ADAPTIVE */
/*             NONLINEAR LEAST-SQUARES ALGORITHM, ACM TRANS. MATH. */
/*             SOFTWARE, VOL. 7, NO. 3. */
/* 2.  GAY, D.M. (1981), COMPUTING OPTIMAL LOCALLY CONSTRAINED STEPS, */
/*             SIAM J. SCI. STATIST. COMPUTING, VOL. 2, NO. 2, PP. */
/*             186-197. */
/* 3.  LAWSON, C.L., AND HANSON, R.J. (1974), SOLVING LEAST SQUARES */
/*             PROBLEMS, PRENTICE-HALL, ENGLEWOOD CLIFFS, N.J. */
/* 4.  MORE, J.J. (1978), THE LEVENBERG-MARQUARDT ALGORITHM, IMPLEMEN- */
/*             TATION AND THEORY, PP.105-116 OF SPRINGER LECTURE NOTES */
/*             IN MATHEMATICS NO. 630, EDITED BY G.A. WATSON, SPRINGER- */
/*             VERLAG, BERLIN AND NEW YORK. */

/*  ***  GENERAL  *** */

/*     CODED BY DAVID M. GAY. */
/*     THIS SUBROUTINE WAS WRITTEN IN CONNECTION WITH RESEARCH */
/*     SUPPORTED BY THE NATIONAL SCIENCE FOUNDATION UNDER GRANTS */
/*     MCS-7600324, DCR75-10143, 76-14311DSS, MCS76-11989, AND */
/*     MCS-7906671. */

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

/*  ***  LOCAL VARIABLES  *** */


/*     ***  CONSTANTS  *** */

/*  ***  INTRINSIC FUNCTIONS  *** */
/* /+ */
/* / */
/*  ***  EXTERNAL FUNCTIONS AND SUBROUTINES  *** */


/*  ***  SUBSCRIPTS FOR V  *** */

/* /6 */
/*     DATA DGNORM/1/, DSTNRM/2/, DST0/3/, EPSLON/19/, GTSTEP/4/, */
/*    1     NREDUC/6/, PHMNFC/20/, PHMXFC/21/, PREDUC/7/, RADIUS/8/, */
/*    2     RAD0/9/, STPPAR/5/ */
/* /7 */
/* / */

/* /6 */
/*     DATA DFAC/256.D+0/, EIGHT/8.D+0/, HALF/0.5D+0/, NEGONE/-1.D+0/, */
/*    1     ONE/1.D+0/, P001/1.D-3/, THREE/3.D+0/, TTOL/2.5D+0/, */
/*    2     ZERO/0.D+0/ */
/* /7 */
/* / */
    /* Parameter adjustments */
    --step;
    --qtr;
    --ipivot;
    --g;
    --d__;
    --r__;
    --v;
    --w;

    /* Function Body */

/*  ***  BODY  *** */

/*     ***  FOR USE IN RECOMPUTING STEP, THE FINAL VALUES OF LK AND UK, */
/*     ***  THE INVERSE DERIVATIVE OF MORE*S PHI AT 0 (FOR NONSING. J) */
/*     ***  AND THE VALUE RETURNED AS V(DSTNRM) ARE STORED AT W(LK0), */
/*     ***  W(UK0), W(PHIPIN), AND W(DSTSAV) RESPECTIVELY. */
    lk0 = *p + 1;
    phipin = lk0 + 1;
    uk0 = phipin + 1;
    dstsav = uk0 + 1;
    rmat0 = dstsav;
/*     ***  A COPY OF THE R-MATRIX FROM THE QR DECOMPOSITION OF J IS */
/*     ***  STORED IN W STARTING AT W(RMAT), AND A COPY OF THE RESIDUAL */
/*     ***  VECTOR IS STORED IN W STARTING AT W(RES).  THE LOOPS BELOW */
/*     ***  THAT UPDATE THE QR DECOMP. FOR A NONZERO MARQUARDT PARAMETER */
/*     ***  WORK ON THESE COPIES. */
    rmat = rmat0 + 1;
    pp1o2 = *p * (*p + 1) / 2;
    res0 = pp1o2 + rmat0;
    res = res0 + 1;
    rad = v[8];
    if (rad > 0.) {
/* Computing 2nd power */
	d__1 = rad;
	psifac = v[19] / (((v[20] + 1.) * 8. + 3.) * (d__1 * d__1));
    }
    if (big <= 0.) {
	big = dr7mdc_(&c__6);
    }
    phimax = v[21] * rad;
    phimin = v[20] * rad;
/*     ***  DTOL, DFAC, AND DFACSQ ARE USED IN RESCALING THE FAST GIVENS */
/*     ***  REPRESENTATION OF THE UPDATED QR DECOMPOSITION. */
    dtol = .00390625;
    dfacsq = 65536.;
/*     ***  OLDPHI IS USED TO DETECT LIMITS OF NUMERICAL ACCURACY.  IF */
/*     ***  WE RECOMPUTE STEP AND IT DOES NOT CHANGE, THEN WE ACCEPT IT. */
    oldphi = 0.;
    lk = 0.;
    uk = 0.;
    kalim = *ka + 12;

/*  ***  START OR RESTART, DEPENDING ON KA  *** */

    if (*ka < 0) {
	goto L10;
    } else if (*ka == 0) {
	goto L20;
    } else {
	goto L370;
    }

/*  ***  FRESH START -- COMPUTE V(NREDUC)  *** */

L10:
    *ka = 0;
    kalim = 12;
    k = *p;
    if (*ierr != 0) {
	k = abs(*ierr) - 1;
    }
    v[6] = dd7tpr_(&k, &qtr[1], &qtr[1]) * .5;

/*  ***  SET UP TO TRY INITIAL GAUSS-NEWTON STEP  *** */

L20:
    v[3] = -1.;
    if (*ierr != 0) {
	goto L90;
    }
    t = dl7svn_(p, &r__[1], &step[1], &w[res]);
    if (t >= 1.) {
	goto L30;
    }
    if (dv2nrm_(p, &qtr[1]) >= big * t) {
	goto L90;
    }

/*  ***  COMPUTE GAUSS-NEWTON STEP  *** */

/*     ***  NOTE -- THE R-MATRIX IS STORED COMPACTLY BY COLUMNS IN */
/*     ***  R(1), R(2), R(3), ...  IT IS THE TRANSPOSE OF A */
/*     ***  LOWER TRIANGULAR MATRIX STORED COMPACTLY BY ROWS, AND WE */
/*     ***  TREAT IT AS SUCH WHEN USING DL7ITV AND DL7IVM. */
L30:
    dl7itv_(p, &w[1], &r__[1], &qtr[1]);
/*     ***  TEMPORARILY STORE PERMUTED -D*STEP IN STEP. */
    i__1 = *p;
    for (i__ = 1; i__ <= i__1; ++i__) {
	j1 = ipivot[i__];
	step[i__] = d__[j1] * w[i__];
/* L60: */
    }
    dst = dv2nrm_(p, &step[1]);
    v[3] = dst;
    phi = dst - rad;
    if (phi <= phimax) {
	goto L410;
    }
/*     ***  IF THIS IS A RESTART, GO TO 110  *** */
    if (*ka > 0) {
	goto L110;
    }

/*  ***  GAUSS-NEWTON STEP WAS UNACCEPTABLE.  COMPUTE L0  *** */

    i__1 = *p;
    for (i__ = 1; i__ <= i__1; ++i__) {
	j1 = ipivot[i__];
	step[i__] = d__[j1] * (step[i__] / dst);
/* L70: */
    }
    dl7ivm_(p, &step[1], &r__[1], &step[1]);
    t = 1. / dv2nrm_(p, &step[1]);
    w[phipin] = t / rad * t;
    lk = phi * w[phipin];

/*  ***  COMPUTE U0  *** */

L90:
    i__1 = *p;
    for (i__ = 1; i__ <= i__1; ++i__) {
/* L100: */
	w[i__] = g[i__] / d__[i__];
    }
    v[1] = dv2nrm_(p, &w[1]);
    uk = v[1] / rad;
    if (uk <= 0.) {
	goto L390;
    }

/*     ***  ALPHAK WILL BE USED AS THE CURRENT MARQUARDT PARAMETER.  WE */
/*     ***  USE MORE*S SCHEME FOR INITIALIZING IT. */

    alphak = abs(v[5]) * v[9] / rad;
/* Computing MIN */
    d__1 = uk, d__2 = max(alphak,lk);
    alphak = min(d__1,d__2);


/*  ***  TOP OF LOOP -- INCREMENT KA, COPY R TO RMAT, QTR TO RES  *** */

L110:
    ++(*ka);
    dv7cpy_(&pp1o2, &w[rmat], &r__[1]);
    dv7cpy_(p, &w[res], &qtr[1]);

/*  ***  SAFEGUARD ALPHAK AND INITIALIZE FAST GIVENS SCALE VECTOR.  *** */

    if (alphak <= 0. || alphak < lk || alphak >= uk) {
/* Computing MAX */
	d__1 = .001, d__2 = sqrt(lk / uk);
	alphak = uk * max(d__1,d__2);
    }
    if (alphak <= 0.) {
	alphak = uk * .5;
    }
    sqrtak = sqrt(alphak);
    i__1 = *p;
    for (i__ = 1; i__ <= i__1; ++i__) {
/* L120: */
	w[i__] = 1.;
    }

/*  ***  ADD ALPHAK*D AND UPDATE QR DECOMP. USING FAST GIVENS TRANS.  *** */

    i__1 = *p;
    for (i__ = 1; i__ <= i__1; ++i__) {
/*        ***  GENERATE, APPLY 1ST GIVENS TRANS. FOR ROW I OF ALPHAK*D. */
/*        ***  (USE STEP TO STORE TEMPORARY ROW)  *** */
	l = i__ * (i__ + 1) / 2 + rmat0;
	wl = w[l];
	d2 = 1.;
	d1 = w[i__];
	j1 = ipivot[i__];
	adi = sqrtak * d__[j1];
	if (adi >= abs(wl)) {
	    goto L150;
	}
L130:
	a = adi / wl;
	b = d2 * a / d1;
	t = a * b + 1.;
	if (t > 2.5) {
	    goto L150;
	}
	w[i__] = d1 / t;
	d2 /= t;
	w[l] = t * wl;
	a = -a;
	i__2 = *p;
	for (j1 = i__; j1 <= i__2; ++j1) {
	    l += j1;
	    step[j1] = a * w[l];
/* L140: */
	}
	goto L170;

L150:
	b = wl / adi;
	a = d1 * b / d2;
	t = a * b + 1.;
	if (t > 2.5) {
	    goto L130;
	}
	w[i__] = d2 / t;
	d2 = d1 / t;
	w[l] = t * adi;
	i__2 = *p;
	for (j1 = i__; j1 <= i__2; ++j1) {
	    l += j1;
	    wl = w[l];
	    step[j1] = -wl;
	    w[l] = a * wl;
/* L160: */
	}

L170:
	if (i__ == *p) {
	    goto L280;
	}

/*        ***  NOW USE GIVENS TRANS. TO ZERO ELEMENTS OF TEMP. ROW  *** */

	ip1 = i__ + 1;
	i__2 = *p;
	for (i1 = ip1; i1 <= i__2; ++i1) {
	    si = step[i1 - 1];
	    if (si == 0.) {
		goto L260;
	    }
	    l = i1 * (i1 + 1) / 2 + rmat0;
	    wl = w[l];
	    d1 = w[i1];

/*             ***  RESCALE ROW I1 IF NECESSARY  *** */

	    if (d1 >= dtol) {
		goto L190;
	    }
	    d1 *= dfacsq;
	    wl /= 256.;
	    k = l;
	    i__3 = *p;
	    for (j1 = i1; j1 <= i__3; ++j1) {
		k += j1;
		w[k] /= 256.;
/* L180: */
	    }

/*             ***  USE GIVENS TRANS. TO ZERO NEXT ELEMENT OF TEMP. ROW */

L190:
	    if (abs(si) > abs(wl)) {
		goto L220;
	    }
L200:
	    a = si / wl;
	    b = d2 * a / d1;
	    t = a * b + 1.;
	    if (t > 2.5) {
		goto L220;
	    }
	    w[l] = t * wl;
	    w[i1] = d1 / t;
	    d2 /= t;
	    i__3 = *p;
	    for (j1 = i1; j1 <= i__3; ++j1) {
		l += j1;
		wl = w[l];
		sj = step[j1];
		w[l] = wl + b * sj;
		step[j1] = sj - a * wl;
/* L210: */
	    }
	    goto L240;

L220:
	    b = wl / si;
	    a = d1 * b / d2;
	    t = a * b + 1.;
	    if (t > 2.5) {
		goto L200;
	    }
	    w[i1] = d2 / t;
	    d2 = d1 / t;
	    w[l] = t * si;
	    i__3 = *p;
	    for (j1 = i1; j1 <= i__3; ++j1) {
		l += j1;
		wl = w[l];
		sj = step[j1];
		w[l] = a * wl + sj;
		step[j1] = b * sj - wl;
/* L230: */
	    }

/*             ***  RESCALE TEMP. ROW IF NECESSARY  *** */

L240:
	    if (d2 >= dtol) {
		goto L260;
	    }
	    d2 *= dfacsq;
	    i__3 = *p;
	    for (k = i1; k <= i__3; ++k) {
/* L250: */
		step[k] /= 256.;
	    }
L260:
	    ;
	}
/* L270: */
    }

/*  ***  COMPUTE STEP  *** */

L280:
    dl7itv_(p, &w[res], &w[rmat], &w[res]);
/*     ***  RECOVER STEP AND STORE PERMUTED -D*STEP AT W(RES)  *** */
    i__1 = *p;
    for (i__ = 1; i__ <= i__1; ++i__) {
	j1 = ipivot[i__];
	k = res0 + i__;
	t = w[k];
	step[j1] = -t;
	w[k] = t * d__[j1];
/* L290: */
    }
    dst = dv2nrm_(p, &w[res]);
    phi = dst - rad;
    if (phi <= phimax && phi >= phimin) {
	goto L430;
    }
    if (oldphi == phi) {
	goto L430;
    }
    oldphi = phi;

/*  ***  CHECK FOR (AND HANDLE) SPECIAL CASE  *** */

    if (phi > 0.) {
	goto L310;
    }
    if (*ka >= kalim) {
	goto L430;
    }
    twopsi = alphak * dst * dst - dd7tpr_(p, &step[1], &g[1]);
    if (alphak >= twopsi * psifac) {
	goto L310;
    }
    v[5] = -alphak;
    goto L440;

/*  ***  UNACCEPTABLE STEP -- UPDATE LK, UK, ALPHAK, AND TRY AGAIN  *** */

L300:
    if (phi < 0.) {
	uk = min(uk,alphak);
    }
    goto L320;
L310:
    if (phi < 0.) {
	uk = alphak;
    }
L320:
    i__1 = *p;
    for (i__ = 1; i__ <= i__1; ++i__) {
	j1 = ipivot[i__];
	k = res0 + i__;
	step[i__] = d__[j1] * (w[k] / dst);
/* L330: */
    }
    dl7ivm_(p, &step[1], &w[rmat], &step[1]);
    i__1 = *p;
    for (i__ = 1; i__ <= i__1; ++i__) {
/* L340: */
	step[i__] /= sqrt(w[i__]);
    }
    t = 1. / dv2nrm_(p, &step[1]);
    alphak += t * phi * t / rad;
    lk = max(lk,alphak);
    alphak = lk;
    goto L110;

/*  ***  RESTART  *** */

L370:
    lk = w[lk0];
    uk = w[uk0];
    if (v[3] > 0. && v[3] - rad <= phimax) {
	goto L20;
    }
    alphak = abs(v[5]);
    dst = w[dstsav];
    phi = dst - rad;
    t = v[1] / rad;
    if (rad > v[9]) {
	goto L380;
    }

/*        ***  SMALLER RADIUS  *** */
    uk = t;
    if (alphak <= 0.) {
	lk = 0.;
    }
    if (v[3] > 0.) {
/* Computing MAX */
	d__1 = lk, d__2 = (v[3] - rad) * w[phipin];
	lk = max(d__1,d__2);
    }
    goto L300;

/*     ***  BIGGER RADIUS  *** */
L380:
    if (alphak <= 0. || uk > t) {
	uk = t;
    }
    lk = 0.;
    if (v[3] > 0.) {
/* Computing MAX */
	d__1 = lk, d__2 = (v[3] - rad) * w[phipin];
	lk = max(d__1,d__2);
    }
    goto L300;

/*  ***  SPECIAL CASE -- RAD .LE. 0 OR (G = 0 AND J IS SINGULAR)  *** */

L390:
    v[5] = 0.;
    dst = 0.;
    lk = 0.;
    uk = 0.;
    v[4] = 0.;
    v[7] = 0.;
    i__1 = *p;
    for (i__ = 1; i__ <= i__1; ++i__) {
/* L400: */
	step[i__] = 0.;
    }
    goto L450;

/*  ***  ACCEPTABLE GAUSS-NEWTON STEP -- RECOVER STEP FROM W  *** */

L410:
    alphak = 0.;
    i__1 = *p;
    for (i__ = 1; i__ <= i__1; ++i__) {
	j1 = ipivot[i__];
	step[j1] = -w[i__];
/* L420: */
    }

/*  ***  SAVE VALUES FOR USE IN A POSSIBLE RESTART  *** */

L430:
    v[5] = alphak;
L440:
/* Computing MIN */
    d__1 = dd7tpr_(p, &step[1], &g[1]);
    v[4] = min(d__1,0.);
    v[7] = (alphak * dst * dst - v[4]) * .5;
L450:
    v[2] = dst;
    w[dstsav] = dst;
    w[lk0] = lk;
    w[uk0] = uk;
    v[9] = rad;

/* L999: */
    return 0;

/*  ***  LAST CARD OF DL7MST FOLLOWS  *** */
} /* dl7mst_ */

