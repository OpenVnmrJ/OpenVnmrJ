/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include "rfconst.h"
#include "acqparms.h"
/*-----------------------------------------------------------------------
|
|	freqcorrect()/3
|	calculate the lock and solvent corrected frequency 
|	used for new freq. calculations by setoffsetsyn.c & 
|	setdirectsyn.c
|
|				Author Greg Brissey  8/20/87
+----------------------------------------------------------------------*/
extern int bgflag;
extern int  lkfreqflg;	/* lockfreq correction flag */
extern double lkfactor;
extern double solvfactor;

double freqcorrect(SpecFreq)	/* SpecFreq in MHz */
double SpecFreq;		/* frequency in MHz */
{
    double dtmp;
    
    SpecFreq *= 1.0e6;	/* put frequency into Hz */

    /*------------------------------------------------------------------------
    |  --- correct for lock frequency ---
    +-----------------------------------------------------------------------*/
    if (lkfreqflg)
    {
        if (bgflag)
        {
	   dtmp = SpecFreq * lkfactor;
	   fprintf(stderr, 
	   "freqcorrect: Lk crt %6.1lf, lkfactor = %12.8lf,new freq= %12.8lf \n",
      		dtmp,lkfactor,SpecFreq+dtmp);
        }

        SpecFreq += (SpecFreq * lkfactor);
    }
 
    /*------------------------------------------------------------------------
    |   --- correct for solvent ---
    +-----------------------------------------------------------------------*/
    if (bgflag)
    {
        dtmp = SpecFreq * solvfactor;
	fprintf(stderr, 
	   "freqcorrect(): Solv crt  %6.1lf, solvfactor = %11.8lf new freq= %12.8lf\n",
		dtmp,solvfactor,SpecFreq - dtmp);
    }
    SpecFreq -= (SpecFreq * solvfactor);

    SpecFreq += 0.05;   /* round up to nearest 0.1 Hz */
    SpecFreq *= 1e-6;   /* convert back to mHz */
    return(SpecFreq);
}
