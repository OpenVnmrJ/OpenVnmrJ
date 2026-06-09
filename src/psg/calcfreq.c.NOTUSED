/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* ---------------------------------------------------------------
|  calcfrequency();/7
|
|  Purpose:
| 	This module calc the proper setting for the frequency offset
|	synthesizer and PTS for the request observe frequency
|	or just calc pts setting for display purposes.
|	returns the calc pts value (double)
|
|				Author Greg Brissey  4/28/86
+------------------------------------------------------------------*/
#define CALC 1
#define SHOW 0
extern int   Tflag;

double calcFrequency(h1freq,type,rfband,basefreq,freq,offset,cflag)
char     rfband;	/* rfband - high or low band */
char     type;		/* RFtype - a,b,c,d,e */
double  *basefreq;  	/* base offset frequency (tbo,dbo) */
double   freq;		/* spectrometer frequency (sfrq,dfrq) */
double   h1freq;	/* spectrometer proton frequency (sfrq,dfrq) */
double  *offset;	/* offset frequency (to,do) */
int      cflag;		/* calc new basefreq or just calc pts syn flag */
{   double iffreq;
    double pts;
    double baseMHz;

    baseMHz = *basefreq * 1.0e-6;	/* convert to MHz */
    if (Tflag)
	Wscrprintf("calcFreqency(): RF-type: %c\n",type);
    if (type != 'c')		/* normal broadband system */	
    {	if (h1freq > 390.0)   /* instrument proton frequency */
	   iffreq = 20.55;
	else
	   iffreq = 15.85;
	if (cflag) 
	    baseMHz = 1.5; /* MHz */
	pts = freq  - (iffreq + baseMHz);      /* obtain pts value */
        if (Tflag)
	   Wscrprintf(
	   "calcFreqency(): %11.7lf(pts) = %11.7lf(frq) - (%11.7lf(base) + %11.7lf(iffrq))\n",
			pts,freq,baseMHz,iffreq);
	if (pts < 0.0) 
	    pts = -pts;		    /* absolute value */
	if (cflag)
	{   pts = ((double) ( ((long) (pts * 1.0e6)) / 100) ) * 1.0e-4;
            if (Tflag)
		Wscrprintf("calcFreqency(): %11.7lf(pts) rounded to 100Hz\n",
				pts);
	    baseMHz = freq - (pts + iffreq);     /* back calc base freq */
            if (Tflag)
	        Wscrprintf(
		  "calcFreqency(): %11.7lf(base) = %11.7lf(frq) - %11.7lf(pts) - %11.4lf(iffrq)\n",
			baseMHz,freq,pts,iffreq);
	    *basefreq = baseMHz * 1.0e6;		/* convert to Hz */
	}
    }
    else		/* New RF scheme */
    {
	if (cflag)
	    *basefreq = 0;
	if ( (rfband == 'h') || (rfband == 'H') ||  /* rfband High or Low ? */
	     (freq > (h1freq * 0.75)) )		    /* or freq > 75% h1freq */
        {
	    pts = (freq + 10.5) / 2.0;  /* rfband = b if high band */
            if (Tflag)
		Wscrprintf(
		"calcFreqency(): %11.7lf(pts) = %11.7lf(freq) + 10.5 / 2.0\n",
			pts,freq);
        }
	else
	{   pts = freq + 10.5;
            if (Tflag)
		Wscrprintf(
		"calcFreqency(): %11.7lf(pts) = %11.7lf(freq) + 10.5\n",
				pts,freq);
	}
    }
    if (cflag)
	*offset = 0.0;
    return(pts);
}

