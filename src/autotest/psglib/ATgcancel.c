// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/*  ATgcancel - gradient equipment two-pulse sequence 
     constant phase excitation: cancels in two scans

    Parameters: 
        gzlvl1 = gradient amplitude (-32768 to +32768)
        gt1 = gradient duration in seconds (0.002)   
        shaped = s gives automatic wurst gradient shape

*/

#include <standard.h>
static int phasecycle[4] = {0, 2, 1, 3};
pulsesequence()
{
   double gzlvl1,gt1;
   char shaped[MAXSTR];

      settable(t1,4,phasecycle);
      setreceiver(t1);

   getstr("shaped",shaped);
   gzlvl1 = getval("gzlvl1");
   gt1 = getval("gt1");
status(A);
   delay(d1);

status(B);
   if (shaped[A] == 's')
    zgradpulse(gzlvl1,gt1);     /* automatic gradient wurst shaping (vnmr1.1C and beyond)*/
   else
    {
     rgpulse(p1, zero,rof1,rof2);
     rgradient('z',gzlvl1);
     delay(gt1);
     rgradient('z',0.0);
     delay(d2);
    }
status(C);
   pulse(pw,zero);
}
