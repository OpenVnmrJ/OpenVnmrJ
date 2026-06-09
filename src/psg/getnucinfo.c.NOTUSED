/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#include <stdio.h>
#include <strings.h>
#include "vnmrsys.h"

extern int  Tflag;

/*-----------------------------------------------------------------
|
|  getnucinfo()/8
|
|  Purpose:
|	This module opens and searches through the proper nucleus
|	table to find the named nucleus. It passed back all relevent
|	information contain in the nucleus table.
|
| -- NUCLEUS TABLE FORMAT --
| Name	Frequency	Baseoffset   FreqBand	Syn	RefFreq
|
|				Author Greg Brissey  5/12/86
+------------------------------------------------------------------*/

getnucinfo(h1freq,rftype,nucname,freq,basefreq,freqband,syn,reffreq)
char	*freqband;	/* high or low band */
char	*nucname;	/* nucleus name */
char     rftype;	/* type of RF a,b,c,d */
char	*syn;		/* presence of PTS synthesizer: yes,reverse,no */
double  *basefreq;   	/* base offset frequency (tbo,dbo) */
double  *freq;		/* spectrometer frequency (sfrq,dfrq) */
double   h1freq;	/* spectrometer proton frequency */
double  *reffreq;	/* reference frequency */
{
    char   nuc[10];		/* name of nucleus */
    char   nuctable[15];	/* name of nucleus file to search */
    char   nucpath[MAXPATHL];	/* path to nucleus table */
    FILE   *stream;
    int     ret;

    if (Tflag)
	Wscrprintf("getnucinfo(): Open nucleus table 'nuctab%d%c' \n",
		   ( ((int) h1freq)/100),rftype);

    /* ---  construct file name from h1freq and rftype --- */
    sprintf(nuctable,"nuctab%d%c",( ((int) h1freq)/100),rftype);

    strcpy(nucpath,systemdir);		/* construct absolute path */
    strcat(nucpath,"/nuctables/");
    strcat(nucpath,nuctable);

    if ((stream = fopen(nucpath,"r")) == 0)
    {   Werrprintf("Cannot open the '%s' nucleus table file. \n",nucpath);
        return(1);
    }

    /* --- read in file and search for the requested nucleus --- */
    fscanf(stream,"%*s%*s%*s%*s%*s%*s");  /* skip header information */
    while (ret != EOF)
    {
        ret = fscanf(stream,"%s%lf%lf%s%s%lf",
		   nuc,freq,basefreq,freqband,syn,reffreq);
        if (ret == 0)
 	{   Werrprintf("Nucleus table's format (%s) has been corrupted.",
			nuctable);
	    return(1);
	}
	if (Tflag > 1)
 	    Wscrprintf(
	     "getnucinfo(): %s - %s,%11.7lf, %11.7lf, %s, %s, %11.7lf\n",
		   nucname,nuc,*freq,*basefreq,freqband,syn,*reffreq);
	if ( strcmp(nucname,nuc) == NULL) 
	{
	    if (Tflag)
		printf("getnucinfo(): Found requested Nucleus: %s\n",nuc);
	    return(0);
	}
    }
    Werrprintf("Requested nucleus, '%s', is not an entry in the nucleus table %s",
		nucname,nuctable);
    return(1);
}
