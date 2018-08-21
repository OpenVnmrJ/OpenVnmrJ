/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* da7sst.f -- translated by f2c (version 20090411).
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

/* Subroutine */ int da7sst_(integer *iv, integer *liv, integer *lv, 
	doublereal *v)
{
    /* System generated locals */
    doublereal d__1, d__2;

    /* Local variables */
    static integer i__, nfc;
    static doublereal gts, emax, xmax, rfac1, emaxs;
    static logical goodx;


/*  ***  ASSESS CANDIDATE STEP (***SOL VERSION 2.3)  *** */


/*  ***  PURPOSE  *** */

/*        THIS SUBROUTINE IS CALLED BY AN UNCONSTRAINED MINIMIZATION */
/*     ROUTINE TO ASSESS THE NEXT CANDIDATE STEP.  IT MAY RECOMMEND ONE */
/*     OF SEVERAL COURSES OF ACTION, SUCH AS ACCEPTING THE STEP, RECOM- */
/*     PUTING IT USING THE SAME OR A NEW QUADRATIC MODEL, OR HALTING DUE */
/*     TO CONVERGENCE OR FALSE CONVERGENCE.  SEE THE RETURN CODE LISTING */
/*     BELOW. */

/* --------------------------  PARAMETER USAGE  -------------------------- */

/*  IV (I/O) INTEGER PARAMETER AND SCRATCH VECTOR -- SEE DESCRIPTION */
/*             BELOW OF IV VALUES REFERENCED. */
/* LIV (IN)  LENGTH OF IV ARRAY. */
/*  LV (IN)  LENGTH OF V ARRAY. */
/*   V (I/O) REAL PARAMETER AND SCRATCH VECTOR -- SEE DESCRIPTION */
/*             BELOW OF V VALUES REFERENCED. */

/*  ***  IV VALUES REFERENCED  *** */

/*    IV(IRC) (I/O) ON INPUT FOR THE FIRST STEP TRIED IN A NEW ITERATION, */
/*             IV(IRC) SHOULD BE SET TO 3 OR 4 (THE VALUE TO WHICH IT IS */
/*             SET WHEN STEP IS DEFINITELY TO BE ACCEPTED).  ON INPUT */
/*             AFTER STEP HAS BEEN RECOMPUTED, IV(IRC) SHOULD BE */
/*             UNCHANGED SINCE THE PREVIOUS RETURN OF DA7SST. */
/*                ON OUTPUT, IV(IRC) IS A RETURN CODE HAVING ONE OF THE */
/*             FOLLOWING VALUES... */
/*                  1 = SWITCH MODELS OR TRY SMALLER STEP. */
/*                  2 = SWITCH MODELS OR ACCEPT STEP. */
/*                  3 = ACCEPT STEP AND DETERMINE V(RADFAC) BY GRADIENT */
/*                       TESTS. */
/*                  4 = ACCEPT STEP, V(RADFAC) HAS BEEN DETERMINED. */
/*                  5 = RECOMPUTE STEP (USING THE SAME MODEL). */
/*                  6 = RECOMPUTE STEP WITH RADIUS = V(LMAXS) BUT DO NOT */
/*                       EVALUATE THE OBJECTIVE FUNCTION. */
/*                  7 = X-CONVERGENCE (SEE V(XCTOL)). */
/*                  8 = RELATIVE FUNCTION CONVERGENCE (SEE V(RFCTOL)). */
/*                  9 = BOTH X- AND RELATIVE FUNCTION CONVERGENCE. */
/*                 10 = ABSOLUTE FUNCTION CONVERGENCE (SEE V(AFCTOL)). */
/*                 11 = SINGULAR CONVERGENCE (SEE V(LMAXS)). */
/*                 12 = FALSE CONVERGENCE (SEE V(XFTOL)). */
/*                 13 = IV(IRC) WAS OUT OF RANGE ON INPUT. */
/*             RETURN CODE I HAS PRECEDENCE OVER I+1 FOR I = 9, 10, 11. */
/* IV(MLSTGD) (I/O) SAVED VALUE OF IV(MODEL). */
/*  IV(MODEL) (I/O) ON INPUT, IV(MODEL) SHOULD BE AN INTEGER IDENTIFYING */
/*             THE CURRENT QUADRATIC MODEL OF THE OBJECTIVE FUNCTION. */
/*             IF A PREVIOUS STEP YIELDED A BETTER FUNCTION REDUCTION, */
/*             THEN IV(MODEL) WILL BE SET TO IV(MLSTGD) ON OUTPUT. */
/* IV(NFCALL) (IN)  INVOCATION COUNT FOR THE OBJECTIVE FUNCTION. */
/* IV(NFGCAL) (I/O) VALUE OF IV(NFCALL) AT STEP THAT GAVE THE BIGGEST */
/*             FUNCTION REDUCTION THIS ITERATION.  IV(NFGCAL) REMAINS */
/*             UNCHANGED UNTIL A FUNCTION REDUCTION IS OBTAINED. */
/* IV(RADINC) (I/O) THE NUMBER OF RADIUS INCREASES (OR MINUS THE NUMBER */
/*             OF DECREASES) SO FAR THIS ITERATION. */
/* IV(RESTOR) (OUT) SET TO 1 IF V(F) HAS BEEN RESTORED AND X SHOULD BE */
/*             RESTORED TO ITS INITIAL VALUE, TO 2 IF X SHOULD BE SAVED, */
/*             TO 3 IF X SHOULD BE RESTORED FROM THE SAVED VALUE, AND TO */
/*             0 OTHERWISE. */
/*  IV(STAGE) (I/O) COUNT OF THE NUMBER OF MODELS TRIED SO FAR IN THE */
/*             CURRENT ITERATION. */
/* IV(STGLIM) (IN)  MAXIMUM NUMBER OF MODELS TO CONSIDER. */
/* IV(SWITCH) (OUT) SET TO 0 UNLESS A NEW MODEL IS BEING TRIED AND IT */
/*             GIVES A SMALLER FUNCTION VALUE THAN THE PREVIOUS MODEL, */
/*             IN WHICH CASE DA7SST SETS IV(SWITCH) = 1. */
/* IV(TOOBIG) (I/O)  IS NONZERO ON INPUT IF STEP WAS TOO BIG (E.G., IF */
/*             IT WOULD CAUSE OVERFLOW).  IT IS SET TO 0 ON RETURN. */
/*   IV(XIRC) (I/O) VALUE THAT IV(IRC) WOULD HAVE IN THE ABSENCE OF */
/*             CONVERGENCE, FALSE CONVERGENCE, AND OVERSIZED STEPS. */

/*  ***  V VALUES REFERENCED  *** */

/* V(AFCTOL) (IN)  ABSOLUTE FUNCTION CONVERGENCE TOLERANCE.  IF THE */
/*             ABSOLUTE VALUE OF THE CURRENT FUNCTION VALUE V(F) IS LESS */
/*             THAN V(AFCTOL) AND DA7SST DOES NOT RETURN WITH */
/*             IV(IRC) = 11, THEN DA7SST RETURNS WITH IV(IRC) = 10. */
/* V(DECFAC) (IN)  FACTOR BY WHICH TO DECREASE RADIUS WHEN IV(TOOBIG) IS */
/*             NONZERO. */
/* V(DSTNRM) (IN)  THE 2-NORM OF D*STEP. */
/* V(DSTSAV) (I/O) VALUE OF V(DSTNRM) ON SAVED STEP. */
/*   V(DST0) (IN)  THE 2-NORM OF D TIMES THE NEWTON STEP (WHEN DEFINED, */
/*             I.E., FOR V(NREDUC) .GE. 0). */
/*      V(F) (I/O) ON BOTH INPUT AND OUTPUT, V(F) IS THE OBJECTIVE FUNC- */
/*             TION VALUE AT X.  IF X IS RESTORED TO A PREVIOUS VALUE, */
/*             THEN V(F) IS RESTORED TO THE CORRESPONDING VALUE. */
/*   V(FDIF) (OUT) THE FUNCTION REDUCTION V(F0) - V(F) (FOR THE OUTPUT */
/*             VALUE OF V(F) IF AN EARLIER STEP GAVE A BIGGER FUNCTION */
/*             DECREASE, AND FOR THE INPUT VALUE OF V(F) OTHERWISE). */
/* V(FLSTGD) (I/O) SAVED VALUE OF V(F). */
/*     V(F0) (IN)  OBJECTIVE FUNCTION VALUE AT START OF ITERATION. */
/* V(GTSLST) (I/O) VALUE OF V(GTSTEP) ON SAVED STEP. */
/* V(GTSTEP) (IN)  INNER PRODUCT BETWEEN STEP AND GRADIENT. */
/* V(INCFAC) (IN)  MINIMUM FACTOR BY WHICH TO INCREASE RADIUS. */
/*  V(LMAXS) (IN)  MAXIMUM REASONABLE STEP SIZE (AND INITIAL STEP BOUND). */
/*             IF THE ACTUAL FUNCTION DECREASE IS NO MORE THAN TWICE */
/*             WHAT WAS PREDICTED, IF A RETURN WITH IV(IRC) = 7, 8, OR 9 */
/*             DOES NOT OCCUR, IF V(DSTNRM) .GT. V(LMAXS) OR THE CURRENT */
/*             STEP IS A NEWTON STEP, AND IF */
/*             V(PREDUC) .LE. V(SCTOL) * ABS(V(F0)), THEN DA7SST RETURNS */
/*             WITH IV(IRC) = 11.  IF SO DOING APPEARS WORTHWHILE, THEN */
/*            DA7SST REPEATS THIS TEST (DISALLOWING A FULL NEWTON STEP) */
/*             WITH V(PREDUC) COMPUTED FOR A STEP OF LENGTH V(LMAXS) */
/*             (BY A RETURN WITH IV(IRC) = 6). */
/* V(NREDUC) (I/O)  FUNCTION REDUCTION PREDICTED BY QUADRATIC MODEL FOR */
/*             NEWTON STEP.  IF DA7SST IS CALLED WITH IV(IRC) = 6, I.E., */
/*             IF V(PREDUC) HAS BEEN COMPUTED WITH RADIUS = V(LMAXS) FOR */
/*             USE IN THE SINGULAR CONVERGENCE TEST, THEN V(NREDUC) IS */
/*             SET TO -V(PREDUC) BEFORE THE LATTER IS RESTORED. */
/* V(PLSTGD) (I/O) VALUE OF V(PREDUC) ON SAVED STEP. */
/* V(PREDUC) (I/O) FUNCTION REDUCTION PREDICTED BY QUADRATIC MODEL FOR */
/*             CURRENT STEP. */
/* V(RADFAC) (OUT) FACTOR TO BE USED IN DETERMINING THE NEW RADIUS, */
/*             WHICH SHOULD BE V(RADFAC)*DST, WHERE  DST  IS EITHER THE */
/*             OUTPUT VALUE OF V(DSTNRM) OR THE 2-NORM OF */
/*             DIAG(NEWD)*STEP  FOR THE OUTPUT VALUE OF STEP AND THE */
/*             UPDATED VERSION, NEWD, OF THE SCALE VECTOR D.  FOR */
/*             IV(IRC) = 3, V(RADFAC) = 1.0 IS RETURNED. */
/* V(RDFCMN) (IN)  MINIMUM VALUE FOR V(RADFAC) IN TERMS OF THE INPUT */
/*             VALUE OF V(DSTNRM) -- SUGGESTED VALUE = 0.1. */
/* V(RDFCMX) (IN)  MAXIMUM VALUE FOR V(RADFAC) -- SUGGESTED VALUE = 4.0. */
/*  V(RELDX) (IN) SCALED RELATIVE CHANGE IN X CAUSED BY STEP, COMPUTED */
/*             (E.G.) BY FUNCTION  DRLDST  AS */
/*                 MAX (D(I)*ABS(X(I)-X0(I)), 1 .LE. I .LE. P) / */
/*                    MAX (D(I)*(ABS(X(I))+ABS(X0(I))), 1 .LE. I .LE. P). */
/* V(RFCTOL) (IN)  RELATIVE FUNCTION CONVERGENCE TOLERANCE.  IF THE */
/*             ACTUAL FUNCTION REDUCTION IS AT MOST TWICE WHAT WAS PRE- */
/*             DICTED AND  V(NREDUC) .LE. V(RFCTOL)*ABS(V(F0)),  THEN */
/*            DA7SST RETURNS WITH IV(IRC) = 8 OR 9. */
/*  V(SCTOL) (IN)  SINGULAR CONVERGENCE TOLERANCE -- SEE V(LMAXS). */
/* V(STPPAR) (IN)  MARQUARDT PARAMETER -- 0 MEANS FULL NEWTON STEP. */
/* V(TUNER1) (IN)  TUNING CONSTANT USED TO DECIDE IF THE FUNCTION */
/*             REDUCTION WAS MUCH LESS THAN EXPECTED.  SUGGESTED */
/*             VALUE = 0.1. */
/* V(TUNER2) (IN)  TUNING CONSTANT USED TO DECIDE IF THE FUNCTION */
/*             REDUCTION WAS LARGE ENOUGH TO ACCEPT STEP.  SUGGESTED */
/*             VALUE = 10**-4. */
/* V(TUNER3) (IN)  TUNING CONSTANT USED TO DECIDE IF THE RADIUS */
/*             SHOULD BE INCREASED.  SUGGESTED VALUE = 0.75. */
/*  V(XCTOL) (IN)  X-CONVERGENCE CRITERION.  IF STEP IS A NEWTON STEP */
/*             (V(STPPAR) = 0) HAVING V(RELDX) .LE. V(XCTOL) AND GIVING */
/*             AT MOST TWICE THE PREDICTED FUNCTION DECREASE, THEN */
/*            DA7SST RETURNS IV(IRC) = 7 OR 9. */
/*  V(XFTOL) (IN)  FALSE CONVERGENCE TOLERANCE.  IF STEP GAVE NO OR ONLY */
/*             A SMALL FUNCTION DECREASE AND V(RELDX) .LE. V(XFTOL), */
/*             THEN DA7SST RETURNS WITH IV(IRC) = 12. */

/* -------------------------------  NOTES  ------------------------------- */

/*  ***  APPLICATION AND USAGE RESTRICTIONS  *** */

/*        THIS ROUTINE IS CALLED AS PART OF THE NL2SOL (NONLINEAR */
/*     LEAST-SQUARES) PACKAGE.  IT MAY BE USED IN ANY UNCONSTRAINED */
/*     MINIMIZATION SOLVER THAT USES DOGLEG, GOLDFELD-QUANDT-TROTTER, */
/*     OR LEVENBERG-MARQUARDT STEPS. */

/*  ***  ALGORITHM NOTES  *** */

/*        SEE (1) FOR FURTHER DISCUSSION OF THE ASSESSING AND MODEL */
/*     SWITCHING STRATEGIES.  WHILE NL2SOL CONSIDERS ONLY TWO MODELS, */
/*    DA7SST IS DESIGNED TO HANDLE ANY NUMBER OF MODELS. */

/*  ***  USAGE NOTES  *** */

/*        ON THE FIRST CALL OF AN ITERATION, ONLY THE I/O VARIABLES */
/*     STEP, X, IV(IRC), IV(MODEL), V(F), V(DSTNRM), V(GTSTEP), AND */
/*     V(PREDUC) NEED HAVE BEEN INITIALIZED.  BETWEEN CALLS, NO I/O */
/*     VALUES EXCEPT STEP, X, IV(MODEL), V(F) AND THE STOPPING TOLER- */
/*     ANCES SHOULD BE CHANGED. */
/*        AFTER A RETURN FOR CONVERGENCE OR FALSE CONVERGENCE, ONE CAN */
/*     CHANGE THE STOPPING TOLERANCES AND CALL DA7SST AGAIN, IN WHICH */
/*     CASE THE STOPPING TESTS WILL BE REPEATED. */

/*  ***  REFERENCES  *** */

/*     (1) DENNIS, J.E., JR., GAY, D.M., AND WELSCH, R.E. (1981), */
/*        AN ADAPTIVE NONLINEAR LEAST-SQUARES ALGORITHM, */
/*        ACM TRANS. MATH. SOFTWARE, VOL. 7, NO. 3. */

/*     (2) POWELL, M.J.D. (1970)  A FORTRAN SUBROUTINE FOR SOLVING */
/*        SYSTEMS OF NONLINEAR ALGEBRAIC EQUATIONS, IN NUMERICAL */
/*        METHODS FOR NONLINEAR ALGEBRAIC EQUATIONS, EDITED BY */
/*        P. RABINOWITZ, GORDON AND BREACH, LONDON. */

/*  ***  HISTORY  *** */

/*        JOHN DENNIS DESIGNED MUCH OF THIS ROUTINE, STARTING WITH */
/*     IDEAS IN (2). ROY WELSCH SUGGESTED THE MODEL SWITCHING STRATEGY. */
/*        DAVID GAY AND STEPHEN PETERS CAST THIS SUBROUTINE INTO A MORE */
/*     PORTABLE FORM (WINTER 1977), AND DAVID GAY CAST IT INTO ITS */
/*     PRESENT FORM (FALL 1978), WITH MINOR CHANGES TO THE SINGULAR */
/*     CONVERGENCE TEST IN MAY, 1984 (TO DEAL WITH FULL NEWTON STEPS). */

/*  ***  GENERAL  *** */

/*     THIS SUBROUTINE WAS WRITTEN IN CONNECTION WITH RESEARCH */
/*     SUPPORTED BY THE NATIONAL SCIENCE FOUNDATION UNDER GRANTS */
/*     MCS-7600324, DCR75-10143, 76-14311DSS, MCS76-11989, AND */
/*     MCS-7906671. */

/* ------------------------  EXTERNAL QUANTITIES  ------------------------ */

/*  ***  NO EXTERNAL FUNCTIONS AND SUBROUTINES  *** */

/* --------------------------  LOCAL VARIABLES  -------------------------- */


/*  ***  SUBSCRIPTS FOR IV AND V  *** */


/*  ***  DATA INITIALIZATIONS  *** */

/* /6 */
/*     DATA HALF/0.5D+0/, ONE/1.D+0/, ONEP2/1.2D+0/, TWO/2.D+0/, */
/*    1     ZERO/0.D+0/ */
/* /7 */
/* / */

/* /6 */
/*     DATA IRC/29/, MLSTGD/32/, MODEL/5/, NFCALL/6/, NFGCAL/7/, */
/*    1     RADINC/8/, RESTOR/9/, STAGE/10/, STGLIM/11/, SWITCH/12/, */
/*    2     TOOBIG/2/, XIRC/13/ */
/* /7 */
/* / */
/* /6 */
/*     DATA AFCTOL/31/, DECFAC/22/, DSTNRM/2/, DST0/3/, DSTSAV/18/, */
/*    1     F/10/, FDIF/11/, FLSTGD/12/, F0/13/, GTSLST/14/, GTSTEP/4/, */
/*    2     INCFAC/23/, LMAXS/36/, NREDUC/6/, PLSTGD/15/, PREDUC/7/, */
/*    3     RADFAC/16/, RDFCMN/24/, RDFCMX/25/, RELDX/17/, RFCTOL/32/, */
/*    4     SCTOL/37/, STPPAR/5/, TUNER1/26/, TUNER2/27/, TUNER3/28/, */
/*    5     XCTOL/33/, XFTOL/34/ */
/* /7 */
/* / */

/* +++++++++++++++++++++++++++++++  BODY  ++++++++++++++++++++++++++++++++ */

    /* Parameter adjustments */
    --iv;
    --v;

    /* Function Body */
    nfc = iv[6];
    iv[12] = 0;
    iv[9] = 0;
    rfac1 = 1.;
    goodx = TRUE_;
    i__ = iv[29];
    if (i__ >= 1 && i__ <= 12) {
	switch (i__) {
	    case 1:  goto L20;
	    case 2:  goto L30;
	    case 3:  goto L10;
	    case 4:  goto L10;
	    case 5:  goto L40;
	    case 6:  goto L280;
	    case 7:  goto L220;
	    case 8:  goto L220;
	    case 9:  goto L220;
	    case 10:  goto L220;
	    case 11:  goto L220;
	    case 12:  goto L170;
	}
    }
    iv[29] = 13;
    goto L999;

/*  ***  INITIALIZE FOR NEW ITERATION  *** */

L10:
    iv[10] = 1;
    iv[8] = 0;
    v[12] = v[13];
    if (iv[2] == 0) {
	goto L110;
    }
    iv[10] = -1;
    iv[13] = i__;
    goto L60;

/*  ***  STEP WAS RECOMPUTED WITH NEW MODEL OR SMALLER RADIUS  *** */
/*  ***  FIRST DECIDE WHICH  *** */

L20:
    if (iv[5] != iv[32]) {
	goto L30;
    }
/*        ***  OLD MODEL RETAINED, SMALLER RADIUS TRIED  *** */
/*        ***  DO NOT CONSIDER ANY MORE NEW MODELS THIS ITERATION  *** */
    iv[10] = iv[11];
    iv[8] = -1;
    goto L110;

/*  ***  A NEW MODEL IS BEING TRIED.  DECIDE WHETHER TO KEEP IT.  *** */

L30:
    ++iv[10];

/*     ***  NOW WE ADD THE POSSIBILITY THAT STEP WAS RECOMPUTED WITH  *** */
/*     ***  THE SAME MODEL, PERHAPS BECAUSE OF AN OVERSIZED STEP.     *** */

L40:
    if (iv[10] > 0) {
	goto L50;
    }

/*        ***  STEP WAS RECOMPUTED BECAUSE IT WAS TOO BIG.  *** */

    if (iv[2] != 0) {
	goto L60;
    }

/*        ***  RESTORE IV(STAGE) AND PICK UP WHERE WE LEFT OFF.  *** */

    iv[10] = -iv[10];
    i__ = iv[13];
    switch (i__) {
	case 1:  goto L20;
	case 2:  goto L30;
	case 3:  goto L110;
	case 4:  goto L110;
	case 5:  goto L70;
    }

L50:
    if (iv[2] == 0) {
	goto L70;
    }

/*  ***  HANDLE OVERSIZE STEP  *** */

    iv[2] = 0;
    if (iv[8] > 0) {
	goto L80;
    }
    iv[10] = -iv[10];
    iv[13] = iv[29];

L60:
    iv[2] = 0;
    v[16] = v[22];
    --iv[8];
    iv[29] = 5;
    iv[9] = 1;
    v[10] = v[12];
    goto L999;

L70:
    if (v[10] < v[12]) {
	goto L110;
    }

/*     *** THE NEW STEP IS A LOSER.  RESTORE OLD MODEL.  *** */

    if (iv[5] == iv[32]) {
	goto L80;
    }
    iv[5] = iv[32];
    iv[12] = 1;

/*     ***  RESTORE STEP, ETC. ONLY IF A PREVIOUS STEP DECREASED V(F). */

L80:
    if (v[12] >= v[13]) {
	goto L110;
    }
    if (iv[10] < iv[11]) {
	goodx = FALSE_;
    } else if (nfc < iv[7] + iv[11] + 2) {
	goodx = FALSE_;
    } else if (iv[12] != 0) {
	goodx = FALSE_;
    }
    iv[9] = 3;
    v[10] = v[12];
    v[7] = v[15];
    v[4] = v[14];
    if (iv[12] == 0) {
	rfac1 = v[2] / v[18];
    }
    v[2] = v[18];
    if (goodx) {

/*     ***  ACCEPT PREVIOUS SLIGHTLY REDUCING STEP *** */

	v[11] = v[13] - v[10];
	iv[29] = 4;
	v[16] = rfac1;
	goto L999;
    }
    nfc = iv[7];

L110:
    v[11] = v[13] - v[10];
    if (v[11] > v[27] * v[7]) {
	goto L140;
    }
    if (iv[8] > 0) {
	goto L140;
    }

/*        ***  NO (OR ONLY A TRIVIAL) FUNCTION DECREASE */
/*        ***  -- SO TRY NEW MODEL OR SMALLER RADIUS */

    if (v[10] < v[13]) {
	goto L120;
    }
    iv[32] = iv[5];
    v[12] = v[10];
    v[10] = v[13];
    iv[9] = 1;
    goto L130;
L120:
    iv[7] = nfc;
L130:
    iv[29] = 1;
    if (iv[10] < iv[11]) {
	goto L160;
    }
    iv[29] = 5;
    --iv[8];
    goto L160;

/*  ***  NONTRIVIAL FUNCTION DECREASE ACHIEVED  *** */

L140:
    iv[7] = nfc;
    rfac1 = 1.;
    v[18] = v[2];
    if (v[11] > v[7] * v[26]) {
	goto L190;
    }

/*  ***  DECREASE WAS MUCH LESS THAN PREDICTED -- EITHER CHANGE MODELS */
/*  ***  OR ACCEPT STEP WITH DECREASED RADIUS. */

    if (iv[10] >= iv[11]) {
	goto L150;
    }
/*        ***  CONSIDER SWITCHING MODELS  *** */
    iv[29] = 2;
    goto L160;

/*     ***  ACCEPT STEP WITH DECREASED RADIUS  *** */

L150:
    iv[29] = 4;

/*  ***  SET V(RADFAC) TO FLETCHER*S DECREASE FACTOR  *** */

L160:
    iv[13] = iv[29];
    emax = v[4] + v[11];
    v[16] = rfac1 * .5;
    if (emax < v[4]) {
/* Computing MAX */
	d__1 = v[24], d__2 = v[4] * .5 / emax;
	v[16] = rfac1 * max(d__1,d__2);
    }

/*  ***  DO FALSE CONVERGENCE TEST  *** */

L170:
    if (v[17] <= v[34]) {
	goto L180;
    }
    iv[29] = iv[13];
    if (v[10] < v[13]) {
	goto L200;
    }
    goto L230;

L180:
    iv[29] = 12;
    goto L240;

/*  ***  HANDLE GOOD FUNCTION DECREASE  *** */

L190:
    if (v[11] < -v[28] * v[4]) {
	goto L210;
    }

/*     ***  INCREASING RADIUS LOOKS WORTHWHILE.  SEE IF WE JUST */
/*     ***  RECOMPUTED STEP WITH A DECREASED RADIUS OR RESTORED STEP */
/*     ***  AFTER RECOMPUTING IT WITH A LARGER RADIUS. */

    if (iv[8] < 0) {
	goto L210;
    }
    if (iv[9] == 1) {
	goto L210;
    }
    if (iv[9] == 3) {
	goto L210;
    }

/*        ***  WE DID NOT.  TRY A LONGER STEP UNLESS THIS WAS A NEWTON */
/*        ***  STEP. */

    v[16] = v[25];
    gts = v[4];
    if (v[11] < (.5 / v[16] - 1.) * gts) {
/* Computing MAX */
	d__1 = v[23], d__2 = gts * .5 / (gts + v[11]);
	v[16] = max(d__1,d__2);
    }
    iv[29] = 4;
    if (v[5] == 0.) {
	goto L230;
    }
    if (v[3] >= 0. && (v[3] < v[2] * 2. || v[6] < v[11] * 1.2)) {
	goto L230;
    }
/*             ***  STEP WAS NOT A NEWTON STEP.  RECOMPUTE IT WITH */
/*             ***  A LARGER RADIUS. */
    iv[29] = 5;
    ++iv[8];

/*  ***  SAVE VALUES CORRESPONDING TO GOOD STEP  *** */

L200:
    v[12] = v[10];
    iv[32] = iv[5];
    if (iv[9] == 0) {
	iv[9] = 2;
    }
    v[18] = v[2];
    iv[7] = nfc;
    v[15] = v[7];
    v[14] = v[4];
    goto L230;

/*  ***  ACCEPT STEP WITH RADIUS UNCHANGED  *** */

L210:
    v[16] = 1.;
    iv[29] = 3;
    goto L230;

/*  ***  COME HERE FOR A RESTART AFTER CONVERGENCE  *** */

L220:
    iv[29] = iv[13];
    if (v[18] >= 0.) {
	goto L240;
    }
    iv[29] = 12;
    goto L240;

/*  ***  PERFORM CONVERGENCE TESTS  *** */

L230:
    iv[13] = iv[29];
L240:
    if (iv[9] == 1 && v[12] < v[13]) {
	iv[9] = 3;
    }
    if (abs(v[10]) < v[31]) {
	iv[29] = 10;
    }
    if (v[11] * .5 > v[7]) {
	goto L999;
    }
    emax = v[32] * abs(v[13]);
    emaxs = v[37] * abs(v[13]);
    if (v[7] <= emaxs && (v[2] > v[36] || v[5] == 0.)) {
	iv[29] = 11;
    }
    if (v[3] < 0.) {
	goto L250;
    }
    i__ = 0;
    if (v[6] > 0. && v[6] <= emax || v[6] == 0. && v[7] == 0.) {
	i__ = 2;
    }
    if (v[5] == 0. && v[17] <= v[33] && goodx) {
	++i__;
    }
    if (i__ > 0) {
	iv[29] = i__ + 6;
    }

/*  ***  CONSIDER RECOMPUTING STEP OF LENGTH V(LMAXS) FOR SINGULAR */
/*  ***  CONVERGENCE TEST. */

L250:
    if (iv[29] > 5 && iv[29] != 12) {
	goto L999;
    }
    if (v[5] == 0.) {
	goto L999;
    }
    if (v[2] > v[36]) {
	goto L260;
    }
    if (v[7] >= emaxs) {
	goto L999;
    }
    if (v[3] <= 0.) {
	goto L270;
    }
    if (v[3] * .5 <= v[36]) {
	goto L999;
    }
    goto L270;
L260:
    if (v[2] * .5 <= v[36]) {
	goto L999;
    }
    xmax = v[36] / v[2];
    if (xmax * (2. - xmax) * v[7] >= emaxs) {
	goto L999;
    }
L270:
    if (v[6] < 0.) {
	goto L290;
    }

/*  ***  RECOMPUTE V(PREDUC) FOR USE IN SINGULAR CONVERGENCE TEST  *** */

    v[14] = v[4];
    v[18] = v[2];
    if (iv[29] == 12) {
	v[18] = -v[18];
    }
    v[15] = v[7];
    i__ = iv[9];
    iv[9] = 2;
    if (i__ == 3) {
	iv[9] = 0;
    }
    iv[29] = 6;
    goto L999;

/*  ***  PERFORM SINGULAR CONVERGENCE TEST WITH RECOMPUTED V(PREDUC)  *** */

L280:
    v[4] = v[14];
    v[2] = abs(v[18]);
    iv[29] = iv[13];
    if (v[18] <= 0.) {
	iv[29] = 12;
    }
    v[6] = -v[7];
    v[7] = v[15];
    iv[9] = 3;
L290:
    if (-v[6] <= v[37] * abs(v[13])) {
	iv[29] = 11;
    }

L999:
    return 0;

/*  ***  LAST LINE OF DA7SST FOLLOWS  *** */
} /* da7sst_ */

