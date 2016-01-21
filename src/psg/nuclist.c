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
#include "variables.h"
#include "group.h"

/*-------------------------------------------------------------------
|    nuclist -  Usage: nuclist(['no'],['solvent_name'])
| 
|   Purpose:
|  	This module lists the nucleus information from the nucleus table. 
|	Frequencies are also corrected for lock freq. drift (lockfreq) 
|	if requested, and for solvent if requested and a valid solvent.
| 
|   Routines:
|	calcFrequency - returns calc PTS synthesizer frequency,
|			w/ cflag set then Base Frequency Offset
|				synthesizer frequency is calc and altered.
|			        frequency Offset is set to Zero.
|	showfrequency - displays the spectrometer frequency, Base Offset
|			frequency, and PTS synthesizer freqency.
|	getparm - retrieves the values of REAL or STRING parameters.
|	setparm - stores the values of REAL or STRING parameters.
|	Author   Greg Brissey   5/19/86
/+-------------------------------------------------------------------*/
#define CALLNAME 0
#define MAXPARAMS 3
#define TRUE 1
#define FALSE 0
#define CALC 1
#define SHOW 0
extern double stringReal();	/* converts string to real */
extern int isReal();

extern char *newString();
extern char *realString();
extern int   Tflag;

/*~~~~~~~~~~~~~~~~~~~~~~  Main Program ~~~~~~~~~~~~~~~~~~~~~~*/
nuclist(argc,argv,retc,retv)
int argc,retc;  char *argv[],*retv[];
{
    char     nuctable[20];	/* nucleus table name */
    char     parm1[20];		/* 1st parameter if passed */
    char     rftype[4];		/* RF Type (a,b,c,d,e)  */
    char     solvent[20];	/* solvent name */
    char     solvpath[MAXPATHL];/* absolute path to solvent table */
    char     solvtable[20];	/* solvent table name */
    double   h1freq;		/* proton freq of instrument */
    double   lockfreq;		/* instrument lock frequency */
    double   solvshift;		/* solvent proton shift */
    int	     correct;		/* equal 1, correct for lkfreq & solvent*/
    int	     rfindex;		/* rftpe index */
     
    if (getparm("rftype","STRING",GLOBAL,&rftype[0],2))
	return(1);
    if (getparm("h1freq","REAL",GLOBAL,&h1freq,1))
	return(1);
    if (getparm("lockfreq","REAL",GLOBAL,&lockfreq,1))
	return(1);


    if (Tflag) Wscrprintf("rftype = %s, h1freq = %8.4lf, lockfreq = %11.7lf\n",
		rftype,h1freq,lockfreq);
    if ( (MAXPARAMS < argc)  )
    {
	fprintf(stdout,"Usage - %s(['no'],['solvent_name']) ",argv[0]);
	return(1);
    }
    /* --- nuclist  or dnuclist called ? --- */
    if (strcmp(argv[CALLNAME],"nuclist") == 0) 
    {
	rfindex = 0;
    }
    else
    {
	rfindex = 1;
    }
    if (Tflag)
	Wscrprintf(" rfindex = %d \n",rfindex);

    /* --- 1st parameter optional  maybe 'no' or solvent name --- */
    if (argc > 1)
    {
        if (!isReal(argv[1]))
	{
	    strncpy(parm1,argv[1],19);
	    if (Tflag)
		Wscrprintf("parameter 1: '%s' \n",argv[1]);
	}
        else
        {
	    Werrprintf("1st parameter '%s' is not a string. \n",argv[1]);
	    return(1);
        }
    }
    /* --- 2nd parameter (optional) is the Solvent Name to be Used --- */
    if (argc > 2) 
    {
    	if (!isReal(argv[2]))
	    strncpy(solvent,argv[2],19);
    	else
    	{
	    Werrprintf("2nd parameter '%s' is not a Solvent Name.\n",argv[2]);
	    return(1);
    	}
    }
    else
    {
        if (getparm("solvent","string",CURRENT,solvent,19))
	    return(1);
    }
    correct = TRUE;
    if (argc > 1)
    {
        if (strcmp(parm1,"no") != NULL) 
        {
	    strcpy(solvent,parm1);
        }
        else
        {
	    correct = FALSE;
	    solvshift = 0.0;
	    lockfreq = 0.0;
        }
    }

    if (Tflag)
	Wscrprintf("Solvent: '%s' \n",solvent);
    if ( (correct == TRUE)  || (argc > 2) )
    {
        strcpy(solvpath,systemdir);
        strcat(solvpath,"/solvents");
        if (getsolventinfo(solvent,solvpath,&solvshift))
        {
            return(1);
        }

        if (Tflag)
    	    Wscrprintf(
	    "%s has a proton shift of %8.5lf ppm.\n",
			    solvent,solvshift);
   }
   /* ---  construct file name from h1freq and rftype --- */
   sprintf(nuctable,"nuctab%d%c",( ((int) h1freq)/100),rftype[rfindex]);

   if (Tflag)
   Wscrprintf("h1freq=%4.1lf,rftype='%c','nuctable='%s',lkfrq=%6.4lf,sol=%6.4lf\n",
	 h1freq,rftype[rfindex],nuctable,lockfreq,solvshift);

   nuclst(h1freq,rftype[rfindex],nuctable,lockfreq,solvshift);
}

