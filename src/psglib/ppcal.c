// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/* ppcal (semut) - spectral editing with multiple quantum trap
  (simplified version for use in decoupler pulse calibration)


         pp -  proton 90 degree decoupler pulse
      pplvl -  power level for proton decoupler pulse  */


#include <standard.h>

static int phasecycle[4] = {0, 2, 1, 3};

void pulsesequence()
{
   int		rxgate;
   double	pp,
		pplvl;

   pp = getval("pp");
   pplvl = getval("pplvl");

   rxgate = (rof1 == 0.0);
   if (rxgate)
      rof1 = 1.0e-6;			/* phase switching time */

   if (newdecamp)
   {
      if (rxgate)
         rof1 = 40.0e-6;
   }

   status(A);
      decpower(pplvl);
      hsdelay(d1);
   status(B);
      settable(t1,4,phasecycle);
      pulse(pw, t1);
      delay(d2);
      simpulse(p1, pp, t1, t1, rof1, rof1);
      decpower(dpwr);
      delay(d2);
   status(C);
   setreceiver(t1);
}
