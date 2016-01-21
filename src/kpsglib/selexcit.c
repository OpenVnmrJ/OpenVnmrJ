// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
#ifndef LINT
#endif
/* 
 */
/*   selexcit - DPFGSE-1D experiment

	Paramters:
		sspul :         selects magnetization randomization option
                selshapeA, selpwrA, selpwA, gzlvlA, gtA -
                        :       shape, power, pulse, level and time for
                                first PFG echo
                selshapeB, selpwrB, selpwB, gzlvlB, gtB -
                                shape, power, pulse, level and time for
                                2nd PFG echo
		gstab	:	Gradient recovery delay
		selfrq  :       Selective frequency (for selective 180)

	selective excitation is based on double PFG spin-echo
		(aka excitation sculpting)
			Shaka, et.al  JACS, 117, 4199 (1995)

KrishK	-	Last revision	: June 1997
KrishK	-	Revised		: July 2004

 */

#include <standard.h>
#include <chempack.h>

static int	ph1[4] = {0,2,1,3},
		ph5[4] = {2,0,3,1};

pulsesequence()
{

   double	   selfrq = getval("selfrq"),
		   selpwrA = getval("selpwrA"),
		   selpwA = getval("selpwA"),
		   gzlvlA = getval("gzlvlA"),
		   gtA = getval("gtA"),
		   selpwrB = getval("selpwrB"),
		   selpwB = getval("selpwB"),
		   gzlvlB = getval("gzlvlB"),
		   gtB = getval("gtB"),
		   gstab = getval("gstab");

   char		   selshapeA[MAXSTR],
		   selshapeB[MAXSTR];

//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gtA = syncGradTime("gtA","gzlvlA",1.0);
        gzlvlA = syncGradLvl("gtA","gzlvlA",1.0);
        gtB = syncGradTime("gtB","gzlvlB",1.0);
        gzlvlB = syncGradLvl("gtB","gzlvlB",1.0);

   getstr("selshapeA",selshapeA);
   getstr("selshapeB",selshapeB);

   settable(t1,4,ph1);
   settable(t5,4,ph5);
   getelem(t1,ct,v1);
   getelem(t5,ct,v5);
   assign(v1,oph);

/* BEGIN THE ACTUAL PULSE SEQUENCE */
   status(A);
      obspower(tpwr);

   delay(5.0e-5);
   if (getflag("sspul"))
        steadystate();

   if (satmode[0] == 'y')
     {
        if ((d1-satdly) > 0.02)
                delay(d1-satdly);
        else
                delay(0.02);
        satpulse(satdly,v6,rof1,rof1);
     }
   else
        delay(d1);

   if (getflag("wet"))
     wet4(zero,one);


   status(B);
      rgpulse(pw, v1, rof1, rof2);
      if (selfrq != tof)
	obsoffset(selfrq);

        zgradpulse(gzlvlA,gtA);
        delay(gstab);
        obspower(selpwrA);
        shaped_pulse(selshapeA,selpwA,v1,rof1,rof1);
        obspower(tpwr);
        zgradpulse(gzlvlA,gtA);
        delay(gstab);

      if (selfrq != tof)
	delay(2*OFFSET_DELAY);

        zgradpulse(gzlvlB,gtB);
        delay(gstab);
        obspower(selpwrB);
        shaped_pulse(selshapeB,selpwB,v5,rof1,rof1);
        obspower(tpwr);
        zgradpulse(gzlvlB,gtB);
        delay(gstab);

      if (selfrq != tof)
	obsoffset(tof);

   status(C);
}

