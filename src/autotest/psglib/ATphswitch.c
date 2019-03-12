// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */

#include <standard.h>


void pulsesequence()
{
   hsdelay(d1);
   rcvroff();
   if (dpwr==0.0) 
    rgpulse(p1, zero,50.0e-6,0.0);
   else
    decrgpulse(p1,zero,50.0e-6,0.0);
   delay(d2-rof1);
   if (dpwr==0.0)
    rgpulse(pw,two,rof1,rof2);
   else
    decrgpulse(pw,two,rof1,rof2);
}
