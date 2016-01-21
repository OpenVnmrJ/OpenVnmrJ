/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* dr7mdc.f -- translated by f2c (version 20090411).
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

static integer c__2 = 2;
static integer c__1 = 1;
static integer c__4 = 4;

doublereal dr7mdc_(integer *k)
{
    /* Initialized data */

    static doublereal big = 0.;
    static doublereal eta = 0.;
    static doublereal machep = 0.;
    static doublereal zero = 0.;

    /* System generated locals */
    doublereal ret_val;

    /* Builtin functions */
    double sqrt(doublereal);

    /* Local variables */
    extern doublereal d1mach_(integer *);


/*  ***  RETURN MACHINE DEPENDENT CONSTANTS USED BY NL2SOL  *** */


/*  ***  THE CONSTANT RETURNED DEPENDS ON K... */

/*  ***        K = 1... SMALLEST POS. ETA SUCH THAT -ETA EXISTS. */
/*  ***        K = 2... SQUARE ROOT OF ETA. */
/*  ***        K = 3... UNIT ROUNDOFF = SMALLEST POS. NO. MACHEP SUCH */
/*  ***                 THAT 1 + MACHEP .GT. 1 .AND. 1 - MACHEP .LT. 1. */
/*  ***        K = 4... SQUARE ROOT OF MACHEP. */
/*  ***        K = 5... SQUARE ROOT OF BIG (SEE K = 6). */
/*  ***        K = 6... LARGEST MACHINE NO. BIG SUCH THAT -BIG EXISTS. */

/* /+ */
/* / */

    if (big > zero) {
	goto L1;
    }
    big = d1mach_(&c__2);
    eta = d1mach_(&c__1);
    machep = d1mach_(&c__4);
L1:

/* -------------------------------  BODY  -------------------------------- */

    switch (*k) {
	case 1:  goto L10;
	case 2:  goto L20;
	case 3:  goto L30;
	case 4:  goto L40;
	case 5:  goto L50;
	case 6:  goto L60;
    }

L10:
    ret_val = eta;
    goto L999;

L20:
    ret_val = sqrt(eta * 256.) / 16.;
    goto L999;

L30:
    ret_val = machep;
    goto L999;

L40:
    ret_val = sqrt(machep);
    goto L999;

L50:
    ret_val = sqrt(big / 256.) * 16.;
    goto L999;

L60:
    ret_val = big;

L999:
    return ret_val;
/*  ***  LAST CARD OF DR7MDC FOLLOWS  *** */
} /* dr7mdc_ */

