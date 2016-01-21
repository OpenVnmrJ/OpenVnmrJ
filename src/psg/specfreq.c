/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include "group.h"
#include "variables.h"
/*-------------------------------------------------------------------
|    SPECFREQ, DECFREQ:   Usage: specfreq(frequency MHz) or specfreq
|				 decfreq(frequency MHz) or decfreq
| 
|   Purpose:
|  	This module displays or sets the spectrometer frequency 
| 	(Transmitter or Decoupler)
| 
|   Parameters altered:
|	tn,tbo,tof,sfrq or dn,dbo,dof,dfrq
|
|   Routines:
| 	specfreq  -  specfreq main function.
|	calcFrequency - returns calc PTS synthesizer frequency,
|			w/ cflag set then Base Frequency Offset
|				synthesizer frequency is calc and altered.
|			        frequency Offset is set to Zero.
|	showfrequency - displays the spectrometer frequency, Base Offset
|			frequency, and PTS synthesizer freqency.
|	getparm - retrieves the values of REAL or STRING parameters.
|	setparm - stores the values of REAL or STRING parameters.
|	Author   Greg Brissey   4/28/86
/+-------------------------------------------------------------------*/

#define CALLNAME 0
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
specfreq(argc,argv,retc,retv)
int argc,retc;  char *argv[],*retv[];
{
    char	*argdgv[5];	/* arg variable for dg program */
    char	nucname[20];
    char     	rfband[4];		/* RF high or low band identifier */
    char     	rftype[4];		/* RF Type (a,b,c,d,e)  */
    extern double calcFrequency();
    double	DO;		/* decoupler offset frequency */
    double	dbo;		/* decoupler base offset frequency */
    double	dfrq;		/* decoupler frequency */
    double	h1freq;		/* proton freq of instrument */
    double	TO;		/* transmitter offset frequency */
    double	tbo;		/* transmitter base offset frequency */
    double	sfrq;		/* transmitter frequency */
    double	reqfrq;		/* requested frequency */
    int		argdgc;		/* arg count for dg */
    int calcsyn;

    if ( 2 < argc )
	fprintf(stdout,"Usage - spcfrq, decfrq, spcfrq(freq)");
    tbo = dbo = sfrq = dfrq = TO = DO = 0.0;	/* initialize to zero */
    if (getparm("rftype","STRING",GLOBAL,&rftype[0],4))
	return(0);
    if (getparm("rfband","STRING",CURRENT,&rfband[0],2))
	return(0);
    if (getparm("h1freq","REAL",GLOBAL,&h1freq,1))
	return(0);

    if (Tflag) 
	Wscrprintf("rftype = %s, rfband = %s, h1freq = %8.4lf\n",
		rftype,rfband,h1freq);

    /* --- calc freq or just display --- */
    if ( argc <= 1 )
	calcsyn = FALSE; /* a freq passed ? */
    else
    {	calcsyn = TRUE;
	if (Tflag)
	    Wscrprintf("Param String: %s \n",argv[1]);
	if (isReal(argv[1]))
	    reqfrq = stringReal(argv[1]);
	else 
	{   Werrprintf("Parameter is Not a Real Type.");
	    return(1);
	}
    }

    /*
    if (TRUE)
    {
	   char *argv[2];
	    int   argc;
	    argv[0] = "dg";
	    argc = 1;
	    ExecD(argc,argv,0,0);
    }
    */
    /* --- specfreq or decfreq called ? --- */
    if (strcmp(argv[CALLNAME],"specfreq") == 0) 
    {	if (getparm("tbo","REAL",CURRENT,&tbo,1))
	    return(1);
	if (calcsyn)
	{  
	    sfrq = reqfrq;
	    strcpy(nucname,"Undef.");
	    setparm("tn","STRING",CURRENT,nucname,1);
	    argdgv[0] = "dg";
	    argdgv[1] = "tn";
	    argdgc = 2;
	    ExecD(argdgc,argdgv,0,0);
	}
	else
	{   if (getparm("sfrq","REAL",CURRENT,&sfrq,1))
	        return(1);
        }
	if (getparm("tof","REAL",CURRENT,&TO,1))
	    return(1);
	if (getparm("tn","STRING",CURRENT,nucname,6))
	    return(1);
	if (Tflag)
	    Wscrprintf("tbo = %8.1lf, sfrq = %11.7lf, tof = %6.1lf\n",
		tbo,sfrq,TO);
	if (calcsyn == TRUE) 
	{ 
	    calcFrequency(h1freq,rftype[0],rfband[0],&tbo,sfrq,&TO,CALC);
    	    setparm("tbo","REAL",CURRENT,&tbo,1);
	    setparm("sfrq","REAL",CURRENT,&sfrq,1);
	    setparm("tof","REAL",CURRENT,&TO,1);
	    argdgv[0] = "dg";
	    argdgv[1] = "sfrq";
	    argdgv[2] = "tof";
	    argdgc = 3;
	    ExecD(argdgc,argdgv,0,0);
	}
	showfrequency("Spectrometer","tbo",h1freq,rftype[0],rfband[0],
					   tbo,sfrq,TO,nucname);
    }
    else
    {	if (strcmp(argv[CALLNAME],"decfreq") == 0) 
	{   if (getparm("dbo","REAL",CURRENT,&dbo,1))
		return(1);
	    if (calcsyn)
	    {   dfrq = reqfrq;
	    	strcpy(nucname,"Undef.");
	    	setparm("dn","STRING",CURRENT,nucname,1);
	    	argdgv[0] = "dg";
	    	argdgv[1] = "dn";
	    	argdgc = 2;
	    	ExecD(argdgc,argdgv,0,0);
	    }
	    else
	    {   if (getparm("dfrq","REAL",CURRENT,&dfrq,1))
	            return(1);
            }
	    if (getparm("dof","REAL",CURRENT,&DO,1))
		return(1);
	    if (getparm("dn","STRING",CURRENT,nucname,6))
		return(1);
	    if (Tflag)
	    	Wscrprintf("dbo = %8.1lf, dfrq = %11.7lf, dof = %6.1lf\n",
				dbo,dfrq,DO);
	    if (calcsyn == TRUE)
	    {   calcFrequency(h1freq,rftype[1],rfband[1],&dbo,dfrq,&DO,CALC);
    	    	setparm("dbo","REAL",CURRENT,&dbo,1);
	    	setparm("dfrq","REAL",CURRENT,&dfrq,1);
	    	setparm("dof","REAL",CURRENT,&DO,1);
	    	argdgv[0] = "dg";
	    	argdgv[1] = "dfrq";
	    	argdgv[2] = "dof";
	    	argdgc = 3;
	    	ExecD(argdgc,argdgv,0,0);
	    }
	    showfrequency("Decoupler","dbo",h1freq,rftype[1],rfband[1],
					    dbo,dfrq,DO,nucname);
	}
	else 
	{   Werrprintf("Illegal Alias: %s", argv[CALLNAME]);
	    return(1);
	}
    }
    return(0);
}
