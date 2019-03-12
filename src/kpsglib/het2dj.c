// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/* het2dj.c - heteronuclear J-resolved experiment; absolute value mode
              is required; the experiment can be performed either in a
              gated or a non-gated mode.


   Parameters:

         pw = 90 degree transmitter pulse  (X nucleus)
         p1 = 180 degree transmitter pulse
       tpwr = power level for transmitter pulses (if BB system)
         pp = 90 degree decoupler pulse  (1H)
      pplvl = power level for decoupler pulses
        dhp = power level for broadband proton decoupling
       dpwr = power level for broadband proton decoupling for systems
              with linear amplifiers
         dm = 'ynyy':  gating
              'ynny':  no gating
        dmm = 'wcw':  waltz
              'fcf':  no waltz
         nt = multiple of  2  (minimum)
	      multiple of 16  (maximum and recommended)


   Processing a J-resolved experiment requires that the command
   ROTATE(45.0) be executed.  The command FOLDJ may be optionally
   executed thereafter.

   This pulse sequence does not require fold-over correction.
*/


#include <standard.h>

void pulsesequence()
{
   double	pp;


/* LOAD VARIABLES */
   pp = getval("pp");

/* CHECK CONDITIONS */
   if (dm[B] == 'y')
   {
      (void) printf("DM must be set to 'n' for the second status.\n");
      psg_abort(1);
   }
   if (dm[C] == 'n')
   {
      if (dmm[B] != 'c')
      {
         (void) printf("DMM must be set to 'c' for the second status\n");
         (void) printf("if a non-gated experiment is performed.\n");
         psg_abort(1);
      }
   }
   if (pp == 0.0)
   {
      if (dm[C] == 'n')
      {
         (void) printf("PP must be non-zero for a non-gated experiment.\n");
         psg_abort(1);
      }
   }
   if ((rof1 < 9.9e-6) && (ix == 1))
      (void) printf("Warning:  ROF1 is less than 10 us.\n");
   if (p1 == 0.0)
      p1 = 2*pw;

/* STEADY-STATE PHASECYCLING
/* This section determines if the phase calculations trigger off of (SS - SSCTR)
   or off of CT */

   ifzero(ssctr);
      dbl(ct, v1);
      hlv(ct, v3);
   elsenz(ssctr);
      sub(ssval, ssctr, v7);	/* v7 = 0,...,ss-1 */
      dbl(v7, v1);
      hlv(v7, v3);
   endif(ssctr);


/* PHASECYCLE CALCULATION */
   hlv(v3, v2);
   mod2(v3, v3);
   add(v3, v1, v1);
   assign(v1, oph);
   dbl(v2, v9);
   add(v9, oph, oph);
   add(v2, v3, v2);


/* BEGIN ACTUAL SEQUENCE */
   status(A);
      hsdelay(d1);
   status(B);
      rgpulse(pw, v1, rof1, rof1);
      delay(d2/2);

      if (dm[C] == 'y')
      {
         rgpulse(p1/2, v2, rof1, 0.0);
         status(C);
         rgpulse(p1/2, v2, 0.0, 2*rof1 + pw/2);
         delay(d2/2);
      }
      else
      {
         if (declvlonoff)
            declvlon();
         else
            decpower(pplvl);
         decrgpulse(pp, zero, rof1, 0.0);
         simpulse(p1, 2*pp, v2, one, 1.0e-6, 0.0);
         decrgpulse(pp, zero, 1.0e-6, 2*rof1 + pw/2);
         if (declvlonoff)
	    declvloff();
         else
	    decpower(dpwr);
         delay(d2/2);
      }
   status(D);
}
