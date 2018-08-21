// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/* troesy1 - transverse cross-relaxation experiment in rotating frame 
   ref:  Shaka, et. al., JACS, 114, 3157 (1992).
*/

#include <standard.h>


static int ph1[4] = {1,3,2,0},
           ph2[8] = {1,1,2,2,3,3,0,0},
           ph3[4] = {1,3,2,0};

void pulsesequence()
{
   double          arraydim,
                   ss,
		   hsgpwr,
                   slpwr,
                   slpw,
                   mix,
                   phase,
                   cycles;
   int             iphase;
   char            sspul[MAXSTR],
                   satflg[MAXSTR];


/* LOAD AND INITIALIZE PARAMETERS */
   arraydim = getval("arraydim");
   mix = getval("mix");
   hsgpwr = getval("hsgpwr");
   phase = getval("phase");
   iphase = (int) (phase + 0.5);
   slpwr = getval("slpwr");
   slpw = getval("slpw");
   ss = getval("ss");
   getstr("sspul", sspul);
   getstr("satflg",satflg);
  

/* DETERMINE STEADY-STATE MODE */
   if (ss < 0)
      ss = (-1) * ss;
   else
    {
      if ((ss > 0) && (ix == 1))
	 ss = ss;
      else
	 ss = 0;
    }
   initval(ss, ssctr);
   initval(ss, ssval);


/* STEADY-STATE PHASECYCLING */
/* This section determines if the phase calculations trigger off of (SS - SSCTR)
   or off of CT */

   ifzero(ssctr);
      assign(ct,v7);
   elsenz(ssctr);
      sub(ssval, ssctr, v7);	/* v7 = 0,...,ss-1 */
   endif(ssctr);

   settable(t1,4,ph1);
   settable(t2,8,ph2);
   settable(t3,4,ph3);

   getelem(t1,v7,v1);
   getelem(t2,v7,v2);
   add(v2,two,v3);
   getelem(t3,v7,oph);

   if (iphase == 2)
      incr(v1);			/* hypercomplex method */
   if (iphase == 3)
      initval((double) (ix-1)/(arraydim/ni), v14);	/* TPPI method */
      add(v1, v14, v1);

     initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v13);
     if ((iphase == 1) || (iphase == 2))
      {
       add(v1,v13,v1);
       add(oph,v13,oph);
      }

   cycles = mix / ((4.0 * slpw) + 4.0e-6);
   initval(cycles, v10);	/* mixing time cycles */


/* BEGIN ACTUAL PULSE SEQUENCE */
   status(A);
      obspower(tpwr);
      delay(5.0e-6);
   if (sspul[0] == 'y')
   {
      zgradpulse(hsgpwr,10.0e-3);
      rgpulse(pw,zero,rof1,rof1);
      zgradpulse(hsgpwr,10.0e-3);
   }

   hsdelay(d1);

   if (satflg[0] == 'y')
   {
     obspower(satpwr);
      if (satfrq != tof)
       obsoffset(satfrq);
      rgpulse(satdly,zero,rof1,rof2);
      if (satfrq != tof)
       obsoffset(tof);
      obspower(tpwr);
      delay(40e-6);
    }


   status(B);
      rgpulse(pw, v1, rof1, 1.0e-6);
      if (d2 > 0.0)
       delay(d2 - POWER_DELAY - (2*pw/PI) - 1.0e-6);
      else
       delay(d2);
      rcvroff();
      obspower(slpwr);

      if (cycles > 1.5000)
       {
		xmtron();
         starthardloop(v10);
		txphase(v2);
		delay(2*slpw);
		txphase(v3);
		delay(2*slpw);
         endhardloop();
		xmtroff();
       }

      obspower(tpwr);
      delay(rof2);
      rcvron();

   status(C);
}
