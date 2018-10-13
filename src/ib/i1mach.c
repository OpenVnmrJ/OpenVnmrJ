/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* i1mach.f -- translated by f2c (version 20090411).
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

static integer c__32767 = 32767;
static integer c_b8 = 16777215;
static integer c__16405 = 16405;
static integer c_b12 = 9876536;
static integer c__0 = 0;
static integer c_b18 = 4194303;
static integer c__9 = 9;
static integer c__1 = 1;
static integer c__3 = 3;

integer i1mach_(integer *i__)
{
    /* Initialized data */

    static integer t3e[3] = { 9777664,5323660,46980 };
    static integer sc = 0;

    /* Format strings */
    static char fmt_9010[] = "(/\002 Adjust autodoubled I1MACH by uncommenti"
	    "ng data\002/\002 statements appropriate for your machine and set"
	    "ting\002/\002 IMACH(I) = IMACH(I+3) for I = 11, 12, and 13.\002)";
    static char fmt_9020[] = "(/\002 Adjust I1MACH by uncommenting data stat"
	    "ements\002/\002 appropriate for your machine.\002)";

    /* System generated locals */
    integer ret_val;
    static integer equiv_0[16];
    static real equiv_1[2];

    /* Builtin functions */
    integer s_wsfe(cilist *), e_wsfe(void);
    /* Subroutine */ int s_stop(char *, ftnlen);
    integer s_wsle(cilist *), do_lio(integer *, integer *, char *, ftnlen), 
	    e_wsle(void);

    /* Local variables */
    static integer j, k, i3;
#define imach (equiv_0)
#define rmach (equiv_1)
    extern /* Subroutine */ int i1mcr1_(integer *, integer *, integer *, 
	    integer *, integer *);
#define small ((integer *)equiv_1)
#define output (equiv_0 + 3)

    /* Fortran I/O blocks */
    static cilist io___7 = { 0, 6, 0, fmt_9010, 0 };
    static cilist io___11 = { 0, 6, 0, fmt_9020, 0 };
    static cilist io___12 = { 0, 6, 0, 0, 0 };



/*    I1MACH( 1) = THE STANDARD INPUT UNIT. */
/*    I1MACH( 2) = THE STANDARD OUTPUT UNIT. */
/*    I1MACH( 3) = THE STANDARD PUNCH UNIT. */
/*    I1MACH( 4) = THE STANDARD ERROR MESSAGE UNIT. */
/*    I1MACH( 5) = THE NUMBER OF BITS PER INTEGER STORAGE UNIT. */
/*    I1MACH( 6) = THE NUMBER OF CHARACTERS PER CHARACTER STORAGE UNIT. */
/*    INTEGERS HAVE FORM SIGN ( X(S-1)*A**(S-1) + ... + X(1)*A + X(0) ) */
/*    I1MACH( 7) = A, THE BASE. */
/*    I1MACH( 8) = S, THE NUMBER OF BASE-A DIGITS. */
/*    I1MACH( 9) = A**S - 1, THE LARGEST MAGNITUDE. */
/*    FLOATS HAVE FORM  SIGN (B**E)*( (X(1)/B) + ... + (X(T)/B**T) ) */
/*               WHERE  EMIN .LE. E .LE. EMAX. */
/*    I1MACH(10) = B, THE BASE. */
/*  SINGLE-PRECISION */
/*    I1MACH(11) = T, THE NUMBER OF BASE-B DIGITS. */
/*    I1MACH(12) = EMIN, THE SMALLEST EXPONENT E. */
/*    I1MACH(13) = EMAX, THE LARGEST EXPONENT E. */
/*  DOUBLE-PRECISION */
/*    I1MACH(14) = T, THE NUMBER OF BASE-B DIGITS. */
/*    I1MACH(15) = EMIN, THE SMALLEST EXPONENT E. */
/*    I1MACH(16) = EMAX, THE LARGEST EXPONENT E. */

/*  THIS VERSION ADAPTS AUTOMATICALLY TO MOST CURRENT MACHINES, */
/*  INCLUDING AUTO-DOUBLE COMPILERS. */
/*  TO COMPILE ON OLDER MACHINES, ADD A C IN COLUMN 1 */
/*  ON THE NEXT LINE */
/*  AND REMOVE THE C FROM COLUMN 1 IN ONE OF THE SECTIONS BELOW. */
/*  CONSTANTS FOR EVEN OLDER MACHINES CAN BE OBTAINED BY */
/*          mail netlib@research.bell-labs.com */
/*          send old1mach from blas */
/*  PLEASE SEND CORRECTIONS TO dmg OR ehg@bell-labs.com. */

/*     MACHINE CONSTANTS FOR THE HONEYWELL DPS 8/70 SERIES. */

/*      DATA IMACH( 1) /    5 / */
/*      DATA IMACH( 2) /    6 / */
/*      DATA IMACH( 3) /   43 / */
/*      DATA IMACH( 4) /    6 / */
/*      DATA IMACH( 5) /   36 / */
/*      DATA IMACH( 6) /    4 / */
/*      DATA IMACH( 7) /    2 / */
/*      DATA IMACH( 8) /   35 / */
/*      DATA IMACH( 9) / O377777777777 / */
/*      DATA IMACH(10) /    2 / */
/*      DATA IMACH(11) /   27 / */
/*      DATA IMACH(12) / -127 / */
/*      DATA IMACH(13) /  127 / */
/*      DATA IMACH(14) /   63 / */
/*      DATA IMACH(15) / -127 / */
/*      DATA IMACH(16) /  127 /, SC/987/ */

/*     MACHINE CONSTANTS FOR PDP-11 FORTRANS SUPPORTING */
/*     32-BIT INTEGER ARITHMETIC. */

/*      DATA IMACH( 1) /    5 / */
/*      DATA IMACH( 2) /    6 / */
/*      DATA IMACH( 3) /    7 / */
/*      DATA IMACH( 4) /    6 / */
/*      DATA IMACH( 5) /   32 / */
/*      DATA IMACH( 6) /    4 / */
/*      DATA IMACH( 7) /    2 / */
/*      DATA IMACH( 8) /   31 / */
/*      DATA IMACH( 9) / 2147483647 / */
/*      DATA IMACH(10) /    2 / */
/*      DATA IMACH(11) /   24 / */
/*      DATA IMACH(12) / -127 / */
/*      DATA IMACH(13) /  127 / */
/*      DATA IMACH(14) /   56 / */
/*      DATA IMACH(15) / -127 / */
/*      DATA IMACH(16) /  127 /, SC/987/ */

/*     MACHINE CONSTANTS FOR THE UNIVAC 1100 SERIES. */

/*     NOTE THAT THE PUNCH UNIT, I1MACH(3), HAS BEEN SET TO 7 */
/*     WHICH IS APPROPRIATE FOR THE UNIVAC-FOR SYSTEM. */
/*     IF YOU HAVE THE UNIVAC-FTN SYSTEM, SET IT TO 1. */

/*      DATA IMACH( 1) /    5 / */
/*      DATA IMACH( 2) /    6 / */
/*      DATA IMACH( 3) /    7 / */
/*      DATA IMACH( 4) /    6 / */
/*      DATA IMACH( 5) /   36 / */
/*      DATA IMACH( 6) /    6 / */
/*      DATA IMACH( 7) /    2 / */
/*      DATA IMACH( 8) /   35 / */
/*      DATA IMACH( 9) / O377777777777 / */
/*      DATA IMACH(10) /    2 / */
/*      DATA IMACH(11) /   27 / */
/*      DATA IMACH(12) / -128 / */
/*      DATA IMACH(13) /  127 / */
/*      DATA IMACH(14) /   60 / */
/*      DATA IMACH(15) /-1024 / */
/*      DATA IMACH(16) / 1023 /, SC/987/ */

    if (sc != 987) {
/*        *** CHECK FOR AUTODOUBLE *** */
	small[1] = 0;
	*rmach = 1e13f;
	if (small[1] != 0) {
/*           *** AUTODOUBLED *** */
	    if (small[0] == 1117925532 && small[1] == -448790528 || small[1] 
		    == 1117925532 && small[0] == -448790528) {
/*               *** IEEE *** */
		imach[9] = 2;
		imach[13] = 53;
		imach[14] = -1021;
		imach[15] = 1024;
	    } else if (small[0] == -2065213935 && small[1] == 10752) {
/*               *** VAX WITH D_FLOATING *** */
		imach[9] = 2;
		imach[13] = 56;
		imach[14] = -127;
		imach[15] = 127;
	    } else if (small[0] == 1267827943 && small[1] == 704643072) {
/*               *** IBM MAINFRAME *** */
		imach[9] = 16;
		imach[13] = 14;
		imach[14] = -64;
		imach[15] = 63;
	    } else {
		s_wsfe(&io___7);
		e_wsfe();
		s_stop("777", (ftnlen)3);
	    }
	    imach[10] = imach[13];
	    imach[11] = imach[14];
	    imach[12] = imach[15];
	} else {
	    *rmach = 1234567.f;
	    if (small[0] == 1234613304) {
/*               *** IEEE *** */
		imach[9] = 2;
		imach[10] = 24;
		imach[11] = -125;
		imach[12] = 128;
		imach[13] = 53;
		imach[14] = -1021;
		imach[15] = 1024;
		sc = 987;
	    } else if (small[0] == -1271379306) {
/*               *** VAX *** */
		imach[9] = 2;
		imach[10] = 24;
		imach[11] = -127;
		imach[12] = 127;
		imach[13] = 56;
		imach[14] = -127;
		imach[15] = 127;
		sc = 987;
	    } else if (small[0] == 1175639687) {
/*               *** IBM MAINFRAME *** */
		imach[9] = 16;
		imach[10] = 6;
		imach[11] = -64;
		imach[12] = 63;
		imach[13] = 14;
		imach[14] = -64;
		imach[15] = 63;
		sc = 987;
	    } else if (small[0] == 1251390520) {
/*              *** CONVEX C-1 *** */
		imach[9] = 2;
		imach[10] = 24;
		imach[11] = -128;
		imach[12] = 127;
		imach[13] = 53;
		imach[14] = -1024;
		imach[15] = 1023;
	    } else {
		for (i3 = 1; i3 <= 3; ++i3) {
		    j = small[0] / 10000000;
		    k = small[0] - j * 10000000;
		    if (k != t3e[i3 - 1]) {
			goto L20;
		    }
		    small[0] = j;
/* L10: */
		}
/*              *** CRAY T3E *** */
		imach[0] = 5;
		imach[1] = 6;
		imach[2] = 0;
		imach[3] = 0;
		imach[4] = 64;
		imach[5] = 8;
		imach[6] = 2;
		imach[7] = 63;
		i1mcr1_(&imach[8], &k, &c__32767, &c_b8, &c_b8);
		imach[9] = 2;
		imach[10] = 53;
		imach[11] = -1021;
		imach[12] = 1024;
		imach[13] = 53;
		imach[14] = -1021;
		imach[15] = 1024;
		goto L35;
L20:
		i1mcr1_(&j, &k, &c__16405, &c_b12, &c__0);
		if (small[0] != j) {
		    s_wsfe(&io___11);
		    e_wsfe();
		    s_stop("777", (ftnlen)3);
		}
/*              *** CRAY 1, XMP, 2, AND 3 *** */
		imach[0] = 5;
		imach[1] = 6;
		imach[2] = 102;
		imach[3] = 6;
		imach[4] = 46;
		imach[5] = 8;
		imach[6] = 2;
		imach[7] = 45;
		i1mcr1_(&imach[8], &k, &c__0, &c_b18, &c_b8);
		imach[9] = 2;
		imach[10] = 47;
		imach[11] = -8188;
		imach[12] = 8189;
		imach[13] = 94;
		imach[14] = -8141;
		imach[15] = 8189;
		goto L35;
	    }
	}
	imach[0] = 5;
	imach[1] = 6;
	imach[2] = 7;
	imach[3] = 6;
	imach[4] = 32;
	imach[5] = 4;
	imach[6] = 2;
	imach[7] = 31;
	imach[8] = 2147483647;
L35:
	sc = 987;
    }
    if (*i__ < 1 || *i__ > 16) {
	goto L40;
    }
    ret_val = imach[*i__ - 1];
    return ret_val;
L40:
    s_wsle(&io___12);
    do_lio(&c__9, &c__1, "I1MACH(I): I =", (ftnlen)14);
    do_lio(&c__3, &c__1, (char *)&(*i__), (ftnlen)sizeof(integer));
    do_lio(&c__9, &c__1, " is out of bounds.", (ftnlen)18);
    e_wsle();
    s_stop("", (ftnlen)0);
/* /+ C source for I1MACH -- remove the * in column 1 +/ */
/* /+ Note that some values may need changing. +/ */
/* #include <stdio.h> */
/* #include <float.h> */
/* #include <limits.h> */
/* #include <math.h> */

/* long i1mach_(long *i) */
/* { */
/* 	switch(*i){ */
/* 	  case 1:  return 5;	/+ standard input +/ */
/* 	  case 2:  return 6;	/+ standard output +/ */
/* 	  case 3:  return 7;	/+ standard punch +/ */
/* 	  case 4:  return 0;	/+ standard error +/ */
/* 	  case 5:  return 32;	/+ bits per integer +/ */
/* 	  case 6:  return sizeof(int); */
/* 	  case 7:  return 2;	/+ base for integers +/ */
/* 	  case 8:  return 31;	/+ digits of integer base +/ */
/* 	  case 9:  return LONG_MAX; */
/* 	  case 10: return FLT_RADIX; */
/* 	  case 11: return FLT_MANT_DIG; */
/* 	  case 12: return FLT_MIN_EXP; */
/* 	  case 13: return FLT_MAX_EXP; */
/* 	  case 14: return DBL_MANT_DIG; */
/* 	  case 15: return DBL_MIN_EXP; */
/* 	  case 16: return DBL_MAX_EXP; */
/* 	  } */
/* 	fprintf(stderr, "invalid argument: i1mach(%ld)\n", *i); */
/* 	exit(1);return 0; /+ some compilers demand return values +/ */
/* } */
    return ret_val;
} /* i1mach_ */

#undef output
#undef small
#undef rmach
#undef imach


/* Subroutine */ int i1mcr1_(integer *a, integer *a1, integer *b, integer *
	c__, integer *d__)
{
/* *** SPECIAL COMPUTATION FOR OLD CRAY MACHINES **** */
    *a1 = (*b << 24) + *c__;
    *a = (*a1 << 24) + *d__;
    return 0;
} /* i1mcr1_ */

