/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "f2c.h"
#include "imagemath.h"

extern int hybsvd_(integer *, integer *, integer *, integer*, integer*,
		   integer*, integer*,
		   real*, real*, integer*, real*, integer*, real*,
		   real*, real*, integer*, integer*, real*);
static doublereal srelpr_();

/* Table of constant values */
static integer c__1 = 1;
static integer c__0 = 0;

static float *yconst = NULL;
static float *transf = NULL;
#define TRANSF(row,col) transf[row*npts+col]
static float *covar = NULL;
/* Note: only lower triangle of covar matrix is stored */
/* NOTE: row >= col in following macro */
#define COVAR(row,col) covar[((row)*((row)+1))/2 + (col)]

/*
 * Returns error code equal to the HYBSVD error code (plus additional err=-6).
 *	err=0: OK
 *	err>0: SVD failed
 *	err=-1 to -5: Various SVD errors
 *	err=-6: More parameters than points
 */
int
linfit_setup(float *x,
	     float *sigma,
	     int npts,
	     int npars,
	     int nvars,
	     void (*func)(int npts, int npars, float *pars, int nvars,
			  float *xvecs, float *yvec)
	     )
{
    int i;
    int j;
    int k;
    float sum;
    float minw;
    float maxw;
    float winv;
    float *pars;
    float *a;
    float *y1;
    float *w;
    float *u;
    float *v;
    float *z;
    float *work;
    integer lnpts;
    integer lnpars;
    integer trueflag;
    integer err = 0;

    /* Note that matrices a, u, and v are passed/returned from the
     * Fortran derived routine "hybsvd_".  They are stored in column
     * order (row number cycles fastest).  The following macros hide
     * this storage convention from the user. */
#define A(i,j) a[j*npts+i]
#define U(i,j) u[j*npts+i]
#define V(i,j) v[j*npars+i]

    if (npts < npars){
	return -6;		/* System excessively singular */
    }

    /* Allocate scratch memory */
    pars = (float *)malloc(sizeof(float) * npars);
    a = (float *)malloc(sizeof(float) * npts * npars);
    y1 = (float *)malloc(sizeof(float) * npts);
    w = (float *)malloc(sizeof(float) * npars);
    u = (float *)malloc(sizeof(float) * npts * npars);
    v = (float *)malloc(sizeof(float) * npars * npars);
    z = (float *)malloc(sizeof(float) * npars * npars);
    work = (float *)malloc(sizeof(float) * npars);
    
    /* Memory for transformation matrix */
    /* (Free any memory allocated on previous calls) */
    release_memitem(yconst);
    release_memitem(transf);
    release_memitem(covar);
    yconst = (float *)getmem(sizeof(float) * npts);
    transf = (float *)getmem(sizeof(float) * npts * npars);
    covar = (float *)getmem(sizeof(float) * (npars * (npars+1))/2);

    /* Load up the "a" matrix */
    for (i=0; i<npars; pars[i++]=0);/* Clear parameter vector */
    (*func)(npts, npars, pars, nvars, x, yconst); /* Y values with null parms */
    for (i=0; i<npars; i++){
	pars[i] = 1;
	(*func)(npts, npars, pars, nvars, x, y1);
	for (j=0; j<npts; j++){
	    A(j,i) = y1[j] - yconst[j];
	}
	pars[i] = 0;
    }
    /*fprintf(stderr,"a=\n");
    for (i=0; i<npts; i++){
	for (j=0; j<npars; j++){
	    fprintf(stderr,"%12.4g ", A(i,j));
	}
	fprintf(stderr,"\n");
    }/*CMP*/

    /* Get the Singular Value Decomposition of "a" into "uwv'" */
    /* Variables needed for Fortran linkage */
    trueflag = TRUE_;
    lnpts = npts;
    lnpars = npars;
    hybsvd_(&lnpts, &lnpts, &lnpars, &lnpars, &lnpts, &lnpts, &lnpars,
	    a, w, &trueflag, u, &trueflag, v, z, NULL, &c__0, &err, work);
    if (!err){
	/* Find max "w", so we can set minimum threshold */
	maxw = w[0];
	for (i=1; i<npars; i++){
	    if (maxw < w[i]) maxw = w[i];
	}
	minw = srelpr_() * npts * maxw;

	/* Multiply out transf = vw'u' to get the transformation matrix
	 * that takes a vector of y values into parameter values.
	 */
	/* inverse(w) * u' */
	for (i=0; i<npars; i++){
	    winv = w[i] < minw ? 0 : 1 / w[i]; /* Edit singular values */
	    for (j=0; j<npts; j++){
		U(j,i) *= winv;
	    }
	}
	/* Mult on left by v */
	for (i=0; i<npars; i++){
	    for (j=0; j<npts; j++){
		TRANSF(i,j) = 0;
		for (k=0; k<npars; k++){
		    TRANSF(i,j) += V(i,k) * U(j,k);
		}
	    }
	}

	/*fprintf(stderr,"transf=\n");
	for (i=0; i<npars; i++){
	    for (j=0; j<npts; j++){
		fprintf(stderr,"%9.3g ", TRANSF(i,j));
	    }
	    fprintf(stderr,"\n");
	}/*CMP*/

	/* Get covariance matrix (assuming sigma of each point = 1 */
	for (i=0; i<npars; i++){
	    work[i] = w[i] == 0 ? 0 : 1 / (w[i] * w[i]);
	}
	for (i=0; i<npars; i++){
	    for (j=0; j<=i; j++){
		sum = 0;
		for (k=0; k<npars; k++){
		    sum += V(i,k) * V(j,k) * work[k];
		}
		COVAR(i,j) = sum;
	    }
	}
    }

    /* Release memory */
    if (pars) free(pars);
    if (a) free(a);
    if (y1) free(y1);
    if (w) free(w);
    if (u) free(u);
    if (v) free(v);
    if (z) free(z);
    if (work) free(work);

    return err;
#undef A
#undef U
#undef V
}

int
linfit_go(float *x, float *y, int npts, int npars, int nvars, int newoffset,
	  void (*func)(int npts, int npars, float *pars, int nvars,
		       float *xvecs, float *yvec),
	  float *pars, float *resid, float *covariance)
{
    int i;
    int j;
    float xx;
    float r;
    float *y1;

    if (!transf || !yconst){
	return 0;
    }
    if (newoffset){
	float *p;
	p = (float *)calloc(npars, sizeof(float));
	(*func)(npts, npars, p, nvars, x, yconst);
	free(p);
    }
    for (i=0; i<npars; i++){
	pars[i] = 0;
	for (j=0; j<npts; j++){
	    pars[i] += TRANSF(i,j) * (y[j] - yconst[j]);
	}
    }
    if (resid || covariance){
	y1 = (float *)malloc(sizeof(float) * npts);
	(*func)(npts, npars, pars, nvars, x, y1); /* y1 = predicted values */
	r = 0;
	for (i=0; i<npts; i++){
	    xx = y[i] - y1[i];
	    r += xx * xx;
	}
	if (resid){
	    *resid = sqrt(r / npts);
	}
	free(y1);
    }
    if (covariance && npts > npars){
	for (i=0; i<(npars*(npars+1))/2; i++){
	    covariance[i] = covar[i] * r / (npts - npars);
	}
    }
		
    return 1;
}

