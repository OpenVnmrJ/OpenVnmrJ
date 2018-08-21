// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */

/*  mtune - probe tuning sequence */

#include <standard.h>

pulsesequence()
{
   double tunesw;
   double freq;
   double attn,fstart,fend;
   int chan;
   double offset_sec;
   int np2;

   tunesw = getval("tunesw");
   if (tunesw < 0.0)
      tunesw = 1.0e7;
   np2 = np / 2;

   status(A);
   if (find("tchan") == -1)
   {
      chan = 1;
   }
   else
   {
      chan = (int) getval("tchan");
      if ((chan < 1) || (chan > 5))
         abort_message("tchan (%d) must be between 1 and 5\n", chan);
   }
   switch (chan)
   {
      case 1: freq = sfrq;
              break;
      case 2: freq = dfrq;
              break;
      case 3: freq = dfrq2;
              break;
      case 4: freq = dfrq3;
              break;
      case 5: freq = dfrq4;
              break;
      default: freq = sfrq;
              break;
   }
   if (find("tupwr") == -1)
   {   /* Optional parameter "tupwr" sets tune pwr */
       attn = 10.0;
   }
   else
   {
       attn = getval("tupwr");
   }
   if (attn > 10.0)
      attn = 10.0;
   fstart = freq - (tunesw/2) * 1e-6;
   fend = freq + (tunesw/2) * 1.0e-6;

   hsdelay(d1);
   set4Tune(chan,getval("gain")); 
   assign(zero,oph);
   genPower(attn,chan);
   delay(0.0001);
   offset_sec = (0.5 / sw);
   SweepNOffsetAcquire(fstart, fend, np2, chan, offset_sec); 
}
