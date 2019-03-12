// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/* dante -   selective excitation via pulse train  */
/*       contact- G. Gray  (palo alto)  revision-  */
#include <standard.h>

void pulsesequence()
{
    double count,channel;
    channel = getval("channel");
    count = getval("count");
    if (count>1.0) count = count/5.0;
    initval(count,v1);
  status(A);
     hsdelay(d1);
     rcvroff(); delay(rof1);
   if (channel < 2.0) 
    {
     if (count == 1.0) pulse(pw,oph);
     else
     {
     starthardloop(v1);
      rgpulse(pw,oph,0.0,d2);
      rgpulse(pw,oph,0.0,d2);
      rgpulse(pw,oph,0.0,d2);
      rgpulse(pw,oph,0.0,d2);
      rgpulse(pw,oph,0.0,d2);
     endhardloop();
     }
    }
    else
    {
     if (count == 1.0) decrgpulse(pw,oph,rof1,rof2);
     else
     {
     starthardloop(v1);
      decrgpulse(pw,oph,0.0,d2);
      decrgpulse(pw,oph,0.0,d2);
      decrgpulse(pw,oph,0.0,d2);
      decrgpulse(pw,oph,0.0,d2);
      decrgpulse(pw,oph,0.0,d2);
     endhardloop();
     }
    }
    rcvron();
}