int
linfit(float *x,
       float *y,
       float *sigma,
       int npts,
       int npars,
       int nvars,
       void (*func)(int npts, int npars, float *pars, int nvars,
			  float *xvecs, float *yvec),
       float *pars,
       float *resid,
       float *covar)
{
    int err;

    err = linfit_setup(x, sigma, npts, npars, nvars, func);
    if (!err){
	err = linfit_go(x, y, npts, npars, nvars, 0, func,
			pars, resid, covar);
    }
    return err;
}

/*
 * The remainder of this file is excerpted from public domain material
 */

/* translated by f2c (version 19961209).
   You must link the resulting object file with the libraries:
	-lf2c -lm   (in that order)
*/

/* ====================================================================== */
/* NIST Guide to Available Math Software. */
/* Fullsource for module 581 from package TOMS. */
/* Retrieved from NETLIB on Tue Jun 10 20:53:14 1997. */
/* ====================================================================== */
/*     ALGORITHM 581, COLLECTED ALGORITHMS FROM ACM. */
/*     ALGORITHM APPEARED IN ACM-TRANS. MATH. SOFTWARE, VOL.8, NO. 1, */
/*     MAR., 1982, P. 84. */
/**************************************************************************/
/*                                                                       * */
/*       THIS FILE CONTAINS PROGRAMS AND SUBROUTINES THAT ARE RELATED    * */
/*       TO THE PAPER 'AN IMPROVED ALGORITHM FOR COMPUTING THE SINGULAR  * */
/*       VALUE DECOMPOSITION' BY T.F. CHAN, WHICH WAS SUBMITTED FOR      * */
/*       PUBLICATIONS IN ACM TOMS.                                       * */
/*                                                                       * */
/*       THEY ARE LISTED BELOW IN THE ORDER THAT THEY APPEAR IN THE      * */
/*       FILE:                                                           * */
/*                                                                       * */
/*                                                                       * */
/*           2) ROUTINE HYBSVD ... HYBRID ALGORITHM ROUTINE.             * */
/*                                                                       * */
/*           3) ROUTINE MGNSVD ... HYBRID ALGORITHM, ASSUMES M .GE. N.   * */
/*                                                                       * */
/*           4) ROUTINE GRSVD  ... GOLUB-REINSCH ALGORITHM ROUTINE       * */
/*                                                                       * */
/*           5) BLAS ROUTINE   ... SSWAP, FOR SWAPPING VECTORS           * */
/*                                                                       * */
/*           6) ROUTINE SRELPR ... COMPUTES MACHINE PRECISION            * */
/*                                                                       * */
/*       THE ROUTINES 2) TO 6) CONSTITUTE THE PACKAGE THAT IMPLEMENTS    * */
/*       THE NEW HYBRID ALGORITHM AND CAN BE USED BY THEMSELVES.         * */
/*                                                                       * */
/*       PLEASE ADDRESS COMMENTS AND SUGGESTIONS TO:                     * */
/*                                                                       * */
/*           TONY CHAN                                                   * */
/*           COMPUTER SCIENCE DEPT., YALE UNIV.,                         * */
/*           BOX 2158, YALE STATION,                                     * */
/*           NEW HAVEN, CT 06520.                                        * */
/*                                                                       * */
/**************************************************************************/


/* Subroutine */ int hybsvd_(na, nu, nv, nz, nb, m, n, a, w, matu, u, matv, v,
	 z__, b, irhs, ierr, rv1)
