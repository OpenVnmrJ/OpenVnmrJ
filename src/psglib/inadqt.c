// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/* inadqt:  carbon-carbon connectivity using double quantum
            spectroscopy;  F1 quadrature is achieved by hardware
            small-angle phaseshifts


  Parameters:

      pw = 90 degree carbon pulse
     jcc = carbon carbon coupling constant in hz
   phase =   0: absolute value with F1 quadrature
           1,2: hypercomplex method (phase-sensitive, F1 quadrature)
             3: TPPI method (phase-sensitive, F1 quadrature)
      nt = min:  multiple of   8 (phase=0)
                 multiple of   4 (phase=1,2  phase=3)
           max:  multiple of 128 (phase=0)
                 multiple of  64 (phase=1,2  phase=3)

  NOTE:  Data acquired with phase=0 should be processed with wft2d.  Data
         acquired with phase=1,2 or phase=3 should be processed with wft2da.
         If phase-sensitive data without F1 quadrature are desired, set phase=1
         and process with wft2da.

	 For 1D spectra, set phase=1 for maximum sensitivity.


  s. farmer  25 February   1988 */


#include <standard.h>
#define BASE 45.0

void pulsesequence()
{
   double          jcc,
                   corr,
                   jtau;

/* INITIALIZE PARAMETER VALUES */
   jcc = getval("jcc");
   jtau = 1.0 / (4.0 * jcc);


/* CHECK CONDITIONS */
   if (rof1 < 2.0e-5)
      rof1 = 2.0e-5;
   if (rof2 < 1.0e-5)
      rof2 = 1.0e-5;

/* STEADY-STATE PHASECYCLING */
/* This section determines if the phase calculations trigger off of (SS - SSCTR)
   or off of CT */

   ifzero(ssctr);
      mod4(ct, v3);
      hlv(ct, v9);
   elsenz(ssctr);
      sub(ssval, ssctr, v12);		/* v12 = 0,...,ss-1 */
      mod4(v12, v3);
      hlv(v12, v9);
   endif(ssctr);


/* CALCULATE PHASECYCLE */
   hlv(v9, v9);
   if (phase1 == 0)		/* ABSOLUTE VALUE SPECTRA */
   {
      assign(v9, v10);
      hlv(v9, v9);
      mod2(v10, v10);
   }
   else
   {
      assign(zero, v10);	/* v10 = F1 quadrature */
   }
   assign(v9, v1);
   hlv(v9, v9);
   assign(v9, v2);
   hlv(v9, v9);
   hlv(v9, v9);
   mod2(v9, v9);		/* v9 = F2 quad. image suppression */

   dbl(v1, v1);			/* v1 = suppresses artifacts due to imperfec-
				        tions in the first 90 degree pulse */
   add(v9, v1, v1);
   assign(v1, oph);


   dbl(v2, v8);
   add(v9, v2, v2);		/* v2 = suppresses artifacts due to imperfect
				   180 refocusing pulse */
   add(v8, oph, oph);


   dbl(v3, v4);
   add(v3, v4, v4);
   add(v3, v9, v3);
   add(v4, oph, oph);		/* v3 = selects DQC during the t1 evolution
				        period */
   add(v10, oph, oph);
   if (phase1 == 2)
      incr(v10);		/* HYPERCOMPLEX */
   if (phase1 == 3)		/* TPPI */
   {
      add(id2, v10, v10);
   }


/* ACTUAL PULSE SEQUENCE BEGINS */
   if (newtrans)
      obsstepsize(BASE);

   status(A);
      hsdelay(d1);
   status(B);
      if (newtrans)
         xmtrphase(v10);
      rgpulse(pw, v1, rof1, rof2);
      delay(jtau - rof1 - rof2 - 3*pw/2);
      rgpulse(2.0*pw, v2, rof1, rof2);
      delay(jtau - rof1 - rof2 - 3*pw/2);
   status(C);
      rgpulse(pw, v9, rof1, 0.2e-6);
      corr = 0.2e-6 + 1.0e-6 + 4.0*pw/3.14159265358979323846;
      if (newtrans)
      {
         xmtrphase(zero);
         corr += SAPS_DELAY;
      }
      else
      {
         phaseshift(-BASE, v10, TODEV);
         corr += 35.0e-6;
      }
      if (d2 > corr)
        delay(d2-corr);
      rgpulse(pw, v3, 1.0e-6, rof2);
   status(D);
}
