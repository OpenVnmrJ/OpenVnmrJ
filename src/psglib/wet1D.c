// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/*  wet1d - standard two-pulse sequence */

#include <standard.h>
#include <chempack.h>

static int phs1[4] = {0,2,1,3},
           phs2[8] = {0,0,1,1,2,2,3,3};

pulsesequence()
{

  char compshape[MAXSTR],
	sspul[MAXSTR],
	wet[MAXSTR],
	composit[MAXSTR];
  getstr("compshape",compshape);
  getstr("composit",composit);
  getstr("sspul",sspul);
  getstr("wet",wet);

  settable(t1,4,phs1);
  settable(t2,8,phs2);
  getelem(t1,ct,oph);
  getelem(t2,ct,v2);
  assign(oph,v1);

   /* equilibrium period */
   status(A);

   delay(5.0e-5);
   if (sspul[0] == 'y')
        steadystate();

	delay(d1);

   if (wet[0] == 'y')
     wet4(zero,one);

   status(B);

   pulse(p1,zero); 
   hsdelay(d2); 

   if (composit[0] == 'y')
          shaped_pulse(compshape,4.0*pw+0.8e-6,v1,rof1,rof2);
   else
      rgpulse(pw,v1,rof1,rof2);

   status(C);
}