integer *na, *nu, *nv, *nz, *nb, *m, *n;
real *a, *w;
logical *matu;
real *u;
logical *matv;
real *v, *z__, *b;
integer *irhs, *ierr;
real *rv1;
{
    /* System generated locals */
    integer a_dim1, a_offset, u_dim1, u_offset, v_dim1, v_offset, z_dim1, 
	    z_offset, b_dim1, b_offset, i__1, i__2;

    /* Local variables */
    static integer i__, j;
    extern /* Subroutine */ int mgnsvd_();


/*     THIS ROUTINE IS A MODIFICATION OF THE GOLUB-REINSCH PROCEDURE (1) */
/*                                                           T */
/*     FOR COMPUTING THE SINGULAR VALUE DECOMPOSITION A = UWV  OF A */
/*     REAL M BY N RECTANGULAR MATRIX. U IS M BY MIN(M,N) CONTAINING */
/*     THE LEFT SINGULAR VECTORS, W IS A MIN(M,N) BY MIN(M,N) DIAGONAL */
/*     MATRIX CONTAINING THE SINGULAR VALUES, AND V IS N BY MIN(M,N) */
/*     CONTAINING THE RIGHT SINGULAR VECTORS. */

/*     THE ALGORITHM IMPLEMENTED IN THIS */
/*     ROUTINE HAS A HYBRID NATURE.  WHEN M IS APPROXIMATELY EQUAL TO N, */
/*     THE GOLUB-REINSCH ALGORITHM IS USED, BUT WHEN EITHER OF THE RATIOS */
/*     M/N OR N/M IS GREATER THAN ABOUT 2, */
/*     A MODIFIED VERSION OF THE GOLUB-REINSCH */
/*     ALGORITHM IS USED.  THIS MODIFIED ALGORITHM FIRST TRANSFORMS A */
/*                                                                T */
/*     INTO UPPER TRIANGULAR FORM BY HOUSEHOLDER TRANSFORMATIONS L */
/*     AND THEN USES THE GOLUB-REINSCH ALGORITHM TO FIND THE SINGULAR */
/*     VALUE DECOMPOSITION OF THE RESULTING UPPER TRIANGULAR MATRIX R. */
/*     WHEN U IS NEEDED EXPLICITLY IN THE CASE M.GE.N (OR V IN THE CASE */
/*     M.LT.N), AN EXTRA ARRAY Z (OF SIZE AT LEAST */
/*     MIN(M,N)**2) IS NEEDED, BUT OTHERWISE Z IS NOT REFERENCED */
/*     AND NO EXTRA STORAGE IS REQUIRED.  THIS HYBRID METHOD */
/*     SHOULD BE MORE EFFICIENT THAN THE GOLUB-REINSCH ALGORITHM WHEN */
/*     M/N OR N/M IS LARGE.  FOR DETAILS, SEE (2). */

/*     WHEN M .GE. N, */
/*     HYBSVD CAN ALSO BE USED TO COMPUTE THE MINIMAL LENGTH LEAST */
/*     SQUARES SOLUTION TO THE OVERDETERMINED LINEAR SYSTEM A*X=B. */
/*     IF M .LT. N (I.E. FOR UNDERDETERMINED SYSTEMS), THE RHS B */
/*     IS NOT PROCESSED. */

/*     NOTICE THAT THE SINGULAR VALUE DECOMPOSITION OF A MATRIX */
/*     IS UNIQUE ONLY UP TO THE SIGN OF THE CORRESPONDING COLUMNS */
/*     OF U AND V. */

/*     THIS ROUTINE HAS BEEN CHECKED BY THE PFORT VERIFIER (3) FOR */
/*     ADHERENCE TO A LARGE, CAREFULLY DEFINED, PORTABLE SUBSET OF */
/*     AMERICAN NATIONAL STANDARD FORTRAN CALLED PFORT. */

/*     REFERENCES: */

/*     (1) GOLUB,G.H. AND REINSCH,C. (1970) 'SINGULAR VALUE */
/*         DECOMPOSITION AND LEAST SQUARES SOLUTIONS,' */
/*         NUMER. MATH. 14,403-420, 1970. */

/*     (2) CHAN,T.F. (1982) 'AN IMPROVED ALGORITHM FOR COMPUTING */
/*         THE SINGULAR VALUE DECOMPOSITION,' ACM TOMS, VOL.8, */
/*         NO. 1, MARCH, 1982. */

/*     (3) RYDER,B.G. (1974) 'THE PFORT VERIFIER,' SOFTWARE - */
/*         PRACTICE AND EXPERIENCE, VOL.4, 359-377, 1974. */

/*     ON INPUT: */

/*        NA MUST BE SET TO THE ROW DIMENSION OF THE TWO-DIMENSIONAL */
/*          ARRAY PARAMETER A AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT.  NOTE THAT NA MUST BE AT LEAST */
/*          AS LARGE AS M. */

/*        NU MUST BE SET TO THE ROW DIMENSION OF THE TWO-DIMENSIONAL */
/*          ARRAY U AS DECLARED IN THE CALLING PROGRAM DIMENSION */
/*          STATEMENT. NU MUST BE AT LEAST AS LARGE AS M. */

/*        NV MUST BE SET TO THE ROW DIMENSION OF THE TWO-DIMENSIONAL */
/*          ARRAY PARAMETER V AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. NV MUST BE AT LEAST AS LARGE AS N. */

/*        NZ MUST BE SET TO THE ROW DIMENSION OF THE TWO-DIMENSIONAL */
/*          ARRAY PARAMETER Z AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT.  NOTE THAT NZ MUST BE AT LEAST */
/*          AS LARGE AS MIN(M,N). */

/*        NB MUST BE SET TO THE ROW DIMENSION OF THE TWO-DIMENSIONAL */
/*          ARRAY PARAMETER B AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. NB MUST BE AT LEAST AS LARGE AS M. */

/*        M IS THE NUMBER OF ROWS OF A (AND U). */

/*        N IS THE NUMBER OF COLUMNS OF A (AND NUMBER OF ROWS OF V). */

/*        A CONTAINS THE RECTANGULAR INPUT MATRIX TO BE DECOMPOSED. */

/*        B CONTAINS THE IRHS RIGHT-HAND-SIDES OF THE OVERDETERMINED */
/*         LINEAR SYSTEM A*X=B. IF IRHS .GT. 0 AND M .GE. N, */
/*         THEN ON OUTPUT, THE FIRST N COMPONENTS OF THESE IRHS COLUMNS */
/*                       T */
/*         WILL CONTAIN U B. THUS, TO COMPUTE THE MINIMAL LENGTH LEAST */
/*                                               + */
/*         SQUARES SOLUTION, ONE MUST COMPUTE V*W  TIMES THE COLUMNS OF */
/*                   +                        + */
/*         B, WHERE W  IS A DIAGONAL MATRIX, W (I)=0 IF W(I) IS */
/*         NEGLIGIBLE, OTHERWISE IS 1/W(I). IF IRHS=0 OR M.LT.N, */
/*         B IS NOT REFERENCED. */

/*        IRHS IS THE NUMBER OF RIGHT-HAND-SIDES OF THE OVERDETERMINED */
/*         SYSTEM A*X=B. IRHS SHOULD BE SET TO ZERO IF ONLY THE SINGULAR */
/*         VALUE DECOMPOSITION OF A IS DESIRED. */

/*        MATU SHOULD BE SET TO .TRUE. IF THE U MATRIX IN THE */
/*          DECOMPOSITION IS DESIRED, AND TO .FALSE. OTHERWISE. */

/*        MATV SHOULD BE SET TO .TRUE. IF THE V MATRIX IN THE */
/*          DECOMPOSITION IS DESIRED, AND TO .FALSE. OTHERWISE. */

/*        WHEN HYBSVD IS USED TO COMPUTE THE MINIMAL LENGTH LEAST */
/*        SQUARES SOLUTION TO AN OVERDETERMINED SYSTEM, MATU SHOULD */
/*        BE SET TO .FALSE. , AND MATV SHOULD BE SET TO .TRUE.  . */

/*     ON OUTPUT: */

/*        A IS UNALTERED (UNLESS OVERWRITTEN BY U OR V). */

/*        W CONTAINS THE (NON-NEGATIVE) SINGULAR VALUES OF A (THE */
/*          DIAGONAL ELEMENTS OF W).  THEY ARE SORTED IN DESCENDING */
/*          ORDER.  IF AN ERROR EXIT IS MADE, THE SINGULAR VALUES */
/*          SHOULD BE CORRECT AND SORTED FOR INDICES IERR+1,...,MIN(M,N). */

/*        U CONTAINS THE MATRIX U (ORTHOGONAL COLUMN VECTORS) OF THE */
/*          DECOMPOSITION IF MATU HAS BEEN SET TO .TRUE.  IF MATU IS */
/*          FALSE, THEN U IS EITHER USED AS A TEMPORARY STORAGE (IF */
/*          M .GE. N) OR NOT REFERENCED (IF M .LT. N). */
/*          U MAY COINCIDE WITH A IN THE CALLING SEQUENCE. */
/*          IF AN ERROR EXIT IS MADE, THE COLUMNS OF U CORRESPONDING */
/*          TO INDICES OF CORRECT SINGULAR VALUES SHOULD BE CORRECT. */

/*        V CONTAINS THE MATRIX V (ORTHOGONAL) OF THE DECOMPOSITION IF */
/*          MATV HAS BEEN SET TO .TRUE.  IF MATV IS */
/*          FALSE, THEN V IS EITHER USED AS A TEMPORARY STORAGE (IF */
/*          M .LT. N) OR NOT REFERENCED (IF M .GE. N). */
/*          IF M .GE. N, V MAY ALSO COINCIDE WITH A.  IF AN ERROR */
/*          EXIT IS MADE, THE COLUMNS OF V CORRESPONDING TO INDICES OF */
/*          CORRECT SINGULAR VALUES SHOULD BE CORRECT. */

/*        Z CONTAINS THE MATRIX X IN THE SINGULAR VALUE DECOMPOSITION */
/*                  T */
/*          OF R=XSY,  IF THE MODIFIED ALGORITHM IS USED. IF THE */
/*          GOLUB-REINSCH PROCEDURE IS USED, THEN IT IS NOT REFERENCED. */
/*          IF MATU HAS BEEN SET TO .FALSE. IN THE CASE M.GE.N (OR */
/*          MATV SET TO .FALSE. IN THE CASE M.LT.N), THEN Z IS NOT */
/*          REFERENCED AND NO EXTRA STORAGE IS REQUIRED. */

/*        IERR IS SET TO */
/*          ZERO       FOR NORMAL RETURN, */
/*          K          IF THE K-TH SINGULAR VALUE HAS NOT BEEN */
/*                     DETERMINED AFTER 30 ITERATIONS. */
/*          -1         IF IRHS .LT. 0 . */
/*          -2         IF M .LT. 1 .OR. N .LT. 1 */
/*          -3         IF NA .LT. M .OR. NU .LT. M .OR. NB .LT. M. */
/*          -4         IF NV .LT. N . */
/*          -5         IF NZ .LT. MIN(M,N). */

/*        RV1 IS A TEMPORARY STORAGE ARRAY OF LENGTH AT LEAST MIN(M,N). */

/*     PROGRAMMED BY : TONY CHAN */
/*                     BOX 2158, YALE STATION, */
/*                     COMPUTER SCIENCE DEPT, YALE UNIV., */
/*                     NEW HAVEN, CT 06520. */
/*     LAST MODIFIED : JANUARY, 1982. */

/*     HYBSVD USES THE FOLLOWING FUNCTIONS AND SUBROUTINES. */
/*       INTERNAL  GRSVD, MGNSVD, SRELPR */
/*       FORTRAN   MIN0,ABS,SQRT,FLOAT,SIGN,AMAX1 */
/*       BLAS      SSWAP */

/*     ----------------------------------------------------------------- */
/*     ERROR CHECK. */

    /* Parameter adjustments */
    a_dim1 = *na;
    a_offset = a_dim1 + 1;
    a -= a_offset;
    u_dim1 = *nu;
    u_offset = u_dim1 + 1;
    u -= u_offset;
    v_dim1 = *nv;
    v_offset = v_dim1 + 1;
    v -= v_offset;
    z_dim1 = *nz;
    z_offset = z_dim1 + 1;
    z__ -= z_offset;
    --w;
    b_dim1 = *nb;
    b_offset = b_dim1 + 1;
    b -= b_offset;
    --rv1;

    /* Function Body */
    *ierr = 0;
    if (*irhs >= 0) {
	goto L10;
    }
    *ierr = -1;
    return 0;
L10:
    if (*m >= 1 && *n >= 1) {
	goto L20;
    }
    *ierr = -2;
    return 0;
L20:
    if (*na >= *m && *nu >= *m && *nb >= *m) {
	goto L30;
    }
    *ierr = -3;
    return 0;
L30:
    if (*nv >= *n) {
	goto L40;
    }
    *ierr = -4;
    return 0;
L40:
    if (*nz >= min(*m,*n)) {
	goto L50;
    }
    *ierr = -5;
    return 0;
L50:

/*     FIRST COPIES A INTO EITHER U OR V ACCORDING TO WHETHER */
/*     M .GE. N OR M .LT. N, AND THEN CALLS SUBROUTINE MGNSVD */
/*     WHICH ASSUMES THAT NUMBER OF ROWS .GE. NUMBER OF COLUMNS. */

    if (*m < *n) {
	goto L80;
    }

/*       M .GE. N  CASE. */

    i__1 = *m;
    for (i__ = 1; i__ <= i__1; ++i__) {
	i__2 = *n;
	for (j = 1; j <= i__2; ++j) {
	    u[i__ + j * u_dim1] = a[i__ + j * a_dim1];
/* L60: */
	}
/* L70: */
    }

    mgnsvd_(nu, nv, nz, nb, m, n, &w[1], matu, &u[u_offset], matv, &v[
	    v_offset], &z__[z_offset], &b[b_offset], irhs, ierr, &rv1[1]);
    return 0;

L80:
/*                              T */
/*       M .LT. N CASE. COPIES A  INTO V. */

    i__1 = *m;
    for (i__ = 1; i__ <= i__1; ++i__) {
	i__2 = *n;
	for (j = 1; j <= i__2; ++j) {
	    v[j + i__ * v_dim1] = a[i__ + j * a_dim1];
/* L90: */
	}
/* L100: */
    }
    mgnsvd_(nv, nu, nz, nb, n, m, &w[1], matv, &v[v_offset], matu, &u[
	    u_offset], &z__[z_offset], &b[b_offset], &c__0, ierr, &rv1[1]);
    return 0;
} /* hybsvd_ */