/*---------------------------------------------------------
|
|
+---------------------------------------------------------*/
nuclst(h1freq,rftype,nuctable,lockfreq,solvshift)
char	*nuctable;
char	 rftype;
double   h1freq;
double   lockfreq;
double	 solvshift;
{
    extern double lkdriftcorrect();
    extern double calcFrequency();
    char    bandn[20];
    char    basen[20];
    char    frqn[20];
    char    nuc[20];
    char    nucn[20];
    char    refn[20];
    char    rfband[20];
    char    syn[20];
    char    synn[20];
    double  base;
    double   FO;		/* freq offset */
    double  nucfreq;
    double  ref;
    double  specfreq;
    FILE   *stream;
    int     compensated;
    int     ret;

    if ((stream = fopen(nuctable,"r")) == 0)
    {   Werrprintf("Cannot open nucleus file: %s \n",nuctable);
        return(1);
    }
    ret=fscanf(stream,"%s%s%s%s%s%s",nucn,frqn,basen,bandn,synn,refn);
    Wscrprintf("%6s    %10s(MHz)  %12s(MHz)  %10s  %4s  %11s \n",
	nucn,frqn,basen,bandn,synn,refn);
    compensated = 0;
    while (ret != EOF)
    {
        ret=fscanf(stream,"%s%lf%lf%s%s%lf",nuc,&nucfreq,&base,rfband,syn,&ref);
        if (ret == 0)
	{
 	    Werrprintf("'%s' Nucleus table format corrupted. \n",nuctable);
	    return(1);
	}
    	/* --- correct for lockfreq if active ---*/
    	if ( lockfreq > 0.0 )
    	{
	    specfreq = lkdriftcorrect(h1freq,nucfreq,lockfreq);
	    compensated++;
	    if (Tflag)
	    Wscrprintf("nucfreq = %11.7lf MHz, corrected freq = %11.7lf MHz \n",
			nucfreq,specfreq);
            nucfreq = specfreq;		/* update nucfreq to corrected one */
    	}

    	/* --- correct for solvent ---*/
    	if ( solvshift > 0.0) 
    	{
	    solventcorrect(&specfreq,solvshift);
	    compensated++;
	    if (Tflag)
	      Wscrprintf("freq = %11.7lf MHz, corrected freq = %11.7lf MHz \n",
			nucfreq,specfreq);
            nucfreq = specfreq;		/* update nucfreq to corrected one */
    	}

    	/* --- if frequency corrected then recalculate offset freq. --- */
    	if (compensated)
    	{   
	    calcFrequency(h1freq,rftype,rfband[0],&base,nucfreq,&FO,CALC);
    	}
 	Wscrprintf("%5s       %11.7lf      %11.7lf     %9s  %6s   %11.7lf\n",
 		  nuc,nucfreq,base*1e-6,rfband,syn,ref);
    }
}
