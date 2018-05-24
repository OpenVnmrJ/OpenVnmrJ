// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

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
KrishK  -       Includes slp saturation option : July 2005
KrishK - includes purge option : Aug. 2006
****v17,v18,v19 are reserved for PURGE ***
JohnR - includes CPMG option : Jan 2015
****v15 is reserved for CPMG ***

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
   int 		   prgcycle=(int)(getval("prgcycle")+0.5);
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

  assign(ct,v17);
  assign(zero,v18);
  assign(zero,v19);

  if (getflag("prgflg") && (satmode[0] == 'y') && (prgcycle > 1.5))
    {
        hlv(ct,v17);
        mod2(ct,v18); dbl(v18,v18);
        if (prgcycle > 2.5)
           {
                hlv(v17,v17);
                hlv(ct,v19); mod2(v19,v19); dbl(v19,v19);
           }
     }

   settable(t1,4,ph1);
   settable(t5,4,ph5);
   getelem(t1,v17,v1);
   getelem(t5,v17,v5);
   assign(v1,oph);

   assign(v1,v6);
   add(oph,v18,oph);
   add(oph,v19,oph);

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
        if (getflag("slpsat"))
           {
                shaped_satpulse("relaxD",satdly,v6);
                if (getflag("prgflg"))
                   shaped_purge(v1,v6,v18,v19);
           }
        else
           {
                satpulse(satdly,v6,rof1,rof1);
                if (getflag("prgflg"))
                   purge(v1,v6,v18,v19);
           }
     }
   else
        delay(d1);

   if (getflag("wet"))
     wet4(zero,one);


   status(B);
      if (getflag("cpmgflg"))
      {
        rgpulse(pw, v1, rof1, 0.0);
        cpmg(v1, v15);
      }
      else
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