/*                                                                      MGN   
10*/
/* Subroutine */ int mgnsvd_(nu, nv, nz, nb, m, n, w, matu, u, matv, v, z__, 
	b, irhs, ierr, rv1)
integer *nu, *nv, *nz, *nb, *m, *n;
real *w;
logical *matu;
real *u;
logical *matv;
real *v, *z__, *b;
integer *irhs, *ierr;
real *rv1;
{
    /* System generated locals */
    integer u_dim1, u_offset, v_dim1, v_offset, z_dim1, z_offset, b_dim1, 
	    b_offset, i__1, i__2, i__3;
    real r__1;

    /* Builtin functions */
    double sqrt(), r_sign();

    /* Local variables */
    static integer krhs;
    static real c__, f, g, h__;
    static integer i__, j, k, iback;
    static real r__, s, t, scale;
    extern /* Subroutine */ int grsvd_(), sswap_();
    static integer ierrp1, id, im1, ip1, nm1;
    static real xovrpt;


/*     THE DESCRIPTION OF SUBROUTINE MGNSVD IS ALMOST IDENTICAL */
/*     TO THAT FOR SUBROUTINE HYBSVD ABOVE, WITH THE EXCEPTION */
/*     THAT MGNSVD ASSUMES M .GE. N. */
/*     IT ALSO ASSUMES THAT A COPY OF THE MATRIX A IS IN THE ARRAY U. */


/*     SET VALUE FOR C. THE VALUE FOR C DEPENDS ON THE RELATIVE */
/*     EFFICIENCY OF FLOATING POINT MULTIPLICATIONS, FLOATING POINT */
/*     ADDITIONS AND TWO-DIMENSIONAL ARRAY INDEXINGS ON THE */
/*     COMPUTER WHERE THIS SUBROUTINE IS TO BE RUN.  C SHOULD */
/*     USUALLY BE BETWEEN 2 AND 4.  FOR DETAILS ON CHOOSING C, SEE */
/*     (2).  THE ALGORITHM IS NOT SENSITIVE TO THE VALUE OF C */
/*     ACTUALLY USED AS LONG AS C IS BETWEEN 2 AND 4. */

    /* Parameter adjustments */
    u_dim1 = *nu;
    u_offset = u_dim1 + 1;
    u -= u_offset;
    v_dim1 = *nv;
    v_offset = v_dim1 + 1;
    v -= v_offset;
    z_dim1 = *nz;
    z_offset = z_dim1 + 1;
    z__ -= z_offset;
    --w;
    b_dim1 = *nb;
    b_offset = b_dim1 + 1;
    b -= b_offset;
    --rv1;

    /* Function Body */
    c__ = (float)4.;

/*     DETERMINE CROSS-OVER POINT */

    if (*matu && *matv) {
	xovrpt = (c__ + (float)2.3333333333333335) / c__;
    }
    if (*matu && ! (*matv)) {
	xovrpt = (c__ + (float)2.3333333333333335) / c__;
    }
    if (! (*matu) && *matv) {
	xovrpt = (float)1.6666666666666667;
    }
    if (! (*matu) && ! (*matv)) {
	xovrpt = (float)1.6666666666666667;
    }

/*     DETERMINE WHETHER TO USE GOLUB-REINSCH OR THE MODIFIED */
/*     ALGORITHM. */

    r__ = (real) (*m) / (real) (*n);
    if (r__ >= xovrpt) {
	goto L10;
    }

/*     USE GOLUB-REINSCH PROCEDURE */

    grsvd_(nu, nv, nb, m, n, &w[1], matu, &u[u_offset], matv, &v[v_offset], &
	    b[b_offset], irhs, ierr, &rv1[1]);
    goto L330;

/*     USE MODIFIED ALGORITHM */

L10:

/*     TRIANGULARIZE U BY HOUSEHOLDER TRANSFORMATIONS, USING */
/*     W AND RV1 AS TEMPORARY STORAGE. */

    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
	g = (float)0.;
	s = (float)0.;
	scale = (float)0.;

/*         PERFORM SCALING OF COLUMNS TO AVOID UNNECSSARY OVERFLOW */
/*         OR UNDERFLOW */

	i__2 = *m;
	for (k = i__; k <= i__2; ++k) {
	    scale += (r__1 = u[k + i__ * u_dim1], dabs(r__1));
/* L20: */
	}
	if (scale == (float)0.) {
	    goto L110;
	}
	i__2 = *m;
	for (k = i__; k <= i__2; ++k) {
	    u[k + i__ * u_dim1] /= scale;
/* Computing 2nd power */
	    r__1 = u[k + i__ * u_dim1];
	    s += r__1 * r__1;
/* L30: */
	}

/*         THE VECTOR E OF THE HOUSEHOLDER TRANSFORMATION I + EE'/H */
/*         WILL BE STORED IN COLUMN I OF U. THE TRANSFORMED ELEMENT */
/*         U(I,I) WILL BE STORED IN W(I) AND THE SCALAR H IN */
/*         RV1(I). */

	f = u[i__ + i__ * u_dim1];
	r__1 = sqrt(s);
	g = -r_sign(&r__1, &f);
	h__ = f * g - s;
	u[i__ + i__ * u_dim1] = f - g;
	rv1[i__] = h__;
	w[i__] = scale * g;

	if (i__ == *n) {
	    goto L70;
	}

/*         APPLY TRANSFORMATIONS TO REMAINING COLUMNS OF A */

	ip1 = i__ + 1;
	i__2 = *n;
	for (j = ip1; j <= i__2; ++j) {
	    s = (float)0.;
	    i__3 = *m;
	    for (k = i__; k <= i__3; ++k) {
		s += u[k + i__ * u_dim1] * u[k + j * u_dim1];
/* L40: */
	    }
	    f = s / h__;
	    i__3 = *m;
	    for (k = i__; k <= i__3; ++k) {
		u[k + j * u_dim1] += f * u[k + i__ * u_dim1];
/* L50: */
	    }
/* L60: */
	}

/*         APPLY TRANSFORMATIONS TO COLUMNS OF B IF IRHS .GT. 0 */

L70:
	if (*irhs == 0) {
	    goto L110;
	}
	i__2 = *irhs;
	for (j = 1; j <= i__2; ++j) {
	    s = (float)0.;
	    i__3 = *m;
	    for (k = i__; k <= i__3; ++k) {
		s += u[k + i__ * u_dim1] * b[k + j * b_dim1];
/* L80: */
	    }
	    f = s / h__;
	    i__3 = *m;
	    for (k = i__; k <= i__3; ++k) {
		b[k + j * b_dim1] += f * u[k + i__ * u_dim1];
/* L90: */
	    }
/* L100: */
	}
L110:
	;
    }

