// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */

/* tnmqcosy - multiple quantum filtered cosy experiment
              (transmitter saturation)
   ref: u. piantini, o.w. sorenson, and r.r. ernst,
        j. am. chem. soc. 104:6800-6801 (1982)
        m. rance et al., bbrc 117:479-485 (1983)


 Parameters:

      pw = 90 excitation pulse (at power level tpwr)
   phase = 1,2: HYPERCOMPLEX phase-sensitive experiment
             3: TPPI phase-sensitive experiment
 satmode = 'ynn' saturates during relaxation delay
  satdly = saturation time
  satpwr = saturation power
   sspul = 'y': selects for trim(x)-trim(y) sequence at start of pulse sequence
           'n': normal MQCOSY experiment
    qlvl = selects the quantum order for filtering, e.g., 2, 3, etc.
      nt = min:  multiple of 2*qlvl
           max:  multiple of 8*qlvl


 NOTE:  If phase = 3, remember that sw1 must be set to twice the
        desired value.  The 28-february revision included the following
        sequence at the beginning of the pulse sequence:  homospoil -
        90 degree pulse - homospoil.  This should eliminate both the
        DQ-like artifacts in the 2D spectrum and the oscillatory nature
        of the steady-state.  This inclusion is selected if sspul='y'.


 This pulse sequence uses the hardware digital phaseshifter for xmtr
 with direct synthesis RF and the software small-angle phaseshifter
 for xmtr with old-style RF.


   s. farmer     28 september  1987
   revised       21 december   1987
   revised       25 february   1988
   revised	 19 july       1988 
*/



#include <standard.h>

void pulsesequence()
{
   double	base,
                qlvl;
   char         sspul[MAXSTR];


/* LOAD VARIABLES AND CHECK CONDITIONS */
   qlvl = getval("qlvl");
   getstr("sspul", sspul);

   base = 180.0 / qlvl;
   initval(2.0 * qlvl, v5);

   if ((rof1 < 9.9e-6) && (ix == 1))
      fprintf(stdout,"Warning:  ROF1 is less than 10 us\n");


/* STEADY-STATE PHASECYCLING */
/* This section determines if the phase calculations trigger off of (SS - SSCTR)
   or off of CT */

   ifzero(ssctr);
      modn(ct, v5, v10);
      divn(ct, v5, v12);
      mod2(ct, v9);
   elsenz(ssctr);
      sub(ssval, ssctr, v14);	/* v14 = 0,...,ss-1 */
      modn(v14, v5, v10);
      divn(v14, v5, v12);
      mod2(v14, v9);
   endif(ssctr);


/* CALCULATE PHASECYCLE */
/* The phasecycle first performs a (2*Q)-step cycle on the third pulse in order
   to select for MQC.  The phasecycle is then adjusted so that the receiver
   goes +- in an alternating fashion.  Second, the 2-step QIS cycle is added
   in.  Third, a 2-step cycle for axial peak suppression is performed on the
   first pulse. */

   assign(v12, v1);
   mod2(v12, v12);		/* v12=quad. image suppression */
   hlv(v1, v1);
   mod2(v1, v1);
   dbl(v1, v1);
   add(v1, v12, v4);
   add(v12, v1, v1);
   assign(v12, v2);
   assign(v12, v3);
   dbl(v9, v9);
   add(v9, v4, v4);
   assign(v4, oph);
   if (phase1 == 2)
      incr(v1);
   if (phase1 == 3)
      add(id2, v1, v1);        /* TPPI increment */


/* BEGIN ACTUAL PULSE SEQUENCE CODE */
   if (newtrans)
      obsstepsize(base);

   status(A);
   if (sspul[A] == 'y')
   {
      rgpulse(200*pw, zero, rof1,0.0e-6);
      rgpulse(200*pw, one, 0.0e-6, rof1);
   }
   if (satmode[A] == 'y')
    {
     obspower(satpwr);
     rgpulse(satdly,zero,rof1,rof1);
     obspower(tpwr);
    }
   status(B);
      if (newtrans)
         xmtrphase(v10);      /* hardware digital phaseshift */
      rgpulse(pw, v1, rof1, 1.0e-6);
      if (satmode[B] == 'y')
       {
         obspower(satpwr);
         if (d2>0.0) rgpulse(d2 -9.4e-6 -rof1 -(4*pw)/3.1416,zero,0.0,0.0);
         obspower(tpwr);
       }
      else
       {
        if (d2>0.0) delay(d2 -1.0e-6 -rof1 -(4*pw)/3.1416);
       }
      rcvroff();
      rgpulse(pw, v2, rof1, 0.0);
      if (newtrans)
      {
         xmtrphase(zero);       /* resets relative phase to absolute phase */
      }
      else
      {
         phaseshift(-base, v10, OBSch);   /* software small-angle phaseshift */
      }
      rgpulse(pw, v3, 1.0e-6, rof2);
   status(C);
}
