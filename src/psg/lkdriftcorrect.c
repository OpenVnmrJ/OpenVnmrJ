/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "variables.h"
#include "group.h"
extern int Tflag;
/* ---------------------------------------------------------------
|  lkdriftcorrect(h1freq,freq,lockfreq);
|
|  Purpose:
|	This module is to calculate the new specrometer frequency
|	corrected for any lock frequency drift.
|       Returns the corrected frequency (double)
|
|	The corrected frequency is the frequency multiplied
|	by the ratio of the drifted D2O frequency over the 
|	nominal D2O frequency. The nominal D2O frequency
|	is dependent on the instrument type (i.e.,200,300,etc.).
|	The D2O drift frequency is the difference of the normal
|	lock receiver frequency (standard thumb wheel setting) and
|	the present lock receiver frequency (thumb wheel setting 'lockfreq').
|
|      Note: the 300 uses the difference in freq mixing for lock receiver
|	     rather than the sum, thus the negation for the 300 case.
|
|				Author Greg Brissey  5/13/86
+------------------------------------------------------------------*/
double lkdriftcorrect(h1freq,freq,lockfreq)
double freq;
double	lockfreq;	/* lock frequency */
double	h1freq;		/* spectrometer frequency */
{
    double	crtfreq;		/* corected lock frequency */
    double	lkfreq;			/* local lock frequency */
    double	d2ofreq;		/* D2O frequency */
    double	d2odrift;		/* drifted D2O frequency */
    double	nomlksetting;		/* nominal lkfreq thumbwheel setting */

    if (Tflag)
	Wscrprintf(
	"lkdriftcorrect(): Frequency: %11.7lf ,h1freq = %4.0lf, lockfreq = %8.4lf \n",
		    freq,h1freq,lockfreq);

    lkfreq = lockfreq;	
    switch( (int) h1freq )	/* select proper d20 freq for instrument */
    {
	case 200:
		nomlksetting = 1.21;
		d2ofreq = 30.71;
		break;
	case 300:
		nomlksetting = -1.206;
		d2ofreq = 46.044;
		lkfreq = -lkfreq;
		break;
	case 400:
		nomlksetting = 1.145;
		d2ofreq = 61.395;
		break;
	case 500:
		nomlksetting = 1.479;
		d2ofreq = 76.729;
		break;
    }

    d2odrift = (d2ofreq - nomlksetting) + lkfreq;

    crtfreq = freq * (d2odrift / d2ofreq);
    if (Tflag)
    {
	Wscrprintf(
	"lkdriftcorrect(): %11.7lf drift = %11.7lf freq - (%11.7lf lkset + %11.7lf lkfreq )\n",
		d2odrift,d2ofreq,nomlksetting,lkfreq);
	Wscrprintf("lkdriftcorrect(): compensated freq %11.7lf  = %11.7lf freq * %11.7lf drift / %11.7lf freq\n",
		crtfreq,freq,d2odrift,d2ofreq);
    }
    return( crtfreq );
}

