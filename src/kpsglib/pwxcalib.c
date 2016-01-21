// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/* pwxcalib - Sequence for pwx   calibration
	
        Paramters:
                j1xh    :       One-bond XH coupling constant
                pwxlvl  :       X-nucleus pulse power
                pwx     :       X-nucleus 90 deg pulse width
                d1      :       relaxation delay

KrishK -	Last revision : Jan 1998

*/

#include <standard.h>
#include <chempack.h>

pulsesequence()
{
   double          j1xh,
		   pwxlvl,
		   pwx,
		   tau;

   pwxlvl = getval("pwxlvl");
   pwx = getval("pwx");
   j1xh = getval("j1xh");
   tau = 1.0 / (2.0*j1xh);

/* BEGIN ACTUAL PULSE SEQUENCE CODE */
   status(A);
      decpower(pwxlvl);
      delay(d1);

   status(B);

      rgpulse(pw, oph, rof1, rof1);
      delay(tau - (2*pw/PI) - 2*rof1);
      simpulse(2*pw,pwx,oph,zero,rof1,rof2);
      decpower(dpwr);
      delay(tau - POWER_DELAY);

   status(C);
}
