/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*---------------------------------------------------------------
|  showfrequency()
|
|   Purpose:
|  	This module displays the settings for the frequency offset
|	synthesizer, PTS, and observe frequency
|
|				Author Greg Brissey  4/28/86
+---------------------------------------------------------------*/
#define CALC 1
#define SHOW 0
extern int   Tflag;
showfrequency(typefrq,typebase,h1freq,type,rfband,basefreq,freq,offset,nucname)
char 	rfband;		/* rfbane high or low band */
char 	*nucname;	/* Nucleus name  (C13,H1,etc.) */
char 	 type;		/* RFtype - a,b,c,d,e */
char 	*typefrq;	/* char string ("spectrometer","decoupler") */
char 	*typebase;	/* char string ("tbo","dbo") */
double	 basefreq;	/* base offset frequency (tbo,dbo) */
double	 freq;		/* spectrometer frequency (sfrq,dfrq) */
double	 h1freq;	/* spectrometer proton frequency */
double   offset;	/* offset frequency (to,do) */
{
    extern double calcFrequency();
    double baseMHz;
    baseMHz = basefreq * 1.0e-6; 	/* convert to MHz */
    offset *= 1.0e-6;		/* convert to MHz */
    if (Tflag) 
	Wscrprintf(
	"showfrequency(): %11.7lf MHz = %11.7lf(frq) + %11.7lf(offset)\n",
	(freq+offset),freq,offset);
    Wscrprintf("%s Frequency = %11.7lf MHz  Nucleus: %s \n", 
		   typefrq,(freq + offset),nucname);
    if (type != 'c') 
	Wscrprintf("%s = %11.7lf MHz",typebase,baseMHz);
    else
	Wscrprintf("%s = Unused",typebase);
    Wscrprintf("   Synthesizer Frequency = %11.7lf MHz\n",
    		calcFrequency(h1freq,type,rfband,&basefreq,freq,&offset,SHOW));
}

