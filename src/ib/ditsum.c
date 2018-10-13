/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* ditsum.f -- translated by f2c (version 20090411).
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

/* Subroutine */ int ditsum_(doublereal *d__, doublereal *g, integer *iv, 
	integer *liv, integer *lv, integer *p, doublereal *v, doublereal *x)
{
    /* Initialized data */

    static char model1[4*6] = "    " "    " "    " "    " "  G " "  S ";
    static char model2[4*6] = " G  " " S  " "G-S " "S-G " "-S-G" "-G-S";

    /* Format strings */
    static char fmt_30[] = "(/\002   IT   NF\002,6x,\002F\002,7x,\002RELD"
	    "F\002,3x,\002PRELDF\002,3x,\002RELDX\002,2x,\002MODEL  STPPAR"
	    "\002)";
    static char fmt_40[] = "(/\002    IT   NF\002,7x,\002F\002,8x,\002RELD"
	    "F\002,4x,\002PRELDF\002,4x,\002RELDX\002,3x,\002STPPAR\002)";
    static char fmt_100[] = "(i6,i5,d10.3,2d9.2,d8.1,a3,a4,2d8.1,d9.2)";
    static char fmt_110[] = "(i6,i5,d11.3,2d10.2,3d9.1,d10.2)";
    static char fmt_70[] = "(/\002    IT   NF\002,6x,\002F\002,7x,\002RELD"
	    "F\002,3x,\002PRELDF\002,3x,\002RELDX\002,2x,\002MODEL  STPPAR"
	    "\002,2x,\002D*STEP\002,2x,\002NPRELDF\002)";
    static char fmt_80[] = "(/\002    IT   NF\002,7x,\002F\002,8x,\002RELD"
	    "F\002,4x,\002PRELDF\002,4x,\002RELDX\002,3x,\002STPPAR\002,3x"
	    ",\002D*STEP\002,3x,\002NPRELDF\002)";
    static char fmt_140[] = "(/\002 ***** X-CONVERGENCE *****\002)";
    static char fmt_160[] = "(/\002 ***** RELATIVE FUNCTION CONVERGENCE **"
	    "***\002)";
    static char fmt_180[] = "(/\002 ***** X- AND RELATIVE FUNCTION CONVERGEN"
	    "CE *****\002)";
    static char fmt_200[] = "(/\002 ***** ABSOLUTE FUNCTION CONVERGENCE **"
	    "***\002)";
    static char fmt_220[] = "(/\002 ***** SINGULAR CONVERGENCE *****\002)";
    static char fmt_240[] = "(/\002 ***** FALSE CONVERGENCE *****\002)";
    static char fmt_260[] = "(/\002 ***** FUNCTION EVALUATION LIMIT *****"
	    "\002)";
    static char fmt_280[] = "(/\002 ***** ITERATION LIMIT *****\002)";
    static char fmt_300[] = "(/\002 ***** STOPX *****\002)";
    static char fmt_320[] = "(/\002 ***** INITIAL F(X) CANNOT BE COMPUTED **"
	    "***\002)";
    static char fmt_340[] = "(/\002 ***** BAD PARAMETERS TO ASSESS *****\002)"
	    ;
    static char fmt_360[] = "(/\002 ***** GRADIENT COULD NOT BE COMPUTED ***"
	    "**\002)";
    static char fmt_380[] = "(/\002 ***** IV(1) =\002,i5,\002 *****\002)";
    static char fmt_400[] = "(/\002     I     INITIAL X(I)\002,8x,\002D(I"
	    ")\002//(1x,i5,d17.6,d14.3))";
    static char fmt_410[] = "(/\002     0\002,i5,d10.3)";
    static char fmt_420[] = "(/\002     0\002,i5,d11.3)";
    static char fmt_450[] = "(/\002 FUNCTION\002,d17.6,\002   RELDX\002,d17."
	    "3/\002 FUNC. EVALS\002,i8,9x,\002GRAD. EVALS\002,i8/\002 PRELD"
	    "F\002,d16.3,6x,\002NPRELDF\002,d15.3)";
    static char fmt_470[] = "(/\002     I      FINAL X(I)\002,8x,\002D(I)"
	    "\002,10x,\002G(I)\002/)";
    static char fmt_490[] = "(1x,i5,d16.6,2d14.3)";
    static char fmt_510[] = "(/\002 INCONSISTENT DIMENSIONS\002)";

    /* System generated locals */
    integer i__1;
    doublereal d__1, d__2;

    /* Builtin functions */
    integer s_wsfe(cilist *), e_wsfe(void), do_fio(integer *, char *, ftnlen);

    /* Local variables */
    static integer i__, m, nf, ng, ol, pu, iv1, alg;
    static doublereal oldf, reldf, nreldf, preldf;

    /* Fortran I/O blocks */
    static cilist io___11 = { 0, 0, 0, fmt_30, 0 };
    static cilist io___12 = { 0, 0, 0, fmt_40, 0 };
    static cilist io___14 = { 0, 0, 0, fmt_100, 0 };
    static cilist io___15 = { 0, 0, 0, fmt_110, 0 };
    static cilist io___16 = { 0, 0, 0, fmt_70, 0 };
    static cilist io___17 = { 0, 0, 0, fmt_80, 0 };
    static cilist io___19 = { 0, 0, 0, fmt_100, 0 };
    static cilist io___20 = { 0, 0, 0, fmt_110, 0 };
    static cilist io___22 = { 0, 0, 0, fmt_140, 0 };
    static cilist io___23 = { 0, 0, 0, fmt_160, 0 };
    static cilist io___24 = { 0, 0, 0, fmt_180, 0 };
    static cilist io___25 = { 0, 0, 0, fmt_200, 0 };
    static cilist io___26 = { 0, 0, 0, fmt_220, 0 };
    static cilist io___27 = { 0, 0, 0, fmt_240, 0 };
    static cilist io___28 = { 0, 0, 0, fmt_260, 0 };
    static cilist io___29 = { 0, 0, 0, fmt_280, 0 };
    static cilist io___30 = { 0, 0, 0, fmt_300, 0 };
    static cilist io___31 = { 0, 0, 0, fmt_320, 0 };
    static cilist io___32 = { 0, 0, 0, fmt_340, 0 };
    static cilist io___33 = { 0, 0, 0, fmt_360, 0 };
    static cilist io___34 = { 0, 0, 0, fmt_380, 0 };
    static cilist io___35 = { 0, 0, 0, fmt_400, 0 };
    static cilist io___36 = { 0, 0, 0, fmt_30, 0 };
    static cilist io___37 = { 0, 0, 0, fmt_40, 0 };
    static cilist io___38 = { 0, 0, 0, fmt_70, 0 };
    static cilist io___39 = { 0, 0, 0, fmt_80, 0 };
    static cilist io___40 = { 0, 0, 0, fmt_410, 0 };
    static cilist io___41 = { 0, 0, 0, fmt_420, 0 };
    static cilist io___43 = { 0, 0, 0, fmt_450, 0 };
    static cilist io___44 = { 0, 0, 0, fmt_470, 0 };
    static cilist io___45 = { 0, 0, 0, fmt_490, 0 };
    static cilist io___46 = { 0, 0, 0, fmt_510, 0 };



/*  ***  PRINT ITERATION SUMMARY FOR ***SOL (VERSION 2.3)  *** */

/*  ***  PARAMETER DECLARATIONS  *** */


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

/*  ***  LOCAL VARIABLES  *** */

/* /6S */
/*     REAL MODEL1(6), MODEL2(6) */
/* /7S */
/* / */

/*  ***  NO EXTERNAL FUNCTIONS OR SUBROUTINES  *** */

/*  ***  SUBSCRIPTS FOR IV AND V  *** */


/*  ***  IV SUBSCRIPT VALUES  *** */

/* /6 */
/*     DATA ALGSAV/51/, NEEDHD/36/, NFCALL/6/, NFCOV/52/, NGCALL/30/, */
/*    1     NGCOV/53/, NITER/31/, OUTLEV/19/, PRNTIT/39/, PRUNIT/21/, */
/*    2     SOLPRT/22/, STATPR/23/, SUSED/64/, X0PRT/24/ */
/* /7 */
/* / */

/*  ***  V SUBSCRIPT VALUES  *** */

/* /6 */
/*     DATA DSTNRM/2/, F/10/, F0/13/, FDIF/11/, NREDUC/6/, PREDUC/7/, */
/*    1     RELDX/17/, STPPAR/5/ */
/* /7 */
/* / */

/* /6 */
/*     DATA ZERO/0.D+0/ */
/* /7 */
/* / */
/* /6S */
/*     DATA MODEL1(1)/4H    /, MODEL1(2)/4H    /, MODEL1(3)/4H    /, */
/*    1     MODEL1(4)/4H    /, MODEL1(5)/4H  G /, MODEL1(6)/4H  S /, */
/*    2     MODEL2(1)/4H G  /, MODEL2(2)/4H S  /, MODEL2(3)/4HG-S /, */
/*    3     MODEL2(4)/4HS-G /, MODEL2(5)/4H-S-G/, MODEL2(6)/4H-G-S/ */
/* /7S */
    /* Parameter adjustments */
    --iv;
    --v;
    --x;
    --g;
    --d__;

    /* Function Body */
/* / */

/* -------------------------------  BODY  -------------------------------- */

    pu = iv[21];
    if (pu == 0) {
	goto L999;
    }
    iv1 = iv[1];
    if (iv1 > 62) {
	iv1 += -51;
    }
    ol = iv[19];
    alg = (iv[51] - 1) % 2 + 1;
    if (iv1 < 2 || iv1 > 15) {
	goto L370;
    }
    if (iv1 >= 12) {
	goto L120;
    }
    if (iv1 == 2 && iv[31] == 0) {
	goto L390;
    }
    if (ol == 0) {
	goto L120;
    }
    if (iv1 >= 10 && iv[39] == 0) {
	goto L120;
    }
    if (iv1 > 2) {
	goto L10;
    }
    ++iv[39];
    if (iv[39] < abs(ol)) {
	goto L999;
    }
L10:
    nf = iv[6] - abs(iv[52]);
    iv[39] = 0;
    reldf = 0.;
    preldf = 0.;
/* Computing MAX */
    d__1 = abs(v[13]), d__2 = abs(v[10]);
    oldf = max(d__1,d__2);
    if (oldf <= 0.) {
	goto L20;
    }
    reldf = v[11] / oldf;
    preldf = v[7] / oldf;
L20:
    if (ol > 0) {
	goto L60;
    }

/*        ***  PRINT SHORT SUMMARY LINE  *** */

    if (iv[36] == 1 && alg == 1) {
	io___11.ciunit = pu;
	s_wsfe(&io___11);
	e_wsfe();
    }
    if (iv[36] == 1 && alg == 2) {
	io___12.ciunit = pu;
	s_wsfe(&io___12);
	e_wsfe();
    }
    iv[36] = 0;
    if (alg == 2) {
	goto L50;
    }
    m = iv[64];
    io___14.ciunit = pu;
    s_wsfe(&io___14);
    do_fio(&c__1, (char *)&iv[31], (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&nf, (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&v[10], (ftnlen)sizeof(doublereal));
    do_fio(&c__1, (char *)&reldf, (ftnlen)sizeof(doublereal));
    do_fio(&c__1, (char *)&preldf, (ftnlen)sizeof(doublereal));
    do_fio(&c__1, (char *)&v[17], (ftnlen)sizeof(doublereal));
    do_fio(&c__1, model1 + (m - 1 << 2), (ftnlen)4);
    do_fio(&c__1, model2 + (m - 1 << 2), (ftnlen)4);
    do_fio(&c__1, (char *)&v[5], (ftnlen)sizeof(doublereal));
    e_wsfe();
    goto L120;

L50:
    io___15.ciunit = pu;
    s_wsfe(&io___15);
    do_fio(&c__1, (char *)&iv[31], (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&nf, (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&v[10], (ftnlen)sizeof(doublereal));
    do_fio(&c__1, (char *)&reldf, (ftnlen)sizeof(doublereal));
    do_fio(&c__1, (char *)&preldf, (ftnlen)sizeof(doublereal));
    do_fio(&c__1, (char *)&v[17], (ftnlen)sizeof(doublereal));
    do_fio(&c__1, (char *)&v[5], (ftnlen)sizeof(doublereal));
    e_wsfe();
    goto L120;

/*     ***  PRINT LONG SUMMARY LINE  *** */

L60:
    if (iv[36] == 1 && alg == 1) {
	io___16.ciunit = pu;
	s_wsfe(&io___16);
	e_wsfe();
    }
    if (iv[36] == 1 && alg == 2) {
	io___17.ciunit = pu;
	s_wsfe(&io___17);
	e_wsfe();
    }
    iv[36] = 0;
    nreldf = 0.;
    if (oldf > 0.) {
	nreldf = v[6] / oldf;
    }
    if (alg == 2) {
	goto L90;
    }
    m = iv[64];
    io___19.ciunit = pu;
    s_wsfe(&io___19);
    do_fio(&c__1, (char *)&iv[31], (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&nf, (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&v[10], (ftnlen)sizeof(doublereal));
    do_fio(&c__1, (char *)&reldf, (ftnlen)sizeof(doublereal));
    do_fio(&c__1, (char *)&preldf, (ftnlen)sizeof(doublereal));
    do_fio(&c__1, (char *)&v[17], (ftnlen)sizeof(doublereal));
    do_fio(&c__1, model1 + (m - 1 << 2), (ftnlen)4);
    do_fio(&c__1, model2 + (m - 1 << 2), (ftnlen)4);
    do_fio(&c__1, (char *)&v[5], (ftnlen)sizeof(doublereal));
    do_fio(&c__1, (char *)&v[2], (ftnlen)sizeof(doublereal));
    do_fio(&c__1, (char *)&nreldf, (ftnlen)sizeof(doublereal));
    e_wsfe();
    goto L120;

L90:
    io___20.ciunit = pu;
    s_wsfe(&io___20);
    do_fio(&c__1, (char *)&iv[31], (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&nf, (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&v[10], (ftnlen)sizeof(doublereal));
    do_fio(&c__1, (char *)&reldf, (ftnlen)sizeof(doublereal));
    do_fio(&c__1, (char *)&preldf, (ftnlen)sizeof(doublereal));
    do_fio(&c__1, (char *)&v[17], (ftnlen)sizeof(doublereal));
    do_fio(&c__1, (char *)&v[5], (ftnlen)sizeof(doublereal));
    do_fio(&c__1, (char *)&v[2], (ftnlen)sizeof(doublereal));
    do_fio(&c__1, (char *)&nreldf, (ftnlen)sizeof(doublereal));
    e_wsfe();

L120:
    if (iv1 <= 2) {
	goto L999;
    }
    i__ = iv[23];
    if (i__ == -1) {
	goto L460;
    }
    if (i__ + iv1 < 0) {
	goto L460;
    }
    switch (iv1) {
	case 1:  goto L999;
	case 2:  goto L999;
	case 3:  goto L130;
	case 4:  goto L150;
	case 5:  goto L170;
	case 6:  goto L190;
	case 7:  goto L210;
	case 8:  goto L230;
	case 9:  goto L250;
	case 10:  goto L270;
	case 11:  goto L290;
	case 12:  goto L310;
	case 13:  goto L330;
	case 14:  goto L350;
	case 15:  goto L500;
    }

L130:
    io___22.ciunit = pu;
    s_wsfe(&io___22);
    e_wsfe();
    goto L430;

L150:
    io___23.ciunit = pu;
    s_wsfe(&io___23);
    e_wsfe();
    goto L430;

L170:
    io___24.ciunit = pu;
    s_wsfe(&io___24);
    e_wsfe();
    goto L430;

L190:
    io___25.ciunit = pu;
    s_wsfe(&io___25);
    e_wsfe();
    goto L430;

L210:
    io___26.ciunit = pu;
    s_wsfe(&io___26);
    e_wsfe();
    goto L430;

L230:
    io___27.ciunit = pu;
    s_wsfe(&io___27);
    e_wsfe();
    goto L430;

L250:
    io___28.ciunit = pu;
    s_wsfe(&io___28);
    e_wsfe();
    goto L430;

L270:
    io___29.ciunit = pu;
    s_wsfe(&io___29);
    e_wsfe();
    goto L430;

L290:
    io___30.ciunit = pu;
    s_wsfe(&io___30);
    e_wsfe();
    goto L430;

L310:
    io___31.ciunit = pu;
    s_wsfe(&io___31);
    e_wsfe();

    goto L390;

L330:
    io___32.ciunit = pu;
    s_wsfe(&io___32);
    e_wsfe();
    goto L999;

L350:
    io___33.ciunit = pu;
    s_wsfe(&io___33);
    e_wsfe();
    if (iv[31] > 0) {
	goto L460;
    }
    goto L390;

L370:
    io___34.ciunit = pu;
    s_wsfe(&io___34);
    do_fio(&c__1, (char *)&iv[1], (ftnlen)sizeof(integer));
    e_wsfe();
    goto L999;

/*  ***  INITIAL CALL ON DITSUM  *** */

L390:
    if (iv[24] != 0) {
	io___35.ciunit = pu;
	s_wsfe(&io___35);
	i__1 = *p;
	for (i__ = 1; i__ <= i__1; ++i__) {
	    do_fio(&c__1, (char *)&i__, (ftnlen)sizeof(integer));
	    do_fio(&c__1, (char *)&x[i__], (ftnlen)sizeof(doublereal));
	    do_fio(&c__1, (char *)&d__[i__], (ftnlen)sizeof(doublereal));
	}
	e_wsfe();
    }
/*     *** THE FOLLOWING ARE TO AVOID UNDEFINED VARIABLES WHEN THE */
/*     *** FUNCTION EVALUATION LIMIT IS 1... */
    v[2] = 0.;
    v[11] = 0.;
    v[6] = 0.;
    v[7] = 0.;
    v[17] = 0.;
    if (iv1 >= 12) {
	goto L999;
    }
    iv[36] = 0;
    iv[39] = 0;
    if (ol == 0) {
	goto L999;
    }
    if (ol < 0 && alg == 1) {
	io___36.ciunit = pu;
	s_wsfe(&io___36);
	e_wsfe();
    }
    if (ol < 0 && alg == 2) {
	io___37.ciunit = pu;
	s_wsfe(&io___37);
	e_wsfe();
    }
    if (ol > 0 && alg == 1) {
	io___38.ciunit = pu;
	s_wsfe(&io___38);
	e_wsfe();
    }
    if (ol > 0 && alg == 2) {
	io___39.ciunit = pu;
	s_wsfe(&io___39);
	e_wsfe();
    }
    if (alg == 1) {
	io___40.ciunit = pu;
	s_wsfe(&io___40);
	do_fio(&c__1, (char *)&iv[6], (ftnlen)sizeof(integer));
	do_fio(&c__1, (char *)&v[10], (ftnlen)sizeof(doublereal));
	e_wsfe();
    }
    if (alg == 2) {
	io___41.ciunit = pu;
	s_wsfe(&io___41);
	do_fio(&c__1, (char *)&iv[6], (ftnlen)sizeof(integer));
	do_fio(&c__1, (char *)&v[10], (ftnlen)sizeof(doublereal));
	e_wsfe();
    }
    goto L999;

/*  ***  PRINT VARIOUS INFORMATION REQUESTED ON SOLUTION  *** */

L430:
    iv[36] = 1;
    if (iv[23] <= 0) {
	goto L460;
    }
/* Computing MAX */
    d__1 = abs(v[13]), d__2 = abs(v[10]);
    oldf = max(d__1,d__2);
    preldf = 0.;
    nreldf = 0.;
    if (oldf <= 0.) {
	goto L440;
    }
    preldf = v[7] / oldf;
    nreldf = v[6] / oldf;
L440:
    nf = iv[6] - iv[52];
    ng = iv[30] - iv[53];
    io___43.ciunit = pu;
    s_wsfe(&io___43);
    do_fio(&c__1, (char *)&v[10], (ftnlen)sizeof(doublereal));
    do_fio(&c__1, (char *)&v[17], (ftnlen)sizeof(doublereal));
    do_fio(&c__1, (char *)&nf, (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&ng, (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&preldf, (ftnlen)sizeof(doublereal));
    do_fio(&c__1, (char *)&nreldf, (ftnlen)sizeof(doublereal));
    e_wsfe();

L460:
    if (iv[22] == 0) {
	goto L999;
    }
    iv[36] = 1;
    if (iv[51] > 2) {
	goto L999;
    }
    io___44.ciunit = pu;
    s_wsfe(&io___44);
    e_wsfe();
    i__1 = *p;
    for (i__ = 1; i__ <= i__1; ++i__) {
/* L480: */
	io___45.ciunit = pu;
	s_wsfe(&io___45);
	do_fio(&c__1, (char *)&i__, (ftnlen)sizeof(integer));
	do_fio(&c__1, (char *)&x[i__], (ftnlen)sizeof(doublereal));
	do_fio(&c__1, (char *)&d__[i__], (ftnlen)sizeof(doublereal));
	do_fio(&c__1, (char *)&g[i__], (ftnlen)sizeof(doublereal));
	e_wsfe();
    }
    goto L999;

L500:
    io___46.ciunit = pu;
    s_wsfe(&io___46);
    e_wsfe();
L999:
    return 0;
/*  ***  LAST CARD OF DITSUM FOLLOWS  *** */
} /* ditsum_ */

