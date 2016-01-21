/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef LINT
#endif

/*  C13spinecho   */

#include <standard.h>

pulsesequence()
{
  double pwClvl,gzlvl1,gt1;
  gzlvl1=getval("gzlvl1"); gt1=getval("gt1");
  pwClvl=getval("pwClvl");
    add(oph,one,v1);
   /* equilibrium period */
   status(A);
   obspower(pwClvl);
   hsdelay(d1);

   /* --- tau delay --- */
   status(B);
   rgpulse(pw, oph,rof1,0.0);
   txphase(zero);
   zgradpulse(gzlvl1,gt1);
   delay(d2-gt1);
   rgpulse(p1,v1,0.0,0.0);
   zgradpulse(gzlvl1,gt1);
   delay(d2-gt1);
  status(C);
}
