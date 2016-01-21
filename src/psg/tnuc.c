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
|    tnuc,dnuc -  Usage: tnuc('nucleus_name')
|			string: 'tn','dn','tndn'
| 
|   Purpose:
|  	This module retrieves the specified nucleus information
|	from the nucleus table. Sets sfrq,tbo,(dfrq,dbo,),syn[],rfband[]
|	according to the nucleus table.
| 
|   Parameters altered:
|	tn,tbo,tof,sfrq,or dn,dbo,dof,dfrq
|	syn[],rfband[]
|
|   Routines:
| 	tnuc,dnuc  -  tnuc main function.
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
#define REQUIREDPARAMS 2
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
 /*   char        rftype[10];		/* RF Type (a,b,c,d,e)  */
tnuc(argc,argv,retc,retv)
int argc,retc;  char *argv[],*retv[];
{
    char	frqband[10];	/* frequency band, high or low */
    char	nucname[20];
    char	nucvar[10];
    char	rfband[10];		/* RF high or low band identifier */
    char        rftype[10];		/* RF Type (a,b,c,d,e)  */
    char	syn[10];		/* PTS present for trans & decoupler */
    char	synthesizer[10];/* PTS synthesizer, yes,no,reverse */
    double	ct;		/* ct is set to zero */
    double	FO;		/* frequency offset */
    double	h1freq;		/* proton freq of instrument */
    double	nucbaseoffset;	/* base offset frequency */
    double	nucfreq;	/* Nucleus frequency */
    double	pshift;		/* solvent proton chemical shift ppm */
    double	reffreq;	/* Reference frequency */
    double	solvshift;	/*  solvent chemicl shift ppm */
    double	specfreq;	/*  Frequency to be corrected */
    int 	calibrated;	/* flag to indicate freq has been corrected */
    int 	rfindex;	/* RF index for trans or decoupler */
    int 	trans;		/* flag to indicate trans or decoupler */

    if ( argc < REQUIREDPARAMS )
    {
	fprintf(stdout,"Usage - %s(nucleus_name,[solvent_name]) ",argv[0]);
	return(1);
    }
    nucfreq = nucbaseoffset = reffreq = 0.0;	/* initialize to zero */

    if (getparm("rftype","STRING",GLOBAL,&rftype[0],4))
	return(1);
    if (getparm("h1freq","REAL",GLOBAL,&h1freq,1))
	return(1);

    if (Tflag) Wscrprintf("rftype = %s, h1freq = %8.4lf\n",
		rftype,h1freq);

    /* --- tnuc  or dnuc called ? --- */
    if (strcmp(argv[CALLNAME],"tnuc") == 0) 
    {
	trans = TRUE;
	rfindex = 0;
    }
    else
    {
	trans = FALSE;
	rfindex = 1;
    }

    /* --- 1st parameter must be the Nucleus Name to be Used --- */
    if (!isReal(argv[1]))
	strcpy(nucname,argv[1]);
    else
    {
	Werrprintf("1st parameter '%s' is not a Nucleus Name. \n",argv[1]);
	return(1);
    }

    /* ----------------- get nucleus table information -------------- */
    if (Tflag) Wscrprintf("rftype[%d] = %c\n",rfindex,rftype[rfindex]);
    if (Tflag) Wscrprintf("rftype = %s\n",rftype);
    if (getnucinfo(h1freq,rftype[rfindex],nucname,&nucfreq,&nucbaseoffset,
		   frqband,synthesizer,&reffreq))
    {
	return(1);
    }
    if (Tflag)
        Wscrprintf(
        "Nuc: %s, tbo = %11.7lf, sfrq = %11.7lf, fband: %s, PTS: %s, ref = %11.7lf\n",
	      nucname,nucbaseoffset,nucfreq,frqband,synthesizer,reffreq);

    if (Tflag)
        showfrequency("Spectrometer","fbo",h1freq,rftype[rfindex],frqband,
				nucbaseoffset,nucfreq,FO,nucname);

    if (trans)
    {
    	setparm("tn","STRING",CURRENT,nucname,1);
    	setparm("tbo","REAL",CURRENT,&nucbaseoffset,1);
    	setparm("sfrq","REAL",CURRENT,&nucfreq,1);
    	setparm("tof","REAL",CURRENT,&FO,1);
    }
    else
    {
    	setparm("dn","STRING",CURRENT,nucname,1);
    	setparm("dbo","REAL",CURRENT,&nucbaseoffset,1);
    	setparm("dfrq","REAL",CURRENT,&nucfreq,1);
    	setparm("dof","REAL",CURRENT,&FO,1);
    }

    if (getparm("syn","STRING",CURRENT,&syn[0],4))
	return(1);
    if (getparm("rfband","STRING",CURRENT,&rfband[0],4))
	return(1);
    if (Tflag)
	Wscrprintf("syn = '%s', rfband = '%s' \n",
	syn,rfband);

    /* --- PTS synthesizer Yes, No, Reverse? */
    syn[rfindex] = synthesizer[0];
    /* --- Set rfband --- */
    rfband[rfindex] = frqband[0];
    if (Tflag)
	Wscrprintf("syn = '%s', rfband = '%s' \n",
	syn,rfband);
    setparm("rfband","STRING",CURRENT,rfband,1);
    setparm("syn","STRING",CURRENT,syn,1);
    ct = 0.0;
    setparm("ct","REAL",CURRENT,&ct,1);
}
