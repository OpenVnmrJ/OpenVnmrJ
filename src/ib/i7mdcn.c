/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* i7mdcn.f -- translated by f2c (version 20090411).
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

integer i7mdcn_(integer *k)
{
    /* Initialized data */

    static integer mdperm[3] = { 2,4,1 };

    /* System generated locals */
    integer ret_val;

    /* Local variables */
    extern integer i1mach_(integer *);



/*  ***  RETURN INTEGER MACHINE-DEPENDENT CONSTANTS  *** */

/*     ***  K = 1 MEANS RETURN STANDARD OUTPUT UNIT NUMBER.   *** */
/*     ***  K = 2 MEANS RETURN ALTERNATE OUTPUT UNIT NUMBER.  *** */
/*     ***  K = 3 MEANS RETURN  INPUT UNIT NUMBER.            *** */
/*          (NOTE -- K = 2, 3 ARE USED ONLY BY TEST PROGRAMS.) */

/*  +++  PORT VERSION FOLLOWS... */
    ret_val = i1mach_(&mdperm[(0 + (0 + (*k - 1 << 2))) / 4]);
/*  +++  END OF PORT VERSION  +++ */

/*  +++  NON-PORT VERSION FOLLOWS... */
/*     INTEGER MDCON(3) */
/*     DATA MDCON(1)/6/, MDCON(2)/8/, MDCON(3)/5/ */
/*     I7MDCN = MDCON(K) */
/*  +++  END OF NON-PORT VERSION  +++ */

/* L999: */
    return ret_val;
/*  ***  LAST CARD OF I7MDCN FOLLOWS  *** */
} /* i7mdcn_ */

