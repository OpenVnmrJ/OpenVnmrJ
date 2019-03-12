// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/*  T1meas - Standard two-pulse sequence with presaturation option
              during relaxation delay d2 and d1 

   Parameters:
	satmode	:	y - selects presaturation option
	satfrq	:	presaturation frequency
	satdly	:	presaturation delay
	satpwr	:	presaturation power
	pw	:	Observe pulse width
	d1	:	relaxation delay

KrishK	-	Last revision	: June 1997
Igor Goljer     Last revision   : Nov 2004

*/


#include <standard.h>

static int phs1[4] = {0,2,1,3},
	   phs2[8] = {0,0,1,1,2,2,3,3};


void pulsesequence()
{
   double 	satfrq,
		satdly,
		satpwr;
   char		satmode[MAXSTR];

   satfrq = getval("satfrq");
   satdly = getval("satdly");
   satpwr = getval("satpwr");
   getstr("satmode",satmode);

   settable(t1,4,phs1);
   settable(t2,8,phs2);
   getelem(t1,ct,oph);

/* equilibration period */
   status(A);

/* presaturation */
   if (satmode[0] == 'y')
   {
      if (satfrq != tof)
         obsoffset(satfrq);
      obspower(satpwr);
      rgpulse(satdly,t2,rof1,rof1);
      obspower(tpwr);
      if (satfrq != tof)
         obsoffset(tof);
   }
   else
      hsdelay(d1);

/* inversion pulse */
   status(B);
   pulse(p1,zero); 

/* presaturation */
   if (satmode[1] == 'y')
   {
      if (satfrq != tof)
         obsoffset(satfrq);
      obspower(satpwr);
      rgpulse(d2,t2,rof1,rof1);
      obspower(tpwr);
      if (satfrq != tof)
         obsoffset(tof);
   }
   else
      hsdelay(d2);

/* observation pulse */
   status(C);
   pulse(pw,oph);
}