/*     COPY R INTO Z IF MATU = .TRUE. */

    if (! (*matu)) {
	goto L290;
    }
    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
	i__2 = *n;
	for (j = i__; j <= i__2; ++j) {
	    z__[j + i__ * z_dim1] = (float)0.;
	    z__[i__ + j * z_dim1] = u[i__ + j * u_dim1];
/* L120: */
	}
	z__[i__ + i__ * z_dim1] = w[i__];
/* L130: */
    }

/*     ACCUMULATE HOUSEHOLDER TRANSFORMATIONS IN U */

    i__1 = *n;
    for (iback = 1; iback <= i__1; ++iback) {
	i__ = *n - iback + 1;
	ip1 = i__ + 1;
	g = w[i__];
	h__ = rv1[i__];
	if (i__ == *n) {
	    goto L150;
	}

	i__2 = *n;
	for (j = ip1; j <= i__2; ++j) {
	    u[i__ + j * u_dim1] = (float)0.;
/* L140: */
	}

L150:
	if (h__ == (float)0.) {
	    goto L210;
	}
	if (i__ == *n) {
	    goto L190;
	}

	i__2 = *n;
	for (j = ip1; j <= i__2; ++j) {
	    s = (float)0.;
	    i__3 = *m;
	    for (k = ip1; k <= i__3; ++k) {
		s += u[k + i__ * u_dim1] * u[k + j * u_dim1];
/* L160: */
	    }
	    f = s / h__;
	    i__3 = *m;
	    for (k = i__; k <= i__3; ++k) {
		u[k + j * u_dim1] += f * u[k + i__ * u_dim1];
/* L170: */
	    }
/* L180: */
	}

L190:
	s = u[i__ + i__ * u_dim1] / h__;
	i__2 = *m;
	for (j = i__; j <= i__2; ++j) {
	    u[j + i__ * u_dim1] *= s;
/* L200: */
	}
	goto L230;

L210:
	i__2 = *m;
	for (j = i__; j <= i__2; ++j) {
	    u[j + i__ * u_dim1] = (float)0.;
/* L220: */
	}
L230:
	u[i__ + i__ * u_dim1] += (float)1.;
/* L240: */
    }

/*     COMPUTE SVD OF R (WHICH IS STORED IN Z) */

    grsvd_(nz, nv, nb, n, n, &w[1], matu, &z__[z_offset], matv, &v[v_offset], 
	    &b[b_offset], irhs, ierr, &rv1[1]);

/*                                      T */
/*     FORM L*X TO OBTAIN U (WHERE R=XWY ). X IS RETURNED IN Z */
/*     BY GRSVD. THE MATRIX MULTIPLY IS DONE ONE ROW AT A TIME, */
/*     USING RV1 AS SCRATCH SPACE. */

    i__1 = *m;
    for (i__ = 1; i__ <= i__1; ++i__) {
	i__2 = *n;
	for (j = 1; j <= i__2; ++j) {
	    s = (float)0.;
	    i__3 = *n;
	    for (k = 1; k <= i__3; ++k) {
		s += u[i__ + k * u_dim1] * z__[k + j * z_dim1];
/* L250: */
	    }
	    rv1[j] = s;
/* L260: */
	}
	i__2 = *n;
	for (j = 1; j <= i__2; ++j) {
	    u[i__ + j * u_dim1] = rv1[j];
/* L270: */
	}
/* L280: */
    }
    goto L330;

/*     FORM R IN U BY ZEROING THE LOWER TRIANGULAR PART OF R IN U */

L290:
    if (*n == 1) {
	goto L320;
    }
    i__1 = *n;
    for (i__ = 2; i__ <= i__1; ++i__) {
	im1 = i__ - 1;
	i__2 = im1;
	for (j = 1; j <= i__2; ++j) {
	    u[i__ + j * u_dim1] = (float)0.;
/* L300: */
	}
	u[i__ + i__ * u_dim1] = w[i__];
/* L310: */
    }
L320:
    u[u_dim1 + 1] = w[1];

    grsvd_(nu, nv, nb, n, n, &w[1], matu, &u[u_offset], matv, &v[v_offset], &
	    b[b_offset], irhs, ierr, &rv1[1]);
L330:
    ierrp1 = *ierr + 1;
    if (*ierr < 0 || *n <= 1 || ierrp1 == *n) {
	return 0;
    }

