/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* drn2g.f -- translated by f2c (version 20090411).
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
static doublereal c_b14 = 0.;
static integer c__0 = 0;
static logical c_true = TRUE_;
static logical c_false = FALSE_;

/* Subroutine */ int drn2g_(doublereal *d__, doublereal *dr, integer *iv, 
	integer *liv, integer *lv, integer *n, integer *nd, integer *n1, 
	integer *n2, integer *p, doublereal *r__, doublereal *rd, doublereal *
	v, doublereal *x)
{
    /* System generated locals */
    integer dr_dim1, dr_offset, i__1;
    doublereal d__1;

    /* Local variables */
    static integer i__, k, l;
    static doublereal t;
    static integer g1, y1, gi, lh, nn, yi, iv1, qtr1, rmat1, jtol1;
    extern /* Subroutine */ int dq7rad_(integer *, integer *, integer *, 
	    doublereal *, logical *, doublereal *, doublereal *, doublereal *)
	    , dn2lrd_(doublereal *, integer *, doublereal *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, doublereal 
	    *, doublereal *, doublereal *), dc7vfn_(integer *, doublereal *, 
	    integer *, integer *, integer *, integer *, integer *, doublereal 
	    *), dd7upd_(doublereal *, doublereal *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    doublereal *), dq7apl_(integer *, integer *, integer *, 
	    doublereal *, doublereal *, integer *), dg7lit_(doublereal *, 
	    doublereal *, integer *, integer *, integer *, integer *, integer 
	    *, doublereal *, doublereal *, doublereal *), dn2cvp_(integer *, 
	    integer *, integer *, integer *, doublereal *);
    extern doublereal dd7tpr_(integer *, doublereal *, doublereal *);
    extern /* Subroutine */ int dl7vml_(integer *, doublereal *, doublereal *,
	     doublereal *), dv7scp_(integer *, doublereal *, doublereal *);
    extern doublereal dv2nrm_(integer *, doublereal *);
    extern /* Subroutine */ int dv7cpy_(integer *, doublereal *, doublereal *)
	    ;
    static integer ivmode;
    extern /* Subroutine */ int divset_(integer *, integer *, integer *, 
	    integer *, doublereal *), ditsum_(doublereal *, doublereal *, 
	    integer *, integer *, integer *, integer *, doublereal *, 
	    doublereal *);


/* *** REVISED ITERATION DRIVER FOR NL2SOL (VERSION 2.3) *** */


/* --------------------------  PARAMETER USAGE  -------------------------- */

/* D........ SCALE VECTOR. */
/* DR....... DERIVATIVES OF R AT X. */
/* IV....... INTEGER VALUES ARRAY. */
/* LIV...... LENGTH OF IV... LIV MUST BE AT LEAST P + 82. */
/* LV....... LENGTH OF V...  LV  MUST BE AT LEAST 105 + P*(2*P+16). */
/* N........ TOTAL NUMBER OF RESIDUALS. */
/* ND....... MAX. NO. OF RESIDUALS PASSED ON ONE CALL. */
/* N1....... LOWEST  ROW INDEX FOR RESIDUALS SUPPLIED THIS TIME. */
/* N2....... HIGHEST ROW INDEX FOR RESIDUALS SUPPLIED THIS TIME. */
/* P........ NUMBER OF PARAMETERS (COMPONENTS OF X) BEING ESTIMATED. */
/* R........ RESIDUALS. */
/* RD....... RD(I) = SQRT(G(I)**T * H(I)**-1 * G(I)) ON OUTPUT WHEN */
/*        IV(RDREQ) IS NONZERO.   DRN2G SETS IV(REGD) = 1 IF RD */
/*        IS SUCCESSFULLY COMPUTED, TO 0 IF NO ATTEMPT WAS MADE */
/*        TO COMPUTE IT, AND TO -1 IF H (THE FINITE-DIFFERENCE HESSIAN) */
/*        WAS INDEFINITE.  IF ND .GE. N, THEN RD IS ALSO USED AS */
/*        TEMPORARY STORAGE. */
/* V........ FLOATING-POINT VALUES ARRAY. */
/* X........ PARAMETER VECTOR BEING ESTIMATED (INPUT = INITIAL GUESS, */
/*             OUTPUT = BEST VALUE FOUND). */

/*  ***  DISCUSSION  *** */

/*  NOTE... NL2SOL AND NL2ITR (MENTIONED BELOW) ARE DESCRIBED IN */
/*  ACM TRANS. MATH. SOFTWARE, VOL. 7, PP. 369-383 (AN ADAPTIVE */
/*  NONLINEAR LEAST-SQUARES ALGORITHM, BY J.E. DENNIS, D.M. GAY, */
/*  AND R.E. WELSCH). */

/*     THIS ROUTINE CARRIES OUT ITERATIONS FOR SOLVING NONLINEAR */
/*  LEAST SQUARES PROBLEMS.  WHEN ND = N, IT IS SIMILAR TO NL2ITR */
/*  (WITH J = DR), EXCEPT THAT R(X) AND DR(X) NEED NOT BE INITIALIZED */
/*  WHEN  DRN2G IS CALLED WITH IV(1) = 0 OR 12.   DRN2G ALSO ALLOWS */
/*  R AND DR TO BE SUPPLIED ROW-WISE -- JUST SET ND = 1 AND CALL */
/*   DRN2G ONCE FOR EACH ROW WHEN PROVIDING RESIDUALS AND JACOBIANS. */
/*     ANOTHER NEW FEATURE IS THAT CALLING  DRN2G WITH IV(1) = 13 */
/*  CAUSES STORAGE ALLOCATION ONLY TO BE PERFORMED -- ON RETURN, SUCH */
/*  COMPONENTS AS IV(G) (THE FIRST SUBSCRIPT IN G OF THE GRADIENT) */
/*  AND IV(S) (THE FIRST SUBSCRIPT IN V OF THE S LOWER TRIANGLE OF */
/*  THE S MATRIX) WILL HAVE BEEN SET (UNLESS LIV OR LV IS TOO SMALL), */
/*  AND IV(1) WILL HAVE BEEN SET TO 14. CALLING  DRN2G WITH IV(1) = 14 */
/*  CAUSES EXECUTION OF THE ALGORITHM TO BEGIN UNDER THE ASSUMPTION */
/*  THAT STORAGE HAS BEEN ALLOCATED. */

/* ***  SUPPLYING R AND DR  *** */

/*      DRN2G USES IV AND V IN THE SAME WAY AS NL2SOL, WITH A SMALL */
/*  NUMBER OF OBVIOUS CHANGES.  ONE DIFFERENCE BETWEEN  DRN2G AND */
/*  NL2ITR IS THAT INITIAL FUNCTION AND GRADIENT INFORMATION NEED NOT */
/*  BE SUPPLIED IN THE VERY FIRST CALL ON  DRN2G, THE ONE WITH */
/*  IV(1) = 0 OR 12.  ANOTHER DIFFERENCE IS THAT  DRN2G RETURNS WITH */
/*  IV(1) = -2 WHEN IT WANTS ANOTHER LOOK AT THE OLD JACOBIAN MATRIX */
/*  AND THE CURRENT RESIDUAL -- THE ONE CORRESPONDING TO X AND */
/*  IV(NFGCAL).  IT THEN RETURNS WITH IV(1) = -3 WHEN IT WANTS TO SEE */
/*  BOTH THE NEW RESIDUAL AND THE NEW JACOBIAN MATRIX AT ONCE.  NOTE */
/*  THAT IV(NFGCAL) = IV(7) CONTAINS THE VALUE THAT IV(NFCALL) = IV(6) */
/*  HAD WHEN THE CURRENT RESIDUAL WAS EVALUATED.  ALSO NOTE THAT THE */
/*  VALUE OF X CORRESPONDING TO THE OLD JACOBIAN MATRIX IS STORED IN */
/*  V, STARTING AT V(IV(X0)) = V(IV(43)). */
/*     ANOTHER NEW RETURN...  DRN2G IV(1) = -1 WHEN IT WANTS BOTH THE */
/*  RESIDUAL AND THE JACOBIAN TO BE EVALUATED AT X. */
/*     A NEW RESIDUAL VECTOR MUST BE SUPPLIED WHEN  DRN2G RETURNS WITH */
/*  IV(1) = 1 OR -1.  THIS TAKES THE FORM OF VALUES OF R(I,X) PASSED */
/*  IN R(I-N1+1), I = N1(1)N2.  YOU MAY PASS ALL THESE VALUES AT ONCE */
/*  (I.E., N1 = 1 AND N2 = N) OR IN PIECES BY MAKING SEVERAL CALLS ON */
/*   DRN2G.  EACH TIME  DRN2G RETURNS WITH IV(1) = 1, N1 WILL HAVE */
/*  BEEN SET TO THE INDEX OF THE NEXT RESIDUAL THAT  DRN2G EXPECTS TO */
/*  SEE, AND N2 WILL BE SET TO THE INDEX OF THE HIGHEST RESIDUAL THAT */
/*  COULD BE GIVEN ON THE NEXT CALL, I.E., N2 = N1 + ND - 1.  (THUS */
/*  WHEN  DRN2G FIRST RETURNS WITH IV(1) = 1 FOR A NEW X, IT WILL */
/*  HAVE SET N1 TO 1 AND N2 TO MIN(ND,N).)  THE CALLER MAY PROVIDE */
/*  FEWER THAN N2-N1+1 RESIDUALS ON THE NEXT CALL BY SETTING N2 TO */
/*  A SMALLER VALUE.   DRN2G ASSUMES IT HAS SEEN ALL THE RESIDUALS */
/*  FOR THE CURRENT X WHEN IT IS CALLED WITH N2 .GE. N. */
/*    EXAMPLE... SUPPOSE N = 80 AND THAT R IS TO BE PASSED IN 8 */
/*  BLOCKS OF SIZE 10.  THE FOLLOWING CODE WOULD DO THE JOB. */

/*      N = 80 */
/*      ND = 10 */
/*      ... */
/*      DO 10 K = 1, 8 */
/*           ***  COMPUTE R(I,X) FOR I = 10*K-9 TO 10*K  *** */
/*           ***  AND STORE THEM IN R(1),...,R(10)  *** */
/*           CALL  DRN2G(..., R, ...) */
/*   10      CONTINUE */

/*     THE SITUATION IS SIMILAR WHEN GRADIENT INFORMATION IS */
/*  REQUIRED, I.E., WHEN  DRN2G RETURNS WITH IV(1) = 2, -1, OR -2. */
/*  NOTE THAT  DRN2G OVERWRITES R, BUT THAT IN THE SPECIAL CASE OF */
/*  N1 = 1 AND N2 = N ON PREVIOUS CALLS,  DRN2G NEVER RETURNS WITH */
/*  IV(1) = -2.  IT SHOULD BE CLEAR THAT THE PARTIAL DERIVATIVE OF */
/*  R(I,X) WITH RESPECT TO X(L) IS TO BE STORED IN DR(I-N1+1,L), */
/*  L = 1(1)P, I = N1(1)N2.  IT IS ESSENTIAL THAT R(I) AND DR(I,L) */
/*  ALL CORRESPOND TO THE SAME RESIDUALS WHEN IV(1) = -1 OR -2. */

/*  ***  COVARIANCE MATRIX  *** */

/*     IV(RDREQ) = IV(57) TELLS WHETHER TO COMPUTE A COVARIANCE */
/*  MATRIX AND/OR REGRESSION DIAGNOSTICS... 0 MEANS NEITHER, */
/*  1 MEANS COVARIANCE MATRIX ONLY, 2 MEANS REG. DIAGNOSTICS ONLY, */
/*  3 MEANS BOTH.  AS WITH NL2SOL, IV(COVREQ) = IV(15) TELLS WHAT */
/*  HESSIAN APPROXIMATION TO USE IN THIS COMPUTING. */

/*  ***  REGRESSION DIAGNOSTICS  *** */

/*     SEE THE COMMENTS IN SUBROUTINE   DN2G. */

/*  ***  GENERAL  *** */

/*     CODED BY DAVID M. GAY. */

/* +++++++++++++++++++++++++++++  DECLARATIONS  ++++++++++++++++++++++++++ */

/*  ***  INTRINSIC FUNCTIONS  *** */
/* /+ */
/* / */
/*  ***  EXTERNAL FUNCTIONS AND SUBROUTINES  *** */


/* DC7VFN... FINISHES COVARIANCE COMPUTATION. */
/* DIVSET.... PROVIDES DEFAULT IV AND V INPUT COMPONENTS. */
/* DD7TPR... COMPUTES INNER PRODUCT OF TWO VECTORS. */
/* DD7UPD...  UPDATES SCALE VECTOR D. */
/* DG7LIT.... PERFORMS BASIC MINIMIZATION ALGORITHM. */
/* DITSUM.... PRINTS ITERATION SUMMARY, INFO ABOUT INITIAL AND FINAL X. */
/* DL7VML.... COMPUTES L * V, V = VECTOR, L = LOWER TRIANGULAR MATRIX. */
/* DN2CVP... PRINTS COVARIANCE MATRIX. */
/* DN2LRD... COMPUTES REGRESSION DIAGNOSTICS. */
/* DQ7APL... APPLIES QR TRANSFORMATIONS STORED BY DQ7RAD. */
/* DQ7RAD.... ADDS A NEW BLOCK OF ROWS TO QR DECOMPOSITION. */
/* DV7CPY.... COPIES ONE VECTOR TO ANOTHER. */
/* DV7SCP... SETS ALL ELEMENTS OF A VECTOR TO A SCALAR. */

/*  ***  LOCAL VARIABLES  *** */



/*  ***  SUBSCRIPTS FOR IV AND V  *** */


/*  ***  IV SUBSCRIPT VALUES  *** */

/* /6 */
/*     DATA CNVCOD/55/, COVMAT/26/, COVREQ/15/, DTYPE/16/, FDH/74/, */
/*    1     G/28/, H/56/, IPIVOT/76/, IVNEED/3/, JCN/66/, JTOL/59/, */
/*    2     LMAT/42/, MODE/35/, NEXTIV/46/, NEXTV/47/, NFCALL/6/, */
/*    3     NFCOV/52/, NF0/68/, NF00/81/, NF1/69/, NFGCAL/7/, NGCALL/30/, */
/*    4     NGCOV/53/, QTR/77/, RESTOR/9/, RMAT/78/, RDREQ/57/, REGD/67/, */
/*    5     TOOBIG/2/, VNEED/4/, Y/48/ */
/* /7 */
/* / */

/*  ***  V SUBSCRIPT VALUES  *** */

/* /6 */
/*     DATA DINIT/38/, DTINIT/39/, D0INIT/40/, F/10/, RLIMIT/46/ */
/* /7 */
/* / */
/* /6 */
/*     DATA HALF/0.5D+0/, ZERO/0.D+0/ */
/* /7 */
/* / */

/* +++++++++++++++++++++++++++++++  BODY  ++++++++++++++++++++++++++++++++ */

    /* Parameter adjustments */
    --iv;
    --v;
    --rd;
    --r__;
    --x;
    dr_dim1 = *nd;
    dr_offset = 1 + dr_dim1;
    dr -= dr_offset;
    --d__;

    /* Function Body */
    lh = *p * (*p + 1) / 2;
    if (iv[1] == 0) {
	divset_(&c__1, &iv[1], liv, lv, &v[1]);
    }
    iv1 = iv[1];
    if (iv1 > 2) {
	goto L10;
    }
    nn = *n2 - *n1 + 1;
    iv[9] = 0;
    i__ = iv1 + 4;
    if (iv[2] == 0) {
	switch (i__) {
	    case 1:  goto L150;
	    case 2:  goto L130;
	    case 3:  goto L150;
	    case 4:  goto L120;
	    case 5:  goto L120;
	    case 6:  goto L150;
	}
    }
    if (i__ != 5) {
	iv[1] = 2;
    }
    goto L40;

/*  ***  FRESH START OR RESTART -- CHECK INPUT INTEGERS  *** */

L10:
    if (*nd <= 0) {
	goto L210;
    }
    if (*p <= 0) {
	goto L210;
    }
    if (*n <= 0) {
	goto L210;
    }
    if (iv1 == 14) {
	goto L30;
    }
    if (iv1 > 16) {
	goto L300;
    }
    if (iv1 < 12) {
	goto L40;
    }
    if (iv1 == 12) {
	iv[1] = 13;
    }
    if (iv[1] != 13) {
	goto L20;
    }
    iv[3] += *p;
    iv[4] += *p * (*p + 13) / 2;
L20:
    dg7lit_(&d__[1], &x[1], &iv[1], liv, lv, p, p, &v[1], &x[1], &x[1]);
    if (iv[1] != 14) {
	goto L999;
    }

/*  ***  STORAGE ALLOCATION  *** */

    iv[76] = iv[46];
    iv[46] = iv[76] + *p;
    iv[48] = iv[47];
    iv[28] = iv[48] + *p;
    iv[66] = iv[28] + *p;
    iv[78] = iv[66] + *p;
    iv[77] = iv[78] + lh;
    iv[59] = iv[77] + *p;
    iv[47] = iv[59] + (*p << 1);
    if (iv1 == 13) {
	goto L999;
    }

L30:
    jtol1 = iv[59];
    if (v[38] >= 0.) {
	dv7scp_(p, &d__[1], &v[38]);
    }
    if (v[39] > 0.) {
	dv7scp_(p, &v[jtol1], &v[39]);
    }
    i__ = jtol1 + *p;
    if (v[40] > 0.) {
	dv7scp_(p, &v[i__], &v[40]);
    }
    iv[68] = 0;
    iv[69] = 0;
    if (*nd >= *n) {
	goto L40;
    }

/*  ***  SPECIAL CASE HANDLING OF FIRST FUNCTION AND GRADIENT EVALUATION */
/*  ***  -- ASK FOR BOTH RESIDUAL AND JACOBIAN AT ONCE */

    g1 = iv[28];
    y1 = iv[48];
    dg7lit_(&d__[1], &v[g1], &iv[1], liv, lv, p, p, &v[1], &x[1], &v[y1]);
    if (iv[1] != 1) {
	goto L220;
    }
    v[10] = 0.;
    dv7scp_(p, &v[g1], &c_b14);
    iv[1] = -1;
    qtr1 = iv[77];
    dv7scp_(p, &v[qtr1], &c_b14);
    iv[67] = 0;
    rmat1 = iv[78];
    goto L100;

L40:
    g1 = iv[28];
    y1 = iv[48];
    dg7lit_(&d__[1], &v[g1], &iv[1], liv, lv, p, p, &v[1], &x[1], &v[y1]);
    if ((i__1 = iv[1] - 2) < 0) {
	goto L50;
    } else if (i__1 == 0) {
	goto L60;
    } else {
	goto L220;
    }

L50:
    v[10] = 0.;
    if (iv[69] == 0) {
	goto L260;
    }
    if (iv[9] != 2) {
	goto L260;
    }
    iv[68] = iv[69];
    dv7cpy_(n, &rd[1], &r__[1]);
    iv[67] = 0;
    goto L260;

L60:
    dv7scp_(p, &v[g1], &c_b14);
    if (iv[35] > 0) {
	goto L230;
    }
    rmat1 = iv[78];
    qtr1 = iv[77];
    dv7scp_(p, &v[qtr1], &c_b14);
    iv[67] = 0;
    if (*nd < *n) {
	goto L90;
    }
    if (*n1 != 1) {
	goto L90;
    }
    if (iv[35] < 0) {
	goto L100;
    }
    if (iv[69] == iv[7]) {
	goto L70;
    }
    if (iv[68] != iv[7]) {
	goto L90;
    }
    dv7cpy_(n, &r__[1], &rd[1]);
    goto L80;
L70:
    dv7cpy_(n, &rd[1], &r__[1]);
L80:
    dq7apl_(nd, n, p, &dr[dr_offset], &rd[1], &c__0);
    dl7vml_(p, &v[y1], &v[rmat1], &rd[1]);
    goto L110;

L90:
    iv[1] = -2;
    if (iv[35] < 0) {
	iv[1] = -1;
    }
L100:
    dv7scp_(p, &v[y1], &c_b14);
L110:
    dv7scp_(&lh, &v[rmat1], &c_b14);
    goto L260;

/*  ***  COMPUTE F(X)  *** */

L120:
    t = dv2nrm_(&nn, &r__[1]);
    if (t > v[46]) {
	goto L200;
    }
/* Computing 2nd power */
    d__1 = t;
    v[10] += d__1 * d__1 * .5;
    if (*n2 < *n) {
	goto L270;
    }
    if (*n1 == 1) {
	iv[69] = iv[6];
    }
    goto L40;

/*  ***  COMPUTE Y  *** */

L130:
    y1 = iv[48];
    yi = y1;
    i__1 = *p;
    for (l = 1; l <= i__1; ++l) {
	v[yi] += dd7tpr_(&nn, &dr[l * dr_dim1 + 1], &r__[1]);
	++yi;
/* L140: */
    }
    if (*n2 < *n) {
	goto L270;
    }
    iv[1] = 2;
    if (*n1 > 1) {
	iv[1] = -3;
    }
    goto L260;

/*  ***  COMPUTE GRADIENT INFORMATION  *** */

L150:
    if (iv[35] > *p) {
	goto L240;
    }
    g1 = iv[28];
    ivmode = iv[35];
    if (ivmode < 0) {
	goto L170;
    }
    if (ivmode == 0) {
	goto L180;
    }
    iv[1] = 2;

/*  ***  COMPUTE GRADIENT ONLY (FOR USE IN COVARIANCE COMPUTATION)  *** */

    gi = g1;
    i__1 = *p;
    for (l = 1; l <= i__1; ++l) {
	v[gi] += dd7tpr_(&nn, &r__[1], &dr[l * dr_dim1 + 1]);
	++gi;
/* L160: */
    }
    goto L190;

/*  *** COMPUTE INITIAL FUNCTION VALUE WHEN ND .LT. N *** */

L170:
    if (*n <= *nd) {
	goto L180;
    }
    t = dv2nrm_(&nn, &r__[1]);
    if (t > v[46]) {
	goto L200;
    }
/* Computing 2nd power */
    d__1 = t;
    v[10] += d__1 * d__1 * .5;

/*  ***  UPDATE D IF DESIRED  *** */

L180:
    if (iv[16] > 0) {
	dd7upd_(&d__[1], &dr[dr_offset], &iv[1], liv, lv, n, nd, &nn, n2, p, &
		v[1]);
    }

/*  ***  COMPUTE RMAT AND QTR  *** */

    qtr1 = iv[77];
    rmat1 = iv[78];
    dq7rad_(&nn, nd, p, &v[qtr1], &c_true, &v[rmat1], &dr[dr_offset], &r__[1])
	    ;
    iv[69] = 0;

L190:
    if (*n2 < *n) {
	goto L270;
    }
    if (ivmode > 0) {
	goto L40;
    }
    iv[81] = iv[7];

/*  ***  COMPUTE G FROM RMAT AND QTR  *** */

    dl7vml_(p, &v[g1], &v[rmat1], &v[qtr1]);
    iv[1] = 2;
    if (ivmode == 0) {
	goto L40;
    }
    if (*n <= *nd) {
	goto L40;
    }

/*  ***  FINISH SPECIAL CASE HANDLING OF FIRST FUNCTION AND GRADIENT */

    y1 = iv[48];
    iv[1] = 1;
    dg7lit_(&d__[1], &v[g1], &iv[1], liv, lv, p, p, &v[1], &x[1], &v[y1]);
    if (iv[1] != 2) {
	goto L220;
    }
    goto L40;

/*  ***  MISC. DETAILS  *** */

/*     ***  X IS OUT OF RANGE (OVERSIZE STEP)  *** */

L200:
    iv[2] = 1;
    goto L40;

/*     ***  BAD N, ND, OR P  *** */

L210:
    iv[1] = 66;
    goto L300;

/*  ***  CONVERGENCE OBTAINED -- SEE WHETHER TO COMPUTE COVARIANCE  *** */

L220:
    if (iv[26] != 0) {
	goto L290;
    }
    if (iv[67] != 0) {
	goto L290;
    }

/*     ***  SEE IF CHOLESKY FACTOR OF HESSIAN IS AVAILABLE  *** */

    k = iv[74];
    if (k <= 0) {
	goto L280;
    }
    if (iv[57] <= 0) {
	goto L290;
    }

/*     ***  COMPUTE REGRESSION DIAGNOSTICS AND DEFAULT COVARIANCE IF */
/*          DESIRED  *** */

    i__ = 0;
    if (iv[57] % 4 >= 2) {
	i__ = 1;
    }
    if (iv[57] % 2 == 1 && abs(iv[15]) <= 1) {
	i__ += 2;
    }
    if (i__ == 0) {
	goto L250;
    }
    iv[35] = *p + i__;
    ++iv[30];
    ++iv[53];
    iv[55] = iv[1];
    if (i__ < 2) {
	goto L230;
    }
    l = abs(iv[56]);
    dv7scp_(&lh, &v[l], &c_b14);
L230:
    ++iv[52];
    ++iv[6];
    iv[7] = iv[6];
    iv[1] = -1;
    goto L260;

L240:
    l = iv[42];
    dn2lrd_(&dr[dr_offset], &iv[1], &v[l], &lh, liv, lv, nd, &nn, p, &r__[1], 
	    &rd[1], &v[1]);
    if (*n2 < *n) {
	goto L270;
    }
    if (*n1 > 1) {
	goto L250;
    }

/*     ***  ENSURE WE CAN RESTART -- AND MAKE RETURN STATE OF DR */
/*     ***  INDEPENDENT OF WHETHER REGRESSION DIAGNOSTICS ARE COMPUTED. */
/*     ***  USE STEP VECTOR (ALLOCATED BY DG7LIT) FOR SCRATCH. */

    rmat1 = iv[78];
    dv7scp_(&lh, &v[rmat1], &c_b14);
    dq7rad_(&nn, nd, p, &r__[1], &c_false, &v[rmat1], &dr[dr_offset], &r__[1])
	    ;
    iv[69] = 0;

/*  ***  FINISH COMPUTING COVARIANCE  *** */

L250:
    l = iv[42];
    dc7vfn_(&iv[1], &v[l], &lh, liv, lv, n, p, &v[1]);
    goto L290;

/*  ***  RETURN FOR MORE FUNCTION OR GRADIENT INFORMATION  *** */

L260:
    *n2 = 0;
L270:
    *n1 = *n2 + 1;
    *n2 += *nd;
    if (*n2 > *n) {
	*n2 = *n;
    }
    goto L999;

/*  ***  COME HERE FOR INDEFINITE FINITE-DIFFERENCE HESSIAN  *** */

L280:
    iv[26] = k;
    iv[67] = k;

/*  ***  PRINT SUMMARY OF FINAL ITERATION AND OTHER REQUESTED ITEMS  *** */

L290:
    g1 = iv[28];
L300:
    ditsum_(&d__[1], &v[g1], &iv[1], liv, lv, p, &v[1], &x[1]);
    if (iv[1] <= 6 && iv[57] > 0) {
	dn2cvp_(&iv[1], liv, lv, p, &v[1]);
    }

L999:
    return 0;
/*  ***  LAST LINE OF  DRN2G FOLLOWS  *** */
} /* drn2g_ */

