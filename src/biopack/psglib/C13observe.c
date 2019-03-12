/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  C13observe  
     spinecho option following s2pul
     p1 and pw pulses are at pwClvl power level
     spinecho pulse is 2.0*pwC at pwClvl
     check for spinecho='n' for rof2

     rof2 is only used for non-spinecho case. Coil ringing and
     acoustic ringing requires significant rof2 or delay before
     acquisition, particularly for cold probes (200-400 usec).  
 */

#include <standard.h>

void pulsesequence()
{
  char spinecho[MAXSTR];
  double gzlvl1,gt1,pwClvl,pwC;
  getstr("spinecho",spinecho);
  gzlvl1=getval("gzlvl1"); gt1=getval("gt1");
  pwClvl=getval("pwClvl"); pwC=getval("pwC");
    add(oph,one,v1);

  if(( rof2 < 10.0e-6) && (spinecho[A] =='n'))
   { text_error("Protect 13C preamp. Set rof2>10.0 for RF probe, >350 for cold probe");   	    psg_abort(1); }
  
 /* equilibrium period */
   status(A);
   obspower(pwClvl);
   delay(d1);

   /* --- tau delay --- */
   status(B);
   rgpulse(p1,zero,rof1,0.0);
   delay(d2);
   if (spinecho[A] =='n')
    rgpulse(pw, oph,rof1,rof2);
   else
    rgpulse(pw, oph,rof1,0.0);

   if (spinecho[A] == 'y') 
   {
    zgradpulse(gzlvl1,gt1);
    obspower(pwClvl);
    delay(d3-gt1-POWER_DELAY);
    rgpulse(2.0*pwC,v1,rof1,rof1);
    obspower(pwClvl);
    zgradpulse(gzlvl1,gt1);
    delay(d3-gt1-POWER_DELAY);
   }
  status(C);
}
