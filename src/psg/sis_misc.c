/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include "acodes.h"
#include "acqparms.h"
#include "rfconst.h"
#include "macros.h"


#define MINHKDELAY	3.0e-3		/* 3 millisecs		*/


extern int curfifocount;
extern int HSlines;


/*-------------------------------------------------------------------------
 * sethkdelay - Sets Housekeeping Delay to delaytim. 
 * Note that only one timerword is used. 
 *	Written by:		M. Howitt	890515
 *	Last Revised:		M. Howitt 
 *------------------------------------------------------------------------*/

S_sethkdelay(delaytim)
double delaytim;
{
int tword1,tword2;
   	notinhwloop("sethkdelay");
	if (delaytim < MINHKDELAY)
	{
		text_error("hkdelay must be >= 3 milliseconds\n");
		psg_abort(1);
	}
	else
	{
		timerwords(delaytim,&tword1,&tword2); /*calc tw array */
		if (tword1 != 0)  {
			/* only if wincr > 0.0 ! */
			putcode(SETHKDELAY);  		/*an event n*/
			putcode((codeint)tword1);	/*timeword 1*/
		}
		else
		{
			text_error("hkdelay timerword error\n");
			psg_abort(1);
		}
	}
}


#define MINIMPLCLP 2	/* Min size for acq h/w loops (Must be even) */


/*-------------------------------------------------------------------------
 * setloopsize - Sets implicit hardware loop used by acqop() to loopsize. 
 * FOR XRPULSER BOARDS ONLY!
 *
 *	Written by:		R. Schmidt	911014
 *	Last Revised:		R. Schmidt	911022
 *------------------------------------------------------------------------*/

S_setloopsize(loopsize)
double loopsize;
{
	/* abort if not using an XRpulser (Pulse control, acquisition
	control) board */
	if (fifolpsize < 64) {
	    text_error(
	       "Warning: setloopsize() does not work with Output Boards\n");
	    return;
	}

   	notinhwloop("setloopsize");

	/* Set the acq implicit h/w loop size, silently force it
	to be an even number, and clip it against upper and lower
	bounds if necessary.  Allowing an odd number as the loop
	size always results in an "number of points acquired != np"
	error; allowing a looping size > the actual loop FIFO size
	always results in a "not enough time between hardware loops"
	error.  Any even number =< the actual loop FIFO size works
	whether or not it is an integral divisor of the np value.
	The only latent problem here is that the acquisition computer
	sizes the loop FIFO and thus has a reliable notion of it, whereas
	PSG relies upon its entry via the Vnmr "config" utility.  The
	value checking/clipping was moved here from apint.c to minimize
	its execution time in the VM02 for fast imaging purposes.
	Russ  911022 */

	putcode(SETLOOPSIZE);
	if (loopsize > (double)fifolpsize)
		putcode((codeint)(fifolpsize & ~1));
	else if (loopsize < (double)MINIMPLCLP)
		putcode((codeint)MINIMPLCLP);
	else
		putcode((codeint)((int)loopsize & ~1));
}
/*------------------------------------------------------------------
| startfifo()
|       User controllable startfifo element for sequence continuity
|       with large preloop fifos.
|                               
+-----------------------------------------------------------------*/
startfifo()
{
        putcode((codeint)JUSTSTFIFO);
}
