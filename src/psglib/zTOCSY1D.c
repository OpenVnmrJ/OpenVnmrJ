// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* zTOCSY.c - 1D DPFGSE-TOCSY experiment w/ Keeler's zero-quantum filter (ZQF)

	Reference:
		Thrippleton MJ and Keeler J, Elimination of zero-quantum interference in two-dimensional
		NMR spectra, Angew Chem Int Ed Engl 42(33) 3938-3941, 2003.

	Features included:
		Randomization of Magnetization prior to relaxation delay
			G-90-
			[selected by sspul flag]
		Solvent suppression during relaxation 
			[selected by satmode flag]
		
	Paramters:
		sspul :		selects magnetization randomization option
		mixT	:	TOCSY spinlock mixing time
		slpatT	:	TOCSY pattern [dipsi2, dipsi3]
		slpwrT	:	spin-lock power level
		slpwT	:	90 deg pulse width for spinlock
                selshapeA, selpwrA, selpwA, gzlvlA, gtA -
                        :       shape, power, pulse, level and time for
                                first PFG echo
                selshapeB, selpwrB, selpwB, gzlvlB, gtB -
                                shape, power, pulse, level and time for
                                2nd PFG echo
		selfrq	:	Selective frequency (for selective 180)
		gstab	:	recovery delay
		Gzqfilt   :	apply ZQF before and after isotropic mixTing
		zqfpat1,zqfpat2:   adiabatic sweep 180 shape files
		zqfpw1,zqfpw2:     adiabatic sweep 180 pulse width
		zqfpwr1,zqfpwr2:   adiabatic sweep 180 pulse power
		gzlvlzq1,gzlvlzq2: gradient levels for ZQFs
		gzlvl1,gzlvl2:     gradient levels for crusher gradients
                gt1,gt2:           gradient durations for the crusher gradients

	Warning:
		For probes with very short RF coils, the calculated gradient levels for the ZQFs may cause
		the gradient amplifier to exceed its duty cycle!

************************************************************************
****NOTE:  v20,v21,v22,v23 and v24 are used by Hardware Loop and reserved ***
************************************************************************

Haitao Hu  -    Last revision   : May 2004
KrishK	-	Revised		: July 2004
KrishK  -       Includes slp saturation option : July 2005
KrishK - includes purge option : Aug. 2006
****v17,v18,v19 are reserved for PURGE ***
JohnR - includes CPMG option : Jan 2015
****v15 is reserved for CPMG ***

*/

#include <standard.h>
#include <chempack.h>

extern int dps_flag;

void pulsesequence()
{
   double	   slpwrT = getval("slpwrT"),
		   slpwT = getval("slpwT"),
		   mixT = getval("mixT"),
		   gzlvl1 = getval("gzlvl1"),
		   gt1 = getval("gt1"),
		   gzlvl2 = getval("gzlvl2"),
		   gt2 = getval("gt2"),
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
                   zqfpw1 = getval("zqfpw1"),
                   zqfpwr1 = getval("zqfpwr1"),
                   zqfpw2 = getval("zqfpw2"),
                   zqfpwr2 = getval("zqfpwr2"),
                   gzlvlzq1 = getval("gzlvlzq1"),
                   gzlvlzq2 = getval("gzlvlzq2");
                   
   char		   slpatT[MAXSTR],
                   selshapeA[MAXSTR],
                   selshapeB[MAXSTR],
                   zqfpat1[MAXSTR],
                   zqfpat2[MAXSTR];
   int 		   prgcycle=(int)(getval("prgcycle")+0.5);

//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gtA = syncGradTime("gtA","gzlvlA",1.0);
        gzlvlA = syncGradLvl("gtA","gzlvlA",1.0);
        gtB = syncGradTime("gtB","gzlvlB",1.0);
        gzlvlB = syncGradLvl("gtB","gzlvlB",1.0);

   getstr("slpatT",slpatT);
   getstr("selshapeA",selshapeA);
   getstr("selshapeB",selshapeB);
   getstr("zqfpat1",zqfpat1);
   getstr("zqfpat2",zqfpat2);

   if (strcmp(slpatT,"mlev17c") &&
        strcmp(slpatT,"dipsi2") &&
        strcmp(slpatT,"dipsi3") &&
        strcmp(slpatT,"mlev17") &&
        strcmp(slpatT,"mlev16"))
        abort_message("SpinLock pattern %s not supported!.\n", slpatT);

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
	/* purge option does not adjust v13 during steady state*/
      sub(ssval, ssctr, v13); /* v13 = 0,...,ss-1 */
   endif(ssctr);

   mod4(v13,v1); /* v1 = 0 1 2 3 */
   hlv(v13,v13);
   hlv(v13,v13);
   mod4(v13,v11); /* v11 = 0000 1111 2222 3333 */
   dbl(v1,oph);
   add(v11,oph,oph);
   add(v11,oph,oph); /* oph = 2v1 + 2v11 */

/* CYCLOPS */
   hlv(v13,v13);
   hlv(v13,v14);
   add(v1,v14,v1);
   add(v11,v14,v11);
   add(oph,v14,oph);
   assign(v14,v21);
   add(one,v21,v21);
   add(two,v21,v12);

   assign(zero,v6);
  if (getflag("prgflg") && (satmode[0] == 'y'))
        assign(v14,v6);
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
                   shaped_purge(v14,v6,v18,v19);
           }
        else
           {
                satpulse(satdly,v6,rof1,rof1);
                if (getflag("prgflg"))
                   purge(v14,v6,v18,v19);
           }
     }
   else
        delay(d1);

   if (getflag("wet"))
     wet4(zero,one);

   status(B);

      if (getflag("cpmgflg"))
      {
        rgpulse(pw, v14, rof1, 0.0);
        cpmg(v14, v15);
      }
      else
        rgpulse(pw, v14, rof1, rof1);

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
        shaped_pulse(selshapeB,selpwB,v11,rof1,rof1);
        obspower(tpwr);
        zgradpulse(gzlvlB,gtB);
        delay(gstab);

      if (selfrq != tof)
        obsoffset(tof);

        rgpulse(pw, v14, rof1, rof1);
        if (getflag("Gzqfilt"))
        {
         obspower(zqfpwr1);
         rgradient('z',gzlvlzq1);
         delay(100.0e-6);
         shaped_pulse(zqfpat1,zqfpw1,v14,rof1,rof1);
         delay(100.0e-6);
         rgradient('z',0.0);
         delay(gstab);
        }
        obspower(slpwrT);

        zgradpulse(gzlvl1,gt1);
        delay(gstab);

	if (mixT > 0.0)
	{
          if (dps_flag)
          	rgpulse(mixT,v21,0.0,0.0);
          else
          	SpinLock(slpatT,mixT,slpwT,v21);
        }

        if (getflag("Gzqfilt"))
        {
         obspower(zqfpwr2);
         rgradient('z',gzlvlzq2);
         delay(100.0e-6);
         shaped_pulse(zqfpat2,zqfpw2,v14,rof1,rof1);
         delay(100.0e-6);
         rgradient('z',0.0);
         delay(gstab);
        }
        obspower(tpwr);

        zgradpulse(gzlvl2,gt2);
        delay(gstab);

	rgpulse(pw,v14,rof1,rof2);

   status(C);
}