/*     SORT SINGULAR VALUES AND EXCHANGE COLUMNS OF U AND V ACCORDINGLY. */
/*     SELECTION SORT MINIMIZES SWAPPING OF U AND V. */

    nm1 = *n - 1;
    i__1 = nm1;
    for (i__ = ierrp1; i__ <= i__1; ++i__) {
/* ...    FIND INDEX OF MAXIMUM SINGULAR VALUE */
	id = i__;
	ip1 = i__ + 1;
	i__2 = *n;
	for (j = ip1; j <= i__2; ++j) {
	    if (w[j] > w[id]) {
		id = j;
	    }
/* L340: */
	}
	if (id == i__) {
	    goto L360;
	}
/* ...    SWAP SINGULAR VALUES AND VECTORS */
	t = w[i__];
	w[i__] = w[id];
	w[id] = t;
	if (*matv) {
	    sswap_(n, &v[i__ * v_dim1 + 1], &c__1, &v[id * v_dim1 + 1], &c__1)
		    ;
	}
	if (*matu) {
	    sswap_(m, &u[i__ * u_dim1 + 1], &c__1, &u[id * u_dim1 + 1], &c__1)
		    ;
	}
	if (*irhs < 1) {
	    goto L360;
	}
	i__2 = *irhs;
	for (krhs = 1; krhs <= i__2; ++krhs) {
	    t = b[i__ + krhs * b_dim1];
	    b[i__ + krhs * b_dim1] = b[id + krhs * b_dim1];
	    b[id + krhs * b_dim1] = t;
/* L350: */
	}
L360:
	;
    }
    return 0;
/*     ************** LAST CARD OF HYBSVD ***************** */
} /* mgnsvd_ */

/* Subroutine */ int grsvd_(nu, nv, nb, m, n, w, matu, u, matv, v, b, irhs, 
	ierr, rv1)
integer *nu, *nv, *nb, *m, *n;
real *w;
logical *matu;
real *u;
logical *matv;
real *v, *b;
integer *irhs, *ierr;
real *rv1;
{
    /* System generated locals */
    integer u_dim1, u_offset, v_dim1, v_offset, b_dim1, b_offset, i__1, i__2, 
	    i__3;
    real r__1, r__2, r__3, r__4;

    /* Builtin functions */
    double sqrt(), r_sign();

    /* Local variables */
    static real c__, f, g, h__;
    static integer i__, j, k, l;
    static real s, x, y, z__, scale;
    static integer i1;
    static real dummy;
    static integer k1, l1, ii, kk, ll, mn;
    extern doublereal srelpr_();
    static real eps;
    static integer its;



/*        EXPERIENCE, VOL.4, 359-377, 1974) FOR ADHERENCE TO A LARGE, */
/*        CAREFULLY DEFINED, PORTABLE SUBSET OF AMERICAN NATIONAL STANDAR */
/*        FORTRAN CALLED PFORT. */

/*        ORIGINAL VERSION OF THIS CODE IS SUBROUTINE SVD IN RELEASE 2 OF */
/*        EISPACK. */

/*        MODIFIED BY TONY F. CHAN, */
/*                    COMP. SCI. DEPT, YALE UNIV., */
/*                    BOX 2158, YALE STATION, */
/*                    CT 06520 */
/*        LAST MODIFIED : JANUARY, 1982. */

/*     ------------------------------------------------------------------ */

/*     ********** SRELPR IS A MACHINE-DEPENDENT FUNCTION SPECIFYING */
/*                THE RELATIVE PRECISION OF FLOATING POINT ARITHMETIC. */

/*                ********** */

    /* Parameter adjustments */
    u_dim1 = *nu;
    u_offset = u_dim1 + 1;
    u -= u_offset;
    v_dim1 = *nv;
    v_offset = v_dim1 + 1;
    v -= v_offset;
    --w;
    b_dim1 = *nb;
    b_offset = b_dim1 + 1;
    b -= b_offset;
    --rv1;

    /* Function Body */
    *ierr = 0;
    if (*irhs >= 0) {
	goto L10;
    }
    *ierr = -1;
    return 0;
L10:
    if (*m >= *n) {
	goto L20;
    }
    *ierr = -2;
    return 0;
L20:
    if (*nu >= *m && *nb >= *m) {
	goto L30;
    }
    *ierr = -3;
    return 0;
L30:
    if (*nv >= *n) {
	goto L40;
    }
    *ierr = -4;
    return 0;
L40:

/*     ********** HOUSEHOLDER REDUCTION TO BIDIAGONAL FORM ********** */
    g = (float)0.;
    scale = (float)0.;
    x = (float)0.;

    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
	l = i__ + 1;
	rv1[i__] = scale * g;
	g = (float)0.;
	s = (float)0.;
	scale = (float)0.;

/*     COMPUTE LEFT TRANSFORMATIONS THAT ZERO THE SUBDIAGONAL ELEMENTS
 */
/*     OF THE I-TH COLUMN. */

	i__2 = *m;
	for (k = i__; k <= i__2; ++k) {
	    scale += (r__1 = u[k + i__ * u_dim1], dabs(r__1));
/* L50: */
	}

	if (scale == (float)0.) {
	    goto L160;
	}

	i__2 = *m;
	for (k = i__; k <= i__2; ++k) {
	    u[k + i__ * u_dim1] /= scale;
/* Computing 2nd power */
	    r__1 = u[k + i__ * u_dim1];
	    s += r__1 * r__1;
/* L60: */
	}

	f = u[i__ + i__ * u_dim1];
	r__1 = sqrt(s);
	g = -r_sign(&r__1, &f);
	h__ = f * g - s;
	u[i__ + i__ * u_dim1] = f - g;
	if (i__ == *n) {
	    goto L100;
	}

/*     APPLY LEFT TRANSFORMATIONS TO REMAINING COLUMNS OF A. */

	i__2 = *n;
	for (j = l; j <= i__2; ++j) {
	    s = (float)0.;

	    i__3 = *m;
	    for (k = i__; k <= i__3; ++k) {
		s += u[k + i__ * u_dim1] * u[k + j * u_dim1];
/* L70: */
	    }

	    f = s / h__;

	    i__3 = *m;
	    for (k = i__; k <= i__3; ++k) {
		u[k + j * u_dim1] += f * u[k + i__ * u_dim1];
/* L80: */
	    }
/* L90: */
	}

/*      APPLY LEFT TRANSFORMATIONS TO THE COLUMNS OF B IF IRHS .GT. 0 */

L100:
	if (*irhs == 0) {
	    goto L140;
	}
	i__2 = *irhs;
	for (j = 1; j <= i__2; ++j) {
	    s = (float)0.;
	    i__3 = *m;
	    for (k = i__; k <= i__3; ++k) {
		s += u[k + i__ * u_dim1] * b[k + j * b_dim1];
/* L110: */
	    }
	    f = s / h__;
	    i__3 = *m;
	    for (k = i__; k <= i__3; ++k) {
		b[k + j * b_dim1] += f * u[k + i__ * u_dim1];
/* L120: */
	    }
/* L130: */
	}

/*     COMPUTE RIGHT TRANSFORMATIONS. */

L140:
	i__2 = *m;
	for (k = i__; k <= i__2; ++k) {
	    u[k + i__ * u_dim1] = scale * u[k + i__ * u_dim1];
/* L150: */
	}

L160:
	w[i__] = scale * g;
	g = (float)0.;
	s = (float)0.;
	scale = (float)0.;
	if (i__ > *m || i__ == *n) {
	    goto L250;
	}

	i__2 = *n;
	for (k = l; k <= i__2; ++k) {
	    scale += (r__1 = u[i__ + k * u_dim1], dabs(r__1));
/* L170: */
	}

	if (scale == (float)0.) {
	    goto L250;
	}

	i__2 = *n;
	for (k = l; k <= i__2; ++k) {
	    u[i__ + k * u_dim1] /= scale;
/* Computing 2nd power */
	    r__1 = u[i__ + k * u_dim1];
	    s += r__1 * r__1;
/* L180: */
	}

	f = u[i__ + l * u_dim1];
	r__1 = sqrt(s);
	g = -r_sign(&r__1, &f);
	h__ = f * g - s;
	u[i__ + l * u_dim1] = f - g;

	i__2 = *n;
	for (k = l; k <= i__2; ++k) {
	    rv1[k] = u[i__ + k * u_dim1] / h__;
/* L190: */
	}

	if (i__ == *m) {
	    goto L230;
	}

	i__2 = *m;
	for (j = l; j <= i__2; ++j) {
	    s = (float)0.;

	    i__3 = *n;
	    for (k = l; k <= i__3; ++k) {
		s += u[j + k * u_dim1] * u[i__ + k * u_dim1];
/* L200: */
	    }

	    i__3 = *n;
	    for (k = l; k <= i__3; ++k) {
		u[j + k * u_dim1] += s * rv1[k];
/* L210: */
	    }
/* L220: */
	}

L230:
	i__2 = *n;
	for (k = l; k <= i__2; ++k) {
	    u[i__ + k * u_dim1] = scale * u[i__ + k * u_dim1];
/* L240: */
	}

L250:
/* Computing MAX */
	r__3 = x, r__4 = (r__1 = w[i__], dabs(r__1)) + (r__2 = rv1[i__], dabs(
		r__2));
	x = dmax(r__3,r__4);
/* L260: */
    }
