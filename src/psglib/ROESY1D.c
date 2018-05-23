// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/*  ROESY1D - DPFGSE-ROESY experiment w/ optional ZQ-suppression

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
                Gzqfilt :	apply ZQF flag
                zqfpat1 :	adiabatic sweep 180 shape files
                zqfpw1  :	adiabatic sweep 180 pulse width
                zqfpwr1 :	adiabatic sweep 180 pulse power
                gzlvlzq1:	gradient levels for ZQFs
                gzlvl1,gzlvl2:     gradient levels for crusher gradients
                gt1,gt2:           gradient durations for the crusher gradients


****NOTE:  v20,v21,v22,v23 and v24 are used by Hardware Loop and reserved ***

KrishK	-	Last revision	: June 1997
KrishK	-	Revised		: July 2004
Haitao Hu -	Revised	ZQ suppression added	: July 2005
KrishK -	Included slp saturation option: Sept 2005
KrishK - includes purge option : Aug. 2006
****v17,v18,v19 are reserved for PURGE ***
JohnR - includes CPMG option : Jan 2015
****v15 is reserved for CPMG ***

*/


#include <standard.h>
#include <chempack.h>

extern int dps_flag;

pulsesequence()
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
		   selfrq = getval("selfrq"),
                   gzlvl1 = getval("gzlvl1"),
                   gt1 = getval("gt1"),
                   gzlvl2 = getval("gzlvl2"),
                   gt2 = getval("gt2"),
                   zqfpw1 = getval("zqfpw1"),
                   zqfpwr1 = getval("zqfpwr1"),
                   gzlvlzq1 = getval("gzlvlzq1");

   char            selshapeA[MAXSTR],
                   selshapeB[MAXSTR],
                   slpatR[MAXSTR],
                   zqfpat1[MAXSTR];
   int 		   prgcycle=(int)(getval("prgcycle")+0.5);

//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gtA = syncGradTime("gtA","gzlvlA",1.0);
        gzlvlA = syncGradLvl("gtA","gzlvlA",1.0);
        gtB = syncGradTime("gtB","gzlvlB",1.0);
        gzlvlB = syncGradLvl("gtB","gzlvlB",1.0);

   getstr("slpatR",slpatR);
   getstr("selshapeA",selshapeA);
   getstr("selshapeB",selshapeB);
   getstr("zqfpat1",zqfpat1);

   if (strcmp(slpatR,"cw") &&
        strcmp(slpatR,"troesy") &&
        strcmp(slpatR,"dante"))
        abort_message("SpinLock pattern %s not supported!.\n", slpatR);

/* STEADY-STATE PHASECYCLING */
/* This section determines if the phase calculations trigger off of (SS - SSCTR) or off of CT */

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

   ifzero(ssctr);
      assign(v17,v13);
   elsenz(ssctr);
		/* purge option does not adjust v13 during steady state */
      sub(ssval, ssctr, v13);
   endif(ssctr);

/* Beginning phase cycling */

   dbl(v13,v1);		/* v1 = 0 2 */
   hlv(v13,v13);
   dbl(v13,v20);		/* v20 = 00 22 */
   hlv(v13,v13);
   dbl(v13,v6);		/* v6 = 0000 2222 */
   hlv(v13,v13);
   dbl(v13,v7);		/* v7 = 00000000 22222222 */

   assign(v1,oph);

   if (getflag("Gzqfilt"))
      add(v7,oph,oph);

/* CYCLOPS */

   assign(v13,v14);	/* v14 = 8x0 8x1 8x2 8x3 */
   
   if (getflag("Gzqfilt"))
      hlv(v13,v14);	/* v14 = 16x0 16x1 16x2 16x3 */

   add(v1,v14,v1);      
   add(v20,v14,v20);      
   add(v6,v14,v6);      
   add(v7,v14,v7);      
   add(oph,v14,oph);

  add(oph,v18,oph);
  add(oph,v19,oph);
  assign(zero,v9);

  if (getflag("prgflg") && (satmode[0] == 'y'))
        assign(v1,v9);
 
   if (!strcmp(slpatR,"troesy")) 
	assign(v20,v21);
   else
	add(v20,one,v21);

/* BEGIN THE ACTUAL PULSE SEQUENCE */
   status(A);


/*   lk_sample(); */

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
                shaped_satpulse("relaxD",satdly,v9);
                if (getflag("prgflg"))
                   shaped_purge(v1,v9,v18,v19);
           }
        else
           {
                satpulse(satdly,v9,rof1,rof1);
                if (getflag("prgflg"))
                   purge(v1,v9,v18,v19);
           }
     }
   else
        delay(d1);

   if (getflag("wet"))
     wet4(zero,one);

/*   lk_hold(); */

   status(B);
      if (getflag("cpmgflg"))
      {
        rgpulse(pw, v1, rof1, 0.0);
        cpmg(v1, v15);
      }
      else
        rgpulse(pw, v1, rof1, rof1);
      if (selfrq != tof)
	obsoffset(selfrq);

        zgradpulse(gzlvlA,gtA);
        delay(gstab);
        obspower(selpwrA);
        shaped_pulse(selshapeA,selpwA,v14,rof1,rof1);
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
		rgpulse(mixR,v21,0.0,0.0);
	  else
		SpinLock(slpatR,mixR,slpwR,v21);
      }

    if (getflag("Gzqfilt"))
    {
     obspower(tpwr);
     rgpulse(pw,v7,rof1,rof1);

     zgradpulse(gzlvl1,gt1);
     delay(gstab);

     obspower(zqfpwr1);
     rgradient('z',gzlvlzq1);
     delay(100.0e-6);
     shaped_pulse(zqfpat1,zqfpw1,zero,rof1,rof1);
     delay(100.0e-6);
     rgradient('z',0.0);
     delay(gstab);
    
     zgradpulse(-gzlvl2,gt2);
     delay(gstab);

     obspower(tpwr);
     rgpulse(pw,v14,rof1,rof2);
    }
    else
     delay(rof2);

   status(C);
}

