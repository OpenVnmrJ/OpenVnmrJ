// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/* hom2dj.c - homonuclear J-resolved experiment; absolute value mode
              is required.


   Parameters:

         pw = 90 degree xmtr pulse
         p1 = 180 degree xmtr pulse
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
/* CHECK CONDITIONS */
   if (rof1 < 9.9e-6)
      fprintf(stdout, "Warning:  ROF1 is less than 10 us.\n");
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
   dbl(v2, v4);
   add(v4, oph, oph);
   add(v2, v3, v2);


/* BEGIN ACTUAL SEQUENCE */
   status(A);
      hsdelay(d1);
   status(B);
      rgpulse(pw, v1, rof1, rof1);
      delay(d2/2);
      rgpulse(p1, v2, rof1, 2*rof1 + pw/2);
      delay(d2/2);
   status(C);
}
