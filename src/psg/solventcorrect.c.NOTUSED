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
|  solventcorrect(freq,solvent_name,solvent_table_name);
|
|  Purpose:
|	This module is to calculate the new spectrometer frequency
|	corrected for any solvent chemical shift.
|
|				Author Greg Brissey  5/13/86
+------------------------------------------------------------------*/
double solventcorrect(freq,pshift)
double *freq;			/* frequency to compensated */
double	pshift;			/* proton chemical shift of solvent */
{
    double	crtfreq;		/* corrected frequency */

    if (Tflag)
    	Wscrprintf("solventcorrect(): Proton shift of %8.5lf ppm.  \n",
			pshift);

    /* corrected frequency = solvent shift from the norm (5ppm) */
    /* i.e., freq MHz * shift ppm -> freq shift in Hz; as shift */
    /* increases, freq decreases 				*/

    crtfreq = *freq - ( (*freq * ( pshift - 5.0)) * 1.0e-6);

    if (Tflag)
	Wscrprintf(
	"solventcorrect(): Frequency: %11.7lf ,Corrected Freq = %11.7lf \n",
		    *freq,crtfreq);
    *freq = crtfreq;
    return(0);
}