/*     ********** ACCUMULATION OF RIGHT-HAND TRANSFORMATIONS ********** */
    if (! (*matv)) {
	goto L350;
    }
/*     ********** FOR I=N STEP -1 UNTIL 1 DO -- ********** */
    i__1 = *n;
    for (ii = 1; ii <= i__1; ++ii) {
	i__ = *n + 1 - ii;
	if (i__ == *n) {
	    goto L330;
	}
	if (g == (float)0.) {
	    goto L310;
	}

	i__2 = *n;
	for (j = l; j <= i__2; ++j) {
/*     ********** DOUBLE DIVISION AVOIDS POSSIBLE UNDERFLOW *****
***** */
	    v[j + i__ * v_dim1] = u[i__ + j * u_dim1] / u[i__ + l * u_dim1] / 
		    g;
/* L270: */
	}

	i__2 = *n;
	for (j = l; j <= i__2; ++j) {
	    s = (float)0.;

	    i__3 = *n;
	    for (k = l; k <= i__3; ++k) {
		s += u[i__ + k * u_dim1] * v[k + j * v_dim1];
/* L280: */
	    }

	    i__3 = *n;
	    for (k = l; k <= i__3; ++k) {
		v[k + j * v_dim1] += s * v[k + i__ * v_dim1];
/* L290: */
	    }
/* L300: */
	}

L310:
	i__2 = *n;
	for (j = l; j <= i__2; ++j) {
	    v[i__ + j * v_dim1] = (float)0.;
	    v[j + i__ * v_dim1] = (float)0.;
/* L320: */
	}

L330:
	v[i__ + i__ * v_dim1] = (float)1.;
	g = rv1[i__];
	l = i__;
/* L340: */
    }
/*     ********** ACCUMULATION OF LEFT-HAND TRANSFORMATIONS ********** */
L350:
    if (! (*matu)) {
	goto L470;
    }
/*     **********FOR I=MIN(M,N) STEP -1 UNTIL 1 DO -- ********** */
    mn = *n;
    if (*m < *n) {
	mn = *m;
    }

    i__1 = mn;
    for (ii = 1; ii <= i__1; ++ii) {
	i__ = mn + 1 - ii;
	l = i__ + 1;
	g = w[i__];
	if (i__ == *n) {
	    goto L370;
	}

	i__2 = *n;
	for (j = l; j <= i__2; ++j) {
	    u[i__ + j * u_dim1] = (float)0.;
/* L360: */
	}

L370:
	if (g == (float)0.) {
	    goto L430;
	}
	if (i__ == mn) {
	    goto L410;
	}

	i__2 = *n;
	for (j = l; j <= i__2; ++j) {
	    s = (float)0.;

	    i__3 = *m;
	    for (k = l; k <= i__3; ++k) {
		s += u[k + i__ * u_dim1] * u[k + j * u_dim1];
/* L380: */
	    }
/*     ********** DOUBLE DIVISION AVOIDS POSSIBLE UNDERFLOW *****
***** */
	    f = s / u[i__ + i__ * u_dim1] / g;

	    i__3 = *m;
	    for (k = i__; k <= i__3; ++k) {
		u[k + j * u_dim1] += f * u[k + i__ * u_dim1];
/* L390: */
	    }
/* L400: */
	}

L410:
	i__2 = *m;
	for (j = i__; j <= i__2; ++j) {
	    u[j + i__ * u_dim1] /= g;
/* L420: */
	}

	goto L450;

L430:
	i__2 = *m;
	for (j = i__; j <= i__2; ++j) {
	    u[j + i__ * u_dim1] = (float)0.;
/* L440: */
	}

L450:
	u[i__ + i__ * u_dim1] += (float)1.;
/* L460: */
    }
/*     ********** DIAGONALIZATION OF THE BIDIAGONAL FORM ********** */
L470:
    eps = srelpr_() * x;
