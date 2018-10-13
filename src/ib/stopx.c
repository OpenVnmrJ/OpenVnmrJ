/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* stopx.f -- translated by f2c (version 20090411).
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

logical stopx_(integer *idummy)
{
    /* System generated locals */
    logical ret_val;

/*     *****PARAMETERS... */

/*     .................................................................. */

/*     *****PURPOSE... */
/*     THIS FUNCTION MAY SERVE AS THE STOPX (ASYNCHRONOUS INTERRUPTION) */
/*     FUNCTION FOR THE NL2SOL (NONLINEAR LEAST-SQUARES) PACKAGE AT */
/*     THOSE INSTALLATIONS WHICH DO NOT WISH TO IMPLEMENT A */
/*     DYNAMIC STOPX. */

/*     *****ALGORITHM NOTES... */
/*     AT INSTALLATIONS WHERE THE NL2SOL SYSTEM IS USED */
/*     INTERACTIVELY, THIS DUMMY STOPX SHOULD BE REPLACED BY A */
/*     FUNCTION THAT RETURNS .TRUE. IF AND ONLY IF THE INTERRUPT */
/*     (BREAK) KEY HAS BEEN PRESSED SINCE THE LAST CALL ON STOPX. */

/*     .................................................................. */

    ret_val = FALSE_;
    return ret_val;
} /* stopx_ */

