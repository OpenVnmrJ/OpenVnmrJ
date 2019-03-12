// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/* cosyps - homonuclear correlation   (phase-sensitive version)

 Parameters:

      pw = 90 degree pulse on the observe nucleus
      p1 = x degree pulse on the observe nucleus  (used only with phase = 0)
           x:  90 degrees for standard COSY experiment
           x:  135 degrees for diagonal suppression with P-type peak selection
           x:  45 degrees for diagonal suppression with N-type peak selection

   phase =   0:  non-phase-sensitive experiment  (P-type peak selection)
           1,2:  phase-sensitive HYPERCOMPLEX experiment
             3:  phase-sensitive TPPI experiment
      nt = (min):  multiple of 4   (phase = 0)
                   multiple of 2   (phase = 1,2 or 3)
           (max):  multiple of 8   (phase = 0)
                   multiple 0f 4   (phase = 1,2 or 3)


 (all phase cycling taken from s.l. patt, to be submitted)
*/


#include <standard.h>

void pulsesequence()
{
/* VARIABLE DECLARATION */
   double          corr,
                   phase;
   int             iphase;


/* INITIALIZE VARIABLES */
   phase = getval("phase");
   iphase = (int) (phase + 0.5);
   if (iphase == 3)
      initval((double) (ix - 1), v14);
   if (p1 == 0.0)
      p1 = pw;


/* CHECK CONDITIONS */
   if ((rof1 < 9.9e-6) && (ix == 1))
      fprintf(stdout,"Warning:  ROF1 is less than 10 us\n");


/* STEADY-STATE PHASECYCLING
/* This section determines if the phase calculations trigger off of (SS - SSCTR)
   or off of CT */

   ifzero(ssctr);
      hlv(ct, v9);
      mod4(ct, v2);
      mod2(ct, v1);
   elsenz(ssctr);
      sub(ssval, ssctr, v12);	/* v12 = 0,...,ss-1 */
      hlv(v12, v9);
      mod4(v12, v2);
      mod2(v12, v1);
   endif(ssctr);


/* CALCULATE PHASECYCLE */
   hlv(v9, v8);
   add(v8, v9, v9);
   mod2(v9, v9);		/* 00111100 */
   dbl(v1, v1);
   add(v1, v9, v1);		/* 0202+00111100 */
   initval(4.0, v10);
   sub(v10, v2, v2);		/* 0321 */
   if ((iphase == 1) || (iphase == 2))
      assign(zero, v2);
   add(v2, v9, v2);		/* 0321+00111100 or 0+00111100 */
   mod4(v1, oph);
   if (iphase == 2)
      incr(v1);
   if (iphase == 3)
      add(v1, v14, v1);
/* Add FAD for phase=1 or phase=2 */
   if ((iphase == 1) || (iphase == 2))
   {
      initval(2.0*(double)((int)(d2*getval("sw1")+0.5)%2),v13);
      add(v1,v13,v1); add(oph,v13,oph);
   }


/* BEGIN ACTUAL PULSE SEQUENCE */
   status(A);
      hsdelay(d1);
   status(B);
      rgpulse(pw, v1, rof1, 1.0e-6);
      corr = 1.0e-6 + rof1 + 4.0*pw/3.1416;
      if (d2 > corr)
         delay(d2-corr);
      if (iphase == 0)
      {
         rgpulse(p1, v2, rof1, rof2);
      }
      else
      {
         rgpulse(pw, v2, rof1, rof2);
      }
   status(C);
}
