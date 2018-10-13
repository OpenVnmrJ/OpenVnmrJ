/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <math.h>
#include "group.h"
#include "variables.h"
#include "params.h"
#include "pvars.h"

/* Sun defines DMAXPOWTWO in values.h, but others don't.  DMAXPOW2 is from Sun's value */
#ifndef DMAXPOW2
#define DMAXPOW2 4503599627370496.0
#endif

/*-----------------------------------------------------------------------------
| rf_stepsize()/3 obtains frequency step size of an RF channel
|  		    using the stepsize of the freq. offset parameter
|		    (e.g. tof,dof,dof2)
|                 the stepsize is returned in the first argument
|                 the name of the parameter is supplied as the second argument
|                 the tree to find parstep is an argument since some programs
|                 use the tree GLOBAL (acqi and PSG) while others (Vnmr) use
|                 SYSTEMGLOBAL
|                 return of 0 is success
|                 return of non-zero is an error
|                 return of -1 signals offset parameter not found
|                 return of positive integer signals that parstep
|                 index not found
|                 in the case of an error, the stepsize is set
|                 to a default of 0.1 Hz
+------------------------------------------------------------------------------*/
int
rf_stepsize(double *stepsize, char *varname, int tree)
{
   int   pindex;
   vInfo  varinfo;

   /*--- determine frequency step size of channel tof, dof, dof2  etc. ---*/
   if ( P_getVarInfo(CURRENT,varname,&varinfo) )
   {   
      *stepsize = 0.1;   /* default value */
      return(-1);
   }
   if (varinfo.prot & P_MMS)     /* stepsize an index into parstep ? */
   {
      pindex = (int) (varinfo.step+0.1);
      if (P_getreal( tree, "parstep", stepsize, pindex ))
      {
         *stepsize = 0.1;   /* default value */
         if (pindex < 1)
            pindex = 1;
         return(pindex);
      }
   }
   else
   {
      *stepsize = varinfo.step;
   }
   return(0);
}

/*-----------------------------------------------------------------------
|   round_freq()/4
|   Round the frequency of the rf channel based on the resolution of
|   that channel
|   Both an offset and initial offset are provided.  In PSG, the base frequency
|   will have the initial offset value incorporated.  Therefore, for arrays of
|   offsets,  the initial value must be subtracted.
|   baseMHz is freq in MHz
|   all other arguments are supplied in Hz.
|   Value is returned as Hz.
|
+----------------------------------------------------------------------*/
double round_freq(double baseMHz, double offsetHz,
                  double init_offsetHz, double stepsizeHz)
{
   double  adj_freq;

   /* SpecFreq in Hz + offset (tof,dof) */
   adj_freq = baseMHz * 1e6 + offsetHz - init_offsetHz; /* Hz */
   if (adj_freq <= 0) {
       return 0;
   }
   if (stepsizeHz <= adj_freq / DMAXPOW2) {
       stepsizeHz = 0.1;	/* Stepsize was too small */
   }
   adj_freq /= stepsizeHz;	/* Units of stepsizeHz */
   adj_freq = floor(adj_freq + 0.5); /* Round to nearest step */
   adj_freq *= stepsizeHz * (1 + 2/DMAXPOW2); /* Hz - rounded up */
   /* All significant digits should be correct */
   return adj_freq;
}

/*--------------------------------------------------------------------
| whatamphibandmin() obtains amplifier high band minimum frequency 
|  		for each channel, if no value calcs defaults 
|		based on passed freq.
|		for h1 < 350, make 0.85 times h1freq so h1 is not low band
+---------------------------------------------------------------------*/
double
whatamphibandmin(int rfchan, double freq)
{
    double amphbmin;

    if (rfchan < 1)
       rfchan = 1;
    if (freq <= 0.0)
    {
       double h1freq;
       P_getreal(GLOBAL,"h1freq",&h1freq,1);
       freq = h1freq;
    }
    if ( P_getreal(GLOBAL,"amphbmin",&amphbmin,rfchan) < 0 )
    {
        if (freq > 790.0)
           amphbmin = 370.0;
        else if (freq > 350.0)
           amphbmin = 310.0;
        else
           amphbmin = freq*0.85; 
    }
    return( amphbmin );
}

