/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* dn2cvp.f -- translated by f2c (version 20090411).
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

/* Subroutine */ int dn2cvp_(integer *iv, integer *liv, integer *lv, integer *
	p, doublereal *v)
{
    /* Format strings */
    static char fmt_10[] = "(/1x,i4,\002 EXTRA FUNC. EVALS FOR COVARIANCE AN"
	    "D DIAGNOSTICS.\002)";
    static char fmt_20[] = "(1x,i4,\002 EXTRA GRAD. EVALS FOR COVARIANCE AND"
	    " DIAGNOSTICS.\002)";
    static char fmt_40[] = "(/\002 RECIPROCAL CONDITION OF F.D. HESSIAN = AT"
	    " MOST\002,d10.2)";
    static char fmt_60[] = "(/\002 RECIPROCAL CONDITION OF (J**T)*J = AT LEA"
	    "ST\002,d10.2)";
    static char fmt_90[] = "(/\002 ++++++ INDEFINITE COVARIANCE MATRIX +++"
	    "+++\002)";
    static char fmt_100[] = "(/\002 ++++++ OVERSIZE STEPS IN COMPUTING COVAR"
	    "IANCE +++++\002)";
    static char fmt_120[] = "(/\002 ++++++ COVARIANCE MATRIX NOT COMPUTED ++"
	    "++++\002)";
    static char fmt_140[] = "(/\002 COVARIANCE = SCALE * H**-1 * (J**T * J) "
	    "* H**-1\002/\002 WHERE H = F.D. HESSIAN\002/)";
    static char fmt_150[] = "(/\002 COVARIANCE = H**-1, WHERE H = FINITE-DIF"
	    "FERENCE HESSIAN\002/)";
    static char fmt_160[] = "(/\002 COVARIANCE = SCALE * J**T * J\002/)";
    static char fmt_180[] = "(\002 ROW\002,i3,2x,5d12.3/(9x,5d12.3))";

    /* System generated locals */
    integer i__1, i__2;
    doublereal d__1;

    /* Builtin functions */
    integer s_wsfe(cilist *), do_fio(integer *, char *, ftnlen), e_wsfe(void);

    /* Local variables */
    static integer i__, j;
    static doublereal t;
    static integer i1, ii, pu, cov1;

    /* Fortran I/O blocks */
    static cilist io___2 = { 0, 0, 0, fmt_10, 0 };
    static cilist io___3 = { 0, 0, 0, fmt_20, 0 };
    static cilist io___6 = { 0, 0, 0, fmt_40, 0 };
    static cilist io___7 = { 0, 0, 0, fmt_60, 0 };
    static cilist io___8 = { 0, 0, 0, fmt_90, 0 };
    static cilist io___9 = { 0, 0, 0, fmt_100, 0 };
    static cilist io___10 = { 0, 0, 0, fmt_120, 0 };
    static cilist io___12 = { 0, 0, 0, fmt_140, 0 };
    static cilist io___13 = { 0, 0, 0, fmt_150, 0 };
    static cilist io___14 = { 0, 0, 0, fmt_160, 0 };
    static cilist io___17 = { 0, 0, 0, fmt_180, 0 };



/*  ***  PRINT COVARIANCE MATRIX FOR  DRN2G  *** */


/*  ***  LOCAL VARIABLES  *** */


/*     ***  IV SUBSCRIPTS  *** */


/* /6 */
/*     DATA COVMAT/26/, COVPRT/14/, COVREQ/15/, NEEDHD/36/, NFCOV/52/, */
/*    1     NGCOV/53/, PRUNIT/21/, RCOND/53/, REGD/67/, STATPR/23/ */
/* /7 */
/* / */
/*  ***  BODY  *** */

    /* Parameter adjustments */
    --iv;
    --v;

    /* Function Body */
    if (iv[1] > 8) {
	goto L999;
    }
    pu = iv[21];
    if (pu == 0) {
	goto L999;
    }
    if (iv[23] == 0) {
	goto L30;
    }
    if (iv[52] > 0) {
	io___2.ciunit = pu;
	s_wsfe(&io___2);
	do_fio(&c__1, (char *)&iv[52], (ftnlen)sizeof(integer));
	e_wsfe();
    }
    if (iv[53] > 0) {
	io___3.ciunit = pu;
	s_wsfe(&io___3);
	do_fio(&c__1, (char *)&iv[53], (ftnlen)sizeof(integer));
	e_wsfe();
    }

L30:
    if (iv[14] <= 0) {
	goto L999;
    }
    cov1 = iv[26];
    if (iv[67] <= 0 && cov1 <= 0) {
	goto L70;
    }
    iv[36] = 1;
/* Computing 2nd power */
    d__1 = v[53];
    t = d__1 * d__1;
    if (abs(iv[15]) > 2) {
	goto L50;
    }

    io___6.ciunit = pu;
    s_wsfe(&io___6);
    do_fio(&c__1, (char *)&t, (ftnlen)sizeof(doublereal));
    e_wsfe();
    goto L70;

L50:
    io___7.ciunit = pu;
    s_wsfe(&io___7);
    do_fio(&c__1, (char *)&t, (ftnlen)sizeof(doublereal));
    e_wsfe();

L70:
    if (iv[14] % 2 == 0) {
	goto L999;
    }
    iv[36] = 1;
    if (cov1 < 0) {
	goto L80;
    } else if (cov1 == 0) {
	goto L110;
    } else {
	goto L130;
    }
L80:
    if (-1 == cov1) {
	io___8.ciunit = pu;
	s_wsfe(&io___8);
	e_wsfe();
    }
    if (-2 == cov1) {
	io___9.ciunit = pu;
	s_wsfe(&io___9);
	e_wsfe();
    }
    goto L999;

L110:
    io___10.ciunit = pu;
    s_wsfe(&io___10);
    e_wsfe();
    goto L999;

L130:
    i__ = abs(iv[15]);
    if (i__ <= 1) {
	io___12.ciunit = pu;
	s_wsfe(&io___12);
	e_wsfe();
    }
    if (i__ == 2) {
	io___13.ciunit = pu;
	s_wsfe(&io___13);
	e_wsfe();
    }
    if (i__ > 2) {
	io___14.ciunit = pu;
	s_wsfe(&io___14);
	e_wsfe();
    }
    ii = cov1 - 1;
    i__1 = *p;
    for (i__ = 1; i__ <= i__1; ++i__) {
	i1 = ii + 1;
	ii += i__;
	io___17.ciunit = pu;
	s_wsfe(&io___17);
	do_fio(&c__1, (char *)&i__, (ftnlen)sizeof(integer));
	i__2 = ii;
	for (j = i1; j <= i__2; ++j) {
	    do_fio(&c__1, (char *)&v[j], (ftnlen)sizeof(doublereal));
	}
	e_wsfe();
/* L170: */
    }

L999:
    return 0;
/*  ***  LAST CARD OF DN2CVP FOLLOWS  *** */
} /* dn2cvp_ */

