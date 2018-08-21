// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/* gXHCAL - Sequence for X/H gradient ratio calibration
		(based on gHSQC) 
	
        Paramters:
                gzlvl1  :       encoding Gradient level
                gt1     :       encoding gradient time
                gzlvl3  :       decoding Gradient level
                gt3     :       decoding gradient time
                gstab   :       recovery delay
                j1xh    :       One-bond XH coupling constant
                pwxlvl  :       X-nucleus pulse power
                pwx     :       X-nucleus 90 deg pulse width
                d1      :       relaxation delay

KrishK -	Last rivision : Jan 1998

*/

#include <standard.h>
#include <chempack.h>

pulsesequence()

{
   double   pwxlvl,
            pwx,
		gzlvl1,
		gt1,
		gzlvl3,
		gt3,
		gstab,
            tau,
            j1xh;

   pwxlvl = getval("pwxlvl");
   pwx    = getval("pwx");
   gzlvl1 = getval("gzlvl1");
   gzlvl3 = getval("gzlvl3");
   gt1 = getval("gt1");
   gt3 = getval("gt3");
   gstab = getval("gstab");
   j1xh    = getval("j1xh");
   tau  = 1/(4*j1xh);

   assign(zero,oph);

   status(A);
     decpower(pwxlvl);
     obspower(tpwr);
      delay(d1);

    status(B);

     rgpulse(pw,zero,rof1,rof1);
     delay(tau);
     simpulse(2*pw,2*pwx,zero,zero,rof1,rof1);
     delay(tau);
     simpulse(pw,pwx,one,zero,rof1,rof1);

     delay(gt1+gstab + 2*GRADIENT_DELAY);
     decrgpulse(2*pwx,zero,rof1,rof1);
     zgradpulse(gzlvl1,gt1);
     delay(gstab);
     
     simpulse(pw,pwx,zero,zero,rof1,rof1);
     delay(tau - (2*pw/PI) - 2*rof1);
     simpulse(2*pw,2*pwx,zero,zero,rof1, rof2);
     decpower(dpwr);
     zgradpulse(gzlvl3,gt3);
     delay(tau - gt3 - 2*GRADIENT_DELAY - POWER_DELAY);

   status(C);
}
