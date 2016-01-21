// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/* mqcosy - multiple quantum filtered cosy experiment

   ref: u. piantini, o.w. sorenson, and r.r. ernst,
        j. am. chem. soc. 104:6800-6801 (1982)
        m. rance et al., bbrc 117:479-485 (1983)


 Parameters:

      pw = 90 excitation pulse (at power level tpwr)
   phase = 1,2: HYPERCOMPLEX phase-sensitive experiment
             3: TPPI phase-sensitive experiment
  presat = decoupler presaturation period; if PRESAT > 0, D1 is reduced
           to (D1 - PRESAT) and the presaturation period is added in
           after that period; does not depend on DM but does depend on
           DMM.
   sspul = 'y': selects for HS-90-HS sequence at start of pulse sequence
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
   revised	 19 july       1988 */



#include <standard.h>

void pulsesequence()
{
   double	base,
                corr,
                presat,
                qlvl;
   char         sspul[MAXSTR];


/* LOAD VARIABLES AND CHECK CONDITIONS */
   presat = getval("presat");
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
   if (phase1 == 3)  /* TPPI */
      add(id2, v1, v1);
/* FAD added for phase=1 or phase=2 */
   if ((phase1 == 1) || (phase1 == 2))
   {
      initval(2.0*(double)(d2_index%2),v13);
      add(v1,v13,v1); add(oph,v13,oph);
   }


/* BEGIN ACTUAL PULSE SEQUENCE CODE */
   if (newtrans)
      obsstepsize(base);

   status(A);
   if (sspul[0] == 'y')
   {
      hsdelay(hst + 0.001);
      rgpulse(pw, v1, 1.0e-6, 1.0e-6);
      hsdelay(hst + 0.001);
   }
   if ((d1 - presat) <= hst)
   {
      rcvroff();
      decon();
      hsdelay(presat);
      decoff();
      delay(1.0e-6);
      rcvron();
   }
   else
   {
      hsdelay(d1 - presat);
      decon();
      rcvroff();
      delay(presat);
      decoff();
      delay(1.0e-6);
      rcvron();
   }
   status(B);
      if (newtrans)
         xmtrphase(v10);      /* hardware digital phaseshift */
      rgpulse(pw, v1, rof1, 1.0e-6);
      corr = 1.0e-6 + rof1 + 4.0*pw/3.1416;
      if (d2  > corr)
        delay(d2-corr); 
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
