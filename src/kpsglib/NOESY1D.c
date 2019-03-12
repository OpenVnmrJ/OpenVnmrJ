// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
#ifndef LINT
#endif
/* 
 */
/*  NOESY1D - DPFGSENOE experiment

	Paramters:
		sspul :		selects magnetization randomization option
		mixN	:	NOESY mixing time
                selshapeA, selpwrA, selpwA, gzlvlA, gtA -
                        :       shape, power, pulse, level and time for
                                first PFG echo
                selshapeB, selpwrB, selpwB, gzlvlB, gtB -
                                shape, power, pulse, level and time for
                                2nd PFG echo
                gstab   :       Gradient recovery delay
		selfrq	:	Selective frequency (for selective 180)
		sweeppw :	sech180 pulse width
		sweeppwr:	sech180 pulse power
		sweepshp:	sech180 pulse shape
		gzlvlC	:	Gradient level during mixing
		gtC	:	Gradient time during mixing

KrishK	-	Last revision	: June 1997
KrishK	-	Revised		: July 2004
HaitaoH	-	ZQ suppression  : Sept 2004
KrishK	-	Revised for CP3	: May 2005
*/



#include <standard.h>
#include <chempack.h>

void pulsesequence()
{
   double	   mixN = getval("mixN"),
		   gzlvlC = getval("gzlvlC"),
		   gtC = getval("gtC"),
		   gstab = getval("gstab"),
                   selpwrA = getval("selpwrA"),
                   selpwA = getval("selpwA"),
                   gzlvlA = getval("gzlvlA"),
                   gtA = getval("gtA"),
                   selpwrB = getval("selpwrB"),
                   selpwB = getval("selpwB"),
                   gzlvlB = getval("gzlvlB"),
                   gtB = getval("gtB"),
		   selfrq = getval("selfrq"),
		   zqfpw3 = getval("zqfpw3"),
		   zqfpwr3 = getval("zqfpwr3"),
		   gzlvlzq3 = getval("gzlvlzq3"),
		   mixNcorr,
		   sweeppw = getval("sweeppw"),
		   sweeppwr = getval("sweeppwr");
   char		   sweepshp[MAXSTR],
                   selshapeA[MAXSTR],
                   selshapeB[MAXSTR],
		   zqfpat3[MAXSTR];
//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gtC = syncGradTime("gtC","gzlvlC",1.0);
        gzlvlC = syncGradLvl("gtC","gzlvlC",1.0);
        gtA = syncGradTime("gtA","gzlvlA",1.0);
        gzlvlA = syncGradLvl("gtA","gzlvlA",1.0);
        gtB = syncGradTime("gtB","gzlvlB",1.0);
        gzlvlB = syncGradLvl("gtB","gzlvlB",1.0);

   getstr("sweepshp",sweepshp);
   getstr("selshapeA",selshapeA);
   getstr("selshapeB",selshapeB);
   getstr("zqfpat3",zqfpat3);

   mixNcorr=0.0;
   if (getflag("Gzqfilt"))
	mixNcorr=getval("zqfpw3");

   hlv(ct,v1); hlv(v1,v1); hlv(v1,v1); hlv(v1,v1); mod4(v1,v1);
   mod4(ct,v2); add(v1,v2,v2);
   hlv(ct,v3); hlv(v3,v3); mod4(v3,v3); add(v1,v3,v3); dbl(v3,v4);
   dbl(v2,oph); add(oph,v4,oph); add(oph,v1,oph);


/* BEGIN THE ACTUAL PULSE SEQUENCE */
   status(A);
   obspower(tpwr);
   delay(5.0e-5);

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
      if (selfrq != tof)
       obsoffset(selfrq);
      rgpulse(pw, v1, rof1, rof1);

      if (selfrq != tof)
        obsoffset(selfrq);

        zgradpulse(gzlvlA,gtA);
        delay(gstab);
        obspower(selpwrA);
        shaped_pulse(selshapeA,selpwA,v2,rof1,rof1);
        obspower(tpwr);
        zgradpulse(gzlvlA,gtA);
        delay(gstab);

      if (selfrq != tof)
        delay(2*OFFSET_DELAY);

        zgradpulse(gzlvlB,gtB);
        delay(gstab);
        obspower(selpwrB);
        shaped_pulse(selshapeB,selpwB,v3,rof1,rof1);
        obspower(tpwr);
        zgradpulse(gzlvlB,gtB);
        delay(gstab);

      if (selfrq != tof)
        obsoffset(tof);

      rgpulse(pw,v1,rof1,rof1);

        obspower(sweeppwr);
	delay(0.31*mixN);
	zgradpulse(gzlvlC,gtC);
	delay(gstab);
	shaped_pulse(sweepshp,sweeppw,zero,rof1,rof1);
	delay(gstab);
	zgradpulse(-gzlvlC,gtC);
	delay(0.49*mixN);
	zgradpulse(-gzlvlC,2*gtC);
	delay(gstab);
        if (getflag("Gzqfilt"))
           {
                obspower(zqfpwr3);
                rgradient('z',gzlvlzq3);
                delay(100.0e-6);
                shaped_pulse(zqfpat3,zqfpw3,zero,rof1,rof1);
                delay(100.0e-6);
                rgradient('z',0.0);
           }
        else
                shaped_pulse(sweepshp,sweeppw,zero,rof1,rof1);
        delay(gstab);
        zgradpulse(gzlvlC,2*gtC);
        delay(0.2*mixN);
        obspower(tpwr);

      rgpulse(pw,v1,rof1,rof2);

   status(C);
}
