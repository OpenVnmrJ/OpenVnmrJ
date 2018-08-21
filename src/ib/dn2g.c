/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* dn2g.f -- translated by f2c (version 20090411).
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

/* Subroutine */ int dn2g_(integer *n, integer *p, doublereal *x, S_fp calcr, 
	S_fp calcj, integer *iv, integer *liv, integer *lv, doublereal *v, 
	integer *ui, doublereal *ur, U_fp uf)
{
    /* System generated locals */
    integer i__1;

    /* Local variables */
    static integer d1, n1, n2, r1, nf, dr1, rd1, iv1;
    extern /* Subroutine */ int drn2g_(doublereal *, doublereal *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, doublereal *, doublereal *, doublereal *, doublereal *)
	    , dn2rdp_(integer *, integer *, integer *, integer *, doublereal *
	    , doublereal *), divset_(integer *, integer *, integer *, integer 
	    *, doublereal *);


/*  ***  VERSION OF NL2SOL THAT CALLS  DRN2G  *** */

/*  ***  PARAMETERS  *** */

/* /6 */
/*     INTEGER IV(LIV), UI(1) */
/*     DOUBLE PRECISION X(P), V(LV), UR(1) */
/* /7 */
/* / */

/*  ***  PARAMETER USAGE  *** */

/* N....... TOTAL NUMBER OF RESIDUALS. */
/* P....... NUMBER OF PARAMETERS (COMPONENTS OF X) BEING ESTIMATED. */
/* X....... PARAMETER VECTOR BEING ESTIMATED (INPUT = INITIAL GUESS, */
/*             OUTPUT = BEST VALUE FOUND). */
/* CALCR... SUBROUTINE FOR COMPUTING RESIDUAL VECTOR. */
/* CALCJ... SUBROUTINE FOR COMPUTING JACOBIAN MATRIX = MATRIX OF FIRST */
/*             PARTIALS OF THE RESIDUAL VECTOR. */
/* IV...... INTEGER VALUES ARRAY. */
/* LIV..... LENGTH OF IV (SEE DISCUSSION BELOW). */
/* LV...... LENGTH OF V (SEE DISCUSSION BELOW). */
/* V....... FLOATING-POINT VALUES ARRAY. */
/* UI...... PASSED UNCHANGED TO CALCR AND CALCJ. */
/* UR...... PASSED UNCHANGED TO CALCR AND CALCJ. */
/* UF...... PASSED UNCHANGED TO CALCR AND CALCJ. */


/*  ***  DISCUSSION  *** */

/*        NOTE... NL2SOL (MENTIONED BELOW) IS A CODE FOR SOLVING */
/*     NONLINEAR LEAST-SQUARES PROBLEMS.  IT IS DESCRIBED IN */
/*     ACM TRANS. MATH. SOFTWARE, VOL. 7 (1981), PP. 369-383 */
/*     (AN ADAPTIVE NONLINEAR LEAST-SQUARES ALGORITHM, BY J.E. DENNIS, */
/*     D.M. GAY, AND R.E. WELSCH). */

/*        LIV GIVES THE LENGTH OF IV.  IT MUST BE AT LEAST 82+P.  IF NOT, */
/*     THEN   DN2G RETURNS WITH IV(1) = 15.  WHEN   DN2G RETURNS, THE */
/*     MINIMUM ACCEPTABLE VALUE OF LIV IS STORED IN IV(LASTIV) = IV(44), */
/*     (PROVIDED THAT LIV .GE. 44). */

/*        LV GIVES THE LENGTH OF V.  THE MINIMUM VALUE FOR LV IS */
/*     LV0 = 105 + P*(N + 2*P + 17) + 2*N.  IF LV IS SMALLER THAN THIS, */
/*     THEN   DN2G RETURNS WITH IV(1) = 16.  WHEN   DN2G RETURNS, THE */
/*     MINIMUM ACCEPTABLE VALUE OF LV IS STORED IN IV(LASTV) = IV(45) */
/*     (PROVIDED LIV .GE. 45). */

/*        RETURN CODES AND CONVERGENCE TOLERANCES ARE THE SAME AS FOR */
/*     NL2SOL, WITH SOME SMALL EXTENSIONS... IV(1) = 15 MEANS LIV WAS */
/*     TOO SMALL.   IV(1) = 16 MEANS LV WAS TOO SMALL. */

/*        THERE ARE TWO NEW V INPUT COMPONENTS...  V(LMAXS) = V(36) AND */
/*     V(SCTOL) = V(37) SERVE WHERE V(LMAX0) AND V(RFCTOL) FORMERLY DID */
/*     IN THE SINGULAR CONVERGENCE TEST -- SEE THE NL2SOL DOCUMENTATION. */

/*  ***  DEFAULT VALUES  *** */

/*        DEFAULT VALUES ARE PROVIDED BY SUBROUTINE DIVSET, RATHER THAN */
/*     DFAULT.  THE CALLING SEQUENCE IS... */
/*             CALL DIVSET(1, IV, LIV, LV, V) */
/*     THE FIRST PARAMETER IS AN INTEGER 1.  IF LIV AND LV ARE LARGE */
/*     ENOUGH FOR DIVSET, THEN DIVSET SETS IV(1) TO 12.  OTHERWISE IT */
/*     SETS IV(1) TO 15 OR 16.  CALLING   DN2G WITH IV(1) = 0 CAUSES ALL */
/*     DEFAULT VALUES TO BE USED FOR THE INPUT COMPONENTS OF IV AND V. */
/*        IF YOU FIRST CALL DIVSET, THEN SET IV(1) TO 13 AND CALL   DN2G, */
/*     THEN STORAGE ALLOCATION ONLY WILL BE PERFORMED.  IN PARTICULAR, */
/*     IV(D) = IV(27), IV(J) = IV(70), AND IV(R) = IV(61) WILL BE SET */
/*     TO THE FIRST SUBSCRIPT IN V OF THE SCALE VECTOR, THE JACOBIAN */
/*     MATRIX, AND THE RESIDUAL VECTOR RESPECTIVELY, PROVIDED LIV AND LV */
/*     ARE LARGE ENOUGH.  IF SO, THEN   DN2G RETURNS WITH IV(1) = 14. */
/*     WHEN CALLED WITH IV(1) = 14,   DN2G ASSUMES THAT STORAGE HAS */
/*     BEEN ALLOCATED, AND IT BEGINS THE MINIMIZATION ALGORITHM. */

/*  ***  SCALE VECTOR  *** */

/*        ONE DIFFERENCE WITH NL2SOL IS THAT THE SCALE VECTOR D IS */
/*     STORED IN V, STARTING AT A DIFFERENT SUBSCRIPT.  THE STARTING */
/*     SUBSCRIPT VALUE IS STILL STORED IN IV(D) = IV(27).  THE */
/*     DISCUSSION OF DEFAULT VALUES ABOVE TELLS HOW TO HAVE IV(D) SET */
/*     BEFORE THE ALGORITHM IS STARTED. */

/*  ***  REGRESSION DIAGNOSTICS  *** */

/*        IF IV(RDREQ) SO DICTATES, THEN ESTIMATES ARE COMPUTED OF THE */
/*     INFLUENCE EACH RESIDUAL COMPONENT HAS ON THE FINAL PARAMETER */
/*     ESTIMATE X.  THE GENERAL IDEA IS THAT ONE MAY WISH TO EXAMINE */
/*     RESIDUAL COMPONENTS (AND THE DATA BEHIND THEM) FOR WHICH THE */
/*     INFLUENCE ESTIMATE IS SIGNIFICANTLY LARGER THAN MOST OF THE OTHER */
/*     INFLUENCE ESTIMATES.  THESE ESTIMATES, HEREAFTER CALLED */
/*     REGRESSION DIAGNOSTICS, ARE ONLY COMPUTED IF IV(RDREQ) = 2 OR 3. */
/*     IN THIS CASE, FOR I = 1(1)N, */
/*                    SQRT( G(I)**T * H(I)**-1 * G(I) ) */
/*     IS COMPUTED AND STORED IN V, STARTING AT V(IV(REGD)), WHERE */
/*     RDREQ = 57 AND REGD = 67.  HERE G(I) STANDS FOR THE GRADIENT */
/*     RESULTING WHEN THE I-TH OBSERVATION IS DELETED AND H(I) STANDS */
/*     FOR AN APPROXIMATION TO THE CORRESPONDING HESSIAN AT X, THE SOLU- */
/*     TION CORRESPONDING TO ALL OBSERVATIONS.  (THIS APPROXIMATION IS */
/*     OBTAINED BY SUBTRACTING THE FIRST-ORDER CONTRIBUTION OF THE I-TH */
/*     OBSERVATION TO THE HESSIAN FROM A FINITE-DIFFERENCE HESSIAN */
/*     APPROXIMATION.  IF H IS INDEFINITE, THEN IV(REGD) IS SET TO -1. */
/*     IF H(I) IS INDEFINITE, THEN -1 IS RETURNED AS THE DIAGNOSTIC FOR */
/*     OBSERVATION I.  IF NO DIAGNOSTICS ARE COMPUTED, PERHAPS BECAUSE */
/*     OF A FAILURE TO CONVERGE, THEN IV(REGD) = 0 IS RETURNED.) */
/*        PRINTING OF THE REGRESSION DIAGNOSTICS IS CONTROLLED BY */
/*     IV(COVPRT) = IV(14)...  IF IV(COVPRT) = 3, THEN BOTH THE */
/*     COVARIANCE MATRIX AND THE REGRESSION DIAGNOSTICS ARE PRINTED. */
/*     IV(COVPRT) = 2 CAUSES ONLY THE REGRESSION DIAGNOSTICS TO BE */
/*     PRINTED, IV(COVPRT) = 1 CAUSES ONLY THE COVARIANCE MATRIX TO BE */
/*     PRINTED, AND IV(COVPRT) = 0 CAUSES NEITHER TO BE PRINTED. */

/*        RDREQ = 57 AND REGD = 67. */

/*  ***  GENERAL  *** */

/*     CODED BY DAVID M. GAY. */

/* +++++++++++++++++++++++++++  DECLARATIONS  +++++++++++++++++++++++++++ */

/*  ***  EXTERNAL SUBROUTINES  *** */

/* DIVSET.... PROVIDES DEFAULT IV AND V INPUT COMPONENTS. */
/*  DRN2G... CARRIES OUT OPTIMIZATION ITERATIONS. */
/* DN2RDP... PRINTS REGRESSION DIAGNOSTICS. */

/*  ***  NO INTRINSIC FUNCTIONS  *** */

/*  ***  LOCAL VARIABLES  *** */


/*  ***  IV COMPONENTS  *** */

/* /6 */
/*     DATA D/27/, J/70/, NEXTV/47/, NFCALL/6/, NFGCAL/7/, R/61/, */
/*    1     REGD/67/, REGD0/82/, TOOBIG/2/, VNEED/4/ */
/* /7 */
/* / */
/* ---------------------------------  BODY  ------------------------------ */

    /* Parameter adjustments */
    --x;
    --iv;
    --v;
    --ui;
    --ur;

    /* Function Body */
    if (iv[1] == 0) {
	divset_(&c__1, &iv[1], liv, lv, &v[1]);
    }
    iv1 = iv[1];
    if (iv1 == 14) {
	goto L10;
    }
    if (iv1 > 2 && iv1 < 12) {
	goto L10;
    }
    if (iv1 == 12) {
	iv[1] = 13;
    }
    if (iv[1] == 13) {
	iv[4] = iv[4] + *p + *n * (*p + 2);
    }
    drn2g_(&x[1], &v[1], &iv[1], liv, lv, n, n, &n1, &n2, p, &v[1], &v[1], &v[
	    1], &x[1]);
    if (iv[1] != 14) {
	goto L999;
    }

/*  ***  STORAGE ALLOCATION  *** */

    iv[27] = iv[47];
    iv[61] = iv[27] + *p;
    iv[82] = iv[61] + *n;
    iv[70] = iv[82] + *n;
    iv[47] = iv[70] + *n * *p;
    if (iv1 == 13) {
	goto L999;
    }

L10:
    d1 = iv[27];
    dr1 = iv[70];
    r1 = iv[61];
    rd1 = iv[82];

L20:
    drn2g_(&v[d1], &v[dr1], &iv[1], liv, lv, n, n, &n1, &n2, p, &v[r1], &v[
	    rd1], &v[1], &x[1]);
    if ((i__1 = iv[1] - 2) < 0) {
	goto L30;
    } else if (i__1 == 0) {
	goto L50;
    } else {
	goto L60;
    }

/*  ***  NEW FUNCTION VALUE (R VALUE) NEEDED  *** */

L30:
    nf = iv[6];
    (*calcr)(n, p, &x[1], &nf, &v[r1], &ui[1], &ur[1], (U_fp)uf);
    if (nf > 0) {
	goto L40;
    }
    iv[2] = 1;
    goto L20;
L40:
    if (iv[1] > 0) {
	goto L20;
    }

/*  ***  COMPUTE DR = GRADIENT OF R COMPONENTS  *** */

L50:
    (*calcj)(n, p, &x[1], &iv[7], &v[dr1], &ui[1], &ur[1], (U_fp)uf);
    if (iv[7] == 0) {
	iv[2] = 1;
    }
    goto L20;

/*  ***  INDICATE WHETHER THE REGRESSION DIAGNOSTIC ARRAY WAS COMPUTED */
/*  ***  AND PRINT IT IF SO REQUESTED... */

L60:
    if (iv[67] > 0) {
	iv[67] = rd1;
    }
    dn2rdp_(&iv[1], liv, lv, n, &v[rd1], &v[1]);

L999:
    return 0;

/*  ***  LAST LINE OF   DN2G FOLLOWS  *** */
} /* dn2g_ */

