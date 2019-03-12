// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
#include <standard.h>

void pulsesequence()
{
  double oslsfrq=getval("oslsfrq");
  status(A);
    obsoffset(tof);
    hsdelay(d1);

  status(B);
    rgpulse(p1,zero,rof1,0.0);
    delay(d2);

  status(C);
    rgpulse(pw,oph,rof1,rof2);
    obsoffset(tof+oslsfrq);
}
