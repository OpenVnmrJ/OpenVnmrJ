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

/*
 *	GDACtest.c: pulse sequence to test gradient DACs.
 *
 *	Parameters used
 *	---------------
 *	dacvalue: The value(s) to which gradients are set.  All three
 *		gradients use the same "dacvalue".  May be arrayed.
 *		Example: dacvalue=1,2,4,8,16,32,64
 *
 *	axes: The order in which gradient axes are tested.  Axes='xyz'
 *		sets the gradients in the order:
 *			x=dacvalue(1)
 *			y=dacvalue(1)
 *			z=dacvalue(1)
 *			x=dacvalue(2)
 *			y=dacvalue(2)
 *			...
 *			z=dacvalue(n)
 *		Use an 'n' or 'N' if no tests are desired on some axis,
 *		e.g., axes='xnz' or 'xzn' will omit tests on axis "y".
 *		Only one gradient is turned on at a time.
 *
 *	ext_trig: If set to 'y' or 'Y', the duration of each gradient
 *		pulse is controlled by the external gating input; otherwise,
 *		the durations are controlled by "d1" and "d2".
 *		The external trigger is a TTL level signal that triggers on
 *		the rising edge.  Connect to "External Trigger" (J8120).
 *		The operation sequence is:
 *			trigger 1:  gradient 1 on	[e.g., x=dacvalue(1)]
 *			trigger 2:  gradient 1 off
 *			trigger 3:  gradient 2 on	[e.g., y=dacvalue(1)]
 *			trigger 4:  gradient 2 off
 *			...
 *			trigger 2n: gradient n off
 *
 *	d1: The default duration of each pulse (in case ext_trig is
 *		not used).
 *
 *	d2: The default time between gradient pulses (in case ext_trig
 *		is not used).
 *
*/

/*
 *	Read in and validate parameters from experiment
 *	For each axis, if axis is used:
 *	    Do pulse sequence for current gradient value:
 *		Wait for trigger or delay d2
 *		Turn on gradient
 *		Wait for trigger or delay d1
 *		Turn off gradient
 *
*/

#include "standard.h"

#ifndef FALSE
#define FALSE (0)
#define TRUE (!FALSE)
#endif

pulsesequence(){


    char axis[3];			/* The first 3 chars of "axes" */
    int iaxis;				/* The running axis number */
    double dacvalue;			/* Desired gradient setting */
    char parmbuf[MAXSTR];		/* Used to examine string parms */
    int extTrig = FALSE;		/* TRUE means use external trigger */


    /* Read in and validate parameters from experiment */

    dacvalue = getval("dacvalue");	/* Accept ANY value */

    getstr("axes", parmbuf);
    if (*parmbuf == '\0') {
        getstr("orient", parmbuf); /* Allow orient for legacy people or code */
        if (*parmbuf == '\0') {
            abort_message("SEQUENCE ERROR: \"axes\" null or not present");
        } else if (getorientation(axis, axis+1, axis+2, "orient") < 0 ) {
            abort_message("SEQUENCE ERROR: \"orient\" set wrong");
        }
    } else if ( getorientation(axis, axis+1, axis+2, "axes") < 0 ) {
        abort_message("SEQUENCE ERROR: \"axes\" set wrong");
    }

    getstr("ext_trig", parmbuf);
    if ( (*parmbuf == 'y') || (*parmbuf == 'Y') )  /* ANYTHING else is "no" */
	extTrig = TRUE;


    /* Do pulse sequence for current gradient value */

    for (iaxis=0; iaxis<3; iaxis++){
	if ( (axis[iaxis] != 'n') && (axis[iaxis] != 'N') ){
	    if (extTrig){
		xgate(1.0);		/* Wait for one clock tick... */
	    }else{
		delay(d2);		/*  or default delay time */
	    }
	    gradient(axis[iaxis], (int)dacvalue);	/* GRADIENT ON */
	    if (extTrig)
		xgate(1.0);
	    else
		delay(d1);		/* Default "on" time */
	    gradient(axis[iaxis], 0);	/* GRADIENT OFF */
	}
    }
}

