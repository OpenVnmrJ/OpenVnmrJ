/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* d1mach.f -- translated by f2c (version 20090411).
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

/* Common Block Declarations */

struct {
    integer cray1[38];
} d9mach_;

#define d9mach_1 d9mach_

/* Table of constant values */

static integer c__8285 = 8285;
static integer c_b5 = 8388608;
static integer c__0 = 0;
static integer c__24574 = 24574;
static integer c_b8 = 16777215;
static integer c_b12 = 16777214;
static integer c__16291 = 16291;
static integer c__16292 = 16292;
static integer c__16383 = 16383;
static integer c_b20 = 10100890;
static integer c_b21 = 8715215;
static integer c_b23 = 16226447;
static integer c_b24 = 9001388;
static integer c__9 = 9;
static integer c__1 = 1;
static integer c__3 = 3;

doublereal d1mach_(integer *i__)
{
    /* Initialized data */

    static integer sc = 0;

    /* Format strings */
    static char fmt_9000[] = "(/\002 Adjust D1MACH by uncommenting data stat"
	    "ements\002/\002 appropriate for your machine.\002)";

    /* System generated locals */
    doublereal ret_val;
    static doublereal equiv_4[5];

    /* Builtin functions */
    integer s_wsfe(cilist *), e_wsfe(void);
    /* Subroutine */ int s_stop(char *, ftnlen);
    integer s_wsle(cilist *), do_lio(integer *, integer *, char *, ftnlen), 
	    e_wsle(void);

    /* Local variables */
    static integer j;
#define log10 ((integer *)equiv_4 + 8)
#define dmach (equiv_4)
#define large ((integer *)equiv_4 + 2)
#define small ((integer *)equiv_4)
#define diver ((integer *)equiv_4 + 6)
#define right ((integer *)equiv_4 + 4)
    extern /* Subroutine */ int i1mcry_(integer *, integer *, integer *, 
	    integer *, integer *);

    /* Fortran I/O blocks */
    static cilist io___9 = { 0, 6, 0, fmt_9000, 0 };
    static cilist io___10 = { 0, 6, 0, fmt_9000, 0 };
    static cilist io___11 = { 0, 6, 0, 0, 0 };



/*  DOUBLE-PRECISION MACHINE CONSTANTS */
/*  D1MACH( 1) = B**(EMIN-1), THE SMALLEST POSITIVE MAGNITUDE. */
/*  D1MACH( 2) = B**EMAX*(1 - B**(-T)), THE LARGEST MAGNITUDE. */
/*  D1MACH( 3) = B**(-T), THE SMALLEST RELATIVE SPACING. */
/*  D1MACH( 4) = B**(1-T), THE LARGEST RELATIVE SPACING. */
/*  D1MACH( 5) = LOG10(B) */

/*  THIS VERSION ADAPTS AUTOMATICALLY TO MOST CURRENT MACHINES. */
/*  R1MACH CAN HANDLE AUTO-DOUBLE COMPILING, BUT THIS VERSION OF */
/*  D1MACH DOES NOT, BECAUSE WE DO NOT HAVE QUAD CONSTANTS FOR */
/*  MANY MACHINES YET. */
/*  TO COMPILE ON OLDER MACHINES, ADD A C IN COLUMN 1 */
/*  ON THE NEXT LINE */
/*  AND REMOVE THE C FROM COLUMN 1 IN ONE OF THE SECTIONS BELOW. */
/*  CONSTANTS FOR EVEN OLDER MACHINES CAN BE OBTAINED BY */
/*          mail netlib@research.bell-labs.com */
/*          send old1mach from blas */
/*  PLEASE SEND CORRECTIONS TO dmg OR ehg@bell-labs.com. */

/*     MACHINE CONSTANTS FOR THE HONEYWELL DPS 8/70 SERIES. */
/*      DATA SMALL(1),SMALL(2) / O402400000000, O000000000000 / */
/*      DATA LARGE(1),LARGE(2) / O376777777777, O777777777777 / */
/*      DATA RIGHT(1),RIGHT(2) / O604400000000, O000000000000 / */
/*      DATA DIVER(1),DIVER(2) / O606400000000, O000000000000 / */
/*      DATA LOG10(1),LOG10(2) / O776464202324, O117571775714 /, SC/987/ */

/*     MACHINE CONSTANTS FOR PDP-11 FORTRANS SUPPORTING */
/*     32-BIT INTEGERS. */
/*      DATA SMALL(1),SMALL(2) /    8388608,           0 / */
/*      DATA LARGE(1),LARGE(2) / 2147483647,          -1 / */
/*      DATA RIGHT(1),RIGHT(2) /  612368384,           0 / */
/*      DATA DIVER(1),DIVER(2) /  620756992,           0 / */
/*      DATA LOG10(1),LOG10(2) / 1067065498, -2063872008 /, SC/987/ */

/*     MACHINE CONSTANTS FOR THE UNIVAC 1100 SERIES. */
/*      DATA SMALL(1),SMALL(2) / O000040000000, O000000000000 / */
/*      DATA LARGE(1),LARGE(2) / O377777777777, O777777777777 / */
/*      DATA RIGHT(1),RIGHT(2) / O170540000000, O000000000000 / */
/*      DATA DIVER(1),DIVER(2) / O170640000000, O000000000000 / */
/*      DATA LOG10(1),LOG10(2) / O177746420232, O411757177572 /, SC/987/ */

/*     ON FIRST CALL, IF NO DATA UNCOMMENTED, TEST MACHINE TYPES. */
    if (sc != 987) {
	dmach[0] = 1e13;
	if (small[0] == 1117925532 && small[1] == -448790528) {
/*           *** IEEE BIG ENDIAN *** */
	    small[0] = 1048576;
	    small[1] = 0;
	    large[0] = 2146435071;
	    large[1] = -1;
	    right[0] = 1017118720;
	    right[1] = 0;
	    diver[0] = 1018167296;
	    diver[1] = 0;
	    log10[0] = 1070810131;
	    log10[1] = 1352628735;
	} else if (small[1] == 1117925532 && small[0] == -448790528) {
/*           *** IEEE LITTLE ENDIAN *** */
	    small[1] = 1048576;
	    small[0] = 0;
	    large[1] = 2146435071;
	    large[0] = -1;
	    right[1] = 1017118720;
	    right[0] = 0;
	    diver[1] = 1018167296;
	    diver[0] = 0;
	    log10[1] = 1070810131;
	    log10[0] = 1352628735;
	} else if (small[0] == -2065213935 && small[1] == 10752) {
/*               *** VAX WITH D_FLOATING *** */
	    small[0] = 128;
	    small[1] = 0;
	    large[0] = -32769;
	    large[1] = -1;
	    right[0] = 9344;
	    right[1] = 0;
	    diver[0] = 9472;
	    diver[1] = 0;
	    log10[0] = 546979738;
	    log10[1] = -805796613;
	} else if (small[0] == 1267827943 && small[1] == 704643072) {
/*               *** IBM MAINFRAME *** */
	    small[0] = 1048576;
	    small[1] = 0;
	    large[0] = 2147483647;
	    large[1] = -1;
	    right[0] = 856686592;
	    right[1] = 0;
	    diver[0] = 873463808;
	    diver[1] = 0;
	    log10[0] = 1091781651;
	    log10[1] = 1352628735;
	} else if (small[0] == 1120022684 && small[1] == -448790528) {
/*           *** CONVEX C-1 *** */
	    small[0] = 1048576;
	    small[1] = 0;
	    large[0] = 2147483647;
	    large[1] = -1;
	    right[0] = 1019215872;
	    right[1] = 0;
	    diver[0] = 1020264448;
	    diver[1] = 0;
	    log10[0] = 1072907283;
	    log10[1] = 1352628735;
	} else if (small[0] == 815547074 && small[1] == 58688) {
/*           *** VAX G-FLOATING *** */
	    small[0] = 16;
	    small[1] = 0;
	    large[0] = -32769;
	    large[1] = -1;
	    right[0] = 15552;
	    right[1] = 0;
	    diver[0] = 15568;
	    diver[1] = 0;
	    log10[0] = 1142112243;
	    log10[1] = 2046775455;
	} else {
	    dmach[1] = 1e27;
	    dmach[2] = 1e27;
	    large[1] -= right[1];
	    if (large[1] == 64 && small[1] == 0) {
		d9mach_1.cray1[0] = 67291416;
		for (j = 1; j <= 20; ++j) {
		    d9mach_1.cray1[j] = d9mach_1.cray1[j - 1] + 
			    d9mach_1.cray1[j - 1];
/* L10: */
		}
		d9mach_1.cray1[21] = d9mach_1.cray1[20] + 321322;
		for (j = 22; j <= 37; ++j) {
		    d9mach_1.cray1[j] = d9mach_1.cray1[j - 1] + 
			    d9mach_1.cray1[j - 1];
/* L20: */
		}
		if (d9mach_1.cray1[37] == small[0]) {
/*                  *** CRAY *** */
		    i1mcry_(small, &j, &c__8285, &c_b5, &c__0);
		    small[1] = 0;
		    i1mcry_(large, &j, &c__24574, &c_b8, &c_b8);
		    i1mcry_(&large[1], &j, &c__0, &c_b8, &c_b12);
		    i1mcry_(right, &j, &c__16291, &c_b5, &c__0);
		    right[1] = 0;
		    i1mcry_(diver, &j, &c__16292, &c_b5, &c__0);
		    diver[1] = 0;
		    i1mcry_(log10, &j, &c__16383, &c_b20, &c_b21);
		    i1mcry_(&log10[1], &j, &c__0, &c_b23, &c_b24);
		} else {
		    s_wsfe(&io___9);
		    e_wsfe();
		    s_stop("779", (ftnlen)3);
		}
	    } else {
		s_wsfe(&io___10);
		e_wsfe();
		s_stop("779", (ftnlen)3);
	    }
	}
	sc = 987;
    }
/*    SANITY CHECK */
    if (dmach[3] >= 1.) {
	s_stop("778", (ftnlen)3);
    }
    if (*i__ < 1 || *i__ > 5) {
	s_wsle(&io___11);
	do_lio(&c__9, &c__1, "D1MACH(I): I =", (ftnlen)14);
	do_lio(&c__3, &c__1, (char *)&(*i__), (ftnlen)sizeof(integer));
	do_lio(&c__9, &c__1, " is out of bounds.", (ftnlen)18);
	e_wsle();
	s_stop("", (ftnlen)0);
    }
    ret_val = dmach[*i__ - 1];
    return ret_val;
/* /+ Standard C source for D1MACH -- remove the * in column 1 +/ */
/* #include <stdio.h> */
/* #include <float.h> */
/* #include <math.h> */
/* double d1mach_(long *i) */
/* { */
/* 	switch(*i){ */
/* 	  case 1: return DBL_MIN; */
/* 	  case 2: return DBL_MAX; */
/* 	  case 3: return DBL_EPSILON/FLT_RADIX; */
/* 	  case 4: return DBL_EPSILON; */
/* 	  case 5: return log10((double)FLT_RADIX); */
/* 	  } */
/* 	fprintf(stderr, "invalid argument: d1mach(%ld)\n", *i); */
/* 	exit(1); return 0; /+ some compilers demand return values +/ */
/* } */
} /* d1mach_ */

#undef right
#undef diver
#undef small
#undef large
#undef dmach
#undef log10


/* Subroutine */ int i1mcry_(integer *a, integer *a1, integer *b, integer *
	c__, integer *d__)
{
/* *** SPECIAL COMPUTATION FOR OLD CRAY MACHINES **** */
    *a1 = (*b << 24) + *c__;
    *a = (*a1 << 24) + *d__;
    return 0;
} /* i1mcry_ */

