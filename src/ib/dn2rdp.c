/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* dn2rdp.f -- translated by f2c (version 20090411).
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

/* Subroutine */ int dn2rdp_(integer *iv, integer *liv, integer *lv, integer *
	n, doublereal *rd, doublereal *v)
{
    /* Format strings */
    static char fmt_20[] = "(/\002 REGRESSION DIAGNOSTIC = SQRT( G(I)**T * H"
	    "(I)**-1 * G(I) / ABS(F) )...\002/(6d12.3))";
    static char fmt_40[] = "(/\002 REGRESSION DIAGNOSTIC = SQRT( G(I)**T * H"
	    "(I)**-1 * G(I) )...\002/(6d12.3))";

    /* System generated locals */
    integer rd_dim1, i__1;

    /* Builtin functions */
    integer s_wsfe(cilist *), do_fio(integer *, char *, ftnlen), e_wsfe(void);

    /* Local variables */
    static integer pu;

    /* Fortran I/O blocks */
    static cilist io___2 = { 0, 0, 0, fmt_20, 0 };
    static cilist io___3 = { 0, 0, 0, fmt_40, 0 };



/*  ***  PRINT REGRESSION DIAGNOSTICS FOR MLPSL AND NL2S1 *** */


/*     ***  NOTE -- V IS PASSED FOR POSSIBLE USE BY REVISED VERSIONS OF */
/*     ***  THIS ROUTINE. */


/*  ***  IV AND V SUBSCRIPTS  *** */


/* /6 */
/*     DATA COVPRT/14/, F/10/, NEEDHD/36/, PRUNIT/21/, REGD/67/ */
/* /7 */
/* / */

/* +++++++++++++++++++++++++++++++  BODY  ++++++++++++++++++++++++++++++++ */

    /* Parameter adjustments */
    --iv;
    --v;
    rd_dim1 = *n;
    --rd;

    /* Function Body */
    pu = iv[21];
    if (pu == 0) {
	goto L999;
    }
    if (iv[14] < 2) {
	goto L999;
    }
    if (iv[67] <= 0) {
	goto L999;
    }
    iv[36] = 1;
    if (v[10] != 0.) {
	goto L10;
    } else {
	goto L30;
    }
L10:
    io___2.ciunit = pu;
    s_wsfe(&io___2);
    i__1 = 1 * rd_dim1;
    do_fio(&i__1, (char *)&rd[1], (ftnlen)sizeof(doublereal));
    e_wsfe();
    goto L999;
L30:
    io___3.ciunit = pu;
    s_wsfe(&io___3);
    i__1 = 1 * rd_dim1;
    do_fio(&i__1, (char *)&rd[1], (ftnlen)sizeof(doublereal));
    e_wsfe();

L999:
    return 0;
/*  ***  LAST LINE OF DN2RDP FOLLOWS  *** */
} /* dn2rdp_ */