/*     ********** FOR K=N STEP -1 UNTIL 1 DO -- ********** */
    i__1 = *n;
    for (kk = 1; kk <= i__1; ++kk) {
	k1 = *n - kk;
	k = k1 + 1;
	its = 0;
/*     ********** TEST FOR SPLITTING. */
/*                FOR L=K STEP -1 UNTIL 1 DO -- ********** */
L480:
	i__2 = k;
	for (ll = 1; ll <= i__2; ++ll) {
	    l1 = k - ll;
	    l = l1 + 1;
	    if ((r__1 = rv1[l], dabs(r__1)) <= eps) {
		goto L550;
	    }
/*     ********** RV1(1) IS ALWAYS ZERO, SO THERE IS NO EXIT */
/*                THROUGH THE BOTTOM OF THE LOOP ********** */
	    if ((r__1 = w[l1], dabs(r__1)) <= eps) {
		goto L500;
	    }
/* L490: */
	}
/*     ********** CANCELLATION OF RV1(L) IF L GREATER THAN 1 ********
** */
L500:
	c__ = (float)0.;
	s = (float)1.;

	i__2 = k;
	for (i__ = l; i__ <= i__2; ++i__) {
	    f = s * rv1[i__];
	    rv1[i__] = c__ * rv1[i__];
	    if (dabs(f) <= eps) {
		goto L550;
	    }
	    g = w[i__];
	    h__ = sqrt(f * f + g * g);
	    w[i__] = h__;
	    c__ = g / h__;
	    s = -f / h__;

/*     APPLY LEFT TRANSFORMATIONS TO B IF IRHS .GT. 0 */

	    if (*irhs == 0) {
		goto L520;
	    }
	    i__3 = *irhs;
	    for (j = 1; j <= i__3; ++j) {
		y = b[l1 + j * b_dim1];
		z__ = b[i__ + j * b_dim1];
		b[l1 + j * b_dim1] = y * c__ + z__ * s;
		b[i__ + j * b_dim1] = -y * s + z__ * c__;
/* L510: */
	    }
L520:

	    if (! (*matu)) {
		goto L540;
	    }

	    i__3 = *m;
	    for (j = 1; j <= i__3; ++j) {
		y = u[j + l1 * u_dim1];
		z__ = u[j + i__ * u_dim1];
		u[j + l1 * u_dim1] = y * c__ + z__ * s;
		u[j + i__ * u_dim1] = -y * s + z__ * c__;
/* L530: */
	    }

L540:
	    ;
	}
/*     ********** TEST FOR CONVERGENCE ********** */
L550:
	z__ = w[k];
	if (l == k) {
	    goto L630;
	}
/*     ********** SHIFT FROM BOTTOM 2 BY 2 MINOR ********** */
	if (its == 30) {
	    goto L660;
	}
	++its;
	x = w[l];
	y = w[k1];
	g = rv1[k1];
	h__ = rv1[k];
	f = ((y - z__) * (y + z__) + (g - h__) * (g + h__)) / (h__ * (float)
		2. * y);
	g = sqrt(f * f + (float)1.);
	f = ((x - z__) * (x + z__) + h__ * (y / (f + r_sign(&g, &f)) - h__)) /
		 x;
/*     ********** NEXT QR TRANSFORMATION ********** */
	c__ = (float)1.;
	s = (float)1.;

	i__2 = k1;
	for (i1 = l; i1 <= i__2; ++i1) {
	    i__ = i1 + 1;
	    g = rv1[i__];
	    y = w[i__];
	    h__ = s * g;
	    g = c__ * g;
	    z__ = sqrt(f * f + h__ * h__);
	    rv1[i1] = z__;
	    c__ = f / z__;
	    s = h__ / z__;
	    f = x * c__ + g * s;
	    g = -x * s + g * c__;
	    h__ = y * s;
	    y *= c__;
	    if (! (*matv)) {
		goto L570;
	    }

	    i__3 = *n;
	    for (j = 1; j <= i__3; ++j) {
		x = v[j + i1 * v_dim1];
		z__ = v[j + i__ * v_dim1];
		v[j + i1 * v_dim1] = x * c__ + z__ * s;
		v[j + i__ * v_dim1] = -x * s + z__ * c__;
/* L560: */
	    }

L570:
	    z__ = sqrt(f * f + h__ * h__);
	    w[i1] = z__;
/*     ********** ROTATION CAN BE ARBITRARY IF Z IS ZERO ********
** */
	    if (z__ == (float)0.) {
		goto L580;
	    }
	    c__ = f / z__;
	    s = h__ / z__;
L580:
	    f = c__ * g + s * y;
	    x = -s * g + c__ * y;

/*     APPLY LEFT TRANSFORMATIONS TO B IF IRHS .GT. 0 */

	    if (*irhs == 0) {
		goto L600;
	    }
	    i__3 = *irhs;
	    for (j = 1; j <= i__3; ++j) {
		y = b[i1 + j * b_dim1];
		z__ = b[i__ + j * b_dim1];
		b[i1 + j * b_dim1] = y * c__ + z__ * s;
		b[i__ + j * b_dim1] = -y * s + z__ * c__;
/* L590: */
	    }
L600:

	    if (! (*matu)) {
		goto L620;
	    }

	    i__3 = *m;
	    for (j = 1; j <= i__3; ++j) {
		y = u[j + i1 * u_dim1];
		z__ = u[j + i__ * u_dim1];
		u[j + i1 * u_dim1] = y * c__ + z__ * s;
		u[j + i__ * u_dim1] = -y * s + z__ * c__;
/* L610: */
	    }

L620:
	    ;
	}

	rv1[l] = (float)0.;
	rv1[k] = f;
	w[k] = x;
	goto L480;
/*     ********** CONVERGENCE ********** */
L630:
	if (z__ >= (float)0.) {
	    goto L650;
	}
/*     ********** W(K) IS MADE NON-NEGATIVE ********** */
	w[k] = -z__;
	if (! (*matv)) {
	    goto L650;
	}

	i__2 = *n;
	for (j = 1; j <= i__2; ++j) {
	    v[j + k * v_dim1] = -v[j + k * v_dim1];
/* L640: */
	}

L650:
	;
    }

    goto L670;
/*     ********** SET ERROR -- NO CONVERGENCE TO A */
/*                SINGULAR VALUE AFTER 30 ITERATIONS ********** */
L660:
    *ierr = k;
L670:
    return 0;
/*     ********** LAST CARD OF GRSVD ********** */
} /* grsvd_ */

/* Subroutine */ int sswap_(n, sx, incx, sy, incy)
integer *n;
real *sx;
integer *incx;
real *sy;
integer *incy;
{
    /* System generated locals */
    integer i__1;

    /* Local variables */
    static integer i__, m;
    static real stemp;
    static integer ix, iy, mp1;


/*     INTERCHANGES TWO VECTORS. */
/*     USES UNROLLED LOOPS FOR INCREMENTS EQUAL TO 1. */
/*     JACK DONGARRA, LINPACK, 3/11/78. */


    /* Parameter adjustments */
    --sy;
    --sx;

    /* Function Body */
    if (*n <= 0) {
	return 0;
    }
    if (*incx == 1 && *incy == 1) {
	goto L20;
    }

/*       CODE FOR UNEQUAL INCREMENTS OR EQUAL INCREMENTS NOT EQUAL */
/*         TO 1 */

    ix = 1;
    iy = 1;
    if (*incx < 0) {
	ix = (-(*n) + 1) * *incx + 1;
    }
    if (*incy < 0) {
	iy = (-(*n) + 1) * *incy + 1;
    }
    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
	stemp = sx[ix];
	sx[ix] = sy[iy];
	sy[iy] = stemp;
	ix += *incx;
	iy += *incy;
/* L10: */
    }
    return 0;

/*       CODE FOR BOTH INCREMENTS EQUAL TO 1 */


/*       CLEAN-UP LOOP */

L20:
    m = *n % 3;
    if (m == 0) {
	goto L40;
    }
    i__1 = m;
    for (i__ = 1; i__ <= i__1; ++i__) {
	stemp = sx[i__];
	sx[i__] = sy[i__];
	sy[i__] = stemp;
/* L30: */
    }
    if (*n < 3) {
	return 0;
    }
L40:
    mp1 = m + 1;
    i__1 = *n;
    for (i__ = mp1; i__ <= i__1; i__ += 3) {
	stemp = sx[i__];
	sx[i__] = sy[i__];
	sy[i__] = stemp;
	stemp = sx[i__ + 1];
	sx[i__ + 1] = sy[i__ + 1];
	sy[i__ + 1] = stemp;
	stemp = sx[i__ + 2];
	sx[i__ + 2] = sy[i__ + 2];
	sy[i__ + 2] = stemp;
/* L50: */
    }
    return 0;
} /* sswap_ */

doublereal srelpr_()
{
    /* System generated locals */
    real ret_val;

    /* Local variables */
    static real s;


/*     SRELPR COMPUTES THE RELATIVE PRECISION OF THE FLOATING POINT */
/*     ARITHMETIC OF THE MACHINE. */

/*     IF TROUBLE WITH AUTOMATIC COMPUTATION OF THESE QUANTITIES, */
/*     THEY CAN BE SET BY DIRECT ASSIGNMENT STATEMENTS. */
/*     ASSUME THE COMPUTER HAS */

/*        B = BASE OF ARITHMETIC */
/*        T = NUMBER OF BASE  B  DIGITS */

/*     THEN */

/*        SRELPR = B**(1-T) */


    ret_val = (float)1.;
L10:
    ret_val /= (float)2.;
    s = ret_val + (float)1.;
    if (s > (float)1.) {
	goto L10;
    }
    ret_val *= (float)2.;
    return ret_val;
} /* srelpr_ */
