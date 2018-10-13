/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#define TRUE 1
#include <stdio.h>
#include <strings.h>
extern int Tflag;
/* ---------------------------------------------------------------
|  getsolventinfo(solvent_name,solvent_table,proton_shift);
|
|  Purpose:
|	This module opens and searches through the solvent
|	table to find the named solvent. It passed back 
|	the proton shift of the solvent.
|
|--  SOLVENT TABLE FORMAT ----
| Solvent	Deuterium	Melting	Boiling	Proton	Carbon Deteurium
| Name	 	Shift		Point	Point	Shifts	Shifts   T1
| ------------------------------------------------------------------------
| Proton Shift	Multiplicities	Coupling Constants
| ------------------------------------------------------------------------
| Carbon Shift	Multiplicities	Coupling Constants
| ========================================================================
|				Author Greg Brissey  5/12/86
+------------------------------------------------------------------*/

getsolventinfo(solvent,solvtable,pshift)
char	*solvent;	/* solvent name */
char	*solvtable;	/* solvent table file name */
double	*pshift;	/* proton shift of solvent */
{
    char    shftname[5];	/* shift label, H1,H2,H3,etc, C1,C2,C3,etc */
    char    solvname[20];	/* solvent name */
    double  boilpt;		/* boiling point of solvent */
    double  cshifts;		/* number of carbon chemical shifts */
    double  coupling;		/* coupling contant */
    double  dshift;		/* deuterium chemical shift (ppm) */
    double  dT1;		/* deuterium T1 constant */
    double  meltpt; 		/* melting point of solvent */
    double  multip;		/* multiplicities of a chemical shift */
    double  pshifts;		/* number of proton chemical shifts */
    double  ppmshift;		/* chemical shift (ppm) */

    FILE   *stream;
    int     i;
    int     ret;
    int     totalshifts;


    *pshift = 0.0;		/* initialize shift to zero */
    if ((stream = fopen(solvtable,"r")) == 0)
    {   Werrprintf("Cannot open the '%s' solvent table file, \n",solvtable);
        return(1);
    }
    /* ---------  Skip Header in File -----------------------  */
    fscanf(stream,"%*s%*s%*s%*s%*s%*s%*s");
    fscanf(stream,"%*s%*s%*s%*s%*s%*s%*s");
    fscanf(stream,"%*s");
    fscanf(stream,"%*s%*s%*s%*s%*s");
    fscanf(stream,"%*s");
    fscanf(stream,"%*s%*s%*s%*s%*s");
    fscanf(stream,"%*s");
    if (Tflag)
	Wscrprintf("getsolventinfo(): Solvent to find: '%s' \n",solvent);
    while (TRUE)
    {
        ret=fscanf(stream,"%s%lf%lf%lf%lf%lf%lf",
	   solvname,&dshift,&meltpt,&boilpt,&pshifts,&cshifts,&dT1);
	if (ret == EOF) break;
        if (ret == 0)
 	{   Werrprintf("Solvent table's format (%s) has been corrupted.",
			solvtable);
	    return(1);
	}
	if (Tflag > 1)
 	    Wscrprintf(
	   "getsolventinfo(): %s -  %8.3lf, %8.3lf, %8.3lf, %8.3lf, %8.3lf, %8.3lf\n",
	    solvname,dshift,meltpt,boilpt,pshifts,cshifts,dT1);

	totalshifts = (int) pshifts + (int) cshifts;
	for ( i = 0; i < totalshifts; i++)
	{
	    ret = fscanf(stream,"%s%lf%lf%lf",
		  	 shftname,&ppmshift,&multip,&coupling);
	    if (ret == EOF) break;
	    if (Tflag > 1)
 	    	Wscrprintf("getsolventinfo(): %s  %8.3lf, %8.3lf, %8.3lf\n",
	    	shftname,ppmshift,multip,coupling);
	}
	if ( strcmp(solvent,solvname) == NULL) 
	{
	    if (Tflag)
		Wscrprintf("getsolventinfo(): Found requested solvent: %s\n",
							solvent);
	    *pshift = dshift; 		/* pass back proton shift */
	    return(0);
	}
    }
    Werrprintf(
	"Requested solvent, '%s', is not an entry in the solvent table %s",
		solvent,solvtable);
    return(1);
}
