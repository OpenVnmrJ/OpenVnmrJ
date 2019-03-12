// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
#ifndef LINT
#endif
/* 
 */
/*  ROESY1D - DPFGSE-ROESY experiment

	Features included:
		
	Paramters:
		sspul :		selects magnetization randomization option
		mixR	:	ROESY spinlock mixing time
		slpwrR	:	spin-lock power level
		slpatR  :	Spinlock pattern [cw or troesy]
                selshapeA, selpwrA, selpwA, gzlvlA, gtA -
                        :       shape, power, pulse, level and time for
                                first PFG echo
                selshapeB, selpwrB, selpwB, gzlvlB, gtB -
                                shape, power, pulse, level and time for
                                2nd PFG echo
                gstab   :       Gradient recovery delay
                selfrq  :       Selective frequency (for selective 180)

****NOTE:  v2,v3,v4,v5 and v9 are used by Hardware Loop and reserved ***
   v2 is spinlock axis for t-roesy
   v3 is spinlock axis for dante and cw roesy
KrishK	-	Last revision	: June 1997
KrishK	-	Revised		: July 2004

*/


#include <standard.h>
#include <chempack.h>
extern int dps_flag;

static int	ph1[8] = {0,2,0,2,1,3,1,3},
		ph2[8] = {2,2,0,0,1,1,3,3},
		ph4[8] = {0,2,0,2,1,3,1,3};

void pulsesequence()
{
   double	   slpwrR = getval("slpwrR"),
		   slpwR = getval("slpwR"),
		   mixR = getval("mixR"),
                   selpwrA = getval("selpwrA"),
                   selpwA = getval("selpwA"),
                   gzlvlA = getval("gzlvlA"),
                   gtA = getval("gtA"),
                   selpwrB = getval("selpwrB"),
                   selpwB = getval("selpwB"),
                   gzlvlB = getval("gzlvlB"),
                   gtB = getval("gtB"),
                   gstab = getval("gstab"),
		   selfrq = getval("selfrq");
   char            selshapeA[MAXSTR],
                   selshapeB[MAXSTR],
                   slpatR[MAXSTR];

//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gtA = syncGradTime("gtA","gzlvlA",1.0);
        gzlvlA = syncGradLvl("gtA","gzlvlA",1.0);
        gtB = syncGradTime("gtB","gzlvlB",1.0);
        gzlvlB = syncGradLvl("gtB","gzlvlB",1.0);

   getstr("slpatR",slpatR);
   getstr("selshapeA",selshapeA);
   getstr("selshapeB",selshapeB);

   if (strcmp(slpatR,"cw") &&
        strcmp(slpatR,"troesy") &&
        strcmp(slpatR,"dante"))
        abort_message("SpinLock pattern %s not supported!.\n", slpatR);

   settable(t1,8,ph1);
   settable(t4,8,ph4);
   settable(t2,8,ph2);

   getelem(t1,ct,v1);
   add(v1,two,v6);
   getelem(t4,ct,oph);
   getelem(t2,ct,v2);
   add(v2,one,v3);
   add(v2,two,v4);
   add(v2,three,v5);

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
        satpulse(satdly,zero,rof1,rof1);
     }
   else
        delay(d1);

   if (getflag("wet"))
     wet4(zero,one);

   status(B);
      rgpulse(pw, v1, rof1, rof1);
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
        shaped_pulse(selshapeB,selpwB,v6,rof1,rof1);
        obspower(slpwrR);
        zgradpulse(gzlvlB,gtB);
        delay(gstab);

      if (selfrq != tof)
        obsoffset(tof);

     if (mixR > 0.0)
      { 
        if (dps_flag)
          {
             if (!strcmp(slpatR,"troesy"))
                rgpulse(mixR,v2,0.0,0.0);
             else
                rgpulse(mixR,v3,0.0,0.0);
          }
        else
          SpinLock(slpatR,mixR,slpwR,v2,v3,v4,v5,v9);
      }

	delay(rof2);

   status(C);
}
